#include "BowController.h"

BowController::BowController(PortHandler& portHandler)
    : m_pPortHandler(&portHandler),
    m_pitchDxl(DxlID::Pitch, *m_pPortHandler, *PortHandler::getPacketHandler()),
    m_rollDxl(DxlID::Roll, *m_pPortHandler, *PortHandler::getPacketHandler()),
    m_yawDxl(DxlID::Yaw, *m_pPortHandler, *PortHandler::getPacketHandler()) {}

BowController::~BowController() {
    reset();
}

int BowController::init() {
    int err = m_epos.init(BOW_NODE_ID);
    if (err != 0) {
        LOG_ERROR("Bow Controller Init");
        return err;
    }

    err = m_epos.setOpMode(OpMode::ProfileVelocity);
    if (err != 0) {
        LOG_ERROR("bow setOpMode");
        return err;
    }

    err = m_pitchDxl.operatingMode(PositionControl);
    if (err != 0) {
        LOG_ERROR("Bow pitch Set Operation Mode");
        return err;
    }

    err = m_rollDxl.operatingMode(PositionControl);
    if (err != 0) {
        LOG_ERROR("Bow roll Set Operation Mode");
        return err;
    }

    err = m_yawDxl.operatingMode(PositionControl);
    if (err != 0) {
        LOG_ERROR("Bow yew Set Operation Mode");
        return err;
    }

    m_bInitialized = true;
    LOG_LOG("Bow Controller Initialized");
    return 0;
}

int BowController::reset() {
    if (!m_bInitialized) return 0;

    enablePDO(false);
    enable(false);

    m_bInitialized = false;
    delay(100);
    return 0;
}

int BowController::enable(bool bEnable) {
    int err = m_epos.setEnable(bEnable);
    if (err != 0) {
        LOG_ERROR("setEnable");
        return err;
    }

    _WORD stat;
    err = m_epos.readStatusWord(&stat);
    if (err != 0)
        LOG_ERROR("Reset (bow): readStatusWord");
    LOG_LOG("Status (%i): %h", BOW_NODE_ID, stat);

    err = m_pitchDxl.torque(bEnable);
    if (err != 0) {
        LOG_ERROR("pitch torque");
        return err;
    }
    err = m_rollDxl.torque(bEnable);
    if (err != 0) {
        LOG_ERROR("roll torque");
        return err;
    }
    err = m_yawDxl.torque(bEnable);
    if (err != 0) {
        LOG_ERROR("yaw torque");
        return err;
    }
    return 0;
}

int BowController::prepareToPlay() {
    int err = enablePDO();
    if (err != 0) {
        LOG_ERROR("enablePDO");
        return err;
    }
    err = enable();
    if (err != 0) {
        LOG_ERROR("enable");
        return err;
    }

    err = m_pitchDxl.moveToPosition((float) PITCH_MAX);
    if (err != 0) {
        LOG_ERROR("pitch dxl move to position");
        return err;
    }
    err = m_yawDxl.moveToPosition((float) YawPos::D);
    if (err != 0) {
        LOG_ERROR("yaw dxl move to position");
        return err;
    }

    err = m_rollDxl.moveToPosition(120.f);
    if (err != 0) {
        LOG_ERROR("roll dxl move to position");
        return err;
    }
    return 0;
}

int BowController::startBowing(float amplitude, BowDirection direction) {
    if (direction != None)
        m_CurrentDirection = direction;
    return m_epos.moveWithVelocity(100 * m_CurrentDirection);
}

int BowController::stopBowing() {
    return m_epos.quickStop();
}

int BowController::changeDirection() {
    m_CurrentDirection = Down ? Up : Down;
}

int BowController::enablePDO(bool bEnable) {
    auto state = bEnable ? NMTState::Operational : NMTState::PreOperational;
    int err = m_epos.setNMTState(state);
    if (err != 0)
        LOG_ERROR("setNMTState");

    return err;
}

int BowController::setVelocity(int32_t iVelocity, bool bPDO) {
    if (bPDO)
        return m_epos.PDO_setVelocity(iVelocity);
    return m_epos.moveWithVelocity(iVelocity);
}

int BowController::setRxMsg(can_message_t& msg) {
    return m_epos.setRxMsg(msg);
}

int BowController::PDO_processMsg(can_message_t& msg) {
    return m_epos.PDO_processMsg(msg);
}

void BowController::handleEMCYMsg(can_message_t& msg) {
    m_epos.handleEMCYMsg(msg);
}
