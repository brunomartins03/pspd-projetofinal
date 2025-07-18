# engine-mpi/server.py
import socket
import concurrent.futures
import json
import threading
import subprocess

HOST = '0.0.0.0'
PORT = 5000
MAX_WORKERS = 4  # adjust as needed

def run_mpi(powmin, powmax):
    try:
        result = subprocess.run(
            ["mpirun", "-np", "4", "./engine-mpi", str(powmin), str(powmax)],
            capture_output=True, text=True
        )

        if result.returncode == 0:
            return result.stdout
        else:
            return f"ERROR: {result.stderr}"
    except Exception as e:
        return f"Exception: {str(e)}"

def handle_client(conn, executor):
    try:
        data = conn.recv(1024).decode()
        if not data:
            print("[MPI] No data received, closing connection.")
            return

        payload = json.loads(data)
        powmin = int(payload.get("powmin"))
        powmax = int(payload.get("powmax"))
        print(f"[MPI] Recebido: POWMIN={powmin}, POWMAX={powmax}")

        future = executor.submit(run_mpi, powmin, powmax)
        result = future.result()  # optional timeout
        conn.sendall(result.encode())

    except Exception as e:
        print(f"[MPI] Erro ao processar requisição: {e}")
        if conn:
            conn.sendall(f"Exceção: {str(e)}".encode())
    finally:
        print(f"[MPI] Fechando conexão {conn.getpeername() if conn else '(desconhecido)'}")
        conn.close()

def start_server():
    print(f"[MPI] Starting server on {HOST}:{PORT}")
    server = socket.socket()
    server.bind((HOST, PORT))
    server.listen()

    with concurrent.futures.ProcessPoolExecutor(max_workers=MAX_WORKERS) as executor:
        while True:
            conn, addr = server.accept()
            print(f"[MPI] Conexão recebida de {addr}")
            threading.Thread(target=handle_client, args=(conn, executor), daemon=True).start()

if __name__ == "__main__":
    start_server()
