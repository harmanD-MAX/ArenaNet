#include "PostgresClient.h"
#include "../common/Logger.h"
#include "../common/Error.h"


namespace arenanet {
namespace database {

void PostgresClient::connect(const std::string& host, const std::string& port, 
                             const std::string& user, const std::string& pass, const std::string& dbname) {
    try {
        std::string connectionString = "host=" + host + " port=" + port + " dbname=" + dbname + 
                                       " user=" + user + " password=" + pass;
        conn_ = std::make_unique<pqxx::connection>(connectionString);
        if (conn_->is_open()) {
            common::Logger::info("Connected to PostgreSQL database successfully.");
        } else {
            common::Logger::error("Failed to open PostgreSQL connection.");
        }
    } catch (const std::exception& e) {
        common::Logger::error("Database connection error: " + std::string(e.what()));
        throw common::ArenaException(common::ErrorCode::DATABASE_ERROR, "Failed to connect to database");
    }
}

void PostgresClient::disconnect() {
    if (conn_ && conn_->is_open()) {
        conn_->close();
        common::Logger::info("Disconnected from PostgreSQL database.");
    }
}

void PostgresClient::initializeSchema() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!conn_ || !conn_->is_open()) return;

    try {
        pqxx::work W(*conn_);

        W.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "id SERIAL PRIMARY KEY, "
            "username VARCHAR(50) UNIQUE NOT NULL, "
            "password_hash VARCHAR(255) NOT NULL, "
            "rating INTEGER DEFAULT 1000, "
            "wins INTEGER DEFAULT 0, "
            "losses INTEGER DEFAULT 0"
            ");"
        );

        W.exec(
            "CREATE TABLE IF NOT EXISTS friends ("
            "player1_id INTEGER REFERENCES users(id) ON DELETE CASCADE, "
            "player2_id INTEGER REFERENCES users(id) ON DELETE CASCADE, "
            "status VARCHAR(20) DEFAULT 'PENDING', "
            "PRIMARY KEY (player1_id, player2_id)"
            ");"
        );

        W.exec(
            "CREATE TABLE IF NOT EXISTS matches ("
            "id VARCHAR(50) PRIMARY KEY, "
            "winner_id INTEGER REFERENCES users(id) ON DELETE SET NULL, "
            "duration_seconds INTEGER DEFAULT 0, "
            "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");"
        );

        W.exec(
            "CREATE TABLE IF NOT EXISTS match_players ("
            "match_id VARCHAR(50) REFERENCES matches(id) ON DELETE CASCADE, "
            "player_id INTEGER REFERENCES users(id) ON DELETE CASCADE, "
            "PRIMARY KEY (match_id, player_id)"
            ");"
        );

        W.commit();
        common::Logger::info("Database schema initialized.");
    } catch (const std::exception& e) {
        common::Logger::error("Schema initialization failed: " + std::string(e.what()));
    }
}

// AUTHENTICATION & PROFILES

common::PlayerId PostgresClient::createUser(const std::string& username, const std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::work W(*conn_);
        pqxx::result R = W.exec(
            "INSERT INTO users (username, password_hash) VALUES ($1, $2) RETURNING id",
            pqxx::params{username, passwordHash}
        );
        W.commit();
        if (!R.empty()) {
            return R[0][0].as<common::PlayerId>();
        }
    } catch (const pqxx::sql_error& e) {
        if (std::string(e.what()).find("duplicate key value violates unique constraint") != std::string::npos) {
            throw common::ArenaException(common::ErrorCode::USER_ALREADY_EXISTS, "Username already taken.");
        }
        common::Logger::error("Failed to create user: " + std::string(e.what()));
    } catch (const std::exception& e) {
        common::Logger::error("Failed to create user: " + std::string(e.what()));
    }
    throw common::ArenaException(common::ErrorCode::DATABASE_ERROR, "Failed to create user.");
}

common::PlayerId PostgresClient::verifyUser(const std::string& username, const std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::nontransaction N(*conn_);
        pqxx::result R = N.exec("SELECT id FROM users WHERE username = $1 AND password_hash = $2", pqxx::params{username, passwordHash});
        if (!R.empty()) {
            return R[0][0].as<common::PlayerId>();
        }
        throw common::ArenaException(common::ErrorCode::INVALID_CREDENTIALS, "Invalid username or password.");
    } catch (const common::ArenaException&) {
        throw;
    } catch (const std::exception& e) {
        common::Logger::error("Failed to verify user: " + std::string(e.what()));
        throw common::ArenaException(common::ErrorCode::DATABASE_ERROR, "Database error during login.");
    }
}

common::PlayerProfile PostgresClient::getPlayerProfile(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::nontransaction N(*conn_);
        pqxx::result R = N.exec(
            "SELECT id, username, rating, wins, losses FROM users WHERE id = $1",
            pqxx::params{playerId}
        );
        if (!R.empty()) {
            common::PlayerProfile profile;
            profile.id = R[0]["id"].as<common::PlayerId>();
            profile.username = R[0]["username"].as<std::string>();
            profile.rating = R[0]["rating"].as<int>();
            profile.wins = R[0]["wins"].as<int>();
            profile.losses = R[0]["losses"].as<int>();
            return profile;
        }
        throw common::ArenaException(common::ErrorCode::UNKNOWN_ERROR, "Player profile not found.");
    } catch (const std::exception& e) {
        common::Logger::error("Failed to get player profile: " + std::string(e.what()));
        throw common::ArenaException(common::ErrorCode::DATABASE_ERROR, "Database error getting profile.");
    }
}

void PostgresClient::updatePlayerStats(common::PlayerId playerId, bool isWin) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::work W(*conn_);
        if (isWin) {
            W.exec("UPDATE users SET wins = wins + 1, rating = rating + 25 WHERE id = $1", pqxx::params{playerId});
        } else {
            W.exec("UPDATE users SET losses = losses + 1, rating = GREATEST(0, rating - 15) WHERE id = $1", pqxx::params{playerId});
        }
        W.commit();
    } catch (const std::exception& e) {
        common::Logger::error("Failed to update player stats: " + std::string(e.what()));
    }
}

std::vector<common::PlayerProfile> PostgresClient::getTopPlayers(int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<common::PlayerProfile> topPlayers;
    try {
        pqxx::nontransaction N(*conn_);
        pqxx::result R = N.exec(
            "SELECT id, username, rating, wins, losses FROM users ORDER BY rating DESC, wins DESC LIMIT $1",
            pqxx::params{limit}
        );
        for (auto row : R) {
            common::PlayerProfile profile;
            profile.id = row["id"].as<common::PlayerId>();
            profile.username = row["username"].as<std::string>();
            profile.rating = row["rating"].as<int>();
            profile.wins = row["wins"].as<int>();
            profile.losses = row["losses"].as<int>();
            topPlayers.push_back(profile);
        }
    } catch (const std::exception& e) {
        common::Logger::error("Failed to get top players: " + std::string(e.what()));
    }
    return topPlayers;
}

int PostgresClient::getPlayerRank(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::nontransaction N(*conn_);
        // Use a subquery to calculate rank
        pqxx::result R = N.exec(
            "SELECT rank FROM ("
            "  SELECT id, RANK() OVER (ORDER BY rating DESC, wins DESC) as rank FROM users"
            ") ranked_users WHERE id = $1",
            pqxx::params{playerId}
        );
        if (!R.empty()) {
            return R[0]["rank"].as<int>();
        }
    } catch (const std::exception& e) {
        common::Logger::error("Failed to get player rank: " + std::string(e.what()));
    }
    return -1; // Return -1 on error or not found
}

// FRIEND SYSTEM

void PostgresClient::sendFriendRequest(common::PlayerId sender, common::PlayerId receiver) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::work W(*conn_);
        W.exec(
            "INSERT INTO friends (player1_id, player2_id, status) VALUES ($1, $2, 'PENDING') "
            "ON CONFLICT (player1_id, player2_id) DO NOTHING",
            pqxx::params{sender, receiver}
        );
        W.commit();
    } catch (const std::exception& e) {
        common::Logger::error("Failed to send friend request: " + std::string(e.what()));
    }
}

void PostgresClient::acceptFriendRequest(common::PlayerId receiver, common::PlayerId sender) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::work W(*conn_);
        W.exec(
            "UPDATE friends SET status = 'ACCEPTED' WHERE player1_id = $1 AND player2_id = $2",
            pqxx::params{sender, receiver}
        );
        
        W.exec(
            "INSERT INTO friends (player1_id, player2_id, status) VALUES ($1, $2, 'ACCEPTED') "
            "ON CONFLICT (player1_id, player2_id) DO UPDATE SET status = 'ACCEPTED'",
            pqxx::params{receiver, sender}
        );
        W.commit();
    } catch (const std::exception& e) {
        common::Logger::error("Failed to accept friend request: " + std::string(e.what()));
    }
}

void PostgresClient::rejectFriendRequest(common::PlayerId receiver, common::PlayerId sender) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::work W(*conn_);
        W.exec("DELETE FROM friends WHERE player1_id = $1 AND player2_id = $2", pqxx::params{sender, receiver});
        W.commit();
    } catch (const std::exception& e) {
        common::Logger::error("Failed to reject friend request: " + std::string(e.what()));
    }
}

void PostgresClient::removeFriend(common::PlayerId player1, common::PlayerId player2) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::work W(*conn_);
        W.exec("DELETE FROM friends WHERE (player1_id = $1 AND player2_id = $2) OR (player1_id = $2 AND player2_id = $1)", pqxx::params{player1, player2});
        W.commit();
    } catch (const std::exception& e) {
        common::Logger::error("Failed to remove friend: " + std::string(e.what()));
    }
}

std::vector<common::FriendInfo> PostgresClient::getFriendsList(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<common::FriendInfo> friends;
    try {
        pqxx::nontransaction N(*conn_);
        
        pqxx::result R = N.exec(
            "SELECT f.player2_id as friend_id, u.username, f.status "
            "FROM friends f JOIN users u ON f.player2_id = u.id "
            "WHERE f.player1_id = $1 "
            "UNION "
            "SELECT f.player1_id as friend_id, u.username, "
            "CASE WHEN f.status = 'PENDING' THEN 'INCOMING_REQUEST' ELSE f.status END as status "
            "FROM friends f JOIN users u ON f.player1_id = u.id "
            "WHERE f.player2_id = $1 AND f.status = 'PENDING'",
            pqxx::params{playerId}
        );
        
        for (auto row : R) {
            common::FriendInfo info;
            info.id = row["friend_id"].as<common::PlayerId>();
            info.username = row["username"].as<std::string>();
            info.status = row["status"].as<std::string>();
            info.presence = "OFFLINE"; // To be filled in later by Redis
            friends.push_back(info);
        }
    } catch (const std::exception& e) {
        common::Logger::error("Failed to get friends list: " + std::string(e.what()));
    }
    return friends;
}

// MATCH HISTORY SYSTEM

void PostgresClient::recordMatchResult(const std::string& matchId, common::PlayerId winnerId, int durationSeconds, const std::vector<common::PlayerId>& players) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::work W(*conn_);
        
        if (winnerId == 0) {
             W.exec(
                "INSERT INTO matches (id, winner_id, duration_seconds) VALUES ($1, NULL, $2)",
                pqxx::params{matchId, durationSeconds}
            );
        } else {
             W.exec(
                "INSERT INTO matches (id, winner_id, duration_seconds) VALUES ($1, $2, $3)",
                pqxx::params{matchId, winnerId, durationSeconds}
            );
        }
       
        for (auto pId : players) {
            W.exec("INSERT INTO match_players (match_id, player_id) VALUES ($1, $2)", pqxx::params{matchId, pId});
        }
        
        W.commit();
    } catch (const std::exception& e) {
        common::Logger::error("Failed to record match result: " + std::string(e.what()));
    }
}

std::vector<common::MatchHistoryEntry> PostgresClient::getMatchHistory(common::PlayerId playerId, int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<common::MatchHistoryEntry> history;
    try {
        pqxx::nontransaction N(*conn_);
        
        pqxx::result R = N.exec(
            "SELECT m.id, m.winner_id, m.duration_seconds, m.timestamp::text as ts "
            "FROM matches m "
            "JOIN match_players mp ON m.id = mp.match_id "
            "WHERE mp.player_id = $1 "
            "ORDER BY m.timestamp DESC "
            "LIMIT $2",
            pqxx::params{playerId, limit}
        );
        
        for (auto row : R) {
            common::MatchHistoryEntry entry;
            entry.matchId = row["id"].as<std::string>();
            entry.winnerId = row["winner_id"].is_null() ? 0 : row["winner_id"].as<common::PlayerId>();
            entry.durationSeconds = row["duration_seconds"].as<int>();
            entry.timestamp = row["ts"].as<std::string>();
            
            pqxx::result pr = N.exec(
                "SELECT u.username FROM users u "
                "JOIN match_players mp ON u.id = mp.player_id "
                "WHERE mp.match_id = $1",
                pqxx::params{entry.matchId}
            );
            
            for (auto pRow : pr) {
                entry.players.push_back(pRow["username"].as<std::string>());
            }
            
            history.push_back(entry);
        }
    } catch (const std::exception& e) {
        common::Logger::error("Failed to get match history: " + std::string(e.what()));
    }
    return history;
}

} // namespace database
} // namespace arenanet
