#include "insight/archive.h"
#include "insight/reporter.h"
#include "insight/server.h"

namespace insight {

void Server::Listen() {
    auto data_result = data_pipe_.Listen(DATA_PIPE_NAME, PIPE_ACCESS_INBOUND);
    if (!data_result) {
        return;
    }

    auto control_result = control_pipe_.Listen(CONTROL_PIPE_NAME, PIPE_ACCESS_OUTBOUND);
    if (!control_result) {
        data_pipe_.Disconnect();
        return;
    }

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    accept_thread_ = std::thread(&Server::AcceptWorker, this);
}

void Server::Stop() {
    is_session_active_ = false;
    data_pipe_.Disconnect();
    control_pipe_.Disconnect();
    StopWorkers();
    NotifyDisconnected();

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
}

void Server::StartSession() {
    if (!IsConnected()) {
        return;
    }
    Reporter::GetInstance().Clear();
    is_session_active_ = true;
    EnqueuePacket(PacketType::SESSION_START, {});
}

void Server::StopSession() {
    if (!IsConnected()) {
        return;
    }
    is_session_active_ = false;
    EnqueuePacket(PacketType::SESSION_STOP, {});
}

TransportResult Server::Send(PacketType type, const ByteBuffer& payload) {
    return control_pipe_.Send(type, payload);
}

TransportResult Server::Receive(PacketHeader& out_header, ByteBuffer& out_payload) {
    return data_pipe_.Receive(out_header, out_payload);
}

void Server::OnPacketReceived(const PacketHeader& header, const ByteBuffer& payload) {
    switch (header.type) {
    case PacketType::HANDSHAKE:
        OnHandshake(payload);
        break;
    case PacketType::FRAME:
        OnFrame(payload);
        break;
    default:
        break;
    }
}

void Server::OnDisconnected() {
    is_session_active_ = false;
    data_pipe_.Disconnect();
    control_pipe_.Disconnect();
    NotifyDisconnected();
}

void Server::AcceptWorker() {
    std::thread data_accept_thread([this]() {
        auto result = data_pipe_.Accept();
        if (!result) {
            data_pipe_.Disconnect();
        }
    });

    std::thread control_accept_thread([this]() {
        auto result = control_pipe_.Accept();
        if (!result) {
            control_pipe_.Disconnect();
        }
    });

    data_accept_thread.join();
    control_accept_thread.join();

    if (!data_pipe_.IsConnected() || !control_pipe_.IsConnected()) {
        data_pipe_.Disconnect();
        control_pipe_.Disconnect();
        return;
    }

    NotifyConnected();
    StartWorkers();
}

void Server::OnHandshake(const ByteBuffer& data) {
    BinaryReader reader(data);

    Registry::GetInstance().Clear();
    owned_groups_.clear();
    owned_descs_.clear();

    int32_t group_count;
    reader << group_count;
    for (int32_t i = 0; i < group_count; ++i) {
        auto group = std::make_unique<Group>();
        reader << *group;
        Registry::GetInstance().RegisterGroup(group.get());
        owned_groups_.push_back(std::move(group));
    }

    int32_t desc_count;
    reader << desc_count;
    for (int32_t i = 0; i < desc_count; ++i) {
        auto desc = std::make_unique<Descriptor>();
        reader << *desc;
        Registry::GetInstance().RegisterDescriptor(desc.get());
        owned_descs_.push_back(std::move(desc));
    }
}

void Server::OnFrame(const ByteBuffer& data) {
    if (!is_session_active_) {
        return;
    }

    BinaryReader reader(data);
    FrameRecord  frame;
    reader << frame;

    Reporter::GetInstance().Submit(std::move(frame));
}

} // namespace insight