#pragma once

#include <asio.hpp>
#include <memory>
#include <unordered_set>
#include <mutex>
#include "Connection.h"

namespace arenanet {
namespace network {

class TCPServer {
public:
    TCPServer(asio::io_context& ioContext, int port);

    void setPacketHandler(Connection::PacketHandler handler);
    void setDisconnectHandler(Connection::DisconnectHandler handler);

    void broadcast(const Packet& packet);
    
    // Thread-safe access to connections
    void addConnection(std::shared_ptr<Connection> connection);
    void removeConnection(std::shared_ptr<Connection> connection);

private:
    void doAccept();

    asio::ip::tcp::acceptor acceptor_;
    std::unordered_set<std::shared_ptr<Connection>> connections_;
    std::mutex connectionsMutex_;
    
    Connection::PacketHandler packetHandler_;
    Connection::DisconnectHandler disconnectHandler_;
};

} // namespace network
} // namespace arenanet
