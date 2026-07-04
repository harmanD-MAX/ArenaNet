#include "PostgresClient.h"
#include "../common/Logger.h"
#include <iostream>

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

        // Create Users table
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

        W.commit();
        common::Logger::info("Database schema initialized.");
    } catch (const std::exception& e) {
        common::Logger::error("Schema initialization failed: " + std::string(e.what()));
    }
}

common::PlayerId PostgresClient::createUser(const std::string& username, const std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    try {
        pqxx::work W(*conn_);
        pqxx::result R = W.exec_params(
            "INSERT INTO users (username, password_hash) VALUES ($1, $2) RETURNING id",
            username, passwordHash
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
        pqxx::result R = N.exec_params(
            "SELECT id FROM users WHERE username = $1 AND password_hash = $2",
            username, passwordHash
        );
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
        pqxx::result R = N.exec_params(
            "SELECT id, username, rating, wins, losses FROM users WHERE id = $1",
            playerId
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
            W.exec_params("UPDATE users SET wins = wins + 1, rating = rating + 25 WHERE id = $1", playerId);
        } else {
            W.exec_params("UPDATE users SET losses = losses + 1, rating = GREATEST(0, rating - 15) WHERE id = $1", playerId);
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
        pqxx::result R = N.exec_params(
            "SELECT id, username, rating, wins, losses FROM users ORDER BY rating DESC, wins DESC LIMIT $1",
            limit
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
        pqxx::result R = N.exec_params(
            "SELECT rank FROM ("
            "  SELECT id, RANK() OVER (ORDER BY rating DESC, wins DESC) as rank FROM users"
            ") ranked_users WHERE id = $1",
            playerId
        );
        if (!R.empty()) {
            return R[0]["rank"].as<int>();
        }
    } catch (const std::exception& e) {
        common::Logger::error("Failed to get player rank: " + std::string(e.what()));
    }
    return -1; // Return -1 on error or not found
}

} // namespace database
} // namespace arenanet
