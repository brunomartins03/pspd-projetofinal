#!/bin/bash
set -e

if [ "$#" -ne 2 ]; then
  echo "Uso: $0 <pod_elasticsearch> <pod_kibana>"
  exit 1
fi

POD_ES=$1
POD_KB=$2

kubectl exec -it "$POD_ES" -- curl -s -X PUT "http://localhost:9200/game-of-life-requests" \
  -H 'Content-Type: application/json' \
  -d '{
        "mappings": {
          "properties": {
            "request_id": {"type": "keyword"},
            "client_id": {"type": "keyword"},
            "engine": {"type": "keyword"},
            "powmin": {"type": "integer"},
            "powmax": {"type": "integer"},
            "start_time": {"type": "date"},
            "end_time": {"type": "date"},
            "duration_ms": {"type": "float"},
            "status": {"type": "keyword"},
            "error_message": {"type": "text"},
            "num_generations": {"type": "integer"},
            "board_size": {"type": "integer"},
            "num_clients_active": {"type": "integer"},
            "host_node": {"type": "keyword"},
            "timestamp": {"type": "date"}
          }
        }
      }'

kubectl cp "dashboard.ndjson" "$POD_KB:/tmp/dashboard.ndjson"

kubectl exec -it "$POD_KB" -- curl -s -X POST "http://localhost:5601/api/saved_objects/_import?overwrite=true" \
  -H "kbn-xsrf: true" --form file=@/tmp/dashboard.ndjson

kubectl exec -it "$POD_KB" -- rm /tmp/dashboard.ndjson

echo "Índice criado e dashboard importado com sucesso."
