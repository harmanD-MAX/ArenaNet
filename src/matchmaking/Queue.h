#pragma once

#include <vector>
#include <mutex>
#include "../common/Types.h"

namespace arenanet {
namespace matchmaking {

struct QueueEntry {
    common::PlayerId playerId;
    int rating;
    long long joinTimeMs;
};

class Queue {
public:
    void addPlayer(common::PlayerId playerId, int rating, long long timestamp);
    void removePlayer(common::PlayerId playerId);
    
    // Simple matchmaking: just find first N players
    std::vector<QueueEntry> getMatch(int playersNeeded);

private:
    std::mutex mutex_;
    std::vector<QueueEntry> entries_;
};

} // namespace matchmaking
} // namespace arenanet
