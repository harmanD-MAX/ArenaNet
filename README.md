# ArenaNet

ArenaNet is a highly modular, high-performance C++20 multiplayer backend framework. It is designed to act as an infrastructure product (similar to services built by Demonware or Nakama) allowing indie game developers to seamlessly integrate essential multiplayer features into their custom engines or commercial ones like Unity and Unreal Engine.

## Overview

Building scalable backend services for multiplayer games is a complex and repetitive task. ArenaNet provides the necessary core abstractions out of the box, ensuring reliability, clean architecture, and ease of integration.

### Features
* **Authentication**: JWT-based login and registration flow with securely hashed passwords.
* **Lobby Management**: Create, join, and manage lobbies with ready-state synchronization.
* **Matchmaking**: Scalable queuing system that groups players and provisions match instances.
* **Real-Time Chat**: Broadcast-based lobby chat.
* **Leaderboards**: Track player Wins, Losses, and Elo Ratings.
* **Player Profiles**: Persistent user statistics.

## Architecture

ArenaNet strictly adheres to modular and clean architecture principles. There are no "God classes." Responsibilities are tightly scoped to specific directories and namespaces.

```text
ArenaNet/
├── CMakeLists.txt

├── docs/
│   └── Architecture.md
├── src/
│   ├── authentication/  # JWT generation and DB verification
│   ├── chat/            # Chat broadcasting logic
│   ├── common/          # Shared utilities (Logger, Types, Errors)
│   ├── config/          # Environment variable loading
│   ├── database/        # PostgreSQL wrappers and schema definitions
│   ├── leaderboard/     # Leaderboard fetching logic
│   ├── lobby/           # Lobby state management
│   ├── matchmaking/     # Match queuing and player grouping
│   ├── network/         # Asio TCP/UDP Servers and packet serialization
│   ├── redis/           # Redis connection for transient data
│   └── main.cpp
└── tests/               # Unit and integration tests
```

## Technologies Used
* **Language:** C++20
* **Networking:** Asio (Standalone)
* **Serialization:** nlohmann/json
* **Database:** PostgreSQL (libpqxx)
* **Cache:** Redis (redis-plus-plus)
* **Authentication:** JWT (jwt-cpp)
* **Build System:** CMake

## Local Development Setup

To run ArenaNet locally on macOS, you can use Homebrew to install the necessary dependencies.

### 1. Install Dependencies
```bash
brew install cmake postgresql@15 hiredis libpqxx
```

### 2. Start Services
Start the PostgreSQL and Redis services natively via Homebrew:
```bash
brew services start postgresql@15
brew services start redis
```

### 3. Create the Database
Set up the `arenanet` PostgreSQL user and database:
```bash
/opt/homebrew/opt/postgresql@15/bin/createuser arenanet || true
/opt/homebrew/opt/postgresql@15/bin/createdb arenanet_dev || true
/opt/homebrew/opt/postgresql@15/bin/psql -d arenanet_dev -c "ALTER DATABASE arenanet_dev OWNER TO arenanet; GRANT ALL ON SCHEMA public TO arenanet;"
/opt/homebrew/opt/postgresql@15/bin/psql -d arenanet_dev -c "ALTER USER arenanet WITH PASSWORD 'password123';"
```

### 4. Build the Server
```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/opt/homebrew ..
make -j$(sysctl -n hw.ncpu)
```

### 5. Run the Server
The configuration is loaded from environment variables.
```bash
export DB_HOST=localhost
export DB_PORT=5432
export DB_USER=arenanet
export DB_PASS=password123
export DB_NAME=arenanet_dev
export REDIS_HOST=localhost
export REDIS_PORT=6379
export JWT_SECRET=super_secret_jwt_key_for_arenanet_dev

./ArenaNet
```

## API / Protocol Overview

ArenaNet communicates over TCP (and optionally UDP for game state). We use a lightweight custom framing protocol:
`[4 Bytes Length Header (Little Endian)] [JSON Payload]`

### Packet Types
* `1`: LOGIN_REQUEST
* `2`: LOGIN_RESPONSE
* `10`: CREATE_LOBBY_REQUEST
* `15`: LOBBY_STATE_UPDATE
* `20`: JOIN_QUEUE_REQUEST
* `23`: MATCH_FOUND_EVENT
* `30`: CHAT_MESSAGE
* *(Refer to `Packet.h` for the complete list of opcodes).*

## Screenshots
*(UI Demo client screenshots placeholder - to be added once demo client is complete)*

## License
MIT License. See `LICENSE` for details.
