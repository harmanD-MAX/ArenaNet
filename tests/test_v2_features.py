import socket
import json
import struct
import time
import threading

HOST = '127.0.0.1'
PORT = 8080

class Client:
    def __init__(self, username, password):
        self.username = username
        self.password = password
        self.token = ""
        self.player_id = 0
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((HOST, PORT))
        self.messages = []
        
        # Start listener thread
        self.listener = threading.Thread(target=self.receive_loop, daemon=True)
        self.listener.start()
        
    def send_packet(self, msg_type, payload):
        payload_str = json.dumps({"type": msg_type, "payload": payload})
        payload_bytes = payload_str.encode('utf-8')
        length_bytes = struct.pack('<I', len(payload_bytes))
        self.sock.sendall(length_bytes + payload_bytes)
        
    def receive_loop(self):
        while True:
            try:
                length_bytes = self.sock.recv(4)
                if not length_bytes: break
                length = struct.unpack('<I', length_bytes)[0]
                payload_bytes = self.sock.recv(length)
                data = json.loads(payload_bytes.decode('utf-8'))
                self.messages.append(data)
            except:
                break
                
    def wait_for(self, packet_type, timeout=5):
        start = time.time()
        while time.time() - start < timeout:
            for m in self.messages:
                if m.get("type") == packet_type:
                    self.messages.remove(m)
                    return m
            time.sleep(0.1)
        return None

def test_v2():
    print("Testing V2 Features...")
    
    # 1. Register & Login Users
    c1 = Client("Alpha", "pass123")
    c1.send_packet(3, {"username": c1.username, "password": c1.password}) # Register
    time.sleep(0.5)
    c1.send_packet(1, {"username": c1.username, "password": c1.password}) # Login
    login_resp1 = c1.wait_for(2)
    assert login_resp1 and login_resp1["payload"]["success"]
    c1.player_id = login_resp1["payload"]["player_id"]
    
    c2 = Client("Beta", "pass123")
    c2.send_packet(3, {"username": c2.username, "password": c2.password})
    time.sleep(0.5)
    c2.send_packet(1, {"username": c2.username, "password": c2.password})
    login_resp2 = c2.wait_for(2)
    assert login_resp2 and login_resp2["payload"]["success"]
    c2.player_id = login_resp2["payload"]["player_id"]
    
    # 2. Friend Request
    print("Testing Friend System...")
    c1.send_packet(50, {"target_id": c2.player_id})
    notify = c2.wait_for(80) # NOTIFICATION_EVENT
    assert notify and notify["payload"]["type"] == "FRIEND_REQUEST"
    
    c2.send_packet(51, {"sender_id": c1.player_id}) # Accept
    time.sleep(0.5)
    
    c1.send_packet(54, {}) # Get Friends
    f_resp = c1.wait_for(55)
    assert f_resp and len(f_resp["payload"]["friends"]) > 0
    assert f_resp["payload"]["friends"][0]["presence"] == "ONLINE"
    
    # 3. Party System
    print("Testing Party System...")
    c1.send_packet(60, {}) # Create Party
    p_update1 = c1.wait_for(64)
    assert p_update1
    party_id = p_update1["payload"]["id"]
    
    c1.send_packet(61, {"target_id": c2.player_id}) # Invite
    notify2 = c2.wait_for(80)
    assert notify2 and notify2["payload"]["type"] == "PARTY_INVITE"
    
    c2.send_packet(62, {"party_id": party_id}) # Accept Invite
    p_update2 = c2.wait_for(64)
    assert p_update2 and len(p_update2["payload"]["members"]) == 2
    
    # 4. Matchmaking Upgrade
    print("Testing Matchmaking (Party Queue)...")
    # c1 is party leader, so c1 queues
    c1.send_packet(20, {}) # JOIN_QUEUE_REQUEST
    q_resp = c1.wait_for(21, timeout=5)
    print("q_resp for c1:", q_resp)
    assert q_resp and q_resp["payload"]["success"]
    
    c3 = Client("Gamma", "pass123")
    c3.send_packet(3, {"username": c3.username, "password": c3.password})
    time.sleep(0.5)
    c3.send_packet(1, {"username": c3.username, "password": c3.password})
    login_resp3 = c3.wait_for(2)
    c3.player_id = login_resp3["payload"]["player_id"]
    
    c4 = Client("Delta", "pass123")
    c4.send_packet(3, {"username": c4.username, "password": c4.password})
    time.sleep(0.5)
    c4.send_packet(1, {"username": c4.username, "password": c4.password})
    login_resp4 = c4.wait_for(2)
    c4.player_id = login_resp4["payload"]["player_id"]
    
    # Make c3 and c4 form a party and queue
    c3.send_packet(60, {})
    p3 = c3.wait_for(64)
    c3.send_packet(61, {"target_id": c4.player_id})
    notify3 = c4.wait_for(80)
    c4.send_packet(62, {"party_id": p3["payload"]["id"]})
    c4.wait_for(64)
    
    c3.send_packet(20, {}) # Queue
    
    # Now we should have a 2v2 match (4 players total), but for our MVP, targetTotalPlayers is 2
    # Since our PartyManager queued c1's party (size 2), and Matchmaker is looking for targetTotalPlayers=2
    # Oh wait! In Queue.cpp we put targetTotalPlayers = 2!
    # Let's see if c1's party matches by itself (since size=2)
    
    m_event1 = c1.wait_for(23, timeout=10)
    assert m_event1
    m_event2 = c2.wait_for(23, timeout=10)
    assert m_event2
    
    match_id = m_event1["payload"]["match_id"]
    print(f"Match started with ID: {match_id}")
    
    # 5. Match History
    print("Testing Match History...")
    # Report result
    c1.send_packet(70, {
        "match_id": match_id,
        "winner_id": c1.player_id,
        "duration_seconds": 300,
        "players": [c1.player_id, c2.player_id]
    })
    time.sleep(1)
    
    c1.send_packet(71, {})
    hist_resp = c1.wait_for(72)
    assert hist_resp and len(hist_resp["payload"]["history"]) > 0
    
    print("All V2 features tested successfully!")
    
if __name__ == '__main__':
    test_v2()
