#!/bin/bash
# Integration test for Control Plane Simulator
# 
# This test validates:
# - HTTP endpoints (/health, /metrics, /status)
# - Event processing and metrics collection
# - Prometheus exposition format
#
# Determinism: Flap injection is disabled (--flap-probability 0) to ensure
# deterministic port states during CI. Without this, random link flaps make
# port state metrics non-deterministic, causing flaky tests.
#
# Prometheus parsing: We use '^metric_name ' (with space anchor) to match
# only metric value lines, not "# TYPE" or "# HELP" comment lines. This is
# required because Prometheus format includes both metadata comments and
# metric lines, and we need to extract only numeric values.

set -euo pipefail

echo "=== Control Plane Simulator Integration Test ==="

# Configuration
BINARY="./build/bin/control_plane_sim"
HTTP_PORT=8080
WAIT_TIME=5

# Check if binary exists
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found at $BINARY"
    echo "Please build the project first: mkdir build && cd build && cmake .. && make"
    exit 1
fi

echo "Starting simulator in background..."
# Disable flap injection for deterministic CI testing
# Without this, random link flaps make port state metrics non-deterministic
$BINARY --ports 4 --tick-ms 100 --http-port $HTTP_PORT --log-level info --flap-probability 0 &
SIM_PID=$!

# Ensure we kill the process on exit (handle all signals)
cleanup() {
    echo "Cleaning up..."
    if kill -0 "$SIM_PID" 2>/dev/null; then
        kill "$SIM_PID" 2>/dev/null || true
        wait "$SIM_PID" 2>/dev/null || true
    fi
    exit
}
trap cleanup EXIT INT TERM

echo "Simulator started with PID $SIM_PID"
echo "Waiting ${WAIT_TIME}s for startup..."
sleep $WAIT_TIME

# Verify simulator is still running
if ! kill -0 "$SIM_PID" 2>/dev/null; then
    echo "ERROR: Simulator process died during startup"
    exit 1
fi

# Test 1: Health endpoint
echo ""
echo "Test 1: Testing /health endpoint..."
HEALTH_RESPONSE=$(curl -s http://localhost:$HTTP_PORT/health)
echo "Response: $HEALTH_RESPONSE"

if echo "$HEALTH_RESPONSE" | grep -q '"status":"ok"'; then
    echo "✓ Health check passed"
else
    echo "✗ Health check failed"
    exit 1
fi

# Test 2: Metrics endpoint
echo ""
echo "Test 2: Testing /metrics endpoint..."
METRICS_RESPONSE=$(curl -s http://localhost:$HTTP_PORT/metrics)
echo "Response (first 10 lines):"
echo "$METRICS_RESPONSE" | head -10

# Check for metric presence (any line containing the metric name)
if echo "$METRICS_RESPONSE" | grep -q "control_plane_events_processed_total"; then
    echo "✓ Metrics endpoint passed - found events_processed_total"
else
    echo "✗ Metrics endpoint failed"
    exit 1
fi

if echo "$METRICS_RESPONSE" | grep -q "control_plane_ports_total"; then
    echo "✓ Metrics endpoint passed - found ports_total"
else
    echo "✗ Metrics endpoint failed"
    exit 1
fi

# Test 3: Verify events are being processed
echo ""
echo "Test 3: Verifying event processing..."
# Use '^metric_name ' (with space anchor) to match ONLY metric value lines,
# not "# TYPE" or "# HELP" comment lines in Prometheus format
EVENTS_1=$(curl -s http://localhost:$HTTP_PORT/metrics | grep '^control_plane_events_processed_total ' | awk '{print $2}')
echo "Events processed (sample 1): $EVENTS_1"

# Validate we got a number
if ! [[ "$EVENTS_1" =~ ^[0-9]+$ ]]; then
    echo "✗ ERROR: Failed to extract numeric metric value. Got: '$EVENTS_1'"
    echo "This usually means the Prometheus format changed or parsing is incorrect."
    exit 1
fi

sleep 2

EVENTS_2=$(curl -s http://localhost:$HTTP_PORT/metrics | grep '^control_plane_events_processed_total ' | awk '{print $2}')
echo "Events processed (sample 2): $EVENTS_2"

# Validate we got a number
if ! [[ "$EVENTS_2" =~ ^[0-9]+$ ]]; then
    echo "✗ ERROR: Failed to extract numeric metric value. Got: '$EVENTS_2'"
    echo "This usually means the Prometheus format changed or parsing is incorrect."
    exit 1
fi

# Compare numeric values (use arithmetic comparison)
if [ "$EVENTS_2" -gt "$EVENTS_1" ]; then
    echo "✓ Event processing verified - events increasing over time ($EVENTS_1 -> $EVENTS_2)"
else
    echo "✗ Event processing check failed - events not increasing ($EVENTS_1 -> $EVENTS_2)"
    exit 1
fi

# Test 4: Status endpoint
echo ""
echo "Test 4: Testing /status endpoint..."
STATUS_RESPONSE=$(curl -s http://localhost:$HTTP_PORT/status)
echo "Response: $STATUS_RESPONSE"

if echo "$STATUS_RESPONSE" | grep -q "total_ports"; then
    echo "✓ Status endpoint passed"
else
    echo "✗ Status endpoint failed"
    exit 1
fi

echo ""
echo "=== All integration tests passed! ==="
echo "Stopping simulator..."

# Cleanup will be handled by trap, but we can explicitly kill here too
kill "$SIM_PID" 2>/dev/null || true
wait "$SIM_PID" 2>/dev/null || true

echo "Integration test completed successfully"
exit 0
