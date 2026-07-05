#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include "Queue.h"
#include "../network/Connection.h"
#include "../network/Packet.h"

namespace arenanet {
namespace matchmaking {

class Matchmaker {
public:
    static Matchmaker& getInstance() {
        static Matchmaker instance;
        return instance;
    }

    void start();
    void stop();

    void handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet);

    void removePlayerFromQueue(common::PlayerId playerId);
    
private:
    Matchmaker() = default;
    ~Matchmaker();

    void matchLoop();
    void notifyMatch(const std::vector<QueueEntry>& matchedPlayers);

    void handleJoinQueue(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleLeaveQueue(std::shared_ptr<network::Connection> conn, const network::Packet& packet);

    Queue queue_;
    std::atomic<bool> isRunning_{false};
    std::unique_ptr<std::thread> matchThread_;
};

} // namespace matchmaking
} // namespace arenanet
