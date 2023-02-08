//
// Created by Raghavasimhan Sankaranarayanan on 11/01/22.
//

#include "BowController.h"

BowController* BowController::pInstance = nullptr;

BowController::BowController(PortHandler& portHandler)
    : m_pPortHandler(&portHandler),
    m_epos(this, &BowController::encoderPositionCallback),
    m_leftDxl(DxlID::Left, *m_pPortHandler, *PortHandler::getPacketHandler(), DXL_LEFT_ZERO),
    m_rightDxl(DxlID::Right, *m_pPortHandler, *PortHandler::getPacketHandler(), DXL_RIGHT_ZERO),
    m_slideDxl(DxlID::Slide, *m_pPortHandler, *PortHandler::getPacketHandler()), 
    m_dxlTimer(TIMER_CH3) 
{}

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
    int err = m_epos.init(BOW_NODE_ID, BOW_ENCODER_DIRECTION);
    if (err != 0) {
        LOG_ERROR("Bow Controller Init");
        return err;
    }

    err = m_leftDxl.operatingMode(ExtendedPositionControl);
    if (err != 0) {
        LOG_ERROR("Left Dxl Set Operation Mode");
        return err;
    }

    err = m_rightDxl.operatingMode(ExtendedPositionControl);
    if (err != 0) {
        LOG_ERROR("Right Dxl Set Operation Mode");
        return err;
    }

    err = m_slideDxl.operatingMode(ExtendedPositionControl);
    if (err != 0) {
        LOG_ERROR("Slide Dxl Set Operation Mode");
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
    delete[] m_afBowTrajectory;
    m_afBowTrajectory = nullptr;

    delete[] m_aiStringId;
    m_aiStringId = nullptr;

    int err = moveBowToPosition(5, 0, false, true);
    if (err != 0) {
        LOG_ERROR("dxl move to position");
        return err;
    }

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

    err = m_epos.setHomingMethod(CurrentThresholdPositive);
    if (err != 0) {
        LOG_ERROR("setHomingMethod");
        return 1;
    }

    err = enable();
    if (err != 0) {
        LOG_ERROR("enable");
        return 1;
    }

    err = moveBowToPosition(5, 0, true, true);
    if (err != 0) {
        LOG_ERROR("dxl pair move to position");
        return err;
    }

    err = m_slideDxl.moveToPosition((int32_t) SLIDE_AVG);
    if (err != 0) {
        LOG_ERROR("slide dxl move to position");
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

int BowController::moveBowToPosition(float fPos, float fAngle, bool isRadian, bool shouldWait) {
    if (!isRadian) {
        fAngle *= ((float) M_PI) / 180.f;
    }

    if (abs(fAngle) > ((float)M_PI)/4.f) { 
        LOG_ERROR("Angle out of limit");
        return 1; 
    }

    m_fAngle = fAngle;

    float L = BOW_CENTER_EDGE_RAIL_DISTANCE;
    float d = BOW_GEAR_PITCH_DIA;
    float phi = (L/d) * tanf(m_fAngle);

    float eta = fPos / d;

    // Serial.print("Angles: ");
    // Serial.print(L/d);
    // Serial.print("\t");
    // Serial.print(m_fAngle * 180.f / M_PI);
    // Serial.print("\t");
    // Serial.print(phi * 180.f / M_PI);
    // Serial.print("\t");
    // Serial.println(eta * 180 / M_PI);
    int err = m_leftDxl.moveToPosition(phi - eta, false, true);
    if (err != 0) {
        LOG_ERROR("left dxl move failed...");
        return err;
    }

    return m_rightDxl.moveToPosition(phi + eta, shouldWait, true);  // Same sign for phi because of the nature of the design, rotations are inverted
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

    err = m_leftDxl.torque(bEnable);
    if (err != 0) {
        LOG_ERROR("left torque");
        return err;
    }
    err = m_rightDxl.torque(bEnable);
    if (err != 0) {
        LOG_ERROR("right torque");
        return err;
    }
    err = m_slideDxl.torque(bEnable);
    if (err != 0) {
        LOG_ERROR("slide torque");
        return err;
    }

    LOG_LOG("Enabled Bow Actuators");
    return 0;
}

int BowController::prepareToPlay(const int bowChanges[], int numChanges, int nPitches, int stringId[]) {
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

    err = enable();
    if (err != 0) {
        LOG_ERROR("enable");
        return err;
    }

    err = moveBowToPosition(5, 0, false, true);
    if (err != 0) {
        LOG_ERROR("dxl move to position");
        return err;
    }

    err = m_slideDxl.moveToPosition((int32_t) SLIDE_AVG, true);
    if (err != 0) {
        LOG_ERROR("slide dxl move to position");
        return err;
    }

    delete[] m_aiStringId;
    m_aiStringId = new int[nPitches];

    memcpy(m_aiStringId, stringId, nPitches * sizeof(int));

    computeBowTrajectory(bowChanges, numChanges, nPitches);
    m_dxlTimer.start();
    return 0;
}

int BowController::computeBowTrajectory(const int bowChanges[], int numChanges, int nPitches) {
    bool dir = true;    // start with down bow
    m_iNPitches = nPitches;
    static const int MAX = 1;
    static const int MIN = 0;
    float p0 = dir ? MIN : MAX;
    float pf = dir ? MAX : MIN;;
    const int kBowEncLen = MAX - MIN;

    delete[] m_afBowTrajectory;
    m_afBowTrajectory = new float[m_iNPitches];

    if (numChanges <= 0) {
        // Compute just a single trajectory
        Util::interpWithBlend(p0, pf, nPitches, 0.1, m_afBowTrajectory);
        return 0;
    }

    const int kNumSegments = numChanges + 1;
    std::pair<int, int> segments[kNumSegments];

    segments[0] = std::pair<int, int>(0, bowChanges[0]);
    for (int i = 0; i < numChanges - 1; ++i) {
        segments[i + 1] = std::pair<int, int>(bowChanges[i], bowChanges[i + 1]);
    }
    segments[kNumSegments - 1] = std::pair<int, int>(bowChanges[numChanges - 1], nPitches);

    for (int i = 0; i < kNumSegments; ++i) {
        int iSegLen = segments[i].second - segments[i].first;
        float iBowLen = MAX_BOW_VELOCITY * iSegLen * kBowEncLen;
        if (iBowLen < kBowEncLen) {  // This means full bowing would be faster than max velocity -> restrict length
            pf = p0 + (dir ? iBowLen : -iBowLen);
        } else { // -> Use full bow
            pf = dir ? MAX : MIN;
        }
        Serial.print("(");
        Serial.print(segments[i].first);
        Serial.print(", ");
        Serial.print(segments[i].second);
        Serial.print(")\t");
        Serial.print(iBowLen);
        Serial.print(" ");
        Serial.print(dir);
        Serial.print(" ");
        Serial.print(p0);
        Serial.print(" ");
        Serial.println(pf);
        Util::interpWithBlend(p0, pf, iSegLen, 0.05, &m_afBowTrajectory[segments[i].first]);
        dir ^= 1;
        p0 = pf;
    }

    return 0;
}

int BowController::changeDirection() {
    if (m_CurrentDirection == BowDirection::Down)
        m_CurrentDirection = BowDirection::Up;
    else if (m_CurrentDirection == BowDirection::Up)
        m_CurrentDirection = BowDirection::Down;
    LOG_LOG("Direction changed");
}

int BowController::enablePDO(bool bEnable) {
    auto state = bEnable ? NMTState::Operational : NMTState::PreOperational;
    int err = m_epos.setNMTState(state);
    if (err != 0)
        LOG_ERROR("setNMTState");
    LOG_LOG("Bow PDO Enable = %i", bEnable);
    return err;
}

int BowController::setNMTState(NMTState nmtState) {
    return m_epos.setNMTState(nmtState);
}

int BowController::setPosition(int32_t iPosition, bool bDirChange, bool bPDO) {
    if (bDirChange)
        changeDirection();

    if (bPDO)
        return m_epos.PDO_setPosition(iPosition);
    return m_epos.moveToPosition(iPosition);
}

int BowController::updatePosition(int i) {
    if (!m_afBowTrajectory || !m_aiStringId)
        return 1;
    
    int idx = (i < m_iNPitches - 1) ? i : m_iNPitches - 1;
    m_iCurrentTrajIdx = idx;

    float val = m_afBowTrajectory[idx];
    int32_t tmp = BOW_ENCODER_MIN + val * (BOW_ENCODER_MAX - BOW_ENCODER_MIN);
    // LOG_LOG("Position: %i", int(tmp));
    return setPosition(tmp);
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
    // float C = 2000 + BOW_ENCODER_MIN + (BOW_ENCODER_MAX - BOW_ENCODER_MIN) / 2;
    // encPos -= C;
    // float corr = encPos * 1.0 / std::max((BOW_ENCODER_MAX - C), (BOW_ENCODER_MIN - C));
    // corr = -pow(corr, 2) * BOW_LIFT_MULTIPLIER;
    // m_fPitch = PITCH_AVG + corr;
}

void BowController::dxlUpdateCallback() {
    static int lastString = -1;
    int idx = pInstance->m_iCurrentTrajIdx;
    // pInstance->m_pitchDxl.moveToPosition(pInstance->m_fPitch, false);
    if (lastString != pInstance->m_aiStringId[idx]) {
        if (pInstance->m_aiStringId[idx] == 0) {
            pInstance->moveBowToPosition(10, -40);
            lastString = pInstance->m_aiStringId[idx];
        } else if (pInstance->m_aiStringId[idx] == 1) {
            pInstance->moveBowToPosition(-2, -20);
            lastString = pInstance->m_aiStringId[idx];
        }
    }
}