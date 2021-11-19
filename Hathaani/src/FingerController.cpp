//
// Created by Raghavasimhan Sankaranarayanan on 1/21/20.
//

#include "FingerController.h"

FingerController::FingerController(CommHandler* pCommHandler,
                                   PortHandler& portHandler):  EposController(FINGER_EPOS4_USB, 1),
                                                                prevSentValue(0),
                                                                m_bInitialized(false),
                                                                currentState(REST),
                                                                m_piCurrentPosition(nullptr),
                                                                m_iCurrentPosition(0),
                                                                m_pCommHandler(pCommHandler)
{
    auto err = EposController::init(EposController::ProfilePosition);
    if (err != kNoError) {
        LOG_ERROR("Epos Controller Init failed.");
        return;
    }
    err = m_finger.init(portHandler);

    m_bInitialized = true;
}

FingerController::~FingerController()
{
}

Error_t FingerController::Reset() {
    m_bInitialized = false;
    return kNoError;
}

Error_t FingerController::move(uint8_t value) {
    if (!m_bInitialized)
        return kNotInitializedError;

    m_pCommHandler->Send(Register::kFinger, value);

    return kNoError;
}

Error_t FingerController::Rest() {
    return setCurrentState(REST, true);
}

Error_t FingerController::On() {
    return setCurrentState(ON, true);
}

Error_t FingerController::Off() {
    return setCurrentState(OFF, true);
}

float FingerController::GetPosition(FingerController::State state) {
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

//Error_t FingerController::UpdatePosition(int position) {
//    m_iCurrentPosition = position;
//    int pos = GetPosition(currentState);
//
//    Error_t err = kNoError;
//    if (pos != prevSentValue) {
////        std::cout << pos << std::endl;
//        err = move(pos);
//        prevSentValue = pos;
//    }
//    return err;
//}

Error_t FingerController::setCurrentState(FingerController::State c_state, bool bMove) {
    if (c_state == currentState)
        return kNoError;

    currentState = c_state;
    if (bMove) {
        auto pos = GetPosition(currentState);
        return m_finger.moveToPosition(-153, pos, false);
    }
    return kNoError;
}

Error_t FingerController::moveToPosition(float fFretPosition)
{
    return moveToPositionl(Util::fret2Position(fFretPosition));
}

Error_t FingerController::setHome()
{
    float theta[2] = FINGER_HOME_THETA;
    Error_t err;
    err = m_finger.moveJoints(theta, true);
    if (err != kNoError) {
        return err;
    }

    err = EposController::setHome();
    if (err != kNoError) {
        return err;
    }

    return m_pCommHandler->Send(Register::kEncoderSetHome, 0);
}

bool FingerController::isInitialized() const
{
    return m_bInitialized;
}
