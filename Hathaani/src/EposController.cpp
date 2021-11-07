//
// Created by Raghavasimhan Sankaranarayanan on 1/4/21.
//

#include "EposController.h"

#include <utility>

EposController::EposController(std::string portName, int iNodeID) : m_iNodeID(iNodeID),
                                                                    m_strPortName(std::move(portName))
{
}

EposController::~EposController()
{
    CloseDevice();
}

Error_t EposController::Init(OperationMode mode)
{
#ifdef __arm__
    BOOL oIsFault = 0;
    unsigned int errorCode = 0;

    Error_t err;
    if ((err = OpenDevice()) != kNoError)
    {
        std::cerr << "OpenDevice" << std::endl;
        return err;
    }

    if (VCS_GetFaultState(pKeyHandle, m_iNodeID, &oIsFault, &errorCode) == 0) {
        std::cerr << "VCS_GetFaultState" << std::endl;
        return kGetValueError;
    }

    if (oIsFault) {
        if (VCS_ClearFault(pKeyHandle, m_iNodeID, &errorCode) == 0) {
            std::cerr << "VCS_ClearFault" << std::endl;
            return kSetValueError;
        }
    }

    BOOL oIsEnabled = 0;
    if (VCS_GetEnableState(pKeyHandle, m_iNodeID, &oIsEnabled, &errorCode) == 0) {
        std::cerr << "VCS_GetEnableState" << std::endl;
        return kGetValueError;
    }

    if (!oIsEnabled) {
        if (VCS_SetEnableState(pKeyHandle, m_iNodeID, &errorCode) == 0) {
            std::cerr << "VCS_SetEnableState" << std::endl;
            return kSetValueError;
        }
    }

    m_operationMode = mode;
    switch (m_operationMode) {
        case Position:
            ActivatePositionMode();
            break;
        case ProfilePosition:
            ActivateProfilePositionMode();
            break;
        case ProfileVelocity:
            ActivateProfileVelocityMode();
            break;
    }

#endif //__arm__
    return kNoError;
}

Error_t EposController::OpenDevice()
{
#ifdef __arm__
    int lResult = MMC_FAILED;
    unsigned int errorCode = 0;
    char *pDeviceName = new char[255];
    char *pProtocolStackName = new char[255];
    char *pInterfaceName = new char[255];
    char *pPortName = new char[255];

    strcpy(pDeviceName, m_strDeviceName.c_str());
    strcpy(pProtocolStackName, m_strProtocolStackName.c_str());
    strcpy(pInterfaceName, m_strInterfaceName.c_str());
    strcpy(pPortName, m_strPortName.c_str());

//    std::cout << "Opening device " << m_strPortName << "..." << std::endl;
    pKeyHandle = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, &errorCode);

    if (pKeyHandle != nullptr && errorCode == 0) {
        unsigned int lBaudrate = 0;
        unsigned int lTimeout = 0;

        if (VCS_GetProtocolStackSettings(pKeyHandle, &lBaudrate, &lTimeout, &errorCode) != 0) {
            if (VCS_SetProtocolStackSettings(pKeyHandle, m_uiBaudrate, lTimeout, &errorCode) != 0) {
                if (VCS_GetProtocolStackSettings(pKeyHandle, &lBaudrate, &lTimeout, &errorCode) != 0) {
                    if (m_uiBaudrate == lBaudrate) {
                        lResult = MMC_SUCCESS;
                    }
                }
            }
        }
    } else {
        pKeyHandle = nullptr;
    }

    delete[]pDeviceName;
    delete[]pProtocolStackName;
    delete[]pInterfaceName;
    delete[]pPortName;

    if (lResult != MMC_SUCCESS)
        return kNotInitializedError;
#endif // __arm__
    return kNoError;
}

Error_t EposController::CloseDevice()
{
#ifdef __arm__
    unsigned int errorCode = 0;

    if (m_iNodeID == 1) {
        MoveToPosition(0, 1000, 1, 1000);
    }

    if (VCS_SetDisableState(pKeyHandle, m_iNodeID, &errorCode) == 0) {
        std::cerr << "VCS_SetDisableState" << std::endl;
        return kSetValueError;
    }

    if (VCS_CloseDevice(pKeyHandle, &errorCode) == 0 || errorCode != 0) {
        std::cerr << "VCS_CloseDevice" << std::endl;
        return kFileCloseError;
    }
#endif // __arm__
    return kNoError;
}

Error_t EposController::ActivateProfilePositionMode()
{
    Error_t err = kNoError;
#ifdef __arm__
    unsigned int errorCode = 0;
    if (VCS_ActivateProfilePositionMode(pKeyHandle, m_iNodeID, &errorCode) == 0) {
        err = kSetValueError;
        std::cerr << "VCS_ActivateProfilePositionMode" << std::endl;
        return err;
    }

    m_operationMode = ProfilePosition;
#endif // __arm__
    return err;
}

Error_t EposController::ActivatePositionMode()
{
#ifdef __arm__
    Error_t err = kNoError;
    unsigned int errorCode = 0;
    if (VCS_ActivatePositionMode(pKeyHandle, m_iNodeID, &errorCode) == 0) {
        err = kSetValueError;
        std::cerr << "VCS_ActivatePositionMode" << std::endl;
        return err;
    }

    if (VCS_SetMaxFollowingError(pKeyHandle, m_iNodeID, MAX_FOLLOW_ERROR, &errorCode) == 0) {
        err = kGetValueError;
        std::cerr << "VCS_GetMaxFollowingError" << std::endl;
        return err;
    }

    unsigned int get_maxErr = 0;

    if (VCS_GetMaxFollowingError(pKeyHandle, m_iNodeID, &get_maxErr, &errorCode) == 0) {
        err = kGetValueError;
        std::cerr << "VCS_GetMaxFollowingError" << std::endl;
        return err;
    }

    if (MAX_FOLLOW_ERROR != get_maxErr) {
        err = kSetValueError;
        std::cerr << "MaxFollowingError" << std::endl;
        return err;
    }
    m_operationMode = Position;

#endif // __arm__
    return kNoError;
}

Error_t EposController::ActivateProfileVelocityMode()
{
    Error_t err = kNoError;
#ifdef __arm__
    unsigned int errorCode = 0;
    if (VCS_ActivateProfileVelocityMode(pKeyHandle, m_iNodeID, &errorCode) == 0) {
        err = kSetValueError;
        std::cerr << "VCS_ActivateProfileVelocityMode" << std::endl;
    }

    m_operationMode = ProfileVelocity;
#endif // __arm__
    return err;
}

Error_t EposController::SetPositionProfile(unsigned long ulVelocity, unsigned long ulAcc)
{
    Error_t err = kNoError;
#ifdef __arm__
    if (m_operationMode != ProfilePosition) {
        err = ActivateProfilePositionMode();
        if (err != kNoError)
            return err;
    }

    unsigned int errorCode = 0;
    if (VCS_SetPositionProfile(pKeyHandle, m_iNodeID, ulVelocity, ulAcc, ulAcc, &errorCode) == 0) {
        err = kSetValueError;
        std::cerr << "VCS_SetPositionProfile" << std::endl;
    }
#endif // __arm__
    return err;
}

Error_t EposController::SetVelocityProfile(unsigned long ulAcc)
{
#ifdef __arm__
    Error_t err = kNoError;
    if (m_operationMode != ProfileVelocity) {
        err = ActivateProfileVelocityMode();
        if (err != kNoError)
            return err;
    }
    unsigned int errorCode = 0;
    if (VCS_SetVelocityProfile(pKeyHandle, m_iNodeID, ulAcc, ulAcc, &errorCode) == 0) {
        err = kSetValueError;
        std::cerr << "VCS_SetPositionProfile" << std::endl;
    }
    return err;
#endif // __arm__
    return kNoError;
}

Error_t EposController::MoveToPositionl(long targetPos)
{
#ifdef __arm__
    if (m_iNodeID != 1) // Only applicable to Finger
        return kFunctionIllegalCallError;

    Error_t lResult;
    unsigned int errorCode = 0;

    if (m_operationMode == ProfilePosition) {
        if (VCS_MoveToPosition(pKeyHandle, m_iNodeID, targetPos, 1, 1, &errorCode) == 0) {
            lResult = kSetValueError;
            std::cerr << "VCS_MoveToPosition" << std::endl;
            return lResult;
        }
    } else if (m_operationMode == Position) {
        if (std::abs(targetPos) < std::abs(MAX_ENCODER_INC) && std::abs(targetPos) >= std::abs(NUT_POSITION)) {
            if (VCS_SetPositionMust(pKeyHandle, m_iNodeID, targetPos, &errorCode) == 0) {
                std::cerr << "VCS_SetPositionMust" << std::endl;
                return kSetValueError;
            }
        }
    }
    return kNoError;
#endif // __arm__
    return kNoError;
}

Error_t EposController::MoveToPosition(long lPos, unsigned long ulAcc, BOOL bAbsolute, int iTimeOut)
{
#ifdef __arm__
    Error_t lResult = kNoError;
    unsigned int errorCode = 0;

    if (VCS_SetPositionProfile(pKeyHandle, m_iNodeID, 2500, ulAcc, ulAcc, &errorCode) == 0) {
        lResult = kSetValueError;
        std::cerr << "VCS_SetPositionProfile" << std::endl;
        return lResult;
    }

    if (VCS_MoveToPosition(pKeyHandle, m_iNodeID, lPos, bAbsolute, 1, &errorCode) == 0) {
        lResult = kSetValueError;
        std::cerr << "VCS_MoveToPosition" << std::endl;
        return lResult;
    }

    if (VCS_WaitForTargetReached(pKeyHandle, m_iNodeID, iTimeOut, &errorCode) == 0) {
        lResult = kGetValueError;
        std::cerr << "VCS_WaitForTargetReached" << std::endl;
    }

    return lResult;
#endif // __arm__
    return kNoError;
}

Error_t EposController::SetHome()
{
#ifdef __arm__
    Error_t lResult = kNoError;
    int _lastPosition = 10000;
    int _currentPosition = -10000;
    unsigned int errorCode = 0;
    ActivateProfilePositionMode();
    while (true) {
        if (MoveToPosition(-25*m_iEncoderDirection, 1000, 0) != kNoError) {
            lResult = kSetValueError;
            std::cerr << "moveToPosition" << std::endl;
            return lResult;
        }
        if (VCS_GetPositionIs(pKeyHandle, m_iNodeID, &_currentPosition, &errorCode) == 0) {
            lResult = kGetValueError;
            std::cerr << "VCS_GetPositionIs" << std::endl;
            return lResult;
        }
        if (abs(_currentPosition - _lastPosition) < 10)
            break;
        _lastPosition = _currentPosition;
    }
    if (VCS_DefinePosition(pKeyHandle, m_iNodeID, 0, &errorCode) == 0) {
        lResult = kSetValueError;
        std::cerr << "VCS_DefinePosition" << std::endl;
    }
    return lResult;
#endif // __arm__
    return kNoError;
}

Error_t EposController::MoveWithVelocity(long lVelocity)
{
#ifdef __arm__
    unsigned errorCode = 0;
    if (VCS_MoveWithVelocity(pKeyHandle, m_iNodeID, lVelocity, &errorCode) == 0) {
        std::cerr << "VCS_MoveWithVelocity" << std::endl;
        return kSetValueError;
    }
#endif // __arm__
    return kNoError;
}

Error_t EposController::Halt()
{
#ifdef __arm__
    unsigned errorCode = 0;
    if (VCS_HaltVelocityMovement(pKeyHandle, m_iNodeID, &errorCode) == 0) {
        std::cerr << "VCS_HaltVelocityMovement" << std::endl;
        return kSetValueError;
    }
#endif // __arm__
    return kNoError;
}
