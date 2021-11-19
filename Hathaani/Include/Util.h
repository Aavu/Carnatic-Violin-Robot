//
// Created by Raghavasimhan Sankaranarayanan on 1/4/21.
//

#ifndef HATHAANI_UTIL_H
#define HATHAANI_UTIL_H

#include <iostream>
#include <cmath>

#include "MyDefinitions.h"

class Util {
public:
    static double FretLength(float fretNumber) {
        return (SCALE_LENGTH - (SCALE_LENGTH / pow(2, (fretNumber / 12.f))));
    }

    static long PositionToPulse(double p, int direction = 1) {
        return (long)(direction * p * P2P_MULTIPLIER);
//        return (long)(direction * p * 24000.0 / 220.0);
    }

    static long fret2Position(float fretPos) {
        return PositionToPulse(FretLength(fretPos)) + NUT_POSITION;
    }

    static Error_t Return(Error_t err=kNoError) {
#ifndef IGNORE_ERROR
        return err;
#else
        return kNoError;
#endif
    }
};
#endif //HATHAANI_UTIL_H
