#include "TCPServer.h"
#include "../common/Logger.h"

namespace arenanet {
namespace network {

TCPServer::TCPServer(asio::io_context& ioContext, int port)
    : acceptor_(ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
    doAccept();
}

void TCPServer::setPacketHandler(Connection::PacketHandler handler) {
    packetHandler_ = std::move(handler);
}

void TCPServer::setDisconnectHandler(Connection::DisconnectHandler handler) {
    disconnectHandler_ = std::move(handler);
}

void TCPServer::broadcast(const Packet& packet) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (const auto& conn : connections_) {
        conn->send(packet);
    }
}

void TCPServer::addConnection(std::shared_ptr<Connection> connection) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_.insert(connection);
}

void TCPServer::removeConnection(std::shared_ptr<Connection> connection) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_.erase(connection);
}

void TCPServer::doAccept() {
    acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                common::Logger::info("New client connected.");

                auto conn = std::make_shared<Connection>(
                    std::move(socket),
                    [this](std::shared_ptr<Connection> c, const Packet& p) {
                        if (packetHandler_) {
                            packetHandler_(c, p);
                        }
                    },
                    [this](std::shared_ptr<Connection> c) {
                        common::Logger::info("Client disconnected.");
                        if (disconnectHandler_) {
                            disconnectHandler_(c);
                        }
                        removeConnection(c);
                    }
                );

                addConnection(conn);
                conn->start();
            } else {
                common::Logger::error("Failed to accept connection: " + ec.message());
            }

            doAccept();
        });
}

} // namespace network
} // namespace arenanet
