//
// Created by Raghavasimhan Sankaranarayanan on 01/20/22.
// 

#pragma once

#include "def.h"
#include "logger.h"
#include "epos4/epos4.h"
#include "dxl/dxl.h"

class BowController {
public:
    enum BowDirection {
        None = 0,
        Down = 1,
        Up = -1
    };

    enum BowState {
        Stopped = 0,
        Bowing = 1
    };

    BowController(PortHandler& portHandler);
    ~BowController();

    int init();
    int reset();

    int enable(bool bEnable = true);
    int prepareToPlay();

    int startBowing(float amplitude = 0.5, BowDirection direction = None);
    int stopBowing();

    int changeDirection();

    int enablePDO(bool bEnable = true);
    int setVelocity(int32_t iVelocity, bool bPDO = true);

    int setRxMsg(can_message_t& msg);
    int PDO_processMsg(can_message_t& msg);
    void handleEMCYMsg(can_message_t& msg);

private:
    bool m_bInitialized = false;
    PortHandler* m_pPortHandler;
    BowState m_bowingState;
    BowDirection m_CurrentDirection = Down;

    Epos4 m_epos;

    Dynamixel m_pitchDxl, m_rollDxl, m_yawDxl;
};
