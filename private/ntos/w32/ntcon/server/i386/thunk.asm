        title  "Thunks"
;++
;
; Copyright (c) 1985 - 1999, Microsoft Corporation
;
; Module Name:
;
;    thunk.asm
;
; Abstract:
;
;   This module implements all Win32 thunks. This includes the
;   first level thread starter...
;
; Author:
;
;   Mark Lucovsky (markl) 28-Sep-1990
;
; Revision History:
;
;   Jim Anderson (jima) 31-Oct-1994
;
;     Copied from base for worker thread cleanup
;
;--
.386p
        .xlist
include callconv.inc
        .list

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;;      align  512

        page ,132

;++
;
; VOID
; SwitchStackThenTerminate(
;     IN PVOID StackLimit,
;     IN PVOID NewStack,
;     IN DWORD ExitCode
;     )
;
;
; Routine Description:
;
;     This API is called during thread termination to delete a thread's
;     stack, switch to a stack in the thread's TEB, and then terminate.
;
; Arguments:
;
;     StackLimit (esp+4) - Supplies the address of the stack to be freed.
;
;     NewStack (esp+8) - Supplies an address within the terminating threads TEB
;         that is to be used as its temporary stack while exiting.
;
;     ExitCode (esp+12) - Supplies the termination status that the thread
;         is to exit with.
;
; Return Value:
;
;     None.
;
;--
        EXTRNP  _FreeStackAndTerminate,2
cPublicProc _SwitchStackThenTerminate,3

        mov     ebx,[esp+12]    ; Save Exit Code
        mov     eax,[esp+4]     ; Get address of stack that is being freed
        mov     esp,[esp+8]     ; Switch to TEB based stack

        push    ebx             ; Push ExitCode
        push    eax             ; Push OldStack
        push    -1
IFDEF STD_CALL
        jmp     _FreeStackAndTerminate@8
ELSE
        jmp     _FreeStackAndTerminate
ENDIF

stdENDP _SwitchStackThenTerminate

_TEXT   ends
        end

