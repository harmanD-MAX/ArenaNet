#include "FriendManager.h"
#include "../database/PostgresClient.h"
#include "../redis/RedisClient.h"
#include "../common/Logger.h"
#include "../network/SessionManager.h"

namespace arenanet {
namespace friend_system {

void FriendManager::handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (conn->getPlayerId() == 0) {
        common::Logger::warning("Unauthenticated user attempted friend action.");
        return;
    }

    switch (packet.type) {
        case network::PacketType::FRIEND_REQUEST:
            handleFriendRequest(conn, packet);
            break;
        case network::PacketType::FRIEND_ACCEPT:
            handleFriendAccept(conn, packet);
            break;
        case network::PacketType::FRIEND_REJECT:
            handleFriendReject(conn, packet);
            break;
        case network::PacketType::FRIEND_REMOVE:
            handleFriendRemove(conn, packet);
            break;
        case network::PacketType::GET_FRIENDS_LIST_REQUEST:
            handleGetFriendsList(conn, packet);
            break;
        default:
            break;
    }
}

void FriendManager::handleFriendRequest(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    try {
        common::PlayerId targetId = packet.payload["target_id"];
        database::PostgresClient::getInstance().sendFriendRequest(conn->getPlayerId(), targetId);
        common::Logger::info("Player " + std::to_string(conn->getPlayerId()) + " sent friend request to " + std::to_string(targetId));
        
        // Send notification if online
        auto targetConn = network::SessionManager::getInstance().getConnection(targetId);
        if (targetConn) {
            network::Packet notify;
            notify.type = network::PacketType::NOTIFICATION_EVENT;
            notify.payload = {
                {"type", "FRIEND_REQUEST"},
                {"sender_id", conn->getPlayerId()}
            };
            targetConn->send(notify);
        }
    } catch (const std::exception& e) {
        common::Logger::error("Error handling friend request: " + std::string(e.what()));
    }
}

void FriendManager::handleFriendAccept(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    try {
        common::PlayerId senderId = packet.payload["sender_id"];
        database::PostgresClient::getInstance().acceptFriendRequest(conn->getPlayerId(), senderId);
        common::Logger::info("Player " + std::to_string(conn->getPlayerId()) + " accepted friend request from " + std::to_string(senderId));
    } catch (const std::exception& e) {
        common::Logger::error("Error handling friend accept: " + std::string(e.what()));
    }
}

void FriendManager::handleFriendReject(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
     try {
        common::PlayerId senderId = packet.payload["sender_id"];
        database::PostgresClient::getInstance().rejectFriendRequest(conn->getPlayerId(), senderId);
        common::Logger::info("Player " + std::to_string(conn->getPlayerId()) + " rejected friend request from " + std::to_string(senderId));
    } catch (const std::exception& e) {
        common::Logger::error("Error handling friend reject: " + std::string(e.what()));
    }
}

void FriendManager::handleFriendRemove(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    try {
        common::PlayerId friendId = packet.payload["friend_id"];
        database::PostgresClient::getInstance().removeFriend(conn->getPlayerId(), friendId);
        common::Logger::info("Player " + std::to_string(conn->getPlayerId()) + " removed friend " + std::to_string(friendId));
    } catch (const std::exception& e) {
        common::Logger::error("Error handling friend remove: " + std::string(e.what()));
    }
}

void FriendManager::handleGetFriendsList(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    try {
        auto friends = database::PostgresClient::getInstance().getFriendsList(conn->getPlayerId());
        
        // Fetch presence
        std::vector<common::PlayerId> fIds;
        for (const auto& f : friends) {
            fIds.push_back(f.id);
        }
        
        auto presences = redis::RedisClient::getInstance().getMultiplePlayerPresence(fIds);
        
        nlohmann::json jFriends = nlohmann::json::array();
        for (size_t i = 0; i < friends.size(); ++i) {
            friends[i].presence = presences[i];
            jFriends.push_back({
                {"id", friends[i].id},
                {"username", friends[i].username},
                {"status", friends[i].status},
                {"presence", friends[i].presence}
            });
        }
        
        network::Packet response;
        response.type = network::PacketType::GET_FRIENDS_LIST_RESPONSE;
        response.payload["friends"] = jFriends;
        
        conn->send(response);
    } catch (const std::exception& e) {
        common::Logger::error("Error fetching friends list: " + std::string(e.what()));
    }
}

} // namespace friend_system
} // namespace arenanet
