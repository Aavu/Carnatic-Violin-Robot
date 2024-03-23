#ifndef ERROR_DEF_H
#define ERROR_DEF_H

enum Error_t
{
    kNoError,

    kFileOpenError,
    kFileAccessError,
    kFileParseError,
    kFileWriteError,
    kFileCloseError,

    kFunctionInvalidArgsError,
    kFunctionExecOrderError,

    kNotInitializedError,
    kFunctionIllegalCallError,
    kInvalidString,

    kGetValueError,
    kSetValueError,

    kOutOfBoundsError,
    kNaNError,

    kMemError,

    kUnknownError,

    kNumErrors
};
#endif // ERROR_DEF_H



