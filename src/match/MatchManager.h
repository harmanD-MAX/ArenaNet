#pragma once

#include <memory>
#include "../network/Connection.h"
#include "../network/Packet.h"

namespace arenanet {
namespace match {

class MatchManager {
public:
    static MatchManager& getInstance() {
        static MatchManager instance;
        return instance;
    }

    void handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet);

private:
    MatchManager() = default;

    void handleReportMatchResult(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleGetMatchHistory(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
};

} // namespace match
} // namespace arenanet
