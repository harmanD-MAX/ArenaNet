#include "LobbyManager.h"
#include "../common/Logger.h"
#include <random>

namespace arenanet {
namespace lobby {

void LobbyManager::handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (conn->getPlayerId() == 0) {
        common::Logger::warning("Unauthenticated player tried to perform lobby action.");
        return;
    }

    // Keep track of the active connection for this player
    {
        std::lock_guard<std::mutex> lock(mutex_);
        playerConnections_[conn->getPlayerId()] = conn;
    }

    switch (packet.type) {
        case network::PacketType::CREATE_LOBBY_REQUEST:
            handleCreateLobby(conn, packet);
            break;
        case network::PacketType::JOIN_LOBBY_REQUEST:
            handleJoinLobby(conn, packet);
            break;
        case network::PacketType::LEAVE_LOBBY_REQUEST:
            handleLeaveLobby(conn, packet);
            break;
        case network::PacketType::LIST_LOBBIES_REQUEST:
            handleListLobbies(conn, packet);
            break;
        case network::PacketType::PLAYER_READY_REQUEST:
            handlePlayerReady(conn, packet);
            break;
        case network::PacketType::KICK_PLAYER_REQUEST:
            handleKickPlayer(conn, packet);
            break;
        default:
            break;
    }
}

void LobbyManager::broadcastToLobby(const common::LobbyId& lobbyId, const network::Packet& packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = lobbies_.find(lobbyId);
    if (it != lobbies_.end()) {
        auto players = it->second->getPlayers();
        for (auto pid : players) {
            auto connIt = playerConnections_.find(pid);
            if (connIt != playerConnections_.end() && connIt->second) {
                connIt->second->send(packet);
            }
        }
    }
}

void LobbyManager::handleCreateLobby(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    std::string lobbyId = generateLobbyId();
    common::PlayerId ownerId = conn->getPlayerId();
    
    bool isPrivate = packet.payload.value("is_private", false);
    int maxCapacity = packet.payload.value("max_capacity", 4);

    auto newLobby = std::make_shared<Lobby>(lobbyId, ownerId, maxCapacity, isPrivate);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        lobbies_[lobbyId] = newLobby;
    }

    network::Packet response;
    response.type = network::PacketType::CREATE_LOBBY_RESPONSE;
    response.payload["success"] = true;
    response.payload["lobby_id"] = lobbyId;
    conn->send(response);
    
    // Broadcast state to the (only) player
    broadcastToLobby(lobbyId, newLobby->getLobbyStatePacket());
    
    common::Logger::info("Lobby created: " + lobbyId);
}

void LobbyManager::handleJoinLobby(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    std::string lobbyId = packet.payload["lobby_id"];
    common::PlayerId playerId = conn->getPlayerId();

    network::Packet response;
    response.type = network::PacketType::JOIN_LOBBY_RESPONSE;

    std::shared_ptr<Lobby> lobby;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = lobbies_.find(lobbyId);
        if (it != lobbies_.end()) {
            lobby = it->second;
        }
    }

    if (lobby && lobby->addPlayer(playerId)) {
        response.payload["success"] = true;
        response.payload["lobby_id"] = lobbyId;
        conn->send(response);
        
        // Notify all players in lobby of the state change
        broadcastToLobby(lobbyId, lobby->getLobbyStatePacket());
        common::Logger::info("Player " + std::to_string(playerId) + " joined lobby " + lobbyId);
    } else {
        response.payload["success"] = false;
        response.payload["error_msg"] = "Lobby not found or full.";
        conn->send(response);
    }
}

void LobbyManager::handleLeaveLobby(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    std::string lobbyId = packet.payload["lobby_id"];
    common::PlayerId playerId = conn->getPlayerId();

    std::shared_ptr<Lobby> lobby;
    bool isEmpty = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = lobbies_.find(lobbyId);
        if (it != lobbies_.end()) {
            lobby = it->second;
            if (lobby->removePlayer(playerId)) {
                if (lobby->getPlayers().empty()) {
                    lobbies_.erase(it);
                    isEmpty = true;
                }
            }
        }
    }

    if (lobby && !isEmpty) {
        broadcastToLobby(lobbyId, lobby->getLobbyStatePacket());
    }
    common::Logger::info("Player " + std::to_string(playerId) + " left lobby " + lobbyId);
}

std::string LobbyManager::generateLobbyId() {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    
    std::string id = "";
    for (int i = 0; i < 6; ++i) {
        id += charset[dist(rng)];
    }
    return id;
}

void LobbyManager::handleListLobbies(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    network::Packet response;
    response.type = network::PacketType::LIST_LOBBIES_RESPONSE;
    nlohmann::json lobbiesArr = nlohmann::json::array();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, lobby] : lobbies_) {
            if (lobby->isPrivate()) continue; // Skip private lobbies
            nlohmann::json l;
            l["lobby_id"] = id;
            l["owner_id"] = lobby->getOwnerId();
            l["player_count"] = lobby->getPlayers().size();
            l["max_capacity"] = lobby->getPlayers().size(); // Note: get actual max later if needed
            lobbiesArr.push_back(l);
        }
    }
    
    response.payload["lobbies"] = lobbiesArr;
    conn->send(response);
}

void LobbyManager::handlePlayerReady(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (!packet.payload.contains("lobby_id") || !packet.payload.contains("is_ready")) {
        return;
    }
    std::string lobbyId = packet.payload["lobby_id"];
    bool isReady = packet.payload["is_ready"];
    common::PlayerId playerId = conn->getPlayerId();
    
    std::shared_ptr<Lobby> lobby;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = lobbies_.find(lobbyId);
        if (it != lobbies_.end()) {
            lobby = it->second;
        }
    }
    
    if (lobby) {
        lobby->setPlayerReady(playerId, isReady);
        broadcastToLobby(lobbyId, lobby->getLobbyStatePacket());
        
        if (lobby->isEveryoneReady() && lobby->getPlayers().size() >= 2) {
            network::Packet matchStart;
            matchStart.type = network::PacketType::MATCH_FOUND_EVENT;
            matchStart.payload["match_id"] = lobbyId;
            
            nlohmann::json playersArr = nlohmann::json::array();
            for (auto pid : lobby->getPlayers()) {
                playersArr.push_back(pid);
            }
            matchStart.payload["players"] = playersArr;
            
            broadcastToLobby(lobbyId, matchStart);
            common::Logger::info("Lobby " + lobbyId + " is fully ready. Match starting!");
        }
    }
}

void LobbyManager::handleKickPlayer(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (!packet.payload.contains("lobby_id") || !packet.payload.contains("target_id")) return;
    
    std::string lobbyId = packet.payload["lobby_id"];
    common::PlayerId targetId = packet.payload["target_id"];
    common::PlayerId ownerId = conn->getPlayerId();
    
    std::shared_ptr<Lobby> lobby;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = lobbies_.find(lobbyId);
        if (it != lobbies_.end()) {
            lobby = it->second;
        }
    }
    
    if (lobby && lobby->getOwnerId() == ownerId) {
        if (lobby->removePlayer(targetId)) {
            broadcastToLobby(lobbyId, lobby->getLobbyStatePacket());
            common::Logger::info("Player " + std::to_string(targetId) + " kicked from lobby " + lobbyId);
        }
    }
}

void LobbyManager::handlePlayerDisconnect(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    playerConnections_.erase(playerId);
    
    for (auto& [id, lobby] : lobbies_) {
        auto players = lobby->getPlayers();
        if (std::find(players.begin(), players.end(), playerId) != players.end()) {
            lobby->setPlayerConnected(playerId, false);
            broadcastToLobby(id, lobby->getLobbyStatePacket());
            common::Logger::info("Player " + std::to_string(playerId) + " marked as disconnected in lobby " + id);
            
            // Spawn a detached thread to handle the timeout
            std::thread([this, id, playerId]() {
                std::this_thread::sleep_for(std::chrono::seconds(30)); // Reconnect timeout
                std::shared_ptr<Lobby> targetLobby;
                {
                    std::lock_guard<std::mutex> innerLock(mutex_);
                    auto it = lobbies_.find(id);
                    if (it != lobbies_.end()) targetLobby = it->second;
                }
                
                if (targetLobby) {
                    // If still not in playerConnections, they haven't reconnected
                    bool remove = false;
                    {
                        std::lock_guard<std::mutex> innerLock(mutex_);
                        if (playerConnections_.find(playerId) == playerConnections_.end()) {
                            remove = true;
                        }
                    }
                    if (remove && targetLobby->removePlayer(playerId)) {
                        broadcastToLobby(id, targetLobby->getLobbyStatePacket());
                        common::Logger::info("Player " + std::to_string(playerId) + " removed from lobby " + id + " due to timeout.");
                    }
                }
            }).detach();
        }
    }
}

void LobbyManager::handlePlayerReconnect(common::PlayerId playerId, std::shared_ptr<network::Connection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    playerConnections_[playerId] = conn;
    
    for (auto& [id, lobby] : lobbies_) {
        auto players = lobby->getPlayers();
        if (std::find(players.begin(), players.end(), playerId) != players.end()) {
            lobby->setPlayerConnected(playerId, true);
            conn->send(lobby->getLobbyStatePacket());
            broadcastToLobby(id, lobby->getLobbyStatePacket());
            common::Logger::info("Player " + std::to_string(playerId) + " reconnected to lobby " + id);
        }
    }
}

} // namespace lobby
} // namespace arenanet
