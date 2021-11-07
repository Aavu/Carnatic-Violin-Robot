//
// Created by Raghavasimhan Sankaranarayanan on 4/3/20.
//

#ifndef HATHAANI_TUNER_H
#define HATHAANI_TUNER_H

#include <algorithm>
#include <memory>

#include "MyDefinitions.h"
#include "Recorder.h"
#include "Fft.h"
#include "Vector.h"
#include "Util.h"

class CTuner {
public:
    inline static const std::string kName = "CTuner";
    enum RangeID {
        Root,
        Fifth,
        None
    };

    CTuner();
    ~CTuner();

    static Error_t Create(CTuner*& pCInstance);
    static Error_t Destroy(CTuner*& pCInstance);

    Error_t Init(float* fretPosition);
    Error_t Reset();

    Error_t Start();
    Error_t Stop();

    Error_t Process();

    float GetNoteCorrection();
    float GetNote();

private:
    Error_t getNote(float& note, float& correction);
    static RangeID isNoteInRange(double note, double ref);
    static bool checkBounds(double note, double ref, RangeID id);
    static bool isRoot(double note, double ref);
    static bool is5th(double note, double ref);

private:
    bool m_bInitialized = false;
    bool m_bRunning = false;
    float m_fNote = -1;
    float m_fCorrection = 0;
    bool m_bInterrupted = true;

    const std::string deviceName = "plughw:1";
    static const int iBufferFrames = 128;
    static const int iNumChannels = 1;
    static const int iBitDepth = 16;
    static const int iFramesPerFFT = 4;

    int iBufferSizePerFrame = iBufferFrames * iNumChannels;
    int iBufferSize = iBufferSizePerFrame * iFramesPerFFT;

    unsigned int m_ulSampleRate = 2000;
    int m_iZeroPaddingFactor = 16;
    int m_iFftLength = iBufferSize * m_iZeroPaddingFactor;
    int m_iMagLength = 0;

    float* m_pfBuffer = nullptr;
    CFft::complex_t *m_pfFreq = nullptr;
    float *m_pfMag = nullptr;

    CRecorder* m_pCRecorder = nullptr;
    CFft *m_pCFft = nullptr;

    constexpr static const double bandwidth = .5;

    float* m_fFretPosition = nullptr;
    int m_iFretToNoteTransform = 3;
};


#endif //HATHAANI_TUNER_H
