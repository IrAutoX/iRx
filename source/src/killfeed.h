#ifndef I_RX_KILLFEED_H
#define I_RX_KILLFEED_H

#include "cube.h"

struct killfeedentry
{
    string killer;
    string victim;
    int weapon;
    bool headshot;
    int millis;
};

extern void killfeed_add(const char *killer, const char *victim, int weapon, bool headshot);
extern void killfeed_render();
extern void killfeed_clear();

#endif
