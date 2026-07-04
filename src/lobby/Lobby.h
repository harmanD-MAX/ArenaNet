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
    Lobby(common::LobbyId id, common::PlayerId ownerId, int maxPlayers = 4);

    common::LobbyId getId() const { return id_; }
    common::PlayerId getOwnerId() const { return ownerId_; }
    
    bool addPlayer(common::PlayerId playerId);
    bool removePlayer(common::PlayerId playerId);
    
    void setPlayerReady(common::PlayerId playerId, bool isReady);
    bool isEveryoneReady() const;
    
    std::vector<common::PlayerId> getPlayers() const;
    
    network::Packet getLobbyStatePacket() const;

private:
    common::LobbyId id_;
    common::PlayerId ownerId_;
    int maxPlayers_;
    
    mutable std::mutex mutex_;
    std::vector<common::PlayerId> players_;
    std::unordered_map<common::PlayerId, bool> readyStates_;
};

} // namespace lobby
} // namespace arenanet
