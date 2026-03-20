#include "insight/insight_server.h"
#include "insight/archive.h"

namespace insight {

TransportResult InsightServer::Listen() {
    return transport_.Listen();
}

std::future<TransportResult> InsightServer::Accept() {
    return std::async(std::launch::async, [this]() {
        return transport_.Accept();
    });
}

void InsightServer::Disconnect() {
    if (is_session_active_) {
        StopSession();
    }
    is_running_ = false;
    transport_.Disconnect();
}

void InsightServer::StartSession() {
    frames_.clear();
    is_running_        = true;
    is_session_active_ = true;
    recv_thread_       = std::thread(&InsightServer::RecvWorker, this);
    transport_.Send(PacketType::SESSION_START, {});
}

void InsightServer::StopSession() {
    is_session_active_ = false;
    is_running_        = false;
    transport_.Send(PacketType::SESSION_STOP, {});

    if (recv_thread_.joinable()) {
        recv_thread_.join();
    }
}

void InsightServer::RecvWorker() {
    while (is_running_) {
        PacketType type;
        ByteBuffer data;

        auto result = transport_.Receive(type, data);
        if (!result) {
            if (result.IsDisconnected()) {
                is_running_ = false;
            }
            return;
        }

        switch (type) {
        case PacketType::HANDSHAKE:
            OnHandshake(data);
            break;
        case PacketType::FRAME:
            OnFrame(data);
            break;
        default:
            break;
        }
    }
}

void InsightServer::OnHandshake(const ByteBuffer& data) {
    BinaryReader reader(data);

    StatRegistry::GetInstance().Clear();
    owned_groups_.clear();
    owned_descs_.clear();

    int32_t group_count;
    reader << group_count;
    for (int32_t i = 0; i < group_count; ++i) {
        auto group = std::make_unique<StatGroup>();
        reader << *group;
        StatRegistry::GetInstance().RegisterGroup(group.get());
        owned_groups_.push_back(std::move(group));
    }

    int32_t desc_count;
    reader << desc_count;
    for (int32_t i = 0; i < desc_count; ++i) {
        auto desc = std::make_unique<StatDescriptor>();
        reader << *desc;
        StatRegistry::GetInstance().RegisterDescriptor(desc.get());
        owned_descs_.push_back(std::move(desc));
    }
}

void InsightServer::OnFrame(const ByteBuffer& data) {
    if (!is_session_active_) {
        return;
    }

    BinaryReader reader(data);
    FrameRecord  frame;
    reader << frame;

    frames_.push_back(std::move(frame));
}

} // namespace insight