        title  "Trap Processing"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    int.asm
;
; Abstract:
;
;    This module implements the code necessary to field and process i386
;    interrupt.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 8-Jan-1990
;
; Environment:
;
;    Kernel mode only.
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

;
; Interrupt flag bit maks for EFLAGS
;

EFLAGS_IF                       equ     200H
EFLAGS_SHIFT                    equ     9

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING

; NOTE  This routine is never actually called on standard x86 hardware,
;       because passive level doesn't actually exist.  It's here to
;       fill out the portable skeleton.
;
; The following code is called when a passive release occurs and there is
; no interrupt to process.
;

cPublicProc _KiPassiveRelease       ,0
        stdRET    _KiPassiveRelease                             ; cReturn
stdENDP _KiPassiveRelease


        page ,132
        subttl  "Disable Processor Interrupts"
;++
;
; BOOLEAN
; KiDisableInterrupts(
;    VOID
;    )
;
; Routine Description:
;
;    This routine disables interrupts at the processor level.  It does not
;    edit the PICS or adjust IRQL, it is for use in the debugger only.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    (eax) = !0 if interrupts were on, 0 if they were off
;
;--
cPublicProc _KiDisableInterrupts    ,0
cPublicFpo 0, 0
        pushfd
        pop     eax
        and     eax,EFLAGS_IF               ; (eax) = the interrupt bit
        shr     eax,EFLAGS_SHIFT            ; low bit of (eax) == interrupt bit
        cli
        stdRET    _KiDisableInterrupts

stdENDP _KiDisableInterrupts


        page ,132
        subttl  "Restore Processor Interrupts"
;++
;
; VOID
; KiRestoreInterrupts(
;    BOOLEAN Restore
;    )
;
; Routine Description:
;
;    This routine restores interrupts at the processor level.  It does not
;    edit the PICS or adjust IRQL, it is for use in the debugger only.
;
; Arguments:
;
;    Restore (esp+4) - a "boolean" returned by KiDisableInterrupts, if
;                       !0 interrupts will be turned on, else left off.
;
;       NOTE: We don't actually test the boolean as such, we just or
;             it directly into the flags!
;
; Return Value:
;
;    none.
;
;--
cPublicProc _KiRestoreInterrupts    ,1
cPublicFpo 1, 0
        xor     eax, eax
        mov     al, byte ptr [esp]+4
        shl     eax,EFLAGS_SHIFT            ; (eax) == the interrupt bit
        pushfd
        or      [esp],eax                   ; or EI into flags
        popfd
        stdRET    _KiRestoreInterrupts

stdENDP _KiRestoreInterrupts

_TEXT   ends
        end
