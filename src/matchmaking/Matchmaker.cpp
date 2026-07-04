#include "Matchmaker.h"
#include "../common/Logger.h"
#include <chrono>
#include <random>

namespace arenanet {
namespace matchmaking {

Matchmaker::~Matchmaker() {
    stop();
}

void Matchmaker::start() {
    if (isRunning_) return;
    isRunning_ = true;
    matchThread_ = std::make_unique<std::thread>(&Matchmaker::matchLoop, this);
    common::Logger::info("Matchmaker started.");
}

void Matchmaker::stop() {
    if (!isRunning_) return;
    isRunning_ = false;
    if (matchThread_ && matchThread_->joinable()) {
        matchThread_->join();
    }
    common::Logger::info("Matchmaker stopped.");
}

void Matchmaker::handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (conn->getPlayerId() == 0) return;

    registerConnection(conn->getPlayerId(), conn);

    switch (packet.type) {
        case network::PacketType::JOIN_QUEUE_REQUEST:
            handleJoinQueue(conn, packet);
            break;
        case network::PacketType::LEAVE_QUEUE_REQUEST:
            handleLeaveQueue(conn, packet);
            break;
        default:
            break;
    }
}

void Matchmaker::registerConnection(common::PlayerId playerId, std::shared_ptr<network::Connection> conn) {
    std::lock_guard<std::mutex> lock(connMutex_);
    connections_[playerId] = conn;
}

void Matchmaker::unregisterConnection(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(connMutex_);
    connections_.erase(playerId);
}

void Matchmaker::handleJoinQueue(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    common::PlayerId playerId = conn->getPlayerId();
    // Assuming client doesn't dictate their rating, but for MVP we might fetch it from DB later. 
    // Here we just use a default rating of 1000.
    int rating = 1000; 
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
        
    queue_.addPlayer(playerId, rating, now);
    
    network::Packet response;
    response.type = network::PacketType::JOIN_QUEUE_RESPONSE;
    response.payload["success"] = true;
    conn->send(response);
    
    common::Logger::info("Player " + std::to_string(playerId) + " joined the matchmaking queue.");
}

void Matchmaker::handleLeaveQueue(std::shared_ptr<network::Connection> conn, const network::Packet& /*packet*/) {
    common::PlayerId playerId = conn->getPlayerId();
    queue_.removePlayer(playerId);
    common::Logger::info("Player " + std::to_string(playerId) + " left the matchmaking queue.");
}

void Matchmaker::matchLoop() {
    while (isRunning_) {
        // We'll try to find 2 players for a match
        auto matched = queue_.getMatch(2);
        
        if (!matched.empty()) {
            notifyMatch(matched);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void Matchmaker::notifyMatch(const std::vector<QueueEntry>& matchedPlayers) {
    std::string matchId = "match_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    network::Packet eventPacket;
    eventPacket.type = network::PacketType::MATCH_FOUND_EVENT;
    eventPacket.payload["match_id"] = matchId;
    
    nlohmann::json playersArr = nlohmann::json::array();
    for (const auto& p : matchedPlayers) {
        playersArr.push_back(p.playerId);
    }
    eventPacket.payload["players"] = playersArr;

    std::lock_guard<std::mutex> lock(connMutex_);
    for (const auto& p : matchedPlayers) {
        auto it = connections_.find(p.playerId);
        if (it != connections_.end() && it->second) {
            it->second->send(eventPacket);
        }
    }
    
    common::Logger::info("Match created: " + matchId + " with " + std::to_string(matchedPlayers.size()) + " players.");
}

} // namespace matchmaking
} // namespace arenanet
