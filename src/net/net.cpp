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

void netPowerSave(bool on) {
  WiFi.setSleep(on);
}
