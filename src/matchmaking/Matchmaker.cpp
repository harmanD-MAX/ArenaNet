#include "Matchmaker.h"
#include "../common/Logger.h"
#include <chrono>
#include <random>
#include "../database/PostgresClient.h"
#include "../party/PartyManager.h"
#include "../network/SessionManager.h"

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

// ---------------------------------------------------------
// QUEUE MANAGEMENT
// ---------------------------------------------------------

void Matchmaker::handleJoinQueue(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    common::PlayerId playerId = conn->getPlayerId();
    auto party = party::PartyManager::getInstance().getPartyOfPlayer(playerId);
    
    QueueEntry entry;
    entry.joinTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
        
    if (party) {
        if (party->leaderId != playerId) {
            common::Logger::warning("Non-leader tried to queue the party.");
            return;
        }
        entry.partyId = party->id;
        entry.members = party->members;
    } else {
        entry.partyId = "";
        entry.members = {playerId};
    }
    
    int totalRating = 0;
    for (auto pId : entry.members) {
        try {
            auto profile = database::PostgresClient::getInstance().getPlayerProfile(pId);
            totalRating += profile.rating;
        } catch (const std::exception& e) {
            common::Logger::warning("Could not fetch profile for player " + std::to_string(pId) + ", defaulting rating to 1000.");
            totalRating += 1000;
        }
    }
    entry.averageRating = entry.members.empty() ? 1000 : (totalRating / entry.members.size());
    
    queue_.addEntry(entry);
    
    network::Packet response;
    response.type = network::PacketType::JOIN_QUEUE_RESPONSE;
    response.payload["success"] = true;
    conn->send(response);
    
    common::Logger::info("Player " + std::to_string(playerId) + " joined the matchmaking queue. Rating: " + std::to_string(entry.averageRating));
}

void Matchmaker::handleLeaveQueue(std::shared_ptr<network::Connection> conn, const network::Packet& /*packet*/) {
    common::PlayerId playerId = conn->getPlayerId();
    removePlayerFromQueue(playerId);
}

void Matchmaker::removePlayerFromQueue(common::PlayerId playerId) {
    queue_.removePlayer(playerId);
    common::Logger::info("Player " + std::to_string(playerId) + " (and their party if any) left the matchmaking queue.");
}

// ---------------------------------------------------------
// MATCHING LOOP
// ---------------------------------------------------------

void Matchmaker::matchLoop() {
    while (isRunning_) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        auto matched = queue_.getMatch(2, now);
        
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
    for (const auto& entry : matchedPlayers) {
        for (auto pId : entry.members) {
            playersArr.push_back(pId);
        }
    }
    eventPacket.payload["players"] = playersArr;

    for (const auto& entry : matchedPlayers) {
        for (auto pId : entry.members) {
            auto targetConn = network::SessionManager::getInstance().getConnection(pId);
            if (targetConn) {
                targetConn->send(eventPacket);
            }
        }
    }
    
    common::Logger::info("Match created: " + matchId + " with " + std::to_string(playersArr.size()) + " players.");
}

} // namespace matchmaking
} // namespace arenanet
