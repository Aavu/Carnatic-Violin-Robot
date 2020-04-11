//
// Created by Raghavasimhan Sankaranarayanan on 4/3/20.
//

#include "Tuner.h"

CTuner::CTuner() {
}

CTuner::~CTuner() {
    Reset();
}

Error_t CTuner::Create(CTuner *&pCInstance) {
    pCInstance = new CTuner();
    return kNoError;
}

Error_t CTuner::Destroy(CTuner *&pCInstance) {
    delete pCInstance;

    return kNoError;
}

Error_t CTuner::Init(double* fretPosition) {
    m_fFretPosition = fretPosition;

    m_pfBuffer  = new float [iBufferSize];

    CRecorder::Create(m_pCRecorder);
    auto err = m_pCRecorder->Init(deviceName, m_pfBuffer, m_ulSampleRate, iBufferFrames, iNumChannels, iBitDepth, false);
    if (err != kNoError)
        return err;

    CFft::createInstance(m_pCFft);
    err = m_pCFft->initInstance(iBufferSize, m_iZeroPaddingFactor, CFft::kWindowHamming, CFft::kNoWindow);
    if (err != kNoError)
        return err;

    m_iFftLength = m_pCFft->getLength(CFft::kLengthFft);
    m_iMagLength = m_pCFft->getLength(CFft::kLengthMagnitude);

    m_pfFreq    = new float [m_iFftLength];
    m_pfMag     = new float [m_iMagLength];

    CVectorFloat::setZero(m_pfBuffer, iBufferSize);
    m_bInitialized = true;
    return kNoError;
}

Error_t CTuner::Reset() {
    m_pCRecorder->Reset();
    CRecorder::Destroy(m_pCRecorder);

    m_pCFft->resetInstance();
    CFft::destroyInstance(m_pCFft);

    delete[] m_pfBuffer;
    delete[] m_pfMag;
    delete[] m_pfFreq;

    return kNoError;
}

Error_t CTuner::Start() {
    if (!m_bInitialized)
        return kNotInitializedError;

    if (m_bRunning)
        return kUnknownError;

    m_bInterrupted = false;

    std::thread([this]() {
        m_bRunning = true;
        Error_t err;
        while (!m_bInterrupted)
        {
            err = Process();
            if (err != kNoError) {
                CUtil::PrintError("Process", err);
                break;
            }

        }
        m_bRunning = false;
    }).detach();

    return kNoError;
}

Error_t CTuner::Stop() {
    m_bInterrupted = true;
    return kNoError;
}

Error_t CTuner::Process() {
    for (int j=iFramesPerFFT; j > 1; j--)
        CVectorFloat::moveInMem(m_pfBuffer, iBufferSize - ( iBufferSizePerFrame * j ), iBufferSize - (iBufferSizePerFrame*(j - 1)), iBufferSizePerFrame);

    auto err = m_pCRecorder->Record(iBufferSize - iBufferSizePerFrame);
    if (err != kNoError) {
        CUtil::PrintError("Record", err);
        return err;
    }

    m_pCFft->doFft(m_pfFreq, m_pfBuffer);
    m_pCFft->getMagnitude(m_pfMag, m_pfFreq);

    float n, c;
    err = getNote(n, c);
    if (err != kNoError) {
        CUtil::PrintError("getNote", err);
        return err;
    }
    m_fNote = n;
    m_fCorrection = c;
    std::cout << m_fNote << std::endl;
//    std::cout << "output: " << m_fNote << std::endl;
    return kNoError;
}

float CTuner::GetNote() {
    return m_fNote;
}

float CTuner::GetNoteCorrection() {
    return m_fCorrection;
}

CTuner::RangeID CTuner::isNoteInRange(double note, double ref) {
    if (isRoot(note, ref)) {
        return Root;
    } else if (is5th(note, ref)) {
        return Fifth;
    }
    return None;
}

bool CTuner::checkBounds(double note, double ref, CTuner::RangeID id) {
    auto lower = CVectorFloat::mod<double>(ref - bandwidth, 12);
    auto upper = CVectorFloat::mod<double>(ref + bandwidth, 12);
//    if (id == Fifth)
//        std::cout << "ref: " << ref << "\tlower: " << lower << "\tupper: " << upper << "\tnote: " << note;
    if (lower < upper)
        return note > lower && note < upper;
    return note > lower || note < upper;
}

bool CTuner::isRoot(double note, double ref) {
    return checkBounds(note, ref, Root);
}

bool CTuner::is5th(double note, double ref) {
    auto _ref = CVectorFloat::mod<double>(ref + 7, 12);
    return checkBounds(note, _ref, Fifth);
}

Error_t CTuner::getNote(float& note, float& correction) {
    auto argmax = std::distance(m_pfMag + 1, std::max_element(m_pfMag, m_pfMag + m_iMagLength / 2));
    auto x = m_pCFft->interpolate(m_pfMag, argmax, m_iMagLength / 2);
    auto f0 = m_pCFft->bin2freq(argmax + x, (float)m_ulSampleRate);
    f0 = std::max(f0, 0.f);
    auto n = CFft::freq2note(f0);
    auto ref = CVectorFloat::mod<double>(*m_fFretPosition + m_iFretToNoteTransform, 12);
    auto rangeID = isNoteInRange(n, ref);
    switch (rangeID) {
        case Root:
            note = n;
            correction = ref - note;
            break;
        case Fifth:
            note = CVectorFloat::mod<float>(n - 7, 12);
            correction = ref - note;
            break;
        case None:
            note = -1;
            correction = 0;
    }
//    std::cout << "ref: " << ref << "\tnote : " << note << "\trangeID: " << rangeID << "\tCorrection: " << correction;
    if (std::abs(correction) > bandwidth*2) {
        note = -1;
        CUtil::PrintMessage(__PRETTY_FUNCTION__, "Out of Bounds");
    }

    return kNoError;
}
