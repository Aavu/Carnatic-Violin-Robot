#ifndef ERROR_DEF_H
#define ERROR_DEF_H

enum Error_t
{
    kNoError,

    kFileOpenError,
    kFileAccessError,
    kFileWriteError,

    kFunctionInvalidArgsError,
    kFunctionExecOrderError,

    kNotInitializedError,
    kFunctionIllegalCallError,
    kInvalidString,

    kGetValueError,
    kSetValueError,

    kMemError,

    kUnknownError,

    kNumErrors
};
#endif // ERROR_DEF_H



