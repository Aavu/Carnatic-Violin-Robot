//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#ifndef VIOLINIST_BOWCONTROLLER_H
#define VIOLINIST_BOWCONTROLLER_H

#include "EposController.h"
#include "MyDefinitions.h"
#include "CommHandler.h"
#include "Util.h"

using std::string;
using std::cout;
using std::endl;

class BowController {
public:
    inline static const std::string kName = "BowController";
    enum State {
        Playing,
        Paused,
        Stopped
    };

    BowController();
    ~BowController();

    static Error_t Create(BowController*& pCInstance);
    static Error_t Destroy(BowController*& pCInstance);

    Error_t Init(CommHandler* commHandler, int* RTPosition);
    Error_t Reset();

    Error_t StartBowing(float amplitude = .5, Bow::Direction direction = Bow::Down, Error_t error = kNoError);
    Error_t StopBowing(Error_t error = kNoError);
//    Error_t ResumeBowing(Error_t error = kNoError);
//    Error_t PauseBowing(Error_t error = kNoError);
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

//    void updateSurge();

    bool m_bInitialized;
    CommHandler* m_commHandler;
    State m_bowingState;
    uint8_t m_iBowSpeed;
    float m_fBowPressure;

    Bow::Direction m_currentDirection;
    uint8_t m_currentBowPitch;
    float m_currentAmplitude;
    uint8_t m_currentSurge;

    int* m_piRTPosition;

//    std::thread positionTrackThread;

    EposController m_wheelController;
};

#endif //VIOLINIST_BOWCONTROLLER_H