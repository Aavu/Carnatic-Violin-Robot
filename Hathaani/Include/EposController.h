//
// Created by Raghavasimhan Sankaranarayanan on 1/4/21.
//

#ifndef HATHAANI_EPOSCONTROLLER_H
#define HATHAANI_EPOSCONTROLLER_H

#include <iostream>
#include <cstring>
#include <string>

#include "MyDefinitions.h"
#include "Definitions.h"
#include "ErrorDef.h"
#include "Util.h"

typedef void* HANDLE;
typedef int BOOL;

#ifndef MMC_SUCCESS
#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
#define MMC_FAILED 1
#endif

class EposController
{
public:
    enum OperationMode {
        Position, ProfilePosition, ProfileVelocity
    };

    explicit EposController(std::string portName, int iNodeID);
    virtual ~EposController();

    Error_t OpenDevice();
    Error_t init(OperationMode mode);
    Error_t CloseDevice();

    Error_t ActivateProfilePositionMode();
    Error_t ActivatePositionMode();
    Error_t ActivateProfileVelocityMode();

    Error_t SetPositionProfile(unsigned long ulVelocity, unsigned long ulAcc);
    Error_t SetVelocityProfile(unsigned long ulAcc);

    Error_t moveToPositionl(long targetPos);
    Error_t MoveToPosition(long lPos, unsigned long ulAcc, BOOL bAbsolute, int iTimeOut = 50);

    Error_t MoveWithVelocity(long lVelocity);
    Error_t Halt();

    virtual Error_t setHome();

private:
    OperationMode m_operationMode = Position;
    int m_iEncoderDirection = -1;

    HANDLE pKeyHandle = nullptr;
    uint16_t m_iNodeID = 1;
    std::string m_strDeviceName = "EPOS4";
    std::string m_strProtocolStackName = "MAXON SERIAL V2";
    std::string m_strInterfaceName = "USB";
    std::string m_strPortName = "USB0";
    unsigned int m_uiBaudrate = 1000000;
};


#endif //HATHAANI_EPOSCONTROLLER_H
