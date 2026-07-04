#include "RedisClient.h"
#include "../common/Logger.h"
#include <iostream>

namespace arenanet {
namespace redis {

void RedisClient::connect(const std::string& host, int port) {
    try {
        sw::redis::ConnectionOptions opts;
        opts.host = host;
        opts.port = port;
        
        redis_ = std::make_unique<sw::redis::Redis>(opts);
        
        // Test connection
        redis_->ping();
        common::Logger::info("Connected to Redis successfully.");
    } catch (const sw::redis::Error& e) {
        common::Logger::error("Redis connection error: " + std::string(e.what()));
    }
}

void RedisClient::addPlayerToLobby(const common::LobbyId& lobbyId, common::PlayerId playerId) {
    if (!redis_) return;
    try {
        redis_->sadd("lobby:" + lobbyId, std::to_string(playerId));
    } catch (const sw::redis::Error& e) {
        common::Logger::error("Redis addPlayerToLobby error: " + std::string(e.what()));
    }
}

void RedisClient::removePlayerFromLobby(const common::LobbyId& lobbyId, common::PlayerId playerId) {
    if (!redis_) return;
    try {
        redis_->srem("lobby:" + lobbyId, std::to_string(playerId));
    } catch (const sw::redis::Error& e) {
        common::Logger::error("Redis removePlayerFromLobby error: " + std::string(e.what()));
    }
}

std::vector<common::PlayerId> RedisClient::getPlayersInLobby(const common::LobbyId& lobbyId) {
    std::vector<common::PlayerId> players;
    if (!redis_) return players;

    try {
        std::vector<std::string> members;
        redis_->smembers("lobby:" + lobbyId, std::back_inserter(members));
        for (const auto& m : members) {
            players.push_back(std::stoull(m));
        }
    } catch (const sw::redis::Error& e) {
        common::Logger::error("Redis getPlayersInLobby error: " + std::string(e.what()));
    }
    return players;
}

void RedisClient::storeToken(common::PlayerId playerId, const std::string& token, int expirationSeconds) {
    if (!redis_) return;
    try {
        redis_->setex("auth:" + std::to_string(playerId), expirationSeconds, token);
    } catch (const sw::redis::Error& e) {
        common::Logger::error("Redis storeToken error: " + std::string(e.what()));
    }
}

bool RedisClient::validateToken(common::PlayerId playerId, const std::string& token) {
    if (!redis_) return false;
    try {
        auto val = redis_->get("auth:" + std::to_string(playerId));
        return val && *val == token;
    } catch (const sw::redis::Error& e) {
        common::Logger::error("Redis validateToken error: " + std::string(e.what()));
        return false;
    }
}

void RedisClient::invalidateToken(common::PlayerId playerId) {
    if (!redis_) return;
    try {
        redis_->del("auth:" + std::to_string(playerId));
    } catch (const sw::redis::Error& e) {
        common::Logger::error("Redis invalidateToken error: " + std::string(e.what()));
    }
}

} // namespace redis
} // namespace arenanet
