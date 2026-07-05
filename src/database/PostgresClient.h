#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <mutex>
#include "../common/Types.h"
#include "../common/Error.h"

namespace arenanet {
namespace database {

class PostgresClient {
public:
    static PostgresClient& getInstance() {
        static PostgresClient instance;
        return instance;
    }

    void connect(const std::string& host, const std::string& port, 
                 const std::string& user, const std::string& pass, const std::string& dbname);
    void disconnect();

    void initializeSchema();

    // Authentication
    common::PlayerId createUser(const std::string& username, const std::string& passwordHash);
    common::PlayerId verifyUser(const std::string& username, const std::string& passwordHash);
    
    // Profile
    common::PlayerProfile getPlayerProfile(common::PlayerId playerId);
    void updatePlayerStats(common::PlayerId playerId, bool isWin);
    
    // Leaderboard
    std::vector<common::PlayerProfile> getTopPlayers(int limit = 10);
    int getPlayerRank(common::PlayerId playerId);

    // Friends
    void sendFriendRequest(common::PlayerId sender, common::PlayerId receiver);
    void acceptFriendRequest(common::PlayerId receiver, common::PlayerId sender);
    void rejectFriendRequest(common::PlayerId receiver, common::PlayerId sender);
    void removeFriend(common::PlayerId player1, common::PlayerId player2);
    std::vector<common::FriendInfo> getFriendsList(common::PlayerId playerId);

    // Match History
    void recordMatchResult(const std::string& matchId, common::PlayerId winnerId, int durationSeconds, const std::vector<common::PlayerId>& players);
    std::vector<common::MatchHistoryEntry> getMatchHistory(common::PlayerId playerId, int limit = 10);

private:
    PostgresClient() = default;

    std::unique_ptr<pqxx::connection> conn_;
    std::mutex dbMutex_; // Simple mutex for MVP. A connection pool would be better for production.
};

} // namespace database
} // namespace arenanet
