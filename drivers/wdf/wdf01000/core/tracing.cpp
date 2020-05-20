/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tracing for the driver frameworks
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/fxtrace.h"
#include "common/fxifr.h"
#include "common/fxautoregistry.h"
#include "common/dbgtrace.h"
#include <ntstrsafe.h>

//-----------------------------------------------------------------------------
// Subcomponents for the In-Flight Recorder follow.
//-----------------------------------------------------------------------------

ULONG
FxIFRGetSize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    Checks to see if the service has overriden the default number of pages that
    are in the IFR.

Arguments:
    RegistryPath - path to the service

Return Value:
    The size of the IFR to create in bytes (not pages!)

  --*/
{
    FxAutoRegKey service, parameters;
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    ULONG numPages;

    //
    // This is the value used in case of any error while retrieving 'LogPages'
    // from the registry.
    //
    numPages  = FxIFRMinLogPages;

    //
    // External representation of the IFR is the "LogPages", so use that term when
    // overriding the size via the registry.
    //
    DECLARE_CONST_UNICODE_STRING(parametersPath, L"Parameters\\Wdf");
    DECLARE_CONST_UNICODE_STRING(valueName, L"LogPages");

    InitializeObjectAttributes(&oa,
                               (PUNICODE_STRING)RegistryPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&service.m_Key, KEY_READ, &oa);
    if (!NT_SUCCESS(status))
    {
        goto defaultValues;
    }

    InitializeObjectAttributes(&oa,
                               (PUNICODE_STRING)&parametersPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               service.m_Key,
                               NULL);

    status = ZwOpenKey(&parameters.m_Key, KEY_READ, &oa);

    if (!NT_SUCCESS(status))
    {
        goto defaultValues;
    }

    status = FxRegKey::_QueryULong(parameters.m_Key, &valueName, &numPages);
    if (!NT_SUCCESS(status))
    {
        goto defaultValues;
    }

    if (numPages == 0)
    {
        numPages = FxIFRMinLogPages;
    }

defaultValues:
    //
    // Use FxIFRAvgLogPages if user specifies greater than FxIFRMaxLogPages and if
    // Verifier flag is on and so is Verbose flag.
    //
    if (numPages > FxIFRMaxLogPages)
    {
        if (FxDriverGlobals->FxVerifierOn && FxDriverGlobals->FxVerboseOn)
        {
            numPages = FxIFRAvgLogPages;
        }
        else
        {
            numPages = FxIFRMinLogPages;
        }
    }

    return numPages * PAGE_SIZE;
}

VOID
FxIFRStart(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegistryPath,
    __in MdDriverObject DriverObject
    )
/*++

Routine Description:

    This routine initialize the In-Flight Recorder (IFR).

    The default log size is set by WDF_IFR_LOG_SIZE and currently
    is 4096 (one x86 page).
    This routine should be called very early in driver initialization
    to allow the capture of all significant events.

--*/
{
    PWDF_IFR_HEADER pHeader;
    ULONG size;

    UNREFERENCED_PARAMETER( DriverObject );

    WDFCASSERT(FxIFRRecordSignature == WDF_IFR_RECORD_SIGNATURE);

    //
    // Return early if IFR is disabled.
    //
    //if (FxLibraryGlobals.IfrDisabled)
    //{
    //    ASSERT(FxDriverGlobals->WdfLogHeader == NULL);
    //    return;
    //}

    if (FxDriverGlobals == NULL || FxDriverGlobals->WdfLogHeader != NULL)
    {
        return;
    }

    size = FxIFRGetSize(FxDriverGlobals, RegistryPath);

    pHeader = (PWDF_IFR_HEADER) ExAllocatePoolWithTag(NonPagedPool,
                                                      size,
                                                      WDF_IFR_LOG_TAG );
    if (pHeader == NULL)
    {
        return;
    }

    RtlZeroMemory(pHeader, size);

    //
    // Initialize the header.
    // Base will be where the IFR records are placed.
    // WPP_ThisDir_CTLGUID_FrameworksTraceGuid
    //
    RtlCopyMemory(&pHeader->Guid, (PVOID) &WdfTraceGuid, sizeof(GUID));

    pHeader->Base = (PUCHAR) &pHeader[1];
    pHeader->Size = size - sizeof(WDF_IFR_HEADER);

    pHeader->Offset.u.s.Current  = 0;
    pHeader->Offset.u.s.Previous = 0;
    RtlStringCchCopyA(pHeader->DriverName, WDF_IFR_HEADER_NAME_LEN, FxDriverGlobals->Public.DriverName);

    FxDriverGlobals->WdfLogHeader = pHeader;

    //InterlockedIncrement(&FxDriverGlobals->WdfLogHeaderRefCount);

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
                        "FxIFR logging started" );

    if (size > FxIFRMinLogSize)
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
            "FxIFR has been started with a size override:  size 0x%x bytes, "
            "# Pages %d.  An extended IFR size may not be written to a minidump!",
            size, size/PAGE_SIZE);
    }
}

VOID
FxIFRStop(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
/*++

Routine Description:

    This routine stops the In-Flight Recorder (IFR).

    It should be called as late in the driver teardown as possible
    to allow for the capture of all significant events.

--*/
{
    if (FxDriverGlobals == NULL || FxDriverGlobals->WdfLogHeader == NULL)
    {
        return;
    }

    //
    // Under normal operation the ref count should usually drop to zero when 
    // FxIfrStop is called by FxLibraryCommonUnregisterClient, unless 
    // FxIfrReplay is in the process of making a copy of the IFR buffer. 
    // In which case that thread will call FxIfrStop.
    //
    //
    // Free the Log buffer.
    //
    ExFreePoolWithTag(FxDriverGlobals->WdfLogHeader, WDF_IFR_LOG_TAG);
    FxDriverGlobals->WdfLogHeader = NULL;
}
