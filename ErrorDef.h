#ifndef ERROR_DEF_H
#define ERROR_DEF_H

enum Error_t
{
    kNoError,

    kFileOpenError,
    kFileAccessError,

    kFunctionInvalidArgsError,

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



