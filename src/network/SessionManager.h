#pragma once

#include "Connection.h"
#include "../common/Types.h"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace arenanet {
namespace network {

class SessionManager {
public:
    static SessionManager& getInstance() {
        static SessionManager instance;
        return instance;
    }

    void registerConnection(common::PlayerId playerId, std::shared_ptr<Connection> conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_[playerId] = conn;
    }

    void unregisterConnection(common::PlayerId playerId) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(playerId);
    }

    std::shared_ptr<Connection> getConnection(common::PlayerId playerId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(playerId);
        if (it != connections_.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    SessionManager() = default;

    std::mutex mutex_;
    std::unordered_map<common::PlayerId, std::shared_ptr<Connection>> connections_;
};

} // namespace network
} // namespace arenanet
