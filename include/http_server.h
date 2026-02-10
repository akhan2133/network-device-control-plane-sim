#pragma once

#include "port_manager.h"
#include <memory>
#include <atomic>
#include <thread>

namespace control_plane {

// Minimal HTTP server for health and metrics endpoints
class HttpServer {
public:
    HttpServer(std::shared_ptr<PortManager> port_manager, int port = 8080);
    ~HttpServer();
    
    // Start server in background thread
    void start();
    
    // Stop server
    void stop();
    
    // Check if running
    bool is_running() const { return running_.load(); }

private:
    std::shared_ptr<PortManager> port_manager_;
    int port_;
    std::atomic<bool> running_;
    
    // Implementation details hidden (uses cpp-httplib)
    void* server_impl_; // Opaque pointer to avoid header dependency
    std::thread server_thread_;
};

} // namespace control_plane
