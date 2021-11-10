//
// Created by Raghavasimhan Sankaranarayanan on 1/21/20.
//

#ifndef HATHAANI_FINGERCONTROLLER_H
#define HATHAANI_FINGERCONTROLLER_H

#include <iostream>
#include <cstring>
#include <cmath>

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
    explicit FingerController(CommHandler* pCommHandler);
    ~FingerController() override;
//    static Error_t Create(FingerController* &pFinger);
//    static Error_t Destroy(FingerController* &pFinger);

//    Error_t Init();

    [[nodiscard]] bool IsInitialized() const;
    Error_t Reset();

    Error_t Move(uint8_t value);

//    Error_t UpdatePosition(int position);

    Error_t SetHome() override;

    Error_t Rest();
    Error_t On();
    Error_t Off();

//    Error_t SetPositionProfile(unsigned long ulVelocity = 2500, unsigned long ulAcc = 10000);
//    Error_t MoveToPosition(long targetPos);
    Error_t MoveToPosition(float fFretPosition);

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

    CommHandler* m_pCommHandler;

    static uint8_t GetPosition(State state);
//    static int Constrain(int val);

    Error_t SetCurrentState(State c_state, bool b_move = false);
};


#endif //HATHAANI_FINGERCONTROLLER_H
