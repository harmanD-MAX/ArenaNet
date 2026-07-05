#include "MatchManager.h"
#include "../database/PostgresClient.h"
#include "../common/Logger.h"
#include <algorithm>

namespace arenanet {
namespace match {

void MatchManager::handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (conn->getPlayerId() == 0) return;

    switch (packet.type) {
        case network::PacketType::REPORT_MATCH_RESULT:
            handleReportMatchResult(conn, packet);
            break;
        case network::PacketType::GET_MATCH_HISTORY_REQUEST:
            handleGetMatchHistory(conn, packet);
            break;
        default:
            break;
    }
}

void MatchManager::handleReportMatchResult(std::shared_ptr<network::Connection> /*conn*/, const network::Packet& packet) {
    try {
        std::string matchId = packet.payload["match_id"];
        common::PlayerId winnerId = packet.payload["winner_id"];
        int durationSeconds = packet.payload.value("duration_seconds", 0);
        
        std::vector<common::PlayerId> players;
        for (auto pId : packet.payload["players"]) {
            players.push_back(pId);
        }
        std::vector<common::PlayerId> winners;
        if (packet.payload.contains("winners")) {
            for (auto pId : packet.payload["winners"]) {
                winners.push_back(pId);
            }
        } else {
            winners.push_back(winnerId); // Fallback for older clients
        }
        
        database::PostgresClient::getInstance().recordMatchResult(matchId, winnerId, durationSeconds, players);
        
        // Update ratings
        for (auto pId : players) {
            bool isWin = (std::find(winners.begin(), winners.end(), pId) != winners.end());
            database::PostgresClient::getInstance().updatePlayerStats(pId, isWin);
        }
        
        common::Logger::info("Recorded match result for match: " + matchId);
    } catch (const std::exception& e) {
        common::Logger::error("Error reporting match result: " + std::string(e.what()));
    }
}

void MatchManager::handleGetMatchHistory(std::shared_ptr<network::Connection> conn, const network::Packet& /*packet*/) {
    try {
        auto history = database::PostgresClient::getInstance().getMatchHistory(conn->getPlayerId());
        
        nlohmann::json jHistory = nlohmann::json::array();
        for (const auto& entry : history) {
            jHistory.push_back({
                {"match_id", entry.matchId},
                {"winner_id", entry.winnerId},
                {"duration_seconds", entry.durationSeconds},
                {"timestamp", entry.timestamp},
                {"players", entry.players}
            });
        }
        
        network::Packet response;
        response.type = network::PacketType::GET_MATCH_HISTORY_RESPONSE;
        response.payload["history"] = jHistory;
        
        conn->send(response);
    } catch (const std::exception& e) {
        common::Logger::error("Error fetching match history: " + std::string(e.what()));
    }
}

} // namespace match
} // namespace arenanet
