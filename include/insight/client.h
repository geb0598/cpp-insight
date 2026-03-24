#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "insight/connection.h"
#include "insight/pipe_transport.h"
#include "insight/scope_profiler.h"
#include "insight/registry.h"

namespace insight {

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

    bool IsSessionActive() const { return is_session_active_; }

protected:
    virtual TransportResult Send(PacketType type, const ByteBuffer& payload)                        override;
    virtual TransportResult Receive(PacketHeader& out_header, ByteBuffer& out_payload)              override;
    virtual void            OnPacketReceived(const PacketHeader& header, const ByteBuffer& payload) override;
    virtual void            OnDisconnected()                                                        override;

private:
    Client() = default;
    ~Client() { Disconnect(); }

    void ConnectWorker();
    TransportResult SendHandshake();

    void OnSessionStart() { is_session_active_ = true; }
    void OnSessionStop()  { is_session_active_ = false; }

    PipeClient        data_pipe_;
    PipeClient        control_pipe_;
    std::thread       connect_thread_;
    std::atomic<bool> is_session_active_ = false;
};

} // namespace insight