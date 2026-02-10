#include <gtest/gtest.h>
#include "port_manager.h"
#include <thread>
#include <vector>

using namespace control_plane;

class ThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        port_manager = std::make_unique<PortManager>(8);
    }
    
    std::unique_ptr<PortManager> port_manager;
};

TEST_F(ThreadSafetyTest, ConcurrentEventProcessing) {
    const int num_threads = 4;
    const int events_per_thread = 100;
    std::vector<std::thread> threads;
    
    // Launch multiple threads processing events concurrently
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, events_per_thread]() {
            for (int i = 0; i < events_per_thread; i++) {
                int port_id = i % port_manager->get_num_ports();
                
                // Cycle through events
                if (i % 3 == 0) {
                    port_manager->process_port_event(port_id, PortEvent::POWER_ON);
                } else if (i % 3 == 1) {
                    port_manager->process_port_event(port_id, PortEvent::INIT_COMPLETE);
                } else {
                    port_manager->process_port_event(port_id, PortEvent::HEARTBEAT_OK);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify that all events were processed
    uint64_t total_events = port_manager->get_total_events_processed();
    EXPECT_EQ(total_events, num_threads * events_per_thread);
}

TEST_F(ThreadSafetyTest, ConcurrentStateReads) {
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> successful_reads(0);
    
    // Launch reader threads
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, &successful_reads]() {
            for (int i = 0; i < 100; i++) {
                auto states = port_manager->get_all_states();
                if (states.size() == static_cast<size_t>(port_manager->get_num_ports())) {
                    successful_reads.fetch_add(1);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All reads should succeed
    EXPECT_EQ(successful_reads.load(), num_threads * 100);
}

TEST_F(ThreadSafetyTest, ConcurrentReadAndWrite) {
    const int num_readers = 4;
    const int num_writers = 4;
    std::vector<std::thread> threads;
    std::atomic<bool> stop(false);
    
    // Launch writer threads
    for (int t = 0; t < num_writers; t++) {
        threads.emplace_back([this, t, &stop]() {
            int count = 0;
            while (!stop.load() && count < 50) {
                int port_id = count % port_manager->get_num_ports();
                port_manager->process_port_event(port_id, PortEvent::POWER_ON);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                count++;
            }
        });
    }
    
    // Launch reader threads
    for (int t = 0; t < num_readers; t++) {
        threads.emplace_back([this, &stop]() {
            int count = 0;
            while (!stop.load() && count < 50) {
                auto states = port_manager->get_all_states();
                EXPECT_EQ(states.size(), static_cast<size_t>(port_manager->get_num_ports()));
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                count++;
            }
        });
    }
    
    // Let them run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop.store(true);
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // No crashes = success
    EXPECT_GT(port_manager->get_total_events_processed(), 0);
}

TEST_F(ThreadSafetyTest, MetricsThreadSafety) {
    const int num_threads = 8;
    std::vector<std::thread> threads;
    
    // Launch threads that increment metrics
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this]() {
            for (int i = 0; i < 100; i++) {
                port_manager->get_metrics().increment_counter("test_counter", 1);
                port_manager->get_metrics().set_gauge("test_gauge", static_cast<double>(i));
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Counter should be exactly num_threads * 100
    uint64_t counter_value = port_manager->get_metrics().get_counter("test_counter");
    EXPECT_EQ(counter_value, num_threads * 100);
}
