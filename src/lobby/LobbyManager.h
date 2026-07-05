#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "Lobby.h"
#include "../network/Connection.h"
#include "../network/Packet.h"

namespace arenanet {
namespace lobby {

class LobbyManager {
public:
    static LobbyManager& getInstance() {
        static LobbyManager instance;
        return instance;
    }

    void handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    
    // Broadcast a packet to everyone in the lobby
    void broadcastToLobby(const common::LobbyId& lobbyId, const network::Packet& packet);

    void handlePlayerDisconnect(common::PlayerId playerId);
    void handlePlayerReconnect(common::PlayerId playerId, std::shared_ptr<network::Connection> conn);

private:
    LobbyManager() = default;

    void handleCreateLobby(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleJoinLobby(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleLeaveLobby(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleListLobbies(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handlePlayerReady(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleKickPlayer(std::shared_ptr<network::Connection> conn, const network::Packet& packet);

    std::string generateLobbyId();

    std::mutex mutex_;
    std::unordered_map<common::LobbyId, std::shared_ptr<Lobby>> lobbies_;
    std::unordered_map<common::PlayerId, std::shared_ptr<network::Connection>> playerConnections_; // Map players to connections
};

} // namespace lobby
} // namespace arenanet
