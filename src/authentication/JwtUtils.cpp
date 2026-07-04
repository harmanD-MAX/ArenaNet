#include "JwtUtils.h"
#include <chrono>

namespace arenanet {
namespace authentication {

std::string JwtUtils::generateToken(common::PlayerId playerId, const std::string& secret, int expirationSeconds) {
    auto now = std::chrono::system_clock::now();
    return jwt::create()
        .set_issuer("arenanet")
        .set_type("JWS")
        .set_payload_claim("player_id", jwt::claim(std::to_string(playerId)))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::seconds(expirationSeconds))
        .sign(jwt::algorithm::hs256{secret});
}

bool JwtUtils::verifyToken(const std::string& token, const std::string& secret, common::PlayerId& outPlayerId) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret})
            .with_issuer("arenanet");
            
        verifier.verify(decoded);
        
        auto idStr = decoded.get_payload_claim("player_id").as_string();
        outPlayerId = std::stoull(idStr);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace authentication
} // namespace arenanet
