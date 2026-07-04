#pragma once

#include <string>
#include <jwt-cpp/jwt.h>
#include "../common/Types.h"

namespace arenanet {
namespace authentication {

class JwtUtils {
public:
    static std::string generateToken(common::PlayerId playerId, const std::string& secret, int expirationSeconds = 86400);
    static bool verifyToken(const std::string& token, const std::string& secret, common::PlayerId& outPlayerId);
};

} // namespace authentication
} // namespace arenanet
