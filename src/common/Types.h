#pragma once

#include <cstdint>
#include <string>
#include <vector>

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

struct FriendInfo {
  PlayerId id;
  std::string username;
  std::string status;   // "PENDING" or "ACCEPTED"
  std::string presence; // "ONLINE", "OFFLINE", "IN_LOBBY", "IN_MATCH"
};

struct MatchHistoryEntry {
  std::string matchId;
  PlayerId winnerId;
  int durationSeconds;
  std::string timestamp;
  std::vector<std::string> players; // Usernames of participants
};

} // namespace common
} // namespace arenanet
