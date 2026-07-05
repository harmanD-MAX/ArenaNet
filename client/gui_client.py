import tkinter as tk
from tkinter import messagebox, scrolledtext
import socket
import struct
import json
import threading
import time

class ArenaNetGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ArenaNet V3")
        self.root.geometry("600x750")
        
        self.sock = None
        self.connected = False
        self.current_lobby = ""
        self.token = ""
        self.player_id = ""
        
        self.setup_ui()
        
    def setup_ui(self):
        # Top Frame - Connection & Auth
        top_frame = tk.Frame(self.root)
        top_frame.pack(pady=10, fill=tk.X, padx=10)
        
        tk.Label(top_frame, text="Username:").grid(row=0, column=0, padx=5)
        self.user_entry = tk.Entry(top_frame, width=15)
        self.user_entry.grid(row=0, column=1, padx=5)
        self.user_entry.insert(0, "TestUser1")
        
        tk.Label(top_frame, text="Password:").grid(row=0, column=2, padx=5)
        self.pass_entry = tk.Entry(top_frame, show="*", width=15)
        self.pass_entry.grid(row=0, column=3, padx=5)
        self.pass_entry.insert(0, "password123")
        
        tk.Button(top_frame, text="Register", command=self.do_register).grid(row=1, column=1, pady=5)
        tk.Button(top_frame, text="Login", command=self.do_login).grid(row=1, column=2, pady=5)
        tk.Button(top_frame, text="Reconnect (Token)", command=self.do_reconnect).grid(row=1, column=3, pady=5)
        
        # Middle Frame - Lobby
        mid_frame = tk.Frame(self.root)
        mid_frame.pack(pady=10, fill=tk.X, padx=10)
        
        tk.Button(mid_frame, text="Create Lobby", command=self.do_create_lobby).grid(row=0, column=0, padx=5)
        
        self.is_private_var = tk.BooleanVar()
        tk.Checkbutton(mid_frame, text="Private", variable=self.is_private_var).grid(row=0, column=1, padx=5)
        
        tk.Button(mid_frame, text="List Lobbies", command=self.do_list_lobbies).grid(row=0, column=2, padx=5)
        
        self.lobby_id_entry = tk.Entry(mid_frame, width=10)
        self.lobby_id_entry.grid(row=0, column=3, padx=5)
        tk.Button(mid_frame, text="Join Lobby", command=self.do_join_lobby).grid(row=0, column=4, padx=5)
        tk.Button(mid_frame, text="Leave Lobby", command=self.do_leave_lobby).grid(row=0, column=5, padx=5)
        
        tk.Button(mid_frame, text="Ready", command=lambda: self.do_ready(True)).grid(row=1, column=0, pady=5)
        tk.Button(mid_frame, text="Unready", command=lambda: self.do_ready(False)).grid(row=1, column=1, pady=5)
        tk.Button(mid_frame, text="Kick (ID)", command=self.do_kick).grid(row=1, column=2, pady=5)
        tk.Button(mid_frame, text="Leaderboard", command=self.do_leaderboard).grid(row=1, column=3, pady=5)

        # V2 Frame - Friends, Party, Match History
        v2_frame = tk.Frame(self.root)
        v2_frame.pack(pady=10, fill=tk.X, padx=10)
        
        tk.Button(v2_frame, text="Get Friends", command=self.do_get_friends).grid(row=0, column=0, padx=5)
        
        self.target_entry = tk.Entry(v2_frame, width=10)
        self.target_entry.grid(row=0, column=1, padx=5)
        tk.Button(v2_frame, text="Add Friend (ID)", command=self.do_add_friend).grid(row=0, column=2, padx=5)
        tk.Button(v2_frame, text="Accept Friend (ID)", command=self.do_accept_friend).grid(row=0, column=3, padx=5)
        
        tk.Button(v2_frame, text="Create Party", command=self.do_create_party).grid(row=1, column=0, pady=5)
        tk.Button(v2_frame, text="Invite Party (ID)", command=self.do_invite_party).grid(row=1, column=1, pady=5)
        tk.Button(v2_frame, text="Accept Party (ID)", command=self.do_accept_party).grid(row=1, column=2, pady=5)
        tk.Button(v2_frame, text="Leave Party", command=self.do_leave_party).grid(row=1, column=3, pady=5)
        
        tk.Button(v2_frame, text="Queue Match", command=self.do_queue_match).grid(row=2, column=0, pady=5)
        tk.Button(v2_frame, text="Match History", command=self.do_match_history).grid(row=2, column=1, pady=5)
        tk.Button(v2_frame, text="Simulate Match Win", command=self.do_simulate_match_win).grid(row=2, column=2, pady=5)

        # Bottom Frame - Chat & Logs
        bottom_frame = tk.Frame(self.root)
        bottom_frame.pack(pady=10, fill=tk.BOTH, expand=True, padx=10)
        
        self.chat_entry = tk.Entry(bottom_frame)
        self.chat_entry.pack(fill=tk.X, pady=5)
        tk.Button(bottom_frame, text="Send Chat", command=self.do_chat).pack()
        
        self.log_area = scrolledtext.ScrolledText(bottom_frame, height=20)
        self.log_area.pack(fill=tk.BOTH, expand=True, pady=5)
        
    def log(self, msg):
        self.log_area.insert(tk.END, msg + "\n")
        self.log_area.see(tk.END)
        
    def connect_if_needed(self):
        if not self.connected:
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.connect(("127.0.0.1", 8080))
                self.connected = True
                
                # Start listener thread
                t = threading.Thread(target=self.receive_loop)
                t.daemon = True
                t.start()
                self.log("Connected to server!")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to connect: {e}")
                
    def send_packet(self, msg_type, payload):
        self.connect_if_needed()
        if self.connected:
            payload_str = json.dumps({"type": msg_type, "payload": payload})
            payload_bytes = payload_str.encode('utf-8')
            length_bytes = struct.pack('<I', len(payload_bytes))
            self.sock.sendall(length_bytes + payload_bytes)
            
    def receive_loop(self):
        while self.connected:
            try:
                length_bytes = self.sock.recv(4)
                if not length_bytes:
                    break
                length = struct.unpack('<I', length_bytes)[0]
                payload_bytes = self.sock.recv(length)
                data = json.loads(payload_bytes.decode('utf-8'))
                
                # Handle in UI thread
                self.root.after(0, self.handle_packet, data)
            except:
                break
        self.connected = False
        self.root.after(0, self.log, "Disconnected from server.")

    def handle_packet(self, data):
        ptype = data.get("type")
        payload = data.get("payload", {})
        
        if ptype == 2: # LOGIN_RESPONSE
            if payload.get("success"):
                self.token = payload.get("token")
                self.player_id = payload.get("player_id")
                self.log(f"Login Success! Player ID: {self.player_id}")
            else:
                self.log("Login Failed: " + payload.get("error_msg", ""))
        elif ptype == 4: # REGISTER_RESPONSE
            if payload.get("success"):
                self.log("Register Success! You can now login.")
            else:
                self.log("Register Failed: " + payload.get("error_msg", ""))
        elif ptype == 11: # CREATE_LOBBY_RESPONSE
            self.current_lobby = payload.get("lobby_id")
            self.log(f"Created Lobby: {self.current_lobby}")
            self.lobby_id_entry.delete(0, tk.END)
            self.lobby_id_entry.insert(0, self.current_lobby)
        elif ptype == 13: # JOIN_LOBBY_RESPONSE
            if payload.get("success"):
                self.current_lobby = payload.get("lobby_id")
                self.log(f"Joined Lobby: {self.current_lobby}")
            else:
                self.log("Failed to join lobby.")
        elif ptype == 15: # LOBBY_STATE_UPDATE
            self.log(f"Lobby State: {json.dumps(payload)}")
        elif ptype == 23: # MATCH_FOUND_EVENT
            self.log(f">>> MATCH STARTED! ID: {payload.get('match_id')} <<<")
            self.current_lobby = payload.get("match_id") # Match ID becomes chat ID
        elif ptype == 30: # CHAT_MESSAGE
            sender = payload.get("sender_id")
            msg = payload.get("message")
            self.log(f"[Chat] {sender}: {msg}")
        elif ptype == 17: # LIST_LOBBIES_RESPONSE
            lobbies = payload.get("lobbies", [])
            self.log(f"Active Lobbies: {len(lobbies)}")
            for l in lobbies:
                self.log(f" - {l['lobby_id']} (Players: {l['player_count']})")
        elif ptype == 41: # GET_LEADERBOARD_RESPONSE
            self.log("--- Leaderboard ---")
            for entry in payload.get("leaderboard", []):
                self.log(f"{entry['username']} - Rating: {entry['rating']}")
        elif ptype == 55: # GET_FRIENDS_LIST_RESPONSE
            self.log("--- Friends List ---")
            for f in payload.get("friends", []):
                self.log(f"ID: {f['id']} | {f['username']} | Status: {f['status']} | Presence: {f['presence']}")
        elif ptype == 64: # PARTY_STATE_UPDATE
            self.log(f"Party State: {json.dumps(payload)}")
        elif ptype == 72: # GET_MATCH_HISTORY_RESPONSE
            self.log("--- Match History ---")
            for m in payload.get("history", []):
                self.log(f"Match {m['match_id']} | Winner: {m['winner_id']} | Time: {m['timestamp']}")
        elif ptype == 80: # NOTIFICATION_EVENT
            self.log(f"Notification: {json.dumps(payload)}")
        else:
            self.log(f"Received (Op {ptype}): {payload}")

    def do_register(self):
        self.send_packet(3, {"username": self.user_entry.get(), "password": self.pass_entry.get()})
        
    def do_login(self):
        self.send_packet(1, {"username": self.user_entry.get(), "password": self.pass_entry.get()})
        
    def do_reconnect(self):
        if self.token:
            self.send_packet(1, {"token": self.token})
        else:
            self.log("No token available. Please login first.")
        
    def do_create_lobby(self):
        self.send_packet(10, {"is_private": self.is_private_var.get(), "max_capacity": 4})
        
    def do_list_lobbies(self):
        self.send_packet(16, {})
        
    def do_join_lobby(self):
        lobby_id = self.lobby_id_entry.get()
        if lobby_id:
            self.send_packet(12, {"lobby_id": lobby_id})
            
    def do_leave_lobby(self):
        if self.current_lobby:
            self.send_packet(14, {"lobby_id": self.current_lobby})
            self.log(f"Left Lobby: {self.current_lobby}")
            self.current_lobby = ""
            
    def do_ready(self, state):
        if self.current_lobby:
            self.send_packet(18, {"lobby_id": self.current_lobby, "is_ready": state})
            
    def do_kick(self):
        target = self.lobby_id_entry.get()
        if target.isdigit() and self.current_lobby:
            self.send_packet(19, {"lobby_id": self.current_lobby, "target_id": int(target)})
            self.log(f"Kicked player {target} from lobby")
            
    def do_chat(self):
        msg = self.chat_entry.get()
        if msg and self.current_lobby:
            self.send_packet(30, {"lobby_id": self.current_lobby, "message": msg})
            self.chat_entry.delete(0, tk.END)

    def do_leaderboard(self):
        self.send_packet(40, {})
        
    def do_get_friends(self):
        self.send_packet(54, {})
        
    def do_add_friend(self):
        target = self.target_entry.get()
        if target.isdigit():
            self.send_packet(50, {"target_id": int(target)})
            self.log(f"Sent friend request to ID {target}")
            
    def do_accept_friend(self):
        target = self.target_entry.get()
        if target.isdigit():
            self.send_packet(51, {"sender_id": int(target)})
            self.log(f"Accepted friend request from ID {target}")
            
    def do_create_party(self):
        self.send_packet(60, {})
        
    def do_invite_party(self):
        target = self.target_entry.get()
        if target.isdigit():
            self.send_packet(61, {"target_id": int(target)})
            self.log(f"Sent party invite to ID {target}")
            
    def do_accept_party(self):
        party_id = self.target_entry.get()
        if party_id:
            self.send_packet(62, {"party_id": party_id})
            self.log(f"Accepted party invite for {party_id}")
            
    def do_leave_party(self):
        self.send_packet(63, {})
        self.log("Requested to leave party.")
        
    def do_queue_match(self):
        self.send_packet(20, {}) # JOIN_QUEUE_REQUEST
        self.log("Joined matchmaking queue.")
        
    def do_match_history(self):
        self.send_packet(71, {})
        
    def do_simulate_match_win(self):
        if not self.current_lobby.startswith("match_"):
            self.log("You must be in a match to simulate a win!")
            return
            
        target = self.target_entry.get()
        players_list = [self.player_id]
        if target.isdigit():
            players_list.append(int(target))
        
        # In a real game, the server or a trusted authority reports this.
        # For testing, we send a mock packet with ourselves as winner and the target as loser.
        self.send_packet(70, {
            "match_id": self.current_lobby,
            "winner_id": self.player_id,
            "duration_seconds": 300,
            "players": players_list
        })
        self.log(f"Reported match {self.current_lobby} as won against {target}!")

if __name__ == "__main__":
    root = tk.Tk()
    app = ArenaNetGUI(root)
    root.mainloop()
