#include "ChatManager.h"
#include "../lobby/LobbyManager.h"
#include "../common/Logger.h"

namespace arenanet {
namespace chat {

void ChatManager::handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    if (conn->getPlayerId() == 0) return;

    if (packet.type == network::PacketType::CHAT_MESSAGE) {
        handleChatMessage(conn, packet);
    }
}

void ChatManager::handleChatMessage(std::shared_ptr<network::Connection> conn, const network::Packet& packet) {
    std::string lobbyId = packet.payload["lobby_id"];
    std::string message = packet.payload["message"];
    common::PlayerId senderId = conn->getPlayerId();
    
    // Broadcast message to all players in the lobby
    network::Packet broadcastPacket;
    broadcastPacket.type = network::PacketType::CHAT_MESSAGE;
    broadcastPacket.payload["lobby_id"] = lobbyId;
    broadcastPacket.payload["sender_id"] = senderId;
    broadcastPacket.payload["message"] = message;
    
    lobby::LobbyManager::getInstance().broadcastToLobby(lobbyId, broadcastPacket);
    
    common::Logger::debug("Chat message in lobby " + lobbyId + " from " + std::to_string(senderId));
}

} // namespace chat
} // namespace arenanet
