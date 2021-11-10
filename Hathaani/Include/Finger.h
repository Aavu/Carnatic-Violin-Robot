//
// Created by Raghavasimhan Sankaranarayanan on 11/7/21.
//

#ifndef HATHAANI_FINGER_H
#define HATHAANI_FINGER_H

#include <cmath>
#include "ErrorDef.h"

#include "cmath"
#include "vector"

#include "Dynamixel.h"

#ifndef FAIL
#define FAIL 1
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#define l14 0
#define l04 1
#define l23 2
#define l35 3

#define NUM_ACTUATORS 2

#define MAX_CURRENT 50

#define DEVICENAME  "/dev/tty.usbserial-FT45BADF"

class Link
{
public:
    Link(float _x, float _y, float len, float angle, bool radians=true):  x(_x), y(_y), l(len),
                                                                          angle((radians) ? angle : deg2rad(angle)) {
        updateTail();
    }

    ~Link() = default;

    void updateTail() {
        x1 = x + l * std::cos(angle);
        y1 = y + l * std::sin(angle);
        m = slope();
    }

    void update(float _x, float _y, float _angle, bool radians = true) {
        x = _x;
        y = _y;
        angle = (radians) ? angle : deg2rad(angle);
        updateTail();
    }

    void update(float _x, float _y, float _x1, float _y1) {
        x = _x;
        y = _y;
        x1 = _x1;
        y1 = _y1;
        m = slope();
    }

    static float deg2rad(float deg) {
        return M_PI * deg / 180.0;
    }

    [[nodiscard]] float slope() const { return (y - y1) / (x - x1); }

public:
    float x, y, x1 = 0, l=10, y1 = 0, angle, m = 0;
};

class Finger {
public:
    explicit Finger(float D = 32);
    ~Finger();

    Error_t init(PortHandler& portHandler);
    Error_t reset();

    Error_t calcIK(float x, float y);
    Error_t moveToPosition(float x, float y);
    Error_t moveJoints(float* pfTheta, bool isRadian = false);
    void getDriveAngles(float& angle0, float& angle1) const;

    static float map(float x, float in_min, float in_max, float out_min, float out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    static float sq(float x) {
        return powf(x, 2);
    }

private:
    float m_fD;
    float _theta1 = 0, _theta2 = 0;

    Link m_links[4] = {
            Link(0, 0, 60, 0.f),
            Link(0, 0, 160, 0.f),
            Link(0, 0, 24, 0.f),
            Link(0, 0, 36, 0.f)
    };

    bool m_bInitialized = false;

    PortHandler* m_pPortHandler;
    std::vector<Dynamixel> m_dxl;
};

#endif //HATHAANI_FINGER_H
