#pragma once

#include <string>
#include <memory>
#include "../network/Connection.h"
#include "../network/Packet.h"

namespace arenanet {
namespace authentication {

class AuthManager {
public:
    static AuthManager& getInstance() {
        static AuthManager instance;
        return instance;
    }

    void handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    
    // Hash a password (simple SHA256 for MVP, bcrypt/argon2 in production)
    std::string hashPassword(const std::string& password);

private:
    AuthManager() = default;

    void handleRegister(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleLogin(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
};

} // namespace authentication
} // namespace arenanet
