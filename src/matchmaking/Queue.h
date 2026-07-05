#pragma once

#include <vector>
#include <mutex>
#include "../common/Types.h"

namespace arenanet {
namespace matchmaking {

struct QueueEntry {
    std::string partyId; // Empty string if solo
    std::vector<common::PlayerId> members;
    int averageRating;
    long long joinTimeMs;
    int targetMatchSize;
};

class Queue {
public:
    void addEntry(const QueueEntry& entry);
    void removePlayer(common::PlayerId playerId); // Removes the entire entry that contains this player
    
    // Rating-based matchmaking
    // Iterates the queue and groups entries by gameMode. Populates matched and timed-out entries.
    void getMatch(long long currentTimeMs, std::vector<QueueEntry>& outMatch, std::vector<QueueEntry>& outTimedOut);

private:
    std::mutex mutex_;
    std::vector<QueueEntry> entries_;
};

} // namespace matchmaking
} // namespace arenanet
