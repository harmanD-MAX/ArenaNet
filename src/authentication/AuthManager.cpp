#include "AuthManager.h"
#include "JwtUtils.h"
#include "../database/PostgresClient.h"
#include "../redis/RedisClient.h"
#include "../network/SessionManager.h"
#include "../lobby/LobbyManager.h"
#include "../config/ConfigManager.h"
#include "../common/Logger.h"
#include "../common/Error.h"


namespace arenanet {
namespace authentication {

void AuthManager::handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    switch (packet.type) {
        case network::PacketType::REGISTER_REQUEST:
            handleRegister(conn, packet);
            break;
        case network::PacketType::LOGIN_REQUEST:
            handleLogin(conn, packet);
            break;
        default:
            break; // Handled elsewhere
    }
}

std::string AuthManager::hashPassword(const std::string& password) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

void AuthManager::handleRegister(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    network::Packet response;
    response.type = network::PacketType::REGISTER_RESPONSE;

    try {
        std::string username = packet.payload["username"];
        std::string password = packet.payload["password"];
        
        if (username.length() < 3 || password.length() < 6) {
            throw common::ArenaException(common::ErrorCode::INVALID_CREDENTIALS, "Username or password too short.");
        }

        std::string hashedPw = hashPassword(password);
        
        // Create user in DB
        common::PlayerId newId = database::PostgresClient::getInstance().createUser(username, hashedPw);
        
        response.payload["success"] = true;
        response.payload["player_id"] = newId;
        common::Logger::info("Registered new user: " + username);

    } catch (const common::ArenaException& e) {
        response.payload["success"] = false;
        response.payload["error_code"] = static_cast<int>(e.getCode());
        response.payload["error_msg"] = e.what();
    } catch (const std::exception& e) {
        response.payload["success"] = false;
        response.payload["error_code"] = static_cast<int>(common::ErrorCode::UNKNOWN_ERROR);
        response.payload["error_msg"] = "Internal server error.";
    }

    conn->send(response);
}

void AuthManager::handleLogin(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    network::Packet response;
    response.type = network::PacketType::LOGIN_RESPONSE;

    try {
        common::PlayerId playerId = 0;
        std::string token = "";
        
        if (packet.payload.contains("token")) {
            // Reconnect flow
            token = packet.payload["token"];
            const std::string& secret = config::ConfigManager::getInstance().getJwtSecret();
            if (!JwtUtils::verifyToken(token, secret, playerId)) {
                throw common::ArenaException(common::ErrorCode::INVALID_CREDENTIALS, "Invalid or expired token.");
            }
        } else {
            // Normal login flow
            std::string username = packet.payload["username"];
            std::string password = packet.payload["password"];
            
            std::string hashedPw = hashPassword(password);
            playerId = database::PostgresClient::getInstance().verifyUser(username, hashedPw);
            
            // Generate JWT
            const std::string& secret = config::ConfigManager::getInstance().getJwtSecret();
            token = JwtUtils::generateToken(playerId, secret);
            
            // Store in Redis (optional, for token invalidation)
            redis::RedisClient::getInstance().storeToken(playerId, token, 86400);
        }

        // Update connection state
        conn->setPlayerId(playerId);

        // Update presence
        redis::RedisClient::getInstance().setPlayerPresence(playerId, "ONLINE");
        
        // Register connection
        network::SessionManager::getInstance().registerConnection(playerId, conn);
        
        // Notify Lobby of Reconnect (if they were disconnected)
        lobby::LobbyManager::getInstance().handlePlayerReconnect(playerId, conn);

        response.payload["success"] = true;
        response.payload["token"] = token;
        response.payload["player_id"] = playerId;
        common::Logger::info("User logged in / reconnected: " + std::to_string(playerId));

    } catch (const common::ArenaException& e) {
        response.payload["success"] = false;
        response.payload["error_code"] = static_cast<int>(e.getCode());
        response.payload["error_msg"] = e.what();
    } catch (const std::exception& e) {
        response.payload["success"] = false;
        response.payload["error_code"] = static_cast<int>(common::ErrorCode::UNKNOWN_ERROR);
        response.payload["error_msg"] = "Internal server error.";
    }

    conn->send(response);
}

} // namespace authentication
} // namespace arenanet
