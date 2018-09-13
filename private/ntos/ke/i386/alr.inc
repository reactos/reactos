;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    alr.inc
;
; Abstract:
;
;    This inlcude file defines all the equates and macros specifically
;    used for ALR Multiprocessor system implementation.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 29-Oct-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--

;
; Virtual address map for ALR CBUS IO and address space
;   CBUS memory address space starting from 64MB to (128MB - 1)
;   CBUS IO space starting from 128MB to (192MB - 1).  We are interested
;       in the first 4MB only (at least for now).
; 

CBUS_ADDR_START_PHYS     equ    4000000h

CBUS_IO_SPACE_START      equ    90000000h
CBUS_IO_SPACE_START_PHYS equ    8000000h
CBUS_IO_SPACE_SIZE       equ    400000h

;
;CPU ID for CBUS PEs and common functions for PEs
;

ALLCPUID        equ     0Fh             ; ID to address all the slaves
BASECPUID       equ     0Eh             ; Base CPU ID

PE_CRESET       equ     0               ; Clear Reset
PE_SRESET       equ     1               ; Set Reset
PE_CONTEND      equ     2               ; Contend (Place slot number on ARB0-3
                                        ; lines)
PE_SETIDA       equ     3               ; Set ID value on winning processor
PE_CSWI         equ     4               ; Clear software interrupt
PE_SSWI         equ     5               ; Set software interrupt
PE_CNMI         equ     6               ; Clear NMI
PE_SNMI         equ     7               ; Set NMI
PE_SLED         equ     8               ; Set LED
PE_CLED         equ     9               ; Clear LED

;
; Miscs CBUS definitions
;

ArbitrateRegister       equ     0F1h
ArbitrateMask           equ     0Fh     ; Lower 4 bits of Arbitrate Register
                                        

;
; Macros to access CBUS I/O space
;
; CBUS_IO_ACCESS func, cpuid
;   func - the function which will be applied to PEs
;   cpuid - the desired PE.  If not specified, the cpuid is in AL register.
;

CBUS_IO_ACCESS  macro   func, cpuid

ifnb    <cpuid>
        mov     eax, (cpuid SHL 18) + CBUS_IO_SPACE_START + (func SHL 4)
else
        movzx   eax, al
        shl     eax, 18
        add     eax, CBUS_IO_SPACE_START + (func SHL 4)
endif
        or      byte ptr [eax], 0ffh

                endm


