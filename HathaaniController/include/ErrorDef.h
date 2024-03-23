//
// Created by Raghavasimhan Sankaranarayanan on 03/30/22.
//

#ifndef ERRORDEF_H
#define ERRORDEF_H

enum Error_t {
    kNoError = 0,

    kNotInitializedError,
    kReInitializationError,
    kNotHomedError,

    kNotImplementedError,
    kInvalidArgsError,
    kArgLimitError,

    kCorruptedDataError,
    kSegvError,
    kInsufficientMemError,

    kFileParseError,

    kFileOpenError,
    kFileCloseError,

    kNotOpenedError,
    kNotClosedError,

    kSetValueError,
    kGetValueError,

    kWriteError,
    kReadError,
    kFlushError,

    kImpossibleError,
    kAlreadyThereError,
    kInterferenceError,

    kTrajectoryError,

    kTimeoutError,

    kNamingError,

    kBufferReadError,
    kBufferWriteError,

    // Low level
    kSetControlWordError,
    kShutdownError,
    kReadStatusError,
    kConfigError,
    kSetFollowErrorWindowError,
    kSetNMTStateError,
    kSetNominalCurrentError,
    kSetOutputCurrentLimitError,
    kSetMotorTorqueConstantError,
    kSetThermalTimeConstantWindingError,
    kSetNumPolePairsError,
    kSetEncoderNumPulsesError,
    kSetEncoderTypeError,
    kSetITPError,

    // Unknown
    kUnknownCaseError,
    kUnknownError
};

#endif // ERRORDEF_H
