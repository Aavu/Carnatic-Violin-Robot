//
// Created by Raghavasimhan Sankaranarayanan on 01/20/22.
// 

#pragma once

#include "logger.h"
#include "dxl/dxl.h"
#include "BowController.h"      // Right hand stuffs
#include "FingerController.h"   // Left hand stuffs

struct performParam_t {
    float* pitches;
    int32_t* positions;
    int length;
    float* bowTraj;
    int* bowChanges;
    int nBowChanges;
    float* amplitude;
    float volume = 1.0;
};


class Hathaani {
public:
    // Not thread safe. It is ok here since this is guaranteed to be called only once - inside setup()
    static Hathaani* create();
    static void destroy(Hathaani* pInstance);

    // Delete copy and assignment constructor
    Hathaani(Hathaani&) = delete;
    void operator=(const Hathaani&) = delete;

    int init(bool shouldHome = true);
    int reset();

    // Dummy function to test callback when encoder value changes
    int enableEncoderTransmission(bool bEnable = true);

    /**
     * @brief Perform the given phrase
     *
     * @param param The parameters of the phrase. Refer @performParam_t
     * @param lpf_alpha Zero Phase Lowpass filtering coefficient. 0 means no filtering
     * @return Error code
     */
    int perform(const performParam_t& param, float lpf_alpha = 0.5, bool shouldBow = true);

    int bowTest(const performParam_t& param);

    static void RPDOTimerIRQHandler();

private:
    Hathaani();
    ~Hathaani() = default;
    static void canRxHandle(can_message_t* arg);

    static Hathaani* pInstance;
    performParam_t m_performParam;

    // Bowing
    BowController* m_pBowController;
    bool m_bShouldBow = true;
    float m_fBowTestLen_s = 0;
    int m_iNBows = 1;

    // left hand
    HardwareTimer RPDOTimer;
    PortHandler m_portHandler;
    FingerController* m_pFingerController;

    bool m_bPlaying = false;
    volatile bool m_bFingerOn = false;
    volatile bool m_bFingerSafeToMove = true;

    const int kOffset = 2000 / PDO_RATE;
};
