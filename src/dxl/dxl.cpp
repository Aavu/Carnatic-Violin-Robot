#include "dxl.h"

Dynamixel::Dynamixel(DxlID id,
    PortHandler& portHandler,
    dynamixel::PacketHandler& packetHandler) : m_id((int) id),
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
        LOG_ERROR("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    } else if (dxl_error != 0) {
        LOG_ERROR("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
    }

    return dxl_error;
}

int Dynamixel::torque(bool bEnable) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;

    dxl_comm_result = m_pPacketHandler->write1ByteTxRx(m_pPortHandler, m_id, ADDR_TORQUE_ENABLE, (uint8_t) bEnable, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        LOG_ERROR("%s", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    } else if (dxl_error != 0) {
        LOG_ERROR("%s", m_pPacketHandler->getRxPacketError(dxl_error));
    } else {
        m_bIsEnabled = bEnable;
    }
    return dxl_error;
}

int Dynamixel::moveToPosition(int32_t position_pulses, bool bWait, long timeout_ms) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    dxl_comm_result = m_pPacketHandler->write4ByteTxRx(m_pPortHandler, m_id, ADDR_GOAL_POSITION, position_pulses, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        LOG_ERROR("%s", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    } else if (dxl_error != 0) {
        LOG_ERROR("%s", m_pPacketHandler->getRxPacketError(dxl_error));
    }

    if (bWait)
        waitForTarget(position_pulses, DXL_MOVING_STATUS_THRESHOLD, timeout_ms);
    return dxl_error;
}

int Dynamixel::getCurrentPosition(int32_t& dxl_present_position) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    dxl_comm_result = m_pPacketHandler->read4ByteTxRx(m_pPortHandler, m_id, ADDR_PRESENT_POSITION, (uint32_t*) &dxl_present_position, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        LOG_ERROR("%s", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    } else if (dxl_error != 0) {
        LOG_ERROR("%s", m_pPacketHandler->getRxPacketError(dxl_error));
    }

    return dxl_error;
}

int Dynamixel::setGoalCurrent(int16_t iCurrent) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    dxl_comm_result = m_pPacketHandler->write2ByteTxRx(m_pPortHandler, m_id, ADDR_GOAL_CURRENT, iCurrent, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        LOG_ERROR("%s", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    } else if (dxl_error != 0) {
        LOG_ERROR("%s", m_pPacketHandler->getRxPacketError(dxl_error));
    }

    return dxl_error;
}

int Dynamixel::moveToPosition(float angle, bool bWait, bool isRadian, long timeout_ms) {
    if (isRadian)
        angle = angle * 180.f / (float) M_PI;

    auto pos = (int32_t) (angle / DEG_PULSE);
    // Serial.println(pos);
    return moveToPosition(pos, bWait, timeout_ms);
}

int Dynamixel::getCurrentPosition(float& current_angle, bool isRadian) {
    int32_t pos;
    auto dxl_error = getCurrentPosition(pos);
    if (dxl_error != 0)
        return dxl_error;

    current_angle = (float) pos * DEG_PULSE;

    if (isRadian)
        current_angle = current_angle * (float) M_PI / 180.f;
    return 0;
}

bool Dynamixel::isMoving() {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    uint8_t moving;
    dxl_comm_result = m_pPacketHandler->read1ByteTxRx(m_pPortHandler, m_id, ADDR_MOVING, &moving, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        LOG_ERROR("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
        return false;
    } else if (dxl_error != 0) {
        LOG_ERROR("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
        return false;
    }

    return (bool) moving;
}

int Dynamixel::LED(bool bOn) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    dxl_comm_result = m_pPacketHandler->write1ByteTxRx(m_pPortHandler, m_id, ADDR_LED, (uint8_t) bOn, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS) {
        LOG_ERROR("%s\n", m_pPacketHandler->getTxRxResult(dxl_comm_result));
    } else if (dxl_error != 0) {
        LOG_ERROR("%s\n", m_pPacketHandler->getRxPacketError(dxl_error));
    }

    return dxl_error;
}

int Dynamixel::waitForTarget(int32_t iGoalPosition, int iThreshold, long timeout_ms) {
    uint8_t dxl_error = 0;
    int dxl_comm_result = COMM_TX_FAIL;
    int32_t iPresentPosition;

    auto timeStart = millis();
    do {
        if (getCurrentPosition(iPresentPosition) != SUCCESS) {
            return -1;
        }

        if (timeout_ms >= 0)
            if (millis() - timeStart > timeout_ms)
                break;

    } while ((abs(iGoalPosition - iPresentPosition) > iThreshold));
    return SUCCESS;
}