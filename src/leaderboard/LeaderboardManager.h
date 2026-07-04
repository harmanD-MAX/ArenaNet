#pragma once

#include <string>
#include <memory>
#include "../network/Connection.h"
#include "../network/Packet.h"

namespace arenanet {
namespace leaderboard {

class LeaderboardManager {
public:
    static LeaderboardManager& getInstance() {
        static LeaderboardManager instance;
        return instance;
    }

    void handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet);

private:
    LeaderboardManager() = default;

    void handleGetLeaderboard(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
};

} // namespace leaderboard
} // namespace arenanet
