//
// Created by Raghavasimhan Sankaranarayanan on 11/26/21.
// 

#include "epos4.h"

Epos4::Epos4() {

}

int Epos4::init(int iNodeID, MotorModel motorModel, int iEncoderResolution) {
    m_uiNodeID = iNodeID;
    m_iEncoderResolution = iEncoderResolution;
    m_currentNMTState = PreOperational;

    int err;
    err = setControlWord(0x87);
    if (err < 0) {
        LOG_ERROR("control word send");
        return err;
    }

    err = shutdown();

    _WORD stat = 0x0;
    err = readStatusWord(&stat);
    if (err < 0) {
        LOG_ERROR("status word err");
        return err;
    }
    LOG_LOG("Status (%i): %h", (int) m_uiNodeID, (int) stat);

    err = setNMTState(PreOperational);
    if (err < 0) {
        LOG_ERROR("Set NMT State Preoperational failed");
        return err;
    }


    switch (motorModel) {
    case EC45:
        err = configMotor(EC45_DATA);
        if (err < 0) {
            LOG_ERROR("EC45 Config Error");
            return err;
        }
        break;

    case EC32:
        err = configMotor(EC32_DATA);
        if (err < 0) {
            LOG_ERROR("EC32 Config Error");
            return err;
        }
        break;
    }


    err = setFollowErrorWindow(MAX_FOLLOW_ERROR);
    if (err < 0) {
        LOG_ERROR("set Follow Error Window Error");
        return err;
    }

    err = setAxisPolarity(MOTOR_POLARITY[m_uiNodeID - 1]);
    if (err < 0) {
        LOG_ERROR("set Axis Polarity Error");
        return err;
    }

    err = PDO_config();
    if (err < 0) {
        LOG_ERROR("PDO Config Error");
        return err;
    }

    err = setNMTState(m_currentNMTState);
    if (err != 0) {
        LOG_ERROR("setNMTState");
        return err;
    }
    return 0;
}

void Epos4::reset() {
    setNMTState(PreOperational);
    setEnable(false);
}

int Epos4::configMotor(const int* data) {
    int err;

    err = setNominalCurrent(data[0]);
    if (err < 0) {
        LOG_ERROR("setNominalCurrent");
        return err;
    }

    err = setOutputCurrentLimit(data[1]);
    if (err < 0) {
        LOG_ERROR("setOutputCurrentLimit");
        return err;
    }

    err = setMotorTorqueConstant(data[2]); // uNm/A
    if (err < 0) {
        LOG_ERROR("setMotorTorqueConstant");
        return err;
    }

    err = setThermalTimeConstantWinding(data[3]); // x 0.1 s
    if (err < 0) {
        LOG_ERROR("setThermalTimeConstantWinding");
        return err;
    }

    err = setNumPolePairs(data[4]);
    if (err < 0) {
        LOG_ERROR("setNumPolePairs");
        return err;
    }

    // Encoder
    err = setEncoderNumPulses(m_iEncoderResolution);
    if (err < 0) {
        LOG_ERROR("setEncoderNumPulses");
        return err;
    }
    // refer FW Spec 6.2.56.2 for type
    err = setEncoderType(data[5]);
    if (err < 0) {
        LOG_ERROR("setEncoderType");
        return err;
    }

    err = setCurrentControlParameters(data[6], data[7]);
    if (err < 0) {
        LOG_ERROR("setCurrentControlParameters");
        return err;
    }

    err = setPositionControlParameters(data[8], data[9], data[10], data[11], data[12]);
    if (err < 0) {
        LOG_ERROR("setPositionControlParameters");
        return err;
    }

    return 0;
}

int Epos4::home(bool bWait) {
    int err = setOpMode(OpMode::Homing);
    if (err != 0) {
        LOG_ERROR("setOpMode");
        return 1;
    }

    // LOG_LOG("Homing node %i with method %i", m_uiNodeID, HOMING_METHOD[m_uiNodeID - 1]);
    err = setHomingMethod((HomingMethod) HOMING_METHOD[m_uiNodeID - 1]);
    if (err != 0) {
        LOG_ERROR("setHomingMethod");
        return 1;
    }

    err = setEnable(true);
    if (err != 0) {
        LOG_ERROR("setEnable");
        return 1;
    }

    err = startHoming();
    if (err || !bWait)
        return err;

    int t = 0;
    bool isHoming = true;

    while (isHoming) {
        isHoming = getHomingStatus() == InProgress;
        delay(1);
        if (t++ > HOMING_TIMEOUT) {
            LOG_ERROR("Homing Timed out");
            return 1;
        }
    }
    return 0;
}

int Epos4::readObj(_WORD index, _BYTE subIndex, _DWORD* answer) {
    // block SDO
    m_bSDOBusy = true;

    can_message_t msg;
    // EPOS4 Communication Guide (2021-03) -> 3.4
    // EPOS4 Application Notes (2021-03) -> 5.4
    msg.id = COB_ID_SDO + m_uiNodeID;
    msg.format = CAN_STD_FORMAT;
    msg.length = 8;
    msg.data[0] = 0x40;
    msg.data[1] = index & 0xFF;
    msg.data[2] = (index & 0xFF00) >> 8;
    msg.data[3] = subIndex;
    msg.data[4] = 0x00;
    msg.data[5] = 0x00;
    msg.data[6] = 0x00;
    msg.data[7] = 0x00;

    if (CanBus.writeMessage(&msg) != msg.length) {
        LOG_ERROR("CanBus WriteMessage failed");
        m_bSDOBusy = false;
        return -1;
    }

    while (m_bSDOBusy) {
        // LOG_LOG("sdo busy");
        delay(1);
    }
    /* check for error code */
    if (m_rxMsg.data[0] == 0x80) {
        m_uiError = (m_rxMsg.data[7] << 24) + (m_rxMsg.data[6] << 16) + (m_rxMsg.data[5] << 8) + m_rxMsg.data[4];
        return -1;
    }

    *answer = (((int32_t) (m_rxMsg.data[7])) << 24) + (((int32_t) (m_rxMsg.data[6])) << 16) + (((int32_t) (m_rxMsg.data[5])) << 8) + (int32_t) (m_rxMsg.data[4]);

    return 0;
}

int Epos4::writeObj(_WORD index, _BYTE subIndex, _DWORD param) {
    // EPOS4 Communication Guide (2021-03) -> 3.4
    // EPOS4 Application Notes (2021-03) -> 5.4
    m_bSDOBusy = true;

    can_message_t msg;

    msg.id = COB_ID_SDO + m_uiNodeID;
    msg.format = CAN_STD_FORMAT;
    msg.length = 8;
    msg.data[0] = 0x22;
    msg.data[1] = index & 0xFF;
    msg.data[2] = (index & 0xFF00) >> 8;
    msg.data[3] = subIndex;
    msg.data[4] = param & 0xFF;
    msg.data[5] = (param & 0xFF00) >> 8;
    msg.data[6] = (param & 0xFF0000) >> 16;
    msg.data[7] = (param & 0xFF000000) >> 24;

    if (CanBus.writeMessage(&msg) != msg.length) {
        LOG_ERROR("CanBus WriteMessage failed");
        m_bSDOBusy = false;
        return -1;
    }

    while (m_bSDOBusy) {
        // LOG_LOG("sdo busy");
        delay(1);
    }

    /* check for error code */
    if (m_rxMsg.data[0] == 0x80) {
        m_uiError = (m_rxMsg.data[7] << 24) + (m_rxMsg.data[6] << 16) + (m_rxMsg.data[5] << 8) + (m_rxMsg.data[4]);
        LOG_ERROR("Device Error: %h", m_uiError);
        // LOG_ERROR("Device Error (sep): %h %h %h %h", m_rxMsg.data[7], m_rxMsg.data[6], m_rxMsg.data[5], m_rxMsg.data[4]);
        return -1;
    }

    return 0;
}

_WORD Epos4::firmwareVersion() {
    _DWORD answer;

    int n = 0;
    if ((n = readObj(0x1018, 0x03, &answer)) < 0) {
        LOG_ERROR("readObj in readSWversion. Error code: %h", m_uiError);
        return n;
    }

    n = answer >> 0x10;
    return n;
}

int Epos4::readStatusWord(_WORD* status) {
    _DWORD ret;

    if (readObj(0x6041, 0x00, &ret) < 0) {
        LOG_ERROR("Read status word. ");
        return -1;
    }

    *status = ret & 0xFFFF;
    return 0;
}

int Epos4::readITP(uint8_t* time, int8_t* unit) {
    _DWORD val;
    int n = readObj(INTERP_TIME_ADDR, 0x01, &val);
    if (n < 0) {
        LOG_ERROR("Read Obj failed. Error code: ", m_uiError);
        return -1;
    }

    *time = val;

    n = readObj(INTERP_TIME_ADDR, 0x02, &val);
    if (n < 0) {
        LOG_ERROR("Read Obj failed. Error code: ", m_uiError);
        return -1;
    }

    *unit = val;
}

int Epos4::setITP(uint8_t value, int8_t index) {
    _BYTE idx = index & 0xFF;
    int n = writeObj(INTERP_TIME_ADDR, 0x01, value);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }

    n = writeObj(INTERP_TIME_ADDR, 0x02, idx);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }

    value = 0;
    index = 0;
    readITP(&value, &index);
    LOG_LOG("ITP (%i) : %i, %i", m_uiNodeID, value, (int) idx);

    return 0;
}

int32_t Epos4::angle2Pos(float fAngle, bool bRadian) {
    if (!bRadian)
        fAngle = fAngle * (M_PI / 180.f);

    return round((fAngle / (2.f * M_PI)) * m_iEncoderResolution);
}

int Epos4::getActualPosition(int32_t* piPos) {
    _DWORD ret;

    if (readObj(0x6064, 0x00, &ret) < 0) {
        LOG_ERROR("Get positon failed. Error: %h", m_uiError);
        return -1;
    }

    LOG_TRACE("Position: %i", (int) ret);
    *piPos = ret;

    return 0;
}

HomingStatus Epos4::getHomingStatus() {
    if (m_iCurrentMode != Homing) {
        return Error;
    }

    _WORD stat;
    int err = readStatusWord(&stat);
    if (err != 0) {
        LOG_ERROR("readStatusWord");
        return Error;
    }
    // LOG_LOG("homing status: %h", stat);

    if (stat & (1 << 13)) {
        LOG_ERROR("Homing error");
        return Error;
    }

    if ((stat & (1 << 12)) && (stat & (1 << 15))) {
        return Completed;
    }


    return stat & (1 << 10) ? Interrupted : InProgress;
}

int Epos4::getOpMode(OpMode* opMode, char* sOpMode) {
    _DWORD ret;

    if (readObj(MODE_OF_OPERATION_DISP_ADDR, 0x00, &ret) < 0) {
        LOG_ERROR("Get op mode failed. Error: %h", m_uiError);
        return -1;
    }

    m_iCurrentMode = static_cast<OpMode>(ret & 0xFF);
    if (opMode) *opMode = m_iCurrentMode;

    if (sOpMode) {
        const char* m = getOpModeString(m_iCurrentMode);
        strcpy(sOpMode, m);
    }
    return 0;
}

char* Epos4::getOpModeString(OpMode mode) const {
    switch (mode) {
    case OpMode::ProfilePosition:
        return "Profile Position Mode";
        break;

    case OpMode::ProfileVelocity:
        return "Profile Velocity Mode";
        break;
    case OpMode::Homing:
        return "Homing Mode";
        break;
    case OpMode::CyclicSyncPosition:
        return "Cyclic Synchronous Position Mode";
        break;
    case OpMode::CyclicSyncVelocity:
        return "Cyclic Synchronous Velocity Mode";
        break;
    case OpMode::CyclicSyncTorque:
        return "Cyclic Synchronous Torque Mode";
        break;
    default:
        break;
    }
    return "";
}

int Epos4::setOpMode(OpMode opMode, uint8_t uiInterpolationTime, int8_t iInterpolationIndex, HomingMethod homingMethod) {
    int n;
    n = writeObj(MODE_OF_OPERATION_ADDR, 0x0, (uint8_t) opMode);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    m_iCurrentMode = opMode;

    switch (opMode) {
    case ProfilePosition:
        n = setProfile();
        if (n != 0) {
            LOG_ERROR("setProfile");
            return -1;
        }
        break;

    case ProfileVelocity:
        n = setProfile();
        if (n != 0) {
            LOG_ERROR("setProfile");
            return -1;
        }
        break;

    case Homing:
        n = setHomingMethod(homingMethod);
        if (n != 0) {
            LOG_ERROR("setHomingMethod");
            return -1;
        }

        n = setHomingCurrentThreshold(HOMING_THRESHOLD[m_uiNodeID - 1]);
        if (n != 0) {
            LOG_ERROR("setHomingCurrentThreshold");
            return -1;
        }
        break;

    case CyclicSyncPosition:
        n = setITP(uiInterpolationTime, iInterpolationIndex);
        if (n != 0) return -1;
        break;

    case CyclicSyncVelocity:
        break;

    case CyclicSyncTorque:
        break;
    }

    // Refer App notes -> 7.6
    // control word Shutdown (quickstop and enable voltage)
    int err = shutdown();

    _DWORD mooDisplay;
    n = readObj(0x6061, 0x0, &mooDisplay);
    if (n != 0) {
        LOG_ERROR("read Modes of operation display");
        return -1;
    }

    LOG_LOG("Mode display (%i): %h", m_uiNodeID, mooDisplay);

    if (mooDisplay != opMode) {
        LOG_WARN("Mode not set properly");
    }
    return err;
}

int Epos4::setNominalCurrent(_DWORD current) {
    if (current < 0 || current > 5000) {    // For EPOS4 50/5
        LOG_ERROR("Current Not in range");
        return -1;
    }

    int n;
    n = writeObj(MOTOR_DATA_ADDR, NOMINAL_CURRENT, current);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    return 0;
}

int Epos4::setOutputCurrentLimit(_DWORD currentLimit) {
    if (currentLimit < 0 || currentLimit > 15000) {    // For EPOS4 50/5
        LOG_ERROR("Current Not in range");
        return -1;
    }

    int n;
    n = writeObj(MOTOR_DATA_ADDR, OUTPUT_CURRENT_LIMIT, currentLimit);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    return 0;
}

int Epos4::setNumPolePairs(_BYTE polePairs) {
    if (polePairs < 1 || polePairs > 255) {    // For EPOS4 50/5
        LOG_ERROR("Current Not in range");
        return -1;
    }

    int n;
    n = writeObj(MOTOR_DATA_ADDR, POLE_PAIRS, polePairs);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    return 0;
}

int Epos4::setThermalTimeConstantWinding(_WORD ttconstWinding) {
    if (ttconstWinding < 1 || ttconstWinding > 10000) {
        LOG_ERROR("torque Constant Not in range. Refer FW spec 6.2.53.4");
        return -1;
    }
    int n;
    n = writeObj(MOTOR_DATA_ADDR, THERMAL_TIME_CONST_WINDING, ttconstWinding);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    return 0;

}

int Epos4::setMotorTorqueConstant(_DWORD torqueConst) {
    if (torqueConst < 0 || torqueConst > 10000000) {
        LOG_ERROR("torque Constant Not in range. Refer FW spec 6.2.53.5");
        return -1;
    }

    int n;
    n = writeObj(MOTOR_DATA_ADDR, TORQUE_CONST, torqueConst);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    return 0;
}

int Epos4::setAxisPolarity(bool value) {
    // refer: 6.2.49.4 Axis configuration miscellaneous

    return writeObj(0x3000, 0x04, (uint32_t) value);
}

int Epos4::setEncoderNumPulses(_DWORD pulses) {
    if (pulses < 16 || pulses > 2500000) {
        LOG_ERROR("Pulses Not in range: %i. Refer FW spec 6.2.56.1", (int) pulses);
        return -1;
    }

    return writeObj(0x3010, 0x01, pulses);
}

int Epos4::setEncoderType(_WORD type) {
    return writeObj(0x3010, 0x02, type);
}

int Epos4::setControlWord(_WORD cw) {
    int n = writeObj(CTRL_WORD_ADDR, 0x00, cw);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    m_uiCurrentCtrlWord = cw;
    delay(5);
    return 0;
}

int Epos4::shutdown() {
    return setControlWord(0x0006);
}

int Epos4::setEnable(bool bEnable) {
    // control word switch on and enable
    // Refer FW spec -> 6.2.94
    // bit 0 -> Switched On
    // bit 1 -> Enable Voltage
    // bit 2 -> Quick Stop
    // bit 3 -> Enable Operation
    _WORD cw = 0x000F;
    // bit 4 -> new setpoint (PPM) | Homing start (HMM)
    // bit 5 -> Change immediately (PPM)
    // bit 6 -> Abs (0) / rel (1) (PPM)
    // bit 7 -> Fault Reset
    // bit 8 -> Halt (Profiled modes)
    // bit 15 -> Endless Move (PPM)

    // if (m_iCurrentMode == ProfilePosition) cw = 0xBB;

    if (!bEnable) cw = 0x0004;

    return setControlWord(cw);
}

int Epos4::setProfile(_DWORD vel, _DWORD acc) {
    int err;
    // Profile Velocity
    err = writeObj(0x6081, 0x0, vel);
    if (err < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }

    // Profile Acc
    err = writeObj(0x6083, 0x0, acc);
    if (err < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }

    // Profile Deceleration
    err = writeObj(0x6084, 0x0, acc);
    if (err < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }

    // Profile Quick stop deceleration
    err = writeObj(0x6085, 0x0, 10000);
    if (err < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }

    // Shutdown
    err = setControlWord(0x0006);
    if (err < 0) {
        LOG_ERROR("Shutdown Failed");
        return -1;
    }

    // Switch on & Enable
    err = setControlWord(0x000F);
    if (err < 0) {
        LOG_ERROR("Switch on & Enable Failed");
        return -1;
    }
}

int Epos4::setHomingMethod(HomingMethod method) {
    return writeObj(0x6098, 0x00, (uint8_t) method);
}

int Epos4::setHomingCurrentThreshold(_WORD currentThreshold) {
    return writeObj(0x30B2, 0x0, currentThreshold);
}

int Epos4::moveToPosition(int32_t pos, bool bWait) {
    int err;
    // Target position
    err = writeObj(0x607A, 0x0, pos);
    if (err < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }

    // Abs Pos, start immediately
    err = setControlWord(0x003F);
    if (err < 0) {
        LOG_ERROR("Abs Pos, start immediately");
        return -1;
    }

    _WORD stat = 0x0;
    err = readStatusWord(&stat);
    if (err < 0) {
        LOG_ERROR("readStatusWord");
        return -1;
    }

    if (bWait) {
        // refer FW spec table 7-64 -> 
        while (!(stat & 0x400)) {
            err = readStatusWord(&stat);
            // LOG_LOG("wait stat: %h", stat);
            if (err < 0) {
                LOG_ERROR("readStatusWord");
                return -1;
            }

            if ((stat) & 0x8) {
                LOG_ERROR("Fault state");
                return -1;
            }
            err = setControlWord(0x000F);
            if (err < 0) {
                LOG_ERROR("Abs Pos, start immediately");
                return -1;
            }

        }
        delay(100);
    }

    return 0;

}

int Epos4::rotate(float fAngle, bool bRadian, bool bWait) {
    int32_t pos = angle2Pos(fAngle, bRadian);
    return moveToPosition(pos, bWait);
}

int Epos4::moveWithVelocity(int32_t velocity) {
    int err;
    // Target velocity
    err = writeObj(0x60FF, 0x0, velocity);
    if (err < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    delay(10);
    // Start Move
    err = setControlWord(0x000F);
    if (err < 0) {
        LOG_ERROR("Start Move failed");
        return -1;
    }
    return 0;
}

int Epos4::setFollowErrorWindow(_DWORD errWindow) {
    int err;
    err = writeObj(0x6065, 0x00, errWindow);
    if (err < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }

    return 0;
}

int Epos4::quickStop() {
    return setControlWord(0x000B);
}

int Epos4::startHoming() {
    // This delay is needed for homing to work
    delay(20);  // Can be as low as 10ms
    return setControlWord(0x001F);
}

int Epos4::SetHomePosition(int32_t iPos) {
    int n;
    n = writeObj(0x30B0, 0x00, iPos);
    if (n < 0) {
        LOG_ERROR("Write Obj failed. Error code: ", m_uiError);
        return -1;
    }
    return 0;
}

int Epos4::setCurrentControlParameters(uint32_t P, uint32_t I) {
    int n;
    n = writeObj(CURRENT_CTRL_PARAM_ADDR, CC_P_GAIN, P);
    if (n < 0) {
        LOG_ERROR("Write CC_P_GAIN failed. value: %i, Error code: %h", P, m_uiError);
        return -1;
    }

    n = writeObj(CURRENT_CTRL_PARAM_ADDR, CC_I_GAIN, I);
    if (n < 0) {
        LOG_ERROR("Write CC_I_GAIN failed. value: %i, Error code: %h", I, m_uiError);
        return -1;
    }

    return 0;
}

int Epos4::setPositionControlParameters(uint32_t P, uint32_t I, uint32_t D, uint32_t FF_V, uint32_t FF_A) {
    int n;
    n = writeObj(POS_CTRL_PARAM_ADDR, PC_P_GAIN, P);
    if (n < 0) {
        LOG_ERROR("Write PC_P_GAIN failed. value: %i, Error code: %h", P, m_uiError);
        return -1;
    }

    _DWORD ans;
    n = readObj(POS_CTRL_PARAM_ADDR, PC_P_GAIN, &ans);
    if (n < 0) {
        LOG_ERROR("Read Obj failed. Error code: %h", m_uiError);
        return -1;
    }

    n = writeObj(POS_CTRL_PARAM_ADDR, PC_I_GAIN, I);
    if (n < 0) {
        LOG_ERROR("Write PC_I_GAIN failed. value: %i, Error code: %h", I, m_uiError);
        return -1;
    }

    n = writeObj(POS_CTRL_PARAM_ADDR, PC_D_GAIN, D);
    if (n < 0) {
        LOG_ERROR("Write PC_D_GAIN failed. value: %i, Error code: %h", D, m_uiError);
        return -1;
    }

    n = writeObj(POS_CTRL_PARAM_ADDR, PC_FF_V_GAIN, FF_V);
    if (n < 0) {
        LOG_ERROR("Write PC_FF_V_GAIN failed. value: %i, Error code: %h", FF_V, m_uiError);
        return -1;
    }

    n = writeObj(POS_CTRL_PARAM_ADDR, PC_FF_A_GAIN, FF_A);
    if (n < 0) {
        LOG_ERROR("Write PC_FF_A_GAIN failed. value: %i, Error code: %h", FF_A, m_uiError);
        return -1;
    }

    return 0;
}

int Epos4::setNMTState(NMTState nmtState) {
    can_message_t msg;

    msg.id = 0x0;
    msg.length = 2;
    msg.format = CAN_STD_FORMAT;
    msg.data[0] = (uint8_t) nmtState;
    msg.data[1] = m_uiNodeID;

    int n = CanBus.writeMessage(&msg);
    if (n != msg.length) {
        LOG_ERROR("CanBus write msg failed. %i != %i", n, 2);
        m_bSDOBusy = false;
        return -1;
    }

    m_currentNMTState = nmtState;
    return 0;
}

int Epos4::PDO_config() {
    int err;

    /********** TPDO 3 stuffs **********/

    // Set COD ID for TPDO3
    err = writeObj(0x1802, 0x01, 0x40000000 + COB_ID_TPDO3 + m_uiNodeID);
    if (err < 0) {
        LOG_ERROR("Set COB ID Failed. Error code: %h", m_uiError);
    }

    // Set TPDO3 transmission type to Asynchronous
    err = writeObj(0x1802, 0x02, 255);
    if (err < 0) {
        LOG_ERROR("Set transmission type failed. Error code: %h", m_uiError);
    }

    // Set TPDO3 Inhibition time
    err = writeObj(0x1802, 0x03, 50);
    if (err < 0) {
        LOG_ERROR("Set transmission type failed. Error code: %h", m_uiError);
    }
    /********** RPDO 2 stuffs **********/
    // Default mapping is ctrlword and modes of operation. We need ctrlword and targetTorque

    if (m_uiNodeID == FINGER + 1) {
        // Set COB ID for RPDO2
        err = writeObj(0x1401, 0x01, COB_ID_RPDO2 + m_uiNodeID);
        if (err < 0) {
            LOG_ERROR("Set COB ID Failed. Error code: %h", m_uiError);
        }

        // Set RPDO2 transmission type to Asynchronous
        err = writeObj(0x1401, 0x02, 255);
        if (err < 0) {
            LOG_ERROR("Set transmission type failed. Error code: %h", m_uiError);
        }

        // Disable PDO
        err = writeObj(0x1601, 0x00, 0);
        if (err < 0) {
            LOG_ERROR("Disable PDO failed. Error code: %h", m_uiError);
        }

        uint32_t obj = (0x6071 << 16) | (0x00 << 8) | 0x10;

        // Modify desired objects in subindex 0x01...0x0n
        err = writeObj(0x1601, 0x02, obj);
        if (err < 0) {
            LOG_ERROR("modify object failed. Error code: %h", m_uiError);
        }

        // Write the desired number of mapped objects to subindex 0x00
        err = writeObj(0x1601, 0x00, 2);
        if (err < 0) {
            LOG_ERROR("set num objects failed. Error code: %h", m_uiError);
        }
    }

    /********** RPDO 3 stuffs **********/

    // Set COB ID for RPDO3
    err = writeObj(0x1402, 0x01, COB_ID_RPDO3 + m_uiNodeID);
    if (err < 0) {
        LOG_ERROR("Set COB ID Failed. Error code: %h", m_uiError);
    }

    // Set RPDO3 transmission type to Asynchronous
    err = writeObj(0x1402, 0x02, 255);
    if (err < 0) {
        LOG_ERROR("Set transmission type failed. Error code: %h", m_uiError);
    }

    /********* RPDO 4 stuffs ***********/

    // Set COB ID for RPDO4
    err = writeObj(0x1403, 0x01, COB_ID_RPDO4 + m_uiNodeID);
    if (err < 0) {
        LOG_ERROR("Set COB ID Failed. Error code: %h", m_uiError);
    }

    // Set RPDO4 transmission type to Asynchronous
    err = writeObj(0x1402, 0x02, 255);
    if (err < 0) {
        LOG_ERROR("Set transmission type failed. Error code: %h", m_uiError);
    }
    return 0;
}

int Epos4::PDO_setControlWord(_WORD cw) {
    can_message_t msg;
    msg.id = 0x200 + m_uiNodeID;
    msg.format = CAN_STD_FORMAT;
    msg.length = 2;
    msg.data[0] = cw & 0xFF;
    msg.data[1] = (cw >> 8) & 0xFF;
    int n = CanBus.writeMessage(&msg);

    if (n != msg.length) {
        LOG_ERROR("CanBus write msg failed.");
        m_bSDOBusy = false;
        return -1;
    }

    m_uiCurrentCtrlWord = cw;
    return 0;
}

int Epos4::PDO_processMsg(can_message_t& msg) {
    m_uiCurrentStatusWord = ((msg.data[1] & 0xFF) << 8) + msg.data[0];
    static int32_t callbackEncPos = 0;
    m_iEncoderPosition = ((msg.data[5] & 0xFF) << 24) + ((msg.data[4] & 0xFF) << 16) + ((msg.data[3] & 0xFF) << 8) + msg.data[2];
    m_uiError = ((msg.data[7] & 0xFF) << 8) + msg.data[6];

    m_bFault = (m_uiCurrentStatusWord & (1 << 3));

    if (m_bFault) {
        LOG_WARN("status (%i): %h - Fault detected", m_uiNodeID, m_uiCurrentStatusWord);
        // LOG_WARN("Error code: %h", m_uiError);
    }
    // LOG_LOG("Position: %i", m_iEncoderPosition);
    return 0;
}

int Epos4::PDO_setPosition(int32_t position) {
    // static int32_t lastPos = -1;
    // static ulong t = millis();
    // if (lastPos != position && m_uiNodeID == LEFT_HAND) {
    //     LOG_LOG("%i, %i", position, millis() - t);
    // }
    // lastPos = position;
    // t = millis();

    // LOG_LOG("%i: %i", m_uiNodeID, position);
    uint8_t LSB = 0x0F;
    if (m_bFault)
        LSB = 0x8F;

    can_message_t msg;

    msg.id = COB_ID_RPDO3 + m_uiNodeID;
    msg.format = CAN_STD_FORMAT;
    msg.length = 6;
    msg.data[0] = LSB;  // cw set to enable and switch on. Clear fault if present
    msg.data[1] = 0x0;
    msg.data[2] = (uint8_t) (position & 0xFF);
    msg.data[3] = (uint8_t) ((position >> 8) & 0xFF);
    msg.data[4] = (uint8_t) ((position >> 16) & 0xFF);
    msg.data[5] = (uint8_t) ((position >> 24) & 0xFF);

    if (CanBus.writeMessage(&msg) != msg.length) {
        LOG_ERROR("CanBus write msg failed.");
        m_bSDOBusy = false;
        return -1;
    }

    return 0;
}

int Epos4::PDO_rotate(float fAngle, bool bRadian) {
    int32_t pos = angle2Pos(fAngle, bRadian);
    return PDO_setPosition(pos);
}

int Epos4::PDO_setVelocity(int32_t iVelocity) {
    uint8_t LSB = 0x0F;
    if (m_bFault)
        LSB = 0x8F;

    can_message_t msg;

    msg.id = COB_ID_RPDO4 + m_uiNodeID;
    msg.format = CAN_STD_FORMAT;
    msg.length = 6;
    msg.data[0] = LSB;  // cw set to enable and switch on. Clear fault if present
    msg.data[1] = 0x0;
    msg.data[2] = (uint8_t) (iVelocity & 0xFF);
    msg.data[3] = (uint8_t) ((iVelocity >> 8) & 0xFF);
    msg.data[4] = (uint8_t) ((iVelocity >> 16) & 0xFF);
    msg.data[5] = (uint8_t) ((iVelocity >> 24) & 0xFF);
    int n = CanBus.writeMessage(&msg);

    if (n != msg.length) {
        LOG_ERROR("CanBus write msg failed.");
        m_bSDOBusy = false;
        return -1;
    }

    return 0;
}

int Epos4::PDO_setTorque(int16_t iTorquePerThousand) {
    // LOG_LOG("torque: %i", iTorquePerThousand);
    uint8_t LSB = 0x0F;
    if (m_bFault)
        LSB = 0x8F;

    can_message_t msg;

    msg.id = COB_ID_RPDO2 + m_uiNodeID;
    msg.format = CAN_STD_FORMAT;
    msg.length = 4;
    msg.data[0] = LSB;  // cw set to enable and switch on. Clear fault if present
    msg.data[1] = 0x0;
    msg.data[2] = (uint8_t) (iTorquePerThousand & 0xFF);
    msg.data[3] = (uint8_t) ((iTorquePerThousand >> 8) & 0xFF);
    int n = CanBus.writeMessage(&msg);

    if (n != msg.length) {
        LOG_ERROR("CanBus write msg failed.");
        m_bSDOBusy = false;
        return -1;
    }

    return 0;
}

int Epos4::setRxMsg(can_message_t& msg) {
    if (!m_bSDOBusy) {
        LOG_WARN("No one requested CAN Message. Not copying...");
        return -1;
    }
    memcpy(&m_rxMsg, &msg, sizeof(can_message_t));
    m_bSDOBusy = false;
    return 0;
}

void Epos4::handleEMCYMsg(can_message_t& msg) {
    m_uiDevError = (msg.data[1] << 8) + msg.data[0];
    if (m_uiDevError != 0)
        LOG_ERROR("Device Error (%i): %h", m_uiNodeID, m_uiDevError);
}
