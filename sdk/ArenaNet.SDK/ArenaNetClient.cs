using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Collections.Concurrent;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;

namespace ArenaNet.SDK
{
    public class ArenaNetClient : IDisposable
    {
        private TcpClient _client;
        private NetworkStream _stream;
        private Thread _receiveThread;
        private CancellationTokenSource _cancellationTokenSource;
        
        public string Host { get; private set; }
        public int Port { get; private set; }
        public bool IsConnected => _client != null && _client.Connected;

        // SDK Services
        public AuthService Auth { get; private set; }
        public LobbyService Lobby { get; private set; }
        public MatchmakingService Matchmaking { get; private set; }
        public ChatService Chat { get; private set; }
        public LeaderboardService Leaderboard { get; private set; }
        public SocialService Social { get; private set; }

        // Session
        public string JwtToken { get; internal set; }
        public int PlayerId { get; internal set; }

        // Events
        public event Action OnDisconnected;
        public event Action<JObject> OnPacketReceived;

        private ConcurrentDictionary<int, Action<JObject>> _responseHandlers = new ConcurrentDictionary<int, Action<JObject>>();

        public ArenaNetClient(string host, int port)
        {
            Host = host;
            Port = port;
            
            Auth = new AuthService(this);
            Lobby = new LobbyService(this);
            Matchmaking = new MatchmakingService(this);
            Chat = new ChatService(this);
            Leaderboard = new LeaderboardService(this);
            Social = new SocialService(this);
        }

        public async Task ConnectAsync()
        {
            if (IsConnected) return;

            try
            {
                _client = new TcpClient();
                await _client.ConnectAsync(Host, Port);
                _stream = _client.GetStream();

                _cancellationTokenSource = new CancellationTokenSource();
                _receiveThread = new Thread(ReceiveData);
                _receiveThread.IsBackground = true;
                _receiveThread.Start();
            }
            catch (Exception ex)
            {
                throw new Exception($"Failed to connect to {Host}:{Port}", ex);
            }
        }

        private void ReceiveData()
        {
            byte[] lengthBuffer = new byte[4];

            try
            {
                while (!_cancellationTokenSource.IsCancellationRequested && IsConnected)
                {
                    int bytesRead = _stream.Read(lengthBuffer, 0, 4);
                    if (bytesRead == 0) break; // Disconnected

                    int messageLength = BitConverter.ToInt32(lengthBuffer, 0);
                    byte[] messageBuffer = new byte[messageLength];
                    
                    int totalRead = 0;
                    while (totalRead < messageLength)
                    {
                        int read = _stream.Read(messageBuffer, totalRead, messageLength - totalRead);
                        if (read == 0) throw new Exception("Connection closed while reading packet payload");
                        totalRead += read;
                    }

                    string jsonMessage = Encoding.UTF8.GetString(messageBuffer);
                    var packet = JObject.Parse(jsonMessage);
                    
                    HandleIncomingPacket(packet);
                }
            }
            catch (Exception)
            {
                // Disconnected
            }
            finally
            {
                Disconnect();
            }
        }

        private void HandleIncomingPacket(JObject packet)
        {
            int type = packet["type"].Value<int>();
            
            // Allow awaiting specific responses
            if (_responseHandlers.TryRemove(type, out var handler))
            {
                handler.Invoke(packet);
            }
            
            OnPacketReceived?.Invoke(packet);
            
            // Route to services
            Matchmaking.HandleEvent(type, packet);
            Chat.HandleEvent(type, packet);
            Lobby.HandleEvent(type, packet);
        }

        internal void SendPacket(int type, object payload)
        {
            if (!IsConnected) throw new InvalidOperationException("Not connected to server");

            string payloadJson = JsonConvert.SerializeObject(payload);
            string fullJson = $"{{\"type\":{type},\"payload\":{payloadJson}}}";
            
            byte[] payloadBytes = Encoding.UTF8.GetBytes(fullJson);
            byte[] lengthBytes = BitConverter.GetBytes(payloadBytes.Length);
            
            byte[] finalPacket = new byte[lengthBytes.Length + payloadBytes.Length];
            Buffer.BlockCopy(lengthBytes, 0, finalPacket, 0, lengthBytes.Length);
            Buffer.BlockCopy(payloadBytes, 0, finalPacket, lengthBytes.Length, payloadBytes.Length);

            _stream.Write(finalPacket, 0, finalPacket.Length);
        }

        internal Task<JObject> SendAndAwaitResponseAsync(int requestType, object payload, int expectedResponseType, int timeoutMs = 5000)
        {
            var tcs = new TaskCompletionSource<JObject>();
            
            _responseHandlers[expectedResponseType] = (response) => 
            {
                tcs.TrySetResult(response);
            };

            SendPacket(requestType, payload);

            // Timeout handling
            Task.Delay(timeoutMs).ContinueWith(t => 
            {
                if (tcs.TrySetException(new TimeoutException("Request timed out waiting for response.")))
                {
                    _responseHandlers.TryRemove(expectedResponseType, out _);
                }
            });

            return tcs.Task;
        }

        public void Disconnect()
        {
            _cancellationTokenSource?.Cancel();
            _stream?.Close();
            _client?.Close();
            OnDisconnected?.Invoke();
        }

        public void Dispose()
        {
            Disconnect();
        }
    }
}
