#ifndef _FXTRACE_H_
#define _FXTRACE_H_

#include "common/fxglobals.h"

VOID
FxIFRStart(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegistryPath,
    __in MdDriverObject DriverObject
    );

VOID
FxIFRStop(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

#endif //_FXTRACE_H_
