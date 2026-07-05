using System;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace ArenaNet.SDK
{
    /// <summary>
    /// Service for handling friends and party features.
    /// </summary>
    public class SocialService
    {
        private readonly ArenaNetClient _client;

        public event Action<JObject> OnFriendRequestReceived;
        public event Action<JObject> OnPartyInviteReceived;
        public event Action<JObject> OnPartyStateUpdated;

        internal SocialService(ArenaNetClient client)
        {
            _client = client;
        }

        internal void HandleEvent(int type, JObject packet)
        {
            if (type == 50) OnFriendRequestReceived?.Invoke(packet["payload"] as JObject); // FRIEND_REQUEST
            if (type == 63) OnPartyInviteReceived?.Invoke(packet["payload"] as JObject); // PARTY_INVITE
            if (type == 64) OnPartyStateUpdated?.Invoke(packet["payload"] as JObject); // PARTY_STATE
        }

        /// <summary>
        /// Retrieves the list of friends for the current player.
        /// </summary>
        public async Task<JArray> GetFriendsAsync()
        {
            var response = await _client.SendAndAwaitResponseAsync(52, new { }, 53); // 52=GET_FRIENDS
            return response["payload"]["friends"] as JArray;
        }

        /// <summary>
        /// Sends a friend request to a specific player ID.
        /// </summary>
        public void SendFriendRequest(int targetId)
        {
            _client.SendPacket(50, new { target_id = targetId });
        }

        /// <summary>
        /// Accepts a pending friend request from a player.
        /// </summary>
        public void AcceptFriendRequest(int senderId)
        {
            _client.SendPacket(51, new { target_id = senderId, accept = true });
        }

        /// <summary>
        /// Creates a new party.
        /// </summary>
        public void CreateParty()
        {
            _client.SendPacket(60, new { });
        }

        /// <summary>
        /// Invites a player to the current party.
        /// </summary>
        public void InviteToParty(int targetId)
        {
            _client.SendPacket(61, new { target_id = targetId });
        }

        /// <summary>
        /// Accepts a party invite.
        /// </summary>
        public void AcceptPartyInvite(string partyId)
        {
            _client.SendPacket(62, new { party_id = partyId, accept = true });
        }

        /// <summary>
        /// Leaves the current party.
        /// </summary>
        public void LeaveParty()
        {
            _client.SendPacket(63, new { });
        }
    }
}
