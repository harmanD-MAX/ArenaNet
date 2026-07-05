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
};

class Queue {
public:
    void addEntry(const QueueEntry& entry);
    void removePlayer(common::PlayerId playerId); // Removes the entire entry that contains this player
    
    // Rating-based matchmaking
    // Returns a list of entries that form a match, or empty if no match found.
    std::vector<QueueEntry> getMatch(int targetTotalPlayers, long long currentTimeMs);

private:
    std::mutex mutex_;
    std::vector<QueueEntry> entries_;
};

} // namespace matchmaking
} // namespace arenanet
