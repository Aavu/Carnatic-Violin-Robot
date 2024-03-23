/*
* Author: Raghavasimhan Sankaranarayanan
* Date: 02/26/24
*/

#pragma once

#include <CAN.h>

#include "../include/definitions.h"
#include "../include/ErrorDef.h"
#include "logger.h"
#include "epos4/epos4.h"

#include "networkHandler.h"
#include "trajectory.h"
#include "util.h"


class Hathaani {
public:
    static Hathaani* create() {
        pInstance = new Hathaani();
        return pInstance;
    }

    static void destroy(Hathaani* pInstance) {
        delete pInstance;
    }

    // Delete copy and assignment constructor
    Hathaani(Hathaani&) = delete;
    void operator=(const Hathaani&) = delete;

    Error_t init() {
        Error_t err;
        if (initCAN() != kNoError)
            return err;

        if (initNetwork() != kNoError)
            return err;

#ifndef SIMULATE
        if (initMotors() != kNoError) {
            reset();
            return err;
        }
#endif
        m_trajTimer.setPeriod(PDO_RATE * 1000);
        m_trajTimer.attachInterrupt(&Hathaani::RPDOTimerIRQHandler);
        LOG_LOG("Hathaani Initialized");
        return kNoError;
    }

    void reset() {
        m_trajTimer.stop();

#ifndef SIMULATE
        for (int i = 0; i < NUM_MOTORS; ++i) {
            m_epos[i].reset();
        }
#endif
        m_socket.close();

        m_bInitialized = false;
        m_bSendDataRequest = true;
        m_bDataRequested = false;
        delay(100);
    }

    Error_t initCAN() {
        if (!CanBus.begin(CAN_BAUD_1000K, CAN_STD_FORMAT)) {
            LOG_ERROR("CAN open failed");
            return kConfigError;
        }

        CanBus.attachRxInterrupt(&Hathaani::canRxHandle);
        return kNoError;
    }

    Error_t initNetwork() {
        Error_t err = m_socket.init(&Hathaani::packetCallback);
        if (err != kNoError)
            return err;

        int status = 0;
        bool printed = false;
        while (status == 0) {
            status = m_socket.connectToMaster();
            if (status == 0 && !printed) {
                LOG_LOG("Waiting for Master. Status %i", status);
                printed = true;
                delay(100);
            }
        }

        if (status != 1) {
            LOG_ERROR("Error connecting to master: code %i", status);
            return kConfigError;
        }

        return err;
    }

    Error_t initMotors() {
        int err = 0;
        for (int i = 0; i < NUM_MOTORS; ++i) {

            err = m_epos[i].init(i + 1, MOTOR_MODEL[i], ENCODER_TICKS_PER_TURN[i]);
            if (err) {
                LOG_ERROR("Error initializing motor with node ID %i", i + 1);
                return kNotInitializedError;
            }
        }
        return kNoError;
    }

    Error_t home() {
        int err;

        m_trajTimer.stop();

#ifndef SIMULATE
        for (int i = 0; i < NUM_MOTORS; ++i) {
            err = m_epos[i].home(true);
            if (err != 0) {
                LOG_ERROR("home");
                return kSetValueError;
            }
        }

        delay(10);

        for (int i = 0; i < NUM_MOTORS; ++i) {
            err = m_epos[i].setOpMode(ProfilePosition);
            if (err != 0) {
                LOG_ERROR("setOpMode");
                return kSetValueError;
            }

            err = m_epos[i].setProfile(500, 5000);
            if (err != 0) {
                LOG_ERROR("setProfile");
                return kSetValueError;
            }
            err = m_epos[i].moveToPosition(kInitials[i], true);
            if (err != 0) {
                LOG_ERROR("moveToPosition");
                return kSetValueError;
            }

            m_currentPoint[i] = kInitials[i];
        }
#else
        delay(5000);
#endif

        auto e = enablePDO();
        if (e != kNoError) return e;

        m_bInitialized = true;
        m_trajTimer.start();

        return kNoError;
    }

    Error_t enable(bool bEnable = true) {
#ifndef SIMULATE
        for (int i = 0; i < NUM_MOTORS; ++i) {
            if (!bEnable) {
                if (m_epos[i].setNMTState(PreOperational)) {
                    LOG_ERROR("setNMTState for node %i", i + 1);
                    return kSetValueError;
                }
            }
            if (m_epos[i].setEnable(bEnable)) {
                LOG_ERROR("setEnable for node %i", i + 1);
                return kSetValueError;
            }
        }
#endif
        return kNoError;
    }

    Error_t enablePDO(bool bEnable = true) {
#ifndef SIMULATE
        auto state = bEnable ? NMTState::Operational : NMTState::PreOperational;
        for (int i = 0; i < NUM_MOTORS; ++i) {
            if (m_epos[i].setNMTState(state)) {
                LOG_ERROR("setNMTState for node %i", i + 1);
                return kSetValueError;
            }

            if (m_epos[i].setOpMode(OP_MODE[i])) {
                LOG_ERROR("setOpMode for node %i", i + 1);
                return kSetValueError;
            }
        }
#endif
        return kNoError;
    }

    // Remember to call this function inside loop()!!!
    void poll() {
        // If not enough values in buffer, request master for more values
        if (m_bSendDataRequest && m_bInitialized) {
            if (!m_bDataRequested) {
                m_socket.requestData();
                m_bDataRequested = true;
                // LOG_LOG("Requesed data");
            }
        }

        m_socket.poll();
    }

private:
    // An UdpHandler instance to let us send and receive packets over UDP and TCP
    NetworkHandler m_socket;
    HardwareTimer m_trajTimer;
    Trajectory<int32_t> m_traj;

    static Hathaani* pInstance;

    bool m_bInitialized = false;

    Epos4 m_epos[NUM_MOTORS];

    Trajectory<int32_t>::point_t m_currentPoint {};

    bool m_bSendDataRequest = true;
    bool m_bDataRequested = false;

    // This value is set by the master via Command::DEFAULTS
    uint16_t kInitials[NUM_MOTORS];
    bool m_bInitialAssigned = false;

    Hathaani() : m_trajTimer(TIMER_CH3) {}

    ~Hathaani() {
        reset();
    }

    void setInitials(uint16_t* initials) {
        for (int i = 0; i < NUM_MOTORS; ++i) {
            kInitials[i] = initials[i];
        }
        m_bInitialAssigned = true;

        LOG_LOG("Assigned Default positions");
    }

    static void packetCallback(char* packet, size_t packetSize) {
        // Commanda have a packet size of 1
        if (packetSize == 1) {
            int tmp = packet[0];
            if (tmp >= 0 && tmp < (int) Command::NUM_COMMANDS) {
                Command cmd = static_cast<Command>(tmp);
                switch (cmd) {
                case Command::HOME:
                    if (!pInstance->m_bInitialAssigned) {
                        LOG_ERROR("No default positions not assigned. Call 'INITIALS' before calling  'HOME'");
                        break;
                    }

                    if (!pInstance->m_bInitialized) {
                        LOG_LOG("Homing Robot");
                        pInstance->home();
                        LOG_LOG("Homing Complete");
                    }
                    pInstance->m_socket.send(Command::READY);
                    break;

                case Command::CURRENT_VALUES:
                    pInstance->m_socket.send(pInstance->m_currentPoint.data);
                    break;
                case Command::SERVO_OFF:
                    LOG_LOG("Servo Off...");
                    pInstance->enable(false);
                    break;
                case Command::SERVO_ON:
                    LOG_LOG("Servo On...");
                    pInstance->enable(true);
                    break;
                case Command::RESTART:
                    LOG_LOG("Restarting Controller");
                    pInstance->reset();
                    pInstance->init();
                    break;
                default:
                    break;
                }
            } else {
                LOG_ERROR("Unknown command : %i", tmp);
            }
        } else if (packetSize == (NUM_MOTORS + 1) * NUM_BYTES_PER_VALUE) {
            Command tmp = (Command) packet[0];
            if (tmp != Command::DEFAULTS) {
                LOG_ERROR("Unknown packet command: %i", (int) tmp);
                return;
            }

            uint16_t data[NUM_MOTORS];
            for (size_t j = 0; j < NUM_MOTORS; ++j) {
                size_t jj = NUM_BYTES_PER_VALUE * (j + 1);
                data[j] = (packet[(jj + 1)] & 0xFF) << 8 | packet[jj];
            }

            pInstance->setInitials(data);

        } else {
            pInstance->m_traj.push(packet, packetSize);
            pInstance->m_bDataRequested = false;
        }
    }

    static void RPDOTimerIRQHandler() {
        static bool errorAtPop = false;
        static ulong idx = 0;

        if (!pInstance->m_bInitialized)
            return;

        Trajectory<int32_t>::point_t point { pInstance->m_currentPoint };

        // If new point is available, grab it. Else keep using last point

        // If there was an error, this new point might not be continuous.
        if (errorAtPop) {
            // TODO: Form a trajectory from current point to this new point and then move using the new point.
            errorAtPop = false;
        } else {
            if (pInstance->m_traj.count() > 0) {
                Trajectory<int32_t>::point_t pt { pInstance->m_currentPoint };
                auto err = pInstance->m_traj.peek(pt);
                if (err)
                    LOG_ERROR("Error peeking trajectory. Code %i", (int) err);

                // If the point is not close to the previous point, generate transition trajectory
                if (!pt.isClose(pInstance->m_currentPoint, DISCONTINUITY_THRESHOLD)) {
                    LOG_WARN("Trajectory discontinuous. Generating Transitions...");
                    // pInstance->m_traj.generateTransitions(pInstance->m_currentPoint, pt, TRANSITION_LENGTH);
                }
                // Pop from traj queue. If transition was added, this point is from the generated transition
                err = pInstance->m_traj.pop(point);
                if (err != kNoError) {
                    errorAtPop = true;
                    LOG_ERROR("Error getting value from trajectory. Code %i", (int) err);
                    // TODO: If we cannot obtain a datapoint, there will be a discontinuity.
                    // In that case, form a trajectory to go to the last point and stay.
                }

            }
        }

        if (idx % 1 == 0) {
            for (int i = 0; i < NUM_MOTORS; ++i) {
                Serial.print(point[i]);
                Serial.print(" ");
            }
            Serial.println("");
        }

        idx += 1;
#ifndef SIMULATE
        // drive actuators here...
        for (int i = 0; i < NUM_MOTORS; ++i)
            pInstance->m_epos[i].PDO_setPosition(point[i]);
#endif

        // Set sendRequest flag if total trajectory time is less than the buffer time.
        pInstance->m_bSendDataRequest = (pInstance->m_traj.count() * PDO_RATE / 1000.f) < BUFFER_TIME;
        pInstance->m_currentPoint = point;
    }

    static void canRxHandle(can_message_t* arg) {
        bool validId = false;
        auto id = arg->id - COB_ID_SDO_SC;
        if (id > 0 && id <= NUM_MOTORS) {
            pInstance->m_epos[id - 1].setRxMsg(*arg);
            validId = true;
        }

        id = arg->id - COB_ID_TPDO3;
        if (id > 0 && id <= NUM_MOTORS) {
            pInstance->m_epos[id - 1].PDO_processMsg(*arg);
            validId = true;
        }

        id = arg->id - COB_ID_EMCY;
        if (id > 0 && id <= NUM_MOTORS) {
            pInstance->m_epos[id - 1].handleEMCYMsg(*arg);
            validId = true;
        }

        // if (!validId)
        //     LOG_ERROR("Invalid Id in msg: %h", (int) arg->id);
    }
};

Hathaani* Hathaani::pInstance = nullptr;