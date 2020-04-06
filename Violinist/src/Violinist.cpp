//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#include "Violinist.h"

Violinist::Violinist(): m_iRTPosition(0),
                        ulErrorCode(0),
                        m_bStopPositionUpdates(false),
                        m_commHandler(nullptr),
                        m_bowController(nullptr),
                        m_fingerController(nullptr)
{
    SetDefaultParameters();
}

Violinist::~Violinist() = default;

void Violinist::SetDefaultParameters() {
    //USB
    g_usNodeId = 1;
    g_deviceName = "EPOS4";
    g_protocolStackName = "MAXON SERIAL V2";
    g_interfaceName = "USB";
    g_portName = "USB0";
    g_baudrate = 1000000;
}

Error_t Violinist::OpenDevice() {
    Error_t lResult = kNoError;
    ulErrorCode = 0;
    char* pDeviceName = new char[255];
    char* pProtocolStackName = new char[255];
    char* pInterfaceName = new char[255];
    char* pPortName = new char[255];

    strcpy(pDeviceName, g_deviceName.c_str());
    strcpy(pProtocolStackName, g_protocolStackName.c_str());
    strcpy(pInterfaceName, g_interfaceName.c_str());
    strcpy(pPortName, g_portName.c_str());

    LogInfo("Open device...");

    g_pKeyHandle = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, &ulErrorCode);

    if (g_pKeyHandle != nullptr && ulErrorCode == 0) {
        unsigned int lBaudrate = 0;
        unsigned int lTimeout = 0;

        if (VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, &ulErrorCode) != 0) {
            if (VCS_SetProtocolStackSettings(g_pKeyHandle, g_baudrate, lTimeout, &ulErrorCode) != 0) {
                if (VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, &ulErrorCode) != 0) {
                    if (g_baudrate != (int)lBaudrate) {
                        lResult = kSetValueError;
                        LogError("VCS_SetProtocolStackSettings", lResult, ulErrorCode);
                        return lResult;
                    }
                }
            }
        }
    } else {
        g_pKeyHandle = nullptr;
    }

    delete[] pDeviceName;
    delete[] pProtocolStackName;
    delete[] pInterfaceName;
    delete[] pPortName;

    return lResult;
}

Error_t Violinist::Init(bool shouldHome) {
    Error_t lResult;
    BOOL oIsFault = 0;

    if ((lResult = OpenDevice()) != kNoError) {
        LogError("OpenDevice", lResult, ulErrorCode);
        return lResult;
    }

    if (VCS_GetFaultState(g_pKeyHandle, g_usNodeId, &oIsFault, &ulErrorCode) == 0) {
        LogError("VCS_GetFaultState", lResult, ulErrorCode);
        lResult = kGetValueError;
        return lResult;
    }

    if (oIsFault) {
        stringstream msg;
        msg << "clear fault, node = '" << g_usNodeId << "'";
        LogInfo(msg.str());

        if (VCS_ClearFault(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
            LogError("VCS_ClearFault", lResult, ulErrorCode);
            lResult = kSetValueError;
            return lResult;
        }
    }

    BOOL oIsEnabled = 0;

    if (VCS_GetEnableState(g_pKeyHandle, g_usNodeId, &oIsEnabled, &ulErrorCode) == 0) {
        LogError("VCS_GetEnableState", lResult, ulErrorCode);
        lResult = kGetValueError;
        return lResult;
    }

    if (!oIsEnabled) {
        if (VCS_SetEnableState(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
            LogError("VCS_SetEnableState", lResult, ulErrorCode);
            lResult = kSetValueError;
            return lResult;
        }
    }

    stringstream msg;
    msg << "set position mode, node = " << g_usNodeId;
//    LogInfo(msg.str());

    if (shouldHome) {
        lResult = SetHome();
        if (lResult != kNoError) {
            LogError("SetHome", lResult, ulErrorCode);
            return lResult;
        }
    }

    if (VCS_ActivatePositionMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
        lResult = kSetValueError;
        LogError("VCS_ActivatePositionMode", lResult, ulErrorCode);
        return lResult;
    }

//    if (VCS_SetMaxAcceleration(g_pKeyHandle, g_usNodeId, 2000, &ulErrorCode) == 0) {
//        lResult = kSetValueError;
//        LogError("VCS_SetMaxAcceleration", lResult, ulErrorCode);
//        return lResult;
//    }

    if (VCS_SetMaxFollowingError(g_pKeyHandle, g_usNodeId, m_ulMaxFollowErr, &ulErrorCode) == 0) {
        lResult = kGetValueError;
        LogError("VCS_GetMaxFollowingError", lResult, ulErrorCode);
        return lResult;
    }

    unsigned int get_maxErr = 0;

    if (VCS_GetMaxFollowingError(g_pKeyHandle, g_usNodeId, &get_maxErr, &ulErrorCode) == 0) {
        lResult = kGetValueError;
        LogError("VCS_GetMaxFollowingError", lResult, ulErrorCode);
        return lResult;
    }

    if (m_ulMaxFollowErr != get_maxErr) {
        lResult = kSetValueError;
        LogError("MaxFollowingError", lResult, ulErrorCode);
        return lResult;
    }

    if ((lResult = CommHandler::Create(m_commHandler)) != kNoError) {
        LogError("CommHandlerCreationError", lResult, ulErrorCode);
        return lResult;
    }

    if ((lResult = m_commHandler->Init()) != kNoError) {
        LogError("CommHandlerInitError", lResult, ulErrorCode);
        return lResult;
    }

    if ((lResult = BowController::Create(m_bowController)) != kNoError) {
        LogError("BowControllerCreationError", lResult, ulErrorCode);
        return lResult;
    }

    if ((lResult = m_bowController->Init(m_commHandler, &m_iRTPosition)) != kNoError) {
        LogError("BowControllerInitError", lResult, ulErrorCode);
        return lResult;
    }

    if ((lResult = FingerController::Create(m_fingerController)) != kNoError) {
        LogError("FingerCreationError", lResult, ulErrorCode);
        return lResult;
    }

    if ((lResult = m_fingerController->Init(m_commHandler)) != kNoError) {
        LogError("FingerInitError", lResult, ulErrorCode);
        return lResult;
    }

//    TrackEncoderPosition();
    TrackTargetPosition();

    return kNoError;
}

Error_t Violinist::CloseDevice() {
    Error_t err = kNoError;
    if (VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
        err = kSetValueError;
        LogError("VCS_SetDisableState", err, ulErrorCode);
    }

    m_bStopPositionUpdates = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    LogInfo("Close device");

    if ((err = FingerController::Destroy(m_fingerController)) != kNoError) {
        LogError("FingerController::destroy", err, ulErrorCode);
        return err;
    }

    if ((err = BowController::Destroy(m_bowController)) != kNoError) {
        LogError("BowController::destroy", err, ulErrorCode);
        return err;
    }

    if (VCS_CloseDevice(g_pKeyHandle, &ulErrorCode) == 0 && ulErrorCode != 0) {
        err = kUnknownError;
        LogError("VCS_CloseDevice", err, ulErrorCode);
    }

    return err;
}

void Violinist::LogInfo(const string &message) {
    cout << message << endl;
}

void Violinist::LogError(const string &functionName, int p_lResult, unsigned int p_ulErrorCode) {
    cerr << g_programName << ": " << functionName << " failed (result=" << p_lResult << ", errorCode=0x" << std::hex << p_ulErrorCode << ")" << endl;
}

Error_t Violinist::MoveToPosition(long targetPos) {
    Error_t lResult = kNoError;
    if (std::abs(targetPos) < std::abs(MAX_ENCODER_INC) && std::abs(targetPos) >= std::abs(NUT_POSITION)) {
        m_fingerController->UpdatePosition(std::abs(targetPos));
        if (VCS_SetPositionMust(g_pKeyHandle, g_usNodeId, targetPos, &ulErrorCode) == 0) {
            lResult = kSetValueError;
            LogError("VCS_SetPositionMust", lResult, ulErrorCode);
        }
    }
    return lResult;
}

double Violinist::FretLength(double fretNumber) {
    return (SCALE_LENGTH - (SCALE_LENGTH / pow(2, (fretNumber / 12.0))));
}

long Violinist::PositionToPulse(double p) {
    return (long)(m_iEncoderDirection * p * 24000.0 / 220.0);
}

Error_t Violinist::UpdateEncoderPosition() {
    if (VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &m_iRTPosition, &ulErrorCode) == 0) {
        std::cerr << "get value error...\n";
        return kGetValueError;
    }

//    m_fingerController->UpdatePosition();
//    cout << m_iRTPosition << endl;
    return kNoError;
}

Error_t Violinist::UpdateTargetPosition() {
    long targetPosition = ConvertToTargetPosition(m_pfFretPosition);
    return MoveToPosition(targetPosition);
}

long Violinist::ConvertToTargetPosition(double fretPos) {
    return PositionToPulse(FretLength(fretPos)) + NUT_POSITION;
}

void Violinist::TrackEncoderPosition() {
    m_bStopPositionUpdates = false;
    std::thread([this]() {
        while (!m_bStopPositionUpdates)
        {
            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_iTimeInterval);
            if (UpdateEncoderPosition() != kNoError)
                break;
            std::this_thread::sleep_until(x);
        }
    }).detach();
}

void Violinist::TrackTargetPosition() {
    m_bStopPositionUpdates = false;
    std::thread([this]() {
        while (!m_bStopPositionUpdates)
        {
//            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_iTimeInterval);
            if (UpdateTargetPosition() != kNoError)
                break;
//            std::this_thread::sleep_until(x);
//            cout << GetPosition() << " " << ConvertToTargetPosition(m_pfFretPosition) << endl;
        }
    }).detach();
}

unsigned int Violinist::GetErrorCode() {
    return ulErrorCode;
}

int Violinist::GetPosition() {
    UpdateEncoderPosition();
    return m_iRTPosition;
}

Error_t Violinist::SetHome() {
    LogInfo("Setting Home..");
    Error_t lResult = kNoError;
    int _lastPosition = 10000;
    int _currentPosition = -10000;
    unsigned int errorCode = 0;
    if (VCS_ActivateProfilePositionMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
        lResult = kSetValueError;
        LogError("VCS_ActivateProfilePositionMode", lResult, ulErrorCode);
        return lResult;
    }
    while (true) {
        if (RawMoveToPosition(-25*m_iEncoderDirection, 1000, 0) != kNoError) {
            lResult = kSetValueError;
            LogError("moveToPosition", lResult, errorCode);
            return lResult;
        }
        if (VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &_currentPosition, &errorCode) == 0) {
            lResult = kGetValueError;
            LogError("VCS_GetPositionIs", lResult, errorCode);
            return lResult;
        }
        if (abs(_currentPosition - _lastPosition) < 10)
            break;
//        cout << "moving" << endl;
        _lastPosition = _currentPosition;
    }
    if (VCS_DefinePosition(g_pKeyHandle, g_usNodeId, 0, &errorCode) == 0) {
        lResult = kSetValueError;
        LogError("VCS_DefinePosition", lResult, errorCode);
    }
    return lResult;
}

Error_t Violinist::RawMoveToPosition(int _pos, unsigned int _acc, BOOL _absolute) {
    Error_t lResult = kNoError;
    unsigned int errorCode = 0;

    if (VCS_ActivateProfilePositionMode(g_pKeyHandle, g_usNodeId, &errorCode) == 0) {
        lResult = kSetValueError;
        LogError("VCS_ActivateProfilePositionMode", lResult, errorCode);
        return lResult;
    }

    stringstream msg;
    msg << "move to position = " << _pos << ", acc = " << _acc;
    LogInfo(msg.str());

    if (VCS_SetPositionProfile(g_pKeyHandle, g_usNodeId, 2500, _acc, _acc, &errorCode) == 0) {
        lResult = kSetValueError;
        LogError("VCS_SetPositionProfile", lResult, errorCode);
        return lResult;
    }

    if (VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, _pos, _absolute, 1, &errorCode) == 0) {
        lResult = kSetValueError;
        LogError("VCS_MoveToPosition", lResult, errorCode);
        return lResult;
    }

    if (VCS_WaitForTargetReached(g_pKeyHandle, g_usNodeId, 50, &errorCode) == 0) {
        lResult = kGetValueError;
        LogError("VCS_WaitForTargetReached", lResult, errorCode);
    }
    return lResult;
}

Error_t Violinist::Perform(const double *pitches, const size_t& length, short transpose) {
    auto err = kNoError;
    err = m_fingerController->Rest();
    err = m_bowController->StartBowing(0.7, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (size_t i=0; i < length; i++) {
        if (pitches[i] >= 0.0) {
            m_fingerController->On();
            m_pfFretPosition = pitches[i] + transpose;
        } else {
//            m_fingerController->Off();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(11)); // number depends on hopsize of pitch track
    }

    std::this_thread::sleep_for(std::chrono::seconds (1));

    err = m_bowController->StopBowing(kNoError);
    if (err != kNoError)
        return err;

    err = m_fingerController->Rest();
    return err;
}

Error_t Violinist::Perform(Violinist::Key key, Violinist::Mode mode, int interval_ms, float amplitude, short transpose) {
    auto *positions = new float[8];
//    float amplitude = 0.5;
    auto err = GetPositionsForScale(positions, key, mode, transpose);
    if (err != kNoError)
        goto out;

    err = m_fingerController->Rest();
    err = m_bowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        goto out;

    for (int i=0; i < 8; i++) {
        m_pfFretPosition = positions[i];
        if (m_pfFretPosition >= 1)
            err = m_fingerController->On();
        else
            err = m_fingerController->Off();
        m_bowController->SetAmplitude(amplitude, err);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
//        m_bowController->ChangeDirection();
    }
    m_bowController->SetAmplitude(0.2, kNoError);
    m_bowController->ChangeDirection();
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

    for (int i=7; i > -1; i--) {
        m_pfFretPosition = positions[i];
        if (m_pfFretPosition >= 1)
            err = m_fingerController->On();
        else
            err = m_fingerController->Off();
        m_bowController->SetAmplitude(amplitude, err);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
//        m_bowController->ChangeDirection();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

    err = m_bowController->StopBowing(err);
    if (err != kNoError)
        goto out;

    err = m_fingerController->Rest();

    out:
    delete[] positions;
    return err;
}

Error_t Violinist::GetPositionsForScale(float *positions, Violinist::Key key, Violinist::Mode mode, short transpose) {
    switch (mode) {
        case Major:
//            positions = new float[8] {0, 2, 4, 5, 7, 9, 11, 12};
            positions[0] = 0;
            positions[1] = 2;
            positions[2] = 4;
            positions[3] = 5;
            positions[4] = 7;
            positions[5] = 9;
            positions[6] = 11;
            positions[7] = 12;
            break;

        case Minor:
//            positions = new float[8] {0, 2, 3, 5, 7, 8, 10, 12};
            positions[0] = 0;
            positions[1] = 2;
            positions[2] = 3;
            positions[3] = 5;
            positions[4] = 7;
            positions[5] = 8;
            positions[6] = 10;
            positions[7] = 12;
            break;

        case Lydian:
//            positions = new float[8] {0, 2, 4, 6, 7, 9, 11, 12};
            positions[0] = 0;
            positions[1] = 2;
            positions[2] = 4;
            positions[3] = 6;
            positions[4] = 7;
            positions[5] = 9;
            positions[6] = 11;
            positions[7] = 12;
            break;

        case Dorian:
//            positions = new float[8] {0, 2, 3, 5, 7, 9, 10, 12};
            positions[0] = 0;
            positions[1] = 2;
            positions[2] = 3;
            positions[3] = 5;
            positions[4] = 7;
            positions[5] = 9;
            positions[6] = 10;
            positions[7] = 12;
            break;

        default:
            return kFunctionIllegalCallError;
    }

    for (int i=0; i<8; i++) {
        positions[i] += (key + transpose);
    }
    return kNoError;
}


