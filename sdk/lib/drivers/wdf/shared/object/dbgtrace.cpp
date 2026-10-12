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

#if DBG && defined(__REACTOS__)

/**
 * @brief Returns the printf conversion used in place of a WPP %!Type! token.
 */
static
PCSTR
FxTraceGetTypeConversion(
    _In_reads_(TypeLength) PCSTR TypeName,
    _In_ SIZE_T TypeLength)
{
    if (TypeLength == 2 && strncmp(TypeName, "wZ", 2) == 0)
        return "%wZ";

    if (TypeLength == 4 && strncmp(TypeName, "GUID", 4) == 0)
        return "%p";

    if (TypeLength == 4 && strncmp(TypeName, "FUNC", 4) == 0)
        return "(function)";

    if (TypeLength == 7 && strncmp(TypeName, "BOOLEAN", 7) == 0)
        return "%u";

    /* Status codes, error codes and enumerations all arrive as 32-bit values */
    return "0x%08lx";
}

/**
 * @brief Rewrites WPP %!Type! tokens in a trace format into printf conversions.
 */
static
VOID
FxTraceTranslateFormat(
    _In_z_ PCSTR Format,
    _Out_writes_z_(OutputSize) PCHAR Output,
    _In_ SIZE_T OutputSize)
{
    SIZE_T outIndex;
    SIZE_T convLength;
    PCSTR typeEnd;
    PCSTR conversion;

    outIndex = 0;

    while (*Format != ANSI_NULL && outIndex + 1 < OutputSize)
    {
        if (Format[0] == '%' && Format[1] == '%')
        {
            if (outIndex + 2 >= OutputSize)
                break;

            Output[outIndex++] = *Format++;
            Output[outIndex++] = *Format++;
            continue;
        }

        if (Format[0] == '%' && Format[1] == '!')
        {
            typeEnd = strchr(Format + 2, '!');
            if (typeEnd != NULL)
            {
                conversion = FxTraceGetTypeConversion(Format + 2,
                                                      typeEnd - (Format + 2));
                convLength = strlen(conversion);

                /* Stop rather than emit a partial conversion */
                if (outIndex + convLength >= OutputSize)
                    break;

                RtlCopyMemory(&Output[outIndex], conversion, convLength);
                outIndex += convLength;
                Format = typeEnd + 1;
                continue;
            }
        }

        Output[outIndex++] = *Format++;
    }

    Output[outIndex] = ANSI_NULL;
}

#endif

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
#ifdef __REACTOS__
#define     FORMAT_BUFFER_SIZE      512
    CHAR       formatBuffer[FORMAT_BUFFER_SIZE];
#endif
    NTSTATUS   status;

    va_start(list, DebugMessage);

    if (DebugMessage) {

#ifdef __REACTOS__
        FxTraceTranslateFormat(DebugMessage,
                               formatBuffer,
                               sizeof(formatBuffer));
#endif

        //
        // Using new safe string functions instead of _vsnprintf.
        // This function takes care of NULL terminating if the message
        // is longer than the buffer.
        //
#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
        status = RtlStringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
#ifdef __REACTOS__
                                      formatBuffer,
#else
                                      DebugMessage,
#endif
                                      list );
#else
        HRESULT hr;
        hr = StringCbVPrintfA( debugMessageBuffer,
                                      sizeof(debugMessageBuffer),
#ifdef __REACTOS__
                                      formatBuffer,
#else
                                      DebugMessage,
#endif
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
#ifdef __REACTOS__
        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
#else
        if(!NT_SUCCESS(status)) {
#endif

#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
            DbgPrint ("WDFTrace: RtlStringCbVPrintfA failed 0x%x\n", status);
#else
            OutputDebugString("WDFTrace: Unable to expand: ");
#ifdef __REACTOS__
            OutputDebugString(formatBuffer);
#else
            OutputDebugString(DebugMessage);
#endif
#endif
#ifdef __REACTOS__
            va_end(list);
#endif
            return;
        }
        if (DebugPrintLevel <= TRACE_LEVEL_ERROR ||
            (DebugPrintLevel <= DebugLevel &&
             ((DebugPrintFlag & DebugFlag) == DebugPrintFlag))) {
#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
            DbgPrint("WDFTrace: %s\n", debugMessageBuffer);
#else
            OutputDebugString("WDFTrace: ");
#ifdef __REACTOS__
            OutputDebugString(debugMessageBuffer);
#else
            OutputDebugString(DebugMessage);
#endif
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
