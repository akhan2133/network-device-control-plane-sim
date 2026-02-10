#include "config.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

namespace control_plane {

Config Config::load_from_file(const std::string& path) {
    Config config;  // Start with defaults
    
    try {
        YAML::Node yaml_config = YAML::LoadFile(path);
        
        if (!yaml_config.IsMap()) {
            std::cerr << "Warning: Config file " << path 
                      << " is not a valid YAML map, using defaults\n";
            return config;
        }
        
        // Parse ports_count with validation
        if (yaml_config["ports_count"]) {
            try {
                int value = yaml_config["ports_count"].as<int>();
                if (value > 0 && value <= 1000) {
                    config.ports_count = value;
                } else {
                    std::cerr << "Warning: ports_count value " << value 
                              << " out of range, using default " << config.ports_count << "\n";
                }
            } catch (const YAML::BadConversion& e) {
                std::cerr << "Warning: Failed to parse ports_count: " << e.what() 
                          << ", using default " << config.ports_count << "\n";
            }
        }
        
        // Parse tick_ms with validation
        if (yaml_config["tick_ms"]) {
            try {
                int value = yaml_config["tick_ms"].as<int>();
                if (value > 0 && value <= 10000) {
                    config.tick_ms = value;
                } else {
                    std::cerr << "Warning: tick_ms value " << value 
                              << " out of range, using default " << config.tick_ms << "\n";
                }
            } catch (const YAML::BadConversion& e) {
                std::cerr << "Warning: Failed to parse tick_ms: " << e.what() 
                          << ", using default " << config.tick_ms << "\n";
            }
        }
        
        // Parse flap_probability with validation
        if (yaml_config["flap_probability"]) {
            try {
                double value = yaml_config["flap_probability"].as<double>();
                if (value >= 0.0 && value <= 1.0) {
                    config.flap_probability = value;
                } else {
                    std::cerr << "Warning: flap_probability value " << value 
                              << " out of range, using default " << config.flap_probability << "\n";
                }
            } catch (const YAML::BadConversion& e) {
                std::cerr << "Warning: Failed to parse flap_probability: " << e.what() 
                          << ", using default " << config.flap_probability << "\n";
            }
        }
        
        // Parse flap_min_ms with validation
        if (yaml_config["flap_min_ms"]) {
            try {
                int value = yaml_config["flap_min_ms"].as<int>();
                if (value >= 0) {
                    config.flap_min_ms = value;
                } else {
                    std::cerr << "Warning: flap_min_ms value " << value 
                              << " out of range, using default " << config.flap_min_ms << "\n";
                }
            } catch (const YAML::BadConversion& e) {
                std::cerr << "Warning: Failed to parse flap_min_ms: " << e.what() 
                          << ", using default " << config.flap_min_ms << "\n";
            }
        }
        
        // Parse flap_max_ms with validation
        if (yaml_config["flap_max_ms"]) {
            try {
                int value = yaml_config["flap_max_ms"].as<int>();
                if (value >= 0) {
                    config.flap_max_ms = value;
                } else {
                    std::cerr << "Warning: flap_max_ms value " << value 
                              << " out of range, using default " << config.flap_max_ms << "\n";
                }
            } catch (const YAML::BadConversion& e) {
                std::cerr << "Warning: Failed to parse flap_max_ms: " << e.what() 
                          << ", using default " << config.flap_max_ms << "\n";
            }
        }
        
        // Parse log_level - trim whitespace
        if (yaml_config["log_level"]) {
            try {
                std::string value = yaml_config["log_level"].as<std::string>();
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                if (!value.empty()) {
                    config.log_level = value;
                } else {
                    std::cerr << "Warning: log_level is empty, using default " << config.log_level << "\n";
                }
            } catch (const YAML::BadConversion& e) {
                std::cerr << "Warning: Failed to parse log_level: " << e.what() 
                          << ", using default " << config.log_level << "\n";
            }
        }
        
        // Parse http_port with validation
        if (yaml_config["http_port"]) {
            try {
                int value = yaml_config["http_port"].as<int>();
                if (value >= 1 && value <= 65535) {
                    config.http_port = value;
                } else {
                    std::cerr << "Warning: http_port value " << value 
                              << " out of range, using default " << config.http_port << "\n";
                }
            } catch (const YAML::BadConversion& e) {
                std::cerr << "Warning: Failed to parse http_port: " << e.what() 
                          << ", using default " << config.http_port << "\n";
            }
        }
        
    } catch (const YAML::BadFile& e) {
        std::cerr << "Warning: Could not open config file: " << path 
                  << ", using defaults\n";
    } catch (const YAML::Exception& e) {
        std::cerr << "Warning: Error parsing config file " << path 
                  << ": " << e.what() << ", using defaults\n";
    } catch (const std::exception& e) {
        std::cerr << "Warning: Unexpected error loading config file " << path 
                  << ": " << e.what() << ", using defaults\n";
    }
    
    return config;
}

void Config::apply_cli_args(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        
        if (arg == "--help" || arg == "-h") {
            std::cout << "Control Plane Simulator\n"
                      << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --config PATH        Path to config YAML file\n"
                      << "  --ports N            Number of ports (default: 8)\n"
                      << "  --tick-ms MS         Tick duration in milliseconds (default: 100)\n"
                      << "  --seed N             Random seed for determinism\n"
                      << "  --log-level LEVEL    Log level: debug, info, warn, error (default: info)\n"
                      << "  --http-port PORT     HTTP server port (default: 8080)\n"
                      << "  --flap-probability P  Link flap probability 0.0-1.0 (default: 0.01)\n"
                      << "  --help               Show this help\n";
            exit(0);
        } else if (arg == "--config" && i + 1 < argc) {
            // Config file is handled separately
            i++;
        } else if (arg == "--ports" && i + 1 < argc) {
            ports_count = std::stoi(argv[++i]);
        } else if (arg == "--tick-ms" && i + 1 < argc) {
            tick_ms = std::stoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--log-level" && i + 1 < argc) {
            log_level = argv[++i];
        } else if (arg == "--http-port" && i + 1 < argc) {
            http_port = std::stoi(argv[++i]);
        } else if (arg == "--flap-probability" && i + 1 < argc) {
            flap_probability = std::stod(argv[++i]);
        }
    }
}

bool Config::validate() const {
    if (ports_count <= 0 || ports_count > 1000) {
        std::cerr << "Error: ports_count must be between 1 and 1000\n";
        return false;
    }
    
    if (tick_ms <= 0 || tick_ms > 10000) {
        std::cerr << "Error: tick_ms must be between 1 and 10000\n";
        return false;
    }
    
    if (flap_probability < 0.0 || flap_probability > 1.0) {
        std::cerr << "Error: flap_probability must be between 0.0 and 1.0\n";
        return false;
    }
    
    if (flap_min_ms < 0 || flap_max_ms < flap_min_ms) {
        std::cerr << "Error: Invalid flap duration range\n";
        return false;
    }
    
    if (http_port < 1 || http_port > 65535) {
        std::cerr << "Error: http_port must be between 1 and 65535\n";
        return false;
    }
    
    return true;
}

std::string Config::to_string() const {
    std::ostringstream oss;
    oss << "Configuration:\n"
        << "  ports_count: " << ports_count << "\n"
        << "  tick_ms: " << tick_ms << "\n"
        << "  flap_probability: " << flap_probability << "\n"
        << "  flap_min_ms: " << flap_min_ms << "\n"
        << "  flap_max_ms: " << flap_max_ms << "\n"
        << "  log_level: " << log_level << "\n"
        << "  http_port: " << http_port << "\n";
    
    if (seed.has_value()) {
        oss << "  seed: " << seed.value() << "\n";
    }
    
    return oss.str();
}

} // namespace control_plane
