#include "Util.h"

using namespace Util;

void Util::zeroPhaseLPF(float* p, int nPitches, float a) {
    for (int i = 1; i < nPitches; ++i) {
        p[i] = (a * p[i - 1]) + ((1 - a) * p[i]);
    }

    for (int i = nPitches - 2; i > -1; --i) {
        p[i] = (a * p[i + 1]) + ((1 - a) * p[i]);
    }
}

void Util::interpolateGaps(float* p, int nPitches) {
    int startIdx = -1;
    int stopIdx = -1;
    bool bInterpolate = false;
    bool bTracking = false;

    for (int i = 0; i < nPitches; ++i) {
        if (p[i] <= 0) {
            if (!bTracking) {
                startIdx = i - 1;    // Store the last non neg idx
                bTracking = true;
            }
        } else {
            if (bTracking) {
                stopIdx = i;
                bInterpolate = true;
                bTracking = false;
            }
        }

        if (bInterpolate) {
            // eq of line : (y - y1) = m(x - x1)
            float m = (p[stopIdx] - p[startIdx]) / (stopIdx - startIdx);

            for (int j = startIdx + 1; j < stopIdx; ++j)
                p[j] = p[startIdx] + (m * (j - startIdx));

            bInterpolate = false;
        }
    }

}

void Util::trimEndZeros(float* p, int& nPitches) {
    int length = 0;
    // Trim End Zeros
    for (int i = 0; i < nPitches; ++i) {
        if (abs(p[i]) > 1e-6)
            length = i + 1;
    }

    nPitches = length;
    // Trim Start Zeros
    for (int i = 0; i < nPitches; ++i) {
        if (abs(p[i]) > 1e-6) {
            p = &p[i];
            nPitches -= i;
            break;
        }
    }

    LOG_LOG("Trimmed length: %i | original length: %i", length, nPitches);
}

void Util::cleanUpData(float* p, int& length, float lpf_alpha) {
    interpolateGaps(p, length);
    zeroPhaseLPF(p, length, lpf_alpha);
    // trimEndZeros(p, length);
}

bool Util::checkFollowError(int32_t* p, int nPitches) {
    for (int i = 1; i < nPitches; ++i) {
        if (abs(p[i] - p[i - 1]) > MAX_FOLLOW_ERROR) {
            LOG_WARN("%i, %i, %i", i, p[i], p[i - 1]);
            return false;
        }
    }
    return true;
}

float Util::__fretLength(float fretNumber) {
    return (SCALE_LENGTH - (SCALE_LENGTH / pow(2, (fretNumber / 12.f))));
}

int32_t Util::__pos2Pulse(float pos, int direction) {
    return (int32_t) (direction * pos * P2P_MULT);
}

int32_t Util::fret2Pos(float fretPos) {
    return __pos2Pulse(__fretLength(fretPos), ENCODER_DIR) + (NUT_POS * ENCODER_DIR);
}

void Util::convertPitchToInc(int32_t* inc, const float* p, int nPitches, int offset) {
    // int stayLen = (offset + 1) / 2;
    // int curveLen = (offset + 1) - stayLen;

    if (offset > 0) {
        float* temp = new float[offset];
        thirdOrderPolyFit(offset, p[0], temp);

        for (int i = 0; i < offset; ++i)
            inc[i] = fret2Pos(temp[i]);
        delete[] temp;
    }

    // int pos = fret2Pos(p[0]);
    // for (int i = curveLen; i < offset; ++i)
    //     inc[i] = pos;

    for (int i = offset; i < nPitches + offset; ++i)
        inc[i] = fret2Pos(p[i]);

}

void Util::thirdOrderPolyFit(int t1, float y, float* arr) {
    int y3 = 3 * y;
    int y2 = 2 * y;
    Serial.println(y);
    for (int i = 0; i < t1; ++i) {
        float t = 1.f * i / t1;
        arr[i] = (y3 * (powf(t, 2))) - (y2 * (powf(t, 3)));
        // Serial.println(arr[i]);
    }
}
