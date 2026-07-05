using System;
using System.Threading.Tasks;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace ArenaNet.SDK
{
    /// <summary>
    /// Service for managing lobbies, player readiness, and game sessions.
    /// </summary>
    public class LobbyService
    {
        private readonly ArenaNetClient _client;
        public event Action<JObject> OnLobbyStateUpdated;

        internal LobbyService(ArenaNetClient client)
        {
            _client = client;
        }

        internal void HandleEvent(int type, JObject packet)
        {
            if (type == 15) // LOBBY_STATE_UPDATE
            {
                OnLobbyStateUpdated?.Invoke(packet["payload"] as JObject);
            }
        }

        /// <summary>
        /// Creates a new lobby and returns the lobby ID.
        /// </summary>
        public async Task<string> CreateLobbyAsync(bool isPrivate = false, int maxCapacity = 4)
        {
            var payload = new { is_private = isPrivate, max_capacity = maxCapacity };
            var response = await _client.SendAndAwaitResponseAsync(10, payload, 11); // 10=CREATE, 11=RESPONSE
            
            var payloadObj = response["payload"];
            if (payloadObj["success"].Value<bool>())
            {
                return payloadObj["lobby_id"].Value<string>();
            }
            throw new Exception(payloadObj["error"]?.Value<string>() ?? "Create lobby failed");
        }

        /// <summary>
        /// Joins an existing lobby by its ID.
        /// </summary>
        public async Task<bool> JoinLobbyAsync(string lobbyId)
        {
            var payload = new { lobby_id = lobbyId };
            var response = await _client.SendAndAwaitResponseAsync(12, payload, 13); // 12=JOIN, 13=RESPONSE
            
            var payloadObj = response["payload"];
            if (!payloadObj["success"].Value<bool>())
            {
                throw new Exception(payloadObj["error"]?.Value<string>() ?? "Join lobby failed");
            }
            return true;
        }

        /// <summary>
        /// Leaves the currently connected lobby.
        /// </summary>
        public async Task LeaveLobbyAsync(string lobbyId)
        {
            var payload = new { lobby_id = lobbyId };
            _client.SendPacket(14, payload); // 14=LEAVE
            await Task.Delay(50); // Small wait
        }

        /// <summary>
        /// Sets the ready state of the current player in the lobby.
        /// </summary>
        public async Task SetReadyAsync(string lobbyId, bool isReady)
        {
            var payload = new { lobby_id = lobbyId, is_ready = isReady };
            _client.SendPacket(18, payload); // 18=PLAYER_READY
            await Task.Delay(50);
        }

        /// <summary>
        /// Kicks a specific player from the lobby (Host only).
        /// </summary>
        public async Task KickPlayerAsync(string lobbyId, int targetId)
        {
            var payload = new { lobby_id = lobbyId, target_id = targetId };
            _client.SendPacket(19, payload); // 19=KICK_PLAYER (new opcode)
            await Task.Delay(50);
        }
    }
}
