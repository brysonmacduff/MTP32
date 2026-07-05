#include "mtp32.h"

namespace MTP32 
{
TransportManager::TransportManager(Role role, TxCallback tx_callback, PollRxCallback poll_rx_callback, ReceivedPacketCallback received_packet_callback, std::chrono::milliseconds rx_timeout)
: m_role(role)
, m_tx_callback(tx_callback)
, m_poll_rx_callback(poll_rx_callback)
, m_received_packet_callback(received_packet_callback)
, m_rx_timeout(rx_timeout)
{
    InitializeState();
}

bool TransportManager::EnqueuePacket(const Packet& packet_bytes)
{
    return Enqueue(packet_bytes);
}

void TransportManager::Run(std::chrono::time_point<std::chrono::steady_clock> current_time)
{
    SetCurrentTime(current_time);

    switch (m_state)
    {
        case State::TRANSMIT:
        {
            Transmit();
            break;
        }
        case State::RECEIVE:
        {
            Receive();
            break;
        }
        default:
            break;
    }
}

void TransportManager::InitializeState()
{
    if(IsMaster())
    {
        ChangeState(State::TRANSMIT);
    }
    else
    {
        ChangeState(State::RECEIVE);
    }
}

void TransportManager::ChangeState(State state)
{
    if(m_state == State::RECEIVE)
    {   
        // Set the RX timeout timepoint in case this is the master
        SetRxTimeoutTimepoint();
    }

    m_state = state;
}

bool TransportManager::IsMaster()
{
    return m_role == Role::MASTER;
}

void TransportManager::Transmit()
{
    if(HasPendingOutboundPacket())
    {
        SendMessagePacket();
    }
    else
    {
        SendNopPacket();
    }

    // Transition to the receive state after sending a packet
    ChangeState(State::RECEIVE);
}

void TransportManager::Receive()
{
    // RX timeout behavior only exists for the master, not the slave.
    if(IsMaster() && HasRxTimeoutOccured())
    {
        ChangeState(State::TRANSMIT);
        return;
    }

    // Attempt to read packet from the endpoint
    auto rx_packet_opt = m_poll_rx_callback();

    if(rx_packet_opt.has_value())
    {   
        // Report the received packet to the callback listener
        m_received_packet_callback(rx_packet_opt.value());

        SetLastRxTime(std::chrono::steady_clock::now());

        // Transition to the transmit state after a packet is received.
        ChangeState(State::TRANSMIT);
    }
}

bool TransportManager::HasPendingOutboundPacket()
{
    return m_tx_queue_count > 0;
}

void TransportManager::SendNopPacket()
{
    m_tx_callback(NOP_PACKET);
}

void TransportManager::SendMessagePacket()
{
    if(not HasPendingOutboundPacket())
    {
        return;
    }

    auto tx_packet = Dequeue();

    m_tx_callback(tx_packet);
}

void TransportManager::SetRxTimeoutTimepoint()
{
    m_rx_timeout_timepoint = m_current_time + m_rx_timeout;
}

bool TransportManager::HasRxTimeoutOccured()
{
    return m_current_time >= m_rx_timeout_timepoint;
}

void TransportManager::SetCurrentTime(const std::chrono::time_point<std::chrono::steady_clock>& current_time)
{
    m_current_time = current_time;
}

void TransportManager::SetLastRxTime(const std::chrono::time_point<std::chrono::steady_clock>& current_time)
{
    m_last_rx_timepoint = current_time;
}

bool TransportManager::Enqueue(const Packet &packet)
{
    if(m_tx_queue_count == MAXIMUM_TX_BUFFER_SIZE)
    {
        return false;
    }

    m_tx_queue[m_tx_queue_tail] = packet;
    m_tx_queue_tail = (m_tx_queue_tail + 1) % MAXIMUM_TX_BUFFER_SIZE;
    ++m_tx_queue_count;

    return true;
}

Packet& TransportManager::Dequeue()
{
    Packet& packet = m_tx_queue[m_tx_queue_head];
    m_tx_queue_head = (m_tx_queue_head + 1) % MAXIMUM_TX_BUFFER_SIZE;
    --m_tx_queue_count;
    return packet;
}

} // namespace MTP32
