/*++

Copyright (c) Microsoft Corporation

ModuleName:

    DbgTrace.cpp

Abstract:

    Temporary file to be used until ETW can be used
    for UM

Author:



Revision History:



--*/

#include "fxobjectpch.hpp"

#if FX_CORE_MODE==FX_CORE_USER_MODE
#include "strsafe.h"
#endif

#if !defined(EVENT_TRACING)

VOID
__cdecl
DoTraceLevelMessage(
    __in PVOID FxDriverGlobals,
    __in ULONG   DebugPrintLevel,
    __in ULONG   DebugPrintFlag,
    __drv_formatString(FormatMessage)
    __in PCSTR   DebugMessage,
    ...
    )

/*++

Routine Description:

    Print the trace message to debugger.

Arguments:

    TraceEventsLevel - print level between 0 and 3, with 3 the most verbose

Return Value:

    None.

 --*/
 {
#if DBG
    UNREFERENCED_PARAMETER(FxDriverGlobals);

#define     TEMP_BUFFER_SIZE        1024
    va_list    list;
    CHAR       debugMessageBuffer[TEMP_BUFFER_SIZE];
    NTSTATUS   status;

    va_start(list, DebugMessage);

    if (DebugMessage) {

        //
        // Using new safe string functions instead of _vsnprintf.
        // This function takes care of NULL terminating if the message
        // is longer than the buffer.
        //
#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
        status = RtlStringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
                                      DebugMessage,
                                      list );
#else
        HRESULT hr;
        hr = StringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
                                      DebugMessage,
                                      list );


        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        {
            status = WinErrorToNtStatus(HRESULT_CODE(hr));
        }
        else
        {
            status = SUCCEEDED(hr) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
        }
#endif
        if(!NT_SUCCESS(status)) {

#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
            DbgPrint ("WDFTrace: RtlStringCbVPrintfA failed 0x%x\n", status);
#else
            OutputDebugString("WDFTrace: Unable to expand: ");
            OutputDebugString(DebugMessage);
#endif
            return;
        }
        if (DebugPrintLevel <= TRACE_LEVEL_ERROR ||
            (DebugPrintLevel <= DebugLevel &&
             ((DebugPrintFlag & DebugFlag) == DebugPrintFlag))) {
#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
            DbgPrint("WDFTrace: %s", debugMessageBuffer);
#else
            OutputDebugString("WDFTrace: ");
            OutputDebugString(DebugMessage);
#endif
        }
    }
    va_end(list);

    return;
#else
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    UNREFERENCED_PARAMETER(DebugPrintLevel);
    UNREFERENCED_PARAMETER(DebugPrintFlag);
    UNREFERENCED_PARAMETER(DebugMessage);
#endif
}

#endif
