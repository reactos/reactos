        title  "NtCurTeb.asm"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    NtCurTeb.asm
;
; Abstract:
;
;    Efficient NtCurrentTeb code.
;
; Author:
;
;    Bryan Willman (bryanwi) 28 feb 90
;
; Environment:
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

        page ,132
        subttl  "NtCurrentTeb"

IFDEF NTOS_KERNEL_RUNTIME
.PAGE   SEGMENT DWORD PUBLIC 'CODE'
ELSE
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
ENDIF
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; PTEB
; NtCurrentTeb();
;
; Routine Description:
;
;    This routine returns the address of the current TEB.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    Address of TEB.
;
;--
cPublicProc _NtCurrentTeb   ,0
cPublicFpo 0,0

;
;   How this works in both user and kernel mode.
;
;   In user mode, TEB.TIB.Self is flat address of containing structure.
;   In kernel mode, PCR.TIB.Self is flat address of the TEB.
;   Same offset both places, so fs:PcTeb is always the flat address
;   of the TEB.
;

        mov     eax,fs:[PcTeb]
        stdRET    _NtCurrentTeb

stdENDP _NtCurrentTeb
IFDEF NTOS_KERNEL_RUNTIME
.PAGE           ENDS
ELSE
_TEXT           ENDS
ENDIF
                end
