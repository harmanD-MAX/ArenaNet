#pragma once

#include <memory>
#include "../network/Connection.h"
#include "../network/Packet.h"

namespace arenanet {
namespace friend_system {

class FriendManager {
public:
    static FriendManager& getInstance() {
        static FriendManager instance;
        return instance;
    }

    void handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet);

    void registerConnection(common::PlayerId playerId, std::shared_ptr<network::Connection> conn);
    void unregisterConnection(common::PlayerId playerId);

private:
    FriendManager() = default;

    void handleFriendRequest(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleFriendAccept(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleFriendReject(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleFriendRemove(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleGetFriendsList(std::shared_ptr<network::Connection> conn, const network::Packet& packet);

    std::mutex mutex_;
    std::unordered_map<common::PlayerId, std::shared_ptr<network::Connection>> connections_;
};

} // namespace friend_system
} // namespace arenanet
