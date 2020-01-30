//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#ifndef VIOLINIST_VIOLINIST_H
#define VIOLINIST_VIOLINIST_H

#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <sstream>

#include "Definitions.h"
#include "MyDefinitions.h"
#include "Finger.h"

using namespace std;

typedef void* HANDLE;
typedef int BOOL;

class Violinist {
public:
    Violinist();
    ~Violinist();

    Error_t OpenDevice();
    Error_t Prepare(float* fretPos, bool shouldHome = false);
    Error_t CloseDevice();
//    Error_t Play();

    void TrackEncoderPosition();
    void TrackTargetPosition();

    static void LogInfo(const string& message);
    void LogError(const string& functionName, int p_lResult, unsigned int p_ulErrorCode);

    unsigned int GetErrorCode();
    int GetPosition();

#ifdef USE_FINGER
    Finger* finger;
#endif

private:
    int iRTPosition = 0;
    int iTimeInterval = 1; //ms
    float *p_fFretPosition = nullptr;
    float fPrevFretPosition = 0;
    unsigned int ulErrorCode = 0;

    unsigned int set_maxErr = 20000;

    bool b_stopPositionUpdates;

    HANDLE g_pKeyHandle = nullptr;
    unsigned short g_usNodeId = 6;
    string g_deviceName;
    string g_protocolStackName;
    string g_interfaceName;
    string g_portName;
    int g_baudrate = 0;

    const string g_programName = "Violinist";
    void SetDefaultParameters();
    static double FretLength(uint8_t fretNumber);
    static long PositionToPulse(double p);

    Error_t UpdateEncoderPosition();
    Error_t UpdateTargetPosition();

    Error_t SetHome();

    Error_t MoveToPosition(long targetPos);
    Error_t RawMoveToPosition(int _pos, unsigned int _acc, BOOL _absolute);
    unsigned int GetAcceleration();
    static long convertToTargetPosition(float fretPos);
};


#endif //VIOLINIST_VIOLINIST_H
