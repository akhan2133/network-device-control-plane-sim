#include "port_manager.h"
#include "logger.h"
#include <sstream>

namespace control_plane {

PortManager::PortManager(int num_ports)
    : num_ports_(num_ports),
      port_mutexes_(num_ports),
      total_events_processed_(0) {
    
    // Initialize all ports
    ports_.reserve(num_ports);
    for (int i = 0; i < num_ports; i++) {
        ports_.push_back(std::make_unique<PortStateMachine>(i));
    }
    
    std::stringstream ss;
    ss << "PortManager initialized with " << num_ports << " ports";
    Logger::instance().info(ss.str(), "PortManager");
    
    // Initialize metrics
    metrics_.set_gauge("ports_total", static_cast<double>(num_ports));
    metrics_.set_gauge("ports_down", static_cast<double>(num_ports));
    metrics_.set_gauge("ports_init", 0.0);
    metrics_.set_gauge("ports_up", 0.0);
}

bool PortManager::process_port_event(int port_id, PortEvent event) {
    if (!is_valid_port(port_id)) {
        std::stringstream ss;
        ss << "Invalid port ID: " << port_id;
        Logger::instance().error(ss.str(), "PortManager");
        return false;
    }
    
    // Lock this specific port's mutex
    std::lock_guard<std::mutex> lock(port_mutexes_[port_id]);
    
    // Capture old state before processing event
    PortState old_state = ports_[port_id]->get_state();
    
    // Process the event
    bool changed = ports_[port_id]->process_event(event);
    
    total_events_processed_.fetch_add(1);
    
    // Update metrics
    metrics_.increment_counter("events_processed_total");
    
    if (changed) {
        metrics_.increment_counter("state_transitions_total");
        
        // Capture new state after transition
        PortState new_state = ports_[port_id]->get_state();
        
        // Update state gauges: decrement old state, increment new state
        switch (old_state) {
            case PortState::DOWN:
                metrics_.set_gauge("ports_down", metrics_.get_gauge("ports_down") - 1.0);
                break;
            case PortState::INIT:
                metrics_.set_gauge("ports_init", metrics_.get_gauge("ports_init") - 1.0);
                break;
            case PortState::UP:
                metrics_.set_gauge("ports_up", metrics_.get_gauge("ports_up") - 1.0);
                break;
        }
        
        switch (new_state) {
            case PortState::DOWN:
                metrics_.set_gauge("ports_down", metrics_.get_gauge("ports_down") + 1.0);
                break;
            case PortState::INIT:
                metrics_.set_gauge("ports_init", metrics_.get_gauge("ports_init") + 1.0);
                break;
            case PortState::UP:
                metrics_.set_gauge("ports_up", metrics_.get_gauge("ports_up") + 1.0);
                break;
        }
    }
    
    return changed;
}

std::vector<PortState> PortManager::get_all_states() const {
    std::vector<PortState> states;
    states.reserve(num_ports_);
    
    for (int i = 0; i < num_ports_; i++) {
        std::lock_guard<std::mutex> lock(port_mutexes_[i]);
        states.push_back(ports_[i]->get_state());
    }
    
    return states;
}

PortState PortManager::get_port_state(int port_id) const {
    if (!is_valid_port(port_id)) {
        return PortState::DOWN;
    }
    
    std::lock_guard<std::mutex> lock(port_mutexes_[port_id]);
    return ports_[port_id]->get_state();
}

} // namespace control_plane
