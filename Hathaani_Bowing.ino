#include "src/BowController.h"
#include "src/def.h"
#include <vector>
#include <map>

// Test data
const int nBowChanges = 12;
const int numPitchValues = 600;
int bowChanges[nBowChanges] = { 100, 150, 200, 225, 250, 275, 300, 320, 340, 360, 380, 400 };
int stringId[numPitchValues];

bool bPlaying = false;
BowController* pBowController = nullptr;
HardwareTimer RPDOTimer(TIMER_CH1);

void setup() {
    // init test data
    memset(stringId, 0, numPitchValues * sizeof(int));
    for (int i = 0; i < 280; ++i) {
        stringId[i] = 1;
    }

    for (int i = 280; i < numPitchValues; ++i) {
        stringId[i] = 0;
    }

    // Comm
    if (!CanBus.begin(CAN_BAUD_1000K, CAN_STD_FORMAT)) {
        LOG_ERROR("CAN open failed");
        return;
    }

    CanBus.attachRxInterrupt(canRxHandle);

    PortHandler portHandler(DXL_DEVICE_NAME);

    if (portHandler.openPort() != 0) {
        LOG_ERROR("Open Port");
        return;
    }

    if (portHandler.setBaudRate(DXL_BAUDRATE) != 0) {
        LOG_ERROR("Set BaudRate");
        return;
    }

    pBowController = BowController::create(portHandler);

    int err = pBowController->init(true);
    if (err != 0) {
        LOG_ERROR("Bow Controller Init failed");
        return;
    }

    err = pBowController->prepareToPlay(bowChanges, nBowChanges, numPitchValues, stringId);

    RPDOTimer.stop();
    RPDOTimer.setPeriod(PDO_RATE * 1000);
    RPDOTimer.attachInterrupt(RPDOTimerIRQHandler);

    bPlaying = true;
    RPDOTimer.start();

    // Wait for the performance to complete
    while (bPlaying) {
        delay(PDO_RATE);
    }

    pBowController->enablePDO(false);
    RPDOTimer.stop();
    delay(10);

    err = pBowController->reset();
    BowController::destroy(pBowController);
    pBowController = nullptr;
    CanBus.end();
}

void loop() {
    // put your main code here, to run repeatedly:

}

void canRxHandle(can_message_t* arg) {
    if (!pBowController) return;

    auto id = arg->id - COB_ID_SDO_SC;
    // TODO: Improve Implementation
    // Bow
    if (id == BOW_NODE_ID) {
        pBowController->setRxMsg(*arg);
    }

    if (arg->id == COB_ID_TPDO3 + BOW_NODE_ID) {
        pBowController->PDO_processMsg(*arg);
    }

    if (arg->id == COB_ID_EMCY + BOW_NODE_ID) {
        pBowController->handleEMCYMsg(*arg);
    }
}

void RPDOTimerIRQHandler() {
    static int i = 0;
    static int32_t lastPos = 0;
    int kTotalLength = numPitchValues;
    // int32_t pos = BOW_ENCODER_MIN + param.bowTraj[i] * (BOW_ENCODER_MAX - BOW_ENCODER_MIN);
    pBowController->updatePosition(i);
    ++i;
    // pInstance->m_pBowController->setPosition(pos, false, true);
    if (i >= kTotalLength) {
        bPlaying = false;
    }

    if (i >= kTotalLength) {
        bPlaying = false;
        i = kTotalLength - 1;
    }
}
