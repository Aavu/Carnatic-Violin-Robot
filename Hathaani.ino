#include <Arduino.h>
#include "src/logger.h"
#include "src/Hathaani.h"
#include "examples/anandha.h"

// PortHandler portHandler(DXL_DEVICE_NAME);
// Dynamixel* pDxl;

Hathaani* pHathaani;

void setup() {
    LOG_LOG("Welcome");

    // if (portHandler.openPort() != SUCCESS) {
    //     LOG_ERROR("Open Port");
    // }

    // if (portHandler.setBaudRate(DXL_BAUDRATE) != SUCCESS) {
    //     LOG_ERROR("Set BaudRate");
    // }

    // auto* packetHandler = PortHandler::getPacketHandler();
    // if (!packetHandler) {
    //     LOG_ERROR("PacketHandler NULL");
    // }

    // pDxl = new Dynamixel(DxlID::String, portHandler, *packetHandler);

    // pDxl->LED(1);

    // float angle;

    // for (int i = 0; i < 100; ++i) {
    //     pDxl->getCurrentPosition(angle);
    //     Serial.println(angle);
    //     delay(1000);
    // }

    // if (pDxl->torque() != SUCCESS) {
    //     LOG_ERROR("Torque Enable");
    // }

    // if (pDxl->moveToPosition(FINGER_ON) != SUCCESS) {
    //     Serial.println("Error: Move to Position");
    // }

    // delay(10000);

    // if (pDxl->moveToPosition(FINGER_OFF) != SUCCESS) {
    //     Serial.println("Error: Move to Position");
    // }

    // delay(5000);

    // if (pDxl->torque(false) != SUCCESS) {
    //     LOG_ERROR("Torque Disable");
    // }
    // pDxl->LED(0);
    // portHandler.closePort();

    int err;
    pHathaani = Hathaani::create();
    err = pHathaani->init(true);
    performParam_t param;
    param.pitches = phrase::pitch;
    param.length = phrase::nPitches;
    param.bowChangeIdx = phrase::bow;
    param.nBowChanges = phrase::nBowChanges;
    param.amplitude = phrase::amplitude;
    param.volume = 0.5;

    err = pHathaani->perform(param, 0.6);

    pHathaani->reset();
    Hathaani::destroy(pHathaani);
}



void loop() {}

///////////////////////////////////////////////////////////////////
    /* velocity example */

    // err = controller.setOpMode(OpMode::ProfileVelocity);
    // if (err != 0) {
    //     LOG_ERROR("setOpMode");
    // }

    // err = controller.setEnable();
    // if (err != 0) {
    //     LOG_ERROR("setEnable");
    // }

    // err = controller.moveWithVelocity(100);
    // if (err != 0) {
    //     LOG_ERROR("moveWithVelocity");
    // }
    // delay(1000);
    // err = controller.moveWithVelocity(-80);
    // if (err != 0) {
    //     LOG_ERROR("moveWithVelocity");
    // }
    // delay(1000);

    // err = controller.quickStop();
/////////////////////////////////////////////////////////////////

// void setup()
// {
//     Serial.begin(9600);
//     if (portHandler.openPort() != SUCCESS)
//     {
//         Serial.println("Error: Open Port");
//         stopControllerIndefinitely();
//     }

//     if (portHandler.setBaudRate(DXL_BAUDRATE) != SUCCESS)
//     {
//         Serial.println("Error: Open Port");
//         stopControllerIndefinitely();
//     }

//     auto *packetHandler = PortHandler::getPacketHandler();
//     if (!packetHandler)
//     {
//         Serial.println("Error: packetHandler NULL");
//         stopControllerIndefinitely();
//     }

//     pDxl = new Dynamixel(1, portHandler, *packetHandler);
//     pDxl->operatingMode(OperatingMode::CurrentBasedPositionControl);
//     pDxl->setGoalCurrent(MAX_CURRENT);

//     if (pDxl->torque() != SUCCESS)
//     {
//         Serial.println("Error: Torque enable");
//         stopControllerIndefinitely();
//     }
// }

// void loop()
// {

//     if (pDxl->moveToPosition(180.f) != SUCCESS)
//     {
//         Serial.println("Error: Move to Position");
//     }

//     delay(5000);
//     if (pDxl->moveToPosition(0.f) != SUCCESS)
//     {
//         Serial.println("Error: Move to Position");
//     }
//     delay(1000);
//     if (pDxl->torque(false) != SUCCESS)
//     {
//         Serial.println("Error: Torque enable");
//     }
//     else
//     {
//         Serial.println("SUCCESS: Torque disabled");
//     }
//     stopControllerIndefinitely();
// }
