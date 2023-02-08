#include <Arduino.h>
#include "src/logger.h"
#include "src/Hathaani.h"
#include "examples/anandha.h"

Hathaani* pHathaani;

int getch() {
    while (1) {
        delay(10);
        if (Serial.available() > 0) {
            break;
        }
    }

    return Serial.read();
}

void setup() {
    LOG_LOG("Welcome");
    int err;
    pHathaani = Hathaani::create();
    err = pHathaani->init(true);
    performParam_t param;
    param.pitches = phrase::pitch;
    param.length = phrase::nPitches;
    param.bowTraj = phrase::bow;
    param.bowChanges = phrase::bowChanges;
    param.nBowChanges = phrase::nBowChanges;
    param.amplitude = phrase::amplitude;
    param.volume = 0.5;

    // err = pHathaani->perform(param, 0.5, true);
    err = pHathaani->bowTest(param);

    while (1) {
        Serial.print("press q to quit!\n");
        if (getch() == 'q')
            break;
    }

    pHathaani->reset();
    Hathaani::destroy(pHathaani);
}

void loop() {}
