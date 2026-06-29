/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Unwinding related x64 asm functions
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ksamd64.inc>

.code64

EXTERN RtlpExecuteHandlerForUnwindHandler:PROC

//
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForUnwind(
//    _Inout_ PEXCEPTION_RECORD* ExceptionRecord,
//    _In_ PVOID EstablisherFrame,
//    _Inout_ PCONTEXT ContextRecord,
//    _In_ PDISPATCHER_CONTEXT DispatcherContext);
//
PUBLIC RtlpExecuteHandlerForUnwind
#ifdef _USE_ML
RtlpExecuteHandlerForUnwind PROC FRAME:RtlpExecuteHandlerForUnwindHandler
#else
.PROC RtlpExecuteHandlerForUnwind
.seh_handler RtlpExecuteHandlerForUnwindHandler, @unwind, @except
.seh_handlerdata
.long 0 // Count
.seh_code
#endif

    /* Save ExceptionRecord->ExceptionFlags in the home space */
    mov eax, [rcx + ErExceptionFlags]
    mov [rsp + P1Home], eax

    /* Save DispatcherContext in the home space */
    mov [rsp + P4Home], r9

    sub rsp, 40
    .ALLOCSTACK 40
    .ENDPROLOG

    /* Call the language handler */
    call qword ptr [r9 + DcLanguageHandler]

    /* This nop prevents RtlVirtualUnwind from unwinding the epilog,
       which would lead to ignoring the handler */
    nop

    add rsp, 40
    ret

#ifdef _USE_ML
RtlpExecuteHandlerForUnwind ENDP
#else
.ENDP
#endif

END
