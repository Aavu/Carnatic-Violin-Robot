//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#include "BowController.h"

BowController::BowController() : m_bInitialized(false),
                                 m_commHandler(nullptr),
                                 m_pPortHandler(nullptr),
                                 m_bowingState(Stopped),
                                 m_iBowSpeed(0), m_fBowPressure(0),
                                 m_currentDirection(Bow::Down), m_currentBowPitch(MIN_PITCH),
                                 m_currentAmplitude(0), m_currentSurge(0),
                                 m_piRTPosition(nullptr), m_wheelController(BOW_EPOS4_USB, 2) {}


BowController::~BowController()
{
    delete m_pPitchDxl;
    delete m_pYawDxl;
    delete m_pRollDxl;
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
    err = pCInstance->stopBowing(err);
    if (err != kNoError) {
        LOG_ERROR("Stop bowing failed");
//        CUtil::PrintError(__PRETTY_FUNCTION__, err);
        return err;
    }
    delete pCInstance;
    return kNoError;
#else
    return kNoError;
#endif
}

Error_t BowController::Init(CommHandler* commHandler, PortHandler& portHandler, int* RTPosition) {
    auto err = Reset();
    if (err != kNoError)
        return err;

    m_pPortHandler = &portHandler;
    auto* packetHandler = PortHandler::getPacketHandler();

    m_pPitchDxl = new Dynamixel(2, *m_pPortHandler, *packetHandler);
    m_pPitchDxl->operatingMode(OperatingMode::PositionControl);
    if (m_pPitchDxl->torque() != 0) {
        LOG_ERROR("Cannot set torque on pitch dxl");
        return kSetValueError;
    }

    if (m_pPitchDxl->moveToPosition(180.f) != SUCCESS) {
        LOG_ERROR("Cannot set torque on pitch dxl");
        return kSetValueError;
    }

    m_pRollDxl = new Dynamixel(4, *m_pPortHandler, *packetHandler);
    m_pRollDxl->operatingMode(OperatingMode::PositionControl);
    if (m_pRollDxl->torque() != 0) {
        LOG_ERROR("Cannot set torque on Roll dxl");
        return kSetValueError;
    }

    if (m_pRollDxl->moveToPosition(180.f) != SUCCESS) {
        LOG_ERROR("Cannot set torque on pitch dxl");
        return kSetValueError;
    }

    m_pYawDxl = new Dynamixel(3, *m_pPortHandler, *packetHandler);
    m_pYawDxl->operatingMode(OperatingMode::PositionControl);
    if (m_pYawDxl->torque() != 0) {
        LOG_ERROR("Cannot set torque on Yaw dxl");
        return kSetValueError;
    }

    if (m_pYawDxl->moveToPosition(140.f) != SUCCESS) {
        LOG_ERROR("Cannot set torque on pitch dxl");
        return kSetValueError;
    }



#ifdef ENABLE_BOWING
    m_piRTPosition = RTPosition;
    m_commHandler = commHandler;
    if (m_commHandler)
        m_bInitialized = true;
    else
        return kNotInitializedError;

    err = m_wheelController.init(EposController::ProfileVelocity);
    if (err != kNoError)
        return err;

    return m_wheelController.SetVelocityProfile(BOW_ACCELERATION);
#else
    return kNoError;
#endif
}

Error_t BowController::setSpeed(Bow::Direction direction, long bowSpeed, Error_t error) {
    if (error != kNoError)
        return error;
#ifdef ENABLE_BOWING
    m_iBowSpeed = bowSpeed;
    m_currentDirection = direction;
    if (direction == Bow::Up) {
        return m_wheelController.MoveWithVelocity(m_iBowSpeed);
    }
    return m_wheelController.MoveWithVelocity(-m_iBowSpeed);
#else
    return kNoError;
#endif
}

/* Pressure of 0 means not touching the string. Pressure of 1.0 means screaching sound */
Error_t BowController::setPressure(float fAmplitude) {
#ifdef ENABLE_BOWING
    m_fBowPressure = fAmplitude;
    auto fAngle = transformPressure(m_fBowPressure);
//    std::cout << fAmplitude << " " << fAngle << std::endl;
    if (m_pPitchDxl->moveToPosition(fAngle) != SUCCESS) {
        LOG_ERROR("Pitch dxl Move to position error");
        return kSetValueError;
    }
#endif
    return kNoError;
}

Error_t BowController::StartBowing(float amplitude, Bow::Direction direction, Error_t error) {
    if (error != kNoError)
        return error;
#ifdef ENABLE_BOWING
    m_currentDirection = direction;
    auto err = setAmplitude(amplitude, kNoError);

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

Error_t BowController::stopBowing(Error_t error) {
    if (error != kNoError)
        return error;
#ifdef ENABLE_BOWING
    auto err = kNoError;
    bowOnString(false, err);
    err = m_wheelController.Halt();
    if (err != kNoError)
        return err;
    m_bowingState = Stopped;
    return kNoError;
#else
    return kNoError;
#endif
}

Error_t BowController::bowOnString(bool on, Error_t error) {
    if (error != kNoError)
        return error;

#ifdef ENABLE_BOWING
    if (!m_bInitialized)
        return kNotInitializedError;

    auto err = kNoError;
    if (on) {
        err = setPressure(1.f);
    } else {
        err = setPressure(0.f);
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

//Error_t BowController::Send(Register::Bow reg, const uint8_t &data, Error_t error) {
//    if (error != kNoError)
//        return error;
//
//    return m_commHandler->Send(reg, data);
//}

template <typename T>
static T clamp(T value, T min, T max) {
    return std::max(std::min(value, max), min);
}

Error_t BowController::setAmplitude(float amplitude, Error_t error) {
    if (error != kNoError)
        return error;
#ifdef ENABLE_BOWING
    amplitude = clamp(amplitude, 0.4f, 1.0f);
    m_currentAmplitude = amplitude;

    long velocity;
    float pressure;
    auto err = ComputeVelocityAndPressureFor(m_currentAmplitude, &velocity, &pressure);
    err = setPressure(m_currentAmplitude);
    err = setSpeed(m_currentDirection, velocity, err);

    return err;
#else
    return kNoError;
#endif
}

/* Pressure of 0 means not touching the string. Pressure of 1.0 means screaching sound */
float BowController::transformPressure(float pressure) {
    float multiplier = .75;
    float range = MAX_PITCH * 1.0f / MIN_PITCH;
    return MIN_PITCH * (1.0 + ((range - 1) * pressure * multiplier));
}

/* Velocity of 0 means not playing. 1.0 means full rpm */
long BowController::transformVelocity(float velocity) {
    float multiplier = 1;
    float range = MAX_VELOCITY * 1.0f / MIN_VELOCITY;
    return MIN_VELOCITY * (1.0 + ((range - 1) * velocity * multiplier));
}

Error_t BowController::ComputeVelocityAndPressureFor(float amplitude, long* velocity, float* pressure) {
    *pressure = transformPressure(amplitude);
    *velocity = transformVelocity(amplitude);
    return kNoError;
}

Bow::Direction BowController::changeDirection() {
#ifdef ENABLE_BOWING
    m_currentDirection = static_cast<Bow::Direction>(!m_currentDirection);
    return m_currentDirection;
#else
    return Bow::Down;
#endif
}

Error_t BowController::setDirection(Bow::Direction direction) {
#ifdef ENABLE_BOWING
    m_currentDirection = direction;
    if (m_bowingState == Playing)
        return setAmplitude(m_currentAmplitude);
    return kNoError;
#else
    return kNoError;
#endif
}

Error_t BowController::resumeBowing(Error_t error) {
#ifdef ENABLE_BOWING
    error = setSpeed(m_currentDirection, m_currentAmplitude, error);
    if (error != kNoError)
        m_bowingState = Playing;
    return error;
#else
    return kNoError;
#endif
}

Error_t BowController::PauseBowing(Error_t error) {
#ifdef ENABLE_BOWING
    error = setSpeed(m_currentDirection, 0, error);
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
    auto err = setPressure((uint8_t)ROSIN_PRESSURE);
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

Error_t BowController::setString(BowController::String str) {
    float angle = 180;
    switch (str) {
        case E:
            angle = 195;
            break;
        case A:
            angle = 185;
            break;
        case D:
            angle = 178;
            break;
        case G:
            angle = 165;
            break;
    }

    if (m_pRollDxl->moveToPosition(angle) != SUCCESS) {
        LOG_ERROR("Cannot set torque on pitch dxl");
        return kSetValueError;
    }
    return kNoError;
}
