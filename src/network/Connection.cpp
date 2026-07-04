#include "Connection.h"
#include "../common/Logger.h"
#include <iostream>

namespace arenanet {
namespace network {

Connection::Connection(asio::ip::tcp::socket socket, PacketHandler onPacket, DisconnectHandler onDisconnect)
    : socket_(std::move(socket)), onPacket_(std::move(onPacket)), onDisconnect_(std::move(onDisconnect)) {
}

void Connection::start() {
    doReadHeader();
}

void Connection::send(const Packet& packet) {
    std::string serialized = packet.serialize();
    uint32_t length = serialized.size();
    
    // We send 4 bytes length header followed by the payload
    std::string data;
    data.append(reinterpret_cast<const char*>(&length), sizeof(length));
    data.append(serialized);

    bool writeInProgress = isWriting_;
    writeQueue_.push(std::move(data));
    
    if (!writeInProgress) {
        doWrite();
    }
}

void Connection::disconnect() {
    asio::error_code ec;
    socket_.close(ec);
}

std::string Connection::getRemoteAddress() const {
    asio::error_code ec;
    auto endpoint = socket_.remote_endpoint(ec);
    if (!ec) {
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }
    return "Unknown";
}

void Connection::doReadHeader() {
    readBuffer_.resize(sizeof(uint32_t));
    auto self(shared_from_this());
    
    asio::async_read(socket_, asio::buffer(readBuffer_.data(), sizeof(uint32_t)),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                std::memcpy(&incomingMessageLength_, readBuffer_.data(), sizeof(uint32_t));
                // Simple sanity check for max packet size to prevent abuse (e.g. 1MB)
                if (incomingMessageLength_ > 1024 * 1024) {
                    common::Logger::warning("Oversized packet received from " + getRemoteAddress());
                    disconnect();
                    onDisconnect_(self);
                    return;
                }
                doReadBody();
            } else {
                onDisconnect_(self);
            }
        });
}

void Connection::doReadBody() {
    readBuffer_.resize(incomingMessageLength_);
    auto self(shared_from_this());

    asio::async_read(socket_, asio::buffer(readBuffer_.data(), incomingMessageLength_),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                try {
                    Packet packet = Packet::deserialize(readBuffer_);
                    onPacket_(self, packet);
                } catch (const std::exception& e) {
                    common::Logger::error("Failed to parse packet: " + std::string(e.what()));
                }
                doReadHeader();
            } else {
                onDisconnect_(self);
            }
        });
}

void Connection::doWrite() {
    auto self(shared_from_this());
    isWriting_ = true;

    asio::async_write(socket_, asio::buffer(writeQueue_.front()),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                writeQueue_.pop();
                if (!writeQueue_.empty()) {
                    doWrite();
                } else {
                    isWriting_ = false;
                }
            } else {
                isWriting_ = false;
                onDisconnect_(self);
            }
        });
}

} // namespace network
} // namespace arenanet
