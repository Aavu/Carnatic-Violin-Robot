//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#include "CommHandler.h"

Error_t CommHandler::Create(CommHandler *&pCInstance) {
    pCInstance = new CommHandler();
    if (!pCInstance)
        return kMemError;
    return kNoError;
}

Error_t CommHandler::Destroy(CommHandler *&pCInstance) {
    pCInstance->Reset();
    delete pCInstance;
    return kNoError;
}

Error_t CommHandler::Init() {
    if ((m_file_i2c = open((char*)m_i2cName.c_str(), O_RDWR)) < 0) {
        cerr << "Failed to open the i2c bus" << endl;
        return kFileOpenError;
    }

    if (ioctl(m_file_i2c, I2C_SLAVE, m_iSlaveAddress) < 0) {
        cerr << "Failed to acquire bus access and/or talk to slave." << endl;
        return kFileAccessError;
    }

    m_bInitialized = true;
    return kNoError;
}

Error_t CommHandler::Reset() {
    m_bInitialized = false;
    return kNoError;
}

Error_t CommHandler::Send(const Register::Bow& reg, const uint8_t& data) {
    if (!m_bInitialized)
        return kNotInitializedError;

    uint8_t _buf[2] = {static_cast<uint8_t>(reg), data};
    return Send(_buf);
}

Error_t CommHandler::Send(const Register::Fingering& reg, const uint8_t& data) {
    if (!m_bInitialized)
        return kNotInitializedError;
    uint8_t _buf[2] = {static_cast<uint8_t>(reg), data};
    return Send(_buf);
}

Error_t CommHandler::Send(uint8_t *buff) {
    std::lock_guard<std::mutex> locker(m_mutex);
    if (write(m_file_i2c, (char*)buff, 2) != 2) {
        cerr << "Failed to write to the i2c bus.\n";
        return kFileWriteError;
    }
//    pprint(buff);
    return kNoError;
}

void CommHandler::pprint(const uint8_t* buff) {
    string msg1;
    switch (buff[0]) {
        case Register::kFinger:
            msg1 = "Finger";
            break;
        case Register::kRoller:
            msg1 = "Roller";
            break;
        case Register::kPitch:
            msg1 = "Pitch";
            break;
        case Register::kRollerEnable:
            msg1 = "Roller Enable";
            break;
        case Register::kSurge:
            msg1 = "Surge";
            break;
        case Register::kSurgeMultiplier:
            msg1 = "Surge Multiplier";
            break;
        default:
            msg1 = "Num Registers";
    }
    cout << msg1 << " : " << (int) buff[1] << endl;
}

int sign(double value) {
    if (value > 0)
        return 1;
    else if (value < 0)
        return -1;
    return 0;
}
