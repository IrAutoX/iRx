#include "cube.h"
#include "killfeed.h"

struct killfeedentry
{
    string killer;
    string victim;
    int weapon;
    bool special;
    int millis;
};

static vector<killfeedentry> entries;

void killfeed_add(const char *killer, const char *victim, int weapon, bool special)
{
    killfeedentry &e = entries.add();
    copystring(e.killer, killer ? killer : "World");
    copystring(e.victim, victim ? victim : "Unknown");
    e.weapon = weapon;
    e.special = special;
    e.millis = lastmillis;
    while(entries.length() > 6) entries.remove(0);
}

void killfeed_clear()
{
    entries.shrink(0);
}

void killfeed_render()
{
    if(!entries.length()) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, VIRTW, VIRTH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int y = 70;
    for(int i = entries.length()-1; i >= 0; --i)
    {
        killfeedentry &e = entries[i];
        int age = lastmillis - e.millis;
        if(age > 5000) continue;

        float fade = age > 4000 ? float(5000-age)/1000.0f : 1.0f;
        float t = min(1.0f, float(age)/220.0f);
        float scale = 0.82f + 0.18f*t;
        float slide = (1.0f-t)*35.0f;

        defformatstring(msg)("%s  >  %s%s", e.killer, e.victim, e.special ? "  *" : "");
        float tw = text_width(msg);
        float x = VIRTW - tw - 28.0f + slide;

        glPushMatrix();
        glTranslatef(x + tw*0.5f, y, 0);
        glScalef(scale, scale, 1.0f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.45f*fade);
        draw_text(msg, -tw*0.5f + 2, 2);
        glColor4f(1.0f, 1.0f, 1.0f, fade);
        draw_text(msg, -tw*0.5f, 0);
        glPopMatrix();
        y += FONTH + 10;
    }

    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

extern "C" void killfeed_hud_wrapper(int w, int h, int curfps, int nquads, int curvert, bool underwater, int elapsed) asm("__wrap__Z10gl_drawhudiiiiibi");
extern "C" void killfeed_hud_real(int w, int h, int curfps, int nquads, int curvert, bool underwater, int elapsed) asm("__real__Z10gl_drawhudiiiiibi");

extern "C" void killfeed_hud_wrapper(int w, int h, int curfps, int nquads, int curvert, bool underwater, int elapsed)
{
    killfeed_hud_real(w, h, curfps, nquads, curvert, underwater, elapsed);
    killfeed_render();
}

extern "C" void killfeed_dokill_wrapper(playerent *act, playerent *pl, int gun, bool gib) asm("__wrap__Z6dokillP10playerentS0_ib");
extern "C" void killfeed_dokill_real(playerent *act, playerent *pl, int gun, bool gib) asm("__real__Z6dokillP10playerentS0_ib");

extern "C" void killfeed_dokill_wrapper(playerent *act, playerent *pl, int gun, bool gib)
{
    killfeed_dokill_real(act, pl, gun, gib);
    killfeed_add(act ? act->name : "World", pl ? pl->name : "Unknown", gun, gib);
}
