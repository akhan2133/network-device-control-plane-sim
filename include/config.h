#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace control_plane {

// Configuration structure
struct Config {
    int ports_count = 8;
    int tick_ms = 100;
    double flap_probability = 0.01;  // Probability per tick per port
    int flap_min_ms = 500;
    int flap_max_ms = 5000;
    std::string log_level = "info";  // debug, info, warn, error
    std::optional<uint32_t> seed;    // Random seed for determinism
    int http_port = 8080;
    
    // Load from YAML file
    static Config load_from_file(const std::string& path);
    
    // Override with command-line arguments
    void apply_cli_args(int argc, char** argv);
    
    // Validate configuration
    bool validate() const;
    
    // Print configuration
    std::string to_string() const;
};

} // namespace control_plane
