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

// Error Handling
#define IGNORE_ERROR

static const float SCALE_LENGTH             = 310;  //mm
static const int MAX_ENCODER_INC            = 23000;
static const int MAX_ALLOWED_VALUE          = 20000;
static const int NUT_POSITION               = 200;
static const int OPEN_STRING                = NUT_POSITION;
static const float PITCH_CORRECTION_FACTOR  = 0; //0.001;

static const unsigned int MAX_FOLLOW_ERROR  = 20000;
// Finger
#define FINGER_OFF 0
#define FINGER_ON 1

#define FINGER_OFF_MAX 135
#define FINGER_ON_MAX 95
#define FINGER_OFF_MIN 105
#define FINGER_ON_MIN 85

// Bow
#define MIN_PITCH 178
#define MAX_PITCH 218
#define MIN_VELOCITY 72
#define MAX_VELOCITY 120

#define BOW_ACCELERATION 5000L

class Register {
public:
    enum Bow {
        kPitch = 1
    };

    enum Fingering {
        kFinger = 0,
        kFingerOffMax = 2,
        kFingerOnMax = 3,
        kFingerOffMin = 4,
        kFingerOnMin = 5,
        kEncoderSetHome = 6,
    };
};

namespace Bow {
    enum Direction {
        Up = true,
        Down = false
    };
}

#endif //VIOLINIST_MYDEFINITIONS_H
