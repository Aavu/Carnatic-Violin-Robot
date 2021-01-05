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
//    PitchFileParser pitchFileParser;
//    pitchFileParser.Set("pitches.txt");
//    size_t length = 0;
//    lResult = pitchFileParser.GetLength(length, lResult);
//    std::unique_ptr<double> pitches(new double[length]);
//    lResult = pitchFileParser.GetPitches(pitches.get(), lResult);
//
//    if (lResult != kNoError) {
//        CUtil::PrintError(PitchFileParser::kName, lResult);
//        return 1;
//    }

    cout << "Read pitches successfully!\n";

    Violinist violinist;

    if ((lResult = violinist.Init()) != kNoError) {
        Violinist::LogError("Main Init", lResult, violinist.GetErrorCode());
        return lResult;
    }
#ifdef SET_HOME
    return 0;
#endif //SET_HOME

//    violinist.Perform(pitches.get(), length, TRANSPOSE);
    violinist.Perform(Violinist::Key::C_sharp, Violinist::Mode::Lydian, 1000, 0.5);

//    if ((lResult = violinist.Play(0.55)) != kNoError) {
//        Violinist::LogError("Play", lResult, violinist.GetErrorCode());
//        return lResult;
//    }
//    if ((lResult = violinist.Perform(Violinist::Kampita, 3, 400, 0.5)) != kNoError)
//    {
//        Violinist::LogError("Play", lResult, violinist.GetErrorCode());
//        return lResult;
//    }

    return 0;
}
