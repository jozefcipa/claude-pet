#pragma once
#include <stdint.h>

// Scenery behind the bot (laptop, block pile, mini-bots)
void drawPropsBehind(uint32_t now);
// Things held or in front of the bot (magnifier, juggling balls, carried block)
void drawPropsFront(uint32_t now);
// Bubbles, hearts, confetti, z's, attention border
void drawEffects(uint32_t now);
// Wi-Fi icon over the bot: filling up while searching, crossed out when there is none
void drawOfflineEffects(uint32_t now, bool searching);
