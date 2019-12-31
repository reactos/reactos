#ifndef _FXPOOL_H_
#define _FXPOOL_H_

#include <ntddk.h>
#include "wdf.h"

extern "C"
_Must_inspect_result_
PWDF_DRIVER_GLOBALS
FxAllocateDriverGlobals(
    VOID
    );

#endif //_FXPOOL_H_