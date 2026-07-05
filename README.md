# ArenaNet (Version 1 MVP)

ArenaNet is a highly modular, high-performance C++20 multiplayer backend framework. It is designed to act as an infrastructure product allowing indie game developers to seamlessly integrate essential multiplayer features into their custom engines or commercial ones like Unity and Unreal Engine.

## Overview

Building scalable backend services for multiplayer games is a complex and repetitive task. ArenaNet provides the necessary core abstractions out of the box, ensuring reliability, clean architecture, and ease of integration.

### Features
* **Authentication**: JWT-based login and registration flow with securely hashed passwords.
* **Lobby Management**: Create, join, leave, and list manual lobbies. Players can toggle their Ready state.
* **Matchmaking**: Scalable queuing system that groups players and provisions match instances, automatically bypassing the queue for private manual lobbies when everyone readies up.
* **Real-Time Chat**: Broadcast-based lobby chat.
* **Leaderboards**: Track player Wins, Losses, and calculate global Player Rank dynamically using PostgreSQL window functions.
* **Unity Integration**: Includes a complete C# `NetworkClient.cs` and `ArenaNetUIManager.cs` for drag-and-drop Unity integration.
* **Desktop Client**: Includes a fully-functional Python Tkinter GUI desktop client for instant manual testing (`client/gui_client.py`).

## Architecture

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
* **Desktop Client:** Python (Tkinter, socket)
* **Unity Client:** C# (TcpClient, UnityEngine.UI)

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
* `3`: REGISTER_REQUEST
* `4`: REGISTER_RESPONSE
* `10`: CREATE_LOBBY_REQUEST
* `12`: JOIN_LOBBY_REQUEST
* `14`: LEAVE_LOBBY_REQUEST
* `15`: LOBBY_STATE_UPDATE
* `16`: LIST_LOBBIES_REQUEST
* `18`: PLAYER_READY_REQUEST
* `20`: JOIN_QUEUE_REQUEST
* `23`: MATCH_FOUND_EVENT
* `30`: CHAT_MESSAGE
* `40`: GET_LEADERBOARD_REQUEST
* *(Refer to `Packet.h` for the complete list of opcodes).*

## Testing via GUI Client

ArenaNet comes with a fully-functional Python desktop client for testing the server without needing to build a Unity project.

To launch the GUI client, simply run:
```bash
python3 client/gui_client.py
```

You can open multiple terminal windows and run this command multiple times to simulate multiple concurrent players interacting with the backend!


## License
MIT License. See `LICENSE` for details.
