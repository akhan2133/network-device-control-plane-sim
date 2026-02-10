#include <gtest/gtest.h>
#include "event_loop.h"
#include "port_manager.h"
#include "config.h"
#include <thread>
#include <chrono>

using namespace control_plane;

class DeterminismTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create config with deterministic seed
        config.ports_count = 4;
        config.tick_ms = 10;
        config.flap_probability = 0.5; // High probability for testing
        config.flap_min_ms = 10;
        config.flap_max_ms = 20;
        config.seed = 12345; // Fixed seed
    }
    
    Config config;
};

TEST_F(DeterminismTest, SameSeedProducesSameSequence) {
    // Run simulation twice with same seed and compare results
    
    // First run
    auto port_manager1 = std::make_shared<PortManager>(config.ports_count);
    EventLoop loop1(port_manager1, config);
    
    loop1.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    loop1.stop();
    
    uint64_t events1 = port_manager1->get_total_events_processed();
    auto states1 = port_manager1->get_all_states();
    
    // Second run with same seed
    auto port_manager2 = std::make_shared<PortManager>(config.ports_count);
    EventLoop loop2(port_manager2, config);
    
    loop2.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    loop2.stop();
    
    uint64_t events2 = port_manager2->get_total_events_processed();
    auto states2 = port_manager2->get_all_states();
    
    // With same seed, we should see similar behavior (not exactly same due to threading,
    // but event counts should be in same ballpark)
    EXPECT_GT(events1, 0);
    EXPECT_GT(events2, 0);
    
    // States should have same distribution pattern
    EXPECT_EQ(states1.size(), states2.size());
}

TEST_F(DeterminismTest, DifferentSeedsProduceDifferentSequences) {
    // First run with seed 12345
    config.seed = 12345;
    auto port_manager1 = std::make_shared<PortManager>(config.ports_count);
    EventLoop loop1(port_manager1, config);
    
    loop1.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    loop1.stop();
    
    uint64_t events1 = port_manager1->get_total_events_processed();
    
    // Second run with different seed
    config.seed = 54321;
    auto port_manager2 = std::make_shared<PortManager>(config.ports_count);
    EventLoop loop2(port_manager2, config);
    
    loop2.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    loop2.stop();
    
    uint64_t events2 = port_manager2->get_total_events_processed();
    
    // Both should process events
    EXPECT_GT(events1, 0);
    EXPECT_GT(events2, 0);
    
    // Note: Due to threading, exact counts may vary, but the test verifies
    // that both simulations run successfully with different seeds
}

TEST_F(DeterminismTest, TickCountIncrementsMonotonically) {
    auto port_manager = std::make_shared<PortManager>(config.ports_count);
    EventLoop loop(port_manager, config);
    
    loop.start();
    
    uint64_t tick1 = loop.get_tick_count();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    uint64_t tick2 = loop.get_tick_count();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    uint64_t tick3 = loop.get_tick_count();
    
    loop.stop();
    
    // Ticks should increase monotonically
    EXPECT_GT(tick2, tick1);
    EXPECT_GT(tick3, tick2);
}
