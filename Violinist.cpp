//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#include "Violinist.h"

Violinist::Violinist(): iRTPosition(0),
                        #ifdef USE_FINGER
                        finger(nullptr),
                        #endif
                        ulErrorCode(0),
                        b_stopPositionUpdates(false) {
    SetDefaultParameters();
}

Violinist::~Violinist() = default;

void Violinist::SetDefaultParameters() {
    //USB
    g_usNodeId = 6;
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

#ifdef USE_FINGER
        lResult = Finger::create(finger, &iRTPosition);
#endif
    } else {
        g_pKeyHandle = nullptr;
    }
    cout << lResult << endl;

    delete[] pDeviceName;
    delete[] pProtocolStackName;
    delete[] pInterfaceName;
    delete[] pPortName;

    return lResult;
}

Error_t Violinist::Prepare(float* fretPos, bool shouldHome) {
    p_fFretPosition = fretPos;

    Error_t lResult;
    BOOL oIsFault = 0;

    if ((lResult = OpenDevice()) != kNoError) {
        LogError("OpenDevice", lResult, ulErrorCode);
        return lResult;
    }

    if (VCS_GetFaultState(g_pKeyHandle, g_usNodeId, &oIsFault, &ulErrorCode) == 0) {
        LogError("VCS_GetFaultState", lResult, ulErrorCode);
        lResult = kGetValueError;
    }

    if (lResult == kNoError) {
        if (oIsFault) {
            stringstream msg;
            msg << "clear fault, node = '" << g_usNodeId << "'";
            LogInfo(msg.str());

            if (VCS_ClearFault(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
                LogError("VCS_ClearFault", lResult, ulErrorCode);
                lResult = kSetValueError;
            }
        }

        if (lResult == kNoError) {
            BOOL oIsEnabled = 0;

            if (VCS_GetEnableState(g_pKeyHandle, g_usNodeId, &oIsEnabled, &ulErrorCode) == 0) {
                LogError("VCS_GetEnableState", lResult, ulErrorCode);
                lResult = kGetValueError;
            }

            if (lResult == 0) {
                if (!oIsEnabled) {
                    if (VCS_SetEnableState(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
                        LogError("VCS_SetEnableState", lResult, ulErrorCode);
                        lResult = kSetValueError;
                    }
                }
            }
        }
    }

    stringstream msg;
    msg << "set position mode, node = " << g_usNodeId;
//    LogInfo(msg.str());

//    if (VCS_ActivateProfilePositionMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
//        lResult = kSetValueError;
//        LogError("VCS_ActivateProfilePositionMode", lResult, ulErrorCode);
//        return lResult;
//    }

    if (VCS_ActivatePositionMode(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
        lResult = kSetValueError;
        LogError("VCS_ActivatePositionMode", lResult, ulErrorCode);
        return lResult;
    }

//typedef unsigned short WORD;
//    WORD pHardwareVersion = 0;
//    WORD pSoftwareVersion = 0;
//    WORD pApplicationNumber = 0;
//    WORD pApplicationVersion = 0;
//
//    if (VCS_GetVersion(g_pKeyHandle, g_usNodeId, &pHardwareVersion, &pSoftwareVersion, &pApplicationNumber, &pApplicationVersion, &ulErrorCode) == 0) {
//        lResult = kGetValueError;
//        LogError("VCS_GetVersion", lResult, ulErrorCode);
//        return lResult;
//    }
//
//    cout << std::hex << pHardwareVersion << " " << pSoftwareVersion << " " << pApplicationNumber << " " << pApplicationVersion << endl;

    if (shouldHome) {
        lResult = SetHome();
        if (lResult != kNoError) {
            LogError("SetHome", lResult, ulErrorCode);
            return lResult;
        }
    }

    TrackEncoderPosition();
    TrackTargetPosition();

    if (VCS_SetMaxFollowingError(g_pKeyHandle, g_usNodeId, set_maxErr, &ulErrorCode) == 0) {
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

    if (set_maxErr != get_maxErr) {
        lResult = kSetValueError;
        LogError("MaxFollowingError", lResult, ulErrorCode);
    }

//    if (VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
//        lResult = kSetValueError;
//        LogError("VCS_SetDisableState", lResult, ulErrorCode);
//    }
    return lResult;
}

Error_t Violinist::CloseDevice() {
    Error_t err = kNoError;
    if (VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &ulErrorCode) == 0) {
        err = kSetValueError;
        LogError("VCS_SetDisableState", err, ulErrorCode);
    }

    b_stopPositionUpdates = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    LogInfo("Close device");
#ifdef USE_FINGER
    err = Finger::destroy(finger);
    if (err != kNoError) {
        LogError("Finger::destroy", err, ulErrorCode);
        return err;
    }
#endif

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

//Error_t Violinist::Play() {
//    Error_t lResult = kNoError;
//
//    vector<uint8_t> positionList = {2, 2, 6, 4, 6, 4, 6, 6, 7, 6, 7, 6, 7, 9, 9, 14, 11, 14, 11, 14, 13, 14,  //Arohanam
//                                    9, 14, 14, 16, 14, 13, 14, 14, 11, 9, 9, 6, 9, 7, 6, 6, 4, 6, 4, 2};
//    vector<float> timeList = {1.0, 0.5, 0.35, 0.15, .1, .1, .8, .3, 0.05, 0.3, 0.05, .05, 0.25, 1, 0.5, 0.35, 0.15, 0.15, 0.15, 0.65, 0.05, 1.0,  //Arohanam
//                              0.1, 0.9, 0.1, 0.1, .6, 0.1, 0.1, 0.2, 0.8, 1.0, 0.7, 0.15, 0.15, 0.5, 0.5, 0.7, 0.15, 0.15, 0.25, 0.75};
//
//    // Count Down
//    for (int i = 4; i > 0; i--) {
//        std::cout << i << std::endl;
//        usleep(1 * 1000000 / 1.25);
//    }
//
//    for (int i = 0; i < (int)positionList.size(); i++) {
//        long targetPosition = PositionToPulse(FretLength(positionList[i])) + NUT_POSITION;
//        stringstream msg;
//        msg << "move to position = " << targetPosition;
//        LogInfo(msg.str());
//
////        finger->on();
//        if (targetPosition > MAX_ENCODER_INC) {
//            if (VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, targetPosition, 1, 1, &ulErrorCode) == 0) {
//                LogError("VCS_MoveToPosition", lResult, ulErrorCode);
//                lResult = kSetValueError;
//                break;
//            }
//        }
//        auto t = (useconds_t)(timeList[i] * 1000000 / 1.25);
//        usleep(t);
//    }
//
////    finger->rest();
//
//    if (lResult != kNoError) {
//        LogError("play", lResult, ulErrorCode);
//    }
//
//    return lResult;
//}

Error_t Violinist::MoveToPosition(long targetPos) {
    Error_t lResult = kNoError;
//    unsigned int acc = GetAcceleration();
//    acc = 10000;
//    cout << targetPos << " " << iRTPosition << endl;
    if (std::abs(targetPos) < std::abs(MAX_ENCODER_INC) && std::abs(targetPos) > NUT_POSITION) {
//        if (VCS_SetPositionProfile(g_pKeyHandle, g_usNodeId, 7000, acc, acc, &ulErrorCode) == 0) {
//            lResult = kSetValueError;
//            LogError("VCS_SetPositionProfile", lResult, ulErrorCode);
//            return lResult;
//        }
//
//        if (VCS_MoveToPosition(g_pKeyHandle, g_usNodeId, targetPos, 1, 1, &ulErrorCode) == 0) {
//            lResult = kSetValueError;
//            LogError("VCS_MoveToPosition", lResult, ulErrorCode);
//            return lResult;
//        }

        if (VCS_SetPositionMust(g_pKeyHandle, g_usNodeId, targetPos, &ulErrorCode) == 0) {
            lResult = kSetValueError;
            LogError("VCS_SetPositionMust", lResult, ulErrorCode);
            return lResult;
        }

//        if (VCS_WaitForTargetReached(g_pKeyHandle, g_usNodeId, 20, &ulErrorCode) == 0) {
//            lResult = kSetValueError;
//            LogError("VCS_WaitForTargetReached", lResult, ulErrorCode);
//        }
    }
    return lResult;
}

unsigned int Violinist::GetAcceleration() {
    double mult = 4; //4;
    double s = std::abs(fPrevFretPosition - *p_fFretPosition);
    double t = iTimeInterval*0.001;
    int u = 0;
    VCS_GetVelocityIs(g_pKeyHandle, g_usNodeId, &u, &ulErrorCode);
//    int acc = std::ceil(mult*2*s/(t*t));
    int acc_u = std::ceil(mult*(2*s/(t*t) - (2*std::abs(u*0.001)/t)));
//    cout << "velocity: " << u << " acc_raw: " << acc << " acc_u: " << std::ceil(acc - (2*std::abs(u*0.001)/t)) << endl;
    return std::min(acc_u, 15000);
//    return std::min(std::ceil(mult*2*s/(t*t)), 14000.0); // s = ut + 0.5*at^2
}

double Violinist::FretLength(uint8_t fretNumber) {
    return (SCALE_LENGTH - (SCALE_LENGTH / pow(2, (fretNumber / 12.0))));
}

long Violinist::PositionToPulse(double p) {
    return (long)(-p * 24000.0 / 220.0);
}

Error_t Violinist::UpdateEncoderPosition() {
    if (VCS_GetPositionIs(g_pKeyHandle, g_usNodeId, &iRTPosition, &ulErrorCode) == 0)
        return kGetValueError;
#ifdef USE_FINGER
    finger->updatePosition();
#endif
//    cout << iRTPosition << endl;
    return kNoError;
}

Error_t Violinist::UpdateTargetPosition() {
    long targetPosition = convertToTargetPosition(*p_fFretPosition);
    if (*p_fFretPosition != fPrevFretPosition) {
        Error_t err = MoveToPosition(targetPosition);

        if (err != kNoError)
            return err;
        fPrevFretPosition = *p_fFretPosition;
    }
    return kNoError;
}

long Violinist::convertToTargetPosition(float fretPos) {
    return PositionToPulse(FretLength(fretPos)) + NUT_POSITION;
}

void Violinist::TrackEncoderPosition() {
    b_stopPositionUpdates = false;
//    std::thread([this]() {
//        while (!b_stopPositionUpdates)
//        {
//            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(iTimeInterval);
//            if (UpdateEncoderPosition() != kNoError)
//                break;
//            std::this_thread::sleep_until(x);
//        }
//    }).detach();
}

void Violinist::TrackTargetPosition() {
    b_stopPositionUpdates = false;
    std::thread([this]() {
        while (!b_stopPositionUpdates)
        {
            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(iTimeInterval);
            if (UpdateTargetPosition() != kNoError)
                break;
//            std::this_thread::sleep_until(x);
//            cout << GetPosition() << " " << convertToTargetPosition(*p_fFretPosition) << endl;
        }
    }).detach();
}

unsigned int Violinist::GetErrorCode() {
    return ulErrorCode;
}

int Violinist::GetPosition() {
    UpdateEncoderPosition();
    return iRTPosition;
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
        if (RawMoveToPosition(25, 1000, 0) != kNoError) {
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
//        LogInfo(msg.str());

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


