#pragma once

#include <string>
#include <chrono>

namespace control_plane {

// Port states following the state machine: DOWN -> INIT -> UP
enum class PortState {
    DOWN,  // Port is down/offline
    INIT,  // Port is initializing
    UP     // Port is operational
};

// Events that can trigger state transitions
enum class PortEvent {
    POWER_ON,      // Brings port from DOWN to INIT
    INIT_COMPLETE, // Brings port from INIT to UP
    LINK_FLAP,     // Brings port from any state to DOWN
    HEARTBEAT_OK   // Keeps port in UP (no transition)
};

// Convert enum to string for logging
std::string port_state_to_string(PortState state);
std::string port_event_to_string(PortEvent event);

class PortStateMachine {
public:
    explicit PortStateMachine(int port_id);
    
    // Process an event and potentially transition state
    // Returns true if state changed
    bool process_event(PortEvent event);
    
    // Get current state
    PortState get_state() const { return state_; }
    
    // Get port ID
    int get_port_id() const { return port_id_; }
    
    // Get state transition count
    uint64_t get_transition_count() const { return transition_count_; }
    
    // Get last transition time
    std::chrono::steady_clock::time_point get_last_transition_time() const {
        return last_transition_time_;
    }

private:
    int port_id_;
    PortState state_;
    uint64_t transition_count_;
    std::chrono::steady_clock::time_point last_transition_time_;
    
    // Helper to transition to new state
    void transition_to(PortState new_state);
};

} // namespace control_plane
