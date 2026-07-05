# ArenaNet

The goal of this project was to create a clean, C++20 backend that you could actually plug into an engine like Unity or Godot. It handles everything from logging in and adding friends, to grouping up in a party and queueing for a match.

## How it works (The Architecture)

The whole thing is built in C++20. I used Asio for the networking because I wanted to handle TCP sockets asynchronously without a massive framework. 

When a game client talks to the server, it sends a tiny custom packet containing a 4-byte length header followed by some JSON payload. 

For storing data, I split things into two places:
1. **PostgreSQL**: This holds all the permanent stuff—user accounts, friend lists, and match history. I even used Postgres window functions to calculate the global leaderboard ranks dynamically.
2. **Redis**: This acts as a super-fast cache. It tracks who is currently online and handles temporary authentication tokens.

All the different systems (like lobbies and parties) live in the server's memory. To keep things thread-safe without slowing the server down, each manager uses standard mutexes to protect its state when multiple players are interacting at the exact same time.

## Features

### Session Recovery
If your internet drops out for a second while you're waiting in a lobby, you shouldn't lose your spot. The backend handles this by giving disconnected players a 30-second grace period. If you reconnect in time with your auth token, the server slides you right back into the lobby.

### Custom Lobbies
Players can create custom lobbies to play with friends. The server remembers who created the lobby and marks them as the "Host". I added server-side checks so that only the Host is allowed to kick people. The match won't start until everyone toggles their status to "Ready".

### Parties & Matchmaking
The matchmaking system runs on its own background thread. Every second, it checks the queue to see if it can pair people up. If you've been waiting too long, the algorithm slowly widens the ELO rating gap to find you a match. 

You can also invite friends to a party. If you queue up as a party, the server calculates your group's average rating and matches you against others as a single team.

---

## Testing it out (GUI Client)

I wrote a simple Python GUI client so I could test the entire backend without having to boot up Unity every time. If you want to try it out locally on macOS, here is how you set it up.

First, install the dependencies using Homebrew:
```bash
brew install cmake postgresql@15 hiredis libpqxx
```

Start Postgres and Redis:
```bash
brew services start postgresql@15
brew services start redis
```

Set up the database:
```bash
/opt/homebrew/opt/postgresql@15/bin/createuser arenanet || true
/opt/homebrew/opt/postgresql@15/bin/createdb arenanet_dev || true
/opt/homebrew/opt/postgresql@15/bin/psql -d arenanet_dev -c "ALTER DATABASE arenanet_dev OWNER TO arenanet; GRANT ALL ON SCHEMA public TO arenanet;"
/opt/homebrew/opt/postgresql@15/bin/psql -d arenanet_dev -c "ALTER USER arenanet WITH PASSWORD 'password123';"
```

Compile the server:
```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/opt/homebrew ..
make -j$(sysctl -n hw.ncpu)
```

Run the server (with env vars):
```bash
export DB_HOST=localhost DB_PORT=5432 DB_USER=arenanet DB_PASS=password123 DB_NAME=arenanet_dev REDIS_HOST=localhost REDIS_PORT=6379 JWT_SECRET=secret
./ArenaNet
```

### Step-by-Step Test Guide

Once the server is running, open two separate terminals and run `python3 client/gui_client.py` in both. Let's call your left window **Client A** and your right window **Client B**.

1. **Register & Login**
   - In Client A, type `User1` as the username and `pass123` as the password. Click Register.
   - In Client B, type `User2` as the username and `pass123` as the password. Click Register.
   - In both clients, click Login. You'll see "Login Success" in the console. Notice your Player ID at the top of the logs (e.g., ID 1 and ID 2).

2. **Social System (Adding Friends)**
   - Find your Player ID in Client B (e.g., ID: 2).
   - In Client A, type `2` in the empty box next to the Add Friend (ID) button, then click that button.
   - In Client B, type `1` in the empty box next to the Accept Friend (ID) button, then click it.
   - Click Get Friends on both to see each other's status (ONLINE).

3. **Party System (Grouping Up & Leaving)**
   - In Client A, click Create Party.
   - Type Client B's Player ID into the box and click Invite Party (ID).
   - In Client B, copy the Party ID from the incoming chat log, paste it into the box, and click Accept Party (ID).
   - You will see both clients update with the party state!
   - Click Leave Party on Client B. Client A will be notified that the party size shrunk. (Or try closing the GUI without leaving—the backend will now correctly kick you from the party automatically!)

4. **Lobby System (Custom Games)**
   - In Client A, click Create Lobby. Take note of the Lobby ID that appears in the box.
   - In Client B, paste that ID into its lobby box and click Join Lobby.
   - Chat: Type a message in the bottom text field and click Send Chat.
   - Ready System: Click Ready on both clients.
   - Kicking: In Client A (host), type Client B's ID in the box and click Kick (ID). Client B will be removed!
   - Click Leave Lobby.

5. **Matchmaking**
   - First, click Leave Party in Client A so you are both solo.
   - Have both clients click Queue Match. After a few seconds, the matchmaking algorithm will find you both and trigger a `>>> MATCH STARTED! <<<` event.
   - (If you want to test Party Matchmaking, you can group up in a party again and ONLY the party leader needs to click Queue Match!)

6. **Leaderboard & History**
   - Click Leaderboard to see everyone's current rating (1000).
   - To test rank changes, type the other player's ID into the box and click Simulate Match Win. Then check the Leaderboard and Match History again to see the ELO rating update!

---

## C# SDK Integration

I also included a C# SDK (`ArenaNet.SDK`) so you can drop this right into Unity without having to write all the networking code from scratch. Here's a quick example of how you use it:

```csharp
var client = new ArenaNetClient("127.0.0.1", 8080);
await client.ConnectAsync();

bool success = await client.Auth.LoginAsync("my_user", "password123");

client.Matchmaking.OnMatchFound += (matchId, players) => {
    Console.WriteLine($"Match Found! Match ID: {matchId}");
};
await client.Matchmaking.JoinQueueAsync();
```

## License
Apache License 2.0. See `LICENSE` for details.
