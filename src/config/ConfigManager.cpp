#include "ConfigManager.h"
#include "../common/Logger.h"

namespace arenanet {
namespace config {

void ConfigManager::loadFromEnvironment() {
    dbHost_ = getEnvOrDefault("DB_HOST", "localhost");
    dbPort_ = getEnvOrDefault("DB_PORT", "5432");
    dbUser_ = getEnvOrDefault("DB_USER", "arenanet");
    dbPass_ = getEnvOrDefault("DB_PASS", "password123");
    dbName_ = getEnvOrDefault("DB_NAME", "arenanet_dev");

    redisHost_ = getEnvOrDefault("REDIS_HOST", "localhost");
    redisPort_ = getEnvOrDefaultInt("REDIS_PORT", 6379);

    jwtSecret_ = getEnvOrDefault("JWT_SECRET", "super_secret_jwt_key_for_arenanet_dev");

    tcpPort_ = getEnvOrDefaultInt("TCP_PORT", 8080);
    udpPort_ = getEnvOrDefaultInt("UDP_PORT", 8081);

    matchSize_ = getEnvOrDefaultInt("MATCH_SIZE", 2);
    matchmakingExpansionRate_ = getEnvOrDefaultInt("MATCH_EXPANSION_RATE", 100);
    queueTimeoutSeconds_ = getEnvOrDefaultInt("QUEUE_TIMEOUT", 300);
    reconnectTimeoutSeconds_ = getEnvOrDefaultInt("RECONNECT_TIMEOUT", 30);

    common::Logger::info("Configuration loaded from environment variables.");
}

std::string ConfigManager::getEnvOrDefault(const char* envVar, const std::string& defaultValue) {
    const char* val = std::getenv(envVar);
    return val ? std::string(val) : defaultValue;
}

int ConfigManager::getEnvOrDefaultInt(const char* envVar, int defaultValue) {
    const char* val = std::getenv(envVar);
    if (val) {
        try {
            return std::stoi(val);
        } catch (const std::exception& e) {
            common::Logger::error(std::string("Invalid integer format for environment variable ") + envVar + ": " + e.what());
        }
    }
    return defaultValue;
}

} // namespace config
} // namespace arenanet
