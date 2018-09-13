        page    78,132
        title   emulator - 80387 emulator for flat 32-bit OS
;*******************************************************************************
;        Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;emulator.asm -  80387 emulator
;       by Tim Paterson
;
;Revision History:
;
; []    09/05/91  TP    Initial 32-bit version.
; []    11/13/92  JWM   Bug fixes for esp-indexed addressing, handling of denormals.
; []    01/18/93  JWM   Bug fixes for preservation of condition & error codes.
;
;*******************************************************************************

        .386p
        .387
        .model  flat,Pascal
        option oldstructs                               ;JWM

;*******************************************************************************
;
;   Define segments.
;
;*******************************************************************************


;These equates give access to the program that's using floating point.
dseg    equ     ss                      ;Segment of program's data
cseg    equ     es                      ;Segment of program's code

edata           segment dword public 'FAR_DATA'
edata           ends

ecode           segment dword public 'CODE'
ecode           ends


assume  cs:ecode

ifdef NT386
assume ds:nothing
assume fs:edata
else
assume ds:edata
assume fs:nothing
endif

assume  es:nothing
assume  gs:nothing
assume  ss:nothing

ifdef NT386
        include  ks386.inc
        include  nt386npx.inc
        include  callconv.inc
        include ..\..\..\inc\vdmtib.inc
endif                           ; NT386

;*******************************************************************************
;
;   List external functions.
;
;*******************************************************************************

ifdef  NT386
        EXTRNP   _NtRaiseException,3
        EXTRNP   _RtlRaiseStatus,1
        EXTRNP   _ZwRaiseException,3
        EXTRNP   _NpxNpSkipInstruction,1
endif           ; NT386

ifdef _DOS32EXT
        extern  _SelKrnGetEmulData:NEAR
        extern  DOS32RAISEEXCEPTION:NEAR
endif           ; _DOS32EXT

ifdef _CRUISER
        extern  DOS32IRAISEEXCEPTION:near
endif           ; CRUISER


;*******************************************************************************
;
;   Segment override macro (for NT)
;
;*******************************************************************************

ifdef NT386
        EMSEG EQU FS
else
        EMSEG EQU DS
endif

;;*******************************************************************************
;;
;;   Include some more macros and constants.
;;
;;*******************************************************************************
;
        include em387.inc
        include emstack.inc             ; stack management macros
;**************************************************************************
;**************************************************************************
;**************************************************************************
subttl  emulator.asm - Emulator Task DATA Segment
page
;*********************************************************************;
;                                                                     ;
;                 Emulator Task DATA Segment                          ;
;                                                                     ;
;*********************************************************************;

edata   segment

ifdef NT386
        db size EmulatorTebData dup (?) ; Make space for varibles
else					; ifdef NT386

Numlev          equ     8               ; Number of stack registers

InitControlWord	equ	37FH		; Default - Round near,
					; 64 bits, all exceptions masked

RoundMode       dd      ?               ;Address of rounding routine
SavedRoundMode  dd      ?               ;For restoring RoundMode
ZeroVector      dd      ?               ;Address of sum-to-zero routine
TransRound      dd      ?               ;Round mode w/o precision
Result          dd      ?               ;Result pointer

PrevCodeOff     dd      ?
PrevDataOff     dd      ?

(See note below on 'Emulator stack area')
CURstk          dd      ?

XBEGstk		db	(Numlev-1)*Reg87Len dup(?)	;Allocate register 1 - 7

BEGstk EQU offset edata:XBEGstk
INITstk EQU offset edata:XINITstk
ENDstk EQU offset edata:XENDstk

FloatTemp       db      Reg87Len dup(?)
ArgTemp         db      Reg87Len dup(?)

public Trap7Handler
Trap7Handler    dd      0

;We're DWORD aligned at this point

LongStatusWord  label   dword           ;Combined Einstall, CURerr, StatusWord
.erre   Einstall eq $
.erre   StatusWord eq $+1
.erre   CURerr eq $+3

Einstall        db      0               ; Emulator installed flag

StatusWord      label   word
    SWerr       db      ?               ; Initially no exceptions (sticky flags)
CurErrCond      label   word            ; Combined error and condition codes
    SWcc        db      ?               ; Condition codes from various operations

    CURerr      db      ?               ; initially 8087 exception flags clear
                                        ; this is the internal flag reset after
                                        ; each operation to detect per instruction
                                        ; errors

LongControlWord label   dword           ;Combined ControlWord and ErrMask
.erre   ControlWord eq $
.erre   ErrMask eq $+2

ControlWord     label   word
    CWmask      db      ?               ; exception masks
    CWcntl      db      ?               ; arithmetic control flags

    ErrMask     db      ?
    dummy       db      ?

endif                                   ; ifdef NT386 else

;*******************************************************************************
;
; Emulator stack area
;
;The top of stack pointer CURstk is initialized to the last register 
;in the list; on a real 8087, this corresponds to hardware register 0.
;The stack grows toward lower addresses, so the first push (which is
;hardware register 7) is stored into the second-to-last slot.  This gives
;the following relationship between hardware registers and memory
;locations:
;
; BEGstk --> |    reg 1    |  (lowest memory address)
; 	     |    reg 2    |
; 	     |    reg 3    |
; 	     |    reg 4    |
; 	     |    reg 5    |
; 	     |    reg 6    |
; 	     |    reg 7    |
; 	     |    reg 0    |  <-- Initial top of stack (empty)
; ENDstk -->
;
;This means that the wrap-around case on decrementing CURstk will not
;occur until the last (8th) item is pushed.
;
;Note that the physical register numbers are only used in regard to
;the tag word.  All other operations are relative the current top.


edata	ends

subttl  emulator.asm
page
;*********************************************************************;
;                                                                     ;
;               Start of Code Segment                                 ;
;                                                                     ;
;*********************************************************************;


ecode segment

;         public  __fpemulatorbegin       ; unused code label commented out for BBT
;__fpemulatorbegin equ       $            ; emulator really starts here   


        include emfinit.asm
        include emerror.asm             ; error handler
        include emdisp.asm              ; dispatch tables

        include emf386.asm              ; Flat 386 emulation entry
        include emdecode.asm            ; instruction decoder

        include emarith.asm             ; arithmetic dispatcher
        include emfadd.asm              ; add and subtract
        include emfmul.asm              ; multiply
        include emfdiv.asm              ; division
        include emround.asm             ; rounding
        include emload.asm              ; load memory operands
        include emstore.asm             ; store memory operands
        include emfmisc.asm             ; miscellaneous instructions
        include emfcom.asm              ; compare
        include emfconst.asm            ; constant loading
        include emlsbcd.asm             ; packed BCD conversion
        include emxtract.asm            ; xtract and scale
        include emfprem.asm             ; partial remainder
        include emtrig.asm              ; trig instructions
        include emftran.asm             ; transcendentals
        include emlsenv.asm
        include emfsqrt.asm             ; square root
ifndef NT386
        include emccall.asm
endif

UNUSED:
eFSETPM:
eFNOP:
eFENI:
eFDISI:
        ret                     ;Return to EMLFINISH


;        public  __fpemulatorend         ; unused code label commented out for BBT
;__fpemulatorend equ     $               ; emulator ends here  ; commented out for BBT

ecode   ends
END
