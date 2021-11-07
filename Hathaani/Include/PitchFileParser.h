//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#ifndef HATHAANI_PITCHFILEPARSER_H
#define HATHAANI_PITCHFILEPARSER_H

#include <fstream>
#include <memory>
#include <vector>

#include "MyDefinitions.h"
#include "rapidjson/document.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;

class PitchFileParser {
public:
    inline static const std::string kName = "PitchFileParser";

    explicit PitchFileParser(string  filePath);
    ~PitchFileParser();

    Error_t Set(const string& filePath, const Error_t&  error = kNoError);
    Error_t GetLength(size_t& length, const Error_t&  error = kNoError);
    Error_t GetPitches(double* pitches, const size_t &length = 0, const Error_t& error = kNoError);

    Error_t parseJson(std::vector<float>& pitches, std::vector<size_t>& bowChanges, std::vector<float>& amplitude);
private:
    Error_t readPitches();
    Error_t readSize();

    size_t m_iLength = 0;
    ifstream m_file;
    string m_sFilePath;
    bool m_bSetPath;

//    std::unique_ptr<double>(m_fPitches);
    double* m_fPitches;
};


#endif //HATHAANI_PITCHFILEPARSER_H
