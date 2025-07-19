# Imagem base com Spark e PySpark já instalados
FROM bitnami/spark:3.4.0

USER root

RUN apt-get update && \
    apt-get install -y python3 python3-pip

RUN pip3 install --upgrade pip
RUN pip3 install pyspark==3.4.0

# Cria diretório da aplicação
WORKDIR /app

# Copia o script e o requirements (caso use bibliotecas extras)
COPY jogo_da_vida_pyspark.py /app/
COPY server-spark.py /app/

# Executa o script com spark-submit
CMD ["python3", "server-spark.py"]

