#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <chrono>
#include <mutex>

namespace control_plane {

// Log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

// Convert log level to string
std::string log_level_to_string(LogLevel level);

// Parse log level from string
LogLevel parse_log_level(const std::string& level_str);

// Structured JSON logger (thread-safe)
class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    // Set minimum log level
    void set_level(LogLevel level) {
        min_level_ = level;
    }
    
    // Log a structured message
    void log(LogLevel level, const std::string& message, 
             const std::string& component = "",
             int port_id = -1);
    
    // Convenience methods
    void debug(const std::string& message, const std::string& component = "", int port_id = -1) {
        log(LogLevel::DEBUG, message, component, port_id);
    }
    
    void info(const std::string& message, const std::string& component = "", int port_id = -1) {
        log(LogLevel::INFO, message, component, port_id);
    }
    
    void warn(const std::string& message, const std::string& component = "", int port_id = -1) {
        log(LogLevel::WARN, message, component, port_id);
    }
    
    void error(const std::string& message, const std::string& component = "", int port_id = -1) {
        log(LogLevel::ERROR, message, component, port_id);
    }

private:
    Logger() : min_level_(LogLevel::INFO) {}
    
    LogLevel min_level_;
    std::mutex mutex_;
    
    // Get ISO8601 timestamp
    std::string get_timestamp() const;
    
    // Escape JSON string
    std::string escape_json(const std::string& str) const;
};

// Convenience macros
#define LOG_DEBUG(msg, comp, port) control_plane::Logger::instance().debug(msg, comp, port)
#define LOG_INFO(msg, comp, port) control_plane::Logger::instance().info(msg, comp, port)
#define LOG_WARN(msg, comp, port) control_plane::Logger::instance().warn(msg, comp, port)
#define LOG_ERROR(msg, comp, port) control_plane::Logger::instance().error(msg, comp, port)

} // namespace control_plane
