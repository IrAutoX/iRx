#pragma once

void InitDiscordRPC();

void UpdateDiscordRPC(const char* details, const char* state);

void UpdateMenuRPC(const char* player);

void UpdateGameRPC(const char* player, const char* weapon, const char* server);

void DiscordCallbacks();

void ShutdownDiscordRPC();

extern bool discordInGame;

bool IsDiscordInGame();

const char* currentserver(int i);