#pragma once

#include <atomic>
#include <string>
#include <map>
#include <mutex>

namespace control_plane {

// Thread-safe metrics collector for Prometheus-style exposition
class Metrics {
public:
    Metrics();
    
    // Increment a counter
    void increment_counter(const std::string& name, uint64_t value = 1);
    
    // Set a gauge value
    void set_gauge(const std::string& name, double value);
    
    // Get counter value
    uint64_t get_counter(const std::string& name) const;
    
    // Get gauge value
    double get_gauge(const std::string& name) const;
    
    // Export all metrics in Prometheus text format
    std::string export_prometheus() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::atomic<uint64_t>> counters_;
    std::map<std::string, std::atomic<double>> gauges_;
    
    // Helper to get or create counter
    std::atomic<uint64_t>& get_or_create_counter(const std::string& name);
    
    // Helper to get or create gauge
    std::atomic<double>& get_or_create_gauge(const std::string& name);
};

} // namespace control_plane
