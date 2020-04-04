//
// Created by Raghavasimhan Sankaranarayanan on 1/21/20.
//

#ifndef VIOLINIST_FINGERCONTROLLER_H
#define VIOLINIST_FINGERCONTROLLER_H

#include <iostream>
#include <cstring>

#include "MyDefinitions.h"
#include "CommHandler.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;

class FingerController {
    enum State {
        OFF = 0,
        ON,
        REST
    };

    const uint8_t OFF_MAX = 160;
    const uint8_t ON_MAX = 110;
    const uint8_t ON_MIN = 98;
    const uint8_t OFF_MIN = 125;

    uint8_t prevSentValue;
    bool m_bInitialized;

    State currentState;

    int* p_iCurrentPosition;

    CommHandler* m_commHandler;

    FingerController();

    uint8_t GetPosition(State state);
    int Constrain(int val);

    Error_t SetCurrentState(State c_state, bool b_move = false);

public:
    Error_t SetPositionTracker(int* position);
    static Error_t Create(FingerController* &pFinger);
    static Error_t Destroy(FingerController* &pFinger);

    Error_t Init(CommHandler* commHandler, int* position = nullptr);
    Error_t Reset();

    Error_t Move(uint8_t value);

    Error_t UpdatePosition();

    Error_t Rest();
    Error_t On();
    Error_t Off();
};


#endif //VIOLINIST_FINGERCONTROLLER_H
