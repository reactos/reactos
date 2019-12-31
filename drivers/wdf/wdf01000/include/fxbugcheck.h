#ifndef __FXBUGCHECK_H__
#define __FXBUGCHECK_H__

#include <ntddk.h>
#include "common/fxldr.h"
#include "wdf.h"

#ifdef __cplusplus
extern "C" {
#endif

// forward definitions
typedef struct _FX_DRIVER_GLOBALS *PFX_DRIVER_GLOBALS;

//
// The initial/increment size of the array to hold driver info. 
//
#define FX_DUMP_DRIVER_INFO_INCREMENT       10      // # entries.

//
// This macro takes a size and rounds it down to a multiple of the alignemnt.
// Alignment doesn't need to be a power of 2.
//
#define EXP_ALIGN_DOWN_ON_BOUNDARY(_size, _alignment) \
        ((_size) - ((_size) % _alignment))

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