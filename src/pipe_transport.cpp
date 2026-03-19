// src/pipe_transport.cpp
#include <cstring>
#include <vector>

#include "insight/pipe_transport.h"

namespace insight {

#if defined(_WIN32)

void UniquePipeHandle::Reset(HandleType handle) {
    if (handle_ != INVALID_HANDLE) {
        CloseHandle(handle_);
    }
    handle_ = handle;
}

TransportResult UniquePipeHandle::Send(PacketType type, const ByteBuffer& payload) {
    PacketHeader header;
    header.type         = type;
    header.payload_size = static_cast<uint32_t>(payload.size());

    ByteBuffer buffer;
    buffer.resize(sizeof(PacketHeader) + payload.size());
    std::memcpy(buffer.data(), &header, sizeof(PacketHeader));
    if (!payload.empty()) {
        std::memcpy(buffer.data() + sizeof(PacketHeader), payload.data(), payload.size());
    }

    DWORD bytes_written = 0;
    if (!WriteFile(handle_, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_written, nullptr)) {
        return TransportResult::GetSystemError();
    }
    return TransportResult::Ok();
}

TransportResult UniquePipeHandle::Receive(PacketType& out_type, ByteBuffer& out_data) {
    PacketHeader header;

    DWORD bytes_read = 0;
    if (!ReadFile(handle_, &header, sizeof(PacketHeader), &bytes_read, nullptr)) {
        return TransportResult::GetSystemError();
    }

    out_type = header.type;

    if (header.payload_size == 0) {
        out_data.clear();
        return TransportResult::Ok();
    }

    out_data.resize(header.payload_size);
    if (!ReadFile(handle_, out_data.data(), static_cast<DWORD>(header.payload_size), &bytes_read, nullptr)) {
        return TransportResult::GetSystemError();
    }
    return TransportResult::Ok();
}

TransportResult PipeClient::Send(PacketType type, const ByteBuffer& payload) {
    if (!IsConnected()) {
        return TransportResult::NotConnected();
    }

    auto result = handle_.Send(type, payload);
    if (!result && result.IsDisconnected()) {
        Disconnect();
    }

    return result;
}

TransportResult PipeClient::Receive(PacketType& out_type, ByteBuffer& out_data) {
    if (!IsConnected()) {
        return TransportResult::NotConnected();
    }

    auto result = handle_.Receive(out_type, out_data);
    if (!result && result.IsDisconnected()) {
        Disconnect();
    }

    return result;
}

TransportResult PipeClient::Connect() {
    handle_.Reset(CreateFileW(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr
    ));

    if (!IsConnected()) {
        return TransportResult::GetSystemError();
    }

    DWORD mode = PIPE_READMODE_BYTE;
    if (!SetNamedPipeHandleState(handle_.Get(), &mode, nullptr, nullptr)) {
        Disconnect();
        return TransportResult::GetSystemError();
    }

    return TransportResult::Ok();
}

TransportResult PipeServer::Send(PacketType type, const ByteBuffer& payload) {
    if (!IsConnected()) {
        return TransportResult::NotConnected();
    }

    auto result = handle_.Send(type, payload);
    if (!result && result.IsDisconnected()) {
        Disconnect();
    }

    return result;
}

TransportResult PipeServer::Receive(PacketType& out_type, ByteBuffer& out_data) {
    if (!IsConnected()) {
        return TransportResult::NotConnected();
    }

    auto result = handle_.Receive(out_type, out_data);
    if (!result && result.IsDisconnected()) {
        Disconnect();
    }

    return result;
}

TransportResult PipeServer::Listen() {
    handle_.Reset(CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1, 4096, 4096, 0, nullptr
    ));

    if (!IsConnected()) {
        return TransportResult::GetSystemError();
    }

    return TransportResult::Ok();
}

TransportResult PipeServer::Accept() {
    if (!IsConnected()) {
        return TransportResult::NotConnected();
    }

    if (!ConnectNamedPipe(handle_.Get(), nullptr)) {
        DWORD err = GetLastError();
        if (err != ERROR_PIPE_CONNECTED) {
            CleanUp();
            return TransportResult::GetSystemError();
        }
    }

    return TransportResult::Ok();
}

void PipeServer::CleanUp() {
    if (handle_.IsValid()) {
        DisconnectNamedPipe(handle_.Get());
        handle_.Reset();
    }
}

#else

// @todo Linux

#endif // _WIN32

} // namespace insight