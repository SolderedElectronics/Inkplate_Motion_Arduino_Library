# Simple script to send and receieve data via UDP

import socket
import threading

# Define the IP address and port of the Inkplate and the local machine
INKPLATE_IP = '192.168.0.0'  # Replace with the actual IP address of Inkplate
INKPLATE_PORT = 123
LOCAL_IP = '192.168.0.0' # Put your local IP
LOCAL_PORT = 123

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((LOCAL_IP, LOCAL_PORT))

def receive_messages():
    while True:
        data, addr = sock.recvfrom(1024)
        print(f"Received from {addr}: {data.decode()}")

def send_messages():
    while True:
        message = input("Enter message to send: ")
        sock.sendto(message.encode(), (INKPLATE_IP, INKPLATE_PORT))
        print(f"Sent: {message}")

# Start the receiving thread
recv_thread = threading.Thread(target=receive_messages)
recv_thread.daemon = True
recv_thread.start()

# Start sending messages
send_messages()
