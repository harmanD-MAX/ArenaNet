using System;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace ArenaNet.SDK
{
    /// <summary>
    /// Service responsible for handling user authentication, including login, registration, and session recovery.
    /// </summary>
    public class AuthService
    {
        private readonly ArenaNetClient _client;

        internal AuthService(ArenaNetClient client)
        {
            _client = client;
        }

        /// <summary>
        /// Authenticates a user with a username and password and initializes the session.
        /// </summary>
        public async Task<bool> LoginAsync(string username, string password)
        {
            var payload = new { username, password };
            var response = await _client.SendAndAwaitResponseAsync(1, payload, 2); // 1=LOGIN_REQUEST, 2=LOGIN_RESPONSE
            
            var payloadObj = response["payload"];
            if (payloadObj["success"].Value<bool>())
            {
                _client.JwtToken = payloadObj["token"].Value<string>();
                _client.PlayerId = payloadObj["player_id"].Value<int>();
                return true;
            }
            throw new Exception(payloadObj["error"]?.Value<string>() ?? "Login failed");
        }

        /// <summary>
        /// Restores an existing session using a JWT token.
        /// </summary>
        public async Task<bool> ReconnectAsync(string token)
        {
            var payload = new { token };
            var response = await _client.SendAndAwaitResponseAsync(1, payload, 2); // Send LOGIN_REQUEST with token
            
            var payloadObj = response["payload"];
            if (payloadObj["success"].Value<bool>())
            {
                _client.JwtToken = payloadObj["token"]?.Value<string>() ?? token;
                _client.PlayerId = payloadObj["player_id"].Value<int>();
                return true;
            }
            return false;
        }

        /// <summary>
        /// Registers a new user account with the provided credentials.
        /// </summary>
        public async Task<bool> RegisterAsync(string username, string password)
        {
            var payload = new { username, password };
            var response = await _client.SendAndAwaitResponseAsync(3, payload, 4); // 3=REGISTER_REQUEST, 4=REGISTER_RESPONSE
            
            var payloadObj = response["payload"];
            if (payloadObj["success"].Value<bool>()) return true;
            
            throw new Exception(payloadObj["error"]?.Value<string>() ?? "Registration failed");
        }
    }
}
