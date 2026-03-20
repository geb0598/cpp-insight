#pragma once

#include <atomic>
#include <deque>
#include <future>
#include <memory>
#include <thread>

#include "insight/pipe_transport.h"
#include "insight/scope_profiler.h"
#include "insight/stat_registry.h"

namespace insight {

class InsightServer {
public:
    static InsightServer& GetInstance() {
        static InsightServer instance;
        return instance;
    }

    InsightServer(const InsightServer&)            = delete;
    InsightServer& operator=(const InsightServer&) = delete;
    InsightServer(InsightServer&&)                 = delete;
    InsightServer& operator=(InsightServer&&)      = delete;

    TransportResult              Listen();
    std::future<TransportResult> Accept();
    void                         Disconnect();

    void StartSession();
    void StopSession();
    bool IsSessionActive() const { return is_session_active_; }

    size_t GetFrameCount()   const { return frames_.size(); }

    const std::deque<FrameRecord>& GetFrames() const { return frames_; }

    void Clear() { frames_.clear(); }

private:
    InsightServer() = default;
    ~InsightServer() { Disconnect(); }

    void RecvWorker();

    void OnHandshake(const ByteBuffer& data);
    void OnFrame(const ByteBuffer& data);

    std::vector<std::unique_ptr<StatGroup>>      owned_groups_;
    std::vector<std::unique_ptr<StatDescriptor>> owned_descs_;

    PipeServer              transport_;
    std::thread             recv_thread_;
    std::atomic<bool>       is_running_        = false;
    std::atomic<bool>       is_session_active_ = false;
    std::deque<FrameRecord> frames_;
};

} // namespace insight