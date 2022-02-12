#include "Hathaani.h"
#include "Util.h"

Hathaani* Hathaani::pInstance = nullptr;

Hathaani::Hathaani() : RPDOTimer(TIMER_CH1),
m_portHandler(DXL_DEVICE_NAME)//, m_bowController(m_portHandler) 
{
    m_pFingerController = FingerController::create(m_portHandler);
}

Hathaani* Hathaani::create() {
    pInstance = new Hathaani();
    return pInstance;
}

void Hathaani::destroy(Hathaani* pInstance) {
    delete pInstance;
}

int Hathaani::init(bool shouldHome) {
    pInstance = this;
    // Comm
    if (!CanBus.begin(CAN_BAUD_1000K, CAN_STD_FORMAT)) {
        LOG_ERROR("CAN open failed");
        return 1;
    }

    CanBus.attachRxInterrupt(canRxHandle);

    if (m_portHandler.openPort() != 0) {
        LOG_ERROR("Open Port");
        return 1;
    }

    if (m_portHandler.setBaudRate(DXL_BAUDRATE) != 0) {
        LOG_ERROR("Set BaudRate");
        return 1;
    }

    // Bow
    // int err = m_bowController.init();
    // if (err != 0) {
    //     LOG_ERROR("Bow Controller Init failed");
    //     return 1;
    // }

    // Finger
    RPDOTimer.stop();
    RPDOTimer.setPeriod(PDO_RATE * 1000);
    RPDOTimer.attachInterrupt(RPDOTimerIRQHandler);

    return m_pFingerController->init(shouldHome);
}

int Hathaani::reset() {
    int err;
    // err = m_bowController.reset();

    err = m_pFingerController->reset();
    FingerController::destroy(m_pFingerController);

    CanBus.end();

    return err;
}

int Hathaani::enableEncoderTransmission(bool bEnable) {
    return m_pFingerController->enablePDO(bEnable);
}

int Hathaani::perform(const performParam_t& param, float lpf_alpha) {
    int err;

    // Shallow copy
    m_performParam = param;

    m_performParam.positions = new int32_t[m_performParam.length];
    Util::cleanUpData(m_performParam.pitches, m_performParam.length, lpf_alpha);
    Util::convertPitchToInc(m_performParam.positions, m_performParam.pitches, m_performParam.length, 0);
    // m_performParam.length += kOffset;

    // for (int i = 0; i < m_performParam.length; ++i) {
    //     Serial.println(m_performParam.positions[i]);
    //     // Serial.print(",");
    // }
    // Serial.println();

    if (!Util::checkFollowError(m_performParam.positions, m_performParam.length)) {
        LOG_ERROR("Playing this phrase will violate max follow window... Aborting");
        err = 1;
        goto cleanup;
    }

    err = m_pFingerController->fingerOff();
    if (err != 0) {
        LOG_ERROR("fingerOff");
        goto cleanup;
    }
    m_bFingerOn = false;

    err = m_pFingerController->prepareToPlay(m_performParam.pitches[0]);
    delay(100);

    /*********** Switch to Cyclic Position to use PDO ***********/
    // err = m_pFingerController->getReadyToPlay();
    // if (err != 0) {
    //     LOG_ERROR("getReadyToPlay");
    //     goto cleanup;
    // }

    // err = m_bowController.prepareToPlay();

    delay(100);
    m_bPlaying = true;
    RPDOTimer.start();

    // Wait for the performance to complete
    while (m_bPlaying) {
        delay(PDO_RATE);
    }

    RPDOTimer.stop();
    delay(100);

    err = m_pFingerController->fingerOff();
    if (err != 0)
        LOG_ERROR("fingerOff");
    m_bFingerOn = false;

    // err = m_bowController.reset();

cleanup:
    delete[] m_performParam.positions;
    return err;
}

void Hathaani::RPDOTimerIRQHandler() {
    static int32_t i = 0;

    performParam_t& param = pInstance->m_performParam;
    // LOG_LOG("%i: IRQ Time: %i", i, millis());

    // LOG_LOG("%i", param.positions[i]);
    if (i < param.length) {
        // pInstance->m_pFingerController->moveToPosition(param.positions[i]);
        // pInstance->m_bowController.setVelocity(100);
        ++i;
    } else {
        // pInstance->m_pFingerController->moveToPosition(param.positions[i - 1]);
        // pInstance->m_bowController.setVelocity(100);
        pInstance->m_bPlaying = false;
    }

}

void Hathaani::canRxHandle(can_message_t* arg) {
    auto id = arg->id - COB_ID_SDO_SC;
    // TODO: Improve Implementation
    // Bow
    if (id == BOW_NODE_ID) {
        // pInstance->m_bowController.setRxMsg(*arg);
    }

    if (arg->id == COB_ID_TPDO3 + BOW_NODE_ID) {
        // pInstance->m_bowController.PDO_processMsg(*arg);
    }

    if (arg->id == COB_ID_EMCY + BOW_NODE_ID) {
        // pInstance->m_bowController.handleEMCYMsg(*arg);
    }

    // Finger
    if (id == FINGER_NODE_ID) {
        pInstance->m_pFingerController->setRxMsg(*arg);
    }

    if (arg->id == COB_ID_TPDO3 + FINGER_NODE_ID) {
        pInstance->m_pFingerController->PDO_processMsg(*arg);

        // Serial.print("ID : ");
        // Serial.print(arg->id, HEX);
        // Serial.print(", Length : ");
        // Serial.print(arg->length);
        // Serial.print(", Data : ");
        // for (int i = 0; i < arg->length; i++) {
        //     Serial.print(arg->data[i], HEX);
        //     Serial.print(" ");
        // }
        // Serial.println();

    }

    if (arg->id == COB_ID_EMCY + FINGER_NODE_ID) {
        pInstance->m_pFingerController->handleEMCYMsg(*arg);
    }
}
