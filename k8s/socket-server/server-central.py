import socket
import json
import uuid
import threading
import logging
import re
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
import requests

# === CONFIGS ===
# MPI_ENGINE_HOST = "0.0.0.0"
# SPK_ENGINE_HOST = "0.0.0.0"
MPI_ENGINE_HOST = "engine-mpi"
SPK_ENGINE_HOST = "engine-spark"
HOST = "0.0.0.0"
MPI_ENGINE_PORT = 5000
SPK_ENGINE_PORT = 5001
TCP_PORT = 3000
MAX_WORKERS = 100
logging.basicConfig(level=logging.INFO)

# === Init Elasticsearch ===
ES_HOST = "http://elasticsearch:9200"

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
            logging.info(f"[Central] Resposta do engine recebida: {request_id}")
            return {"result": response.decode(), "request_id": request_id}
        else:
            # Envia o novo payload como JSON
            engine_sock.sendall(json.dumps(new_payload).encode())
            response = engine_sock.recv(4096)
            return {"result": response.decode(), "request_id": request_id}


def log_to_elasticsearch(log_doc):
    url = f"{ES_HOST}/game-of-life-requests/_doc"
    headers = {"Content-Type": "application/json"}
    try:
        response = requests.post(url, headers=headers, data=json.dumps(log_doc), timeout=10)
        response.raise_for_status()
        logging.info("[Central-ES] Log enviado: %s", log_doc)
    except requests.RequestException as e:
        logging.error("[Central-ES] Erro ao enviar log: %s", e)

def extract_result(result):
    if not result:
        return {}

    output = {}
    total_gens = 0
    tam = None
    time_val = None
    for line in result.splitlines():
        matches = re.findall(r'(\w+)\s*=\s*([+-]?\d+(?:\.\d+)?)', line)
        for k, v in matches:
            value = float(v) if '.' in v else int(v)
            output[k] = value
            if k == "gens":
                total_gens += int(value)
            if k == "tam":
                tam = int(value)
            if k == "tempo":
                time_val = float(value)
    return (tam, total_gens, time_val)


def handle_client(conn, addr):
    client_id = f"{addr[0]}:{addr[1]}"
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
                logging.info(f"[Central] Recebido: POWMIN={powmin}, POWMAX={powmax}, ENGINE={engine}")
            except Exception:
                conn.sendall(b'{"status":"error","message":"invalid JSON"}')
                return

            request_id = str(uuid.uuid4())
            start_time = datetime.utcnow()


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

            result = engine_response.get("result") if engine_response else None

            extracted_values = extract_result(result) if result is not None else (None, None, None)

            # === Elasticsearch logging ===
            log_doc = {
                "request_id": request_id,
                "client_id": client_id,
                "engine": "Spark" if engine == "spark" else "MPI e OpenMP",
                "powmin": powmin,
                "powmax": powmax,
                "start_time": start_time.isoformat(),
                "end_time": end_time.isoformat(),
                "duration_ms": ((float(extracted_values[2]) * 1000) if extracted_values[2] is not None else 0),
                "status": status,
                "error_message": error_message,
                "num_generations": extracted_values[1] if extracted_values[1] is not None else -1,
                "board_size": extracted_values[0] if extracted_values[0] is not None else -1,
                "host_node": host_node,
                "num_clients_active": threading.active_count() - 1,
                "timestamp": datetime.utcnow().isoformat()
            }

            logging.info(f"[Central] Enviando log.")
            executor.submit(log_to_elasticsearch, log_doc)

            # === Send response back to client ===
            logging.info(f"[Central] Enviando resposta ao cliente ({client_id}).")
            conn.sendall(json.dumps({
                "status": status,
                "data": result if status == "ok" else None,
                "error": error_message,
                "request_id": request_id
            }).encode())

    finally:
        conn.close()
        logging.info(f"[Central] Cliente {addr} desconectado.")


def start_tcp_server():
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.bind((HOST, TCP_PORT))
    server_sock.listen()
    logging.info(f"[Central] Servidor escutando em {HOST}:{TCP_PORT}...")

    while True:
        conn, addr = server_sock.accept()
        logging.info(f"[Central] Conexão recebida de {addr}")
        executor.submit(handle_client, conn, addr)


if __name__ == "__main__":
    start_tcp_server()
