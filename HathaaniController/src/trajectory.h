#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <ArduinoQueue.h>
#include <ArduinoEigen.h>
#include "../include/definitions.h"
#include "../include/ErrorDef.h"
#include "util.h"


template<typename T>
class Trajectory {
public:
    struct point_t {
        T data[NUM_MOTORS] {};

        T& operator [](int idx) {
            return data[idx];
        }

        bool operator==(const point_t& other) {
            return isClose(other, 0);
        }

        bool operator!=(const point_t& other) {
            return !this->operator==(other);
        }

        bool isClose(const point_t& other, int threshold = 100) {
            for (int i = 0; i < NUM_MOTORS; ++i)
                if (abs(data[i] - other.data[i]) > threshold) return false;
            return true;
        }
    };

    Trajectory(size_t maxQueueItems = (size_t) -1) :m_queue(maxQueueItems) {

    }

    Error_t push(const char* pPacket, size_t packetSize) {
        /*
        * data structure: n values in little-endian -> [LSB0, MSB0, LSB1, MSB1, ...]
        */

        // packetSize should be NUM_VALUES_PER_DIM * NUM_DIM * NUM_BYTES_PER_VALUE
        if ((packetSize % (NUM_BYTES_PER_VALUE * NUM_MOTORS)) != 0 || packetSize <= 0) {
            return kBufferWriteError;   // Packet corrupted
        }

        size_t numValuesPerDim = packetSize / (NUM_BYTES_PER_VALUE * NUM_MOTORS);

        for (int i = 0; i < numValuesPerDim; ++i) {
            point_t temp;
            int o = i * NUM_BYTES_PER_VALUE * NUM_MOTORS;
            for (size_t j = 0; j < NUM_MOTORS; ++j) {
                size_t jj = NUM_BYTES_PER_VALUE * j;
                temp[j] = (pPacket[o + (jj + 1)] << 8) & (0xFF00) | pPacket[o + jj];
            }

            auto err = push(temp);
            if (err != kNoError) {
                Serial.println("Error pushing trajectory...");
                return err;
            }
        }

        return kNoError;
    }

    Error_t push(point_t& point) {
        if (!m_queue.enqueue(point))
            return kBufferWriteError;
        return kNoError;
    }

    Error_t pushFront(point_t& point) {
        if (!m_queue.pushFront(point))
            return kBufferWriteError;

        return kNoError;
    }

    Error_t pop(point_t& point) {
        if (!m_queue.isEmpty()) {
            point = m_queue.dequeue();
            return kNoError;
        }
        return kBufferReadError;
    }

    Error_t peek(point_t& point) {
        if (!m_queue.isEmpty()) {
            point = m_queue.getHead();
            return kNoError;
        }
        return kBufferReadError;
    }

    size_t count() {
        return m_queue.itemCount();
    }

    bool generateTransitions(const point_t& q0, const point_t& qf, const size_t length) {
        int32_t data[NUM_MOTORS][length];

        for (int i = 0; i < NUM_MOTORS; ++i) {
            bool success = Util::interp(q0.data[i], qf.data[i], length, 0.2, data[i]);
            if (!success) return false;
        }

        // Since we push to front everytime, we traverse from backward of the trajectory
        for (int i = length - 1; i >= 0; --i) {
            point_t p;
            for (int j = 0; j < NUM_MOTORS; ++j)
                p[j] = data[j][i];

            auto e = pushFront(p);
            if (e != kNoError) return e;
        }

        return true;
    }

private:
    ArduinoQueue<point_t> m_queue;
};

#endif //TRAJECTORY_H