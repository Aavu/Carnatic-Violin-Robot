#pragma once

#include <stdint.h>

#define E_OK      0x4f  ///< EPOS answer code for 'all fine'
#define E_FAIL    0x46  ///< EPOS answer code to indicate a 'failure'
#define E_ANS     0x00  ///< EPOS code to indicate an answer 'frame'

/* EPOS Device Error */
// EPOS4 Firmware spec (2021-04) -> 7.2
#define EP_NOERR        0x0000
#define EP_GENERR       0x1000
#define EP_OCERR        0x2310
#define EP_OVERR        0x3210
#define EP_UVERR        0x3220
#define EP_OTERR        0x4210
#define EP_SUPVOLLOW    0x5113
#define EP_OUTVOLLOW    0x5114
#define EP_INTSOFT      0x6100
#define EP_SOFTPAR      0x6320
#define EP_POSSENS      0x7320
#define EP_OBJLOST      0x8110
#define EP_CANOVRUN     0x8111
#define EP_CANPASS      0x8120
#define EP_HEARTBEAT    0x8130
#define EP_CANCOLLSN    0x8150
#define EP_CANOFF       0x81FD
#define EP_CANRxOVRFLW  0x81FE
#define EP_CANTxOVRFLW  0x81FF
#define EP_CANPDOLEN    0x8210
#define EP_FOLLOW       0x8611

/* CANopen defined error codes */
// EPOS4 Firmware spec (2021-04) -> 7.3
#define E_NOERR             0x00000000      ///< Error code: no error
#define E_TOGGLE            0x05030000      ///< Error code: Toggle bit not alternated
#define E_SDOTOUT           0x05040000      ///< Error code: SDO protocol timed out
#define E_CMDUKNOWN         0x05040001      ///< Error code: Client/server command specifier not valid or unknown
#define E_CRCERR            0x05040004      ///< Error code: CRC error
#define E_NOACCES           0x06010000      ///< Error code: Unsupported access to an object
#define E_WRITEONLY         0x06010001      ///< Error code: Attempt to read a write-only object
#define E_READONLY          0x06010002      ///< Error code: Attempt to write a read-only object
#define E_SUBIDX_W          0x06010003      ///< Error code: Subindex cannot be written, subindex 0 must be “0” (zero) for write access
#define E_SDO_ACCESS        0x06010004      ///< Error code: The object cannot be accessed via complete access
#define E_ONOTEX            0x06020000      ///< Error code: object does not exist
#define E_PDOMAP            0x06040041      ///< Error code: The object cannot be mapped to the PDO
#define E_PDOLEN            0x06040042      ///< Error code: The number and length of the objects to be would exceed PDO length
#define E_PARAMINCOMP       0x06040043      ///< Error code: general parameter incompatibility
#define E_INTINCOMP         0x06040047      ///< Error code: general internal incompatibility in the device
#define E_HWERR             0x06060000      ///< Error code: access failed due to an hardware error
#define E_SVCPAR            0x06070010      ///< Error code: Data type does not match, length or service does not match
#define E_SVCPARLO          0x06070013      ///< Error code: Data type does not match, length or service too low
#define E_SUBINEX           0x06090011      ///< Error code: Last read or write command had wrong object SubIndex
#define E_PRAGNEX           0x06090030      ///< Error code: value range of parameter exeeded
#define E_GENERR            0x08000000      ///< Error code: General error
#define E_TFERSTORE         0x08000020      ///< Error code: Data cannot be transferred or stored
#define E_DEVSTAT           0x08000022      ///< Error code: Data cannot be transferred or stored to because of the present device state
#define E_PWD               0x0F00FFBE      ///< Error code: Password is incorrect
#define E_ILLEGAL_CMD       0x0F00FFBF      ///< Error code: Command code is illegal (does not exist)
#define E_WRONG_NMT_STATE   0x0F00FFC0      ///< Error code: Device is in wrong NMT state
/*
#define E_INVBLKSIZE    0x05040002   ///< Error code: Invalid block size (block mode only)
#define E_INVSEQ        0x05040003   ///< Error code: Invalid sequence number (block mode only)
#define E_OUTMEM        0x05040005   ///< Error code: out of memory

#define E_SVCPARHI      0x06070012   ///< Error code: Data type does not match, length or service too high
#define E_PARHIGH       0x06090031   ///< Error code: value of parameter written is too high
#define E_PARLOW        0x06090032   ///< Error code: value of parameter written is too low
#define E_PARREL        0x06090036   ///< Error code: maximum value is less than minimum value
#define E_LOCALCTL      0x08000021   ///< Error code: Data cannot be transferred or stored to because of local control
*/

#define CTRL_WORD_ADDR 0x6040

#define COB_ID_SDO      0x00000600  // COB_ID SDO Client to Server
#define COB_ID_SDO_SC   0x00000580  // COB_ID SDO Server to Client  

#define COB_ID_TPDO3    0x00000380
#define COB_ID_RPDO2    0x00000300
#define COB_ID_RPDO3    0x00000400
#define COB_ID_RPDO4    0x00000500

#define COB_ID_EMCY     0x00000080
/* EPOS modes of operation */
// EPOS4 Firmware spec (2021-04) -> 6.2.100
enum OpMode {
    ProfilePosition = 1,    ///< EPOS4 Operation Mode: profile position mode
    ProfileVelocity = 3,    ///< EPOS4 Operation Mode: profile velocity mode
    Homing = 6,             ///< EPOS4 Operation Mode: homing
    CyclicSyncPosition = 8, ///< EPOS4 Operation Mode: Cyclic Synchronous Position Mode (CSP)
    CyclicSyncVelocity = 9, ///< EPOS4 Operation Mode: Cyclic Synchronous Velocity Mode (CSV)
    CyclicSyncTorque = 10   ///< EPOS4 Operation Mode: Cyclic Synchronous Torque Mode (CST)
};

typedef uint32_t _DWORD;
typedef uint16_t _WORD;
typedef uint8_t _BYTE;

/* EPOS will reset communication after 500ms of inactivity */

/*! \brief try NTRY times to read one byte from EPOS, the give up */
#define NTRY      5
/*! \brief sleep TRYSLEEP usec between read() from EPOS, if no data available */
//#define TRYSLEEP  (unsigned int)1e5 
#define TRYSLEEP  (unsigned int)1e4


#define MOTOR_DATA_ADDR 0x3001
enum MotorData_SIDef {
    NOMINAL_CURRENT = 0x01,
    OUTPUT_CURRENT_LIMIT = 0x02,
    POLE_PAIRS = 0x03,
    THERMAL_TIME_CONST_WINDING = 0x04,
    TORQUE_CONST = 0x05
};

#define MODE_OF_OPERATION_ADDR 0x6060
#define MODE_OF_OPERATION_DISP_ADDR 0x6061

#define CURRENT_CTRL_PARAM_ADDR 0x30A0
enum CurrentCtrl_SIDef {
    CC_P_GAIN = 0x01,
    CC_I_GAIN = 0x02
};

#define POS_CTRL_PARAM_ADDR 0x30A1
enum PosCtrl_SIDef {
    PC_P_GAIN = 0x01,
    PC_I_GAIN = 0x02,
    PC_D_GAIN = 0x03,
    PC_FF_V_GAIN = 0x04,
    PC_FF_A_GAIN = 0x05
};

#define INTERP_TIME_ADDR 0x60C2

enum NMTState {
    PreOperational = 0x80,
    Operational = 0x01,
    Stopped = 0x02,
    ResetApp = 0x81,
    ResetComm = 0x82
};

enum HomingMethod {
    CurrentThresholdPositive = -3,
    CurrentThresholdNegative = -4
};

enum HomingStatus {
    InProgress,
    Interrupted,
    Completed,
    Error = -1
};

enum MotorModel {
    EC45,
    EC32
};

#define HOMING_TIMEOUT 10000    // ms
