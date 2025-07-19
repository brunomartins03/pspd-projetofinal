import socket
import json
import threading
import time
import random

HOST = "192.168.49.2"
PORT = 30000
NUM_CLIENTS = 10
ENGINE = ["mpi", "spark"]
POWS = [(3, 5), (3, 4), (4, 6), (5, 7)]

def client_task(id):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((HOST, PORT))
            powmin, powmax = random.choice(POWS)

            payload = {
                "engine": random.choice(ENGINE),
                "powmin": powmin,
                "powmax": powmax
            }
            sock.sendall(json.dumps(payload).encode())
            response = sock.recv(4096)
            response_data = json.loads(response.decode())

            print(f"[Client {id}] Status: {response_data.get('status', 'Erro')}")
    except Exception as e:
        print(f"[Client {id}] Erro: {e}")


threads = []
start = time.time()

for i in range(NUM_CLIENTS):
    t = threading.Thread(target=client_task, args=(i,))
    threads.append(t)
    t.start()

for t in threads:
    t.join()

end = time.time()
print(f"\nFinalizado {NUM_CLIENTS} conexões em {end - start:.2f}s")
