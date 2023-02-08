//
// Created by Raghavasimhan Sankaranarayanan on 12/05/21.
// 

#pragma once

#include "logger.h"
#include "epos4/epos4.h"
#include "dxl/dxl.h"
#include "def.h"


enum fingerPos {
    Off = 0,
    On = 1
};

struct fingerPos_t {
    float on = FINGER_ON;
    float off = FINGER_OFF;
    fingerPos currentPosition = Off;
};

class FingerController {
public:
    // Not thread safe. It is ok here since this is guaranteed to be called only once - inside setup()
    static FingerController* create(PortHandler& portHandler);
    static void destroy(FingerController* pInstance);

    // Delete copy and assignment constructor
    FingerController(FingerController&) = delete;
    void operator=(const FingerController&) = delete;

    int init(bool shouldHome = true);
    int reset();
    bool isInitialized() const;

    int setHome();

    /**
     * @brief Convert the defined ON and OFF finger positions to respect the current fret position
     *
     * @return Error code
     */
    void encoderPositionCallback(int32_t encPos);

    static void fingerUpdateCallback();

    int fingerRest();
    int fingerOn();
    int fingerOff();

    int setMode(OpMode mode);

    int moveToPosition(float fFretPosition, bool bPDO = true);
    int moveToPosition(int32_t iPos, bool bPDO = true);

    int setString(StringPos strPos = StringPos::D);

    int enable(bool bEnableDxl = true, bool bEnableEpos = true);
    int prepareToPlay(float firstPitch);
    int getReadyToPlay();
    int enablePDO(bool bEnable = true);

    int PDO_processMsg(can_message_t& msg);
    int setRxMsg(can_message_t& msg);
    void handleEMCYMsg(can_message_t& msg);

private:
    FingerController(PortHandler& portHandler);
    ~FingerController();

    static FingerController* pInstance;

    Epos4 m_epos;
    PortHandler* m_pPortHandler;
    Dynamixel m_fingerDxl, m_strDxl;

    bool m_bInitialized = false;
    fingerPos_t m_fingerPos;

    HardwareTimer m_fingerDxlTimer;
};
