using System;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace ArenaNet.SDK
{
    /// <summary>
    /// Service for handling matchmaking queues and match found events.
    /// </summary>
    public class MatchmakingService
    {
        private readonly ArenaNetClient _client;
        public event Action<string, JArray> OnMatchFound;

        internal MatchmakingService(ArenaNetClient client)
        {
            _client = client;
        }

        internal void HandleEvent(int type, JObject packet)
        {
            if (type == 23) // MATCH_FOUND_EVENT
            {
                var payload = packet["payload"] as JObject;
                string matchId = payload["match_id"].Value<string>();
                JArray players = payload["players"] as JArray;
                OnMatchFound?.Invoke(matchId, players);
            }
        }

        /// <summary>
        /// Joins the global matchmaking queue.
        /// <param name="opponents">Number of opponents to search for, or -1 for 'Any' match size.</param>
        /// </summary>
        public async Task<bool> JoinQueueAsync(int opponents = -1)
        {
            var response = await _client.SendAndAwaitResponseAsync(20, new { opponents = opponents }, 21); // 20=JOIN_QUEUE, 21=RESPONSE
            return response["payload"]["success"]?.Value<bool>() ?? false;
        }

        /// <summary>
        /// Leaves the matchmaking queue.
        /// </summary>
        public void LeaveQueue()
        {
            _client.SendPacket(22, new { }); // 22=LEAVE_QUEUE
        }
    }
}
