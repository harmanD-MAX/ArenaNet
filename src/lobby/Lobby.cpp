#include "Lobby.h"
#include <algorithm>

namespace arenanet {
namespace lobby {

Lobby::Lobby(common::LobbyId id, common::PlayerId ownerId, int maxPlayers, bool isPrivate)
    : id_(id), ownerId_(ownerId), maxPlayers_(maxPlayers), isPrivate_(isPrivate) {
    players_.push_back(ownerId);
    readyStates_[ownerId] = false;
    connectedStates_[ownerId] = true;
}

void Lobby::setPrivate(bool isPrivate) {
    std::lock_guard<std::mutex> lock(mutex_);
    isPrivate_ = isPrivate;
}

void Lobby::setOwner(common::PlayerId newOwner) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::find(players_.begin(), players_.end(), newOwner) != players_.end()) {
        ownerId_ = newOwner;
    }
}

bool Lobby::addPlayer(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (players_.size() >= maxPlayers_) {
        return false;
    }
    if (std::find(players_.begin(), players_.end(), playerId) != players_.end()) {
        return false; // Already in lobby
    }
    players_.push_back(playerId);
    readyStates_[playerId] = false;
    connectedStates_[playerId] = true;
    return true;
}

bool Lobby::removePlayer(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(players_.begin(), players_.end(), playerId);
    if (it != players_.end()) {
        players_.erase(it);
        readyStates_.erase(playerId);
        connectedStates_.erase(playerId);
        if (playerId == ownerId_ && !players_.empty()) {
            ownerId_ = players_.front();
        }
        return true;
    }
    return false;
}

void Lobby::setPlayerReady(common::PlayerId playerId, bool isReady) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (readyStates_.find(playerId) != readyStates_.end()) {
        readyStates_[playerId] = isReady;
    }
}

void Lobby::setPlayerConnected(common::PlayerId playerId, bool isConnected) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connectedStates_.find(playerId) != connectedStates_.end()) {
        connectedStates_[playerId] = isConnected;
    }
}

void Lobby::addChatMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    chatHistory_.push_back(msg);
    if (chatHistory_.size() > 50) {
        chatHistory_.erase(chatHistory_.begin());
    }
}

std::vector<std::string> Lobby::getChatHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return chatHistory_;
}

bool Lobby::isEveryoneReady() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (players_.empty()) return false;
    for (const auto& p : players_) {
        // Must be ready AND connected to start match
        if (!readyStates_.at(p) || !connectedStates_.at(p)) return false;
    }
    return true;
}

std::vector<common::PlayerId> Lobby::getPlayers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return players_;
}

network::Packet Lobby::getLobbyStatePacket() const {
    std::lock_guard<std::mutex> lock(mutex_);
    network::Packet packet;
    packet.type = network::PacketType::LOBBY_STATE_UPDATE;
    
    packet.payload["lobby_id"] = id_;
    packet.payload["owner_id"] = ownerId_;
    packet.payload["max_players"] = maxPlayers_;
    packet.payload["is_private"] = isPrivate_;
    
    nlohmann::json playersArray = nlohmann::json::array();
    for (common::PlayerId pid : players_) {
        nlohmann::json pObj;
        pObj["player_id"] = pid;
        pObj["is_ready"] = readyStates_.at(pid);
        pObj["is_connected"] = connectedStates_.at(pid);
        playersArray.push_back(pObj);
    }
    packet.payload["players"] = playersArray;
    
    return packet;
}

} // namespace lobby
} // namespace arenanet
