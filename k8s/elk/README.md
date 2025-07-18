# ELK

## Como Acessar o Kibana

Após executar o script `setup-cluster.sh`, entre na interface web do Kibana, você pode encontrar o url e a porta com o comando:

```bash
minikube -p pspd-cluster service kibana --url -n default
```

Acessando a interface, você vai precisar gerar um token de autenticação para o Kibana. Você pode fazer isso com o seguinte comando substituindo `nome_pod_elastic_search` pelo nome do pod do Elasticsearch:

```bash
kubectl exec -it nome_pod_elastic_search -- bin/elasticsearch-create-enrollment-token --scope kibana
```

Depois, você pode usar o comando abaixo para obter o código de verificação do Kibana. Substitua `nome_pod_kibana` pelo nome do pod do Kibana:

```bash
kubectl exec -it nome_pod_kibana -- bin/kibana-verification-code
```

Você pode usar o usuário `elastic` e a senha gerada pelo comando abaixo. Substitua `nome_pod_elastic_search` pelo nome do pod do Elasticsearch:

```bash
kubectl exec -it nome_pod_elastic_search -- bin/elasticsearch-reset-password -u elastic
```

# Criando o Índice no Elasticsearch

Para criar o índice no Elasticsearch, você pode usar o seguinte comando:

```bash
curl -X PUT "http://localhost:9200/game-of-life-requests" \
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
```

# Importando o Dashboard no Kibana

Para importar o dashboard no Kibana, vá na seção "Stack Management" e clique em "Saved Objects". Em seguida, clique em "Import" e selecione o arquivo `dashboard.ndjson` que está localizado na pasta `elk/`. 
Após isso, o dashboard será importado e você poderá visualizá-lo na seção "Dashboard". Conforme os dados forem sendo enviados para o Elasticsearch, eles serão automaticamente atualizados no dashboard.