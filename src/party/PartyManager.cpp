#include "PartyManager.h"
#include "../common/Logger.h"
#include "../network/SessionManager.h"
#include <random>
#include <algorithm>

namespace arenanet {
namespace party {

void PartyManager::handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (conn->getPlayerId() == 0) return;

    switch (packet.type) {
        case network::PacketType::CREATE_PARTY_REQUEST:
            handleCreateParty(conn, packet);
            break;
        case network::PacketType::PARTY_INVITE:
            handlePartyInvite(conn, packet);
            break;
        case network::PacketType::PARTY_ACCEPT:
            handlePartyAccept(conn, packet);
            break;
        case network::PacketType::LEAVE_PARTY:
            handleLeaveParty(conn, packet);
            break;
        default:
            break;
    }
}

std::shared_ptr<Party> PartyManager::getPartyOfPlayer(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playerToParty_.find(playerId);
    if (it != playerToParty_.end()) {
        auto pIt = parties_.find(it->second);
        if (pIt != parties_.end()) {
            return pIt->second;
        }
    }
    return nullptr;
}

void PartyManager::handleCreateParty(std::shared_ptr<network::Connection> conn, const network::Packet& /*packet*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    common::PlayerId playerId = conn->getPlayerId();

    if (playerToParty_.count(playerId)) {
        common::Logger::warning("Player " + std::to_string(playerId) + " tried to create a party but is already in one.");
        return;
    }

    auto party = std::make_shared<Party>();
    party->id = generatePartyId();
    party->leaderId = playerId;
    party->members.push_back(playerId);

    parties_[party->id] = party;
    playerToParty_[playerId] = party->id;

    common::Logger::info("Player " + std::to_string(playerId) + " created party " + party->id);
    
    // Broadcast state to self
    network::Packet updatePacket;
    updatePacket.type = network::PacketType::PARTY_STATE_UPDATE;
    updatePacket.payload = party->toJson();
    conn->send(updatePacket);
}

void PartyManager::handlePartyInvite(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    common::PlayerId senderId = conn->getPlayerId();
    
    if (!playerToParty_.count(senderId)) {
        return; // Must be in a party to invite
    }

    std::string partyId = playerToParty_[senderId];
    auto party = parties_[partyId];
    
    // Only leader can invite
    if (party->leaderId != senderId) return;

    try {
        common::PlayerId targetId = packet.payload["target_id"];
        auto targetConn = network::SessionManager::getInstance().getConnection(targetId);
        if (targetConn) {
            network::Packet invite;
            invite.type = network::PacketType::NOTIFICATION_EVENT;
            invite.payload = {
                {"type", "PARTY_INVITE"},
                {"party_id", partyId},
                {"sender_id", senderId}
            };
            targetConn->send(invite);
            common::Logger::info("Player " + std::to_string(senderId) + " invited " + std::to_string(targetId) + " to party.");
        }
    } catch (const std::exception& e) {
        common::Logger::error("Error handling party invite: " + std::string(e.what()));
    }
}

void PartyManager::handlePartyAccept(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    std::lock_guard<std::mutex> lock(mutex_);
    common::PlayerId playerId = conn->getPlayerId();
    
    if (playerToParty_.count(playerId)) {
        return; // Already in a party
    }

    try {
        std::string partyId = packet.payload["party_id"];
        auto pIt = parties_.find(partyId);
        if (pIt != parties_.end()) {
            auto party = pIt->second;
            party->members.push_back(playerId);
            playerToParty_[playerId] = partyId;
            
            common::Logger::info("Player " + std::to_string(playerId) + " joined party " + partyId);
            
            // Broadcast state to all members
            network::Packet updatePacket;
            updatePacket.type = network::PacketType::PARTY_STATE_UPDATE;
            updatePacket.payload = party->toJson();
            
            for (auto mId : party->members) {
                auto mConn = network::SessionManager::getInstance().getConnection(mId);
                if (mConn) {
                    mConn->send(updatePacket);
                }
            }
        }
    } catch (const std::exception& e) {
        common::Logger::error("Error accepting party invite: " + std::string(e.what()));
    }
}

void PartyManager::handleLeaveParty(std::shared_ptr<network::Connection> conn, const network::Packet& /*packet*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    common::PlayerId playerId = conn->getPlayerId();
    
    if (!playerToParty_.count(playerId)) return;
    
    std::string partyId = playerToParty_[playerId];
    auto party = parties_[partyId];
    
    auto it = std::find(party->members.begin(), party->members.end(), playerId);
    if (it != party->members.end()) {
        party->members.erase(it);
    }
    
    playerToParty_.erase(playerId);
    
    // Notify the player they left
    network::Packet updatePacket;
    updatePacket.type = network::PacketType::PARTY_STATE_UPDATE;
    updatePacket.payload = nlohmann::json::object(); // Empty means no party
    conn->send(updatePacket);

    if (party->members.empty()) {
        parties_.erase(partyId);
        common::Logger::info("Party " + partyId + " destroyed.");
    } else {
        if (party->leaderId == playerId) {
            party->leaderId = party->members[0]; // Assign new leader
        }
        
        network::Packet bcast;
        bcast.type = network::PacketType::PARTY_STATE_UPDATE;
        bcast.payload = party->toJson();
        for (auto mId : party->members) {
            auto mConn = network::SessionManager::getInstance().getConnection(mId);
            if (mConn) {
                mConn->send(bcast);
            }
        }
    }
}

std::string PartyManager::generatePartyId() {
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string id;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    for (int i = 0; i < 6; ++i) {
        id += alphanum[dis(gen)];
    }
    return id;
}

} // namespace party
} // namespace arenanet
