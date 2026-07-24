#pragma once
#include <functional>
#include <array>
#include <chrono>
#include <optional>

namespace MTP32 
{

static inline constexpr uint8_t MAXIMUM_PACKET_SIZE = 32;
using Packet = std::array<uint8_t, MAXIMUM_PACKET_SIZE>;
static inline constexpr Packet NOP_PACKET = { 0 };

enum class Role 
{
    MASTER,
    SLAVE
};

/**
 * @brief This class represents an implementation of the micro-transport protocol for 32 byte messages. 
 * @note This class behaves like a classic finite-state-machine. 
 * - The FSM makes progress through consecutive calls of TransportManager::Run.
 * - The FSM should never be blocked by either the TX or RX callback logic. Everything should be non-blocking.
 * @warning This class does not take ownership of the communication endpoint, such as a file descriptor, for sending and receiving packets. Both TX and RX logic is
 *  configurable via callback injections.
 * @warning This class is not thread-safe. Take caution with how callback functions are defined.
 */
class TransportManager 
{
public:

    static constexpr std::chrono::milliseconds RX_TIMEOUT {100};
    static constexpr size_t DEFAULT_TX_BUFFER_SIZE = 64;

    using PollRxCallback = std::function<std::optional<Packet>()>;
    using TxCallback = std::function<void(Packet)>;
    using ReceivedPacketCallback = std::function<void(Packet)>;

    /**
     * @param role : Impacts behaviour affecting deadlock avoidance and communication order. For example, the "master" role takes responsibility for preventing RX deadlock.
     * @param tx_callback : TransportManager activates this callback to send a packet.
     * @param poll_rx_callback : TransportManager activates this callback when it wants to read a packet.
     * @param received_packet_callback : This is activated to publish a packet received by activation of the poll_rx_callback.
     * @warning It is assumed that the TX and RX callbacks are non-blocking. The RX callback is expected to be called periodically such as when TransportManager::Run is 
     * called, assuming that TransportManager is in the receive state.
     */
    TransportManager(Role role, 
        TxCallback tx_callback, 
        PollRxCallback poll_rx_callback,
        ReceivedPacketCallback received_packet_callback,
        std::chrono::milliseconds rx_timeout = RX_TIMEOUT
    );

    /**
     * @brief Enqueues a packet for transmission at the next opportunity.
     * @returns The result of whether the packet was enqueued. A packet can be rejected for if there is not enough buffer capacity.
     */
    bool EnqueuePacket(const Packet& packet_bytes);

    /**
     * @brief Runs the primary worker task that is responsible for managing TX and RX responsibilities. This function must be called periodically to advance the state machine.
     * It is recommended that this function be called every 50 milliseconds at the latest because it is twice the frequency of the RX timeout interval (100 milliseconds).
     * @param current_time : This argument tells the state machine how much time has passed between calls of this function.
     */
    void Run(std::chrono::time_point<std::chrono::steady_clock> current_time);

    /**
     * @brief Provides the number of TX messages that are in the outbound queue waiting to be transmitted.
     */
    size_t GetPendingTxMessageCount() const { return m_tx_queue_count; }

    /**
     * @brief Reports the time at which the last packet was received. This is useful for gauging the radio connection quality.
     */
    std::chrono::time_point<std::chrono::steady_clock> GetLastRxTimepoint() const { return m_last_rx_timepoint; }

private:

    enum class State
    {
        TRANSMIT
        , RECEIVE
    };

    PollRxCallback m_poll_rx_callback;
    TxCallback m_tx_callback;
    ReceivedPacketCallback m_received_packet_callback;
    std::chrono::milliseconds m_rx_timeout;

    Role m_role;
    State m_state;
    std::array<Packet, DEFAULT_TX_BUFFER_SIZE> m_tx_queue;
    size_t m_tx_queue_head { 0 };
    size_t m_tx_queue_tail { 0 };
    size_t m_tx_queue_count { 0 };

    std::chrono::time_point<std::chrono::steady_clock> m_rx_timeout_timepoint { std::chrono::seconds{0} };
    std::chrono::time_point<std::chrono::steady_clock> m_current_time { std::chrono::seconds{0} };
    std::chrono::time_point<std::chrono::steady_clock> m_last_rx_timepoint { std::chrono::seconds{0} };

    void InitializeState();
    void ChangeState(State state);
    bool IsMaster();
    void Transmit();
    void Receive();
    bool HasPendingOutboundPacket();
    void SendNopPacket();
    void SendMessagePacket();
    void SetRxTimeoutTimepoint();
    bool HasRxTimeoutOccured();
    void SetCurrentTime(const std::chrono::time_point<std::chrono::steady_clock>& current_time);
    void SetLastRxTime(const std::chrono::time_point<std::chrono::steady_clock>& current_time);

    bool Enqueue(const Packet& packet);
    Packet& Dequeue();
};

} // namespace MTP32
