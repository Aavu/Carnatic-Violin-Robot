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
#include "FingerController.h"
#include "CommHandler.h"
#include "BowController.h"

using namespace std;

typedef void* HANDLE;
typedef int BOOL;

class Violinist {
public:

    enum Key {
        C, C_sharp, D, D_sharp, E, F, F_sharp, G, G_sharp, A, A_sharp, B
    };

    enum Mode {
        Major, Minor, Dorian, Lydian
    };

    Violinist();
    ~Violinist();

    Error_t OpenDevice();
    Error_t Init(bool shouldHome = false);
    Error_t CloseDevice();

    Error_t Perform(const double* pitches, const size_t& length, short transpose=0);
    Error_t Perform(Key key, Mode mode, int interval_ms, float amplitude, short transpose=0);

    static Error_t GetPositionsForScale(float* positions, Key key, Mode mode, short transpose=0);

    void TrackEncoderPosition();
    void TrackTargetPosition();

    static void LogInfo(const string& message);
    void LogError(const string& functionName, int p_lResult, unsigned int p_ulErrorCode);

    unsigned int GetErrorCode();
    int GetPosition();

private:
    void SetDefaultParameters();
    static double FretLength(double fretNumber);
    long PositionToPulse(double p);

    Error_t UpdateEncoderPosition();
    Error_t UpdateTargetPosition();

    Error_t SetHome();

    Error_t MoveToPosition(long targetPos);
    Error_t RawMoveToPosition(int _pos, unsigned int _acc, BOOL _absolute);
    long ConvertToTargetPosition(double fretPos);

    int m_iRTPosition;

    const string g_programName = "Violinist";
    int m_iTimeInterval = 25; //ms

    double m_pfFretPosition = 0;

    unsigned int ulErrorCode = 0;

    unsigned int m_ulMaxFollowErr = 20000;

    bool m_bStopPositionUpdates;

    HANDLE g_pKeyHandle = nullptr;
    unsigned short g_usNodeId = 1;
    string g_deviceName;
    string g_protocolStackName;
    string g_interfaceName;
    string g_portName;
    int g_baudrate = 0;

    short m_iEncoderDirection = 1;

    // Communication Handler
    CommHandler* m_commHandler;

    // Bowing
    BowController* m_bowController;

    // Fingering
    FingerController* m_fingerController;
};


#endif //VIOLINIST_VIOLINIST_H
