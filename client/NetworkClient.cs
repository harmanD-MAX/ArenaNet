using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Collections.Generic;
using UnityEngine;

// Minimal JSON parser needed in Unity.
// Recommended: Unity's built-in JsonUtility or a lightweight JSON library (like Newtonsoft.Json for Unity).
// For this demo, we assume a custom JSON wrapper or simple string manipulation for MVP.

public class NetworkClient : MonoBehaviour
{
    private TcpClient client;
    private NetworkStream stream;
    private Thread receiveThread;

    [Header("Server Settings")]
    public string serverHost = "127.0.0.1";
    public int serverPort = 8080;

    private Queue<string> mainThreadActionQueue = new Queue<string>();

    void Start()
    {
        ConnectToServer();
    }

    void ConnectToServer()
    {
        try
        {
            client = new TcpClient(serverHost, serverPort);
            stream = client.GetStream();
            
            receiveThread = new Thread(ReceiveData);
            receiveThread.IsBackground = true;
            receiveThread.Start();
            
            Debug.Log("Connected to ArenaNet server.");
        }
        catch (Exception e)
        {
            Debug.LogError("Connection error: " + e.Message);
        }
    }

    void ReceiveData()
    {
        byte[] lengthBuffer = new byte[4];
        
        while (client != null && client.Connected)
        {
            try
            {
                int bytesRead = stream.Read(lengthBuffer, 0, 4);
                if (bytesRead == 0) break; // Disconnected

                int messageLength = BitConverter.ToInt32(lengthBuffer, 0);
                
                byte[] messageBuffer = new byte[messageLength];
                bytesRead = stream.Read(messageBuffer, 0, messageLength);
                
                string jsonMessage = Encoding.UTF8.GetString(messageBuffer, 0, bytesRead);
                
                lock (mainThreadActionQueue)
                {
                    mainThreadActionQueue.Enqueue(jsonMessage);
                }
            }
            catch (Exception e)
            {
                Debug.Log("Disconnected from server: " + e.Message);
                break;
            }
        }
    }

    void Update()
    {
        lock (mainThreadActionQueue)
        {
            while (mainThreadActionQueue.Count > 0)
            {
                string message = mainThreadActionQueue.Dequeue();
                HandleServerMessage(message);
            }
        }
    }

    void HandleServerMessage(string json)
    {
        Debug.Log("Received from server: " + json);
        // Here you would parse the JSON and update the UI accordingly.
        // Example: If it's a MATCH_FOUND_EVENT, transition to the game scene.
    }

    public void SendPacket(int type, string jsonPayload)
    {
        if (client == null || !client.Connected) return;

        // Construct JSON
        string fullJson = $"{{\"type\":{type},\"payload\":{jsonPayload}}}";
        byte[] payloadBytes = Encoding.UTF8.GetBytes(fullJson);
        byte[] lengthBytes = BitConverter.GetBytes(payloadBytes.Length);
        
        byte[] finalPacket = new byte[lengthBytes.Length + payloadBytes.Length];
        Buffer.BlockCopy(lengthBytes, 0, finalPacket, 0, lengthBytes.Length);
        Buffer.BlockCopy(payloadBytes, 0, finalPacket, lengthBytes.Length, payloadBytes.Length);

        stream.Write(finalPacket, 0, finalPacket.Length);
    }

    // --- Helper functions for UI Buttons ---

    public void Login(string username, string password)
    {
        string payload = $"{{\"username\":\"{username}\",\"password\":\"{password}\"}}";
        SendPacket(1, payload); // 1 = LOGIN_REQUEST
    }

    public void Register(string username, string password)
    {
        string payload = $"{{\"username\":\"{username}\",\"password\":\"{password}\"}}";
        SendPacket(3, payload); // 3 = REGISTER_REQUEST
    }

    public void CreateLobby()
    {
        SendPacket(10, "{}"); // 10 = CREATE_LOBBY_REQUEST
    }

    public void JoinQueue()
    {
        SendPacket(20, "{}"); // 20 = JOIN_QUEUE_REQUEST
    }

    public void SendChatMessage(string lobbyId, string message)
    {
        string payload = $"{{\"lobby_id\":\"{lobbyId}\",\"message\":\"{message}\"}}";
        SendPacket(30, payload); // 30 = CHAT_MESSAGE
    }

    public void GetLeaderboard()
    {
        SendPacket(40, "{}"); // 40 = GET_LEADERBOARD_REQUEST
    }

    void OnApplicationQuit()
    {
        if (stream != null) stream.Close();
        if (client != null) client.Close();
    }
}
