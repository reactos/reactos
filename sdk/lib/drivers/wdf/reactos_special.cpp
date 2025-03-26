#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ntverp.h>

extern "C" {
#include <ntddk.h>
#include <ntstrsafe.h>
}

#define  FX_DYNAMICS_GENERATE_TABLE   1

#include "fx.hpp"

// #include <fxldr.h>
// #include "fxbugcheck.h"


//-----------------------------------------    ------------------------------------

extern "C" {

#include "fxdynamics.h"

// #include "FxLibraryCommon.h"

typedef VOID (*WDFFUNC) (VOID);

const WDFFUNC *WdfFunctions_01017 = (WDFFUNC *)(&WdfVersion.Functions);
// DECLSPEC_ALIGN(MAX_NATURAL_ALIGNMENT) UINT8 WdfDriverGlobalsVal[248] = { 0 }; // sizeof(FX_DRIVER_GLOBALS)
// PWDF_DRIVER_GLOBALS WdfDriverGlobals = &((PFX_DRIVER_GLOBALS)&WdfDriverGlobalsVal)->Public;
WDF_BIND_INFO WdfBindInfo = {0};
PWDF_DRIVER_GLOBALS WdfDriverGlobals;
extern FxLibraryGlobalsType FxLibraryGlobals;

#define  KMDF_DEFAULT_NAME   "Wdf01000"

PCHAR WdfLdrType = KMDF_DEFAULT_NAME;

WDFAPI
NTSTATUS
STDCALL
WdfApiNotImplemented()
{
	DbgPrint("ReactOS KMDF: %s non-implemented API called\n");
    __debugbreak();
	return STATUS_UNSUCCESSFUL;
}

// called in WdfDriverCreate in fxdriverapi.cpp
VOID
RosInitWdf()
{
	WdfDriverGlobals = FxAllocateDriverGlobals();
	PFX_DRIVER_GLOBALS fxDriverGlobals = GetFxDriverGlobals(WdfDriverGlobals);

	WdfBindInfo.Size = sizeof(WDF_BIND_INFO);
	WdfBindInfo.Version.Major = 1;
	WdfBindInfo.Version.Minor = 9;
	WdfBindInfo.Version.Build = 7600;
	WdfBindInfo.FuncCount = WdfVersion.FuncCount;
	WdfBindInfo.FuncTable = (WDFFUNC *)(&WdfVersion.Functions);
	fxDriverGlobals->WdfBindInfo = &WdfBindInfo;
	FxLibraryGlobals.OsVersionInfo.dwMajorVersion = 5;
	FxLibraryGlobals.OsVersionInfo.dwMinorVersion = 1;
}

void
__cxa_pure_virtual()
{
	__debugbreak();
}

}  // extern "C"
