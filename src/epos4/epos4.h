//
// Created by Raghavasimhan Sankaranarayanan on 11/26/21.
// 
// Maxon EPOS4 library for STM32F7 based Robotis OpenCR1.0 controller board.
// Based on libEPOS_STM32 Liyu Wang, SJTU repo for epos2
//
// Refer: EPOS4 Firmware Specification (2021-04)

#pragma once 

#include <Arduino.h>
#include "epos4_def.h"
#include <CAN.h>
#include "../logger.h"
#include "../def.h"

/*
    *  Reference: canTxRxNessage Example from OpenCR1.0 Arduino Library
    *
    *  typedef struct
    *  {
    *    uint32_t id      : Identifier of received message
    *    uint32_t length  : Length of received message data
    *    uint8_t  data[8] : Data of received message
    *    uint8_t  format  : Type of ID
    *  } can_message_t;
    *
    * BAUDRATE :
    *   CAN_BAUD_125K
    *   CAN_BAUD_250K
    *   CAN_BAUD_500K
    *   CAN_BAUD_1000K
    *
    * FORMAT :
    *   CAN_STD_FORMAT
    *   CAN_EXT_FORMAT
*/

class FingerController;
class BowController;

class Epos4 {
public:
    Epos4(FingerController* pFingerController = nullptr, void (FingerController::* pFingerCallback)(int32_t) = nullptr, BowController* pBowController = nullptr, void (BowController::* pBowCallback)(int32_t) = nullptr);
    Epos4(BowController* pBowController = nullptr, void (BowController::* pBowCallback)(int32_t) = nullptr);
    ~Epos4() = default;

    int init(int iNodeID = 1, bool inverted = false);
    void reset();

    int configEC45();

    // Checks
    _WORD firmwareVersion();

    int readObj(_WORD index, _BYTE subIndex, _DWORD* answer);
    int writeObj(_WORD index, _BYTE subIndex, _DWORD param);

    // Read
    int readStatusWord(_WORD* status);
    int readITP(uint8_t* time, int8_t* unit);
    // Refer: FW Spec 6.2.103
    int getActualPosition(int32_t* piPos);

    // Getter for the stored encoder value from PDO_msg
    int32_t getEncoderPosition() {
        return m_iEncoderPosition;
    }

    HomingStatus getHomingStatus();

    // Refer: FW Spec 6.2.101
    int getOpMode(OpMode* opMode = nullptr, char* sOpMode = nullptr);
    char* getOpModeString(OpMode mode) const;

    // Write
    int setOpMode(OpMode opMode, uint8_t uiInterpolationTime = PDO_RATE, int8_t iInterpolationIndex = -3, HomingMethod homingMethod = CurrentThresholdPositive);

    int setControlWord(_WORD cw);
    int shutdown();
    int setEnable(bool bEnable = true);
    int setProfile(_DWORD vel = 1000, _DWORD acc = 10000);
    int setHomingMethod(HomingMethod method);
    int setHomingCurrentThreshold(_WORD currentThreshold);
    int moveToPosition(int32_t pos, bool bWait = true);
    int moveWithVelocity(int32_t velocity);
    int setFollowErrorWindow(_DWORD errWindow);
    int quickStop();
    int startHoming();
    int SetHomePosition(int32_t iPos = 0);

    int setCurrentControlParameters();
    int setPositionControlParameters();

    // Motor Data
    int setNominalCurrent(_DWORD current);
    int setOutputCurrentLimit(_DWORD currentLimit);
    int setNumPolePairs(_BYTE polePairs);
    int setThermalTimeConstantWinding(_WORD ttconstWinding);
    int setMotorTorqueConstant(_DWORD torqueConst);

    // Encoder
    int setEncoderNumPulses(_DWORD pulses);
    int setEncoderType(_WORD type);

    // Refer App notes -> 5.5
    int setNMTState(NMTState nmtState);

    // All PDO functions. Will work only with NMTState -> Operational

    // Configure mapping for each PDO
    int PDO_config();
    // Set Control word through PDO. Uses RPDO 1 - object 1
    int PDO_setControlWord(_WORD cw);
    // Process incoming TPDO Messages from EPOS4 device
    int PDO_processMsg(can_message_t& msg);
    // Set target position through PDO. Uses RPDO 3 - object 2
    int PDO_setPosition(int32_t position);
    // Set target velocity through PDO. Uses RPDO 4 - object 2
    int PDO_setVelocity(int32_t iVelocity);

    int setRxMsg(can_message_t& msg);
    void handleEMCYMsg(can_message_t& msg);

private:
    uint8_t m_uiNodeID = 1;
    int m_iDirMultiplier = 1;
    _DWORD m_uiError;    ///< EPOS global error status
    _WORD m_uiDevError;     // Device error

    NMTState m_currentNMTState;
    OpMode m_iCurrentMode;
    _WORD m_uiCurrentCtrlWord;
    _WORD m_uiCurrentStatusWord;

    int32_t m_iEncoderPosition;

    can_message_t m_txMsg;
    can_message_t m_rxMsg;

    volatile bool m_bSDOBusy = true;
    bool m_bCANRxReady = false;
    bool m_bCANTxReady = false;
    bool m_bIsPDO = false;

    volatile bool m_bFault = false;

    FingerController* m_pFingerController = nullptr;
    void (FingerController::* m_pFingerCallback)(int32_t) = nullptr;

    BowController* m_pBowController = nullptr;
    void (BowController::* m_pBowCallback)(int32_t) = nullptr;

    // write
    // Interpolation time period
    int setITP(uint8_t value = 1, int8_t index = -3);
};