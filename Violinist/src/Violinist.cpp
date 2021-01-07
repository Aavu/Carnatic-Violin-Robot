//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#include "Violinist.h"

Violinist::Violinist(): m_iRTPosition(0),
                        ulErrorCode(0),
                        m_bStopPositionUpdates(false),
                        m_pCommHandler(nullptr),
                        m_pBowController(nullptr),
                        m_pFingerController(nullptr),
                        m_pCTuner(nullptr)
{
//    SetDefaultParameters();
}

Violinist::~Violinist() {
//    Error_t lResult;
//    if ((lResult = CloseDevice()) != kNoError) {
//        Violinist::LogError("CloseDevice", lResult, ulErrorCode);
//    }

    if (m_pCTuner) m_pCTuner->Stop();

    m_bStopPositionUpdates = true;

    if (positionTrackThread.joinable())
        positionTrackThread.join();

    if (encoderTrackThread.joinable())
        encoderTrackThread.join();

    if (m_pCTuner)
        if (m_bTunerOn) {
            m_pCTuner->Reset();
            CTuner::Destroy(m_pCTuner);
        }

    delete m_pFingerController;
    BowController::Destroy(m_pBowController);
    delete m_pCommHandler;
}

//void Violinist::SetDefaultParameters() {
//    //USB
//    g_usNodeId = 1;
//    g_deviceName = "EPOS4";
//    g_protocolStackName = "MAXON SERIAL V2";
//    g_interfaceName = "USB";
//    g_portName = "USB0";
//    g_baudrate = 1000000;
//}

//Error_t Violinist::OpenDevice() {
//    Error_t lResult = kNoError;
//    ulErrorCode = 0;
//    char* pDeviceName = new char[255];
//    char* pProtocolStackName = new char[255];
//    char* pInterfaceName = new char[255];
//    char* pPortName = new char[255];
//
//    strcpy(pDeviceName, g_deviceName.c_str());
//    strcpy(pProtocolStackName, g_protocolStackName.c_str());
//    strcpy(pInterfaceName, g_interfaceName.c_str());
//    strcpy(pPortName, g_portName.c_str());
//
//    LogInfo("Open device...");
//
//    g_pKeyHandle = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, &ulErrorCode);
//
//    if (g_pKeyHandle != nullptr && ulErrorCode == 0) {
//        unsigned int lBaudrate = 0;
//        unsigned int lTimeout = 0;
//
//        if (VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, &ulErrorCode) != 0) {
//            if (VCS_SetProtocolStackSettings(g_pKeyHandle, g_baudrate, lTimeout, &ulErrorCode) != 0) {
//                if (VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, &ulErrorCode) != 0) {
//                    if (g_baudrate != (int)lBaudrate) {
//                        lResult = kSetValueError;
//                        LogError("VCS_SetProtocolStackSettings", lResult, ulErrorCode);
//                        return lResult;
//                    }
//                }
//            }
//        }
//    } else {
//        g_pKeyHandle = nullptr;
//    }
//
//    delete[] pDeviceName;
//    delete[] pProtocolStackName;
//    delete[] pInterfaceName;
//    delete[] pPortName;
//
//    return lResult;
//}

Error_t Violinist::Init(bool shouldHome, bool usePitchCorrection) {
    Error_t lResult;
//    BOOL oIsFault = 0;
    m_bShouldHome = shouldHome;
    m_bUsePitchCorrection = usePitchCorrection;

    m_pCommHandler = new CommHandler(CommHandler::I2C);
    m_pFingerController = new FingerController(m_pCommHandler);

    if (!m_pFingerController->IsInitialized()) {
        lResult = kNotInitializedError;
        LogError("FingerInitError", lResult, ulErrorCode);
        return lResult;
    }

    if (shouldHome) {
        lResult = m_pFingerController->SetHome();
        if (lResult != kNoError) {
            LogError("SetHome", lResult, ulErrorCode);
            return lResult;
        }
    }

    if ((lResult = BowController::Create(m_pBowController)) != kNoError) {
        LogError("BowControllerCreationError", lResult, ulErrorCode);
        return lResult;
    }

    if ((lResult = m_pBowController->Init(m_pCommHandler, &m_iRTPosition)) != kNoError) {
        LogError("BowControllerInitError", lResult, ulErrorCode);
        return lResult;
    }

    // Tuner
    if (m_bUsePitchCorrection)
        SetupTuner();

//    TrackEncoderPosition();
    m_bStopPositionUpdates = false;
//    encoderTrackThread = std::thread(&Violinist::TrackEncoderPosition, this);
//    positionTrackThread = std::thread(&Violinist::TrackTargetPosition, this);

    return kNoError;
}

//Error_t Violinist::CloseDevice() {
//    Error_t err = kNoError;
//
//    if (VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
//        err = kSetValueError;
//        LogError("VCS_SetDisableState", err, ulErrorCode);
//    }
//
//    LogInfo("Close device");
//    if (VCS_CloseDevice(g_pKeyHandle, &ulErrorCode) == 0 && ulErrorCode != 0) {
//        err = kUnknownError;
//        LogError("VCS_CloseDevice", err, ulErrorCode);
//    }
//
//    return err;
//}

void Violinist::LogInfo(const string &message) {
    cout << message << endl;
}

void Violinist::LogError(const string &functionName, Error_t p_lResult, unsigned int p_ulErrorCode) {
    cerr << kName << ": " << functionName << " failed (result=" << CUtil::GetErrorString(p_lResult) << ", errorCode=0x" << std::hex << p_ulErrorCode << ")" << endl;
}

//Error_t Violinist::MoveToPosition(long targetPos) {
//    Error_t lResult = kNoError;
//    unsigned int errorCode = 0;
//
//    if  (m_operationMode == ProfilePosition) {
//        if (VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, targetPos, 1, 1, &errorCode) == 0) {
//            lResult = kSetValueError;
//            LogError("VCS_MoveToPosition", lResult, errorCode);
//            return lResult;
//        }
//        return lResult;
//    }
//
//    if (std::abs(targetPos) < std::abs(MAX_ENCODER_INC) && std::abs(targetPos) >= std::abs(NUT_POSITION)) {
//        m_pFingerController->UpdatePosition(std::abs(targetPos));
//        if (VCS_SetPositionMust(g_pKeyHandle, g_usNodeId, targetPos, &ulErrorCode) == 0) {
//            lResult = kSetValueError;
//            LogError("VCS_SetPositionMust", lResult, ulErrorCode);
//        }
////        std::cout << targetPos << std::endl;
//    }
//    return lResult;
//}

//double Violinist::FretLength(double fretNumber) {
//    return (SCALE_LENGTH - (SCALE_LENGTH / pow(2, (fretNumber / 12.0))));
//}

//long Violinist::PositionToPulse(double p) const {
//    return (long)(m_iEncoderDirection * p * 24000.0 / 220.0);
//}

Error_t Violinist::UpdateEncoderPosition() {
//    if (VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &m_iRTPosition, &ulErrorCode) == 0) {
//        std::cerr << "get value error...\n";
//        return kGetValueError;
//    }

//    m_pFingerController->UpdatePosition(m_iRTPosition);
//    cout << m_iRTPosition << endl;
    return kNoError;
}

Error_t Violinist::UpdateTargetPosition() {
    long targetPosition = Util::Fret2Position(m_fFretPosition);
    return m_pFingerController->MoveToPositionl(targetPosition);
}

//long Violinist::ConvertToTargetPosition(double fretPos) {
//    return PositionToPulse(FretLength(fretPos)) + NUT_POSITION;
//}

void Violinist::TrackEncoderPosition() {
    m_bStopPositionUpdates = false;
    while (!m_bStopPositionUpdates)
    {
        auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_iTimeInterval);
        if (UpdateEncoderPosition() != kNoError)
            break;
        std::this_thread::sleep_until(x);
    }
}

void Violinist::TrackTargetPosition() {
    while (!m_bStopPositionUpdates)
    {
//            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_iTimeInterval);
        if (UpdateTargetPosition() != kNoError)
            break;
//            std::this_thread::sleep_until(x);
//            cout << GetPosition() << " " << ConvertToTargetPosition(m_pfFretPosition) << endl;
    }
}

//unsigned int Violinist::GetErrorCode() const {
//    return ulErrorCode;
//}

//int Violinist::GetPosition() {
//    UpdateEncoderPosition();
//    return m_iRTPosition;
//}

//Error_t Violinist::SetHome() {
//    LogInfo("Setting Home..");
//    Error_t lResult = kNoError;
//    int _lastPosition = 10000;
//    int _currentPosition = -10000;
//    unsigned int errorCode = 0;
//    if (VCS_ActivateProfilePositionMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
//        lResult = kSetValueError;
//        LogError("VCS_ActivateProfilePositionMode", lResult, ulErrorCode);
//        return lResult;
//    }
//    while (true) {
//        if (RawMoveToPosition(-25*m_iEncoderDirection, 1000, 0) != kNoError) {
//            lResult = kSetValueError;
//            LogError("moveToPosition", lResult, errorCode);
//            return lResult;
//        }
//        if (VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &_currentPosition, &errorCode) == 0) {
//            lResult = kGetValueError;
//            LogError("VCS_GetPositionIs", lResult, errorCode);
//            return lResult;
//        }
//        if (abs(_currentPosition - _lastPosition) < 10)
//            break;
////        cout << "moving" << endl;
//        _lastPosition = _currentPosition;
//    }
//    if (VCS_DefinePosition(g_pKeyHandle, g_usNodeId, 0, &errorCode) == 0) {
//        lResult = kSetValueError;
//        LogError("VCS_DefinePosition", lResult, errorCode);
//    }
//    return lResult;
//}

//Error_t Violinist::RawMoveToPosition(int _pos, unsigned int _acc, BOOL _absolute) {
//    Error_t lResult = kNoError;
//    unsigned int errorCode = 0;
//
//    stringstream msg;
//    msg << "move to position = " << _pos << ", acc = " << _acc;
//    LogInfo(msg.str());
//
//    if (VCS_SetPositionProfile(g_pKeyHandle, g_usNodeId, 2500, _acc, _acc, &errorCode) == 0) {
//        lResult = kSetValueError;
//        LogError("VCS_SetPositionProfile", lResult, errorCode);
//        return lResult;
//    }
//
//    if (VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, _pos, _absolute, 1, &errorCode) == 0) {
//        lResult = kSetValueError;
//        LogError("VCS_MoveToPosition", lResult, errorCode);
//        return lResult;
//    }
//
//    if (VCS_WaitForTargetReached(g_pKeyHandle, g_usNodeId, 50, &errorCode) == 0) {
//        lResult = kGetValueError;
//        LogError("VCS_WaitForTargetReached", lResult, errorCode);
//    }
//    return lResult;
//}

Error_t Violinist::Perform(const double *pitches, const size_t& length, short transpose) {
    auto err = kNoError;
    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(0.7, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (size_t i=0; i < length; i++) {
        if (pitches[i] >= 0.0) {
            m_pFingerController->On();
            m_fFretPosition = pitches[i] + transpose;
        } else {
//            m_fingerController->Off();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(11)); // number depends on hopsize of pitch track
    }

    std::this_thread::sleep_for(std::chrono::seconds (1));

    err = m_pBowController->StopBowing(kNoError);
    if (err != kNoError)
        return err;

    err = m_pFingerController->Rest();
    return err;
}

void Violinist::PitchCorrect() {
    while (!m_bInterruptPitchCorrection) {
        auto correction = m_pCTuner->GetNoteCorrection();
        m_fFretPosition += (PITCH_CORRECTION_FACTOR * correction);
//        std::cout << m_pfFretPosition << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

Error_t Violinist::Perform(Violinist::Key key, Violinist::Mode mode, int interval_ms, float amplitude, short transpose) {
    auto err = m_pFingerController->ActivatePositionMode();
    if (err != kNoError)
        return err;

//    std::unique_ptr<float[]> positions = std::make_unique<float[]>(8); // new float[8];
    std::vector<float> positions(8);

    err = GetPositionsForScale(positions, key, mode, transpose);
    if (err != kNoError)
        return err;

    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i=0; i < 8; i++) {
        m_fFretPosition = positions[i];
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);

        if (m_fFretPosition >= .5) {
            err = m_pFingerController->On();
//            std::thread([this] {
//                m_bInterruptPitchCorrection = false;
//                PitchCorrect();
//            }).detach();
        } else {
            err = m_pFingerController->Off();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        m_bInterruptPitchCorrection = true;
        m_pBowController->ChangeDirection();
        m_pBowController->SetAmplitude(amplitude, err);
    }

    m_pBowController->SetAmplitude(0.2, kNoError);
    m_pBowController->ChangeDirection();
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    m_pBowController->SetAmplitude(amplitude, err);

    for (int i=7; i > -1; i--) {
        m_fFretPosition = positions[i];
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);

        if (m_fFretPosition >= .5) {
            err = m_pFingerController->On();
//            std::thread([this] {
//                m_bInterruptPitchCorrection = false;
//                PitchCorrect();
//            }).detach();
        } else {
            err = m_pFingerController->Off();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        m_bInterruptPitchCorrection = true;
        m_pBowController->ChangeDirection();
        m_pBowController->SetAmplitude(amplitude, err);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

    m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();
}

Error_t Violinist::GetPositionsForScale(std::vector<float>& positions, Violinist::Key key, Violinist::Mode mode, short transpose) {
    switch (mode) {
        case Major:
            positions = {0, 2, 4, 5, 7, 9, 11, 12};
            break;

        case Minor:
            positions = {0, 2, 3, 5, 7, 8, 10, 12};
            break;

        case Lydian:
            positions = {0, 2, 4, 6, 7, 9, 11, 12};
            break;

        case Dorian:
            positions = {0, 2, 3, 5, 7, 9, 10, 12};
            break;

        case SingleNote:
            positions = {0, 0, 0, 0, 0, 0, 0, 0};

        default:
            return kFunctionIllegalCallError;
    }

    for (int i=0; i<8; i++) {
        positions[i] += (key + transpose);
    }
    return kNoError;
}

Error_t Violinist::Play(float amplitude) {
    auto err = m_pFingerController->ActivateProfilePositionMode();
    if (err != kNoError)
        return err;

    vector<float> positionArohanam      = {2, 2, 6, 4, 6, 4, 6, 6, 7, 6, 7, 6, 7, 9, 9, 14, 11, 14, 11, 14, 13, 14};  // 22
    vector<float> positionAvarohanam    = {9, 14, 14, 16, 14, 13, 14, 14, 11, 9, 9, 6, 9, 7, 6, 6, 4, 6, 4, 2}; // 20

    vector<float> timeArohanam          = {1.0, 0.5, 0.35, 0.15, .1, .1, .8, .3, 0.05, 0.3, 0.05, .05, 0.25, 1, 0.5, 0.35, 0.15, 0.15, 0.15, 0.65, 0.05, 1.0};  // 22
    vector<float> timeAvarohanam        = {0.1, 0.9, 0.1, 0.1, .6, 0.1, 0.1, 0.2, 0.8, 1.0, 0.7, 0.15, 0.15, 0.5, 0.5, 0.7, 0.15, 0.15, 0.25, 1};

    vector<bool> bowChangeArohanam      = {1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0}; // 23 (1 dummy)
    vector<bool> bowChangeAvarohanam    = {1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0}; // 21 (1 dummy)

    err = m_pFingerController->Rest();

    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)positionArohanam.size(); i++) {
        m_fFretPosition = positionArohanam[i] - 1;
        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();
        stringstream msg;
        msg << "move to position = " << m_fFretPosition;
//        LogInfo(msg.str());
        msg.str(std::string());

        auto t = (useconds_t)(timeArohanam[i] * 1000000 / 1.25);
        std::this_thread::sleep_for(std::chrono::microseconds(t));

        if (i == 12) { // Pa
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }
    }

    m_pBowController->SetAmplitude(0, kNoError);
    m_pBowController->ChangeDirection();
    m_pBowController->SetAmplitude(amplitude, kNoError);

    for (int i = 0; i < (int)positionAvarohanam.size(); i++) {
        m_fFretPosition = positionAvarohanam[i] - 1;
        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();
        stringstream msg;
        msg << "move to position = " << m_fFretPosition;
//        LogInfo(msg.str());
        msg.str(std::string());

        auto t = (useconds_t)(timeAvarohanam[i] * 1000000 / 1.25);
        std::this_thread::sleep_for(std::chrono::microseconds(t));

        if (i == 9) { // Ma
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }
    }


    err = m_pBowController->StopBowing(err);
    if (err != kNoError)
        return err;

    return m_pFingerController->Rest();
}

//Error_t Violinist::Play(float amplitude) {
//    auto err = ActivateProfilePositionMode();
//    if (err != kNoError)
//        return err;
//
//    vector<float> position  = {0, 0, 2, 0, 5, 4, 0, 0, 2, 0, 7, 5, 0, 0, 12, 9, 5, 4, 2, 10, 10, 9, 5, 7, 5};  // 25
//    vector<float> time      = {.25, .25, .5, .5, .5, 1, .25, .25, .5, .5, .5, 1, .25, .25, .5, .5, .5, .5, 1, .25, .25, .5, .5, .5, 1}; // 25
//    vector<bool> bowChange  = {1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}; // 23 (1 dummy)
//
//    err = m_fingerController->Rest();
//
//    err = m_bowController->StartBowing(amplitude, Bow::Down, err);
//    if (err != kNoError)
//        return err;
//
//    for (int i = 0; i < (int)position.size(); i++) {
//        if (position[i] >= 1) {
//            m_pfFretPosition = position[i];
//            err = m_fingerController->On();
//        } else {
//            err = m_fingerController->Off();
//        }
//        stringstream msg;
//        msg << "move to position = " << m_pfFretPosition;
////        LogInfo(msg.str());
//        msg.str(std::string());
//
//        auto t = (useconds_t)(time[i] * 1000000);
//        std::this_thread::sleep_for(std::chrono::microseconds(t));
//
//        if (bowChange[i+1])
//            m_bowController->ChangeDirection();
//        m_bowController->SetAmplitude(amplitude, kNoError);
//    }
//
//    err = m_bowController->StopBowing(err);
//    if (err != kNoError)
//        return err;
//
//    return m_fingerController->Rest();
//}

//Error_t Violinist::ActivatePositionMode() {
//    stringstream msg;
//    msg << "set position mode, node = " << g_usNodeId;
//    LogInfo(msg.str());
//    msg.str(std::string());
//
//    auto err = kNoError;
//    if (VCS_ActivatePositionMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
//        err = kSetValueError;
//        LogError("VCS_ActivatePositionMode", err, ulErrorCode);
//        return err;
//    }
//
//    if (VCS_SetMaxFollowingError(g_pKeyHandle, g_usNodeId, m_ulMaxFollowErr, &ulErrorCode) == 0) {
//        err = kGetValueError;
//        LogError("VCS_GetMaxFollowingError", err, ulErrorCode);
//        return err;
//    }
//
//    unsigned int get_maxErr = 0;
//
//    if (VCS_GetMaxFollowingError(g_pKeyHandle, g_usNodeId, &get_maxErr, &ulErrorCode) == 0) {
//        err = kGetValueError;
//        LogError("VCS_GetMaxFollowingError", err, ulErrorCode);
//        return err;
//    }
//
//    if (m_ulMaxFollowErr != get_maxErr) {
//        err = kSetValueError;
//        LogError("MaxFollowingError", err, ulErrorCode);
//        return err;
//    }
//
//    m_operationMode = Position;
//    return kNoError;
//}

//Error_t Violinist::ActivateProfilePositionMode(unsigned int uiVelocity, unsigned int uiAcc) {
//    auto lResult = kNoError;
//    unsigned int lErrorCode = 0;
//    stringstream msg;
//    msg << "set profile position mode, node = " << g_usNodeId;
//    LogInfo(msg.str());
//    msg.str(std::string());
//
//    if (VCS_ActivateProfilePositionMode(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0) {
//        lResult = kSetValueError;
//        LogError("VCS_ActivateProfilePositionMode", lResult, lErrorCode);
//        return lResult;
//    }
//
//    m_operationMode = ProfilePosition;
//    return kNoError;
//}

void Violinist::SetupTuner() {
    CTuner::Create(m_pCTuner);

    if (m_pCTuner->Init(&m_fFretPosition) != kNoError) {
        LogInfo("Tuner init failed. Tuner will be switched off...");
        m_bTunerOn = false;
        CTuner::Destroy(m_pCTuner);
        m_pCTuner = nullptr;
        return;
    }

    if (m_pCTuner->Start() != kNoError) {
        LogInfo("Tuner Start failed. Tuner will be switched off...");
        m_bTunerOn = false;
        m_pCTuner->Reset();
        CTuner::Destroy(m_pCTuner);
        m_pCTuner = nullptr;
        return;
    }
}

Error_t Violinist::Perform(Violinist::Gamaka gamaka, int exampleNumber, int interval_ms, float amplitude)
{
    switch (gamaka) {
        case Spurita:
            return PerformSpurita(exampleNumber, interval_ms, amplitude);

        case Jaaru:
            return PerformJaaru(interval_ms, amplitude);

        case Nokku:
            return PerformNokku(interval_ms, amplitude);

        case Kampita:
            return PerformKampita(exampleNumber, interval_ms, amplitude);

        case Odukkal:
            return PerformOdukkal(interval_ms, amplitude);

        case Orikkai:
            return PerformOrikkai(interval_ms, amplitude);

        case Khandimpu:
            return PerformKhandimpu(interval_ms, amplitude);

        case Vali:
            return PerformVali(interval_ms, amplitude);

        default:
            return kUnknownError;
    }
}

Error_t Violinist::PerformSpurita(int exampleNumber, int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(5000, 20000);
    if (err != kNoError)
        return err;

    vector<float> position;

    switch(exampleNumber) {
        case 0:
            position = {2, 1.5, 2, 4, 3.5, 4, 6, 5.5, 6, 8, 7.5, 8, 9};
            break;
        case 1:
            position = {2, 1.5, 2, 7, 6.5, 7, 6, 5.5, 6, 4, 3.5, 4, 2};
            break;
        default:
            return kUnknownError;
    }

    vector<float> time          = {1, 0.05, 0.95, 1, 0.05, 0.95, 1, 0.05, 0.95, 1, 0.05, 0.95, 2};  // 13
//    vector<uint8_t> bowChange  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 13 (1st dummy = 0)
    vector<uint8_t> bowChange   = {0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1}; // 13 (1st dummy = 0)

    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {
        if (bowChange[i]) {
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }

        m_fFretPosition = position[i] - 1;
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);
        if (err != kNoError) {
            return m_pFingerController->Rest();;
        }

        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();

        if (err != kNoError) {
            return err;
        }

        auto t = (useconds_t)(time[i] * interval_ms * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();
}

Error_t Violinist::PerformJaaru(int interval_ms, float amplitude)
{
    auto err = m_pFingerController->ActivateProfilePositionMode();
    if (err != kNoError)
        return err;

    vector<float> position  = {6, 4, 6, 6, 9, 9, 14, 11, 9, 14, 14};  // 11
    vector<float> time      = {1, 0.1, 0.9, 0.1, 1.9, 2, 0.1, 0.9, 1, 2, 2};  // 11
    vector<uint8_t> bowChange  = {0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1}; // 11 (1st dummy = 0)

    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {
        if (bowChange[i]) {
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }

        m_fFretPosition = position[i] - 1;
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);
        if (err != kNoError) {
            return m_pFingerController->Rest();;
        }

        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();

        if (err != kNoError) {
            return err;
        }

        auto t = (useconds_t)(time[i] * interval_ms * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();
}

Error_t Violinist::PerformNokku(int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(2500, 12000);
    if (err != kNoError)
        return err;

    vector<float> position  = {2, 2, 7, 4, 7, 9};  // 6
    vector<float> time      = {1, 0.8, 0.2, 0.2, 0.8, 2};  // 6
    vector<uint8_t> bowChange  = {0, 1, 0, 1, 0, 1}; // 6 (1st dummy = 0)

    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {
        if (bowChange[i]) {
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }

        m_fFretPosition = position[i] - 1;
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);

        if (err != kNoError)
            return m_pFingerController->Rest();

        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();

        if (err != kNoError) {
            return err;
        }

        auto t = (useconds_t)(time[i] * interval_ms * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    err = m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();
}

Error_t Violinist::PerformOdukkal(int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(2500, 10000);
    if (err != kNoError)
        return err;

    vector<float> position      = {2, 2, 2.5, 2, 3, 9, 7, 9, 9.5, 9, 9, 9.5, 9, 9.5, 9};
    vector<float> time          = {1, .8, .1, .1, .2, .6, .2, .8, .1, .1, .8, .2, .6, .1, .1};
    vector<uint8_t> bowChange   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {
        if (bowChange[i] == 1) {
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }

        m_fFretPosition = position[i] - 1;
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);
        if (err != kNoError)
            return m_pFingerController->Rest();

        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();

        if (err != kNoError) {
            return err;
        }

        auto t = (useconds_t)(time[i] * interval_ms * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();
}

Error_t Violinist::PerformOrikkai(int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(2500, 10000);
    if (err != kNoError)
        return err;

    vector<float> position      = {14, 13, 14, 10, 13, 7, 8, 6, 7, 2, 3, 2};
    vector<float> time          = {2, .7, .3, .7, .3, .7, .3, .7, .3, .7, .2, .1};
    vector<uint8_t> bowChange   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {
        if (bowChange[i]) {
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }

        m_fFretPosition = position[i] - 1;
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);
        if (err != kNoError)
            return m_pFingerController->Rest();

        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();

        if (err != kNoError) {
            return err;
        }

        auto t = (useconds_t)(time[i] * interval_ms * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();
}

Error_t Violinist::PerformKhandimpu(int interval_ms, float amplitude)
{
    auto err = m_pFingerController->ActivateProfilePositionMode();
    if (err != kNoError)
        return err;

    vector<float> position  = {6, 9, 6, 4, 4.2, 2};
    vector<float> time      = {1, 1, 0.25, 0.75, 0.05, 0.95};
    vector<uint8_t> bowChange  = {0, 1, 1, 0, 0, 1};

    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {
        if (bowChange[i]) {
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }

        m_fFretPosition = position[i] - 1;
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);
        if (err != kNoError)
            return m_pFingerController->Rest();

        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();

        if (err != kNoError) {
            return err;
        }

        auto t = (useconds_t)(time[i] * interval_ms * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();
}

Error_t Violinist::PerformVali(int interval_ms, float amplitude)
{
    return kNoError;
}

Error_t Violinist::PerformKampita(int exampleNumber, int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(2500, 15000);
    if (err != kNoError)
        return err;

    vector<float> position;
    vector<float> time;
    vector<uint8_t> bowChange;

    switch (exampleNumber) {
        case 0:
            position    = {6, 6.5, 6, 6.5, 6, 6, 11, 9, 6, 4, 2};
            time        = {.6, .2, .6, .2, .6, .3, .1, .6, .2, .8, 2};
            bowChange   = {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1};
            break;

        case 1:
            position    = {6, 6.5, 6, 6.5, 6, 4, 2, 1, 4, 2, 4, 6, 6.5, 6, 6.5, 4};
            time        = {.6, .2, .6, .2, .6, .2, .2, 1, 1, .2, .2, .6, .2, .6, .2, .5};
            bowChange   = {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0};
            break;

        case 2:
            position    = {2, 6, 4, 6, 6.5, 6, 6, 6.5, 6, 6.5, 6};
            time        = {.6, .2, .2, .6, .2, .6, .6, .2, .4, .1, .4};
            bowChange   = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};
            break;

        case 3:
            position    = {9, 9, 14, 11, 14, 11, 14, 11, 16, 14, 12, 10, 12, 9};
            time        = {1, .6, .6, .2, .6, .2, .2, .2, .2, .6, .2, .4, .2, .6};
            bowChange   = {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1};
            break;

        default:
            return kUnknownError;
    }

    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {
        if (bowChange[i] == 1) {
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }

        m_fFretPosition = position[i] - 1;
        err = m_pFingerController->MoveToPositionf(m_fFretPosition);
        if (err != kNoError)
            return m_pFingerController->Rest();

        if (m_fFretPosition >= 1)
            err = m_pFingerController->On();
        else
            err = m_pFingerController->Off();

        if (err != kNoError) {
            return err;
        }

        auto t = (useconds_t)(time[i] * interval_ms * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();
}


