//
// Created by Raghavasimhan Sankaranarayanan on 1/21/20.
//

#include "Finger.h"

Finger::Finger():   file_i2c(0),
                    length(1),
                    i2c_buffer(nullptr),
                    prevSentValue(0),
                    b_isInitialized(false),
                    currentState(REST),
                    p_iCurrentPosition(nullptr)
{}

Error_t Finger::create(Finger* &pFinger, int* position) {
    pFinger = new Finger();
    if (position) {
        Error_t err = pFinger->setPositionTracker(position);
        if (err != kNoError) {
            cerr << "Cannot set position...\n";
            return err;
        }
    }
    return pFinger->i2c_setup();
}

Error_t Finger::destroy(Finger* &pFinger) {
    delete [] pFinger->i2c_buffer;
    pFinger->b_isInitialized = false;
    delete pFinger;
    pFinger = nullptr;
    return kNoError;
}

Error_t Finger::i2c_setup() {
    if ((file_i2c = open((char*)i2cName.c_str(), O_RDWR)) < 0)
    {
        //ERROR HANDLING: you can check errno to see what went wrong
        cerr << "Failed to open the i2c bus" << endl;
        return kFileOpenError;
    }

    if (ioctl(file_i2c, I2C_SLAVE, slaveAddress) < 0)
    {
        //ERROR HANDLING; you can check errno to see what went wrong
        cerr << "Failed to acquire bus access and/or talk to slave." << endl;
        return kUnknownError;
    }

    i2c_buffer = new char[length];

    b_isInitialized = true;
    return kNoError;
}

Error_t Finger::move(uint8_t value) {
    std::lock_guard<std::mutex> locker(m_mutex);
    if (!b_isInitialized) {
        cerr << "Call i2c_setup before calling i2c_write..." << endl;
        return kNotInitializedError;
    }

//    i2c_buffer[0] = (value >> 8u) & 0xFFu;
//    i2c_buffer[1] = value & 0xFFu;
    i2c_buffer[0] = value;
    if (write(file_i2c, i2c_buffer, length) != length)    //write() returns the number of bytes actually written, if it doesn't match then an error occurred (e.g. no response from the device)
    {
        /* ERROR HANDLING: i2c transaction failed */
        cerr << "Failed to write to the i2c bus.\n";
        return kFileAccessError;
    }
    return kNoError;
}

Error_t Finger::rest() {
    return setCurrentState(REST, true);
}

Error_t Finger::on() {
    return setCurrentState(ON, true);
}

Error_t Finger::off() {
    return setCurrentState(OFF, true);
}

uint8_t Finger::getPosition(Finger::State state) {
    int cPos = - *p_iCurrentPosition;
    double multiplier = std::abs(std::max((cPos - NUT_POSITION), 0)/(double)(MAX_ALLOWED_VALUE));
    uint8_t ulPosition = 0;
    switch (state) {
        case ON:
            ulPosition = ON_MIN + (uint8_t)(multiplier * (ON_MAX - ON_MIN));
            break;
        case OFF:
            ulPosition = OFF_MIN + (uint8_t)(multiplier * (OFF_MAX - OFF_MIN));
            break;
        case REST:
            ulPosition = OFF_MAX;
            break;
    }
    return constrain(ulPosition);
}

Error_t Finger::setPositionTracker(int* position) {
    if (!position)
        return kNotInitializedError;
    p_iCurrentPosition = position;
    return kNoError;
}

int Finger::constrain(int val) {
    return std::min(std::max(val, (int)ON_MIN), (int)OFF_MAX);
}

Error_t Finger::updatePosition() {
    uint8_t pos = getPosition(currentState);
    Error_t err = kNoError;
    if (pos != prevSentValue) {
        err = move(pos);
        prevSentValue = pos;
    }
    return err;
}

Error_t Finger::setCurrentState(Finger::State c_state, bool b_move) {
    currentState = c_state;
    if (b_move) {
        int pos = getPosition(currentState);
        return move(pos);
    }
    return kNoError;
}
