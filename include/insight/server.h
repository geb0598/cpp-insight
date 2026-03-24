#pragma once

#include <atomic>
#include <deque>
#include <future>
#include <memory>
#include <thread>

#include "insight/connection.h"
#include "insight/pipe_transport.h"
#include "insight/scope_profiler.h"
#include "insight/registry.h"

namespace insight {

class Server : public Connection {
public:
    static Server& GetInstance() {
        static Server instance;
        return instance;
    }

    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&)                 = delete;
    Server& operator=(Server&&)      = delete;

    void Listen();
    void Stop();

    void StartSession();
    void StopSession();

    bool IsSessionActive() const { return is_session_active_; }

protected:
    virtual TransportResult Send(PacketType type, const ByteBuffer& payload)                        override;
    virtual TransportResult Receive(PacketHeader& out_header, ByteBuffer& out_payload)              override;
    virtual void            OnPacketReceived(const PacketHeader& header, const ByteBuffer& payload) override;
    virtual void            OnDisconnected()                                                        override;

private:
    Server() = default;
    ~Server() { Stop(); }

    void AcceptWorker();

    void OnHandshake(const ByteBuffer& data);
    void OnFrame(const ByteBuffer& data);

    std::vector<std::unique_ptr<Group>>      owned_groups_;
    std::vector<std::unique_ptr<Descriptor>> owned_descs_;

    PipeServer        data_pipe_;
    PipeServer        control_pipe_;
    std::thread       accept_thread_;
    std::atomic<bool> is_session_active_ = false;
};

} // namespace insight