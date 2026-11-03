/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Unwinding related ARM64 asm functions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

    IMPORT RtlpExecuteHandlerForUnwindHandler

    TEXTAREA

//
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForUnwind(
//     _Inout_ PEXCEPTION_RECORD ExceptionRecord,
//     _In_ PVOID EstablisherFrame,
//     _Inout_ PCONTEXT ContextRecord,
//     _In_ PDISPATCHER_CONTEXT DispatcherContext);
//
// The exception flags and the dispatcher context stay right below the
// caller's sp, RtlpExecuteHandlerForUnwindHandler finds them there through
// the establisher frame of this function.
//
    NESTED_ENTRY RtlpExecuteHandlerForUnwind

    stp     x29, x30, [sp, #-32]!
    ldr     w16, [x0, #ErExceptionFlags]
    stp     x16, x3, [sp, #16]

    ldr     x16, [x3, #DcLanguageHandler]
    blr     x16

    /* Keeps the return address out of the epilog, where the handler would not apply */
    nop

    ldp     x29, x30, [sp], #32
    ret
LABEL(RtlpExecuteHandlerForUnwindEnd)

    NESTED_END RtlpExecuteHandlerForUnwind

    XDATAAREA
LABEL(RtlpExecuteHandlerForUnwindXdata)
    /* Exception handler, the epilog reuses the prolog codes, one code word */
    dcd     ((RtlpExecuteHandlerForUnwindEnd - RtlpExecuteHandlerForUnwind) / 4) + 0x08300000
    dcb     0x83, 0xE4, 0xE3, 0xE3      // save_fplr_x 32, end, padding
    DCRVA   RtlpExecuteHandlerForUnwindHandler

    PDATAAREA
    DCRVA   RtlpExecuteHandlerForUnwind
    DCRVA   RtlpExecuteHandlerForUnwindXdata

    END
