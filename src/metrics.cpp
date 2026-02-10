#include "metrics.h"
#include <sstream>
#include <iomanip>

namespace control_plane {

Metrics::Metrics() {
    // Initialize common metrics
    increment_counter("events_processed_total", 0);
    increment_counter("state_transitions_total", 0);
    increment_counter("link_flaps_injected_total", 0);
    
    set_gauge("ports_total", 0.0);
    set_gauge("ports_down", 0.0);
    set_gauge("ports_init", 0.0);
    set_gauge("ports_up", 0.0);
}

void Metrics::increment_counter(const std::string& name, uint64_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& counter = get_or_create_counter(name);
    counter.fetch_add(value);
}

void Metrics::set_gauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& gauge = get_or_create_gauge(name);
    gauge.store(value);
}

uint64_t Metrics::get_counter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = counters_.find(name);
    if (it != counters_.end()) {
        return it->second.load();
    }
    return 0;
}

double Metrics::get_gauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return it->second.load();
    }
    return 0.0;
}

std::string Metrics::export_prometheus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // Export counters
    oss << "# TYPE control_plane_events_processed_total counter\n";
    for (const auto& [name, value] : counters_) {
        oss << "control_plane_" << name << " " << value.load() << "\n";
    }
    
    // Export gauges
    oss << "# TYPE control_plane_ports gauge\n";
    for (const auto& [name, value] : gauges_) {
        oss << "control_plane_" << name << " " << std::fixed 
            << std::setprecision(2) << value.load() << "\n";
    }
    
    return oss.str();
}

std::atomic<uint64_t>& Metrics::get_or_create_counter(const std::string& name) {
    // Note: This assumes mutex is already locked by caller
    auto it = counters_.find(name);
    if (it == counters_.end()) {
        counters_[name].store(0);
        it = counters_.find(name);
    }
    return const_cast<std::atomic<uint64_t>&>(it->second);
}

std::atomic<double>& Metrics::get_or_create_gauge(const std::string& name) {
    // Note: This assumes mutex is already locked by caller
    auto it = gauges_.find(name);
    if (it == gauges_.end()) {
        gauges_[name].store(0.0);
        it = gauges_.find(name);
    }
    return const_cast<std::atomic<double>&>(it->second);
}

} // namespace control_plane
