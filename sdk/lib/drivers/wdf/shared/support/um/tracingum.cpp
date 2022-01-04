/*++

Copyright (c) Microsoft Corporation

Module Name:

    Tracing.cpp

Abstract:

    This module implements tracing for the driver frameworks

Author:




Revision History:




--*/

#include "FxSupportPch.hpp"
#include "fxldrum.h"

extern "C" {
#if defined(EVENT_TRACING)
#include "TracingUM.tmh"
#endif
}

#include <strsafe.h>
#include <initguid.h>
#include "fxIFR.h"       // shared struct between IFR and debug ext.
#include "fxIFRKm.h"     // kernel mode only IFR definitions
























































































































































































































_Must_inspect_result_
NTSTATUS
FxTraceInitialize(
    VOID
    )

/*++

Routine Description:

    This routine initializes the frameworks tracing.

    It must be called early on in the frameworks DriverEntry
    initialization.

Arguments:

    None

Returns:

    NTSTATUS code

--*/
{
    //
    // Initialize the tracing package.
    //
    WPP_INIT_TRACING(NULL, NULL);

    return STATUS_SUCCESS;
}

VOID
TraceUninitialize(
    VOID
    )
/*++

Routine Description:
    This routine uninitializes the frameworks tracing.  It must be called just
    before DriverUnload

Arguments:
    None

Returns:
    None

--*/
{
    WPP_CLEANUP(NULL);
}

_Must_inspect_result_
NTSTATUS
FxWmiQueryTraceInformation(
    __in TRACE_INFORMATION_CLASS /* TraceInformationClass */,
    __out_bcount(TraceInformationLength) PVOID /* TraceInformation */,
    __in ULONG /* TraceInformationLength */,
    __out_opt PULONG /* RequiredLength */,
    __in_opt PVOID /* Buffer */
    )
{
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
FxWmiTraceMessage(
    __in TRACEHANDLE  LoggerHandle,
    __in ULONG        MessageFlags,
    __in LPGUID       MessageGuid,
    __in USHORT       MessageNumber,
         ...
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    va_list va;
    va_start(va, MessageNumber);
    //
    // UMDF is supported only on XP and newer OS so no need to support w2k
    // tracing (which requires using a different tracing api, see kmdf impl)
    //
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "Recommneded by EndClean");
    status = TraceMessageVa(LoggerHandle,
                            MessageFlags,
                            MessageGuid,
                            MessageNumber,
                            va);

    va_end(va);

    return status;
}


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
    ULONG numPages;

    //
    // This is the value used in case of any error while retrieving 'LogPages'
    // from the registry.
    //
    numPages  = FxIFRMinLogPages;

    //
    // External representation of the IFR is the "log", so use tha term when
    // overriding the size via the registry.
    //
    DECLARE_CONST_UNICODE_STRING(parametersPath, L"Parameters\\Wdf");
    DECLARE_CONST_UNICODE_STRING(valueName, L"LogPages");

    //
    // UMDF may not provide this registry path
    //
    if (NULL == RegistryPath) {
        goto defaultValues;
    }

    status = FxRegKey::_OpenKey(NULL,
                                (PCUNICODE_STRING)RegistryPath,
                                &service.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status)) {
        goto defaultValues;
    }

    status = FxRegKey::_OpenKey(service.m_Key,
                                (PCUNICODE_STRING)&parametersPath,
                                &parameters.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status)) {
        goto defaultValues;
    }

    status = FxRegKey::_QueryULong(parameters.m_Key, &valueName, &numPages);
    if (!NT_SUCCESS(status)) {
        goto defaultValues;
    }

    if (numPages == 0) {
        numPages = FxIFRMinLogPages;
    }

defaultValues:
    //
    // This behavior is different from KMDF. KMDF would allocate Average page count (5)
    // if Verifier is on otherwise 1 page if the request is large.
    // Since for UMDF the memory is coming from WudfRd, which does not know about verifier
    // we will give it max pages here.
    //
    if (numPages > FxIFRMaxLogPages) {
        numPages = FxIFRMaxLogPages;
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

Arguments:

Returns:

--*/
{
    PWDF_IFR_HEADER pHeader;
    ULONG pageCount;
    ULONG sizeCb;
    HRESULT hr;
    IWudfDeviceStack2 *pDeviceStack2;
    PDRIVER_OBJECT_UM umDriverObject;
    PWCHAR serviceName;
    size_t bufferLengthCch;
    ULONG allocatedBufferLengthCb;
    LONG i;

    //
    // Return early if IFR is disabled.
    //
    if (FxLibraryGlobals.IfrDisabled) {
        ASSERT(FxDriverGlobals->WdfLogHeader == NULL);
        return;
    }

    WDFCASSERT(FxIFRRecordSignature == WDF_IFR_RECORD_SIGNATURE);

    if (FxDriverGlobals == NULL || FxDriverGlobals->WdfLogHeader != NULL) {
        return;
    }

    //
    // It is safe to use StringCchLength here as WudfHost makes sure that this
    // RegistryPath is null terminated
    //
    hr = StringCchLength(RegistryPath->Buffer,
                         RegistryPath->MaximumLength/sizeof(WCHAR),
                         &bufferLengthCch);

    if (FAILED(hr)) {
        return;
    }

    //
    // Lets find the last '\' that will mark the begining of the service name
    //
    for (i = (ULONG)bufferLengthCch - 1;
        i >= 0 && RegistryPath->Buffer[i] != '\\';
        i--);

    //
    // We did not find the service name
    //
    if (i < 0) {
        return;
    }

    serviceName = &RegistryPath->Buffer[i+1];

    sizeCb = FxIFRGetSize(FxDriverGlobals, RegistryPath);
    pageCount = sizeCb / PAGE_SIZE;

    //
    // Get the IWudfDeviceStack interface
    //
    umDriverObject = (PDRIVER_OBJECT_UM)DriverObject;

    pDeviceStack2 = (IWudfDeviceStack2 *)umDriverObject->WudfDevStack;

    if(pDeviceStack2 == NULL) {
        return;
    }

    allocatedBufferLengthCb = 0;
    hr = pDeviceStack2->AllocateIfrMemory(serviceName,
                                          pageCount,
                                          FALSE,
                                          TRUE,
                                          (PVOID *)&pHeader,
                                          &allocatedBufferLengthCb);

    if (pHeader == NULL || allocatedBufferLengthCb <= sizeof(WDF_IFR_HEADER)) {
        return;
    }

    //
    // Initialize the header.
    // Base will be where the IFR records are placed.
    // WPP_ThisDir_CTLGUID_FrameworksTraceGuid
    //
    RtlCopyMemory(&pHeader->Guid, (PVOID) &WdfTraceGuid, sizeof(GUID));

    pHeader->Base = (PUCHAR) &pHeader[1];
    pHeader->Size = allocatedBufferLengthCb - sizeof(WDF_IFR_HEADER);

    pHeader->Offset.u.s.Current  = 0;
    pHeader->Offset.u.s.Previous = 0;
    pHeader->SequenceNumberPointer = &(DriverObject->IfrSequenceNumber);

    StringCchCopyA(pHeader->DriverName, WDF_IFR_HEADER_NAME_LEN, FxDriverGlobals->Public.DriverName);

    FxDriverGlobals->WdfLogHeader = pHeader;

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
                        "FxIFR logging started" );

    if (sizeCb > FxIFRMinLogSize) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
            "FxIFR has been started with a size override:  size 0x%x bytes, "
            "# Pages %d.  An extended IFR size may not be written to a minidump!",
            sizeCb, sizeCb/PAGE_SIZE);
    }

    if (sizeCb != allocatedBufferLengthCb) {
        ASSERTMSG("FxIFR requested buffer size could not be allocated",FALSE);
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDRIVER,
            "FxIFR requested an allocation of size 0x%x bytes, "
            "Allocated memory was of size 0x%x bytes",
            sizeCb, allocatedBufferLengthCb);
    }
}

VOID
FxIFRStop(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
/*++

Routine Description:

    This routine stops the In-Flight Recorder (IFR).

    For UMDF FxIFRStop is no longer required as WudfRd manages the buffer's lifetime
    The buffer is kept alive till WudfRd unloads to aid in debugging in cases of
    WudfHost crash or in dumps with WudfHost paged out or not captured

Arguments:

Returns:

--*/
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);

    //
    // Uncomment the code below if you add any logic to this function.
    //
    // if (FxLibraryGlobals.IfrDisabled) {
    //     ASSERT(FxDriverGlobals->WdfLogHeader == NULL);
    //     return;
    // }
    //
}

_Must_inspect_result_
NTSTATUS
FxIFR(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in UCHAR              MessageLevel,
    __in ULONG              MessageFlags,
    __in LPGUID             MessageGuid,
    __in USHORT             MessageNumber,
         ...
    )
/*++

Routine Description:

    This routine is the main In-Flight Recorder (IFR) routine.

    It captures a WPP message to the IFR log.
    The IFR is always running, e.g. not WPP logger is necessary
    to start logging.

Arguments:

    MessageLevel  - The WPP message level for this event
    MessageFlags  - The WPP message flags for this event (see trace GUID)
    MessageGuid   - The tracewpp generated guid for module emitting this event.
    MessageNumber - The tracewpp generated message number within
                    the emitting module.
    ...           - Variable arguments associates with the emitted message.

Returns:

    NTSTATUS

--*/
{
    size_t            size;
    PWDF_IFR_RECORD   record;

    UNREFERENCED_PARAMETER( MessageLevel );

    //
    // Return early if IFR is disabled.
    //
    if (FxLibraryGlobals.IfrDisabled) {
        ASSERT(FxDriverGlobals->WdfLogHeader == NULL);
        return STATUS_SUCCESS;
    }

    if ( FxDriverGlobals->WdfLogHeader == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

























    UNREFERENCED_PARAMETER( MessageFlags );


    //
    // Determine the number bytes to follow header
    //
    size = 0;   // For Count of Bytes

    //
    // Determine how much log space is needed for this
    // trace record's data.
    //
    {
        va_list   ap;
        size_t    argLen;

        va_start(ap, MessageNumber);
#pragma prefast(suppress: __WARNING_BUFFER_OVERFLOW, "Recommneded by EndClean");
        while ((va_arg(ap, PVOID)) != NULL) {

            argLen = va_arg(ap, size_t);

            if (argLen > 0) {

                if (argLen > FxIFRMaxMessageSize) {
                    goto drop_message;
                }
                size += (USHORT) argLen;
            }
        }

        va_end(ap);

        //
        // NOTE: The final size must be 32-bit (ULONG) aligned.
        //       This is necessary for IA64 to prevent Alignment Faults.
        //
        size += (size % sizeof(ULONG)) ? sizeof(ULONG) - (size % sizeof(ULONG)) : 0;

        if (size > FxIFRMaxMessageSize) {
            goto drop_message;
        }
    }

    size += sizeof(WDF_IFR_RECORD);

    //
    // Allocate log space of the calculated size
    //
    {
        PWDF_IFR_HEADER  header;
        WDF_IFR_OFFSET   offsetRet;
        WDF_IFR_OFFSET   offsetCur;
        WDF_IFR_OFFSET   offsetNew;
        USHORT           usSize = (USHORT) size;  // for a prefast artifact.

        header = (PWDF_IFR_HEADER) FxDriverGlobals->WdfLogHeader;
        ASSERT(header->Size < FxIFRMaxLogSize); // size doesn't include header.
        ASSERT(header->Size >= header->Offset.u.s.Current);
        ASSERT(header->Size >= header->Offset.u.s.Previous);

        offsetRet.u.AsLONG = header->Offset.u.AsLONG;
        offsetNew.u.AsLONG = offsetRet.u.s.Current;

        do {
            offsetCur.u.AsLONG = offsetRet.u.AsLONG;

            if (&header->Base[header->Size] < &header->Base[offsetCur.u.s.Current+size]) {

                offsetNew.u.s.Current  = 0;
                offsetNew.u.s.Previous = offsetRet.u.s.Previous;

                offsetRet.u.AsLONG =
                    InterlockedCompareExchange( &header->Offset.u.AsLONG,
                                                offsetNew.u.AsLONG,
                                                offsetCur.u.AsLONG );

                if (offsetCur.u.AsLONG != offsetRet.u.AsLONG) {
                    continue;
                } else {
                    offsetNew.u.s.Current  = offsetCur.u.s.Current + usSize;
                    offsetNew.u.s.Previous = offsetRet.u.s.Current;
                }
            } else {

                offsetNew.u.s.Current  = offsetCur.u.s.Current + usSize;
                offsetNew.u.s.Previous = offsetCur.u.s.Current;
            }

            offsetRet.u.AsLONG =
                InterlockedCompareExchange( &header->Offset.u.AsLONG,
                                            offsetNew.u.AsLONG,
                                            offsetCur.u.AsLONG );

        } while (offsetCur.u.AsLONG != offsetRet.u.AsLONG);

        record = (PWDF_IFR_RECORD) &header->Base[offsetRet.u.s.Current];

        // RtlZeroMemory( record, sizeof(WDF_IFR_RECORD) );

        //
        // Build record (fill all fields!)
        //
        record->Signature     = FxIFRRecordSignature;
        record->Length        = (USHORT) size;
        record->PrevOffset    = (USHORT) offsetRet.u.s.Previous;
        record->MessageNumber = MessageNumber;
        record->Sequence      = InterlockedIncrement(header->SequenceNumberPointer);
        record->MessageGuid   = *MessageGuid;
    }

    //
    // Move variable part of data into log.
    //
    {
        va_list  ap;
        size_t   argLen;
        PVOID    source;
        PUCHAR   argsData;

        argsData = (UCHAR*) &record[1];

        va_start(ap, MessageNumber);
#pragma prefast(suppress: __WARNING_BUFFER_OVERFLOW, "Recommneded by EndClean");
        while ((source = va_arg(ap, PVOID)) != NULL) {

            argLen = va_arg(ap, size_t);

            if (argLen > 0) {

                RtlCopyMemory( argsData, source, argLen );
                argsData += argLen;
            }
        }

        va_end(ap);
    }

    return STATUS_SUCCESS;

    {
        //
        // Increment sequence number to indicate dropped message
        //
drop_message:
        PWDF_IFR_HEADER  header;
        header = (PWDF_IFR_HEADER) FxDriverGlobals->WdfLogHeader;
        InterlockedIncrement(header->SequenceNumberPointer);
        return STATUS_UNSUCCESSFUL;
    }
}

