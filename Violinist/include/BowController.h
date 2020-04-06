//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#ifndef VIOLINIST_BOWCONTROLLER_H
#define VIOLINIST_BOWCONTROLLER_H

#include "../../include/MyDefinitions.h"
#include "CommHandler.h"

using std::string;
using std::cout;
using std::endl;

class BowController {
public:
#define MIN_PITCH 175
#define MAX_PITCH 215
#define MIN_VELOCITY 72
#define MAX_VELOCITY 115

    enum State {
        Playing,
        Stopped
    };

    BowController();

    static Error_t Create(BowController*& pCInstance);
    static Error_t Destroy(BowController*& pCInstance);

    Error_t Init(CommHandler* commHandler, int* RTPosition);
    Error_t Reset();

    Error_t StartBowing(float amplitude = .5, Bow::Direction direction = Bow::Down, Error_t error = kNoError);
    Error_t StopBowing(Error_t error = kNoError);
    Error_t BowOnString(bool on = true, Error_t error = kNoError);
    Error_t SetAmplitude(float amplitude, Error_t error = kNoError);
    Error_t SetSpeed(Bow::Direction direction, uint8_t bowSpeed, Error_t error = kNoError);
    Error_t SetPressure(float bowPressure, Error_t error = kNoError);
    Error_t SetDirection(Bow::Direction direction = Bow::Down);
    Bow::Direction ChangeDirection();

private:
    Error_t Send(Register::Bow reg, const uint8_t& data, Error_t error = kNoError);
    Error_t SetPressure(uint8_t bowPressure, Error_t error = kNoError);

    static Error_t ComputeVelocityAndPressureFor(float amplitude, uint8_t* velocity, uint8_t* pressure, Error_t error = kNoError);
    static uint8_t TransformPressure(float pressure);
    static uint8_t TransformVelocity(float velocity);

    bool m_bInitialized;
    CommHandler* m_commHandler;
    State m_bowingState;
    uint8_t m_iBowSpeed;
    float m_fBowPressure;

    Bow::Direction m_currentDirection;
    uint8_t m_currentBowPitch;
    float m_currentAmplitude;

    int* m_iRTPosition;
};

#endif //VIOLINIST_BOWCONTROLLER_H