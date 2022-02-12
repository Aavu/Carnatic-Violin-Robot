//
// Created by Raghavasimhan Sankaranarayanan on 1/21/20.
//

#include "FingerController.h"

FingerController::FingerController(CommHandler* pCommHandler):  EposController("USB0", 1),
                                                                prevSentValue(0),
                                                                m_bInitialized(false),
                                                                currentState(REST),
                                                                m_piCurrentPosition(nullptr),
                                                                m_iCurrentPosition(0),
                                                                m_pCommHandler(pCommHandler)
{
    auto err = EposController::Init(EposController::ProfilePosition);
    if (err == kNoError)
        m_bInitialized = true;
}

FingerController::~FingerController()
{

}

//Error_t FingerController::Create(FingerController* &pFinger) {
//    pFinger = new FingerController();
//
//    if(!pFinger)
//        return kMemError;
//
//    return kNoError;
//}
//
//Error_t FingerController::Destroy(FingerController* &pFinger) {
//    pFinger->Reset();
//    delete pFinger;
//    pFinger = nullptr;
//    return kNoError;
//}

//Error_t FingerController::Init() {
//    auto err = EposController::Init(EposController::ProfilePosition);
//    if (err != kNoError)
//        return err;
//    m_bInitialized = true;
//    return kNoError;
//}

Error_t FingerController::Reset() {
    m_bInitialized = false;
    return kNoError;
}

Error_t FingerController::Move(uint8_t value) {
    if (!m_bInitialized)
        return kNotInitializedError;

    m_pCommHandler->Send(Register::kFinger, value);

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
    switch(state) {
        case OFF:
            return FINGER_OFF;
        case ON:
            return FINGER_ON;
        default:
            return FINGER_OFF;
    }
}

//int FingerController::Constrain(int val) {
//    return std::min(std::max(val, (int)ON_MIN), (int)OFF_MAX);
//}

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

Error_t FingerController::MoveToPositionf(float fFretPosition)
{
    return MoveToPositionl(Util::Fret2Position(fFretPosition));
}

Error_t FingerController::SetHome()
{
    Error_t err = EposController::SetHome();
    if (err != kNoError) {
        return err;
    }

    err = m_pCommHandler->Send(Register::kFingerOffMax, FINGER_OFF_MAX);
    if (err != kNoError) {
        std::cerr << "Set Value error: kFingerOffMax" << std::endl;
        return err;
    }

    err = m_pCommHandler->Send(Register::kFingerOnMax, FINGER_ON_MAX);
    if (err != kNoError) {
        std::cerr << "Set Value error: kFingerOnMax" << std::endl;
        return err;
    }

    err = m_pCommHandler->Send(Register::kFingerOffMin, FINGER_OFF_MIN);
    if (err != kNoError) {
        std::cerr << "Set Value error: kFingerOffMin" << std::endl;
        return err;
    }

    err = m_pCommHandler->Send(Register::kFingerOnMin, FINGER_ON_MIN);
    if (err != kNoError) {
        std::cerr << "Set Value error: kFingerOnMin" << std::endl;
        return err;
    }

    return m_pCommHandler->Send(Register::kEncoderSetHome, 0);
}

bool FingerController::IsInitialized() const
{
    return m_bInitialized;
}
