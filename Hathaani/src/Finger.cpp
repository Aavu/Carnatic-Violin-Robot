//
// Created by Raghavasimhan Sankaranarayanan on 11/7/21.
//

#include "Finger.h"

Finger::Finger(float D) : m_fD(D), m_pPortHandler(nullptr) {
    m_dxl.reserve(NUM_ACTUATORS);
}

Finger::~Finger() {
    reset();
}

Error_t Finger::moveToPosition(float x, float y, bool bWait) {
    auto ret = calcIK(x, y);
    if (ret != SUCCESS)
        return ret;

    float a[2];
    getDriveAngles(a[0], a[1]);

//    std::cout<< a[0] << " " << a[1] << std::endl;
    return moveJoints(a, bWait, true);
}

Error_t Finger::calcIK(float x, float y) {
    auto j = m_links;

    float phi = std::atan2(y, -x);
    float sq_L01 = sq(x) + sq(y);
    float L01 = std::sqrt(sq_L01);
    float temp = (sq_L01 + sq(j[l14].l) - sq(j[l04].l)) / (2 * L01 * j[l14].l);

    if (std::abs(temp) > 1)
        return kNaNError;
    float alpha = std::acos(temp);

    _theta1 = M_PI - phi - alpha;

    j[l14].update(0, 0, _theta1);
    temp = (sq_L01 - sq(j[l14].l) - sq(j[l04].l)) / (2 * j[l04].l * j[l14].l);
    if (std::abs(temp) > 1)
        return kNaNError;

    float theta4 = std::acos(temp);
    auto x1 = j[l14].x1;
    auto y1 = j[l14].y1;

    j[l04].update(x1, y1, theta4 + _theta1);

    float m = j[l04].m;
    float x0 = j[l04].x;
    float y0 = j[l04].y;

    float c = y - (m * x);

    float A = 1 + sq(m);
    float B = -2 * x0 + 2 * m * c - 2 * y0 * m;
    float C = sq(x0) + sq(c) + sq(y0) - 2 * y0 * c - sq(m_fD);
    float det = sq(B) - 4 * A * C;
    float temp_x = (-B + std::sqrt(det)) / (2 * A);
    float temp_x1 = (-B - std::sqrt(det)) / (2 * A);
    float x5 = (temp_x < x0) ? temp_x : temp_x1;
    float y5 = m * x5 + c;

    float theta5 = std::atan2(y5, x5 + m_fD);
    auto L35 = j[l35].l;
    auto L23 = j[l23].l;
    auto L25 = std::sqrt(sq(y5) + sq(x5 + m_fD));
    temp = (sq(L25) + sq(L23) - sq(L35)) / (2 * L25 * L23);
    if (std::abs(temp) > 1)
        return kNaNError;

    float phi2 = std::acos(temp);

    _theta2 = theta5 + phi2; // Change the sign to invert the inner joint

    j[l23].update(-m_fD, 0, _theta2);
    auto x3 = j[l23].x1;
    auto y3 = j[l23].y1;
    float phi5 = M_PI - std::atan2(y5 - y3, -x5 + x3);
    j[l35].update(x3, y3, phi5);

    _theta1 = M_PI_2 - _theta1;
    _theta2 = M_PI_2 - _theta2;

    if (std::isnan(_theta1) || std::isnan(_theta2)) {
        std::cout<< _theta1 * 180.f/M_PI << " " << _theta2 * 180.f/M_PI << std::endl;
        return kNaNError;
    }

    return kNoError;
}

void Finger::getDriveAngles(float &angle0, float &angle1) const {
    angle0 = _theta1;
    angle1 = _theta2;
}

Error_t Finger::init(PortHandler& portHandler) {
    m_pPortHandler = &portHandler;

    auto* packetHandler = PortHandler::getPacketHandler();
    if (!packetHandler) {
        return kNotInitializedError;
    }

    for (int i=0; i<NUM_ACTUATORS; ++i) {
        m_dxl[i] = Dynamixel(i, *m_pPortHandler, *packetHandler);
        m_dxl[i].operatingMode(OperatingMode::CurrentBasedPositionControl);
        m_dxl[i].setGoalCurrent(MAX_CURRENT);

        if (m_dxl[i].torque() != 0) {
            LOG_ERROR("Cannot set torque on {}", i);
            return kSetValueError;
        }
    }

    return kNoError;
}

Error_t Finger::reset() {
    for (int i=0; i<NUM_ACTUATORS; ++i)
        m_dxl[i].torque(false);

    return kNoError;
}

Error_t Finger::moveJoints(float* pfTheta, bool bWait, bool isRadian) {
    if (pfTheta == nullptr)
        return kFunctionIllegalCallError;

    for (int i=0; i<NUM_ACTUATORS; ++i) {
        auto ret = m_dxl[i].moveToPosition(pfTheta[i], isRadian);
        if (ret != SUCCESS)
            return kSetValueError;
    }

    if (bWait) wait();

    return kNoError;
}

void Finger::wait() {
    while (m_dxl[0].isMoving() || m_dxl[1].isMoving());
}
