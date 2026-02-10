#include "config.h"
#include "logger.h"
#include "port_manager.h"
#include "event_loop.h"
#include "http_server.h"
#include <csignal>
#include <atomic>
#include <iostream>

using namespace control_plane;

// Global flag for graceful shutdown
std::atomic<bool> shutdown_requested(false);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        Logger::instance().info("Shutdown signal received", "main");
        shutdown_requested.store(true);
    }
}

int main(int argc, char** argv) {
    // Load configuration
    Config config;
    
    // Check for config file argument
    std::string config_path = "config/config.yaml";
    for (int i = 1; i < argc - 1; i++) {
        if (std::string(argv[i]) == "--config") {
            config_path = argv[i + 1];
            break;
        }
    }
    
    // Load from file first
    config = Config::load_from_file(config_path);
    
    // Override with CLI args
    config.apply_cli_args(argc, argv);
    
    // Validate configuration
    if (!config.validate()) {
        std::cerr << "Invalid configuration. Exiting.\n";
        return 1;
    }
    
    // Set log level
    Logger::instance().set_level(parse_log_level(config.log_level));
    
    // Print configuration
    Logger::instance().info("Starting Control Plane Simulator", "main");
    std::cout << config.to_string() << std::endl;
    
    // Setup signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        // Create port manager
        auto port_manager = std::make_shared<PortManager>(config.ports_count);
        
        // Create and start HTTP server
        HttpServer http_server(port_manager, config.http_port);
        http_server.start();
        
        // Create and start event loop
        EventLoop event_loop(port_manager, config);
        event_loop.start();
        
        Logger::instance().info("Control plane simulator is running", "main");
        Logger::instance().info("Press Ctrl+C to stop", "main");
        
        // Main loop - wait for shutdown
        while (!shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Graceful shutdown
        Logger::instance().info("Initiating graceful shutdown", "main");
        
        event_loop.stop();
        http_server.stop();
        
        // Print final statistics
        std::ostringstream stats;
        stats << "Final statistics:\n"
              << "  Total events processed: " << port_manager->get_total_events_processed() << "\n"
              << "  State transitions: " << port_manager->get_metrics().get_counter("state_transitions_total") << "\n"
              << "  Link flaps injected: " << port_manager->get_metrics().get_counter("link_flaps_injected_total") << "\n"
              << "  Ports UP: " << port_manager->get_metrics().get_gauge("ports_up") << "\n"
              << "  Ports INIT: " << port_manager->get_metrics().get_gauge("ports_init") << "\n"
              << "  Ports DOWN: " << port_manager->get_metrics().get_gauge("ports_down");
        
        Logger::instance().info(stats.str(), "main");
        Logger::instance().info("Control plane simulator stopped cleanly", "main");
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        Logger::instance().error(std::string("Fatal error: ") + e.what(), "main");
        return 1;
    }
    
    return 0;
}
