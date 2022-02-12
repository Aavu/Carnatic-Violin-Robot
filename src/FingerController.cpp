#include "FingerController.h"
#include "Util.h"

FingerController* FingerController::pInstance = nullptr;

FingerController::FingerController(PortHandler& portHandler) : m_epos(this, &FingerController::encoderPositionCallback), m_pPortHandler(&portHandler),
m_fingerDxl(DxlID::Finger, *m_pPortHandler, *PortHandler::getPacketHandler()),
m_strDxl(DxlID::String, *m_pPortHandler, *PortHandler::getPacketHandler()), m_fingerDxlTimer(TIMER_CH2) {}

FingerController::~FingerController() {}

FingerController* FingerController::create(PortHandler& portHandler) {
    pInstance = new FingerController(portHandler);
    return pInstance;
}

void FingerController::destroy(FingerController* pInstance) {
    delete pInstance;
}

int FingerController::init(bool shouldHome) {
    int err = m_epos.init(FINGER_NODE_ID);
    if (err != 0) {
        LOG_ERROR("Init");
        return 1;
    }

    err = m_fingerDxl.operatingMode(CurrentBasedPositionControl);
    if (err != 0) {
        LOG_ERROR("Set Operation Mode");
        return err;
    }

    err = m_fingerDxl.setGoalCurrent(DXL_MAX_CURRENT);
    if (err != 0) {
        LOG_ERROR("Set Operation Mode");
        return err;
    }

    if (shouldHome) {
        err = setHome();
        if (err != 0) {
            LOG_ERROR("setHome");
            return 1;
        }
    }

    m_fingerDxlTimer.stop();
    m_fingerDxlTimer.setPeriod(FINGER_UPDATE_RATE * 1000);
    m_fingerDxlTimer.attachInterrupt(fingerUpdateCallback);

    err = enable(true, false);
    if (err != 0) {
        LOG_ERROR("enable dxls");
        return err;
    }

    m_bInitialized = true;
    return 0;
}

int FingerController::reset() {
    if (!m_bInitialized) return 0;

    // epos
    int err = m_epos.setEnable(false);
    _WORD stat;
    err = m_epos.readStatusWord(&stat);
    LOG_LOG("Status (finger): %h", stat);

    m_epos.setNMTState(NMTState::PreOperational);

    // dxl
    fingerOff();
    m_fingerDxlTimer.stop();

    err = enable(false, false);

    m_bInitialized = false;
    delay(1000);

    return 0;
}

bool FingerController::isInitialized() const {
    return m_bInitialized;
}

int FingerController::setHome() {
    int err = fingerRest();
    if (err != 0) {
        LOG_ERROR("rest");
        return 1;
    }

    err = m_epos.setOpMode(OpMode::Homing);
    if (err != 0) {
        LOG_ERROR("setOpMode");
        return 1;
    }

    err = m_epos.setEnable();
    if (err != 0) {
        LOG_ERROR("setEnable");
        return 1;
    }

    err = m_epos.startHoming();
    if (err != 0) {
        LOG_ERROR("startHoming");
        return 1;
    }

    int ii = 0;
    int32_t pos;
    bool isHoming = true;
    while (isHoming) {
        isHoming = (m_epos.getHomingStatus() == InProgress);
        delay(50);
        if (ii++ > 200) break;
    }

    delay(1000);

    err = m_epos.setEnable(false);
    if (err != 0) {
        LOG_ERROR("Disable");
        return 1;
    }

    return 0;
}

void FingerController::encoderPositionCallback(int32_t encPos) {
    // float& on = m_fingerPos.on;
    float& off = m_fingerPos.off;

    float m = constrain(encPos * 1.f / MAX_ENCODER_INC, 0, 1);

    // on = FINGER_ON - m * FINGER_RISE_LEVEL;
    off = FINGER_OFF - m * FINGER_RISE_LEVEL;
}

void FingerController::fingerUpdateCallback() {
    auto& fingerPos = pInstance->m_fingerPos;

    if (fingerPos.currentPosition == Off) {
        int err = pInstance->m_fingerDxl.moveToPosition(fingerPos.off, false);
        if (err != 0)
            LOG_ERROR("dxl MoveToPosition");
    }
}

// float FingerController::getFingerPosition(float pos) {
//     float end = pos - 10;
//     float m = m_epos.getEncoderPosition() * 1.f / MAX_ENCODER_INC;
//     return (m * end) + ((1 - m) * pos);
// }

int FingerController::fingerRest() {
    return fingerOff();
}

int FingerController::fingerOn() {
    int err;

    if (!m_fingerDxl.isEnabled()) {
        err = m_fingerDxl.torque();
        if (err != 0) {
            LOG_ERROR("dxl torque enable");
            return err;
        }
    }

    m_fingerPos.currentPosition = On;

    err = m_fingerDxl.moveToPosition(m_fingerPos.on);
    if (err != 0)
        LOG_ERROR("dxl MoveToPosition");

    return err;
}

int FingerController::fingerOff() {
    int err;

    if (!m_fingerDxl.isEnabled()) {
        err = m_fingerDxl.torque();
        if (err != 0) {
            LOG_ERROR("dxl torque enable");
            return err;
        }
    }

    m_fingerPos.currentPosition = Off;

    err = m_fingerDxl.moveToPosition(m_fingerPos.off);
    if (err != 0)
        LOG_ERROR("dxl MoveToPosition");

    return err;
}

int FingerController::setMode(OpMode mode) {
    int err = m_epos.setOpMode(mode);
    if (err != 0)
        LOG_ERROR("setOpMode");

    return err;
}

int FingerController::moveToPosition(float fFretPosition, bool bPDO) {
    int32_t iPos = Util::fret2Pos(fFretPosition);
    LOG_LOG("%i", iPos);

    return moveToPosition(iPos, bPDO);
}

int FingerController::moveToPosition(int32_t iPos, bool bPDO) {
    // LOG_LOG("%i", iPos);
    // return 0;
    if (bPDO)
        return m_epos.PDO_setPosition(iPos);

    return m_epos.moveToPosition(iPos);
}

int FingerController::setString(StringPos strPos) {
    return m_strDxl.moveToPosition((float) strPos);
}

int FingerController::enable(bool bEnableDxl, bool bEnableEpos) {
    int err;

    err = m_fingerDxl.torque(bEnableDxl);
    if (err != 0) {
        LOG_ERROR("finger dxl torque enable/disable");
        return err;
    }

    err = m_strDxl.torque(bEnableDxl);
    if (err != 0) {
        LOG_ERROR("str dxl torque enable/disable");
        return err;
    }

    return m_epos.setEnable(bEnableEpos);
}

int FingerController::prepareToPlay(float firstPitch) {
    int err = setString(StringPos::D);
    if (err != 0) {
        LOG_ERROR("setString");
        return err;
    }

    /*********** Move to the first pitch position with a profile **************/
    err = setMode(OpMode::ProfilePosition);
    if (err != 0) {
        LOG_ERROR("setMode");
        return err;
    }

    err = enable();
    if (err != 0) {
        LOG_ERROR("enable");
        return err;
    }

    // Serial.println(m_performParam.pitches[0]);
    // Since we cleaned up the data, the 0th index is guaranteed to be valid
    err = moveToPosition(firstPitch, false);
    if (err != 0) {
        LOG_ERROR("moveToPosition");
        return err;
    }

    err = fingerOn();
    if (err != 0) {
        LOG_ERROR("fingerOn");
        return err;
    }

    return 0;
}

int FingerController::getReadyToPlay() {
    int err = setMode(CyclicSyncPosition);
    if (err != 0) {
        LOG_ERROR("setMode");
        return err;
    }

    err = enable();
    if (err != 0) {
        LOG_ERROR("setEnable");
        return err;
    }

    _WORD stat;

    err = m_epos.readStatusWord(&stat);
    // LOG_LOG("Status: %h", stat);
    delay(100);

    m_fingerDxlTimer.start();
    return enablePDO();
}

int FingerController::enablePDO(bool bEnable) {
    auto state = bEnable ? NMTState::Operational : NMTState::PreOperational;
    int err = m_epos.setNMTState(state);
    if (err != 0)
        LOG_ERROR("setNMTState");

    return err;
}

int FingerController::PDO_processMsg(can_message_t& msg) {
    return m_epos.PDO_processMsg(msg);
}

int FingerController::setRxMsg(can_message_t& msg) {
    return m_epos.setRxMsg(msg);
}

void FingerController::handleEMCYMsg(can_message_t& msg) {
    m_epos.handleEMCYMsg(msg);
}
