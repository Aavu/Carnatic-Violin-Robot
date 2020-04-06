//
// Created by Raghavasimhan Sankaranarayanan on 4/3/20.
//

#ifndef VIOLINIST_TUNER_H
#define VIOLINIST_TUNER_H

#include "../../include/MyDefinitions.h"

class CTuner {
public:
    CTuner();
    ~CTuner();

    static void Create(CTuner*& pCInstance);
    static void Destroy(CTuner*& pCInstance);

    Error_t Init();
    Error_t Reset();

    float GetNoteCorrection();

private:
    bool m_bInitialized = false;
    float m_fNote = 0;

};


#endif //VIOLINIST_TUNER_H
