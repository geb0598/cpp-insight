#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "insight/pipe_transport.h"
#include "insight/scope_profiler.h"
#include "insight/registry.h"

namespace insight {

class Client {
public:
    static Client& GetInstance() {
        static Client instance;
        return instance;
    }

    Client(const Client&)            = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&)                 = delete;
    Client& operator=(Client&&)      = delete;

    TransportResult Connect();
    void            Disconnect();
    void            SendFrame(FrameRecord frame);

private:
    Client() = default;
    ~Client() { Disconnect(); }

    TransportResult SendHandshake();

    void OnSessionStart();
    void OnSessionStop();

    void SendWorker();
    void RecvWorker();

    PipeClient                transport_;
    std::thread               send_thread_;
    std::thread               recv_thread_;
    std::queue<FrameRecord>   queue_;
    std::mutex                mutex_;
    std::condition_variable   cv_;
    std::atomic<bool>         is_running_        = false;
    std::atomic<bool>         is_session_active_ = false;
};

} // namespace insight