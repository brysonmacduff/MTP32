#include "mtp32.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>
#include <vector>
#include <chrono>

using namespace MTP32;
using ::testing::ElementsAreArray;

static Packet MakePacket(uint8_t first_byte_value)
{
    Packet packet{};
    packet[0] = first_byte_value;
    return packet;
}

/**
 * @brief Represents one side of the simulated RF link.
 *
 * Each Endpoint contains:
 *  - A TransportManager instance
 *  - A FIFO queue representing packets "on the wire" waiting to be read
 *  - A vector of packets delivered to the application layer
 *
 * The TX and RX callbacks of each endpoint are wired to the peer endpoint
 * inside each test. This allows us to simulate a real master–slave link
 * without hardware.
 */
struct Endpoint
{
    Role role;
    TransportManager transport_manager;
    std::deque<Packet> inbound_wire_fifo;
    std::vector<Packet> application_received_packets;

    Endpoint(
        Role endpoint_role,
        TransportManager::TxCallback transmit_callback,
        TransportManager::RxCallback receive_callback
    )
        : role(endpoint_role)
        , transport_manager(
            endpoint_role,
            transmit_callback,
            receive_callback,
            [&](Packet received_packet)
            {
                application_received_packets.push_back(received_packet);
            }
        )
    {}
};

//
// ============================================================================
//  TEST SUITE
// ============================================================================
TEST(TransportManagerIntegrationTests, MasterSendsMessageToSlave)
{
    using Clock = std::chrono::system_clock;
    auto simulated_time = Clock::now();

    // These FIFOs represent the RF link between master and slave.
    std::deque<Packet> master_inbound_fifo;
    std::deque<Packet> slave_inbound_fifo;

    // -----------------------------
    // Endpoint Initialization
    // -----------------------------
    // Master TX pushes packets into the slave's inbound FIFO.
    // Master RX pops packets from its own inbound FIFO.
    Endpoint master_endpoint(
        Role::MASTER,
        [&](Packet transmitted_packet)
        {
            slave_inbound_fifo.push_back(transmitted_packet);
        },
        [&]() -> std::optional<Packet>
        {
            if (master_inbound_fifo.empty())
                return std::nullopt;

            Packet packet = master_inbound_fifo.front();
            master_inbound_fifo.pop_front();
            return packet;
        }
    );

    // Slave TX pushes packets into the master's inbound FIFO.
    // Slave RX pops packets from its own inbound FIFO.
    Endpoint slave_endpoint(
        Role::SLAVE,
        [&](Packet transmitted_packet)
        {
            master_inbound_fifo.push_back(transmitted_packet);
        },
        [&]() -> std::optional<Packet>
        {
            if (slave_inbound_fifo.empty())
                return std::nullopt;

            Packet packet = slave_inbound_fifo.front();
            slave_inbound_fifo.pop_front();
            return packet;
        }
    );

    // -----------------------------
    // Test Goal
    // -----------------------------
    // Verify that a message enqueued on the master is eventually delivered
    // to the slave's application layer.
    Packet message_packet = MakePacket(0x42);
    master_endpoint.transport_manager.EnqueuePacket(message_packet);

    for (int iteration = 0; iteration < 20 && slave_endpoint.application_received_packets.empty(); ++iteration)
    {
        simulated_time += std::chrono::milliseconds(10);
        master_endpoint.transport_manager.Run(simulated_time);
        slave_endpoint.transport_manager.Run(simulated_time);
    }

    ASSERT_EQ(slave_endpoint.application_received_packets.size(), 1u);
    EXPECT_EQ(slave_endpoint.application_received_packets[0][0], 0x42);
}

TEST(TransportManagerIntegrationTests, SlaveRepliesToMasterMessage)
{
    using Clock = std::chrono::system_clock;
    auto simulated_time = Clock::now();

    std::deque<Packet> master_inbound_fifo;
    std::deque<Packet> slave_inbound_fifo;

    std::vector<Packet> master_application_received;
    std::vector<Packet> slave_application_received;

    // -----------------------------
    // Endpoint Initialization
    // -----------------------------
    // Master endpoint wiring
    TransportManager master_transport_manager(
        Role::MASTER,
        [&](Packet transmitted_packet)
        {
            slave_inbound_fifo.push_back(transmitted_packet);
        },
        [&]() -> std::optional<Packet>
        {
            if (master_inbound_fifo.empty())
                return std::nullopt;

            Packet packet = master_inbound_fifo.front();
            master_inbound_fifo.pop_front();
            return packet;
        },
        [&](Packet received_packet)
        {
            master_application_received.push_back(received_packet);
        }
    );

    // Slave endpoint wiring
    TransportManager slave_transport_manager(
        Role::SLAVE,
        [&](Packet transmitted_packet)
        {
            master_inbound_fifo.push_back(transmitted_packet);
        },
        [&]() -> std::optional<Packet>
        {
            if (slave_inbound_fifo.empty())
                return std::nullopt;

            Packet packet = slave_inbound_fifo.front();
            slave_inbound_fifo.pop_front();
            return packet;
        },
        [&](Packet received_packet)
        {
            slave_application_received.push_back(received_packet);

            // Slave replies immediately when receiving a packet
            Packet reply_packet = MakePacket(0x99);
            slave_transport_manager.EnqueuePacket(reply_packet);
        }
    );

    // -----------------------------
    // Test Goal
    // -----------------------------
    // Master sends a request; slave receives it and replies; master receives reply.
    Packet request_packet = MakePacket(0x11);
    master_transport_manager.EnqueuePacket(request_packet);

    for (int iteration = 0; iteration < 50 && master_application_received.empty(); ++iteration)
    {
        simulated_time += std::chrono::milliseconds(10);
        master_transport_manager.Run(simulated_time);
        slave_transport_manager.Run(simulated_time);
    }

    ASSERT_FALSE(slave_application_received.empty());
    EXPECT_EQ(slave_application_received[0][0], 0x11);

    ASSERT_FALSE(master_application_received.empty());
    EXPECT_EQ(master_application_received[0][0], 0x99);
}

TEST(TransportManagerIntegrationTests, MasterSendsNopWhenNoMessagesAreQueued)
{
    using Clock = std::chrono::system_clock;
    auto simulated_time = Clock::now();

    std::deque<Packet> master_inbound_fifo;
    std::deque<Packet> slave_inbound_fifo;

    // -----------------------------
    // Endpoint Initialization
    // -----------------------------
    Endpoint master_endpoint(
        Role::MASTER,
        [&](Packet transmitted_packet)
        {
            slave_inbound_fifo.push_back(transmitted_packet);
        },
        [&]() -> std::optional<Packet>
        {
            if (master_inbound_fifo.empty())
                return std::nullopt;

            Packet packet = master_inbound_fifo.front();
            master_inbound_fifo.pop_front();
            return packet;
        }
    );

    Endpoint slave_endpoint(
        Role::SLAVE,
        [&](Packet transmitted_packet)
        {
            master_inbound_fifo.push_back(transmitted_packet);
        },
        [&]() -> std::optional<Packet>
        {
            if (slave_inbound_fifo.empty())
                return std::nullopt;

            Packet packet = slave_inbound_fifo.front();
            slave_inbound_fifo.pop_front();
            return packet;
        }
    );

    // -----------------------------
    // Test Goal
    // -----------------------------
    // With no queued messages, the master should send NOP packets.
    for (int iteration = 0; iteration < 5; ++iteration)
    {
        simulated_time += std::chrono::milliseconds(10);
        master_endpoint.transport_manager.Run(simulated_time);
        slave_endpoint.transport_manager.Run(simulated_time);
    }

    ASSERT_FALSE(slave_endpoint.application_received_packets.empty());
    EXPECT_THAT(slave_endpoint.application_received_packets[0], ElementsAreArray(NOP_PACKET));
}

TEST(TransportManagerIntegrationTests, MasterAvoidsDeadlockUsingRxTimeout)
{
    using Clock = std::chrono::system_clock;
    auto simulated_time = Clock::now();

    std::deque<Packet> master_inbound_fifo;
    std::deque<Packet> slave_inbound_fifo;

    // -----------------------------
    // Endpoint Initialization
    // -----------------------------
    Endpoint master_endpoint(
        Role::MASTER,
        [&](Packet transmitted_packet)
        {
            slave_inbound_fifo.push_back(transmitted_packet);
        },
        [&]() -> std::optional<Packet>
        {
            if (master_inbound_fifo.empty())
                return std::nullopt;

            Packet packet = master_inbound_fifo.front();
            master_inbound_fifo.pop_front();
            return packet;
        }
    );

    Endpoint slave_endpoint(
        Role::SLAVE,
        [&](Packet transmitted_packet)
        {
            master_inbound_fifo.push_back(transmitted_packet);
        },
        [&]() -> std::optional<Packet>
        {
            if (slave_inbound_fifo.empty())
                return std::nullopt;

            Packet packet = slave_inbound_fifo.front();
            slave_inbound_fifo.pop_front();
            return packet;
        }
    );

    // -----------------------------
    // Test Goal
    // -----------------------------
    // Even if no packets are exchanged, the master must periodically transmit
    // (NOP packets) to avoid RX deadlock.
    for (int iteration = 0; iteration < 50; ++iteration)
    {
        simulated_time += std::chrono::milliseconds(10);
        master_endpoint.transport_manager.Run(simulated_time);
        slave_endpoint.transport_manager.Run(simulated_time);
    }

    ASSERT_FALSE(slave_endpoint.application_received_packets.empty());
}
