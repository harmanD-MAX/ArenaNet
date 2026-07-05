#pragma once

#include <string>
#include <cstdlib>

namespace arenanet {
namespace config {

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    void loadFromEnvironment();

    // Database configuration
    const std::string& getDbHost() const { return dbHost_; }
    const std::string& getDbPort() const { return dbPort_; }
    const std::string& getDbUser() const { return dbUser_; }
    const std::string& getDbPass() const { return dbPass_; }
    const std::string& getDbName() const { return dbName_; }

    // Redis configuration
    const std::string& getRedisHost() const { return redisHost_; }
    int getRedisPort() const { return redisPort_; }

    // Authentication
    const std::string& getJwtSecret() const { return jwtSecret_; }

    // Server
    int getTcpPort() const { return tcpPort_; }
    int getUdpPort() const { return udpPort_; }

    // Game Settings
    int getMatchSize() const { return matchSize_; }
    int getMatchmakingExpansionRate() const { return matchmakingExpansionRate_; }
    int getQueueTimeoutSeconds() const { return queueTimeoutSeconds_; }
    int getReconnectTimeoutSeconds() const { return reconnectTimeoutSeconds_; }

private:
    ConfigManager() = default;

    std::string getEnvOrDefault(const char* envVar, const std::string& defaultValue);
    int getEnvOrDefaultInt(const char* envVar, int defaultValue);

    std::string dbHost_;
    std::string dbPort_;
    std::string dbUser_;
    std::string dbPass_;
    std::string dbName_;

    std::string redisHost_;
    int redisPort_;

    std::string jwtSecret_;

    int tcpPort_;
    int udpPort_;

    int matchSize_;
    int matchmakingExpansionRate_;
    int queueTimeoutSeconds_;
    int reconnectTimeoutSeconds_;
};

} // namespace config
} // namespace arenanet
