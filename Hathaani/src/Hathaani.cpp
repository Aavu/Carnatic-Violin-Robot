//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#include "Hathaani.h"

Hathaani::Hathaani(): m_iRTPosition(0),
                        ulErrorCode(0),
                        m_bStopPositionUpdates(false),
                        m_pCommHandler(nullptr),
                        m_pBowController(nullptr),
                        m_pFingerController(nullptr),
                        m_pCTuner(nullptr)
{
}

Hathaani::~Hathaani() {
    if (m_pCTuner) m_pCTuner->Stop();

    m_bStopPositionUpdates = true;

    if (positionTrackThread.joinable())
        positionTrackThread.join();

    if (m_pCTuner)
        if (m_bTunerOn) {
            m_pCTuner->Reset();
            CTuner::Destroy(m_pCTuner);
        }

    delete m_pFingerController;
    BowController::Destroy(m_pBowController);
    delete m_pCommHandler;
}

Error_t Hathaani::Init(bool shouldHome, bool usePitchCorrection) {
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

    m_bStopPositionUpdates = false;
    positionTrackThread = std::thread(&Hathaani::TrackTargetPosition, this);

    return kNoError;
}

void Hathaani::LogInfo(const string &message) {
    cout << message << endl;
}

void Hathaani::LogError(const string &functionName, Error_t p_lResult, unsigned int p_ulErrorCode) {
    cerr << kName << ": " << functionName << " failed (result=" << CUtil::GetErrorString(p_lResult) << ", errorCode=0x" << std::hex << p_ulErrorCode << ")" << endl;
}

Error_t Hathaani::UpdateTargetPosition() {
    long targetPosition = Util::Fret2Position(m_fFretPosition);
    return m_pFingerController->MoveToPositionl(targetPosition);
}

void Hathaani::TrackTargetPosition() {
    while (!m_bStopPositionUpdates)
    {
//            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_iTimeInterval);
        if (UpdateTargetPosition() != kNoError)
            break;
//            std::this_thread::sleep_until(x);
//            cout << GetPosition() << " " << ConvertToTargetPosition(m_pfFretPosition) << endl;
    }
}

Error_t Hathaani::Perform(const std::vector<float>& pitches, const std::vector<size_t>& bowChange, float amplitude, int8_t transpose) {
//Error_t Hathaani::Perform(const double *pitches, const size_t& length, const std::vector<size_t>& bowChange, float amplitude, int8_t transpose) {
    auto err = m_pFingerController->SetPositionProfile(4000, 25000);
//    auto err = m_pFingerController->ActivatePositionMode();
    if (err != kNoError)
        return err;

    m_pFingerController->Rest();

    // Go to first note's position
    m_fFretPosition = pitches[0] + transpose;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    size_t bowIdx = 0;
    for (size_t i=0; i < pitches.size(); i++) {
        if (bowIdx < bowChange.size())
            if (bowChange[bowIdx] == i) {
                m_pBowController->ChangeDirection();
                m_pBowController->SetAmplitude(amplitude, kNoError);
                bowIdx++;
            }
        float p = pitches[i] + transpose;
//        std::cout << p << std::endl;
        if (p >= 0.0) {
            m_pFingerController->On();
            m_fFretPosition = p;
        } else {
//            m_fingerController->Off();
        }
        std::this_thread::sleep_for(std::chrono::microseconds(5500)); // number depends on hopsize of pitch track
    }

    std::this_thread::sleep_for(std::chrono::seconds (1));

    m_pBowController->StopBowing(kNoError);
    return m_pFingerController->Rest();
}

Error_t Hathaani::Perform(const vector<float> &pitches, const vector<size_t> &bowChange, const vector<float> &amplitude, float maxAmplitude, int8_t transpose)
{
//    auto err = m_pFingerController->SetPositionProfile(4000, 20000);
    auto err = m_pFingerController->SetPositionProfile(4000, 30000);
//    auto err = m_pFingerController->SetPositionProfile(5000, 45000);
//    auto err = m_pFingerController->ActivatePositionMode();
    if (err != kNoError)
        return err;

    m_pFingerController->Rest();

    // Go to first note's position
    float firstPitch = -1;
    for (float pitch : pitches) {
        if (pitch > 0)
        {
            firstPitch = pitch;
            break;
        }

    }
    m_fFretPosition = firstPitch + transpose;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    err = m_pBowController->StartBowing(maxAmplitude, Bow::Down, err);
    if (err != kNoError)
        return err;
    size_t bowIdx = 0;
    for (size_t i=0; i < pitches.size(); i++) {
//        auto now = std::chrono::steady_clock::now();
        if (bowIdx < bowChange.size())
            if (bowChange[bowIdx] == i) {
                m_pBowController->ChangeDirection();
//                m_pBowController->SetAmplitude(maxAmplitude);
                bowIdx++;
            }

        if (i%50 == 0)
            m_pBowController->SetAmplitude(amplitude[i] * maxAmplitude);
        float p = pitches[i] + transpose;
//        std::cout << p << std::endl;
        if (pitches[i] >= 0.0) {
            m_pFingerController->On();
            m_fFretPosition = p;
        } else {
//            std::cout << pitches[i] << std::endl;
            m_pFingerController->Off();
        }
//        std::this_thread::sleep_until(now + std::chrono::microseconds(5500));
        std::this_thread::sleep_for(std::chrono::microseconds(5500)); // number depends on hopsize of pitch track
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_pBowController->StopBowing(kNoError);
    std::this_thread::sleep_for(std::chrono::seconds (1));
    return m_pFingerController->Rest();
}

void Hathaani::PitchCorrect() {
    while (!m_bInterruptPitchCorrection) {
        auto correction = m_pCTuner->GetNoteCorrection();
        m_fFretPosition += (PITCH_CORRECTION_FACTOR * correction);
//        std::cout << m_pfFretPosition << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

Error_t Hathaani::Perform(Hathaani::Key key, Hathaani::Mode mode, int interval_ms, float amplitude, short transpose) {
    auto err = m_pFingerController->ActivatePositionMode();
    if (err != kNoError)
        return err;

//    std::unique_ptr<float[]> positions = std::make_unique<float[]>(8); // new float[8];
    std::vector<float> positions(8);

    err = GetPositionsForScale(positions, key, mode, transpose);
    if (err != kNoError)
    {
        LogError("Perform", err, 0);
        return err;
    }

    int n = (int) positions.size();
    err = m_pFingerController->Rest();
    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;
    for (float position : positions) {
        m_fFretPosition = position;

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

    if (mode != Major_Ascend) {
        m_pBowController->PauseBowing();
        m_pBowController->ChangeDirection();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        m_pBowController->SetAmplitude(amplitude, err);
        for (int i = n - 1; i > -1; --i) {
            m_fFretPosition = positions[i];

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
    }
    m_pBowController->StopBowing(err);
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    return m_pFingerController->Rest();
}

Error_t Hathaani::GetPositionsForScale(std::vector<float>& positions, Hathaani::Key key, Hathaani::Mode mode, short transpose) {
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
            break;

        case Major_Ascend:
        {
            positions.resize(11);
            positions = {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17};
            break;
        }

        default:
            return kFunctionIllegalCallError;
    }

    for (float & position : positions) {
        position += (float)(key + transpose);
    }
    return kNoError;
}

Error_t Hathaani::Play(float amplitude) {
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

//Error_t Hathaani::Play(float amplitude) {
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

//Error_t Hathaani::ActivatePositionMode() {
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

//Error_t Hathaani::ActivateProfilePositionMode(unsigned int uiVelocity, unsigned int uiAcc) {
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

void Hathaani::SetupTuner() {
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

Error_t Hathaani::Perform(Hathaani::Gamaka gamaka, int exampleNumber, int interval_ms, float amplitude)
{
    switch (gamaka) {
        case Spurita:
            return PerformSpurita(exampleNumber, interval_ms, amplitude);

        case Jaaru:
            return PerformJaaru(exampleNumber, interval_ms, amplitude);

        case Nokku:
            return PerformNokku(exampleNumber, interval_ms, amplitude);

        case Kampita:
            return PerformKampita(exampleNumber, interval_ms, amplitude);

        case Odukkal:
            return PerformOdukkal(interval_ms, amplitude);

        case Orikkai:
            return PerformOrikkai(interval_ms, amplitude);

        case Khandippu:
            return PerformKhandippu(interval_ms, amplitude);

        default:
            return kUnknownError;
    }
}

Error_t Hathaani::PerformSpurita(int exampleNumber, int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(5000, 20000);
    if (err != kNoError)
        return err;

    vector<float> position;

    switch(exampleNumber) {
        case 0:
            position = {2, 1, 2, 4, 3, 4, 6, 5, 6, 8, 7, 8, 9};
            break;
        case 1:
            position = {2, 1, 2, 7, 6, 7, 6, 5, 6, 4, 3, 4, 2};
            break;
        case 2:
            position = {14, 13.5, 14, 12, 11.5, 12, 11, 10.5, 11, 9, 8.5, 9, 7};
            break;
        default:
            return kUnknownError;
    }

    vector<float> time          = {1, 0.05, 0.95, 1, 0.05, 0.95, 1, 0.05, 0.95, 1, 0.05, 0.95, 2};  // 13
    vector<uint8_t> bowChange   = {0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1}; // 13 (1st dummy = 0)

    m_pFingerController->Rest();

    // Go to first note's position
    err = m_pFingerController->MoveToPositionf(position[0]);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
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

Error_t Hathaani::PerformJaaru(int exampleNumber, int interval_ms, float amplitude)
{
    Error_t err = kNoError;
    vector<float> position;
    vector<float> time;
    vector<uint8_t> bowChange;

    switch(exampleNumber) {
        case 0:
            position  = {6, 4, 6, 6, 9, 9, 14, 11, 9, 14, 14};
            time      = {1, 0.1, 0.9, 0.1, 1.9, 2, 0.1, 0.9, 1, 2, 2};
            bowChange  = {0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1};

            err = m_pFingerController->SetPositionProfile(2500, 8000);
            if (err != kNoError)
                return err;
            break;

        case 1:
            position    = {9, 11.5, 9, 7,  6.5, 7, 7, 6,  7, 6, 7, 6,  7, 4}; // 16
            time        = {1.2, .1, .2, .4,  .1, .8, .8, .2,  .2, .2, .2, .2,  .6, 1};
            bowChange   = {0, 0, 1, 0,  0, 0, 1, 0,  1, 0, 0, 0,  0, 0};

            err = m_pFingerController->SetPositionProfile(5000, 20000);
            if (err != kNoError)
                return err;
            break;

        default:
            return kUnknownError;
    }

    m_pFingerController->Rest();

    // Go to first note's position
    err = m_pFingerController->MoveToPositionf(position[0]);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {

        if (bowChange[i]) {
            m_pBowController->ChangeDirection();
            m_pBowController->SetAmplitude(amplitude, kNoError);
        }

        if (i ==11 && exampleNumber == 1) {
            err = m_pFingerController->SetPositionProfile(2500, 2000);
            if (err != kNoError)
            {
                std::cerr << "Error : SetPositionProfile" << std::endl;
                return err;
            }
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

Error_t Hathaani::PerformNokku(int exampleNumber, int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(6000, 30000);
    if (err != kNoError)
        return err;

    vector<float> position;
    vector<float> time;
    vector<uint8_t> bowChange;

    switch(exampleNumber) {
        case 0:
            position    = {2, 7, 5, 7, 4, 2, 7, 4, 7, 9};
            time        = {1, .1, .5, .1, .8, 0.2, 0.3, 0.1, 0.9, 2};
            bowChange   = {0, 1, 0, 0, 1, 1, 0, 1, 0, 1};
            break;

        case 1:
            position    = {};
            time        = {};
            bowChange   = {};
            break;

        default:
            return kUnknownError;
    }

    m_pFingerController->Rest();

    // Go to first note's position
    err = m_pFingerController->MoveToPositionf(position[0]);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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

Error_t Hathaani::PerformOdukkal(int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(2500, 12000);
    if (err != kNoError)
        return err;

    vector<float> position      = {2, 2, 2.5, 2, 2.5, 9, 7, 9, 9.5, 9, 9, 9.5, 9, 9.5, 9};
    vector<float> time          = {1, .8, .2, .4, .2, .6, .2, .8, .1, .3, .8, .2, .6, .1, .3};
    vector<uint8_t> bowChange   = {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};

    m_pFingerController->Rest();
    // Go to first note's position
    err = m_pFingerController->MoveToPositionf(position[0]);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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

Error_t Hathaani::PerformOrikkai(int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(2500, 10000);
    if (err != kNoError)
        return err;

    vector<float> position      = {13, 14, 10, 13, 7, 10, 6, 7, 2, 3};
    vector<float> time          = {.8, .2, .8, .2, .8, .2, .8, .2, .4, .2};
    vector<uint8_t> bowChange   = {0, 0, 1, 0, 1, 0, 1, 0, 1, 0};

    m_pFingerController->Rest();

    // Go to first note's position
    err = m_pFingerController->MoveToPositionf(position[0]);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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

Error_t Hathaani::PerformKhandippu(int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(4000, 40000);
    if (err != kNoError)
        return err;

    vector<float> position  = {6, 9, 6, 4, 4.2, 2};
    vector<float> time      = {.8, 1, 0.1, 0.8, 0.05, 1};
    vector<uint8_t> bowChange  = {0, 1, 1, 0, 0, 1};

    m_pFingerController->Rest();

    // Go to first note's position
    err = m_pFingerController->MoveToPositionf(position[0]);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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

Error_t Hathaani::PerformKampita(int exampleNumber, int interval_ms, float amplitude)
{
    auto err = m_pFingerController->SetPositionProfile(2500, 20000);
    if (err != kNoError)
        return err;

    vector<float> position;
    vector<float> time;
    vector<uint8_t> bowChange;

    switch (exampleNumber) {
        case 0:
            position    = {6, 6.5, 6, 6.5, 6, 6.5, 6, 11, 9, 6, 4, 2};
            time        = {.7, .2, .6, .2, .5, .1, .3, .2, .6, .15, .85, 2};
            bowChange   = {0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1};
            break;

        case 1:
            position    = {6, 6.5, 6, 6.5, 6, 6.5, 4, 2, 1, 4, 2, 6, 4, 6, 6.5, 6, 6.5, 4};
            time        = {.8, .2, .6, .2, .4, .2, .2, .2, 1, 1, .3, .1, .2, .7, .1, .4, .1, .3};
            bowChange   = {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0};
            m_pFingerController->SetPositionProfile(4000, 30000);
            break;

        case 2:
            position    = {2, 6, 4, 6, 6.5, 6, 6, 6.5, 6, 6.5, 6, 6.5};
            time        = {.6, .2, .2, .6, .2, .6, .6, .2, .4, .1, .5, .05};
            bowChange   = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0};
            break;

        case 3:
            position    = {9, 9, 14, 11, 14, 11, 14, 11, 15, 14, 12, 10, 11.5, 9};
            time        = {1, .6, .6, .2, .6, .2, .05, .15, .2, .5, .2, .6, .1, .6};
            bowChange   = {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1};
            break;

        default:
            return kUnknownError;
    }

    m_pFingerController->Rest();

    // Go to first note's position
    err = m_pFingerController->MoveToPositionf(position[0]);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    err = m_pBowController->StartBowing(amplitude, Bow::Down, err);
    if (err != kNoError)
        return err;

    for (int i = 0; i < (int)position.size(); ++i) {
        if (position[i] < 0) { // Rests
            m_pBowController->PauseBowing();
            goto wait;
        }

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

        wait:
        auto t = (useconds_t)(time[i] * interval_ms * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(t));
    }

    m_pBowController->StopBowing(err);
    return m_pFingerController->Rest();;
}

Error_t Hathaani::ApplyRosin(int time)
{
    std::cout << "Apply Rosin Now!" << std::endl;
    for (int i=0; i < 10; ++i)
    {
        auto err = m_pBowController->RosinMode();
        if (err != kNoError) {
            return m_pBowController->StopBowing();
        }

        std::this_thread::sleep_for(std::chrono::seconds(time / 10));
        m_pBowController->ChangeDirection();
    }
    return m_pBowController->StopBowing();
}
