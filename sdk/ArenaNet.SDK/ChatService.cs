using System;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace ArenaNet.SDK
{
    public class ChatService
    {
        private readonly ArenaNetClient _client;
        public event Action<string, string, string> OnMessageReceived; // LobbyId, SenderName, Message

        internal ChatService(ArenaNetClient client)
        {
            _client = client;
        }

        internal void HandleEvent(int type, JObject packet)
        {
            if (type == 30) // CHAT_MESSAGE
            {
                var payload = packet["payload"] as JObject;
                string lobbyId = payload["lobby_id"].Value<string>();
                string sender = payload["sender_name"].Value<string>();
                string msg = payload["message"].Value<string>();
                OnMessageReceived?.Invoke(lobbyId, sender, msg);
            }
        }

        public void SendMessage(string lobbyId, string message)
        {
            var payload = new { lobby_id = lobbyId, message = message };
            _client.SendPacket(30, payload); // 30=CHAT_MESSAGE
        }
    }
}
