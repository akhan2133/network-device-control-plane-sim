#include "port_state_machine.h"
#include "logger.h"
#include <sstream>

namespace control_plane {

std::string port_state_to_string(PortState state) {
    switch (state) {
        case PortState::DOWN: return "DOWN";
        case PortState::INIT: return "INIT";
        case PortState::UP: return "UP";
        default: return "UNKNOWN";
    }
}

std::string port_event_to_string(PortEvent event) {
    switch (event) {
        case PortEvent::POWER_ON: return "POWER_ON";
        case PortEvent::INIT_COMPLETE: return "INIT_COMPLETE";
        case PortEvent::LINK_FLAP: return "LINK_FLAP";
        case PortEvent::HEARTBEAT_OK: return "HEARTBEAT_OK";
        default: return "UNKNOWN";
    }
}

PortStateMachine::PortStateMachine(int port_id)
    : port_id_(port_id),
      state_(PortState::DOWN),
      transition_count_(0),
      last_transition_time_(std::chrono::steady_clock::now()) {
    
    std::stringstream ss;
    ss << "Port " << port_id_ << " initialized in DOWN state";
    Logger::instance().debug(ss.str(), "PortStateMachine", port_id_);
}

bool PortStateMachine::process_event(PortEvent event) {
    PortState old_state = state_;
    bool state_changed = false;
    
    // State machine logic
    switch (state_) {
        case PortState::DOWN:
            if (event == PortEvent::POWER_ON) {
                transition_to(PortState::INIT);
                state_changed = true;
            }
            break;
            
        case PortState::INIT:
            if (event == PortEvent::INIT_COMPLETE) {
                transition_to(PortState::UP);
                state_changed = true;
            } else if (event == PortEvent::LINK_FLAP) {
                transition_to(PortState::DOWN);
                state_changed = true;
            }
            break;
            
        case PortState::UP:
            if (event == PortEvent::LINK_FLAP) {
                transition_to(PortState::DOWN);
                state_changed = true;
            }
            // HEARTBEAT_OK keeps us in UP state (no transition)
            break;
    }
    
    if (state_changed) {
        std::stringstream ss;
        ss << "Port " << port_id_ << " transitioned from " 
           << port_state_to_string(old_state) << " to " 
           << port_state_to_string(state_) << " on event " 
           << port_event_to_string(event);
        Logger::instance().info(ss.str(), "PortStateMachine", port_id_);
    } else {
        std::stringstream ss;
        ss << "Port " << port_id_ << " received event " 
           << port_event_to_string(event) << " in state " 
           << port_state_to_string(state_) << " (no transition)";
        Logger::instance().debug(ss.str(), "PortStateMachine", port_id_);
    }
    
    return state_changed;
}

void PortStateMachine::transition_to(PortState new_state) {
    state_ = new_state;
    transition_count_++;
    last_transition_time_ = std::chrono::steady_clock::now();
}

} // namespace control_plane
