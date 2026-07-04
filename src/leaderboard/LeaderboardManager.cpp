#include "LeaderboardManager.h"
#include "../database/PostgresClient.h"
#include "../common/Logger.h"

namespace arenanet {
namespace leaderboard {

void LeaderboardManager::handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (packet.type == network::PacketType::GET_LEADERBOARD_REQUEST) {
        handleGetLeaderboard(conn, packet);
    }
}

void LeaderboardManager::handleGetLeaderboard(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    network::Packet response;
    response.type = network::PacketType::GET_LEADERBOARD_RESPONSE;
    
    try {
        common::PlayerId playerId = conn->getPlayerId();
        if (playerId != 0) {
            auto topPlayers = database::PostgresClient::getInstance().getTopPlayers(10);
            int playerRank = database::PostgresClient::getInstance().getPlayerRank(playerId);
            
            nlohmann::json entries = nlohmann::json::array();
            for (const auto& profile : topPlayers) {
                nlohmann::json entry;
                entry["player_id"] = profile.id;
                entry["username"] = profile.username;
                entry["rating"] = profile.rating;
                entry["wins"] = profile.wins;
                entry["losses"] = profile.losses;
                entries.push_back(entry);
            }
            
            response.payload["leaderboard"] = entries;
            response.payload["your_rank"] = playerRank;
            response.payload["success"] = true;
        } else {
            response.payload["success"] = false;
            response.payload["error_msg"] = "Not authenticated";
        }
    } catch (const std::exception& e) {
        response.payload["success"] = false;
        response.payload["error_msg"] = "Failed to fetch leaderboard.";
        common::Logger::error("Leaderboard error: " + std::string(e.what()));
    }
    
    conn->send(response);
}

} // namespace leaderboard
} // namespace arenanet
