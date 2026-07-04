#pragma once

#include <string>
#include <memory>
#include "../network/Connection.h"
#include "../network/Packet.h"

namespace arenanet {
namespace chat {

class ChatManager {
public:
    static ChatManager& getInstance() {
        static ChatManager instance;
        return instance;
    }

    void handlePacket(std::shared_ptr<network::Connection> conn, const network::Packet& packet);

private:
    ChatManager() = default;

    void handleChatMessage(std::shared_ptr<network::Connection> conn, const network::Packet& packet);
};

} // namespace chat
} // namespace arenanet
