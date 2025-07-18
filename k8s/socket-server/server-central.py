import socket
import json
import uuid
import threading
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from elasticsearch import Elasticsearch

# === CONFIGS ===
MPI_ENGINE_HOST = "engine-mpi"
SPK_ENGINE_HOST = "engine-spark"
HOST = "0.0.0.0"
MPI_ENGINE_PORT = 5000
SPK_ENGINE_PORT = 5001
TCP_PORT = 3000
MAX_WORKERS = 100

# === Init Elasticsearch ===
ES_HOST = "http://elasticsearch-master:9200"
es = Elasticsearch([ES_HOST])

# === Thread Pool ===
executor = ThreadPoolExecutor(max_workers=MAX_WORKERS)


def send_to_engine(powmin, powmax, engine, request_id):
    engine_port = SPK_ENGINE_PORT if engine == "spark" else MPI_ENGINE_PORT
    engine_host = SPK_ENGINE_HOST if engine == "spark" else MPI_ENGINE_HOST
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as engine_sock:
        # print(f"[Central] Conectando ao engine {engine} em {ENGINE_HOST}:{engine_port}")
        
        engine_sock.settimeout(600)
        engine_sock.connect((engine_host, engine_port))
        # Cria novo payload para enviar ao engine
        new_payload = {
            "powmin": powmin,
            "powmax": powmax
        }

        if engine == "mpi":
            # Envia apenas powmin,powmax como string
            engine_sock.sendall(json.dumps(new_payload).encode())
            response = engine_sock.recv(4096)
            print(f"[Central] Resposta do engine recebida: {request_id}")
            return {"result": response.decode(), "request_id": request_id}
        else:
            # Envia o novo payload como JSON
            engine_sock.sendall(json.dumps(new_payload).encode())
            response = engine_sock.recv(4096)
            return {"result": response.decode(), "request_id": request_id}


def log_to_elasticsearch(log_doc):
    try:
        es.index(index="observabilidade", document=log_doc)
    except Exception as e:
        print(f"[ES] Erro ao enviar log: {e}")


def handle_client(conn, addr):
    client_id = addr[0]
    host_node = socket.gethostname()

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            try:
                payload = json.loads(data.decode())
                engine = payload.get("engine")
                powmin = payload["powmin"]
                powmax = payload["powmax"]
                print(f"[Central] Recebido: POWMIN={powmin}, POWMAX={powmax}, ENGINE={engine}")
            except Exception:
                conn.sendall(b'{"status":"error","message":"invalid JSON"}')
                return

            request_id = str(uuid.uuid4())
            start_time = datetime.utcnow()

            engine_payload = {
                "request_id": request_id,
                "powmin": powmin,
                "powmax": powmax,
                "client_id": client_id
            }

            try:
                engine_response = send_to_engine(powmin, powmax, engine, request_id)
                if engine_response.get("request_id") != request_id:
                    raise ValueError("Mismatched request ID")

                status = "ok"
                error_message = None

            except Exception as e:
                status = "error"
                engine_response = None
                error_message = str(e)

            end_time = datetime.utcnow()
            duration_ms = (end_time - start_time).total_seconds() * 1000

            # === Elasticsearch logging ===
            log_doc = {
                "request_id": request_id,
                "client_id": client_id,
                "engine": "engine-service",
                "powmin": powmin,
                "powmax": powmax,
                "start_time": start_time.isoformat(),
                "end_time": end_time.isoformat(),
                "duration_ms": duration_ms,
                "status": status,
                "error_message": error_message,
                "host_node": host_node,
                "num_clients_active": threading.active_count() - 1,
                "timestamp": datetime.utcnow().isoformat()
            }

            # executor.submit(log_to_elasticsearch, log_doc)

            # === Send response back to client ===
            conn.sendall(json.dumps({
                "status": status,
                "data": engine_response.get("result") if status == "ok" else None,
                "error": error_message,
                "request_id": request_id
            }).encode())

    finally:
        conn.close()
        print(f"[TCP] Cliente {addr} desconectado.")


def start_tcp_server():
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.bind((HOST, TCP_PORT))
    server_sock.listen()
    print(f"[TCP] Servidor escutando em {HOST}:{TCP_PORT}...")

    while True:
        conn, addr = server_sock.accept()
        print(f"[TCP] Conexão recebida de {addr}")
        executor.submit(handle_client, conn, addr)


if __name__ == "__main__":
    start_tcp_server()
