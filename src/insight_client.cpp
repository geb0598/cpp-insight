#include "insight/insight_client.h"
#include "insight/archive.h"
#include "insight/stat_aggregator.h"

namespace insight {

TransportResult InsightClient::Connect() {
    auto result = transport_.Connect();
    if (!result) {
        return result;
    }

    result = SendHandshake();
    if (!result) {
        return result;
    }

    is_running_ = true;
    send_thread_ = std::thread(&InsightClient::SendWorker, this);
    recv_thread_ = std::thread(&InsightClient::RecvWorker, this);

    return TransportResult::Ok();
}

void InsightClient::Disconnect() {
    is_running_ = false;
    cv_.notify_all();

    if (send_thread_.joinable()) send_thread_.join();
    if (recv_thread_.joinable()) recv_thread_.join();

    transport_.Disconnect();
}

void InsightClient::SendFrame(FrameRecord frame) {
    if (!is_running_ || !is_session_active_) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(frame));
    }
    cv_.notify_one();
}

TransportResult InsightClient::SendHandshake() {
    BinaryWriter writer;

    auto& groups = StatRegistry::GetInstance().GetGroups();
    auto& descs  = StatRegistry::GetInstance().GetDescriptors();

    int32_t group_count = static_cast<int32_t>(groups.size());
    writer << group_count;
    for (auto& [id, group] : groups) { 
        writer << *group; 
    }

    int32_t desc_count = static_cast<int32_t>(descs.size());
    writer << desc_count;
    for (auto& [id, desc] : descs) { 
        writer << *desc; 
    }

    return transport_.Send(PacketType::HANDSHAKE, writer.GetBuffer());
}

void InsightClient::OnSessionStart() { is_session_active_ = true; }

void InsightClient::OnSessionStop()  { is_session_active_ = false; }

void InsightClient::SendWorker() {
    while (is_running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return !queue_.empty() || !is_running_ || !is_session_active_;
        });

        if (!is_running_ && queue_.empty()) {
            return;
        }

        FrameRecord frame = std::move(queue_.front());
        queue_.pop();
        lock.unlock();

        BinaryWriter writer;
        writer << frame;

        auto result = transport_.Send(PacketType::FRAME, writer.GetBuffer());
        if (!result && result.IsDisconnected()) {
            is_running_ = false;
            return;
        }
    }
}

void InsightClient::RecvWorker() {
    while (is_running_) {
        PacketType type;
        ByteBuffer data;

        auto result = transport_.Receive(type, data);
        if (!result) {
            if (result.IsDisconnected()) {
                is_running_        = false;
                is_session_active_ = false;
                cv_.notify_all();
            }
            return;
        }

        switch (type) {
        case PacketType::SESSION_START:
            OnSessionStart();
            break;
        case PacketType::SESSION_STOP:
            OnSessionStop();
            break;
        default:
            break;
        }
    }
}

} // namespace insight