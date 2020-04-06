//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//
#include <cstring>
#include <fstream>
#include <iostream>

#include "Violinist.h"
#include "PitchFileParser.h"

#define TRANSPOSE 2

//#define SET_HOME
 
using namespace std;

int main(int argc, char **argv) {
    Error_t lResult = kNoError;
    PitchFileParser pitchFileParser;
    pitchFileParser.Set("pitches.txt");
    size_t length = 0;
    lResult = pitchFileParser.GetLength(length, lResult);
    std::unique_ptr<double> pitches(new double[length]);
    lResult = pitchFileParser.GetPitches(pitches.get(), lResult);

    if (lResult != kNoError)
        return 1;

    cout << "Read pitches successfully!\n";

    Violinist violinist;

#ifdef SET_HOME
    if ((lResult = violinist.Init(true)) != kNoError) {
        violinist.LogError("Main Init", lResult, violinist.GetErrorCode());
        return lResult;
    }

    if ((lResult = violinist.CloseDevice()) != kNoError) {
        violinist.LogError("CloseDevice", lResult, violinist.GetErrorCode());
        return lResult;
    }
    return 0;
#else
    if ((lResult = violinist.Init()) != kNoError) {
        violinist.LogError("Main Init", lResult, violinist.GetErrorCode());
        return lResult;
    }
#endif //SET_HOME

//    violinist.Perform(pitches.get(), length, TRANSPOSE);
    violinist.Perform(Violinist::Key::C_sharp, Violinist::Mode::Minor, 1000, 0);

    if ((lResult = violinist.CloseDevice()) != kNoError) {
        violinist.LogError("CloseDevice", lResult, violinist.GetErrorCode());
        return lResult;
    }

    return 0;
}
