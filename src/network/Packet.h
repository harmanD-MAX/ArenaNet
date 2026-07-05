#pragma once

#include <string>
#include <nlohmann/json.hpp>


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
    LIST_LOBBIES_REQUEST = 16,
    LIST_LOBBIES_RESPONSE = 17,
    PLAYER_READY_REQUEST = 18,
    KICK_PLAYER_REQUEST = 19,

    // Matchmaking
    JOIN_QUEUE_REQUEST = 20,
    JOIN_QUEUE_RESPONSE = 21,
    LEAVE_QUEUE_REQUEST = 22,
    MATCH_FOUND_EVENT = 23,
    MATCH_TIMEOUT_EVENT = 24,

    // Chat
    CHAT_MESSAGE = 30,

    // Leaderboard
    GET_LEADERBOARD_REQUEST = 40,
    GET_LEADERBOARD_RESPONSE = 41,

    // Friends
    FRIEND_REQUEST = 50,
    FRIEND_ACCEPT = 51,
    FRIEND_REJECT = 52,
    FRIEND_REMOVE = 53,
    GET_FRIENDS_LIST_REQUEST = 54,
    GET_FRIENDS_LIST_RESPONSE = 55,

    // Parties
    CREATE_PARTY_REQUEST = 60,
    PARTY_INVITE = 61,
    PARTY_ACCEPT = 62,
    LEAVE_PARTY = 63,
    PARTY_STATE_UPDATE = 64,

    // Match History
    REPORT_MATCH_RESULT = 70,
    GET_MATCH_HISTORY_REQUEST = 71,
    GET_MATCH_HISTORY_RESPONSE = 72,

    // Notifications
    NOTIFICATION_EVENT = 80
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
