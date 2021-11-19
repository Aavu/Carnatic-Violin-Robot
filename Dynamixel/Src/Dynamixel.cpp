//
// Created by violinsimma on 11/7/21.
//

#include "Dynamixel.h"

Dynamixel::Dynamixel(int id,
                     PortHandler& portHandler,
                     dynamixel::PacketHandler &packetHandler) :
        m_id(id),
        m_pPacketHandler(&packetHandler),
        m_pPortHandler(portHandler.getdxlPortHandler()),
        m_bIsEnabled(false) {}

Dynamixel::~Dynamixel() {
    if (m_bIsEnabled) {
        torque(false);
    }
}

int Dynamixel::operatingMode(OperatingMode mode) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    dxl_comm_result = m_pPacketHandler->write1ByteTxRx(m_pPortHandler, m_id, ADDR_OPERATING_MODE, mode, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        printf("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0) {
        printf("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
    }
//    else {
//        printf("Succeeded changing mode.\n");
//    }
    return dxl_error;
}

int Dynamixel::torque(bool bEnable) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;

    dxl_comm_result = m_pPacketHandler->write1ByteTxRx(m_pPortHandler, m_id, ADDR_TORQUE_ENABLE, (uint8_t)bEnable, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        printf("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0) {
        printf("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
    }
    else {
        if (bEnable) {
//            printf("Succeeded enabling DYNAMIXEL Torque.\n");
            m_bIsEnabled = true;
        } else {
//            printf("Succeeded disabling DYNAMIXEL Torque.\n");
            m_bIsEnabled = false;
        }
    }
    return dxl_error;
}

int Dynamixel::moveToPosition(int32_t position_pulses) {
    std::cout << (int) position_pulses << std::endl;
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    dxl_comm_result = m_pPacketHandler->write4ByteTxRx(m_pPortHandler, m_id, ADDR_GOAL_POSITION, position_pulses, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        printf("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0) {
        printf("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
    }
    return dxl_error;
}

int Dynamixel::getCurrentPosition(int32_t& dxl_present_position) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    dxl_comm_result = m_pPacketHandler->read4ByteTxRx(m_pPortHandler, m_id, ADDR_PRESENT_POSITION, (uint32_t*)&dxl_present_position, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        printf("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0) {
        printf("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
    }

    return dxl_error;
}

int Dynamixel::setGoalCurrent(int16_t iCurrent) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    dxl_comm_result = m_pPacketHandler->write2ByteTxRx(m_pPortHandler, m_id, ADDR_GOAL_CURRENT, iCurrent, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        printf("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    }
    else if (dxl_error != 0) {
        printf("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
    }
//    else {
//        printf("Succeeded setting goal current.\n");
//    }

    return dxl_error;
}

int Dynamixel::moveToPosition(float angle, bool isRadian) {
    if(isRadian)
        angle = angle*180.f/(float)M_PI;

    auto pos = (int32_t)(angle/DEG_PULSE);
    return moveToPosition(pos);
}

int Dynamixel::getCurrentPosition(float &current_angle, bool isRadian) {
    int32_t pos;
    auto dxl_error = getCurrentPosition(pos);
    if(dxl_error != 0)
        return dxl_error;

    current_angle = (float)pos*DEG_PULSE;

    if (isRadian)
        current_angle = current_angle * (float)M_PI / 180.f;
    return 0;
}

bool Dynamixel::isMoving() {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    uint8_t moving;
    dxl_comm_result = m_pPacketHandler->read1ByteTxRx(m_pPortHandler, m_id, ADDR_MOVING, &moving, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        printf("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
        return false;
    }
    else if (dxl_error != 0) {
        printf("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
        return false;
    }

    return (bool)moving;
}
