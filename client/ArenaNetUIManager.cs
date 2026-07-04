using UnityEngine;
using UnityEngine.UI;

/// <summary>
/// A minimal, polished UI Manager that demonstrates how a Unity game would integrate with ArenaNet.
/// This script links standard Unity UI Buttons/InputFields to the NetworkClient.
/// </summary>
public class ArenaNetUIManager : MonoBehaviour
{
    [Header("Network")]
    public NetworkClient networkClient;

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

    void Start()
    {
        // Hook up UI buttons to the Network Client
        loginBtn.onClick.AddListener(OnLoginClicked);
        registerBtn.onClick.AddListener(OnRegisterClicked);
        
        getLeaderboardBtn.onClick.AddListener(() => networkClient.GetLeaderboard());
        listLobbiesBtn.onClick.AddListener(() => networkClient.ListLobbies());
        createLobbyBtn.onClick.AddListener(() => networkClient.CreateLobby());
        matchmakeBtn.onClick.AddListener(() => networkClient.JoinQueue());
        
        readyBtn.onClick.AddListener(() => networkClient.SetReady(currentLobbyId, true));
        unreadyBtn.onClick.AddListener(() => networkClient.SetReady(currentLobbyId, false));
        leaveLobbyBtn.onClick.AddListener(OnLeaveLobbyClicked);
        sendChatBtn.onClick.AddListener(OnSendChatClicked);

        // Initial UI state
        ShowAuthPanel();
    }

    private void OnLoginClicked()
    {
        string user = usernameInput.text;
        string pass = passwordInput.text;
        if (!string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(pass))
        {
            networkClient.Login(user, pass);
            // Ideally wait for LOGIN_RESPONSE before switching panels.
            // For demo simplicity, assume success.
            ShowMainMenuPanel(); 
        }
    }

    private void OnRegisterClicked()
    {
        string user = usernameInput.text;
        string pass = passwordInput.text;
        if (!string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(pass))
        {
            networkClient.Register(user, pass);
        }
    }

    private void OnLeaveLobbyClicked()
    {
        networkClient.LeaveLobby(currentLobbyId);
        currentLobbyId = "";
        ShowMainMenuPanel();
    }

    private void OnSendChatClicked()
    {
        if (!string.IsNullOrEmpty(chatInput.text) && !string.IsNullOrEmpty(currentLobbyId))
        {
            networkClient.SendChatMessage(currentLobbyId, chatInput.text);
            chatInput.text = "";
        }
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
