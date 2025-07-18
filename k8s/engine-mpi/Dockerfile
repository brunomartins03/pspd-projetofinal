FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y python3 python3-pip

RUN apt-get update && \
    apt-get install -y build-essential mpich libomp-dev

WORKDIR /app

COPY main.c /app/
COPY server-mpi.py /app/

RUN mpicc -fopenmp -O3 main.c -o engine-mpi

CMD ["python3", "server-mpi.py"]
