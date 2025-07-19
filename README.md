# Projeto Final - PSPD

## Descrição

Este repositório apresenta uma aplicação de larga escala baseada no clássico **Jogo da Vida de Conway**, implementada com **Apache Spark**, **MPI/OpenMP**, **Kubernetes** e monitoramento com **Elasticsearch/Kibana**. O projeto explora *paralelismo* e *elasticidade* de forma integrada, unificando conceitos de programação paralela e distribuída.


## 📌 Objetivo

Investigar e comparar **frameworks de computação paralela/distribuída** (Spark, MPI, OpenMP) e **ferramentas de elasticidade** (Kubernetes), em uma aplicação que:

- Processa múltiplas requisições de simulações do Jogo da Vida via rede (TCP).
- Executa o cálculo dos estágios em dois engines distintos: Spark e MPI/OpenMP.
- Coleta e exibe métricas de desempenho via Elasticsearch/Kibana.
- Escala horizontalmente via Kubernetes.


## 🧠 Sobre o Jogo da Vida

O Jogo da Vida é um autômato celular onde, em uma grade 2D, cada célula (viva ou morta) evolui de acordo com:

1. Célula viva com 2 ou 3 vizinhas vivas → continua viva  
2. Célula morta com exatamente 3 vizinhas vivas → nasce  
3. Caso contrário → morre ou permanece morta

## 🧩 Arquitetura do Projeto

```
.
├── cliente.py                             # Script do cliente para interagir com o servidor
├── k8s
│   ├── elk                                # Configurações do Elasticsearch/Kibana
│   │   ├── dashboard.ndjson
│   │   ├── deployment.yaml
│   │   ├── elasticsearch-service.yaml
│   │   ├── kibana-service.yaml
│   │   └── setup-elk.sh
│   ├── engine-mpi                         # Configurações do engine MPI
│   │   ├── deployment.yaml
│   │   ├── Dockerfile
│   │   ├── engine-mpi
│   │   ├── main.c
│   │   ├── server-mpi.py
│   │   └── service.yaml
│   ├── engine-spark                       # Configurações do engine Spark
│   │   ├── deployment.yaml
│   │   ├── Dockerfile
│   │   ├── jogo_da_vida_pyspark.py
│   │   ├── server-spark.py
│   │   └── service.yaml
│   ├── setup-cluster.sh                   # Script para configurar o cluster Kubernetes
│   └── socket-server                      # Configurações do servidor socket
│       ├── deployment.yaml
│       ├── Dockerfile
│       ├── server-central.py
│       └── service.yaml
└── README.md
```

## 🚀 Como Executar

1. **Configurar o Cluster Kubernetes**: Execute o script `setup-cluster.sh` para configurar o cluster do Kubernetes com os componentes necessários e subir todos os pods. Após a execução, o dashboard do Kubernetes será aberto automaticamente no seu navegador.
2. **Setar o Índice do ElasticSearch e o Dashboard do Kibana**: Execute o script `setup-elk.sh` passando os nomes dos pods do ElasticSearch e do Kibana como argumento para terminar de configurar os serviços (você pode obter essas informações no dashboard do Kubernetes).
3. **Execute o Código Cliente**: Execute o script `cliente.py` para iniciar as requisições de simulação do Jogo da Vida. O cliente irá se conectar ao servidor socket, que por sua vez irá distribuir as tarefas entre os engines MPI e Spark e depois coletar as métricas de desempenho, enviar para o Elasticsearch e exibi-las no Kibana.
4. **Visualizar Métricas**: Acesse o Kibana para visualizar no dashboard `Dashboard Jogo da Vida` as métricas de desempenho coletadas durante a execução das simulações. (Para descobrir o endereço do Kibana, você pode usar o comando `minikube -p pspd-cluster service kibana --url -n default`no terminal).


## 👥 Autores

<div align="center">
   <table style="margin-left: auto; margin-right: auto;">
        <tr>
            <td align="center">
                <a href="https://github.com/arthurgrandao">
                    <img style="border-radius: 50%;" src="https://avatars.githubusercontent.com/u/85596312?v=4" width="150px;"/>
                    <h5 class="text-center">Arthur Grandão <br>211039250</h5>
                </a>
            </td>
            <td align="center">
                <a href="https://github.com/gitbmvb">
                    <img style="border-radius: 50%;" src="https://avatars.githubusercontent.com/u/30751876?v=4" width="150px;"/>
                    <h5 class="text-center">Bruno Martins <br>211039297</h5>
                </a>
            </td>
            <td align="center">
                <a href="https://github.com/dougAlvs">
                    <img style="border-radius: 50%;" src="https://avatars.githubusercontent.com/u/98109429?v=4" width="150px;"/>
                    <h5 class="text-center">Douglas Alves <br>211029620</h5>
                </a>
            </td>
            <td align="center">
                <a href="https://github.com/g16c">
                    <img style="border-radius: 50%;" src="https://avatars.githubusercontent.com/u/90865675?v=4" width="150px;"/>
                    <h5 class="text-center">Gabriel Campello <br>211039439</h5>
                </a>
            </td>
            <td align="center">
                <a href="https://github.com/manuziny">
                    <img style="border-radius: 50%;" src="https://avatars.githubusercontent.com/u/88348637?v=4" width="150px;"/>
                    <h5 class="text-center">Geovanna Avelino <br>202016328</h5>
                </a>
            </td>
    </table>
</div>


## 📚 Referências

CONWAY, John. *Conway's Game of Life*. Wikipedia, 2024. Disponível em: [https://en.wikipedia.org/wiki/Conway%27s\_Game\_of\_Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life). Acesso em: 18 Jul. 2025.

OPEN MPI. *Official Open MPI website*. \[S.l.]: Open MPI, \[2025?]. Disponível em: [https://www.open-mpi.org/](https://www.open-mpi.org/). Acesso em: 18 Jul. 2025.

OPENMP ARCHITECTURE REVIEW BOARD. *The OpenMP API specification for parallel programming*. \[S.l.]: OpenMP, \[2025?]. Disponível em: [https://www.openmp.org/](https://www.openmp.org/). Acesso em: 18 Jul. 2025.

KUBERNETES. *Production-Grade Container Orchestration*. [S.l.]: Cloud Native Computing Foundation, [2025?]. Disponível em: [https://kubernetes.io](https://kubernetes.io). Acesso em: 18 Jul. 2025.

ELASTICSEARCH. *Elasticsearch: Distributed, RESTful Search and Analytics Engine*. [S.l.]: Elastic, [2025?]. Disponível em: [https://www.elastic.co/elasticsearch](https://www.elastic.co/elasticsearch). Acesso em: 18 Jul. 2025.

KIBANA. *Kibana: Explore, Visualize, Discover Data*. [S.l.]: Elastic, [2025?]. Disponível em: [https://www.elastic.co/kibana](https://www.elastic.co/kibana). Acesso em: 18 Jul. 2025.

APACHE SPARK. *Apache Spark: Unified Analytics Engine for Big Data*. [S.l.]: Apache Software Foundation, [2025?]. Disponível em: [https://spark.apache.org](https://spark.apache.org). Acesso em: 18 Jul. 2025.
