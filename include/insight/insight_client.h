#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "insight/pipe_transport.h"
#include "insight/scope_profiler.h"
#include "insight/stat_registry.h"

namespace insight {

class InsightClient {
public:
    static InsightClient& GetInstance() {
        static InsightClient instance;
        return instance;
    }

    InsightClient(const InsightClient&)            = delete;
    InsightClient& operator=(const InsightClient&) = delete;
    InsightClient(InsightClient&&)                 = delete;
    InsightClient& operator=(InsightClient&&)      = delete;

    TransportResult Connect();
    void            Disconnect();
    void            SendFrame(FrameRecord frame);

private:
    InsightClient() = default;
    ~InsightClient() { Disconnect(); }

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