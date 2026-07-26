#include <Arduino.h>
#include <ESP32Servo.h>

//////////////////////////////////////////////////////////////
// Protocol
//////////////////////////////////////////////////////////////

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
    uint8_t header;

    PacketType packet_type;

    int8_t steering;

    int8_t throttle;

    RequestedMode requested_mode;

    uint8_t checksum;
};

#pragma pack(pop)

//////////////////////////////////////////////////////////////
// Servo
//////////////////////////////////////////////////////////////

constexpr uint8_t STEERING_SERVO_PIN = 6;

Servo steeringServo;

constexpr uint8_t ESC_PIN = 5;

Servo esc;


//////////////////////////////////////////////////////////////
// Checksum
//////////////////////////////////////////////////////////////

uint8_t calculateChecksum(const AICommandPacket &packet)
{
    return packet.header ^
           static_cast<uint8_t>(packet.packet_type) ^
           static_cast<uint8_t>(packet.steering) ^
           static_cast<uint8_t>(packet.throttle) ^
           static_cast<uint8_t>(packet.requested_mode);
}

//////////////////////////////////////////////////////////////
// Steering Conversion
//////////////////////////////////////////////////////////////

int steeringToServoAngle(int8_t steering)
{
    steering = constrain(steering, -100, 100);

    return map(
        steering,
        -100,
        100,
        0,
        180);
}

int throttleToPWM(int8_t throttle)
{
    throttle = constrain(
        throttle,
        0,
        100);

    return map(
        throttle,
        0,
        100,
        1500,
        2000);
}

//////////////////////////////////////////////////////////////
// Apply AI Command
//////////////////////////////////////////////////////////////

void applyAICommand(const AICommandPacket& packet)
{
    steeringServo.write(
        steeringToServoAngle(packet.steering));

    esc.writeMicroseconds(
        throttleToPWM(packet.throttle));
}

//////////////////////////////////////////////////////////////
// Setup
//////////////////////////////////////////////////////////////

void setup()
{
    Serial.begin(115200);

    steeringServo.setPeriodHertz(50);
    esc.setPeriodHertz(50);

    steeringServo.attach(
        STEERING_SERVO_PIN,
        500,
        2500);

    esc.attach(
        ESC_PIN,
        1000,
        2000);

    // Start at neutral
    esc.writeMicroseconds(1500);

    delay(3000);        
}

//////////////////////////////////////////////////////////////
// Loop
//////////////////////////////////////////////////////////////

AICommandPacket packet;

unsigned long lastPacketTime = 0;

constexpr unsigned long PACKET_TIMEOUT = 500;

bool failsafeActive = false;

void loop()
{
    if (Serial.available() >= sizeof(packet))
    {
        Serial.readBytes(
            reinterpret_cast<char*>(&packet),
            sizeof(packet));

        if (packet.header == PROTOCOL_HEADER &&
            calculateChecksum(packet) == packet.checksum)
        {
            lastPacketTime = millis();

            failsafeActive = false;

            applyAICommand(packet);
        }
    }

    // Always execute the failsafe
    if (!failsafeActive &&
        millis() - lastPacketTime > PACKET_TIMEOUT)
    {
        esc.writeMicroseconds(1500);
        steeringServo.write(90);

        failsafeActive = true;
    }
}