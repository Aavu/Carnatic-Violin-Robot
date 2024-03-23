#if !defined(__Util_hdr__)
#define __Util_hdr__

#include <cassert>
#include <cstring>
#include <limits>
#include <string>
#include <iostream>

#include "ErrorDef.h"

/*! \brief class with static utility functions 
*/
class CUtil
{
public:
    inline static const std::string kName = "CUtil";
    /*! converts a float to an int
    \param fInput float value
    \return T
    */
    template<typename T>
    static T float2int (float fInput)
    {
        if (fInput >= 0.F)
            return static_cast<T>(fInput + .5F);
        else
            return static_cast<T>(fInput - .5F);
    }
    /*! converts a double to an int
    \param fInput double value
    \return T
    */
    template<typename T>
    static T double2int (double fInput)
    {
        if (fInput >= 0)
            return static_cast<T>(fInput + .5);
        else
            return static_cast<T>(fInput - .5);
    }

    /*! checks if the input is a power of 2
    \param n integer value
    \return bool
    */
    static bool isPowOf2 (int n) 
    {
        return !(n & (n-1));
    }

    /*! converts an arbitrary integer (positive) to the next larger power of two
    \param n integer value
    \return int
    */
    static int nextPowOf2(int n)
    {
        int iOrder = 0;

        if (n == 0)
            return 0;

        while (n>>iOrder)
            iOrder++;

        if (!(n%(1<<(iOrder-1))))
            iOrder--;

        return (1<<(iOrder));
    }

    template<typename T>
    static void swap (T &tValue1, T &tValue2)
    {
        T tTmp = tValue1;

        tValue1 = tValue2;
        tValue2 = tTmp;
    }

    static void PrintMessage(const std::string& objName, const std::string& msg) {
        std::cout << msg << " (" << objName << ")\n";
    }

    static void PrintError(const std::string& objName, Error_t err) {
        auto cErr = GetErrorString(err);
        std::cerr << objName << " failed (" << cErr << ")\n";
    }

    static std::string GetErrorString(Error_t err) {
        switch (err) {
            case kNoError:
                return "kNoError";
            case kFileOpenError:
                return "kFileOpenError";
            case kFileAccessError:
                return "kFileAccessError";
            case kFileWriteError:
                return "kFileWriteError";
            case kFunctionInvalidArgsError:
                return "kFunctionInvalidArgsError";
            case kFunctionExecOrderError:
                return "kFunctionExecOrderError";
            case kNotInitializedError:
                return "kNotInitializedError";
            case kFunctionIllegalCallError:
                return "kFunctionIllegalCallError";
            case kInvalidString:
                return "kInvalidString";
            case kGetValueError:
                return "kGetValueError";
            case kSetValueError:
                return "kSetValueError";
            case kOutOfBoundsError:
                return "kOutOfBoundsError";
            case kMemError:
                return "kMemError";
            default:
                return "kUnknownError";
        }
    }

};
#endif // __Util_hdr__