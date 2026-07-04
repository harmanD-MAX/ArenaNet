import socket
import struct
import json
import time

def send_packet(sock, msg_type, payload):
    payload_str = json.dumps({"type": msg_type, "payload": payload})
    payload_bytes = payload_str.encode('utf-8')
    length_bytes = struct.pack('<I', len(payload_bytes))
    sock.sendall(length_bytes + payload_bytes)

def receive_packet(sock):
    length_bytes = sock.recv(4)
    if not length_bytes:
        return None
    length = struct.unpack('<I', length_bytes)[0]
    payload_bytes = sock.recv(length)
    return json.loads(payload_bytes.decode('utf-8'))

def test():
    print("Connecting to server...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("127.0.0.1", 8080))
    print("Connected.")

    print("Sending REGISTER_REQUEST...")
    send_packet(sock, 3, {"username": "testuser", "password": "password123"})
    res = receive_packet(sock)
    print("Register Response:", res)

    print("Sending LOGIN_REQUEST...")
    send_packet(sock, 1, {"username": "testuser", "password": "password123"})
    res = receive_packet(sock)
    print("Login Response:", res)
    token = res.get('payload', {}).get('token')
    if not token:
        print("Login failed, no token!")
        return

    print("Sending GET_LEADERBOARD_REQUEST...")
    send_packet(sock, 40, {})
    res = receive_packet(sock)
    print("Leaderboard Response:", res)
    
    sock.close()
    print("Test finished successfully.")

if __name__ == "__main__":
    test()
