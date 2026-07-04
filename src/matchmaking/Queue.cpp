#include "Queue.h"
#include <algorithm>

namespace arenanet {
namespace matchmaking {

void Queue::addPlayer(common::PlayerId playerId, int rating, long long timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(entries_.begin(), entries_.end(), 
        [playerId](const QueueEntry& e) { return e.playerId == playerId; });
        
    if (it == entries_.end()) {
        entries_.push_back({playerId, rating, timestamp});
    }
}

void Queue::removePlayer(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [playerId](const QueueEntry& e) { return e.playerId == playerId; }),
        entries_.end()
    );
}

std::vector<QueueEntry> Queue::getMatch(int playersNeeded) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // For MVP, we don't do complex rating matching. We just grab the first N players who have been waiting.
    // We sort by join time to ensure FIFO.
    std::sort(entries_.begin(), entries_.end(), 
        [](const QueueEntry& a, const QueueEntry& b) { return a.joinTimeMs < b.joinTimeMs; });

    std::vector<QueueEntry> match;
    if (entries_.size() >= static_cast<size_t>(playersNeeded)) {
        for (int i = 0; i < playersNeeded; ++i) {
            match.push_back(entries_[i]);
        }
        // Remove from queue
        entries_.erase(entries_.begin(), entries_.begin() + playersNeeded);
    }
    
    return match;
}

} // namespace matchmaking
} // namespace arenanet
