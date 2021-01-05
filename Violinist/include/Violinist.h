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
#include "Util.h"

#include "Tuner.h"

using namespace std;

typedef void* HANDLE;
typedef int BOOL;

class Violinist {
public:
    inline static const std::string kName = "Violinist";
    enum Key {
        C = 0, C_sharp, D, D_sharp, E, F, F_sharp, G, G_sharp, A, A_sharp, B
    };

    enum Mode {
        Major, Minor, Dorian, Lydian, SingleNote
    };

    enum Gamaka {
        Spurita,
        Jaaru,
        Kampita,
        Nokku,
        Odukkal,
        Orikkai,
        Khandimpu,
        Vali
    };

    enum OperationMode {
        Position, ProfilePosition
    };

    Violinist();
    ~Violinist();

    Error_t OpenDevice();
    Error_t Init(bool shouldHome = true, bool usePitchCorrection = false);
    Error_t CloseDevice();

    Error_t Perform(const double* pitches, const size_t& length, short transpose=0);
    Error_t Perform(Key key, Mode mode, int interval_ms, float amplitude, short transpose=0);
    // Refer: https://docs.google.com/document/d/1pFtqsbGZRWFdYnXaYPe7DsxtM3WqSJLXV0DlrrbXF2c/edit#heading=h.85q2gj4ocoei
    Error_t Perform(Gamaka gamaka, int exampleNumber, int interval_ms, float amplitude);
    Error_t Play(float amplitude);

    static Error_t GetPositionsForScale(std::vector<float>& positions, Key key, Mode mode, short transpose=0);

    void TrackEncoderPosition();
    void TrackTargetPosition();

    static void LogInfo(const string& message);
    static void LogError(const string& functionName, Error_t p_lResult, unsigned int p_ulErrorCode);

    [[nodiscard]] unsigned int GetErrorCode() const;
    int GetPosition();

private:
    void SetDefaultParameters();
    void SetupTuner();

    // Gamakas
    Error_t PerformSpurita(int interval_ms, float amplitude);
    Error_t PerformJaaru(int interval_ms, float amplitude);
    Error_t PerformNokku(int interval_ms, float amplitude);
    Error_t PerformOdukkal(int interval_ms, float amplitude);
    Error_t PerformOrikkai(int interval_ms, float amplitude);
    Error_t PerformKhandimpu(int interval_ms, float amplitude);
    Error_t PerformVali(int interval_ms, float amplitude);
    Error_t PerformKampita(int exampleNumber, int interval_ms, float amplitude);

    static double FretLength(double fretNumber);
    [[nodiscard]] long PositionToPulse(double p) const;

    Error_t UpdateEncoderPosition();
    Error_t UpdateTargetPosition();

    Error_t SetHome();

    Error_t MoveToPosition(long targetPos);
    Error_t RawMoveToPosition(int _pos, unsigned int _acc, BOOL _absolute);
    long ConvertToTargetPosition(double fretPos);

    Error_t ActivatePositionMode();
    Error_t ActivateProfilePositionMode(unsigned int uiVelocity = 2500, unsigned int uiAcc = 10000);

    void PitchCorrect();

    int m_iRTPosition;

    int m_iTimeInterval = 100; //ms

    double m_pfFretPosition = 0;

    unsigned int ulErrorCode = 0;

    unsigned int m_ulMaxFollowErr = 20000;

    bool m_bStopPositionUpdates;
    bool m_bShouldHome = false;
    bool m_bTunerOn = false;
    bool m_bUsePitchCorrection = false;
    bool m_bInterruptPitchCorrection = false;

    std::thread positionTrackThread;

    HANDLE g_pKeyHandle = nullptr;
    unsigned short g_usNodeId = 1;
    string g_deviceName;
    string g_protocolStackName;
    string g_interfaceName;
    string g_portName;
    int g_baudrate = 0;

    short m_iEncoderDirection = 1;

    OperationMode m_operationMode = Position;

    // Communication Handler
    CommHandler* m_commHandler;

    // Bowing
    BowController* m_pBowController;

    // Fingering
    FingerController* m_pFingerController;

    // Tuner
    CTuner* m_pCTuner;
};


#endif //VIOLINIST_VIOLINIST_H
