#pragma once

#include <cstdint>
#include <string>

#if defined(_WIN32) && !defined(IRAUTOX_SDK_STATIC)
#  ifdef IRAUTOX_SDK_BUILD
#    define IRAUTOX_SDK_API __declspec(dllexport)
#  else
#    define IRAUTOX_SDK_API __declspec(dllimport)
#  endif
#else
#  define IRAUTOX_SDK_API
#endif

extern "C" {

struct IrAutoXInitOptions {
    std::uint64_t game_id;
    const char *game_name;
    std::uint16_t port;
    int auto_start_launcher;
};

struct IrAutoXPresence {
    int playing;
    const char *details;
    const char *state;
    int party_size;
    int party_max;
};

struct IrAutoXUser {
    std::uint64_t id;
    char username[128];
};

int irautox_initialize(const IrAutoXInitOptions *options);
int irautox_set_presence(const IrAutoXPresence *presence);
int irautox_clear_presence();
int irautox_heartbeat();
int irautox_get_user(IrAutoXUser *user);
int irautox_is_connected();
void irautox_shutdown();
const char *irautox_last_error();
const char *irautox_sdk_version();

}

namespace IrAutoX {

struct Options {
    std::uint64_t gameId = 0;
    std::string gameName;
    std::uint16_t port = 6769;
    bool autoStartLauncher = true;
};

struct Presence {
    bool playing = true;
    std::string details;
    std::string state;
    int partySize = 0;
    int partyMax = 0;
};

struct User {
    std::uint64_t id = 0;
    std::string username;
};

class Client final {
public:
    Client() = default;

    ~Client()
    {
        shutdown();
    }

    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;

    bool initialize(const Options &options)
    {
        IrAutoXInitOptions raw;

        raw.game_id = options.gameId;
        raw.game_name = options.gameName.c_str();
        raw.port = options.port;
        raw.auto_start_launcher = options.autoStartLauncher ? 1 : 0;

        return irautox_initialize(&raw) != 0;
    }

    bool setPresence(const Presence &presence)
    {
        IrAutoXPresence raw;

        raw.playing = presence.playing ? 1 : 0;
        raw.details = presence.details.c_str();
        raw.state = presence.state.c_str();
        raw.party_size = presence.partySize;
        raw.party_max = presence.partyMax;

        return irautox_set_presence(&raw) != 0;
    }

    bool clearPresence()
    {
        return irautox_clear_presence() != 0;
    }

    bool heartbeat()
    {
        return irautox_heartbeat() != 0;
    }

    bool connected() const
    {
        return irautox_is_connected() != 0;
    }

    User currentUser() const
    {
        IrAutoXUser raw{};

        if(!irautox_get_user(&raw))
            return {};

        return User{
            raw.id,
            std::string(raw.username)
        };
    }

    std::string lastError() const
    {
        const char *error = irautox_last_error();

        if(!error)
            return {};

        return std::string(error);
    }

    std::string version() const
    {
        const char *ver = irautox_sdk_version();

        if(!ver)
            return {};

        return std::string(ver);
    }

    void shutdown()
    {
        irautox_shutdown();
    }
};

}