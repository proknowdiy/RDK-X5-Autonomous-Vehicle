#pragma once

#include <cstdint>

constexpr uint8_t PROTOCOL_HEADER = 0xAA;

enum class PacketType : uint8_t
{
    AICommand = 0x01,

    VehicleStatus = 0x02
};

enum class RequestedMode : uint8_t
{
    Manual = 0,

    Assisted,

    Autonomous,

    EmergencyStop,

    MissionComplete
};

#pragma pack(push, 1)

struct AICommandPacket
{
    uint8_t header = PROTOCOL_HEADER;

    PacketType packet_type = PacketType::AICommand;

    int8_t steering = 0;

    int8_t throttle = 0;

    RequestedMode requested_mode =
        RequestedMode::Autonomous;

    uint8_t checksum = 0;
};

#pragma pack(pop)

inline uint8_t calculateChecksum(
    const AICommandPacket& packet)
{
    return packet.header ^
           static_cast<uint8_t>(packet.packet_type) ^
           static_cast<uint8_t>(packet.steering) ^
           static_cast<uint8_t>(packet.throttle) ^
           static_cast<uint8_t>(packet.requested_mode);
}

