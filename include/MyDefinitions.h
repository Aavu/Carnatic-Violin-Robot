//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#ifndef VIOLINIST_MYDEFINITIONS_H
#define VIOLINIST_MYDEFINITIONS_H

#include<iostream>
#include <cstring>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "ErrorDef.h"
#include "Util.h"

static const float SCALE_LENGTH             = 310;  //mm
static const int MAX_ENCODER_INC            = 23000;
static const int MAX_ALLOWED_VALUE          = 20000;
static const int NUT_POSITION               = 200;
static const int OPEN_STRING                = NUT_POSITION;
static const float PITCH_CORRECTION_FACTOR  = 0; //0.001;

// Finger
#define OFF_MAX 125
#define ON_MAX 90
#define ON_MIN 85
#define OFF_MIN 110

// Bow
#define MIN_PITCH 178
#define MAX_PITCH 218
#define MIN_VELOCITY 72
#define MAX_VELOCITY 120

#define MIN_SURGE 0
#define MAX_SURGE 100

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
