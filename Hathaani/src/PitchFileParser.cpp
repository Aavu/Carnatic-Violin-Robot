//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#include "PitchFileParser.h"

#include <utility>

PitchFileParser::PitchFileParser(string filePath) : m_sFilePath(std::move(filePath)), m_bSetPath(false), m_fPitches(nullptr) {
    m_file.open(m_sFilePath);
    if (!m_file.is_open()) {
        std::cerr << "Failed to open " << m_sFilePath << endl;
    }
    m_bSetPath = true;
}

PitchFileParser::~PitchFileParser() {
    if (m_file.is_open())
        m_file.close();

    delete[] m_fPitches;
}

Error_t PitchFileParser::Set(const string& filePath, const Error_t& error) {
    if (error != kNoError)
        return error;
    m_sFilePath = filePath;
    m_file.open(m_sFilePath);
    if (!m_file.is_open()) {
        std::cerr << "Failed to open " << m_sFilePath << endl;
        return kFileOpenError;
    }
    m_bSetPath = true;
    return kNoError;
}

Error_t PitchFileParser::readSize() {
    if (!m_bSetPath)
        return kNotInitializedError;
    string header;
    if (!std::getline(m_file, header)) {
        std::cerr << "Failed to read line\n";
        return kUnknownError;
    }
    m_iLength = stoi(header.substr(2, header.size()));
    return kNoError;
}

Error_t PitchFileParser::readPitches() {
    if (!m_bSetPath)
        return kNotInitializedError;

    if (!m_iLength) {
        auto err = readSize();
        if (err != kNoError)
            return err;
    }

    delete[] m_fPitches;
    m_fPitches = new double[m_iLength];

    for (size_t i = 0; i < m_iLength; i++) {
        string value;
        if (std::getline(m_file, value))
            m_fPitches[i] = stod(value);
        else
            return kUnknownError;
    }

    return kNoError;
}

Error_t PitchFileParser::GetPitches(double *pitches, const size_t &length, const Error_t& error) {
    if (error != kNoError)
        return error;

    if (!m_fPitches){
        auto err = readPitches();
        if (err != kNoError)
            return err;
    }

    if (!pitches)
        return kFunctionIllegalCallError;

    auto _n = length;

    if (length == 0 || length > m_iLength)
        _n = m_iLength;

//    memcpy(pitches, m_fPitches, _n*sizeof(double));
    for (size_t i=0; i < _n; i++)
        pitches[i] = m_fPitches[i];

    return kNoError;
}

Error_t PitchFileParser::GetLength(size_t& length, const Error_t& error) {
    if (error != kNoError)
        return error;
    if (!m_iLength) {
        auto err = readSize();

        if (err != kNoError)
            return err;
    }
    length = m_iLength;
    return kNoError;
}

Error_t PitchFileParser::parseJson(std::vector<float>& pitches, std::vector<size_t>& bowChanges, std::vector<float>& amplitude)
{
    if (!m_file.is_open())
        return kFileOpenError;

    using namespace rapidjson;
    Document map;
    string strObj;
    std::getline(m_file, strObj, '\0');

    if (map.Parse(strObj.c_str()).HasParseError())
        return kFileParseError;

    assert(map.HasMember("pitch"));
    assert(map["pitch"].IsArray());
    const Value& pitch = map["pitch"];
    pitches.resize(pitch.Size());
    for (SizeType i = 0; i < pitch.Size(); ++i) {
        pitches[i] = pitch[i].GetFloat();
    }

    assert(map.HasMember("bow"));
    assert(map["bow"].IsArray());
    const Value& bow = map["bow"];
    bowChanges.resize(bow.Size());
    for (SizeType i = 0; i < bow.Size(); ++i) {
        bowChanges[i] = bow[i].GetInt();
    }

    assert(map.HasMember("amplitude"));
    assert(map["amplitude"].IsArray());
    const Value& amp = map["amplitude"];
    amplitude.resize(amp.Size());
    for (SizeType i = 0; i < amp.Size(); ++i) {
        amplitude[i] = amp[i].GetFloat();
    }

    return kNoError;
}

