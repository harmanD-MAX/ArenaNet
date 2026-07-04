#pragma once

#include <string>
#include <stdexcept>

namespace arenanet {
namespace common {

enum class ErrorCode {
    SUCCESS = 0,
    UNKNOWN_ERROR = 1,
    INVALID_CREDENTIALS = 2,
    USER_ALREADY_EXISTS = 3,
    LOBBY_NOT_FOUND = 4,
    LOBBY_FULL = 5,
    DATABASE_ERROR = 6,
    REDIS_ERROR = 7,
    UNAUTHORIZED = 8,
    INVALID_PACKET = 9
};

class ArenaException : public std::runtime_error {
public:
    ArenaException(ErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}

    ErrorCode getCode() const { return code_; }

private:
    ErrorCode code_;
};

} // namespace common
} // namespace arenanet
