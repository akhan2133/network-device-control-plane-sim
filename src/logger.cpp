#include "logger.h"
#include <iomanip>
#include <ctime>
#include <algorithm>

namespace control_plane {

std::string log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

LogLevel parse_log_level(const std::string& level_str) {
    std::string lower = level_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "debug") return LogLevel::DEBUG;
    if (lower == "info") return LogLevel::INFO;
    if (lower == "warn" || lower == "warning") return LogLevel::WARN;
    if (lower == "error") return LogLevel::ERROR;
    
    return LogLevel::INFO; // default
}

void Logger::log(LogLevel level, const std::string& message, 
                 const std::string& component, int port_id) {
    if (level < min_level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Build JSON log line
    std::ostringstream json;
    json << "{";
    json << "\"timestamp\":\"" << get_timestamp() << "\",";
    json << "\"level\":\"" << log_level_to_string(level) << "\",";
    json << "\"message\":\"" << escape_json(message) << "\"";
    
    if (!component.empty()) {
        json << ",\"component\":\"" << escape_json(component) << "\"";
    }
    
    if (port_id >= 0) {
        json << ",\"port_id\":" << port_id;
    }
    
    json << "}\n";
    
    // Write to stdout
    std::cout << json.str() << std::flush;
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return oss.str();
}

std::string Logger::escape_json(const std::string& str) const {
    std::ostringstream oss;
    
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    oss << c;
                }
                break;
        }
    }
    
    return oss.str();
}

} // namespace control_plane
