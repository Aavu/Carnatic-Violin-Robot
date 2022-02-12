//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#include "PitchFileParser.h"

PitchFileParser::PitchFileParser() : m_iLength(0), m_bSetPath(false), m_fPitches(nullptr) {

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

