//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#ifndef VIOLINIST_MYDEFINITIONS_H
#define VIOLINIST_MYDEFINITIONS_H

#include "ErrorDef.h"
#include<iostream>
#include <cstring>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#ifndef MMC_SUCCESS
#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
#define MMC_FAILED 1
#endif

static const float SCALE_LENGTH = 304.5;  //mm
static const int MAX_ENCODER_INC = 23000;
static const int MAX_ALLOWED_VALUE = 18000;
static const int NUT_POSITION = 1000; //-1600 without block bolt
static const int OPEN_STRING = NUT_POSITION;

class Register {
public:
    enum Bow {
        kRoller = 1,
        kRollerEnable,
        kPitch,
        kSurge,
        kSurgeMultiplier,

        kNumBowRegisters
    };

    enum Fingering {
        kFinger,

        kNumFingerRegisters
    };
};

namespace Bow {
    enum Direction {
        Up = true,
        Down = false
    };
}

#endif //VIOLINIST_MYDEFINITIONS_H
