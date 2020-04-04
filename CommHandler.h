//
// Created by Raghavasimhan Sankaranarayanan on 3/23/20.
//

#ifndef VIOLINIST_COMMHANDLER_H
#define VIOLINIST_COMMHANDLER_H

#include "MyDefinitions.h"
#include <mutex>


using std::cout;
using std::cerr;
using std::endl;
using std::string;

class CommHandler {
public:
    static Error_t Create(CommHandler*& pCInstance);
    static Error_t Destroy(CommHandler*& pCInstance);
    Error_t Init();
    Error_t Reset();

    Error_t Send(const Register::Bow& reg, const uint8_t& data);
    Error_t Send(const Register::Fingering& reg, const uint8_t& data);

    static void pprint(const uint8_t* buff);

private:
    Error_t Send(uint8_t* buff);

    std::mutex m_mutex;

    const string m_i2cName = "/dev/i2c-1";
    int m_file_i2c = 0;
    const int m_iSlaveAddress = 0x08;
    bool m_bInitialized = false;


};


#endif //VIOLINIST_COMMHANDLER_H
