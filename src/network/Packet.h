#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "../common/Types.h"

namespace arenanet {
namespace network {

enum class PacketType {
    // Auth
    LOGIN_REQUEST = 1,
    LOGIN_RESPONSE = 2,
    REGISTER_REQUEST = 3,
    REGISTER_RESPONSE = 4,

    // Lobby
    CREATE_LOBBY_REQUEST = 10,
    CREATE_LOBBY_RESPONSE = 11,
    JOIN_LOBBY_REQUEST = 12,
    JOIN_LOBBY_RESPONSE = 13,
    LEAVE_LOBBY_REQUEST = 14,
    LOBBY_STATE_UPDATE = 15,

    // Matchmaking
    JOIN_QUEUE_REQUEST = 20,
    JOIN_QUEUE_RESPONSE = 21,
    LEAVE_QUEUE_REQUEST = 22,
    MATCH_FOUND_EVENT = 23,

    // Chat
    CHAT_MESSAGE = 30,

    // Leaderboard
    GET_LEADERBOARD_REQUEST = 40,
    GET_LEADERBOARD_RESPONSE = 41
};

struct Packet {
    PacketType type;
    nlohmann::json payload;

    std::string serialize() const {
        nlohmann::json j;
        j["type"] = static_cast<int>(type);
        j["payload"] = payload;
        return j.dump();
    }

    static Packet deserialize(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        Packet p;
        p.type = static_cast<PacketType>(j["type"].get<int>());
        p.payload = j["payload"];
        return p;
    }
};

} // namespace network
} // namespace arenanet
