//
// Created by Raghavasimhan Sankaranarayanan on 01/20/22.
//

#ifndef UTIL_H
#define UTIL_H

#include "logger.h"
#include "def.h"

namespace Util {
    void zeroPhaseLPF(float* p, int nPitches, float a);
    void interpolateGaps(float* p, int nPitches);
    void trimEndZeros(float* p, int& nPitches);
    void cleanUpData(float* p, int& length, float lpf_alpha);
    bool checkFollowError(int32_t* p, int nPitches);
    float __fretLength(float fretNumber);
    int32_t __pos2Pulse(float pos, int direction = ENCODER_DIR);
    int32_t fret2Pos(float fretPos);

    void thirdOrderPolyFit(int t1, float y, float* arr);
    void convertPitchToInc(int32_t* inc, const float* p, int nPitches, int offset = 0);

    void arange(int start, int end, int* array = nullptr, int step = 1);
    void linspace(float start, float end, int N, float* array = nullptr);

    // refer: https://cs.gmu.edu/~kosecka/cs685/cs685-trajectories.pdf
    void interpWithBlend(float q0, float qf, int32_t N, float tb_cent = 0.1, float* curve = nullptr);
} // namespace Util

#endif // UTIL_H