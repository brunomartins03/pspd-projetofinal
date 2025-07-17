# Imagem base com Spark e PySpark já instalados
FROM bitnami/spark:3.4.0

USER root

# Cria diretório da aplicação
WORKDIR /app

# Copia o script e o requirements (caso use bibliotecas extras)
COPY jogo_da_vida_pyspark.py /app/

# Executa o script com spark-submit
CMD ["spark-submit", "/app/jogo_da_vida_pyspark.py", "3", "10"]

