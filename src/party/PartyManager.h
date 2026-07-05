#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "../common/Types.h"
#include "../network/Connection.h"
#include "../network/Packet.h"

namespace arenanet {
namespace party {

struct Party {
    std::string id;
    common::PlayerId leaderId;
    std::vector<common::PlayerId> members;
    
    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"leader_id", leaderId},
            {"members", members}
        };
    }
};

class PartyManager {
public:
    static PartyManager& getInstance() {
        static PartyManager instance;
        return instance;
    }

    void handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    
    // For Matchmaker to retrieve party members
    std::shared_ptr<Party> getPartyOfPlayer(common::PlayerId playerId);
    
    void handlePlayerDisconnect(common::PlayerId playerId);

private:
    PartyManager() = default;

    void handleCreateParty(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handlePartyInvite(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handlePartyAccept(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    void handleLeaveParty(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
    
    void broadcastPartyState(std::shared_ptr<Party> party);
    std::string generatePartyId();

    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Party>> parties_;
    std::unordered_map<common::PlayerId, std::string> playerToParty_;
};

} // namespace party
} // namespace arenanet
