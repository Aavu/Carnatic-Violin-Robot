//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#ifndef HATHAANI_COMMHANDLER_H
#define HATHAANI_COMMHANDLER_H

#include <mutex>

#ifdef __arm__
#include <bcm2835.h>
#endif // __arm__

#include "Logger.h"
#include "MyDefinitions.h"
#include "Util.h"

#include "Dynamixel.h"

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
#ifdef __arm__
        if (m_protocol == SPI)
            bcm2835_spi_end();
        bcm2835_close();
#endif // __arm__
        m_bInitialized = false;
    }

//    static Error_t Create(CommHandler*& pCInstance) {
//        pCInstance = new CommHandler();
//        if (!pCInstance)
//            return kMemError;
//        return kNoError;
//    }

//    static Error_t Destroy(CommHandler*& pCInstance) {
//        pCInstance->reset();
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
            return kNotInitializedError;
        uint8_t _buf[2] = {static_cast<uint8_t>(reg), data};
        return Send(_buf);
    }

    Error_t Send(const Register::Fingering& reg, const uint8_t& data) {
        if (!m_bInitialized)
            return kNotInitializedError;
        uint8_t _buf[2] = {static_cast<uint8_t>(reg), data};
        return Send(_buf);
    }

    static void pprint(const uint8_t* buff) {
        string msg1;
        switch (buff[0]) {
            case Register::kFinger:
                msg1 = "Finger";
                break;
            case Register::kPitch:
                msg1 = "Pitch";
                break;
            default:
                cout << (int) buff[0] << " : " << (int) buff[1] << endl;
                return;
        }
        cout << msg1 << " : " << (int) buff[1] << endl;
    }

    [[nodiscard]] bool isInitialized() const {
        return m_bInitialized;
    }

    PortHandler* getDxlPortHandler() {
        return m_pPortHandler;
    }

private:
    Error_t Init() {
        m_pPortHandler = new PortHandler(DXL_DEVICE_NAME);
        if (m_pPortHandler->openPort() != SUCCESS) {
            LOG_ERROR("Open Port Handler Failed...");
            return kFileOpenError;
        }

        if (m_pPortHandler->setBaudRate(DXL_BAUDRATE) != 0) {
            LOG_ERROR("Cannot set baudrate...");
            return kSetValueError;
        }
#ifdef __arm__
        Error_t err;
        if (m_protocol == I2C) {
            if ((m_file_i2c = open((char*)m_i2cName.c_str(), O_RDWR)) < 0) {
                LOG_ERROR("Failed to open the i2c bus");
                return kFileOpenError;
            }

            if (ioctl(m_file_i2c, I2C_SLAVE, m_iSlaveAddress) < 0) {

                LOG_ERROR("Failed to acquire bus access and/or talk to slave.");
                return kFileAccessError;
            }
        } else {
            if (!bcm2835_init()) {
                err = kNotInitializedError;
                LOG_ERROR("bcm2835_init");
                return err;
            }

            if (!bcm2835_spi_begin()) {
                err = kNotInitializedError;
                LOG_ERROR("bcm2835_spi_begin");
                return err;
            }

            bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
            bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
            bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16384); // Dont go faster than this
        }
#endif // __arm__
        return kNoError;
    }

    Error_t Send(const uint8_t* buff) {
#ifdef __arm__
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_protocol == I2C) {
//            pprint(buff);
            if (write(m_file_i2c, (char *) buff, 2) != 2) {
                LOG_ERROR("Failed to write to the i2c bus.");
                return kFileWriteError;
            }
        } else {
//            pprint(buff);
            bcm2835_spi_transfern((char *) buff, 2);
        }
#endif // __arm__
        return kNoError;
    }

    std::mutex m_mutex{};
    Protocol m_protocol = SPI;
    const string m_i2cName = "/dev/i2c-1";
    int m_file_i2c = 0;
    const int m_iSlaveAddress = 0x08;

    bool m_bInitialized = false;

    PortHandler* m_pPortHandler = nullptr;
};


#endif //HATHAANI_COMMHANDLER_H
