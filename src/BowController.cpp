#include "BowController.h"

BowController* BowController::pInstance = nullptr;

BowController::BowController(PortHandler& portHandler)
    : m_pPortHandler(&portHandler),
    m_epos(this, &BowController::encoderPositionCallback),
    m_pitchDxl(DxlID::Pitch, *m_pPortHandler, *PortHandler::getPacketHandler()),
    m_rollDxl(DxlID::Roll, *m_pPortHandler, *PortHandler::getPacketHandler()),
    m_yawDxl(DxlID::Yaw, *m_pPortHandler, *PortHandler::getPacketHandler()), m_dxlTimer(TIMER_CH3) {}

BowController::~BowController() {
    reset();
}

BowController* BowController::create(PortHandler& portHandler) {
    pInstance = new BowController(portHandler);
    return pInstance;
}

void BowController::destroy(BowController* pInstance) {
    delete pInstance;
}

int BowController::init(bool shouldHome) {
    int err = m_epos.init(BOW_NODE_ID);
    if (err != 0) {
        LOG_ERROR("Bow Controller Init");
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

    if (shouldHome) {
        err = setHome();
        if (err != 0) {
            LOG_ERROR("setHome");
            return 1;
        }
    }

    err = m_epos.setOpMode(OpMode::ProfilePosition);
    if (err != 0) {
        LOG_ERROR("bow setOpMode");
        return err;
    }

    m_dxlTimer.stop();
    m_dxlTimer.setPeriod(DXL_UPDATE_RATE * 1000);
    m_dxlTimer.attachInterrupt(dxlUpdateCallback);

    m_bInitialized = true;
    LOG_LOG("Bow Controller Initialized");
    return 0;
}

int BowController::reset() {
    if (!m_bInitialized) return 0;

    enablePDO(false);
    enable(false);

    m_dxlTimer.stop();
    m_bInitialized = false;
    delay(100);
    return 0;
}

int BowController::setHome() {
    int err;

    err = m_epos.setOpMode(OpMode::Homing);
    if (err != 0) {
        LOG_ERROR("setOpMode");
        return 1;
    }

    err = m_epos.setHomingMethod(CurrentThresholdNegative);
    if (err != 0) {
        LOG_ERROR("setHomingMethod");
        return 1;
    }

    err = enable();
    if (err != 0) {
        LOG_ERROR("enable");
        return 1;
    }

    err = m_pitchDxl.moveToPosition((float) PITCH_MAX, false);
    if (err != 0) {
        LOG_ERROR("pitch dxl move to position");
        return err;
    }

    err = m_yawDxl.moveToPosition((int32_t) YawPos::Mid, false);
    if (err != 0) {
        LOG_ERROR("yaw dxl move to position");
        return err;
    }

    err = m_rollDxl.moveToPosition((float) ROLL_AVG, false);
    if (err != 0) {
        LOG_ERROR("roll dxl move to position");
        return err;
    }

    delay(100);
    err = m_epos.startHoming();
    if (err != 0) {
        LOG_ERROR("startHoming");
        return 1;
    }

    int ii = 0;
    bool isHoming = true;
    while (isHoming) {
        isHoming = (m_epos.getHomingStatus() == InProgress);
        delay(50);
        if (ii++ > 200) break;
    }

    int32_t pos = -1;
    m_epos.getActualPosition(&pos);
    LOG_LOG("Actual Position: %i", pos);

    err = m_epos.setEnable(false);
    if (err != 0) {
        LOG_ERROR("Disable");
        return 1;
    }

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

    LOG_LOG("Enabled Bow Actuators");
    return 0;
}

int BowController::prepareToPlay() {
    int err;

    err = m_epos.setOpMode(ProfilePosition);
    if (err != 0) {
        LOG_ERROR("setOpMode");
        return err;
    }

    err = m_epos.setEnable();

    err = m_epos.moveToPosition(BOW_ENCODER_MIN);
    if (err != 0) {
        LOG_ERROR("moveToPosition");
        return err;
    }

    err = m_epos.setEnable(false);

    err = m_epos.setOpMode(CyclicSyncPosition);
    if (err != 0) {
        LOG_ERROR("setOpMode");
        return err;
    }

    err = enablePDO();
    if (err != 0) {
        LOG_ERROR("enablePDO");
        return err;
    }

    // err = m_epos.setProfile(200, 10000);
    // if (err != 0) {
    //     LOG_ERROR("setProfile");
    //     return err;
    // }

    err = enable();
    if (err != 0) {
        LOG_ERROR("enable");
        return err;
    }

    err = m_pitchDxl.setProfileVelocity(PROFILE_VELOCITY_PITCH);
    if (err != 0) {
        LOG_ERROR("pitch dxl setProfileVelocity");
        return err;
    }

    // Dummy test -- Change to PITCH_AVG later
    err = m_pitchDxl.moveToPosition((float) PITCH_AVG, false);
    if (err != 0) {
        LOG_ERROR("pitch dxl move to position");
        return err;
    }

    err = m_yawDxl.moveToPosition((int32_t) YawPos::D, false);
    if (err != 0) {
        LOG_ERROR("yaw dxl move to position");
        return err;
    }

    err = m_rollDxl.moveToPosition((float) ROLL_AVG, false);
    if (err != 0) {
        LOG_ERROR("roll dxl move to position");
        return err;
    }

    // err = m_epos.moveToPosition(BOW_ENCODER_MAX);
    // if (err != 0) {
    //     LOG_ERROR("moveToPosition");
    //     return err;
    // }
    // // delay(1000);
    // err = m_epos.moveToPosition(BOW_ENCODER_MIN);
    // if (err != 0) {
    //     LOG_ERROR("moveToPosition");
    //     return err;
    // }
    m_dxlTimer.start();
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
    LOG_LOG("Direction changed");
}

int BowController::enablePDO(bool bEnable) {
    auto state = bEnable ? NMTState::Operational : NMTState::Stopped;
    int err = m_epos.setNMTState(state);
    if (err != 0)
        LOG_ERROR("setNMTState");
    LOG_LOG("Bow PDO Enable = %i", bEnable);
    return err;
}

int BowController::setPosition(int32_t iPosition, bool bDirChange, bool bPDO) {
    if (bDirChange)
        changeDirection();

    if (bPDO)
        return m_epos.PDO_setPosition(iPosition);

    return m_epos.moveToPosition(iPosition);
}

int BowController::setVelocity(int32_t iVelocity, bool bPDO) {
    static int32_t prevVel = 0;

    if (abs(prevVel - iVelocity) > 5) {
        m_pitchDxl.moveToPosition(m_fPitch, false);
        prevVel - iVelocity;
    }

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

void BowController::encoderPositionCallback(int32_t encPos) {
    m_iCurrentPosition = encPos;
    // LOG_LOG("Position: %i", encPos);
    float C = 2000 + BOW_ENCODER_MIN + (BOW_ENCODER_MAX - BOW_ENCODER_MIN) / 2;
    encPos -= C;
    float corr = encPos * 1.0 / std::max((BOW_ENCODER_MAX - C), (BOW_ENCODER_MIN - C));
    corr = -pow(corr, 2) * BOW_LIFT_MULTIPLIER;
    m_fPitch = PITCH_AVG + corr;
    // Serial.print(m_fPitch);
    // Serial.print("\t");
    // Serial.println(corr);
}

void BowController::dxlUpdateCallback() {
    pInstance->m_pitchDxl.moveToPosition(pInstance->m_fPitch, false);
}
