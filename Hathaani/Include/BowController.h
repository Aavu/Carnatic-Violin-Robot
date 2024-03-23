//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#ifndef HATHAANI_BOWCONTROLLER_H
#define HATHAANI_BOWCONTROLLER_H

#include "EposController.h"
#include "MyDefinitions.h"
#include "CommHandler.h"
#include "Util.h"
#include "Dynamixel.h"

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

    enum String {
        E,
        A,
        D,
        G
    };

    BowController();
    ~BowController();

    static Error_t Create(BowController*& pCInstance);
    static Error_t Destroy(BowController*& pCInstance);

    Error_t Init(CommHandler* commHandler, PortHandler& portHandler, int* RTPosition);
    Error_t Reset();

    Error_t RosinMode();

    Error_t StartBowing(float amplitude = .5, Bow::Direction direction = Bow::Down, Error_t error = kNoError);
    Error_t stopBowing(Error_t error = kNoError);
    Error_t resumeBowing(Error_t error = kNoError);
    Error_t PauseBowing(Error_t error = kNoError);
    Error_t bowOnString(bool on = true, Error_t error = kNoError);
    Error_t setAmplitude(float amplitude, Error_t error = kNoError);
    Error_t setSpeed(Bow::Direction direction, long bowSpeed, Error_t error = kNoError);
    Error_t setPressure(float fAmplitude);
    Error_t setDirection(Bow::Direction direction = Bow::Down);
    Bow::Direction changeDirection();

    Error_t setString(String str);
private:
//    Error_t Send(Register::Bow reg, const uint8_t& data, Error_t error = kNoError);

    static Error_t ComputeVelocityAndPressureFor(float amplitude, long* velocity, float* pressure);
    static float transformPressure(float pressure);
    static long transformVelocity(float velocity);

    bool m_bInitialized;
    CommHandler* m_commHandler;
    PortHandler* m_pPortHandler;
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

    Dynamixel* m_pPitchDxl;
    Dynamixel* m_pYawDxl;
    Dynamixel* m_pRollDxl;
};

#endif //HATHAANI_BOWCONTROLLER_H