#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "insight/transport.h"

namespace insight {

class Connection {
public:
    virtual ~Connection() = default;

    Connection(const Connection&)            = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&)                 = delete;
    Connection& operator=(Connection&&)      = delete;

    bool IsRunning()   const { return is_running_; }
    bool IsConnected() const { return is_connected_; }

    void SetOnConnected(std::function<void()> callback) { 
        on_connected_ = std::move(callback);
    }
    void SetOnDisconnected(std::function<void()> callback) {
        on_disconnected_ = std::move(callback);
    }

protected:
    Connection() = default;

    void StartWorkers();
    void StopWorkers();
    void EnqueuePacket(PacketType type, ByteBuffer payload);

    void NotifyConnected() {
        is_connected_ = true;
        if (on_connected_) {
            on_connected_();
        }
    }

    void NotifyDisconnected() {
        is_connected_ = false;
        if (on_disconnected_) {
            on_disconnected_();
        }
    }

    virtual TransportResult Send(PacketType type, const ByteBuffer& payload)                        = 0;
    virtual TransportResult Receive(PacketHeader& out_header, ByteBuffer& out_payload)              = 0;
    virtual void            OnPacketReceived(const PacketHeader& header, const ByteBuffer& payload) = 0;
    virtual void            OnDisconnected()                                                        = 0;

private:
    struct Packet {
        PacketType type;
        ByteBuffer payload;
    };

    void SendWorker();
    void RecvWorker();

    std::thread             send_thread_;
    std::thread             recv_thread_;
    std::queue<Packet>      send_queue_;
    std::mutex              send_mutex_;
    std::condition_variable send_cv_;

    std::atomic<bool> is_running_  = false;
    std::atomic<bool> is_connected_= false;

    std::function<void()> on_connected_;
    std::function<void()> on_disconnected_;
};

} // namespace insight