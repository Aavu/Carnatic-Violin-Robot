//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#ifndef HATHAANI_HATHAANI_H
#define HATHAANI_HATHAANI_H

#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <sstream>

#include "EposController.h"
#include "MyDefinitions.h"
#include "FingerController.h"
#include "CommHandler.h"
#include "BowController.h"
#include "Util.h"

#include "Tuner.h"

using namespace std;

typedef void* HANDLE;
typedef int BOOL;

class Hathaani {
public:
    inline static const std::string kName = "Hathaani";
    enum Key {
        C = 0, C_sharp, D, D_sharp, E, F, F_sharp, G, G_sharp, A, A_sharp, B
    };

    enum Mode {
        Major, Minor, Dorian, Lydian, SingleNote, Major_Ascend
    };

    enum Gamaka {
        Spurita,
        Jaaru,
        Kampita,
        Nokku,
        Odukkal,
        Orikkai,
        Khandippu
    };

    Hathaani();
    ~Hathaani();

    Error_t Init(bool shouldHome = true, bool usePitchCorrection = false);

    Error_t ApplyRosin(int time = 10 /* sec */);
    Error_t Perform(const std::vector<float>& pitches, const std::vector<size_t>& bowChange, float amplitude, int8_t transpose);
    Error_t Perform(const std::vector<float>& pitches, const std::vector<size_t>& bowChange, const std::vector<float>& amplitude, float maxAmplitude, int8_t transpose);
//    Error_t Perform(const double* pitches, const size_t& length, const std::vector<size_t>& bowChange, float amplitude, int8_t transpose);
    Error_t Perform(Key key, Mode mode, int interval_ms, float amplitude, short transpose=0);
    // Refer: https://docs.google.com/document/d/1pFtqsbGZRWFdYnXaYPe7DsxtM3WqSJLXV0DlrrbXF2c/edit#heading=h.85q2gj4ocoei
    Error_t Perform(Gamaka gamaka, int exampleNumber, int interval_ms, float amplitude);
    Error_t Play(float amplitude);

    static Error_t GetPositionsForScale(std::vector<float>& positions, Key key, Mode mode, short transpose=0);

    void TrackTargetPosition();

    static void LogInfo(const string& message);
    static void LogError(const string& functionName, Error_t p_lResult, unsigned int p_ulErrorCode);

private:
    void SetupTuner();

    // Gamakas
    Error_t PerformSpurita(int exampleNumber, int interval_ms, float amplitude);
    Error_t PerformJaaru(int exampleNumber, int interval_ms, float amplitude);
    Error_t PerformNokku(int exampleNumber, int interval_ms, float amplitude);
    Error_t PerformOdukkal(int interval_ms, float amplitude);
    Error_t PerformOrikkai(int interval_ms, float amplitude);
    Error_t PerformKhandippu(int interval_ms, float amplitude);
    Error_t PerformKampita(int exampleNumber, int interval_ms, float amplitude);

    Error_t UpdateTargetPosition();

    void PitchCorrect();

    int m_iRTPosition;

    int m_iTimeInterval = 10; //ms

    float m_fFretPosition = 0;

    unsigned int ulErrorCode = 0;

    unsigned int m_ulMaxFollowErr = 20000;

    bool m_bStopPositionUpdates;
    bool m_bShouldHome = false;
    bool m_bTunerOn = false;
    bool m_bUsePitchCorrection = false;
    bool m_bInterruptPitchCorrection = false;

    std::thread positionTrackThread;

    EposController::OperationMode m_operationMode = EposController::Position;

    // Communication Handler
    CommHandler* m_pCommHandler;

    // Bowing
    BowController* m_pBowController;

    // Fingering
    FingerController* m_pFingerController;

    // Tuner
    CTuner* m_pCTuner;
};


#endif //HATHAANI_HATHAANI_H
