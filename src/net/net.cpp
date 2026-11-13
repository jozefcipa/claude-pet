#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <time.h>
#include "net/net.h"
#include "config.h"
#include "pet/speech.h"
#include "pet/state.h"
#include "usage/usage.h"
#include "secrets.h"

static WebServer server(80);
static uint32_t offlineSince = 0;
static uint32_t lastAttempt = 0;

static void handleState() {
  State s;
  if (!parseState(server.arg("s").c_str(), s)) {
    server.send(400, "text/plain", "unknown state\n");
    return;
  }
  petEvent(s, server.arg("tool").c_str(), server.arg("msg").c_str());
  server.send(200, "text/plain", "ok\n");
}

// Speech bubble only, the pet keeps doing whatever it was doing
static void handleSay() {
  String msg = server.arg("msg");
  if (msg.isEmpty()) {
    server.send(400, "text/plain", "missing msg\n");
    return;
  }
  say(msg.c_str(), millis(), server.arg("ms").toInt());
  server.send(200, "text/plain", "ok\n");
}

static void handleUsage() {
  usage.today = strtoull(server.arg("today").c_str(), nullptr, 10);
  usage.sessions = server.arg("sessions").toInt();
  usageSetWeek(server.arg("week").c_str());
  usage.has = true;
  server.send(200, "text/plain", "ok\n");
}

void netInit() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setHostname(HOSTNAME);
  if (netConfigured()) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    lastAttempt = millis();
  }
  configTzTime(TZ_INFO, "pool.ntp.org");

  server.on("/state", handleState);
  server.on("/usage", handleUsage);
  server.on("/say", handleSay);
  server.on("/", []() { server.send(200, "text/plain", String(stateName(pet.state)) + "\n"); });
  server.begin();
}

void netLoop() {
  static bool mdnsUp = false;
  uint32_t now = millis();
  if (netConnected()) {
    offlineSince = 0;
  } else if (netConfigured()) {
    if (!offlineSince) {
      offlineSince = now;
    }
    // auto-reconnect doesn't cover every failure (e.g. AP missing at boot), so retry ourselves too
    if (now - lastAttempt > WIFI_RETRY_MS) {
      lastAttempt = now;
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }
  if (!mdnsUp && netConnected()) {
    mdnsUp = MDNS.begin(HOSTNAME);
    MDNS.addService("http", "tcp", 80);
    Serial.printf("ready: http://%s.local  (%s)\n", HOSTNAME, WiFi.localIP().toString().c_str());
  }
  server.handleClient();
}

bool netConfigured() {
  return strlen(WIFI_SSID) > 0;
}

bool netConnected() {
  return WiFi.status() == WL_CONNECTED;
}

NetStatus netStatus(uint32_t now) {
  if (!netConfigured()) {
    return NET_DEMO;
  }
  if (netConnected()) {
    return NET_ONLINE;
  }
  return offlineSince && now - offlineSince > NO_WIFI_AFTER_MS ? NET_NO_WIFI : NET_CONNECTING;
}

void netPowerSave(bool on) {
  WiFi.setSleep(on);
}
