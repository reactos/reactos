        title  "Thread Startup"

;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    threadbg.asm
;
; Abstract:
;
;    This module implements the code necessary to startup a thread in kernel
;    mode.
;
; Author:
;
;    Bryan Willman (bryanwi) 22-Feb-1990, derived from DaveC's code.
;
; Environment:
;
;    Kernel mode only, IRQL APC_LEVEL.
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include i386\kimacro.inc
include callconv.inc
        .list

        EXTRNP   KfLowerIrql,1,IMPORT, FASTCALL
        EXTRNP   _KeBugCheck,1
        extrn   _KiServiceExit2:PROC

        page ,132
        subttl  "Thread Startup"

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; Routine Description:
;
;    This routine is called at thread startup. Its function is to call the
;    initial thread procedure. If control returns from the initial thread
;    procedure and a user mode context was established when the thread
;    was initialized, then the user mode context is restored and control
;    is transfered to user mode. Otherwise a bug check will occur.
;
;
; Arguments:
;
;   (TOS)    = SystemRoutine - address of initial system routine.
;   (TOS+4)  = StartRoutine - Initial thread routine.
;   (TOS+8)  = StartContext - Context parm for initial thread routine.
;   (TOS+12) = UserContextFlag - 0 if no user context, !0 if there is one
;   (TOS+16) = Base of KTrapFrame if and only if there's a user context.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiThreadStartup    ,1

        xor     ebx,ebx             ; clear registers
        xor     esi,esi             ;
        xor     edi,edi             ;
        xor     ebp,ebp             ;
        mov     ecx, APC_LEVEL
        fstCall KfLowerIrql         ; KeLowerIrql(APC_LEVEL)

        pop     eax                 ; (eax)->SystemRoutine
        call    eax                 ; SystemRoutine(StartRoutine, StartContext)
IFNDEF STD_CALL
        add     esp,8               ; Clear off args
ENDIF

        pop     ecx                 ; (ecx) = UserContextFlag
        or      ecx, ecx
        jz      short kits10              ; No user context, go bugcheck

        mov     ebp,esp             ; (bp) -> TrapFrame holding UserContext

        jmp     _KiServiceExit2

kits10: stdCall _KeBugCheck, <NO_USER_MODE_CONTEXT>

stdENDP _KiThreadStartup

_TEXT$00   ends
        end

