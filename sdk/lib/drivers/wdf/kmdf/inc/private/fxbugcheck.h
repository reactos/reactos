/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxBugcheck.h

Abstract:
    This module contains private macros/defines for crashdumps.

--*/

#ifndef __FXBUGCHECK_H__
#define __FXBUGCHECK_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Macro for doing pointer arithmetic.
//
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

//
// This macro takes a length & rounds it up to a multiple of the alignment
// Alignment is given as a power of 2
//
#define EXP_ALIGN_UP_PTR_ON_BOUNDARY(_length, _alignment) \
          (PVOID) ((((ULONG_PTR) (_length)) + ((_alignment)-1)) & \
                              ~(ULONG_PTR)((_alignment) - 1))

//
// Checks if 1st argument is aligned on given power of 2 boundary specified
// by 2nd argument
//
#define EXP_IS_PTR_ALIGNED_ON_BOUNDARY(_pointer, _alignment) \
        ((((ULONG_PTR) (_pointer)) & ((_alignment) - 1)) == 0)


//
// This macro takes a size and rounds it down to a multiple of the alignemnt.
// Alignment doesn't need to be a power of 2.
//
#define EXP_ALIGN_DOWN_ON_BOUNDARY(_size, _alignment) \
        ((_size) - ((_size) % _alignment))

//
// Define the max data size the bugcheck callback can write. Per callback the
// total size is around 16K on 32bit OS (32K on 64bit OS).
//
#ifdef _WIN64
#define FX_MAX_DUMP_SIZE                    (32*1024)
#else
#define FX_MAX_DUMP_SIZE                    (16*1024)
#endif

//
// Maximum number of CPUs supported by the driver tracker.
//
#define FX_DRIVER_TRACKER_MAX_CPUS          256     // Max Win7 processors.

//
// The initial/increment size of the array to hold driver info.
//
#define FX_DUMP_DRIVER_INFO_INCREMENT       10      // # entries.

//
// The max size of the array to hold the driver info. This is the max data
// we can write into the minidump.
//
#define FX_MAX_DUMP_DRIVER_INFO_COUNT \
    (FX_MAX_DUMP_SIZE/sizeof(FX_DUMP_DRIVER_INFO_ENTRY))

//
// During run-time we store info about the loaded drivers in an array
// of FX_DUMP_DRIVER_INFO_ENTRY struct entries, on a crash we write the
// entire array in the minidump for postmortem analysis.
//
typedef struct _FX_DUMP_DRIVER_INFO_ENTRY {
    PFX_DRIVER_GLOBALS  FxDriverGlobals;
    WDF_VERSION         Version;
    CHAR                DriverName[WDF_DRIVER_GLOBALS_NAME_LEN];
} FX_DUMP_DRIVER_INFO_ENTRY, *PFX_DUMP_DRIVER_INFO_ENTRY;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __FXBUGCHECK_H__
