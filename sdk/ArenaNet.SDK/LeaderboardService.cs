using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace ArenaNet.SDK
{
    /// <summary>
    /// Service for retrieving player rankings and match history.
    /// </summary>
    public class LeaderboardService
    {
        private readonly ArenaNetClient _client;

        internal LeaderboardService(ArenaNetClient client)
        {
            _client = client;
        }

        /// <summary>
        /// Retrieves the global leaderboard of top players.
        /// </summary>
        public async Task<JArray> GetLeaderboardAsync()
        {
            var response = await _client.SendAndAwaitResponseAsync(40, new { }, 41); // 40=GET_LEADERBOARD, 41=RESPONSE
            return response["payload"]["leaderboard"] as JArray;
        }

        /// <summary>
        /// Retrieves the recent match history for the current player.
        /// </summary>
        public async Task<JArray> GetMatchHistoryAsync()
        {
            var response = await _client.SendAndAwaitResponseAsync(71, new { }, 72); // 71=GET_HISTORY, 72=RESPONSE
            return response["payload"]["history"] as JArray;
        }
    }
}
