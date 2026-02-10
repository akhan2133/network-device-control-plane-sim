#pragma once

#include "port_manager.h"
#include "config.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <memory>

namespace control_plane {

// Event loop that manages simulation timing and worker threads
class EventLoop {
public:
    EventLoop(std::shared_ptr<PortManager> port_manager, const Config& config);
    ~EventLoop();
    
    // Start the event loop and worker threads
    void start();
    
    // Stop the event loop gracefully
    void stop();
    
    // Check if running
    bool is_running() const { return running_.load(); }
    
    // Get current tick count (for determinism)
    uint64_t get_tick_count() const { return tick_count_.load(); }

private:
    std::shared_ptr<PortManager> port_manager_;
    Config config_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> tick_count_;
    
    // Random number generation (with optional seed for determinism)
    std::mt19937 rng_;
    std::mutex rng_mutex_; // Protect RNG access
    
    // Worker threads
    std::vector<std::thread> worker_threads_;
    std::thread tick_thread_;
    
    // Worker functions
    void tick_loop();
    void heartbeat_worker(int worker_id);
    void flap_injector_worker(int worker_id);
    
    // Helper: should inject flap this tick?
    bool should_inject_flap();
    
    // Helper: generate random flap duration
    int generate_flap_duration_ms();
};

} // namespace control_plane
