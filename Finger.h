//
// Created by Raghavasimhan Sankaranarayanan on 1/21/20.
//

#ifndef VIOLINIST_FINGER_H
#define VIOLINIST_FINGER_H

#include <iostream>
#include <cstring>
#include <mutex>

// i2c
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "MyDefinitions.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;

class Finger {
    enum State {
        OFF = 0,
        ON,
        REST
    };

    const uint8_t OFF_MAX = 135;
    const uint8_t ON_MAX = 105;
    const uint8_t ON_MIN = 105;
    const uint8_t OFF_MIN = 125;

    // i2c
    const string i2cName = "/dev/i2c-1";
    int file_i2c;
    int length;
    char* i2c_buffer;
    uint8_t prevSentValue;
    static const int slaveAddress = 0x08;
    bool b_isInitialized;

    State currentState;

    int* p_iCurrentPosition;

    std::mutex m_mutex;

    Finger();
    Error_t i2c_setup();
    uint8_t getPosition(State state);
    int constrain(int val);

    Error_t setCurrentState(State c_state, bool b_move = false);

public:
    Error_t setPositionTracker(int* position);
    static Error_t create(Finger* &pFinger, int* position = nullptr);
    Error_t move(uint8_t value);
    static Error_t destroy(Finger* &pFinger);

    Error_t updatePosition();

    Error_t rest();
    Error_t on();
    Error_t off();
};


#endif //VIOLINIST_FINGER_H
