//
// Created by Raghavasimhan Sankaranarayanan on 2020-01-14.
//

#ifndef VIOLINIST_MYDEFINITIONS_H
#define VIOLINIST_MYDEFINITIONS_H

#include "ErrorDef.h"

#ifndef MMC_SUCCESS
#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
#define MMC_FAILED 1
#endif

static const int SCALE_LENGTH = 310;  //mm
static const int MAX_ENCODER_INC = -23000;
static const int MAX_ALLOWED_VALUE = -20000;
static const int NUT_POSITION = -1000; //-1600 without block bolt
static const int OPEN_STRING = NUT_POSITION;

#endif //VIOLINIST_MYDEFINITIONS_H
