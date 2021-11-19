//
// Created by Raghavasimhan Sankaranarayanan on 1/21/20.
//

#ifndef HATHAANI_FINGERCONTROLLER_H
#define HATHAANI_FINGERCONTROLLER_H

#include <iostream>
#include <cstring>
#include <cmath>

#include "Finger.h"
#include "EposController.h"
#include "MyDefinitions.h"
#include "CommHandler.h"
#include "Util.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;

class FingerController: public EposController {
    inline static const std::string kName = "FingerController";

public:
    FingerController(CommHandler* pCommHandler, PortHandler& portHandler);
    ~FingerController() override;
//    static Error_t Create(FingerController* &pFinger);
//    static Error_t Destroy(FingerController* &pFinger);

//    Error_t init();

    [[nodiscard]] bool isInitialized() const;
    Error_t Reset();

    Error_t move(uint8_t value);

//    Error_t UpdatePosition(int position);

    Error_t setHome() override;

    Error_t Rest();
    Error_t On();
    Error_t Off();

//    Error_t SetPositionProfile(unsigned long ulVelocity = 2500, unsigned long ulAcc = 10000);
//    Error_t moveToPosition(long targetPos);
    Error_t moveToPosition(float fFretPosition);

private:
    enum State {
        OFF = 0,
        ON,
        REST
    };

    uint8_t prevSentValue;
    bool m_bInitialized;

    State currentState;

    int* m_piCurrentPosition;
    int m_iCurrentPosition = 0;

//    EposController m_indexController;

    Finger m_finger;
    CommHandler* m_pCommHandler;

    static float GetPosition(State state);
//    static int Constrain(int val);

    Error_t setCurrentState(State c_state, bool bMove = false);
};


#endif //HATHAANI_FINGERCONTROLLER_H
