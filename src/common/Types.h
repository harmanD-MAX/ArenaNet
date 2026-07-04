#pragma once

#include <string>
#include <cstdint>

namespace arenanet {
namespace common {

using PlayerId = uint64_t;
using LobbyId = std::string;

struct PlayerProfile {
    PlayerId id;
    std::string username;
    int rating;
    int wins;
    int losses;
};

} // namespace common
} // namespace arenanet
