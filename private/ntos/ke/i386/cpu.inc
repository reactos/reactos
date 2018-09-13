;++
;
;   Copyright (c) 1989  Microsoft Corporation
;
;   Module Name:
;
;       cpu.inc
;
;   Abstract:
;
;       This module contains the assembly structures and definitions
;       for INTEL 80x86 CPU specifiec information.  This include file
;       is mainly used by CPU.ASM to determine CPU type and stepping
;       number.
;
;   Author:
;
;       Shie-Lin (shielint) 1-Oct-1991
;
;   Revision History:
;
;--

;
; The following equates define the control bits of CR0 register
;

CR0_AM          equ     40000h
CR0_ET          equ     00010h

;
; The following equates define the control bits of EFALGS register
;

EFLAGS_AC       equ     40000h
EFLAGS_VM       equ     20000h
EFLAGS_RF       equ     10000h
EFLAGS_NF       equ     4000h
EFLAGS_IOPL     equ     3000h
EFLAGS_IF       equ     200h
EFLAGS_TF       equ     100h
EFLAGS_ID       equ     200000h

;
; Define the iret frame
;

IretFrame       struc

IretEip        dd      0
IretCs         dd      0
IretEFlags     dd      0

IretFrame       ends

;
; Misc. definitions
;

ADDRESS_OVERRIDE        equ     67h
OPERAND_OVERRIDE        equ     66h
