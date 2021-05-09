//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "fxobjectpch.hpp"

// Tracing support
extern "C" {
#if defined(EVENT_TRACING)
#include "globalskm.tmh"
#endif

// #include <wdfcxbase.h>
#include <fxldr.h>


}
extern "C" {

VOID
FxFreeAllocatedMdlsDebugInfo(
    __in FxDriverGlobalsDebugExtension* DebugExtension
    )
{
    FxAllocatedMdls* pNext, *pCur;

    pNext = DebugExtension->AllocatedMdls.Next;

    //
    // MDL leaks were already checked for in FxPoolDestroy, just free all
    // the tables here.
    //
    while (pNext != NULL) {
        pCur = pNext;
        pNext = pCur->Next;

        ExFreePool(pCur);
    }
}

KDEFERRED_ROUTINE FxFlushDpc;

__drv_functionClass(KDEFERRED_ROUTINE)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_requiresIRQL(DISPATCH_LEVEL)
__drv_sameIRQL
VOID
STDCALL
FxFlushDpc (
    __in struct _KDPC *Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )

/*++

Routine Description:

    This DPC is called on a processor to assist in flushing previous DPC's

Arguments:

    Dpc - Supplies a pointer to DPC object.

    DeferredContext - Supplies the deferred context (event object address).

    SystemArgument1 - Supplies the first system context parameter (not used).

    SystemArgument2 - Supplies the second system context parameter (not used).

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // Signal that this routine has been called.
    //
    ((FxCREvent*)DeferredContext)->Set();
}

_Must_inspect_result_
BOOLEAN
FX_DRIVER_GLOBALS::IsVersionGreaterThanOrEqualTo(
    __in ULONG  Major,
    __in ULONG  Minor
    )
{
    if ((WdfBindInfo->Version.Major > Major) ||
                (WdfBindInfo->Version.Major == Major &&
                  WdfBindInfo->Version.Minor >= Minor)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

#define WDF_MAJOR_VERSION_VALUE L"WdfMajorVersion"
#define WDF_MINOR_VERSION_VALUE L"WdfMinorVersion"

_Must_inspect_result_
BOOLEAN
FX_DRIVER_GLOBALS::IsCorrectVersionRegistered(
    _In_ PCUNICODE_STRING ServiceKeyName
    )
{
    FxAutoRegKey hDriver, hWdf;
    DECLARE_CONST_UNICODE_STRING(parametersPath, L"Parameters\\Wdf");
    DECLARE_CONST_UNICODE_STRING(wdfMajorValue, WDF_MAJOR_VERSION_VALUE);
    DECLARE_CONST_UNICODE_STRING(wdfMinorValue, WDF_MINOR_VERSION_VALUE);
    ULONG registeredMajor = 0, registeredMinor = 0;
    NTSTATUS status;

    status = FxRegKey::_OpenKey(NULL,
                                ServiceKeyName,
                                &hDriver.m_Key,
                                KEY_READ
                                );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    status = FxRegKey::_OpenKey(hDriver.m_Key,
                                &parametersPath,
                                &hWdf.m_Key,
                                KEY_READ
                                );
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    status = FxRegKey::_QueryULong(hWdf.m_Key,
                                   &wdfMajorValue,
                                   &registeredMajor);

    if (!NT_SUCCESS(status) || registeredMajor != WdfBindInfo->Version.Major) {
        return FALSE;
    }

    status = FxRegKey::_QueryULong(hWdf.m_Key,
                                   &wdfMinorValue,
                                   &registeredMinor);

    if  (!NT_SUCCESS(status) || registeredMinor != WdfBindInfo->Version.Minor){
        return FALSE;
    }
    else {
        return TRUE;
    }
}

VOID
FX_DRIVER_GLOBALS::RegisterClientVersion(
    _In_ PCUNICODE_STRING ServiceKeyName
    )
{
    FxAutoRegKey hDriver, hParameters, hWdf;
    DECLARE_CONST_UNICODE_STRING(parametersPart, L"Parameters");
    DECLARE_CONST_UNICODE_STRING(wdfPart, L"Wdf");
    //
    //  Not defined with the macro because ZwSetValue doesn't use PCUNICODE_STRING
    //
    UNICODE_STRING wdfMajorValue;
    UNICODE_STRING wdfMinorValue;
    NTSTATUS status;

    RtlInitUnicodeString(&wdfMajorValue, WDF_MAJOR_VERSION_VALUE);
    RtlInitUnicodeString(&wdfMinorValue, WDF_MINOR_VERSION_VALUE);

    status = FxRegKey::_OpenKey(NULL,
                                ServiceKeyName,
                                &hDriver.m_Key,
                                KEY_WRITE | KEY_READ
                                );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(this, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
            "Unable to open driver's service key, status %!STATUS!", status);
        return;
    }
    //
    //  Key creation, unlike user mode, must happen one level at a time, since
    //  create will also open take both steps instead of trying open first
    //
    status = FxRegKey::_Create(hDriver.m_Key,
                               &parametersPart,
                               &hParameters.m_Key,
                               KEY_WRITE | KEY_READ
                               );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(this, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
            "Unable to write Parameters key, status %!STATUS!", status);
        return;
    }

    status = FxRegKey::_Create(hParameters.m_Key,
                               &wdfPart,
                               &hWdf.m_Key,
                               KEY_WRITE | KEY_READ
                               );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(this, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
            "Unable to write Parameters key, status %!STATUS!", status);
        return;
    }

    //
    //  Using ZwSetValueKey here to avoid having to change the implementation
    //  in FxRegKey of SetValue to a static / thiscall pair
    //
    status = ZwSetValueKey(hWdf.m_Key,
                           &wdfMajorValue,
                           0,
                           REG_DWORD,
                           &WdfBindInfo->Version.Major,
                           sizeof(WdfBindInfo->Version.Major)
                           );

    if  (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(this, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
            "Failed to record driver major version value, status %!STATUS!", status);
    }

    status = ZwSetValueKey(hWdf.m_Key,
                           &wdfMinorValue,
                           0,
                           REG_DWORD,
                           &WdfBindInfo->Version.Minor,
                           sizeof(WdfBindInfo->Version.Minor)
                           );

    if  (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(this, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
            "Failed to record driver version value, status %!STATUS!", status);
    }
}

} // extern "C"

_Must_inspect_result_
BOOLEAN
FX_DRIVER_GLOBALS::IsDebuggerAttached(
    VOID
    )
{
    return (FALSE == KdRefreshDebuggerNotPresent());
}
