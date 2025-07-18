FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y python3 python3-pip

RUN pip3 install --upgrade pip
RUN pip3 install elasticsearch==9.0.2

WORKDIR /app

COPY server-central.py /app/

CMD ["python3", "server-central.py"]
