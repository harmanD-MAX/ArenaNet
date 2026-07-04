#include <iostream>
#include <asio.hpp>
#include <memory>
#include <thread>
#include <csignal>

#include "common/Logger.h"
#include "config/ConfigManager.h"
#include "database/PostgresClient.h"
#include "redis/RedisClient.h"

#include "network/TCPServer.h"
#include "authentication/AuthManager.h"
#include "lobby/LobbyManager.h"
#include "matchmaking/Matchmaker.h"
#include "chat/ChatManager.h"
#include "leaderboard/LeaderboardManager.h"

using namespace arenanet;

asio::io_context ioContext;

void signalHandler(int signum) {
    common::Logger::info("Interrupt signal (" + std::to_string(signum) + ") received. Shutting down...");
    matchmaking::Matchmaker::getInstance().stop();
    database::PostgresClient::getInstance().disconnect();
    ioContext.stop();
    exit(signum);
}

void routePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    int typeId = static_cast<int>(packet.type);
    
    if (typeId >= 1 && typeId < 10) {
        authentication::AuthManager::getInstance().handlePacket(conn, packet);
    } else if (typeId >= 10 && typeId < 20) {
        lobby::LobbyManager::getInstance().handlePacket(conn, packet);
    } else if (typeId >= 20 && typeId < 30) {
        matchmaking::Matchmaker::getInstance().handlePacket(conn, packet);
    } else if (typeId >= 30 && typeId < 40) {
        chat::ChatManager::getInstance().handlePacket(conn, packet);
    } else if (typeId >= 40 && typeId < 50) {
        leaderboard::LeaderboardManager::getInstance().handlePacket(conn, packet);
    } else {
        common::Logger::warning("Received unknown packet type: " + std::to_string(typeId));
    }
}

int main() {
    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        common::Logger::info("Starting ArenaNet Server...");

        // Load config
        auto& config = config::ConfigManager::getInstance();
        config.loadFromEnvironment();

        // Init Database
        auto& db = database::PostgresClient::getInstance();
        db.connect(config.getDbHost(), config.getDbPort(), config.getDbUser(), config.getDbPass(), config.getDbName());
        db.initializeSchema();

        // Init Redis
        auto& redis = redis::RedisClient::getInstance();
        redis.connect(config.getRedisHost(), config.getRedisPort());

        // Start Matchmaker thread
        matchmaking::Matchmaker::getInstance().start();

        // Start TCP Server
        network::TCPServer tcpServer(ioContext, config.getTcpPort());
        
        tcpServer.setPacketHandler(routePacket);
        
        tcpServer.setDisconnectHandler([](std::shared_ptr<network::Connection> conn) {
            // Cleanup on disconnect
            if (conn->getPlayerId() != 0) {
                // If they were in queue, remove them
                matchmaking::Matchmaker::getInstance().unregisterConnection(conn->getPlayerId());
            }
        });

        common::Logger::info("ArenaNet Server running on TCP port " + std::to_string(config.getTcpPort()));

        ioContext.run();

    } catch (const std::exception& e) {
        common::Logger::error("Fatal error: " + std::string(e.what()));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
