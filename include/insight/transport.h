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
    HANDSHAKE     = 0,
    FRAME         = 1,
    SESSION_START = 2,
    SESSION_STOP  = 3,
};

// -------------------------------------------------
// PacketHeader
// -------------------------------------------------
#pragma pack(push, 1)
struct PacketHeader {
    PacketType type;
    uint32_t   payload_size;
};
#pragma pack(pop)

// -------------------------------------------------
// IClientTransport
// -------------------------------------------------
class IClientTransport {
public:
    virtual ~IClientTransport() = default;

    virtual TransportResult Connect()           = 0;
    virtual void            Disconnect()        = 0;
    virtual bool            IsConnected() const = 0;

    virtual TransportResult Send(PacketType type, const ByteBuffer& data) = 0;
    virtual TransportResult Receive(PacketType& out_type, ByteBuffer& out_data)   = 0;
};

// -------------------------------------------------
// IServerTransport
// -------------------------------------------------
class IServerTransport {
public:
    virtual ~IServerTransport() = default;

    virtual TransportResult Listen()            = 0;
    virtual TransportResult Accept()            = 0;
    virtual void            Disconnect()        = 0;
    virtual bool            IsConnected() const = 0;

    virtual TransportResult Send(PacketType type, const ByteBuffer& data) = 0;
    virtual TransportResult Receive(PacketType& out_type, ByteBuffer& out_data)   = 0;
};

} // namespace insight