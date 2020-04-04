//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#include "BowController.h"

BowController::BowController() :    m_bInitialized(false),
                                    m_commHandler(nullptr),
                                    m_bowingState(Stopped),
                                    m_iBowSpeed(0), m_fBowPressure(0), m_currentDirection(Bow::Down), m_currentBowPitch(MIN_PITCH),
                                    m_iRTPosition(nullptr) {}

Error_t BowController::Create(BowController *&pCInstance) {
    pCInstance = new BowController();
    return kNoError;
}

Error_t BowController::Destroy(BowController *&pCInstance) {
    auto err = kNoError;
    pCInstance->StopBowing(err);
    if (err != kNoError)
        return err;
    delete pCInstance;
    return kNoError;
}

Error_t BowController::Init(CommHandler* commHandler, int* RTPosition) {
    auto err = Reset();
    m_iRTPosition = RTPosition;
    m_commHandler = commHandler;
    if (m_commHandler)
        m_bInitialized = true;
    else
        err = kNotInitializedError;
    return err;
}

Error_t BowController::SetSpeed(Bow::Direction direction, uint8_t bowSpeed, Error_t error) {
    if (error != kNoError)
        return error;
    m_iBowSpeed = bowSpeed;
    auto err = kNoError;
    m_currentDirection = direction;
    if (direction == Bow::Up) {
        err = Send(Register::kRoller, 128 + m_iBowSpeed, err);
    } else {
        err = Send(Register::kRoller, 128 - m_iBowSpeed, err);
    }

    return err;
}

/* Pressure of 0 means not touching the string. Pressure of 1.0 means screaching sound */
Error_t BowController::SetPressure(float bowPressure, Error_t error) {
    if (error != kNoError)
        return error;

    m_fBowPressure = bowPressure;
    auto iPressure = TransformPressure(m_fBowPressure);

    auto err = kNoError;
    return Send(Register::kPitch, iPressure, err);
}

Error_t BowController::SetPressure(uint8_t bowPressure, Error_t error) {
    return Send(Register::kPitch, bowPressure, error);
}

Error_t BowController::StartBowing(float amplitude, Bow::Direction direction, Error_t error) {
    auto err = kNoError;
    if (error != kNoError)
        return error;

    m_currentDirection = direction;
    err = SetAmplitude(amplitude, err);

    err = Send(Register::kRollerEnable, 1, err);

    if (err != kNoError)
        return err;

    m_bowingState = Playing;

    return kNoError;
}

Error_t BowController::StopBowing(Error_t error) {
    if (error != kNoError)
        return error;
    auto err = kNoError;
    err = BowOnString(false, err);
    err = Send(Register::kRoller, 128, err);
    err = Send(Register::kRollerEnable, 0, err);
    if (err != kNoError)
        return err;
    m_bowingState = Stopped;
    return kNoError;
}

Error_t BowController::BowOnString(bool on, Error_t error) {
    if (error != kNoError)
        return error;

    if (!m_bInitialized)
        return kNotInitializedError;

    auto err = kNoError;
    if (on) {
        err = SetPressure(1.f, err);
    } else {
        err = SetPressure(0.f, err);
    }

    return err;
}

Error_t BowController::Reset() {
    if (m_bowingState != Stopped)
        return kUnknownError;
    m_bInitialized = false;
    m_commHandler = nullptr;
    return kNoError;
}

Error_t BowController::Send(Register::Bow reg, const uint8_t &data, Error_t error) {
    if (error != kNoError)
        return error;

    return m_commHandler->Send(reg, data);
}

template <typename T>
static T clamp(T value, T min, T max) {
    return std::max(std::min(value, max), min);
}

Error_t BowController::SetAmplitude(float amplitude, Error_t error) {
    if (error != kNoError)
        return error;

    amplitude = clamp(amplitude, 0.0f, 1.0f);
    m_currentAmplitude = amplitude;

    uint8_t velocity, pressure;
    auto err = ComputeVelocityAndPressureFor(m_currentAmplitude, &velocity, &pressure, error);
    err = SetPressure(pressure, err);
    err = SetSpeed(m_currentDirection, velocity, err);

    return err;
}

/* Pressure of 0 means not touching the string. Pressure of 1.0 means screaching sound */
uint8_t BowController::TransformPressure(float pressure) {
    float multiplier = .75;
    float range = MAX_PITCH * 1.0f / MIN_PITCH;
    return MIN_PITCH * (1.0 + ((range - 1) * pressure * multiplier));
}

/* Velocity of 0 means not playing. 1.0 means full rpm */
uint8_t BowController::TransformVelocity(float velocity) {
    float multiplier = 1;
    float range = MAX_VELOCITY * 1.0f / MIN_VELOCITY;
    return MIN_VELOCITY * (1.0 + ((range - 1) * velocity * multiplier));
}

Error_t BowController::ComputeVelocityAndPressureFor(float amplitude, uint8_t* velocity, uint8_t* pressure, Error_t error) {
    *pressure = TransformPressure(amplitude);
    *velocity = TransformVelocity(amplitude);
    return error;
}

Bow::Direction BowController::ChangeDirection() {
    m_currentDirection = static_cast<Bow::Direction>(!m_currentDirection);
    return m_currentDirection;
}

Error_t BowController::SetDirection(Bow::Direction direction) {
    m_currentDirection = direction;
    if (m_bowingState == Playing)
        return SetAmplitude(m_currentAmplitude);
    return kNoError;
}
