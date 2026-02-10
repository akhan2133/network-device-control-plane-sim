#include "http_server.h"
#include "logger.h"
#include "httplib.h"
#include <sstream>

namespace control_plane {

HttpServer::HttpServer(std::shared_ptr<PortManager> port_manager, int port)
    : port_manager_(port_manager),
      port_(port),
      running_(false),
      server_impl_(nullptr) {
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_.load()) {
        Logger::instance().warn("HttpServer already running", "HttpServer");
        return;
    }
    
    running_.store(true);
    
    server_thread_ = std::thread([this]() {
        auto* svr = new httplib::Server();
        server_impl_ = svr;
        
        // Health endpoint
        svr->Get("/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("{\"status\":\"ok\"}", "application/json");
        });
        
        // Metrics endpoint
        svr->Get("/metrics", [this](const httplib::Request&, httplib::Response& res) {
            std::string metrics = port_manager_->get_metrics().export_prometheus();
            res.set_content(metrics, "text/plain; version=0.0.4");
        });
        
        // Status endpoint (additional)
        svr->Get("/status", [this](const httplib::Request&, httplib::Response& res) {
            std::ostringstream json;
            json << "{\n";
            json << "  \"total_ports\": " << port_manager_->get_num_ports() << ",\n";
            json << "  \"total_events\": " << port_manager_->get_total_events_processed() << ",\n";
            json << "  \"ports_down\": " << port_manager_->get_metrics().get_gauge("ports_down") << ",\n";
            json << "  \"ports_init\": " << port_manager_->get_metrics().get_gauge("ports_init") << ",\n";
            json << "  \"ports_up\": " << port_manager_->get_metrics().get_gauge("ports_up") << "\n";
            json << "}";
            res.set_content(json.str(), "application/json");
        });
        
        std::stringstream ss;
        ss << "HTTP server listening on port " << port_;
        Logger::instance().info(ss.str(), "HttpServer");
        
        // This blocks until stop() is called
        svr->listen("0.0.0.0", port_);
        
        delete svr;
        server_impl_ = nullptr;
    });
}

void HttpServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    Logger::instance().info("Stopping HTTP server", "HttpServer");
    running_.store(false);
    
    if (server_impl_) {
        auto* svr = static_cast<httplib::Server*>(server_impl_);
        svr->stop();
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    Logger::instance().info("HTTP server stopped", "HttpServer");
}

} // namespace control_plane
