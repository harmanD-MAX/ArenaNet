using UnityEngine;
using UnityEngine.UI;
using System.Threading.Tasks;
using ArenaNet.SDK;

/// <summary>
/// A minimal, polished UI Manager that demonstrates how a Unity game would integrate with ArenaNet.
/// This script links standard Unity UI Buttons/InputFields to the new ArenaNet SDK.
/// </summary>
public class ArenaNetUIManager : MonoBehaviour
{
    [Header("Network")]
    public string host = "127.0.0.1";
    public int port = 8080;
    private ArenaNetClient client;

    [Header("Login/Register UI")]
    public GameObject authPanel;
    public InputField usernameInput;
    public InputField passwordInput;
    public Button loginBtn;
    public Button registerBtn;

    [Header("Main Menu UI")]
    public GameObject mainMenuPanel;
    public Button getLeaderboardBtn;
    public Button listLobbiesBtn;
    public Button createLobbyBtn;
    public Button matchmakeBtn;

    [Header("Lobby UI")]
    public GameObject lobbyPanel;
    public Text lobbyTitleText;
    public Button readyBtn;
    public Button unreadyBtn;
    public Button leaveLobbyBtn;
    public InputField chatInput;
    public Button sendChatBtn;
    
    // Internal state
    private string currentLobbyId = "";

    async void Start()
    {
        client = new ArenaNetClient(host, port);
        
        try {
            await client.ConnectAsync();
            Debug.Log("Connected to ArenaNet!");
        } catch (System.Exception e) {
            Debug.LogError("Failed to connect: " + e.Message);
        }

        // Hook up UI buttons to the SDK
        loginBtn.onClick.AddListener(OnLoginClicked);
        registerBtn.onClick.AddListener(OnRegisterClicked);
        
        getLeaderboardBtn.onClick.AddListener(async () => await client.Leaderboard.GetLeaderboardAsync());
        createLobbyBtn.onClick.AddListener(async () => {
            currentLobbyId = await client.Lobby.CreateLobbyAsync();
            OnJoinedLobby(currentLobbyId);
        });
        matchmakeBtn.onClick.AddListener(async () => await client.Matchmaking.JoinQueueAsync());
        
        readyBtn.onClick.AddListener(async () => await client.Lobby.SetReadyAsync(currentLobbyId, true));
        unreadyBtn.onClick.AddListener(async () => await client.Lobby.SetReadyAsync(currentLobbyId, false));
        leaveLobbyBtn.onClick.AddListener(OnLeaveLobbyClicked);
        sendChatBtn.onClick.AddListener(OnSendChatClicked);

        // Initial UI state
        ShowAuthPanel();
    }

    private async void OnLoginClicked()
    {
        string user = usernameInput.text;
        string pass = passwordInput.text;
        if (!string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(pass))
        {
            try {
                await client.Auth.LoginAsync(user, pass);
                ShowMainMenuPanel();
            } catch (System.Exception e) {
                Debug.LogError("Login failed: " + e.Message);
            }
        }
    }

    private async void OnRegisterClicked()
    {
        string user = usernameInput.text;
        string pass = passwordInput.text;
        if (!string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(pass))
        {
            try {
                await client.Auth.RegisterAsync(user, pass);
                Debug.Log("Registered!");
            } catch (System.Exception e) {
                Debug.LogError("Register failed: " + e.Message);
            }
        }
    }

    private async void OnLeaveLobbyClicked()
    {
        await client.Lobby.LeaveLobbyAsync(currentLobbyId);
        currentLobbyId = "";
        ShowMainMenuPanel();
    }

    private void OnSendChatClicked()
    {
        if (!string.IsNullOrEmpty(chatInput.text) && !string.IsNullOrEmpty(currentLobbyId))
        {
            client.Chat.SendMessage(currentLobbyId, chatInput.text);
            chatInput.text = "";
        }
    }

    void OnDestroy()
    {
        client?.Dispose();
    }

    // Call this from NetworkClient when CREATE_LOBBY_RESPONSE or JOIN_LOBBY_RESPONSE is received.
    public void OnJoinedLobby(string lobbyId)
    {
        currentLobbyId = lobbyId;
        lobbyTitleText.text = $"Lobby: {lobbyId}";
        ShowLobbyPanel();
    }

    // UI State Management
    private void ShowAuthPanel()
    {
        authPanel.SetActive(true);
        mainMenuPanel.SetActive(false);
        lobbyPanel.SetActive(false);
    }

    private void ShowMainMenuPanel()
    {
        authPanel.SetActive(false);
        mainMenuPanel.SetActive(true);
        lobbyPanel.SetActive(false);
    }

    private void ShowLobbyPanel()
    {
        authPanel.SetActive(false);
        mainMenuPanel.SetActive(false);
        lobbyPanel.SetActive(true);
    }
}
