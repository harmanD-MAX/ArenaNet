#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "../common/Types.h"
#include "../network/Packet.h"

namespace arenanet {
namespace lobby {

class Lobby {
public:
    Lobby(common::LobbyId id, common::PlayerId ownerId, int maxPlayers = 4, bool isPrivate = false);

    common::LobbyId getId() const { return id_; }
    common::PlayerId getOwnerId() const { return ownerId_; }
    bool isPrivate() const { return isPrivate_; }
    void setPrivate(bool isPrivate);
    
    bool addPlayer(common::PlayerId playerId);
    bool removePlayer(common::PlayerId playerId);
    void setOwner(common::PlayerId newOwner);
    
    void setPlayerReady(common::PlayerId playerId, bool isReady);
    void setPlayerConnected(common::PlayerId playerId, bool isConnected);
    bool isEveryoneReady() const;
    
    std::vector<common::PlayerId> getPlayers() const;
    
    void addChatMessage(const std::string& msg);
    std::vector<std::string> getChatHistory() const;
    
    network::Packet getLobbyStatePacket() const;

private:
    common::LobbyId id_;
    common::PlayerId ownerId_;
    int maxPlayers_;
    bool isPrivate_;
    
    mutable std::mutex mutex_;
    std::vector<common::PlayerId> players_;
    std::unordered_map<common::PlayerId, bool> readyStates_;
    std::unordered_map<common::PlayerId, bool> connectedStates_;
    std::vector<std::string> chatHistory_;
};

} // namespace lobby
} // namespace arenanet
