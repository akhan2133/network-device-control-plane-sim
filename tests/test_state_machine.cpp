#include <gtest/gtest.h>
#include "port_state_machine.h"

using namespace control_plane;

class PortStateMachineTest : public ::testing::Test {
protected:
    void SetUp() override {
        port = std::make_unique<PortStateMachine>(0);
    }
    
    std::unique_ptr<PortStateMachine> port;
};

TEST_F(PortStateMachineTest, InitialStateIsDown) {
    EXPECT_EQ(port->get_state(), PortState::DOWN);
}

TEST_F(PortStateMachineTest, DownToPowerOnTransitionsToInit) {
    EXPECT_TRUE(port->process_event(PortEvent::POWER_ON));
    EXPECT_EQ(port->get_state(), PortState::INIT);
}

TEST_F(PortStateMachineTest, InitToInitCompleteTransitionsToUp) {
    port->process_event(PortEvent::POWER_ON);
    EXPECT_TRUE(port->process_event(PortEvent::INIT_COMPLETE));
    EXPECT_EQ(port->get_state(), PortState::UP);
}

TEST_F(PortStateMachineTest, UpToLinkFlapTransitionsToDown) {
    port->process_event(PortEvent::POWER_ON);
    port->process_event(PortEvent::INIT_COMPLETE);
    EXPECT_TRUE(port->process_event(PortEvent::LINK_FLAP));
    EXPECT_EQ(port->get_state(), PortState::DOWN);
}

TEST_F(PortStateMachineTest, HeartbeatInUpDoesNotTransition) {
    port->process_event(PortEvent::POWER_ON);
    port->process_event(PortEvent::INIT_COMPLETE);
    EXPECT_FALSE(port->process_event(PortEvent::HEARTBEAT_OK));
    EXPECT_EQ(port->get_state(), PortState::UP);
}

TEST_F(PortStateMachineTest, InvalidTransitionsDoNotChangeState) {
    // INIT_COMPLETE in DOWN state should not transition
    EXPECT_FALSE(port->process_event(PortEvent::INIT_COMPLETE));
    EXPECT_EQ(port->get_state(), PortState::DOWN);
    
    // HEARTBEAT_OK in DOWN state should not transition
    EXPECT_FALSE(port->process_event(PortEvent::HEARTBEAT_OK));
    EXPECT_EQ(port->get_state(), PortState::DOWN);
}

TEST_F(PortStateMachineTest, TransitionCountIncrementsOnStateChange) {
    EXPECT_EQ(port->get_transition_count(), 0);
    
    port->process_event(PortEvent::POWER_ON);
    EXPECT_EQ(port->get_transition_count(), 1);
    
    port->process_event(PortEvent::INIT_COMPLETE);
    EXPECT_EQ(port->get_transition_count(), 2);
    
    port->process_event(PortEvent::HEARTBEAT_OK);
    EXPECT_EQ(port->get_transition_count(), 2); // No transition
}

TEST_F(PortStateMachineTest, FullCycleDownToUpAndBack) {
    // DOWN -> INIT -> UP -> DOWN
    EXPECT_EQ(port->get_state(), PortState::DOWN);
    
    port->process_event(PortEvent::POWER_ON);
    EXPECT_EQ(port->get_state(), PortState::INIT);
    
    port->process_event(PortEvent::INIT_COMPLETE);
    EXPECT_EQ(port->get_state(), PortState::UP);
    
    port->process_event(PortEvent::LINK_FLAP);
    EXPECT_EQ(port->get_state(), PortState::DOWN);
    
    EXPECT_EQ(port->get_transition_count(), 3);
}

TEST_F(PortStateMachineTest, LinkFlapFromInitGoesToDown) {
    port->process_event(PortEvent::POWER_ON);
    EXPECT_EQ(port->get_state(), PortState::INIT);
    
    EXPECT_TRUE(port->process_event(PortEvent::LINK_FLAP));
    EXPECT_EQ(port->get_state(), PortState::DOWN);
}
