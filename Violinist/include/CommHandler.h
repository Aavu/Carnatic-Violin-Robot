//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#ifndef VIOLINIST_COMMHANDLER_H
#define VIOLINIST_COMMHANDLER_H

#include <mutex>
#include <bcm2835.h>

#include "MyDefinitions.h"
#include "Util.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

class CommHandler {
public:
    inline static const std::string kName = "CommHandler";

    enum Protocol {
        SPI, I2C
    };

    ~CommHandler() {
        if (m_protocol == SPI)
            bcm2835_spi_end();
        bcm2835_close();
        m_bInitialized = false;
    }

//    static Error_t Create(CommHandler*& pCInstance) {
//        pCInstance = new CommHandler();
//        if (!pCInstance)
//            return kMemError;
//        return kNoError;
//    }

//    static Error_t Destroy(CommHandler*& pCInstance) {
//        pCInstance->Reset();
//        delete pCInstance;
//        pCInstance = nullptr;
//        return kNoError;
//    }

    explicit CommHandler(Protocol protocol = I2C) : m_protocol(protocol) {
        if (Init() == kNoError)
            m_bInitialized = true;
    }

    Error_t Send(const Register::Bow& reg, const uint8_t& data) {
        if (!m_bInitialized)
            return Util::Return(kNotInitializedError);
        uint8_t _buf[2] = {static_cast<uint8_t>(reg), data};
        return Send(_buf);
    }

    Error_t Send(const Register::Fingering& reg, const uint8_t& data) {
        if (!m_bInitialized)
            return Util::Return(kNotInitializedError);
        uint8_t _buf[2] = {static_cast<uint8_t>(reg), data};
        return Send(_buf);
    }

    static void pprint(const uint8_t* buff) {
        string msg1;
        switch (buff[0]) {
            case Register::kFinger:
                msg1 = "Finger";
                break;
//            case Register::kRoller:
//                msg1 = "Roller";
//                break;
            case Register::kPitch:
                msg1 = "Pitch";
                break;
//            case Register::kRollerEnable:
//                msg1 = "Roller Enable";
//                break;
//            case Register::kSurge:
//                msg1 = "Surge";
//                break;
//            case Register::kSurgeMultiplier:
//                msg1 = "Surge Multiplier";
//                break;
            default:
                cout << (int) buff[0] << " : " << (int) buff[1] << endl;
                return;
        }
        cout << msg1 << " : " << (int) buff[1] << endl;
    }

private:
    Error_t Init() {
        Error_t err;
        if (m_protocol == I2C) {
            if ((m_file_i2c = open((char*)m_i2cName.c_str(), O_RDWR)) < 0) {
                cerr << "Failed to open the i2c bus" << endl;
                return Util::Return(kFileOpenError);
            }

            if (ioctl(m_file_i2c, I2C_SLAVE, m_iSlaveAddress) < 0) {
                cerr << "Failed to acquire bus access and/or talk to slave." << endl;
                return Util::Return(kFileAccessError);
            }
        } else {
            if (!bcm2835_init()) {
                err = kNotInitializedError;
                CUtil::PrintError("bcm2835_init", err);
                return Util::Return(err);
            }

            if (!bcm2835_spi_begin()) {
                err = kNotInitializedError;
                CUtil::PrintError("bcm2835_spi_begin", err);
                return Util::Return(err);
            }

            bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
            bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
            bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16384); // Dont go faster than this
        }

        return Util::Return();
    }

    Error_t Send(const uint8_t* buff) {
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_protocol == I2C) {
            pprint(buff);
            if (write(m_file_i2c, (char *) buff, 2) != 2) {
                cerr << "Failed to write to the i2c bus.\n";
                return Util::Return(kFileWriteError);
            }
        } else {
//            pprint(buff);
            bcm2835_spi_transfern((char *) buff, 2);
        }
        return Util::Return();
    }

    std::mutex m_mutex{};
    Protocol m_protocol = SPI;
    const string m_i2cName = "/dev/i2c-1";
    int m_file_i2c = 0;
    const int m_iSlaveAddress = 0x08;

    bool m_bInitialized = false;
};


#endif //VIOLINIST_COMMHANDLER_H
