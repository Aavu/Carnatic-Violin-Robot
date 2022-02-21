#pragma once

#include "../def.h"
#include "math.h"
#include "string"
#include "iostream"
#include <DynamixelSDK.h>
#include "../logger.h"

#ifndef FAIL
#define FAIL 1
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#define DXL_DEVICE_NAME "OpenCR_DXL_Port" // Dummy obj to maintain API function standard. No actual meaning
#define DXL_BAUDRATE 57600

#define ADDR_TORQUE_ENABLE 64
#define ADDR_LED 65
#define ADDR_GOAL_POSITION 116
#define ADDR_GOAL_CURRENT 102
#define ADDR_PRESENT_POSITION 132
#define ADDR_OPERATING_MODE 11
#define ADDR_MOVING 122
#define ADDR_PRESENT_POSITION 132
#define ADDR_PROFILE_VELOCITY 112

#define MINIMUM_POSITION_LIMIT 0    // Refer to the Minimum Position Limit of product eManual
#define MAXIMUM_POSITION_LIMIT 4095 // Refer to the Maximum Position Limit of product eManual
#define UNIT_CURRENT 2.69           // mA
#define DEG_PULSE (360.f / 4096.f)  //  deg/pulse

#define PROTOCOL_VERSION 2.0

#define DXL_MOVING_STATUS_THRESHOLD 25 // DYNAMIXEL moving status threshold

enum OperatingMode {
    CurrentControl = 0,
    VelocityControl = 1,
    PositionControl = 3,
    ExtendedPositionControl = 4,
    CurrentBasedPositionControl = 5,
    PWMControl = 16
};

class PortHandler {
public:
    explicit PortHandler(const std::string& s_deviceName) {
        m_pPortHandler = getPortHandler(s_deviceName);
    }

    ~PortHandler() {
        closePort();
    }

    int openPort() {
        if (m_pPortHandler->openPort()) {
            return 0;
        }

        return 1;
    }

    int setBaudRate(int iBaudrate = -1) {
        if (iBaudrate < 0)
            iBaudrate = DXL_BAUDRATE;

        if (m_pPortHandler->setBaudRate(iBaudrate))
            return 0;

        return 1;
    }

    void closePort() {
        m_pPortHandler->closePort();
    }

    [[nodiscard]] dynamixel::PortHandler* getdxlPortHandler() const {
        return m_pPortHandler;
    }

    static dynamixel::PortHandler* getPortHandler(const std::string& s_deviceName) {
        return dynamixel::PortHandler::getPortHandler(s_deviceName.c_str());
    }

    static dynamixel::PacketHandler* getPacketHandler() {
        return dynamixel::PacketHandler::getPacketHandler();
    }

private:
    dynamixel::PortHandler* m_pPortHandler;
};

class Dynamixel {
public:
    Dynamixel(DxlID id, PortHandler& portHandler, dynamixel::PacketHandler& packetHandler);
    ~Dynamixel();

    [[maybe_unused]] int operatingMode(OperatingMode mode);

    [[maybe_unused]] int setGoalCurrent(int16_t iCurrent);
    int setProfileVelocity(uint16_t uiVelocity);

    [[maybe_unused]] int torque(bool bEnable = true);

    int moveToPosition(int32_t position_pulses, bool bWait = true, long timeout_ms = -1);
    int getCurrentPosition(int32_t& dxl_present_position);

    [[maybe_unused]] int moveToPosition(float angle, bool bWait = true, bool isRadian = false, long timeout_ms = -1);
    [[maybe_unused]] int getCurrentPosition(float& current_angle, bool isRadian = false);

    bool isMoving();
    bool isEnabled() {
        return m_bIsEnabled;
    }

    int waitForTarget(int32_t iPresentPosition, int iThreshold = DXL_MOVING_STATUS_THRESHOLD, long timeout_ms = -1);

    int LED(bool bOn);

private:
    int m_id;
    dynamixel::PacketHandler* m_pPacketHandler;
    dynamixel::PortHandler* m_pPortHandler;
    bool m_bIsEnabled;
};