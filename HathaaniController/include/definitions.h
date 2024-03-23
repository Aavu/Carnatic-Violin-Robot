#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "../src/epos4/epos4_def.h"

// #define SIMULATE

#define IP_ADDR "10.2.1.177"
#define MASTER_ADDR "10.2.1.1"
#define PORT 8888
#define CS_PIN 10

/// @brief Representation of Motor IDs. Node Ids = MotorId + 1
enum MotorId {
    FINGER = 0,
    STRING_CHG = 1,
    LEFT_HAND = 2,
    BOW_D_LEFT = 3,
    BOW_D_RIGHT = 4,
    BOW_SLIDE = 5,
    BOW_ROTOR = 6,

    NUM_MOTORS
};

/// @brief Set of commands supported by the robot
enum class Command {
    STOP = 0,
    START = 1,
    HOME = 2,
    READY = 3,
    REQUEST_DATA = 4,
    CURRENT_VALUES = 5,
    SERVO_OFF = 6,
    SERVO_ON = 7,
    DEFAULTS = 8,
    RESTART = 9,

    NUM_COMMANDS
};

#define NUM_BYTES_PER_VALUE sizeof(uint16_t)   // 16 bit integer
#define MAX_BUFFER_SIZE 1024
#define HOP_SIZE 160
#define SAMPLE_RATE 16000

#define REQUEST_BUFFER_FLAG 1
#define BUFFER_TIME 1   // sec

const MotorModel MOTOR_MODEL[] = { EC32, EC45, EC45, EC45, EC45, EC45, EC45 };
const OpMode OP_MODE[] = { CyclicSyncPosition, CyclicSyncPosition, CyclicSyncPosition, CyclicSyncPosition, CyclicSyncPosition, CyclicSyncPosition, CyclicSyncPosition };
const int ENCODER_TICKS_PER_TURN[] = { 2048, 1024, 1024, 1024, 1024, 1024, 1024 };  // ticks per 90 deg turn
const int MOTOR_POLARITY[] = { 0, 0, 1, 1, 0, 1, 1 };  // Direction of rotation 0: ccw, 1 cw
const int HOMING_METHOD[] = { CurrentThresholdNegative, CurrentThresholdNegative, CurrentThresholdNegative, CurrentThresholdNegative, CurrentThresholdNegative, CurrentThresholdNegative, CurrentThresholdNegative };
const int HOMING_THRESHOLD[] = { 2000, 2000, 2000, 5000, 5000, 2000, 2000 };

#define PDO_RATE (int)(HOP_SIZE * 1000 / SAMPLE_RATE)     // ms
#define TRANSITION_LENGTH PDO_RATE * 5  // ms

#define CALLBACK_ENC_THRESHOLD 5    // ticks
#define MAX_FOLLOW_ERROR 30000
#define DISCONTINUITY_THRESHOLD 2000

const int EC45_DATA[] = { 3210, 5000, 36900, 296, 8, 0, 781067, 1386330, 3303872, 11508462, 79040, 0, 630 };
const int EC32_DATA[] = { 4240, 5000, 21300, 123, 6, 0, 540211, 997786, 4893549, 147893815, 53280, 693, 198 };

#endif // DEFINITIONS_H