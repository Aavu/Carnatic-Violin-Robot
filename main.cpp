//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#include "Violinist.h"

using namespace std;

//#define SET_HOME
//#define USE_FINGER

int main(int argc, char** argv) {
    Error_t lResult;
    Violinist violinist;

    float fretPosition = 0; // 0 - 16

#ifdef SET_HOME
    if ((lResult = violinist.Prepare(&fretPosition, true)) != kNoError) {
#else
    if ((lResult = violinist.Prepare(&fretPosition)) != kNoError) {
#endif //SET_HOME
        violinist.LogError("Main Prepare", lResult, violinist.GetErrorCode());
        return lResult;
    }

//    violinist.finger->on();
    float i = 0;
    while(i < 10) {
        fretPosition = i;
        std::this_thread::sleep_for(std::chrono::microseconds (500));
        i += 0.01;
    }

    while(i > 0) {
        fretPosition = i;
        std::this_thread::sleep_for(std::chrono::microseconds (500));
        i -= 0.01;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//    violinist.finger->rest();

//    if ((lResult = violinist.Play()) != kNoError) {
//        violinist.LogError("Play", lResult, violinist.GetErrorCode());
//        return lResult;
//    }

    if ((lResult = violinist.CloseDevice()) != kNoError) {
        violinist.LogError("CloseDevice", lResult, violinist.GetErrorCode());
        return lResult;
    }

    return 0;
}