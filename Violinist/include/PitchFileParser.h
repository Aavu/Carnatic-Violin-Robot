//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#ifndef VIOLINIST_PITCHFILEPARSER_H
#define VIOLINIST_PITCHFILEPARSER_H

#include "../../include/MyDefinitions.h"
#include <fstream>
#include <memory>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;

class PitchFileParser {
public:
    PitchFileParser();
    ~PitchFileParser();

    Error_t Set(const string& filePath, const Error_t&  error = kNoError);
    Error_t GetLength(size_t& length, const Error_t&  error = kNoError);
    Error_t GetPitches(double* pitches, const size_t &length = 0, const Error_t& error = kNoError);

private:
    Error_t readPitches();
    Error_t readSize();

    size_t m_iLength;
    ifstream m_file;
    string m_sFilePath;
    bool m_bSetPath;

//    std::unique_ptr<double>(m_fPitches);
    double* m_fPitches;
};


#endif //VIOLINIST_PITCHFILEPARSER_H
