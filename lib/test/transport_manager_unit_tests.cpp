#include "mtp32.h"

#include <thread>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace MTP32;
using ::testing::ElementsAreArray;

static Packet MakePacket(uint8_t value)
{
    Packet packet{};
    packet[0] = value;
    return packet;
}

//
// -----------------------------------------------------------------------------
//  TEST SUITE
// -----------------------------------------------------------------------------
TEST(TransportManagerUnitTests, SlaveStartsInReceiveStateAndDoesNotTransmitImmediately)
{
    bool tx_called = false;

    TransportManager tm(
        Role::SLAVE,
        [&](auto){ tx_called = true; },
        [](){ return std::nullopt; },
        [&](auto){}
    );

    // Constructor does not transmit
    EXPECT_FALSE(tx_called);

    // First Run() is RECEIVE → no TX unless packet arrives
    tm.Run(std::chrono::system_clock::now());
    EXPECT_FALSE(tx_called);
}

TEST(TransportManagerUnitTests, MasterStartsInTransmitStateButDoesNotTransmitUntilRun)
{
    bool tx_called = false;

    TransportManager tm(
        Role::MASTER,
        [&](auto){ tx_called = true; },
        [](){ return std::nullopt; },
        [&](auto){}
    );

    // Constructor sets state to TRANSMIT but does not call Transmit()
    EXPECT_FALSE(tx_called);

    // First Run() triggers Transmit()
    tm.Run(std::chrono::system_clock::now());
    EXPECT_TRUE(tx_called);
}

TEST(TransportManagerUnitTests, MasterSendsNopWhenQueueEmpty)
{
    Packet sent_packet{};
    bool tx_called = false;

    TransportManager tm(
        Role::MASTER,
        [&](auto p){ tx_called = true; sent_packet = p; },
        [](){ return std::nullopt; },
        [&](auto){}
    );

    tm.Run(std::chrono::system_clock::now()); // TRANSMIT → NOP

    EXPECT_TRUE(tx_called);
    EXPECT_THAT(sent_packet, ElementsAreArray(NOP_PACKET));
}

TEST(TransportManagerUnitTests, MasterSendsQueuedPacket)
{
    Packet sent_packet{};
    bool tx_called = false;

    auto packet = MakePacket(0xAB);

    TransportManager tm(
        Role::MASTER,
        [&](auto p){ tx_called = true; sent_packet = p; },
        [](){ return std::nullopt; },
        [&](auto){}
    );

    tm.EnqueuePacket(packet);
    tm.Run(std::chrono::system_clock::now()); // TRANSMIT → SEND_MESSAGE

    EXPECT_TRUE(tx_called);
    EXPECT_EQ(sent_packet[0], 0xAB);
}

TEST(TransportManagerUnitTests, ReceiveCallbackDeliversPacket)
{
    auto expected_packet = MakePacket(0x55);
    bool received_called = false;
    Packet received_packet{};

    TransportManager tm(
        Role::SLAVE,
        [&](auto){},
        [&](){ return expected_packet; },
        [&](auto p){ received_called = true; received_packet = p; }
    );

    tm.Run(std::chrono::system_clock::now()); // RECEIVE → packet available

    EXPECT_TRUE(received_called);
    EXPECT_EQ(received_packet[0], 0x55);
}

TEST(TransportManagerUnitTests, ReceiveTransitionsToTransmitButDoesNotTransmitUntilNextRun)
{
    bool tx_called = false;

    TransportManager tm(
        Role::SLAVE,
        [&](auto){ tx_called = true; },
        [&](){ return MakePacket(0x33); },
        [&](auto){}
    );

    tm.Run(std::chrono::system_clock::now()); // RECEIVE → packet → state becomes TRANSMIT
    EXPECT_FALSE(tx_called); // No TX yet

    tm.Run(std::chrono::system_clock::now()); // Now TRANSMIT executes
    EXPECT_TRUE(tx_called);
}

TEST(TransportManagerUnitTests, MasterRxTimeoutTriggersTransmitOnNextRun)
{
    bool tx_called = false;

    TransportManager tm(
        Role::MASTER,
        [&](auto){ tx_called = true; },
        [](){ return std::nullopt; },
        [&](auto){}
    );

    auto initial_timepoint = std::chrono::system_clock::now();

    // First Run() performs initial transmit (master always starts in TRANSMIT)
    tx_called = false;
    tm.Run(initial_timepoint);
    EXPECT_TRUE(tx_called);

    // Now master is in RECEIVE and timeout timer is set
    tx_called = false;

    // Advance forward in time enough for the RX timeout to happen.
    auto timeout_timepoint = initial_timepoint + TransportManager::RX_TIMEOUT + std::chrono::milliseconds{10};

    tm.Run(timeout_timepoint); // RECEIVE → timeout → state becomes TRANSMIT
    EXPECT_FALSE(tx_called); // No TX yet

    tm.Run(timeout_timepoint); // Now TRANSMIT executes
    EXPECT_TRUE(tx_called);
}

TEST(TransportManagerUnitTests, ErrorCallbackIsStoredButNotAutomaticallyInvoked)
{
    bool error_called = false;

    TransportManager tm(
        Role::MASTER,
        [&](auto){},
        [](){ return std::nullopt; },
        [&](auto){}
    );

    // Store the callback
    tm.SetErrorCallback([&](auto, auto){ error_called = true; });

    // TransportManager never calls the error callback internally
    // So error_called must remain false
    EXPECT_FALSE(error_called);

    // Setting the callback again should still not invoke it
    tm.SetErrorCallback([&](auto, auto){ error_called = true; });
    EXPECT_FALSE(error_called);
}
