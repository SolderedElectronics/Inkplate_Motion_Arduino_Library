import socket, threading, time, struct

LOCAL_PORT   = 5005
INKPLATE_IP  = ""
INKPLATE_PORT = 5005
FLOOD_INTERVAL = 0.2 # seconds between flood packets

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", LOCAL_PORT))
sock.settimeout(0.05)

py_counter  = 0
rx_from_inkplate = 0
lock = threading.Lock()

def reply_thread():
    global py_counter, rx_from_inkplate
    while True:
        try:
            data, addr = sock.recvfrom(256)
            if len(data) >= 4:
                arduino_counter = struct.unpack('<I', data[:4])[0]
                with lock:
                    rx_from_inkplate += 1
                    count = py_counter
                    py_counter += 1
                sock.sendto(struct.pack('<I', count), addr)
        except socket.timeout:
            pass

def flood_thread():
    global py_counter
    while True:
        try:
            with lock:
                count = py_counter
            sock.sendto(struct.pack('<I', count), (INKPLATE_IP, INKPLATE_PORT))
        except Exception:
            pass
        time.sleep(FLOOD_INTERVAL)

def stats_thread():
    prev_rx = 0
    while True:
        time.sleep(5)
        with lock:
            cur_rx = rx_from_inkplate
            cur_py = py_counter
        print(f"[stats] rx_from_inkplate={cur_rx} (+{cur_rx - prev_rx} in 5s)  py_sent={cur_py}")
        prev_rx = cur_rx

threading.Thread(target=reply_thread, daemon=True).start()
threading.Thread(target=flood_thread,  daemon=True).start()
threading.Thread(target=stats_thread,  daemon=True).start()

print(f"Flooding {INKPLATE_IP}:{INKPLATE_PORT} at {1/FLOOD_INTERVAL:.1f}fps. Ctrl+C to stop.")
while True:
    time.sleep(1)
