#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "insights/connection.h"
#include "insights/pipe_transport.h"
#include "insights/scope_profiler.h"
#include "insights/registry.h"

namespace insights {

// -------------------------------------------------
// ClientState
// -------------------------------------------------
enum class ClientState {
    DISCONNECTED,
    CONNECTED   ,
    RECORDING   ,
};

// -------------------------------------------------
// Client
// -------------------------------------------------
class Client : public Connection {
public:
    static Client& GetInstance() {
        static Client instance;
        return instance;
    }

    Client(const Client&)            = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&)                 = delete;
    Client& operator=(Client&&)      = delete;

    void Connect();
    void Disconnect();
    void SendFrame(FrameRecord frame);

    ClientState GetState()        const { return state_.load(); }
    bool        IsDisconnected()  const { return GetState() == ClientState::DISCONNECTED; }
    bool        IsConnected()     const { return GetState() == ClientState::CONNECTED; }
    bool        IsRecording()     const { return GetState() == ClientState::RECORDING; }
    bool        IsClientRunning() const { return is_client_running_; }

protected:
    virtual TransportResult Send(PacketType type, const ByteBuffer& payload)                        override;
    virtual TransportResult Receive(PacketHeader& out_header, ByteBuffer& out_payload)              override;
    virtual void            OnPacketReceived(const PacketHeader& header, const ByteBuffer& payload) override;
    virtual void            OnDisconnected()                                                        override;

    void SetState(ClientState state) { state_.store(state); }

private:
    Client() = default;
    ~Client() { Disconnect(); }

    void ConnectWorker();
    TransportResult SendHandshake();

    PipeClient               data_pipe_;
    PipeClient               control_pipe_;
    std::thread              connect_thread_;
    std::atomic<bool>        is_client_running_ = false;
    std::atomic<ClientState> state_ = ClientState::DISCONNECTED;
};

} // namespace insightss