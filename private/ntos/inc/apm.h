/*++

Module Name:

    apm.h

Abstract:

    This is the APM bus specific header file

Author:

Revision History:

--*/




//
// APM 1.0 defines
//

// devices
#define APM_DEVICE_BIOS                     0x0000
#define APM_DEVICE_ALL                      0x0001

#define APM_DEVICE_DISPLAY                  1
#define APM_DEVICE_STORAGE                  2
#define APM_DEVICE_PARALLEL                 3
#define APM_DEVICE_SERIAL                   4
#define APM_MAX_DEVICE_TYPE                 4

#define APM_DEVICE_ID(Type,Inst)    ((Type << 8) | Inst)


#define APM_SET_READY                       0
#define APM_SET_STANDBY                     1
#define APM_SET_SUSPEND                     2
#define APM_SET_OFF                         3

// interface api numbers
#ifndef NEC_98
#define APM_INSTALLATION_CHECK              0x5300
#define APM_REAL_MODE_CONNECT               0x5301
#define APM_PROTECT_MODE_16bit_CONNECT      0x5302
#define APM_PROTECT_MODE_32bit_CONNECT      0x5303
#define APM_DISCONNECT                      0x5304
#define APM_CPU_IDLE                        0x5305
#define APM_CPU_BUSY                        0x5306
#define APM_SET_POWER_STATE                 0x5307
#define APM_ENABLE_FUNCTION                 0x5308
#define APM_RESTORE_DEFAULTS                0x5309
#define APM_GET_POWER_STATUS                0x530A
#define APM_GET_EVENT                       0x530B
#define APM_DRIVER_VERSION                  0x530E
#else
#define APM_INSTALLATION_CHECK              0x9A00
#define APM_REAL_MODE_CONNECT               0x9A01
#define APM_PROTECT_MODE_16bit_CONNECT      0x9A02
#define APM_PROTECT_MODE_32bit_CONNECT      0x9A03
#define APM_DISCONNECT                      0x9A04
#define APM_CPU_IDLE                        0x9A05
#define APM_CPU_BUSY                        0x9A06
#define APM_SET_POWER_STATE                 0x9A07
#define APM_ENABLE_FUNCTION                 0x9A08
#define APM_RESTORE_DEFAULTS                0x9A09
#define APM_GET_POWER_STATUS                0x9A0A
#define APM_GET_EVENT                       0x9A0B
#define APM_DRIVER_VERSION                  0x9A3E
#endif

#define APM_MODE_16BIT                      0x0001
#define APM_MODE_32BIT                      0x0002
#define APM_CPU_IDLE_SLOW                   0x0004
#define APM_DISABLED                        0x0008
#define APM_DISENGAGED                      0x0010


