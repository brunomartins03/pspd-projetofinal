#!/bin/bash

CLUSTER_NAME="pspd-cluster"

minikube start -p $CLUSTER_NAME --nodes=3 --cpus=2 --memory=2200mb

sleep 10

nodes=($(minikube -p $CLUSTER_NAME kubectl -- get nodes -o custom-columns=NAME:.metadata.name --no-headers))

minikube -p $CLUSTER_NAME kubectl -- label node ${nodes[0]} role=master --overwrite
minikube -p $CLUSTER_NAME kubectl -- label node ${nodes[1]} role=worker1 --overwrite
minikube -p $CLUSTER_NAME kubectl -- label node ${nodes[2]} role=worker2 --overwrite

minikube -p $CLUSTER_NAME kubectl -- apply -f socket-server/
minikube -p $CLUSTER_NAME kubectl -- apply -f engine-mpi/
minikube -p $CLUSTER_NAME kubectl -- apply -f engine-spark/
minikube -p $CLUSTER_NAME kubectl -- create configmap kibana-dashboard --from-file=dashboard.ndjson=elk/dashboard.ndjson --dry-run=client -o yaml | minikube -p $CLUSTER_NAME kubectl apply -f -
minikube -p $CLUSTER_NAME kubectl -- apply -f elk/
minikube -p $CLUSTER_NAME kubectl -- apply -f elk/jobs/

minikube -p $CLUSTER_NAME kubectl -- get pods -o wide

minikube -p $CLUSTER_NAME dashboard

