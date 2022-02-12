//
// Created by Raghavasimhan Sankaranarayanan on 1/4/21.
//

#ifndef CARNATICVIOLINIST_UTIL_H
#define CARNATICVIOLINIST_UTIL_H

#include <iostream>
#include <cmath>

#include "MyDefinitions.h"

class Util {
public:
    static double FretLength(float fretNumber) {
        return (SCALE_LENGTH - (SCALE_LENGTH / pow(2, (fretNumber / 12.f))));
    }

    static long PositionToPulse(double p, int direction = 1) {
        return (long)(direction * p * 24000.0 / 220.0);
    }

    static long Fret2Position(float fretPos) {
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
#endif //CARNATICVIOLINIST_UTIL_H
