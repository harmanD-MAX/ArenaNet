#pragma once

#include <sw/redis++/redis++.h>
#include <memory>
#include <string>
#include <vector>
#include "../common/Types.h"

namespace arenanet {
namespace redis {

class RedisClient {
public:
    static RedisClient& getInstance() {
        static RedisClient instance;
        return instance;
    }

    void connect(const std::string& host, int port);

    // Lobby tracking
    void addPlayerToLobby(const common::LobbyId& lobbyId, common::PlayerId playerId);
    void removePlayerFromLobby(const common::LobbyId& lobbyId, common::PlayerId playerId);
    std::vector<common::PlayerId> getPlayersInLobby(const common::LobbyId& lobbyId);
    
    // Auth Token Management
    void storeToken(common::PlayerId playerId, const std::string& token, int expirationSeconds);
    bool validateToken(common::PlayerId playerId, const std::string& token);
    void invalidateToken(common::PlayerId playerId);

private:
    RedisClient() = default;

    std::unique_ptr<sw::redis::Redis> redis_;
};

} // namespace redis
} // namespace arenanet
