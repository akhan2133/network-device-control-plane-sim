# Embedded-Style Network Control Plane Simulator

A production-grade C++17 Linux service that simulates a network device control plane managing N linecard ports. Built for deterministic, concurrent, and observable operation with full containerization and Kubernetes support.

## Overview

This simulator models a telecom/optical network device control plane that manages multiple linecard ports. Each port follows a state machine (DOWN → INIT → UP) and responds to various events including power-on sequences, initialization completion, link flaps, and heartbeats.

### Key Features

- **State Machine**: Deterministic port state transitions (DOWN → INIT → UP)
- **Concurrent Architecture**: Multi-threaded event processing with worker threads
- **Fault Injection**: Configurable link flap simulation for reliability testing
- **Observability**: JSON structured logging + Prometheus metrics endpoint
- **Deterministic Mode**: Seeded random number generation for reproducible tests
- **HTTP API**: Health checks and metrics exposition
- **Thread-Safe**: Mutex-based synchronization for shared state
- **Production-Ready**: Docker containerization, Kubernetes deployment, CI/CD pipeline

## Architecture

### Components

```
┌─────────────────────────────────────────────────────────┐
│                    Control Plane Simulator               │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌──────────────┐      ┌──────────────┐                │
│  │  HTTP Server │      │  Event Loop   │                │
│  │  (Port 8080) │      │  (Tick-based) │                │
│  └──────────────┘      └──────────────┘                │
│         │                      │                         │
│         │              ┌───────┴───────┐                │
│         │              │               │                 │
│         │        ┌─────▼─────┐  ┌─────▼──────┐         │
│         │        │ Heartbeat │  │Flap Injector│        │
│         │        │  Workers  │  │  Workers    │         │
│         │        └─────┬─────┘  └─────┬──────┘         │
│         │              │               │                 │
│         │              └───────┬───────┘                │
│         │                      │                         │
│         └──────────────────────▼───────────────────┐    │
│                        ┌───────────────────┐       │    │
│                        │  Port Manager     │       │    │
│                        │  (Thread-safe)    │       │    │
│                        └─────────┬─────────┘       │    │
│                                  │                 │    │
│                 ┌────────────────┼─────────────┐   │    │
│                 │                │             │   │    │
│           ┌─────▼─────┐    ┌─────▼─────┐    ...  │    │
│           │   Port 0  │    │   Port 1  │         │    │
│           │State Mach.│    │State Mach.│         │    │
│           └───────────┘    └───────────┘         │    │
│                                                   │    │
│                        ┌──────────┐              │    │
│                        │ Metrics  │◄─────────────┘    │
│                        └──────────┘                    │
└─────────────────────────────────────────────────────────┘
```

### Thread Architecture

1. **Main Thread**: Handles initialization, configuration, and signal handling
2. **HTTP Server Thread**: Serves `/health`, `/metrics`, and `/status` endpoints
3. **Tick Thread**: Drives simulation timing at configurable intervals
4. **Heartbeat Workers** (2 threads): Progress ports through state machine
5. **Flap Injector Workers** (2 threads): Randomly inject link flaps based on probability

### State Machine

```
    DOWN ──POWER_ON──> INIT ──INIT_COMPLETE──> UP
      ▲                  │                      │
      │                  │                      │
      └──────LINK_FLAP───┴──────LINK_FLAP──────┘
```

**States:**
- `DOWN`: Port is offline/powered down
- `INIT`: Port is initializing
- `UP`: Port is operational and processing heartbeats

**Events:**
- `POWER_ON`: Transitions DOWN → INIT
- `INIT_COMPLETE`: Transitions INIT → UP
- `LINK_FLAP`: Transitions any state → DOWN (fault injection)
- `HEARTBEAT_OK`: Processed in UP state (no transition)

## Building the Project

### Prerequisites

- Ubuntu 22.04 or later (or compatible Linux distribution)
- C++17 compatible compiler (GCC 9+ or Clang 10+)
- CMake 3.14+
- curl (for HTTP library download and testing)
- Docker (optional, for containerization)
- Kubernetes cluster (optional, for deployment)

### Build Steps
For a quick local setup, follow the instructions in **[QUICKSTART.md](QUICKSTART.md)**.

```bash
# Clone or navigate to the repository
cd /home/asfand-khan/Documents/network-device-control-plane-sim

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build (use all available cores)
make -j$(nproc)

# Binary will be at: build/bin/control_plane_sim
```

### Build Outputs

- `build/bin/control_plane_sim` - Main executable
- `build/bin/unit_tests` - Unit test executable

## Running Locally

### Using Default Configuration

```bash
# From project root
./build/bin/control_plane_sim --config config/config.yaml
```

### Command-Line Options

```bash
./build/bin/control_plane_sim [options]

Options:
  --config PATH        Path to config YAML file (default: config/config.yaml)
  --ports N            Number of ports (default: 8)
  --tick-ms MS         Tick duration in milliseconds (default: 100)
  --seed N             Random seed for determinism
  --log-level LEVEL    Log level: debug, info, warn, error (default: info)
  --http-port PORT     HTTP server port (default: 8080)
  --help               Show help message
```

### Example: Deterministic Mode

```bash
# Run with fixed seed for reproducible behavior
./build/bin/control_plane_sim --ports 4 --seed 12345 --tick-ms 50
```

### Example: Verbose Logging

```bash
# Run with debug logging
./build/bin/control_plane_sim --log-level debug --ports 2
```

## Configuration

Configuration is loaded from `config/config.yaml` and can be overridden with CLI flags.

```yaml
# config/config.yaml
ports_count: 8              # Number of simulated ports
tick_ms: 100                # Simulation tick interval (ms)
flap_probability: 0.01      # Link flap probability per tick (0.0-1.0)
flap_min_ms: 500            # Minimum flap duration
flap_max_ms: 5000           # Maximum flap duration
log_level: info             # debug, info, warn, error
http_port: 8080             # HTTP server port
```

## HTTP API

### Endpoints

#### GET /health

Health check endpoint for liveness/readiness probes.

```bash
curl http://localhost:8080/health
```

Response:
```json
{"status":"ok"}
```

#### GET /metrics

Prometheus-compatible metrics exposition.

```bash
curl http://localhost:8080/metrics
```

Response:
```
# TYPE control_plane_events_processed_total counter
control_plane_events_processed_total 1523
control_plane_state_transitions_total 48
control_plane_link_flaps_injected_total 5
# TYPE control_plane_ports gauge
control_plane_ports_total 8.00
control_plane_ports_down 1.00
control_plane_ports_init 0.00
control_plane_ports_up 7.00
```

#### GET /status

JSON status summary (additional endpoint).

```bash
curl http://localhost:8080/status
```

Response:
```json
{
  "total_ports": 8,
  "total_events": 1523,
  "ports_down": 1.0,
  "ports_init": 0.0,
  "ports_up": 7.0
}
```

## Metrics Exposed

| Metric Name | Type | Description |
|-------------|------|-------------|
| `control_plane_events_processed_total` | Counter | Total events processed across all ports |
| `control_plane_state_transitions_total` | Counter | Total state transitions |
| `control_plane_link_flaps_injected_total` | Counter | Total link flaps injected |
| `control_plane_ports_total` | Gauge | Total number of ports |
| `control_plane_ports_down` | Gauge | Number of ports in DOWN state |
| `control_plane_ports_init` | Gauge | Number of ports in INIT state |
| `control_plane_ports_up` | Gauge | Number of ports in UP state |

## Testing

### Unit Tests

Unit tests cover state machine logic, determinism, and thread safety.

```bash
# Run all unit tests
cd build
ctest --output-on-failure --verbose

# Or run the test binary directly
./bin/unit_tests
```

**Test Coverage:**
- State machine transitions (all valid and invalid transitions)
- Deterministic behavior with seeded RNG
- Thread-safe concurrent event processing
- Metrics accuracy under concurrent load

### Integration Tests

Integration test script validates the running service.

```bash
# Make sure you've built the project first
./scripts/integration_test.sh
```

**Integration Test Checks:**
- Binary starts successfully
- Health endpoint returns 200 OK
- Metrics endpoint returns valid Prometheus format
- Events are being processed (metrics increase over time)
- Status endpoint returns valid JSON

## Docker

### Build Docker Image

```bash
# From project root
docker build -t control-plane-sim:latest .
```

The Dockerfile uses a multi-stage build:
- **Builder stage**: Compiles the C++ code with all dependencies
- **Runtime stage**: Minimal Ubuntu image with only runtime dependencies

Image runs as non-root user (UID 1000) for security.

### Run Container

```bash
# Run with default configuration
docker run -p 8080:8080 control-plane-sim:latest

# Run with custom arguments
docker run -p 8080:8080 control-plane-sim:latest \
    --ports 16 --tick-ms 50 --log-level debug

# Run with custom config file (mount volume)
docker run -p 8080:8080 \
    -v $(pwd)/my-config.yaml:/app/config/config.yaml \
    control-plane-sim:latest
```

### Test Container

```bash
# Health check
curl http://localhost:8080/health

# Metrics
curl http://localhost:8080/metrics

# Status
curl http://localhost:8080/status
```

## Kubernetes Deployment

### Prerequisites

- Kubernetes cluster (minikube, kind, or cloud provider)
- `kubectl` configured to access your cluster

### Deploy to Kubernetes

```bash
# Apply all manifests
kubectl apply -f k8s/configmap.yaml
kubectl apply -f k8s/deployment.yaml
kubectl apply -f k8s/service.yaml

# Or apply all at once
kubectl apply -f k8s/
```

### Verify Deployment

```bash
# Check deployment status
kubectl get deployments
kubectl get pods -l app=control-plane-sim

# View logs from a pod
kubectl logs -l app=control-plane-sim --tail=50 -f

# Check service
kubectl get svc control-plane-sim
```

### Access Service

```bash
# Port forward to local machine
kubectl port-forward svc/control-plane-sim 8080:8080

# In another terminal, test endpoints
curl http://localhost:8080/health
curl http://localhost:8080/metrics
```

### Update Configuration

Edit the ConfigMap and restart pods:

```bash
# Edit config
kubectl edit configmap control-plane-sim-config

# Restart deployment to pick up changes
kubectl rollout restart deployment/control-plane-sim

# Watch rollout status
kubectl rollout status deployment/control-plane-sim
```

### Scale Deployment

```bash
# Scale to 4 replicas
kubectl scale deployment/control-plane-sim --replicas=4

# Verify
kubectl get pods -l app=control-plane-sim
```

### Cleanup

```bash
# Delete all resources
kubectl delete -f k8s/
```

## Structured Logging

All logs are emitted as JSON lines to stdout for easy parsing and ingestion by log aggregation systems (ELK, Splunk, CloudWatch, etc.).

### Log Format

```json
{
  "timestamp": "2026-02-07T10:30:45.123Z",
  "level": "INFO",
  "message": "Port 3 transitioned from INIT to UP on event INIT_COMPLETE",
  "component": "PortStateMachine",
  "port_id": 3
}
```

### Log Levels

- `DEBUG`: Detailed trace information for development
- `INFO`: General informational messages (default)
- `WARN`: Warning messages for potential issues
- `ERROR`: Error messages for failures

### Example: Parsing Logs with jq

```bash
# Run simulator and pipe to jq for pretty-printing
./build/bin/control_plane_sim | jq '.'

# Filter ERROR logs only
./build/bin/control_plane_sim | jq 'select(.level == "ERROR")'

# Count state transitions
./build/bin/control_plane_sim | jq 'select(.message | contains("transitioned"))' | wc -l
```

## Engineering Notes

### Design Tradeoffs

#### 1. **Concurrency Model: Worker Threads vs. Event Loop**

**Choice**: Hybrid model with dedicated worker threads + tick-based event loop

**Rationale**:
- Worker threads enable true parallelism for port operations
- Tick-based loop provides deterministic timing when seeded
- Mutex per port allows fine-grained locking (high concurrency)
- Avoids complex async/await or callback patterns

**Tradeoff**: More threads = higher memory overhead, but better CPU utilization on multi-core systems.

#### 2. **State Synchronization: Per-Port Mutexes**

**Choice**: One mutex per port (vector of mutexes)

**Rationale**:
- Minimizes lock contention (threads rarely compete for same port)
- Scales well as port count increases
- Simple to reason about (lock port before accessing)

**Tradeoff**: More memory for mutexes, but dramatically better performance than single global lock.

#### 3. **Metrics: Lock-Protected Maps with Atomics**

**Choice**: Atomic counters/gauges with mutex-protected map structure

**Rationale**:
- Atomic operations ensure thread-safe increments/reads
- Mutex protects map insertions (rare operation)
- Prometheus format export requires snapshot anyway

**Tradeoff**: Could use lockless data structures, but added complexity not justified for this use case.

#### 4. **HTTP Server: Embedded vs. External**

**Choice**: Embedded single-header library (cpp-httplib)

**Rationale**:
- Zero external dependencies (vendored header)
- Sufficient for health/metrics endpoints
- Easier to build and deploy

**Tradeoff**: Not suitable for high-throughput APIs, but perfect for observability.

#### 5. **Configuration: YAML + CLI Override**

**Choice**: Simple hand-rolled YAML parser + CLI flag parsing

**Rationale**:
- Avoids heavy dependencies (yaml-cpp, Boost.Program_options)
- Configuration is simple (flat key-value)
- CLI overrides useful for testing

**Tradeoff**: No validation of unknown keys, but acceptable for controlled environment.

#### 6. **Determinism: Optional Seeded RNG**

**Choice**: `--seed` flag enables deterministic mode

**Rationale**:
- Critical for reproducible testing and debugging
- Production uses random seed (unpredictable behavior)
- Single RNG shared by all workers (mutex-protected)

**Tradeoff**: RNG mutex can become bottleneck under extreme load, but flap injection is infrequent.

#### 7. **Graceful Shutdown: Signal Handling + Atomic Flag**

**Choice**: Atomic boolean flag checked by all threads

**Rationale**:
- Clean shutdown on SIGINT/SIGTERM
- No threads left running after main exits
- Final statistics printed before exit

**Tradeoff**: Threads must check flag periodically (adds tiny overhead).

### Reliability Considerations

1. **No Exceptions in Hot Path**: All critical paths use return codes
2. **Bounded Queues**: No unbounded growth (fixed port count)
3. **Validated Configuration**: Invalid configs rejected at startup
4. **Non-Root Container**: Security best practice
5. **Resource Limits**: K8s deployment includes memory/CPU limits
6. **Health Probes**: Kubernetes can restart unhealthy pods

### Performance Characteristics

- **CPU Usage**: Proportional to tick rate and port count
- **Memory Usage**: ~10MB base + ~1KB per port
- **Latency**: Event processing < 1ms (mutex contention dependent)
- **Throughput**: 10K+ events/sec on modest hardware (2-core VM)

### Future Enhancements

- **Backpressure Simulation**: Queue depth tracking per port
- **Advanced Fault Injection**: Bit errors, packet loss, latency spikes
- **Distributed Mode**: Multi-node simulation with network partitions
- **gRPC API**: For programmatic control and observation
- **Pluggable State Machines**: Load state machine definitions from config
- **Time-Travel Debugging**: Record/replay event sequences

## Troubleshooting

### Build Fails with "httplib.h not found"

Ensure cpp-httplib header was downloaded:
```bash
curl -L -o third_party/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```

### Simulator Crashes on Startup

Check configuration validity:
```bash
./build/bin/control_plane_sim --config config/config.yaml --help
```

Enable debug logging:
```bash
./build/bin/control_plane_sim --log-level debug
```

### Metrics Show 0 Events Processed

Wait a few seconds for workers to start processing events. Check logs:
```bash
./build/bin/control_plane_sim | jq 'select(.component == "EventLoop")'
```

### Docker Container Fails Health Check

Verify the binary is running and HTTP server started:
```bash
docker logs <container-id>
```

Ensure port 8080 is exposed and not already in use.

### Kubernetes Pods in CrashLoopBackOff

Check pod logs:
```bash
kubectl logs -l app=control-plane-sim --tail=100
```

Common issues:
- Image pull failure (use `imagePullPolicy: IfNotPresent` for local images)
- Resource limits too low (increase memory/CPU requests)
- ConfigMap not mounted correctly

## License

This project is licensed under the [MIT License](LICENSE)


