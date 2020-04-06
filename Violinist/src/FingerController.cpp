//
// Created by Raghavasimhan Sankaranarayanan on 1/21/20.
//

#include "FingerController.h"

FingerController::FingerController(): prevSentValue(0),
                                      m_bInitialized(false),
                                      currentState(REST),
                                      m_piCurrentPosition(nullptr),
                                      m_iCurrentPosition(0),
                                      m_commHandler(nullptr)
{}

Error_t FingerController::Create(FingerController* &pFinger) {
    pFinger = new FingerController();

    if(!pFinger)
        return kMemError;

    return kNoError;
}

Error_t FingerController::Destroy(FingerController* &pFinger) {
    pFinger->Reset();
    delete pFinger;
    pFinger = nullptr;
    return kNoError;
}

Error_t FingerController::Init(CommHandler* commHandler) {
    m_commHandler = commHandler;

    m_bInitialized = true;
    return kNoError;
}

Error_t FingerController::Reset() {
    m_bInitialized = false;
    return kNoError;
}

Error_t FingerController::Move(uint8_t value) {
    if (!m_bInitialized)
        return kNotInitializedError;

    m_commHandler->Send(Register::kFinger, value);

    return kNoError;
}

Error_t FingerController::Rest() {
    return SetCurrentState(REST, true);
}

Error_t FingerController::On() {
    return SetCurrentState(ON, true);
}

Error_t FingerController::Off() {
    return SetCurrentState(OFF, true);
}

uint8_t FingerController::GetPosition(FingerController::State state) {
    int cPos = m_iCurrentPosition;// *m_piCurrentPosition;

    double multiplier = std::abs(std::max((cPos - NUT_POSITION), 0)/(double)(MAX_ALLOWED_VALUE));
    int ulPosition = 0;
    switch (state) {
        case ON:
            ulPosition = ON_MIN + (int)(multiplier * (ON_MAX - ON_MIN));
            break;
        case OFF:
            ulPosition = OFF_MIN + (int)(multiplier * (OFF_MAX - OFF_MIN));
            break;
        case REST:
            ulPosition = OFF_MAX;
            break;
    }

    return Constrain(ulPosition);
}

int FingerController::Constrain(int val) {
    return std::min(std::max(val, (int)ON_MIN), (int)OFF_MAX);
}

Error_t FingerController::UpdatePosition(int position) {
    m_iCurrentPosition = position;
    int pos = GetPosition(currentState);

    Error_t err = kNoError;
    if (pos != prevSentValue) {
//        std::cout << pos << std::endl;
        err = Move(pos);
        prevSentValue = pos;
    }
    return err;
}

Error_t FingerController::SetCurrentState(FingerController::State c_state, bool b_move) {
    currentState = c_state;
    if (b_move) {
        int pos = GetPosition(currentState);
        return Move(pos);
    }
    return kNoError;
}
