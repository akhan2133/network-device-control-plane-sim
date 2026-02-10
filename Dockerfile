# Multi-stage build for minimal runtime image
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build the project
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -u 1000 -s /bin/bash simuser

# Create directories
RUN mkdir -p /app/config /app/bin

# Copy binary and config from builder
COPY --from=builder /build/build/bin/control_plane_sim /app/bin/
COPY --from=builder /build/config/config.yaml /app/config/

# Set ownership
RUN chown -R simuser:simuser /app

# Switch to non-root user
USER simuser
WORKDIR /app

# Expose HTTP port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run the simulator
ENTRYPOINT ["/app/bin/control_plane_sim"]
CMD ["--config", "/app/config/config.yaml"]
