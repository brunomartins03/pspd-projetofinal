import os

from elasticsearch import Elasticsearch
from dotenv import load_dotenv


env = load_dotenv()

secret_key_id = os.getenv("SECRET_KEY_ID")
secret_key_api = os.getenv("SECRET_KEY_API")

es = Elasticsearch(
    "https://localhost:9200/",
    api_key=(secret_key_id, secret_key_api),
    verify_certs=False,
    ssl_show_warn=False
)


INDEX_NAME = "game-of-life-requests"

mappings = {
    "mappings": {
        "properties": {
            "request_id":        {"type": "keyword"},
            "client_id":         {"type": "keyword"},
            "engine":            {"type": "keyword"},
            "powmin":            {"type": "integer"},
            "powmax":            {"type": "integer"},
            "start_time":        {"type": "date"},
            "end_time":          {"type": "date"},
            "duration_ms":       {"type": "float"},
            "status":            {"type": "keyword"},
            "error_message":     {"type": "text"},
            "num_generations":   {"type": "integer"},
            "board_size":        {"type": "integer"},
            "num_clients_active": {"type": "integer"},
            "host_node":         {"type": "keyword"},
            "timestamp":         {"type": "date"}
        }
    }
}

if es.indices.exists(index=INDEX_NAME):
    es.indices.delete(index=INDEX_NAME)

es.indices.create(index=INDEX_NAME, body=mappings)

print(f"Índice '{INDEX_NAME}' criado com sucesso.")
