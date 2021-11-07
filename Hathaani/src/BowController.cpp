//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#include "BowController.h"

BowController::BowController() : m_bInitialized(false),
                                 m_commHandler(nullptr),
                                 m_bowingState(Stopped),
                                 m_iBowSpeed(0), m_fBowPressure(0),
                                 m_currentDirection(Bow::Down), m_currentBowPitch(MIN_PITCH),
                                 m_currentAmplitude(0), m_currentSurge(0),
                                 m_piRTPosition(nullptr), m_wheelController(BOW_EPOS4_USB, 2) {}


BowController::~BowController()
{
}

Error_t BowController::Create(BowController *&pCInstance) {
#ifdef ENABLE_BOWING
    pCInstance = new BowController();
    return kNoError;
#else
    return kNoError;
#endif
}

Error_t BowController::Destroy(BowController *&pCInstance) {
#ifdef ENABLE_BOWING
    auto err = kNoError;
    err = pCInstance->StopBowing(err);
    if (err != kNoError) {
        CUtil::PrintError(__PRETTY_FUNCTION__, err);
        return err;
    }
    delete pCInstance;
    return kNoError;
#else
    return kNoError;
#endif
}

Error_t BowController::Init(CommHandler* commHandler, int* RTPosition) {
    auto err = Reset();
    if (err != kNoError)
        return err;
#ifdef ENABLE_BOWING
    m_piRTPosition = RTPosition;
    m_commHandler = commHandler;
    if (m_commHandler)
        m_bInitialized = true;
    else
        return kNotInitializedError;

    err = m_wheelController.Init(EposController::ProfileVelocity);
    if (err != kNoError)
        return err;

    return m_wheelController.SetVelocityProfile(BOW_ACCELERATION);
#else
    return kNoError;
#endif
}

Error_t BowController::SetSpeed(Bow::Direction direction, uint8_t bowSpeed, Error_t error) {
    if (error != kNoError)
        return error;
#ifdef ENABLE_BOWING
    m_iBowSpeed = bowSpeed;
    m_currentDirection = direction;
    if (direction == Bow::Up) {
        return m_wheelController.MoveWithVelocity(m_iBowSpeed);
//        err = Send(Register::kRoller, 128 + m_iBowSpeed, err);
    }
//    else {
//        err = Send(Register::kRoller, 128 - m_iBowSpeed, err);
//    }
    return m_wheelController.MoveWithVelocity(-m_iBowSpeed);
#else
    return kNoError;
#endif
}

/* Pressure of 0 means not touching the string. Pressure of 1.0 means screaching sound */
Error_t BowController::SetPressure(float bowPressure, Error_t error) {
    if (error != kNoError)
        return error;
#ifdef ENABLE_BOWING
    m_fBowPressure = bowPressure;
    auto iPressure = TransformPressure(m_fBowPressure);

    auto err = kNoError;
    return Send(Register::kPitch, iPressure, err);
#else
    return kNoError;
#endif
}

Error_t BowController::SetPressure(uint8_t bowPressure, Error_t error) {
    return Send(Register::kPitch, bowPressure, error);
}

Error_t BowController::StartBowing(float amplitude, Bow::Direction direction, Error_t error) {
    if (error != kNoError)
        return error;
#ifdef ENABLE_BOWING
    m_currentDirection = direction;
    auto err = SetAmplitude(amplitude, kNoError);

    if (err != kNoError) {
        std::cerr << "Error setting amplitude" << std::endl;
        return err;
    }

    m_bowingState = Playing;
    return kNoError;
#else
    return kNoError;
#endif

}

Error_t BowController::StopBowing(Error_t error) {
    if (error != kNoError)
        return error;
#ifdef ENABLE_BOWING
    auto err = kNoError;
    BowOnString(false, err);
    err = m_wheelController.Halt();
    if (err != kNoError)
        return err;
    m_bowingState = Stopped;
    return kNoError;
#else
    return kNoError;
#endif
}

Error_t BowController::BowOnString(bool on, Error_t error) {
    if (error != kNoError)
        return error;

#ifdef ENABLE_BOWING
    if (!m_bInitialized)
        return kNotInitializedError;

    auto err = kNoError;
    if (on) {
        err = SetPressure(1.f, err);
    } else {
        err = SetPressure(0.f, err);
    }

    return err;
#else
    return kNoError;
#endif
}

Error_t BowController::Reset() {
#ifdef ENABLE_BOWING
    if (m_bowingState != Stopped)
        return kUnknownError;
    m_bInitialized = false;
    m_commHandler = nullptr;
#endif
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
#ifdef ENABLE_BOWING
    amplitude = clamp(amplitude, 0.4f, 1.0f);
    m_currentAmplitude = amplitude;

    uint8_t velocity, pressure;
    auto err = ComputeVelocityAndPressureFor(m_currentAmplitude, &velocity, &pressure, error);
    err = SetPressure(pressure, err);
    err = SetSpeed(m_currentDirection, velocity, err);

    return err;
#else
    return kNoError;
#endif
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
#ifdef ENABLE_BOWING
    m_currentDirection = static_cast<Bow::Direction>(!m_currentDirection);
    return m_currentDirection;
#else
    return Bow::Down;
#endif
}

Error_t BowController::SetDirection(Bow::Direction direction) {
#ifdef ENABLE_BOWING
    m_currentDirection = direction;
    if (m_bowingState == Playing)
        return SetAmplitude(m_currentAmplitude);
    return kNoError;
#else
    return kNoError;
#endif
}

Error_t BowController::ResumeBowing(Error_t error) {
#ifdef ENABLE_BOWING
    error = SetSpeed(m_currentDirection, m_currentAmplitude, error);
    if (error != kNoError)
        m_bowingState = Playing;
    return error;
#else
    return kNoError;
#endif
}

Error_t BowController::PauseBowing(Error_t error) {
#ifdef ENABLE_BOWING
    error = SetSpeed(m_currentDirection, 0, error);
    if (error != kNoError)
        m_bowingState = Paused;
    return error;
#else
    return kNoError;
#endif
}

Error_t BowController::RosinMode()
{
#ifdef ENABLE_BOWING
    auto err = SetPressure((uint8_t)ROSIN_PRESSURE);
    if (err != kNoError)
        return err;

    if (m_currentDirection == Bow::Down)
        return m_wheelController.MoveWithVelocity(ROSIN_SPEED);
    else
        return m_wheelController.MoveWithVelocity(-ROSIN_SPEED);
#else
    return kNoError;
#endif
}
