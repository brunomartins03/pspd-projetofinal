FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y build-essential mpich libomp-dev
WORKDIR /app
COPY main.c .
RUN mpicc -fopenmp -O3 main.c -o engine-mpi
CMD ["mpirun", "-np", "4", "./engine-mpi", "3", "10"]
