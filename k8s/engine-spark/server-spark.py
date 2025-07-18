# engine-spark/server.py
import socket
import json
import threading
import concurrent.futures
from jogo_da_vida_pyspark import main as run_spark

HOST = '0.0.0.0'
PORT = 5001
MAX_WORKERS = 4

def spark_wrapper(powmin, powmax):
    try:
        output = run_spark(powmin, powmax)
        return str(output)
    except Exception as e:
        return f"Exception: {str(e)}"

def handle_client(conn, executor):
    try:
        data = conn.recv(1024).decode()
        if not data:
            return

        payload = json.loads(data)
        powmin = int(payload.get("powmin"))
        powmax = int(payload.get("powmax"))
        print(f"[MPI] Recebido: POWMIN={powmin}, POWMAX={powmax}")

        future = executor.submit(spark_wrapper, powmin, powmax)
        result = future.result()
        conn.sendall(result.encode())

    except Exception as e:
        conn.sendall(f"Exception: {str(e)}".encode())
    finally:
        print(f"[Spark] Fechando conexão ({conn.getpeername()})")
        conn.close()

def start_server():
    print(f"[Spark] Starting server on {HOST}:{PORT}")
    server = socket.socket()
    server.bind((HOST, PORT))
    server.listen()

    with concurrent.futures.ProcessPoolExecutor(max_workers=MAX_WORKERS) as executor:
        while True:
            conn, addr = server.accept()
            print(f"[Spark] Conexão recebida de {addr}")
            threading.Thread(target=handle_client, args=(conn, executor), daemon=True).start()

if __name__ == "__main__":
    start_server()
