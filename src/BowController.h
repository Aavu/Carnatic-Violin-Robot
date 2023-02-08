//
// Created by Raghavasimhan Sankaranarayanan on 11/01/22.
//

#ifndef BOWCONTROLLER_H
#define BOWCONTROLLER_H

#include "dxl/dxl.h"
#include "epos4/epos4.h"
#include "util.h"
#include "def.h"

class BowController {
public:
    enum class BowDirection {
        None = -1,
        Up = 0,
        Down = 1
    };

    enum class BowState {
        Stopped = 0,
        Bowing = 1
    };

    // Not thread safe. It is ok here since this is guaranteed to be called only once - inside setup()
    static BowController* create(PortHandler& portHandler);
    static void destroy(BowController* pInstance);

    // Delete copy and assignment constructor
    BowController(FingerController&) = delete;
    void operator=(const BowController&) = delete;

    int init(bool shouldHome = true);
    int reset();

    int setHome();

    /* position is wrt the top most point in Z Axis (away from the strings) the bow can go.
     * Angle is given in radians
     */
    int moveBowToPosition(float fPos, float fAngle, bool isRadian = false, bool shouldWait = false);

    int enable(bool bEnable = true);
    int prepareToPlay(const int bowChanges[], int numChanges, int nPitches, int stringId[]);

    int changeDirection();

    int enablePDO(bool bEnable = true);
    int setNMTState(NMTState nmtState);
    int setPosition(int32_t iPosition, bool bDirChange = false, bool bPDO = true);
    int updatePosition(int i);
    int setVelocity(int32_t iVelocity, bool bPDO = true);

    int setRxMsg(can_message_t& msg);
    int PDO_processMsg(can_message_t& msg);
    void handleEMCYMsg(can_message_t& msg);

private:
    bool m_bInitialized = false;
    PortHandler* m_pPortHandler;
    BowState m_bowingState;
    BowDirection m_CurrentDirection = BowDirection::Down;

    float* m_afBowTrajectory = nullptr;
    int* m_aiStringId = nullptr;
    int m_iNPitches = 0;
    int m_iCurrentTrajIdx = 0; 

    float m_fAngle = 0;

    Epos4 m_epos;
    volatile int32_t m_iCurrentPosition = 0;

    Dynamixel m_leftDxl, m_rightDxl, m_slideDxl;
    HardwareTimer m_dxlTimer;

    BowController(PortHandler& portHandler);
    ~BowController();

    static BowController* pInstance;

    int computeBowTrajectory(const int bowChanges[], int numChanges, int nPitches);
    void encoderPositionCallback(int32_t encPos);
    static void dxlUpdateCallback();
};

#endif // BOWCONTROLLER_H