COMMENT #
//***************************************************************************
//
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1993
//  All rights reserved
//
// Include file for using DPMI functionality in Windows.
//
//***************************************************************************
END COMMENT #


DPMI                    EQU     031h            ; DPMI interrupt vector
ALLOC_LDT_DESC          EQU     0000h
SEG_TO_DESC             EQU 	0002h
SET_SEG_BASE_ADDR       EQU 	0007h
SET_SEG_LIMIT           EQU 	0008h
ALLOC_DOS_MEM_BLOCK     EQU 	0100h
FREE_DOS_MEM_BLOCK      EQU 	0101h
SET_REALMODE_INTVEC     EQU 	0201h
GET_REALMODE_INTVEC     EQU 	0200h
SIMULATE_RELMODE_INT    EQU 	0300h
CALL_REALMODE_PROC      EQU     0301h

; ***************************************************************************

EXTERNDEF C     IsWindows:WORD
EXTERNDEF C    	IntVector:WORD
EXTERNDEF C    	DosMem:DWORD

; ***************************************************************************
;
; Macro for simulating a real mode interrupt from protect mode using DPMI.
; Before invoking the macro the registers should be setup for the desired
; real mode interrupt call. All register pointer must be to real mode
; memory obtained via DPMI.
;
; ***************************************************************************

DpmiInt MACRO Vector:REQ
    LOCAL   IsDos                           ; Local label
    LOCAL   EndMacro                        ; Local labe

    cmp     IsWindows, 0                    ; See if a Windows app
    je      IsDos                           ; If not not Windows use DOS int

    mov     IntVector, Vector               ; Set vector to simulate
    call    GenericDpmi                     ; Call generic DPMI function
    jmp     SHORT EndMacro

IsDos:
    int     Vector                          ; Do DOS interrupt

EndMacro:

ENDM

; ***************************************************************************
;
; Real Mode call structure for Simulating Real Mode Interrupts using DPMI
;
; ***************************************************************************

RealModeCallStruc   Struc

    RegEDI      dd  	?
    RegESI      dd  	?
    RegEBP      dd  	?
    Regres      dd  	?
    RegEBX      dd  	?
    RegEDX      dd  	?
    RegECX      dd  	?
    RegEAX      dd  	?
    RegFlg      dw  	?
    RegES       dw  	?
    RegDS       dw  	?
    RegFS       dw  	?
    RegGS       dw  	?
    RegIP       dw  	?
    RegCS       dw  	?
    RegSP       dw  	?
    RegSS       dw  	?

RealModeCallStruc	Ends


; ***************************************************************************
;
; Real Mode call structure for Simulating Real Mode Interrupts using DPMI
;
; ***************************************************************************

@proto_100      TYPEDEF     PROTO C
GenericDpmi 	PROTO       @proto_100

@proto_101              TYPEDEF     PROTO C
DpmiDskRdWr     PROTO       @proto_101
