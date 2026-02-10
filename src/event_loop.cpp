#include "event_loop.h"
#include "logger.h"
#include <sstream>
#include <chrono>

namespace control_plane {

EventLoop::EventLoop(std::shared_ptr<PortManager> port_manager, const Config& config)
    : port_manager_(port_manager),
      config_(config),
      running_(false),
      tick_count_(0) {
    
    // Initialize RNG with seed if provided
    if (config_.seed.has_value()) {
        rng_.seed(config_.seed.value());
        std::stringstream ss;
        ss << "EventLoop initialized with deterministic seed: " << config_.seed.value();
        Logger::instance().info(ss.str(), "EventLoop");
    } else {
        std::random_device rd;
        rng_.seed(rd());
        Logger::instance().info("EventLoop initialized with random seed", "EventLoop");
    }
}

EventLoop::~EventLoop() {
    stop();
}

void EventLoop::start() {
    if (running_.load()) {
        Logger::instance().warn("EventLoop already running", "EventLoop");
        return;
    }
    
    running_.store(true);
    Logger::instance().info("Starting EventLoop", "EventLoop");
    
    // Start tick thread
    tick_thread_ = std::thread(&EventLoop::tick_loop, this);
    
    // Start worker threads
    int num_workers = 4; // 2 for heartbeat, 2 for flap injection
    for (int i = 0; i < num_workers; i++) {
        if (i < 2) {
            worker_threads_.emplace_back(&EventLoop::heartbeat_worker, this, i);
        } else {
            worker_threads_.emplace_back(&EventLoop::flap_injector_worker, this, i);
        }
    }
    
    Logger::instance().info("EventLoop started with worker threads", "EventLoop");
}

void EventLoop::stop() {
    if (!running_.load()) {
        return;
    }
    
    Logger::instance().info("Stopping EventLoop", "EventLoop");
    running_.store(false);
    
    // Wait for tick thread
    if (tick_thread_.joinable()) {
        tick_thread_.join();
    }
    
    // Wait for all worker threads
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    Logger::instance().info("EventLoop stopped", "EventLoop");
}

void EventLoop::tick_loop() {
    Logger::instance().info("Tick loop started", "EventLoop");
    
    while (running_.load()) {
        tick_count_.fetch_add(1);
        
        // Sleep for tick duration
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.tick_ms));
        
        // Log tick every 100 ticks
        if (tick_count_ % 100 == 0) {
            std::stringstream ss;
            ss << "Tick " << tick_count_.load() << " - Events processed: " 
               << port_manager_->get_total_events_processed();
            Logger::instance().debug(ss.str(), "EventLoop");
        }
    }
    
    Logger::instance().info("Tick loop stopped", "EventLoop");
}

void EventLoop::heartbeat_worker(int worker_id) {
    std::stringstream ss;
    ss << "Heartbeat worker " << worker_id << " started";
    Logger::instance().info(ss.str(), "EventLoop");
    
    int num_ports = port_manager_->get_num_ports();
    
    while (running_.load()) {
        // Process heartbeats for assigned ports (simple round-robin)
        for (int port_id = worker_id; port_id < num_ports; port_id += 2) {
            if (!running_.load()) break;
            
            PortState state = port_manager_->get_port_state(port_id);
            
            // State machine progression logic
            switch (state) {
                case PortState::DOWN:
                    // Power on the port
                    port_manager_->process_port_event(port_id, PortEvent::POWER_ON);
                    break;
                    
                case PortState::INIT:
                    // Wait a bit then complete init (simulate initialization time)
                    std::this_thread::sleep_for(std::chrono::milliseconds(config_.tick_ms * 2));
                    if (running_.load()) {
                        port_manager_->process_port_event(port_id, PortEvent::INIT_COMPLETE);
                    }
                    break;
                    
                case PortState::UP:
                    // Send periodic heartbeat
                    port_manager_->process_port_event(port_id, PortEvent::HEARTBEAT_OK);
                    break;
            }
        }
        
        // Sleep between heartbeat cycles
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.tick_ms * 5));
    }
    
    ss.str("");
    ss << "Heartbeat worker " << worker_id << " stopped";
    Logger::instance().info(ss.str(), "EventLoop");
}

void EventLoop::flap_injector_worker(int worker_id) {
    std::stringstream ss;
    ss << "Flap injector worker " << worker_id << " started";
    Logger::instance().info(ss.str(), "EventLoop");
    
    int num_ports = port_manager_->get_num_ports();
    
    while (running_.load()) {
        // Check each port for potential flap injection
        for (int port_id = (worker_id - 2); port_id < num_ports; port_id += 2) {
            if (!running_.load()) break;
            
            // Only inject flaps on UP ports
            if (port_manager_->get_port_state(port_id) == PortState::UP) {
                if (should_inject_flap()) {
                    int flap_duration = generate_flap_duration_ms();
                    
                    std::stringstream log_ss;
                    log_ss << "Injecting link flap on port " << port_id 
                           << " for " << flap_duration << "ms";
                    Logger::instance().info(log_ss.str(), "EventLoop", port_id);
                    
                    port_manager_->process_port_event(port_id, PortEvent::LINK_FLAP);
                    port_manager_->get_metrics().increment_counter("link_flaps_injected_total");
                    
                    // Simulate flap duration
                    std::this_thread::sleep_for(std::chrono::milliseconds(flap_duration));
                }
            }
        }
        
        // Sleep between flap checks
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.tick_ms * 10));
    }
    
    ss.str("");
    ss << "Flap injector worker " << worker_id << " stopped";
    Logger::instance().info(ss.str(), "EventLoop");
}

bool EventLoop::should_inject_flap() {
    std::lock_guard<std::mutex> lock(rng_mutex_);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng_) < config_.flap_probability;
}

int EventLoop::generate_flap_duration_ms() {
    std::lock_guard<std::mutex> lock(rng_mutex_);
    std::uniform_int_distribution<int> dist(config_.flap_min_ms, config_.flap_max_ms);
    return dist(rng_);
}

} // namespace control_plane
