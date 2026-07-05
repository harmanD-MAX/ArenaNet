#include "Queue.h"
#include <algorithm>
#include "../config/ConfigManager.h"
#include "../common/Logger.h"

namespace arenanet {
namespace matchmaking {

void Queue::addEntry(const QueueEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove any existing entries for these members (allows re-queuing with different settings)
    for (auto memberId : entry.members) {
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [memberId](const QueueEntry& e) {
                    return std::find(e.members.begin(), e.members.end(), memberId) != e.members.end();
                }),
            entries_.end()
        );
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

void Queue::getMatch(long long currentTimeMs, std::vector<QueueEntry>& outMatch, std::vector<QueueEntry>& outTimedOut) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Sort by join time FIFO
    std::sort(entries_.begin(), entries_.end(), 
        [](const QueueEntry& a, const QueueEntry& b) { return a.joinTimeMs < b.joinTimeMs; });

    // O(N^2) search for combinations
    for (size_t i = 0; i < entries_.size(); ++i) {
        const auto& baseEntry = entries_[i];
        int targetTotalPlayers = baseEntry.targetMatchSize;
        if (targetTotalPlayers == -1) {
            targetTotalPlayers = config::ConfigManager::getInstance().getMatchSize();
        }
        
        int currentCount = baseEntry.members.size();
        
        if (currentCount > targetTotalPlayers) continue;
        
        long long waitSeconds = (currentTimeMs - baseEntry.joinTimeMs) / 1000;
        int queueTimeout = config::ConfigManager::getInstance().getQueueTimeoutSeconds();
        if (waitSeconds > queueTimeout) {
            // Push to outTimedOut and remove from queue
            outTimedOut.push_back(baseEntry);
            entries_.erase(entries_.begin() + i);
            common::Logger::info("Matchmaking queue timeout for a party. Removed from queue.");
            return; // Return early, Matchmaker will handle the timeout and call getMatch again next tick
        }
        
        int expansionRate = config::ConfigManager::getInstance().getMatchmakingExpansionRate();
        int tolerance = 100 + (waitSeconds / 10) * expansionRate; // expand rating every 10s
        
        std::vector<size_t> matchedIndices;
        matchedIndices.push_back(i);
        
        if (currentCount == targetTotalPlayers) {
            // Full party found
            outMatch.push_back(baseEntry);
            entries_.erase(entries_.begin() + i);
            return;
        }
        
        for (size_t j = 0; j < entries_.size(); ++j) {
            if (i == j) continue;
            
            const auto& potentialEntry = entries_[j];
            
            int potentialTarget = potentialEntry.targetMatchSize;
            if (potentialTarget != -1 && potentialTarget != targetTotalPlayers) {
                continue;
            }
            
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
                    outMatch.push_back(entries_[idx]);
                }
                
                // Erase from queue (in reverse order to not invalidate indices)
                std::sort(matchedIndices.rbegin(), matchedIndices.rend());
                for (size_t idx : matchedIndices) {
                    entries_.erase(entries_.begin() + idx);
                }
                
                return;
            }
        }
    }
}

} // namespace matchmaking
} // namespace arenanet
