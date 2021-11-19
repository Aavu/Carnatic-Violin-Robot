//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#ifndef HATHAANI_MYDEFINITIONS_H
#define HATHAANI_MYDEFINITIONS_H

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
//#define IGNORE_ERROR

#define ENABLE_BOWING

#define FINGER_EPOS4_USB    "USB0"
#define BOW_EPOS4_USB       "USB1"
#define DXL_DEVICE_NAME     "/dev/ttyUSB0"

#define DXL_BAUDRATE        57600

static const float SCALE_LENGTH             = 330;  //mm
static const int MAX_ENCODER_INC            = 20000;
static const int NUT_POSITION               = 3000;
static const float P2P_MULTIPLIER           = 10300.f/100.f;    // Pulses per mm
static const unsigned int MAX_FOLLOW_ERROR  = 20000;

static const float PITCH_CORRECTION_FACTOR  = 0; //0.001;

// Finger
#define FINGER_OFF 40
#define FINGER_ON 18

#define MAX_CURRENT 500

#define FINGER_OFF_MAX 135
#define FINGER_ON_MAX 95
#define FINGER_OFF_MIN 105
#define FINGER_ON_MIN 85

// Bow
#define BOW_PITCH_TRANSLATION 127
#define MIN_PITCH 196    // 202.15
#define MAX_PITCH 170       // 158.2
#define ROSIN_PRESSURE 0
#define ROSIN_SPEED 150
#define MIN_VELOCITY 70
#define MAX_VELOCITY 150

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

#endif //HATHAANI_MYDEFINITIONS_H
