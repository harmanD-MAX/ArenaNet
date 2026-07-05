#pragma once

#include <asio.hpp>
#include <memory>
#include <string>
#include <queue>
#include <functional>
#include "../common/Types.h"
#include "Packet.h"

namespace arenanet {
namespace network {

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using PacketHandler = std::function<void(std::shared_ptr<Connection>, const Packet&)>;
    using DisconnectHandler = std::function<void(std::shared_ptr<Connection>)>;

    explicit Connection(asio::ip::tcp::socket socket, PacketHandler onPacket, DisconnectHandler onDisconnect);

    void start();
    void send(const Packet& packet);
    void disconnect();

    // Attach custom data (e.g., player ID once authenticated)
    void setPlayerId(common::PlayerId id) { playerId_ = id; }
    common::PlayerId getPlayerId() const { return playerId_; }

    std::string getRemoteAddress() const;

private:
    void doReadHeader();
    void doReadBody();
    void doWrite();

    asio::ip::tcp::socket socket_;
    PacketHandler onPacket_;
    DisconnectHandler onDisconnect_;

    std::string readBuffer_;
    uint32_t incomingMessageLength_ = 0;
    
    std::queue<std::string> writeQueue_;
    bool isWriting_ = false;

    common::PlayerId playerId_ = 0; // 0 means unauthenticated
};

} // namespace network
} // namespace arenanet
