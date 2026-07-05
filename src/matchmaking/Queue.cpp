#include "Queue.h"
#include <algorithm>

namespace arenanet {
namespace matchmaking {

void Queue::addEntry(const QueueEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Ensure no members are already in queue
    for (auto memberId : entry.members) {
        auto it = std::find_if(entries_.begin(), entries_.end(),
            [memberId](const QueueEntry& e) {
                return std::find(e.members.begin(), e.members.end(), memberId) != e.members.end();
            });
        if (it != entries_.end()) {
            return; // Already in queue
        }
    }
    
    entries_.push_back(entry);
}

void Queue::removePlayer(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [playerId](const QueueEntry& e) {
                return std::find(e.members.begin(), e.members.end(), playerId) != e.members.end();
            }),
        entries_.end()
    );
}

std::vector<QueueEntry> Queue::getMatch(int targetTotalPlayers, long long currentTimeMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<QueueEntry> match;
    
    // Sort by join time FIFO
    std::sort(entries_.begin(), entries_.end(), 
        [](const QueueEntry& a, const QueueEntry& b) { return a.joinTimeMs < b.joinTimeMs; });

    // O(N^2) search for combinations. This is fine for MVP.
    for (size_t i = 0; i < entries_.size(); ++i) {
        const auto& baseEntry = entries_[i];
        int currentCount = baseEntry.members.size();
        
        if (currentCount > targetTotalPlayers) continue;
        
        long long waitSeconds = (currentTimeMs - baseEntry.joinTimeMs) / 1000;
        int tolerance = 100 + (waitSeconds / 10) * 100; // expand 100 rating every 10s
        
        std::vector<size_t> matchedIndices;
        matchedIndices.push_back(i);
        
        if (currentCount == targetTotalPlayers) {
            // Full party found
            match.push_back(baseEntry);
            entries_.erase(entries_.begin() + i);
            return match;
        }
        
        for (size_t j = i + 1; j < entries_.size(); ++j) {
            const auto& potentialEntry = entries_[j];
            
            // Check rating tolerance
            if (std::abs(baseEntry.averageRating - potentialEntry.averageRating) <= tolerance) {
                if (currentCount + potentialEntry.members.size() <= static_cast<size_t>(targetTotalPlayers)) {
                    matchedIndices.push_back(j);
                    currentCount += potentialEntry.members.size();
                }
            }
            
            if (currentCount == targetTotalPlayers) {
                // Match found!
                for (size_t idx : matchedIndices) {
                    match.push_back(entries_[idx]);
                }
                
                // Erase from queue (in reverse order to not invalidate indices)
                std::sort(matchedIndices.rbegin(), matchedIndices.rend());
                for (size_t idx : matchedIndices) {
                    entries_.erase(entries_.begin() + idx);
                }
                
                return match;
            }
        }
    }
    
    return match;
}

} // namespace matchmaking
} // namespace arenanet
