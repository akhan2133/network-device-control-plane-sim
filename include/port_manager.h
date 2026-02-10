#pragma once

#include "port_state_machine.h"
#include "metrics.h"
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>

namespace control_plane {

// Thread-safe manager for all ports
class PortManager {
public:
    explicit PortManager(int num_ports);
    
    // Process an event on a specific port
    // Thread-safe: can be called from multiple threads
    bool process_port_event(int port_id, PortEvent event);
    
    // Get snapshot of all port states (thread-safe)
    std::vector<PortState> get_all_states() const;
    
    // Get state of specific port (thread-safe)
    PortState get_port_state(int port_id) const;
    
    // Get total number of ports
    int get_num_ports() const { return num_ports_; }
    
    // Get total events processed across all ports
    uint64_t get_total_events_processed() const {
        return total_events_processed_.load();
    }
    
    // Get metrics reference
    Metrics& get_metrics() { return metrics_; }
    const Metrics& get_metrics() const { return metrics_; }

private:
    int num_ports_;
    std::vector<std::unique_ptr<PortStateMachine>> ports_;
    mutable std::vector<std::mutex> port_mutexes_; // One mutex per port
    std::atomic<uint64_t> total_events_processed_;
    Metrics metrics_;
    
    // Validate port ID
    bool is_valid_port(int port_id) const {
        return port_id >= 0 && port_id < num_ports_;
    }
};

} // namespace control_plane
