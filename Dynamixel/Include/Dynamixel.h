//
// Created by violinsimma on 11/7/21.
//

#ifndef HATHAANI_DYNAMIXEL_H
#define HATHAANI_DYNAMIXEL_H

#include "cmath"
#include "string"
#include "iostream"
#include <dynamixel_sdk.h>

#define ADDR_TORQUE_ENABLE          64
#define ADDR_GOAL_POSITION          116
#define ADDR_GOAL_CURRENT           102
#define ADDR_PRESENT_POSITION       132
#define ADDR_OPERATING_MODE         11

#define MINIMUM_POSITION_LIMIT      0  // Refer to the Minimum Position Limit of product eManual
#define MAXIMUM_POSITION_LIMIT      4095  // Refer to the Maximum Position Limit of product eManual
#define UNIT_CURRENT                2.69    // mA
#define DEG_PULSE                   (360.f/4096.f)  //  deg/pulse

#define BAUDRATE                    57600

#define PROTOCOL_VERSION  2.0

#define DXL_MOVING_STATUS_THRESHOLD     20  // DYNAMIXEL moving status threshold

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
        if(m_pPortHandler->openPort()) {
            return 0;
        }

        return 1;
    }

    int setBaudRate(int iBaudrate = -1) {
        if(iBaudrate < 0)
            iBaudrate = BAUDRATE;

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
    Dynamixel(int id, PortHandler& portHandler, dynamixel::PacketHandler& packetHandler);
    ~Dynamixel();

    [[maybe_unused]] int operatingMode(OperatingMode mode);

    [[maybe_unused]] int setGoalCurrent(int16_t iCurrent);

    [[maybe_unused]] int torque(bool bEnable=true);

    int moveToPosition(int32_t position_pulses);
    int getCurrentPosition(int32_t& dxl_present_position);

    [[maybe_unused]] int moveToPosition(float angle, bool isRadian = false);
    [[maybe_unused]] int getCurrentPosition(float& current_angle, bool isRadian = false);

private:
    int m_id;
    dynamixel::PacketHandler* m_pPacketHandler;
    dynamixel::PortHandler* m_pPortHandler;
    bool m_bIsEnabled;
};

#endif //HATHAANI_DYNAMIXEL_H
