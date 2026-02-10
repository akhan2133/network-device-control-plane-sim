# Quick Start Guide

## Build and Run Commands

### 1. Build the Project

```bash
cd /home/asfand-khan/Documents/network-device-control-plane-sim
make build
```

Or manually:
```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 2. Run Unit Tests

```bash
make test
```

Or manually:
```bash
cd build
ctest --output-on-failure --verbose
```

### 3. Run the Simulator

```bash
make run
```

Or manually:
```bash
./build/bin/control_plane_sim --config config/config.yaml
```

With custom options:
```bash
./build/bin/control_plane_sim \
    --ports 16 \
    --tick-ms 50 \
    --seed 12345 \
    --log-level debug \
    --http-port 8080
```

### 4. Test HTTP Endpoints

While simulator is running (in another terminal):

```bash
# Health check
curl http://localhost:8080/health

# Metrics (Prometheus format)
curl http://localhost:8080/metrics

# Status (JSON)
curl http://localhost:8080/status
```

### 5. Run Integration Tests

```bash
make integration-test
```

Or manually:
```bash
./scripts/integration_test.sh
```

### 6. Build Docker Image

```bash
make docker-build
```

Or manually:
```bash
docker build -t control-plane-sim:latest .
```

### 7. Run Docker Container

```bash
make docker-run
```

Or manually:
```bash
docker run -p 8080:8080 control-plane-sim:latest
```

With custom config:
```bash
docker run -p 8080:8080 \
    -v $(pwd)/config/config.yaml:/app/config/config.yaml \
    control-plane-sim:latest
```

### 8. Deploy to Kubernetes

```bash
# Create resources
kubectl apply -f k8s/configmap.yaml
kubectl apply -f k8s/deployment.yaml
kubectl apply -f k8s/service.yaml

# Verify deployment
kubectl get pods -l app=control-plane-sim
kubectl get svc control-plane-sim

# View logs
kubectl logs -l app=control-plane-sim --tail=50 -f

# Port forward to local machine
kubectl port-forward svc/control-plane-sim 8080:8080

# Test (in another terminal)
curl http://localhost:8080/health

# Cleanup
kubectl delete -f k8s/
```

### 9. Load Docker Image into Kubernetes

For local Kubernetes (minikube/kind):

```bash
# Build image
docker build -t control-plane-sim:latest .

# For minikube
minikube image load control-plane-sim:latest

# For kind
kind load docker-image control-plane-sim:latest

# Then deploy
kubectl apply -f k8s/
```

## Common Use Cases

### Deterministic Testing

Run with fixed seed for reproducible behavior:
```bash
./build/bin/control_plane_sim --seed 12345 --ports 4 --tick-ms 100
```

### Debug Mode

Run with verbose logging:
```bash
./build/bin/control_plane_sim --log-level debug | jq '.'
```

### High Flap Rate Testing

Edit `config/config.yaml`:
```yaml
flap_probability: 0.1  # 10% chance per tick
```

Then run:
```bash
./build/bin/control_plane_sim --config config/config.yaml
```

### Monitor Metrics in Real-Time

```bash
# In terminal 1
./build/bin/control_plane_sim

# In terminal 2
watch -n 1 'curl -s http://localhost:8080/metrics | grep control_plane'
```

## Troubleshooting

### Build fails

Make sure you have dependencies:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git curl
```

### Tests fail

Clean and rebuild:
```bash
make clean
make build
make test
```

### Binary crashes

Run with debug symbols:
```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
gdb ./bin/control_plane_sim
```

### Docker build fails

Ensure Docker daemon is running:
```bash
sudo systemctl status docker
```

## Next Steps

1. Read the full [README.md](README.md) for detailed architecture explanation
2. Explore the source code in `src/` and `include/`
3. Modify `config/config.yaml` to experiment with different parameters
4. Check out unit tests in `tests/` for usage examples
5. Set up Jenkins pipeline using `Jenkinsfile`
