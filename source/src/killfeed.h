#ifndef I_RX_KILLFEED_H
#define I_RX_KILLFEED_H
#include "cube.h"
void killfeed_add(const char *killer, const char *victim, int weapon, bool special);
void killfeed_render();
void killfeed_clear();
#endif
