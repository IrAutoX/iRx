#include "discord_presence.h"
#include "discord/discord_rpc.h"

#include <time.h>
#include <string.h>
#include <stdio.h>

static time_t discordStartTime;
static const char* clientID = "1534574125790658722";
static bool discordStarted = false;

void InitDiscordRPC()
{
    DiscordEventHandlers handlers;
    memset(&handlers,0,sizeof(handlers));

    Discord_Initialize(
        clientID,
        &handlers,
        1,
        NULL
    );

    discordStartTime = time(NULL);
    discordStarted = true;
}

void UpdateDiscordRPC(const char* details,const char* state)
{
    if(!discordStarted) return;

    DiscordRichPresence rpc;
    memset(&rpc,0,sizeof(rpc));

    rpc.details = details;
    rpc.state = state;
    rpc.largeImageKey = "logo";
    rpc.largeImageText = "iRx Website - irautox.ir/iRx";
    rpc.startTimestamp = discordStartTime;

    Discord_UpdatePresence(&rpc);
}

void UpdateMenuRPC(const char* player)
{
    if(!discordStarted) return;

    DiscordRichPresence rpc;
    memset(&rpc,0,sizeof(rpc));

    static char playerText[128];

    snprintf(
        playerText,
        sizeof(playerText),
        "Player: %s",
        player
    );

    rpc.details = "Main Menu";
    rpc.state = playerText;
    rpc.largeImageKey = "logo";
    rpc.largeImageText = "iRx Website - irautox.ir/iRx";
    rpc.startTimestamp = discordStartTime;

    Discord_UpdatePresence(&rpc);
}
void UpdateGameRPC(const char* player,const char* weapon,const char* server)
{
    if(!discordStarted) return;

    DiscordRichPresence rpc;
    memset(&rpc,0,sizeof(rpc));

    static char detailsText[256];
    static char stateText[256];

    snprintf(
        detailsText,
        sizeof(detailsText),
        "Server: %s",
        (server && server[0]) ? server : "Unknown"
    );

    snprintf(
        stateText,
        sizeof(stateText),
        "Player: %s | \nWeapon: %s",
        player,
        weapon
    );

    rpc.details = detailsText;
    rpc.state = stateText;

    rpc.largeImageKey = "logo";
    rpc.largeImageText = "iRx Website - irautox.ir/iRx";

    rpc.startTimestamp = discordStartTime;

    Discord_UpdatePresence(&rpc);
}

void DiscordCallbacks()
{
    if(!discordStarted) return;

    Discord_RunCallbacks();
}

void ShutdownDiscordRPC()
{
    if(!discordStarted) return;

    Discord_Shutdown();
    discordStarted = false;
}