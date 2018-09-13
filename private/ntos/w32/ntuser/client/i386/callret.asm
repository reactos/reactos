        title  "User Callback Return"
;++
;
; Copyright (c) 1985 - 1999, Microsoft Corporation
;
; Module Name:
;
;    callret.asm
;
; Abstract:
;
;    This module implements the fastpath callback return.
;
; Author:
;
;    David N. Cutler (davec) 21-Dec-95
;
; Environment:
;
;    User mode.
;
; Revision History:
;
;--

.386p
        .xlist
include callconv.inc            ; calling convention macros
        .list

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Return from User Mode Callback"

ifndef BUILD_WOW6432
;++
;
; NTSTATUS
; FASTCALL
; XyCallbackReturn (
;    IN PVOID OutputBuffer OPTIONAL,
;    IN ULONG OutputLength,
;    IN NTSTATUS Status
;    )
;
; Routine Description:
;
;    This function returns from a user mode callout to the kernel mode
;    caller of the user mode callback function.
;
; Arguments:
;
;    OutputBuffer (ecx) - Supplies an optional pointer to an output buffer.
;
;    OutputLength (edx) - Supplies the length of the output buffer.
;
;    Status (esp + 4) - Supplies the status value returned to the caller of the
;        callback function.
;
; Return Value:
;
;    Normally there is no return from this function. If a callbac is not active,
;    then the error status is returned to the caller.
;
;--

cPublicFastCall XyCallbackReturn, 3

        mov     eax,[esp] + 4           ; get return status value
        int     02bH                    ; call fast path system service
        fstRET  XyCallbackReturn        ; return status to caller

fstENDP XyCallbackReturn
endif

_TEXT   ends
        end
