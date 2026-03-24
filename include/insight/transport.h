#pragma once

#include <cstdint>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace insight {

// -------------------------------------------------
// ITransport
// -------------------------------------------------

using ByteBuffer = std::vector<std::byte>;

struct [[nodiscard]] TransportResult {
    std::error_code error;

    static TransportResult Ok()                  { return {}; }
    static TransportResult Err(std::error_code ec) { return { ec }; }
    static TransportResult NotConnected() {
#if defined(_WIN32)
        return Err(std::error_code(ERROR_PIPE_NOT_CONNECTED, std::system_category()));
#else
        return {};
#endif
    }

    static TransportResult GetSystemError() {
#if defined(_WIN32)
    return Err(std::error_code(static_cast<int>(GetLastError()), std::system_category()));
#else
    // @todo linux
    // return Err(std::error_code(errno, std::generic_category()));
    return {};
#endif
    }

    explicit operator bool() const { return !error; }

    bool IsDisconnected() const {
#if defined(_WIN32)
    return error.value() == ERROR_BROKEN_PIPE ||
           error.value() == ERROR_NO_DATA;
#else
    // @todo linux
    // return error.value() == EPIPE ||
    //        error.value() == ECONNRESET;
    return true;
#endif
    }
};

// -------------------------------------------------
// PacketType
// -------------------------------------------------
enum class PacketType : uint8_t {
    NONE          = 0,
    HANDSHAKE     = 1,
    FRAME         = 2,
    SESSION_START = 3,
    SESSION_STOP  = 4,
};

// -------------------------------------------------
// PacketHeader
// -------------------------------------------------
#pragma pack(push, 1)
struct PacketHeader {
    PacketType type         = PacketType::NONE;
    uint32_t   payload_size = 0;
};
#pragma pack(pop)

// -------------------------------------------------
// IClientTransport
// -------------------------------------------------
class IClientTransport {
public:
    virtual ~IClientTransport() = default;

    virtual TransportResult Connect(const wchar_t* pipe_name, DWORD access) = 0;
    virtual void            Disconnect()                                    = 0;
    virtual bool            IsConnected() const                             = 0;

    virtual TransportResult Send(PacketType type, const ByteBuffer& payload)           = 0;
    virtual TransportResult Receive(PacketHeader& out_header, ByteBuffer& out_payload) = 0;
};

// -------------------------------------------------
// IServerTransport
// -------------------------------------------------
class IServerTransport {
public:
    virtual ~IServerTransport() = default;

    virtual TransportResult Listen(const wchar_t* pipe_name, DWORD access) = 0;
    virtual TransportResult Accept()                                       = 0;
    virtual void            Disconnect()                                   = 0;
    virtual bool            IsConnected() const                            = 0;

    virtual TransportResult Send(PacketType type, const ByteBuffer& payload)           = 0;
    virtual TransportResult Receive(PacketHeader& out_header, ByteBuffer& out_payload) = 0;
};

} // namespace insight