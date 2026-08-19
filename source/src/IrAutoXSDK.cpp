#include "IrAutoXSDK.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <shellapi.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

namespace {

std::mutex g_mutex;
std::string g_error;
std::string g_gameName;
std::uint64_t g_gameId = 0;
std::uint16_t g_port = 6769;
bool g_autoStart = true;

#ifdef _WIN32
SOCKET g_socket = INVALID_SOCKET;
bool g_wsaReady = false;
#endif

std::string escapeJson(const std::string &value)
{
    std::string out;
    out.reserve(value.size() + 8);
    for (const char c : value) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

bool jsonBool(const std::string &json, const std::string &key, bool fallback = false)
{
    const std::string needle = "\"" + key + "\":";
    const auto pos = json.find(needle);
    if (pos == std::string::npos)
        return fallback;
    const auto start = pos + needle.size();
    return json.compare(start, 4, "true") == 0 || json.compare(start, 1, "1") == 0;
}

std::uint64_t jsonUInt64(const std::string &json, const std::string &key)
{
    const std::string needle = "\"" + key + "\":";
    const auto pos = json.find(needle);
    if (pos == std::string::npos)
        return 0;
    const char *begin = json.c_str() + pos + needle.size();
    while (*begin == ' ' || *begin == '"')
        ++begin;
    return static_cast<std::uint64_t>(std::strtoull(begin, nullptr, 10));
}

std::string jsonString(const std::string &json, const std::string &key)
{
    const std::string needle = "\"" + key + "\":\"";
    const auto pos = json.find(needle);
    if (pos == std::string::npos)
        return {};
    std::string out;
    bool escaped = false;
    for (std::size_t i = pos + needle.size(); i < json.size(); ++i) {
        const char c = json[i];
        if (escaped) {
            if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == 't') out += '\t';
            else out += c;
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '"')
            break;
        out += c;
    }
    return out;
}

#ifdef _WIN32

void closeSocket()
{
    if (g_socket != INVALID_SOCKET) {
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
    }
}

bool ensureWinsock()
{
    if (g_wsaReady)
        return true;
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        g_error = "WSAStartup failed";
        return false;
    }
    g_wsaReady = true;
    return true;
}

bool connectLocal()
{
    if (!ensureWinsock())
        return false;
    closeSocket();
    g_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_socket == INVALID_SOCKET) {
        g_error = "socket failed";
        return false;
    }
    DWORD timeout = 2200;
    setsockopt(g_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
    setsockopt(g_socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(g_port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    if (connect(g_socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == SOCKET_ERROR) {
        g_error = "IrAutoX Launcher SDK bridge is not reachable";
        closeSocket();
        return false;
    }
    return true;
}

std::wstring registryInstallDir()
{
    wchar_t buffer[32768]{};
    DWORD size = sizeof(buffer);
    const LONG result = RegGetValueW(HKEY_CURRENT_USER, L"Software\\IrAutoX\\Launcher", L"InstallDir",
                                     RRF_RT_REG_SZ, nullptr, buffer, &size);
    if (result != ERROR_SUCCESS)
        return {};
    return buffer;
}

bool startLauncher()
{
    const std::wstring dir = registryInstallDir();
    if (dir.empty()) {
        g_error = "IrAutoX Launcher install path was not found in registry";
        return false;
    }
    std::wstring executable = dir;
    if (!executable.empty() && executable.back() != L'\\')
        executable += L'\\';
    executable += L"IrAutoXLauncher.exe";
    const HINSTANCE result = ShellExecuteW(nullptr, L"open", executable.c_str(), L"--sdk-wake", dir.c_str(), SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        g_error = "Unable to start IrAutoX Launcher";
        return false;
    }
    return true;
}

bool sendAll(const std::string &data)
{
    const char *cursor = data.data();
    int remaining = static_cast<int>(data.size());
    while (remaining > 0) {
        const int sent = send(g_socket, cursor, remaining, 0);
        if (sent == SOCKET_ERROR || sent == 0) {
            g_error = "SDK send failed";
            closeSocket();
            return false;
        }
        cursor += sent;
        remaining -= sent;
    }
    return true;
}

bool receiveLine(std::string &line)
{
    line.clear();
    char buffer[1024];
    while (line.size() < 65536) {
        const int count = recv(g_socket, buffer, sizeof(buffer), 0);
        if (count <= 0) {
            g_error = "SDK response timeout or connection closed";
            closeSocket();
            return false;
        }
        line.append(buffer, buffer + count);
        const auto newline = line.find('\n');
        if (newline != std::string::npos) {
            line.resize(newline);
            return true;
        }
    }
    g_error = "SDK response was too large";
    return false;
}

bool exchange(const std::string &request, std::string *response = nullptr)
{
    if (g_socket == INVALID_SOCKET) {
        g_error = "SDK is not connected";
        return false;
    }
    if (!sendAll(request + "\n"))
        return false;
    std::string reply;
    if (!receiveLine(reply))
        return false;
    if (!jsonBool(reply, "ok")) {
        g_error = jsonString(reply, "error");
        if (g_error.empty())
            g_error = "Launcher rejected SDK request";
        return false;
    }
    if (response)
        *response = reply;
    return true;
}

bool hello()
{
    std::ostringstream json;
    json << "{\"cmd\":\"hello\",\"game_id\":" << g_gameId
         << ",\"game_name\":\"" << escapeJson(g_gameName) << "\",\"sdk_version\":\"2.0.2\"}";
    return exchange(json.str());
}

bool connectAndHello()
{
    if (!connectLocal())
        return false;
    if (!hello()) {
        closeSocket();
        return false;
    }
    return true;
}

bool ensureConnection()
{
    if (g_socket != INVALID_SOCKET)
        return true;
    if (connectAndHello())
        return true;
    if (!g_autoStart || !startLauncher())
        return false;
    for (int i = 0; i < 24; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        if (connectAndHello())
            return true;
    }
    g_error = "IrAutoX Launcher started but SDK bridge did not become ready";
    return false;
}

#endif

}

extern "C" {

int irautox_initialize(const IrAutoXInitOptions *options)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!options || options->game_id == 0) {
        g_error = "game_id must be greater than zero";
        return 0;
    }
    g_gameId = options->game_id;
    g_gameName = options->game_name ? options->game_name : "";
    g_port = options->port == 0 ? 6769 : options->port;
    g_autoStart = options->auto_start_launcher != 0;
#ifdef _WIN32
    return ensureConnection() ? 1 : 0;
#else
    g_error = "This SDK build currently supports Windows";
    return 0;
#endif
}

int irautox_set_presence(const IrAutoXPresence *presence)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!presence) {
        g_error = "presence is null";
        return 0;
    }
#ifdef _WIN32
    if (!ensureConnection())
        return 0;
    std::ostringstream json;
    json << "{\"cmd\":\"presence\",\"game_id\":" << g_gameId
         << ",\"playing\":" << (presence->playing ? "true" : "false")
         << ",\"details\":\"" << escapeJson(presence->details ? presence->details : "")
         << "\",\"state\":\"" << escapeJson(presence->state ? presence->state : "")
         << "\",\"party_size\":" << std::max(0, presence->party_size)
         << ",\"party_max\":" << std::max(std::max(0, presence->party_size), presence->party_max) << "}";
    return exchange(json.str()) ? 1 : 0;
#else
    return 0;
#endif
}

int irautox_clear_presence()
{
    IrAutoXPresence presence{};
    presence.playing = 0;
    return irautox_set_presence(&presence);
}

int irautox_heartbeat()
{
    std::lock_guard<std::mutex> lock(g_mutex);
#ifdef _WIN32
    if (!ensureConnection())
        return 0;
    return exchange("{\"cmd\":\"heartbeat\"}") ? 1 : 0;
#else
    return 0;
#endif
}

int irautox_get_user(IrAutoXUser *user)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!user) {
        g_error = "user is null";
        return 0;
    }
    std::memset(user, 0, sizeof(*user));
#ifdef _WIN32
    if (!ensureConnection())
        return 0;
    std::string response;
    if (!exchange("{\"cmd\":\"get_user\"}", &response))
        return 0;
    user->id = jsonUInt64(response, "user_id");
    const std::string username = jsonString(response, "username");
    const std::size_t length = std::min(username.size(), sizeof(user->username) - 1);
    std::memcpy(user->username, username.data(), length);
    user->username[length] = '\0';
    return user->id > 0 ? 1 : 0;
#else
    return 0;
#endif
}

int irautox_is_connected()
{
    std::lock_guard<std::mutex> lock(g_mutex);
#ifdef _WIN32
    return g_socket != INVALID_SOCKET ? 1 : 0;
#else
    return 0;
#endif
}

void irautox_shutdown()
{
    std::lock_guard<std::mutex> lock(g_mutex);
#ifdef _WIN32
    if (g_socket != INVALID_SOCKET) {
        exchange("{\"cmd\":\"shutdown\"}");
        closeSocket();
    }
#endif
    g_gameId = 0;
    g_gameName.clear();
}

const char *irautox_last_error()
{
    return g_error.c_str();
}

const char *irautox_sdk_version()
{
    return "2.0.2";
}

}
