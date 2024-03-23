//
// Created by Raghavasimhan Sankaranarayanan on 4/2/20.
//

#ifndef TUNER_RECORDER_H
#define TUNER_RECORDER_H

#include <cstdint>
#include "ErrorDef.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <alsa/asoundlib.h>

class CRecorder {
public:
    inline static const std::string kName = "CRecorder";
    static void Create (CRecorder*& pCInstance)
    {
        pCInstance = new CRecorder();
    }

    static void Destroy (CRecorder*& pCInstance)
    {
        delete pCInstance;
    }

    Error_t Init(const std::string& sDevice, float* buffer, unsigned int sampleRate = 44100, int bufferFrames = 128, int numChannel = 1, int bitDepth = 16, bool saveToFile = false)
    {
        m_sDeviceName = sDevice;
        m_pfBuffer = buffer;
        m_uiSampleRate = sampleRate;
        m_iBufferFrames = bufferFrames;
        m_iNumChannels = numChannel;
        m_iBitDepth = bitDepth;
        m_bSaveToFile = saveToFile;
        m_iBufferSize = m_iBufferFrames * m_iNumChannels; // * m_iBitDepth / 8;
        int err;

        if ((err = snd_pcm_open (&m_captureHandle, (char*)m_sDeviceName.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
            fprintf (stderr, "cannot open audio device %s (%s)\n", (char*)m_sDeviceName.c_str(), snd_strerror (err));
            return kFileOpenError;
        }
        fprintf(stdout, "audio interface opened\n");

        if ((err = snd_pcm_hw_params_malloc (&m_hwParams)) < 0) {
            fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
            return kMemError;
        }
        fprintf(stdout, "hw_params allocated\n");

        if ((err = snd_pcm_hw_params_any (m_captureHandle, m_hwParams)) < 0) {
            fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
            return kNotInitializedError;
        }
        fprintf(stdout, "hw_params initialized\n");

        if ((err = snd_pcm_hw_params_set_access (m_captureHandle, m_hwParams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
            fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
            return kFileAccessError;
        }
        fprintf(stdout, "hw_params access set\n");

        if ((err = snd_pcm_hw_params_set_format (m_captureHandle, m_hwParams, m_format)) < 0) {
            fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
            return kUnknownError;
        }
        fprintf(stdout, "hw_params format set\n");

        if ((err = snd_pcm_hw_params_set_rate_near (m_captureHandle, m_hwParams, &m_uiSampleRate, 0)) < 0) {
            fprintf (stderr, "cannot set sample m_ulSampleRate (%s)\n", snd_strerror (err));
            return kUnknownError;
        }
        fprintf(stdout, "hw_params sampleRate set\n");

        if ((err = snd_pcm_hw_params_set_channels (m_captureHandle, m_hwParams, m_iNumChannels)) < 0) {
            fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
            return kUnknownError;
        }
        fprintf(stdout, "hw_params channels set\n");

        if ((err = snd_pcm_hw_params (m_captureHandle, m_hwParams)) < 0) {
            fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
            return kUnknownError;
        }
        fprintf(stdout, "hw_params set\n");

        snd_pcm_hw_params_free (m_hwParams);
        fprintf(stdout, "hw_params freed\n");

        if ((err = snd_pcm_prepare (m_captureHandle)) < 0) {
            fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
            return kNotInitializedError;
        }
        fprintf(stdout, "audio interface prepared\n");

        if (m_bSaveToFile)
            m_pStream = new std::ofstream(m_sOutputFileName, std::ios::out | std::ios::binary);

        AllocateMemory();

        m_bInitialized = true;
        return kNoError;
    }

    void AllocateMemory() {
        m_piBuffer = new int16_t[m_iBufferSize];
    }

    void Reset()
    {
        m_sDeviceName = "plughw:1";
        m_sOutputFileName = "/home/pi/Desktop/Output.wav";

        delete[] m_piBuffer;
        m_piBuffer = nullptr;

        m_pfBuffer = nullptr; // not owned by Recorder
        m_iBufferFrames = 128;
        m_iBufferSize = 0;
        m_uiSampleRate = 44100;
        m_iNumChannels = 1;
        m_format = SND_PCM_FORMAT_S16_LE;
        m_iBitDepth = snd_pcm_format_width(m_format);;
        m_captureHandle = nullptr;
        m_hwParams = nullptr;

        if (m_pStream)
            if (m_pStream->is_open())
                m_pStream->close();
        delete m_pStream;
        m_pStream = nullptr;

        m_bInitialized = false;
        m_bSaveToFile = false;

        if (m_captureHandle) {
            snd_pcm_close(m_captureHandle);
            fprintf(stdout, "audio interface closed\n");
        }
    }

    void SetOutputFileName(const std::string& fileName) {
        m_bSaveToFile = true;
        m_sOutputFileName = fileName;

        if (m_pStream)
            if (m_pStream->is_open()) {
                m_pStream->close();
                delete m_pStream;
            }
        m_pStream = new std::ofstream(m_sOutputFileName, std::ios::out | std::ios::binary);
    }

    Error_t Record(int index) {
        if (!m_bInitialized)
            return kNotInitializedError;

        int err = 0;
        if ((err = snd_pcm_readi (m_captureHandle, m_piBuffer, m_iBufferFrames)) != m_iBufferFrames) {
            fprintf (stderr, "read from audio interface failed (%d), (%s)\n", err, snd_strerror (err));
            return kUnknownError;
        }

        if (m_bSaveToFile) {
            m_pStream->write((char*)m_piBuffer, m_iBufferSize);
        }

        for (size_t i=0; i<m_iBufferSize; i++)
            m_pfBuffer[i + index] = (float)(m_piBuffer[i]) / (float)(( 1 << ( m_iBitDepth - 1 )) - 1);

        return kNoError;
    }

private:
    CRecorder() : m_sDeviceName(""),
                  m_sOutputFileName("Output.wav"),
                  m_piBuffer(nullptr), m_pfBuffer(nullptr),
                  m_iBufferFrames(0),
                  m_iBufferSize(0),
                  m_uiSampleRate(0),
                  m_iNumChannels(0),
                  m_iBitDepth(0),
                  m_captureHandle(nullptr),
                  m_hwParams(nullptr),
                  m_format(SND_PCM_FORMAT_S16_LE),
                  m_pStream(nullptr),
                  m_bInitialized(false),
                  m_bSaveToFile(false)
    {
        Reset();
    }

    ~CRecorder() = default;

    std::string m_sDeviceName;
    std::string m_sOutputFileName;

    int16_t* m_piBuffer;
    float* m_pfBuffer;
    int m_iBufferFrames;
    size_t m_iBufferSize;

    unsigned int m_uiSampleRate;
    int m_iNumChannels;
    unsigned int m_iBitDepth;

    snd_pcm_t *m_captureHandle;
    snd_pcm_hw_params_t *m_hwParams;
    snd_pcm_format_t m_format;

    std::ofstream* m_pStream;

    bool m_bInitialized;
    bool m_bSaveToFile;
};

#endif //TUNER_RECORDER_H
