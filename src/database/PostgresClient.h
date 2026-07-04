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

private:
    PostgresClient() = default;

    std::unique_ptr<pqxx::connection> conn_;
    std::mutex dbMutex_; // Simple mutex for MVP. A connection pool would be better for production.
};

} // namespace database
} // namespace arenanet
