#include <cstdint>

namespace insight {

enum class PacketType : uint8_t {
    Handshake = 0,
    Frame     = 1,
};



} // namespace insight