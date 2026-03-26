#pragma once

#include <atomic>
#include <deque>
#include <filesystem>
#include <future>
#include <memory>
#include <thread>

#include "insight/connection.h"
#include "insight/pipe_transport.h"
#include "insight/scope_profiler.h"
#include "insight/registry.h"

namespace insight {

// -------------------------------------------------
// ServerState
// -------------------------------------------------
enum class ServerState {
    OFFLINE  , 
    LISTENING,
    CONNECTED,
    RECORDING,
};

// -------------------------------------------------
// Server
// -------------------------------------------------
class Server : public Connection {
public:
    static Server& GetInstance() {
        static Server instance;
        return instance;
    }

    static std::filesystem::path GenerateSessionFilename();

    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&)                 = delete;
    Server& operator=(Server&&)      = delete;

    void Listen();
    void Stop();

    void StartRecording();
    void StopRecording();

    void SaveSession(const std::filesystem::path& path);
    void LoadSession(const std::filesystem::path& path);

    ServerState GetState()    const { return state_.load(); }
    bool        IsListening() const { return state_ == ServerState::LISTENING; }
    bool        IsConnected() const { return state_ == ServerState::CONNECTED; }
    bool        IsRecording() const { return state_ == ServerState::RECORDING; }

protected:
    virtual TransportResult Send(PacketType type, const ByteBuffer& payload)                        override;
    virtual TransportResult Receive(PacketHeader& out_header, ByteBuffer& out_payload)              override;
    virtual void            OnPacketReceived(const PacketHeader& header, const ByteBuffer& payload) override;
    virtual void            OnDisconnected()                                                        override;

    void SetState(ServerState state) { state_.store(state); }

private:
    Server() = default;
    ~Server() { Stop(); }

    void AcceptWorker();

    void OnHandshake(const ByteBuffer& data);
    void OnFrame(const ByteBuffer& data);

    std::vector<std::unique_ptr<Group>>      owned_groups_;
    std::vector<std::unique_ptr<Descriptor>> owned_descs_;

    PipeServer               data_pipe_;
    PipeServer               control_pipe_;
    std::thread              accept_thread_;
    std::atomic<ServerState> state_ = ServerState::OFFLINE;
};

} // namespace insight