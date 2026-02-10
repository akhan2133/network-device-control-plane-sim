# Complete Command Reference

This document provides all the exact commands needed to build, test, run, and deploy the Control Plane Simulator.

## Prerequisites Installation (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git curl docker.io
```

## Project Location

```bash
cd /home/asfand-khan/Documents/network-device-control-plane-sim
```

---

## A) Build and Run Locally

### Build the Project

```bash
# Using Makefile (recommended)
make build

# Or manually with CMake
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

**Output**: `build/bin/control_plane_sim`

### Run the Simulator

```bash
# Using default configuration
./build/bin/control_plane_sim --config config/config.yaml

# Or with CLI overrides
./build/bin/control_plane_sim \
    --ports 8 \
    --tick-ms 100 \
    --seed 12345 \
    --log-level info \
    --http-port 8080
```

**Expected Output**: JSON-formatted logs to stdout showing port state transitions

### Test HTTP Endpoints

Open a new terminal while simulator is running:

```bash
# Health check
curl http://localhost:8080/health
# Expected: {"status":"ok"}

# Metrics (Prometheus format)
curl http://localhost:8080/metrics
# Expected: Text format metrics with counters and gauges

# Status (JSON summary)
curl http://localhost:8080/status
# Expected: JSON with port counts and statistics
```

### Stop the Simulator

Press `Ctrl+C` in the terminal running the simulator. It will shut down gracefully and print final statistics.

---

## B) Run Unit Tests

```bash
# Using Makefile
make test

# Or manually
cd build
ctest --output-on-failure --verbose

# Or run test binary directly
./build/bin/unit_tests
```

**Expected Output**: All 16 tests pass (state machine, determinism, thread safety)

---

## C) Run Integration Test

```bash
# Ensure you've built first
make build

# Run integration test
./scripts/integration_test.sh
```

**What it does**:
1. Starts simulator in background
2. Waits for startup
3. Tests /health endpoint
4. Tests /metrics endpoint
5. Verifies events are being processed
6. Tests /status endpoint
7. Stops simulator
8. Reports success/failure

**Expected Output**: "All integration tests passed!"

---

## D) Build and Run Docker

### Build Docker Image

```bash
# Using Makefile
make docker-build

# Or manually
docker build -t control-plane-sim:latest .
```

**Output**: Docker image `control-plane-sim:latest` (~100MB)

### Run Docker Container

```bash
# Basic run
docker run -p 8080:8080 control-plane-sim:latest

# Run with custom arguments
docker run -p 8080:8080 control-plane-sim:latest \
    --ports 16 \
    --tick-ms 50 \
    --log-level debug

# Run with custom config file (volume mount)
docker run -p 8080:8080 \
    -v $(pwd)/my-config.yaml:/app/config/config.yaml:ro \
    control-plane-sim:latest

# Run in detached mode
docker run -d -p 8080:8080 --name sim control-plane-sim:latest

# View logs
docker logs -f sim

# Stop container
docker stop sim
docker rm sim
```

### Test Docker Container

```bash
curl http://localhost:8080/health
curl http://localhost:8080/metrics
curl http://localhost:8080/status
```

---

## E) Deploy to Kubernetes

### Prerequisites

Ensure you have a Kubernetes cluster running and `kubectl` configured:

```bash
# For minikube
minikube start

# For kind
kind create cluster

# Verify cluster
kubectl cluster-info
kubectl get nodes
```

### Load Image into Local Kubernetes

```bash
# Build image first
docker build -t control-plane-sim:latest .

# For minikube
minikube image load control-plane-sim:latest

# For kind
kind load docker-image control-plane-sim:latest
```

### Deploy to Kubernetes

```bash
# Apply all manifests
kubectl apply -f k8s/configmap.yaml
kubectl apply -f k8s/deployment.yaml
kubectl apply -f k8s/service.yaml

# Or apply all at once
kubectl apply -f k8s/
```

**Expected Output**:
```
configmap/control-plane-sim-config created
deployment.apps/control-plane-sim created
service/control-plane-sim created
```

### Verify Deployment

```bash
# Check deployment
kubectl get deployments
kubectl get pods -l app=control-plane-sim

# Wait for pods to be ready
kubectl wait --for=condition=ready pod -l app=control-plane-sim --timeout=60s

# Check service
kubectl get svc control-plane-sim

# View logs from all pods
kubectl logs -l app=control-plane-sim --tail=50

# Follow logs from first pod
kubectl logs -l app=control-plane-sim --tail=50 -f

# Describe deployment
kubectl describe deployment control-plane-sim

# Check pod health
kubectl get pods -l app=control-plane-sim -o wide
```

### Access Service

```bash
# Port forward to local machine
kubectl port-forward svc/control-plane-sim 8080:8080

# In another terminal, test endpoints
curl http://localhost:8080/health
curl http://localhost:8080/metrics
curl http://localhost:8080/status
```

### Scale Deployment

```bash
# Scale to 4 replicas
kubectl scale deployment/control-plane-sim --replicas=4

# Verify scaling
kubectl get pods -l app=control-plane-sim

# Watch rollout status
kubectl rollout status deployment/control-plane-sim
```

### Update Configuration

```bash
# Edit ConfigMap
kubectl edit configmap control-plane-sim-config

# Restart pods to pick up new config
kubectl rollout restart deployment/control-plane-sim

# Watch rollout
kubectl rollout status deployment/control-plane-sim
```

### View Pod Details

```bash
# Get pod names
kubectl get pods -l app=control-plane-sim

# Exec into a pod
kubectl exec -it <pod-name> -- /bin/bash

# Check resource usage
kubectl top pods -l app=control-plane-sim
```

### Cleanup Kubernetes Resources

```bash
# Delete all resources
kubectl delete -f k8s/

# Or delete individually
kubectl delete deployment control-plane-sim
kubectl delete service control-plane-sim
kubectl delete configmap control-plane-sim-config
```

---

## Common Workflows

### Development Workflow

```bash
# 1. Make code changes in src/ or include/

# 2. Rebuild
make clean
make build

# 3. Run tests
make test

# 4. Test locally
./build/bin/control_plane_sim --ports 2 --log-level debug

# 5. Run integration test
./scripts/integration_test.sh

# 6. If all good, rebuild Docker
make docker-build
```

### Testing Different Scenarios

**High Availability Test** (many ports):
```bash
./build/bin/control_plane_sim --ports 100 --tick-ms 50
```

**Deterministic Replay** (fixed seed):
```bash
./build/bin/control_plane_sim --seed 12345 --ports 4
```

**Fault Injection Testing** (modify config):
```yaml
# In config/config.yaml
flap_probability: 0.1  # 10% flap rate
```

**Performance Testing** (fast ticks):
```bash
./build/bin/control_plane_sim --tick-ms 10 --ports 50
```

### Monitoring in Production

**Prometheus Scraping**:
```yaml
# Add to prometheus.yml
scrape_configs:
  - job_name: 'control-plane-sim'
    kubernetes_sd_configs:
      - role: pod
    relabel_configs:
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
        action: keep
        regex: true
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_port]
        action: replace
        target_label: __address__
        regex: ([^:]+)(?::\d+)?;(\d+)
        replacement: $1:$2
```

**Log Aggregation** (ELK/Splunk):
```bash
# Simulator outputs JSON logs - pipe to log collector
./build/bin/control_plane_sim | fluent-bit ...
```

**Grafana Dashboard**:
- Add Prometheus data source
- Query: `control_plane_ports_up`
- Query: `rate(control_plane_events_processed_total[5m])`
- Query: `control_plane_link_flaps_injected_total`

---

## Jenkins CI/CD

The `Jenkinsfile` is already configured. To use it:

1. Create a new Pipeline job in Jenkins
2. Point to your Git repository
3. Set it to use `Jenkinsfile` from SCM
4. Trigger build

**Pipeline Stages**:
1. Checkout - Clone repository
2. Build - Compile with CMake
3. Unit Tests - Run test suite
4. Integration Test - Execute integration script
5. Docker Build - Create container image
6. Archive Artifacts - Store binaries

---

## Advanced Usage

### Debug Build

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run with gdb
gdb ./bin/control_plane_sim
(gdb) run --ports 2
```

### Memory Leak Detection

```bash
# Build with sanitizers
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
make -j$(nproc)

# Run
./bin/control_plane_sim --ports 2
```

### Performance Profiling

```bash
# Install perf
sudo apt-get install linux-tools-generic

# Run with perf
perf record -g ./build/bin/control_plane_sim --ports 100
# Let it run for 10 seconds, then Ctrl+C

# Analyze
perf report
```

### Custom Build Flags

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-march=native -O3" \
      ..
make -j$(nproc)
```

---

## Summary of All Commands

```bash
# Build
make build

# Test
make test
./scripts/integration_test.sh

# Run locally
./build/bin/control_plane_sim --config config/config.yaml

# Docker
make docker-build
make docker-run

# Kubernetes
kubectl apply -f k8s/
kubectl get pods -l app=control-plane-sim
kubectl port-forward svc/control-plane-sim 8080:8080
curl http://localhost:8080/health
kubectl delete -f k8s/
```

---

## Troubleshooting Commands

```bash
# Check if simulator is running
ps aux | grep control_plane_sim

# Check port 8080 usage
sudo netstat -tlnp | grep 8080
# Or
sudo lsof -i :8080

# Check Docker containers
docker ps -a

# Check Kubernetes pod logs
kubectl logs -l app=control-plane-sim --tail=100

# Check Kubernetes events
kubectl get events --sort-by='.lastTimestamp'

# Clean everything
make clean
docker system prune -a
kubectl delete all --all
```

---

**Project Complete and Ready for Production Deployment!**
