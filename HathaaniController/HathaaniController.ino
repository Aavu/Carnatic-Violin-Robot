#include<Arduino.h>
#include "include/definitions.h"
#include "include/ErrorDef.h"
#include "src/hathaani.h"

Hathaani* pHathaani;

void setup() {
    LOG_LOG("Hathaani Ethernet v0.1");

    pHathaani = Hathaani::create();
    Error_t e = pHathaani->init();

    if (e != kNoError) {
        LOG_ERROR("Error initializing socket. code: %i", (int) e);
        while (1) {}
    }
}

void loop() {
    pHathaani->poll();
    delay(1);
}
