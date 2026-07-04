#include "Lobby.h"
#include <algorithm>

namespace arenanet {
namespace lobby {

Lobby::Lobby(common::LobbyId id, common::PlayerId ownerId, int maxPlayers)
    : id_(id), ownerId_(ownerId), maxPlayers_(maxPlayers) {
    players_.push_back(ownerId);
    readyStates_[ownerId] = false;
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
    return true;
}

bool Lobby::removePlayer(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(players_.begin(), players_.end(), playerId);
    if (it != players_.end()) {
        players_.erase(it);
        readyStates_.erase(playerId);
        // If owner leaves, maybe assign new owner or dissolve lobby
        // For MVP, we'll just keep the first player as new owner if any
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

bool Lobby::isEveryoneReady() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (players_.empty()) return false;
    for (const auto& p : players_) {
        if (!readyStates_.at(p)) return false;
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
    
    nlohmann::json playersArray = nlohmann::json::array();
    for (common::PlayerId pid : players_) {
        nlohmann::json pObj;
        pObj["player_id"] = pid;
        pObj["is_ready"] = readyStates_.at(pid);
        playersArray.push_back(pObj);
    }
    packet.payload["players"] = playersArray;
    
    return packet;
}

} // namespace lobby
} // namespace arenanet
