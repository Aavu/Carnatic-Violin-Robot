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

} // namespace Util

#endif // UTIL_H