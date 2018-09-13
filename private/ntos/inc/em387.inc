        subttl  em387.inc - Emulator Internal Format and Macros
	page
;***
;em387.inc - Emulator Internal Format and Macros
;
;	 Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1987, 1992
;
;	 All Rights Reserved
;
;Purpose:
;	Emulator Internal Format and Macros
;
;Revision History:  (also see emulator.hst)
;
;    8/23/91  TP    New tag definitions
;   10/30/89  WAJ   Added this header.
;   02/12/89  WAJ   Added local stack frame definition.
;
;*******************************************************************************


GetEmData	macro	dest,use
ifdef	_CRUISER
	mov	dest,[edataSEG]
elseifdef  _DOS32EXT
ifdifi	<use>,<ax>
	push	eax
	call	_SelKrnGetEmulData
	mov	dest,ax
	pop	eax
else
	call	_SelKrnGetEmulData
	mov	dest,ax
endif
endif
	endm



;The SKIP macro optimizes very short jumps by treating the code
;as data to a "cmp" instruction.  This reduces jump time from
;8 clocks or more down to 2 clocks.  It destroy the flags!

SKIP	macro	dist,target
if	dist eq 4
	db	3DH			;cmp eax,<immed>
elseif	dist eq 3
	db	3DH,0			;cmp eax,<immed>
elseif	dist eq 2
	db	66H,3DH			;cmp ax,<immed>
elseif	dist eq 1
	db	3CH			;cmp al,<immed>
else
	.err
endif

	ifnb	<target>
.erre	$+dist eq target
	endif

	endm

;*******************************************************************************
;
;   80x87 environment structures.
;
;*******************************************************************************


Env80x87_32 struc
	E32_ControlWord	dw	?
	reserved1		dw	?
	E32_StatusWord	dw	?
	reserved2		dw	?
	E32_TagWord		dw	?
	reserved3		dw	?
	E32_CodeOff		dd	?
	E32_CodeSeg		dw	?
	reserved4		dw	?
	E32_DataOff		dd	?
	E32_DataSeg		dw	?
	reserved5		dw	?
Env80x87_32 ends


;---------------------------------------------------------------------------
;
; Emulator Internal Format:
;
;	     +0  +1  +2  +3  +4  +5  +6  +7  +8  +9  +10 +11
;	    .___.___.___.___.___.___.___.___.___.___.___.___.
;   ptr --> |___|___|___|___|___|___|___|___|___|___|___|___|
;	     lsb			 msb tag sgn exl exh
;	    |<---      mantissa 	--->|       |exponent
;
;   The mantissa contains the leading 1 before the decimal point in the hi
;   bit of the msb. The exponent is not biased (signed two's complement).
;   The flag and tag bytes are as below.
;
;   bit:      7   6   5   4   3   2   1   0
;	    .___.___.___.___.___.___.___.___.
;   Sign:   |___|_X_|_X_|_X_|_X_|_X_|_X_|_X_|  X = unused
;	      ^ 
;     SIGN
;
;
;   bit:      7   6   5   4   3   2   1   0
;	    .___.___.___.___.___.___.___.___.
;   Tag:    |___|___|_X_|_X_|___|___|___|___|  X = unused
;	      ^	  ^	      ^	  ^   ^   ^
;	      |	  |	      |	  |   |	  |
;    387 tag -+---+	      |	  |   |   |
;			      |	  |   |	  |
;    Special enumeration -----+---+   |	  |
;				      |	  |
;    Internal tag --------------------+---+
;
;There are four internal tags: Single, Double, Zero, Special.  Within
;Special, there is NAN, Infinity, Denormal, and Empty.
;
;Representations for Single, Double, and Denormal are the same.  Denormals
;are not actually kept denormalized, although they are rounded to the
;correct number of bits as if they were.  The Single tag means the 
;low 32 bits of the mantissa are zero.  This allows optimizing multiply 
;and divide.
;
;Tag		Mantissa	Exponent	Sign
;---------------------------------------------------
;Zero		0		0		valid
;Empty		?		?		?
;NAN		valid		TexpMax		valid
;Infinity	8000...000	TexpMax		valid
;
;The mantissa for a NAN distinguishes between a quiet NAN (QNAN) or a 
;signaling NAN (SNAN).  If the bit below the MSB is 1, it is a QNAN,
;otherwise it is an SNAN.
;


;*******************************************************************************
;*
;*  Stack entry defineds with a struct.
;*
;*******************************************************************************

EmStackEntry struc
    bMan0   db	    ?
    bMan1   db	    ?
    bMan2   db	    ?
    bMan3   db	    ?
    bMan4   db	    ?
    bMan5   db	    ?
    bMan6   db	    ?
    bMan7   db	    ?
    bTag    db	    ?
    bSgn    db      ?
    bExpLo  db      ?
    bExpHi  db      ?
EmStackEntry ends

wMantisa struc
    wMan0   dw	    ?
    wMan1   dw	    ?
    wMan2   dw	    ?
    wMan3   dw	    ?
    TagSgn  dw	    ?
    wExp    dw	    ?
wMantisa ends


lMantisa struc
    lManLo	dd	?
    lManHi	dd	?
    ExpSgn	dd	?
lMantisa ends

.erre	size lMantisa eq size wMantisa

Reg87Len	equ	size lMantisa


;*******************************************************************************
;*
;*  bFlags and bTag constants.
;*
;*******************************************************************************

;The rules for internal number formats:
;
;1. Everything is either normalized or zero--unnormalized formats cannot
;get in.  So if the high half mantissa is zero, the number must be all zero.
;
;2. Although the exponent bias is different, NANs and Infinities are in
;standard IEEE format - exponent is TexpMax, mantissa indicates NAN vs.
;infinity (mantissa for infinity is 800..000H).
;
;3. Denormals have an exponent less than TexpMin.
;
;4. If the low half of the mantissa is zero, it is tagged bTAG_SNGL
;
;5. Everything else is bTAG_VALID


bSign	    equ     80h

;These are the INTERNAL flags
TAG_MASK	equ	3
TAG_SHIFT	equ	2
;
TAG_SNGL	equ	0		;SINGLE: low 32 bits are zero
TAG_VALID	equ	1
TAG_ZERO	equ	2
TAG_SPCL	equ	3		;NAN, Infinity, Denormal, Empty
ZEROorSPCL      equ     2	       ;Test for Zero or Special
;Enumeration of "special":
TAG_SPCLBITS	equ	0CH
TAG_EMPTY	equ	TAG_SPCL+(0 shl TAG_SHIFT)
TAG_INF		equ	TAG_SPCL+(1 shl TAG_SHIFT)
TAG_NAN		equ	TAG_SPCL+(2 shl TAG_SHIFT)
TAG_DEN		equ	TAG_SPCL+(3 shl TAG_SHIFT)

;These are the tags used by the 387
T87_VALID	equ	0
T87_ZERO	equ	1
T87_SPCL	equ	2		;NAN, Infinity, Denormal
T87_EMPTY	equ	3

;The tag word for each stack entry combines these two tags.
;Internal tags are in the low bits, 387 tags are in the high two bits
bTAG_VALID	equ	(T87_VALID shl 6) or TAG_VALID
bTAG_SNGL	equ	(T87_VALID shl 6) or TAG_SNGL
bTAG_ZERO	equ	(T87_ZERO shl 6) or TAG_ZERO
bTAG_NAN	equ	(T87_SPCL shl 6) or TAG_NAN
bTAG_INF	equ	(T87_SPCL shl 6) or TAG_INF
bTAG_EMPTY	equ	(T87_EMPTY shl 6) or TAG_EMPTY
bTAG_DEN	equ	(T87_SPCL shl 6) or TAG_DEN
bTAG_NOPOP	equ	-1

bTAG_MASK   equ     3



MantissaByteCnt equ	8

IexpBias	equ	3FFFh	; 16,383
IexpMax 	equ	7FFFh	; Biased Exponent for Infinity
IexpMin 	equ	0	; Biased Exponent for zero

DexpBias	equ	3FFh	; 1023
DexpMax 	equ	7FFh	; Biased Exponent for Infinity
DexpMin 	equ	0	; Biased Exponent for zero

SexpBias	equ	07Fh	; 127
SexpMax 	equ	0FFh	; Biased Exponent for Infinity
SexpMin 	equ	0	; Biased Exponent for zero

TexpBias	equ	0	; Bias for internal format of temp real
UnderBias	equ	24576	; 3 * 2^13.  Extra bias for unmasked underflow
TexpMax		equ	IexpMax - IexpBias + TexpBias	;NAN/Infinity exponent
TexpMin		equ	IexpMin-IexpBias+1	;Smallest non-denormal exponent

; Control Word Format	CWcntl

RoundControl		equ	0Ch
    RCchop		equ	0Ch
    RCup		equ	08h
    RCdown		equ	04h
    RCnear		equ	 0

PrecisionControl	equ	03h
    PC24		equ	 0
    PC53		equ	02h
    PC64		equ	03h

; Status Word Format	SWcc
    C0			equ	01h
    C1			equ	02h
    C2			equ	04h
    C3			equ	40h
ConditionCode		equ	C3 or C2 or C1 or C0
    CCgreater		equ	 0
    CCless		EQU	C0
    CCequal		equ	C3
    CCincomprable	equ	C3 or C2 or C0

RoundUp			equ	C1
StackOverflow		equ	C1

; Status Flags Format	CURerr

Invalid 		equ	   1h		; chip status flags
Denormal		equ	   2h
ZeroDivide		equ	   4h
Overflow		equ	   8h
Underflow		equ	  10h
Precision		equ	  20h
StackFlag		equ	  40h
Summary 		equ	  80h

SavedErrs		equ	Invalid or Denormal or ZeroDivide or Overflow or Underflow or Precision or StackFlag
LongSavedFlags	equ	(CCincomprable SHL 16) OR (SavedErrs SHL 8)	; save C0, C2, C3 & errs
;*******************************************************************************
;*
;*  Define emulator interrupt stack frame.
;*
;*******************************************************************************

StackFrame   struc
            regEAX	    dd	    ?
	    regECX	    dd	    ?
	    regEDX	    dd	    ?
	    regEBX	    dd	    ?
	    regESP	    dd	    ?
	    regEBP	    dd	    ?
	    regESI	    dd	    ?
	    regEDI	    dd	    ?
	    OldCodeOff      dd      ?
	    OldLongStatus   dd      ?
	    regDS	    dd	    ?
	    regEIP	    dd	    ?
	    regCS	    dd	    ?
	    regFlg	    dd	    ?
StackFrame  ends

regAX	    equ		    word ptr regEAX

; .erre   StatusWord eq LongStatusWord+1
OldStatus   equ		    word ptr OldLongStatus+1

;*******************************************************************************
;*
;*  Define emulator entry point macro.
;*
;*******************************************************************************

EM_ENTRY	macro	entryname
ifdef NT386
public ___&entryname
___&entryname:
endif			; ifdef NT386
		endm

Em87Busy	EQU	1
Em87Idle	EQU	0



ifdef NT386
;*********************************************************************;
;								      ;
;                     Emulator TEB Layout                             ;
;								      ;
;*********************************************************************;

.errnz (TbSystemReserved1 and 3)        ; Make sure TB is dword aligned

Numlev          equ     8               ; Number of stack registers

InitControlWord	equ	37FH		; Default - Round near,
					; 64 bits, all exceptions masked

DefaultControlWord equ  27FH            ; Default - Round near,
                                        ; 53 bits, all exceptions masked

EmulatorTebData struc
    TbSystemResrvd  db      TbSystemReserved1 DUP (?)   ; Skip to Emulator area

    RoundMode       dd      ?           ; Address of rounding routine
    SavedRoundMode  dd      ?           ; For restoring RoundMode
    ZeroVector      dd      ?           ; Address of sum-to-zero routine
    TransRound      dd      ?           ; Round mode w/o precision
    Result          dd      ?           ; Result pointer
    PrevCodeOff     dd      ?
    PrevDataOff     dd      ?

    ;(See comment below on 'emulator stack area'
    CURstk          dd      ?           ; init to start of stack
    BEGstk          db      (Numlev-1)*Reg87Len dup(?) ;Allocate register 1 - 7
    INITstk         db      Reg87Len dup(?)

    FloatTemp       db      Reg87Len dup(?)
    ArgTemp         db      Reg87Len dup(?)

    Einstall        db      0           ; Emulator installed flag
    SWerr           db      ?           ; Initially no exceptions (sticky flags)
    SWcc            db      ?           ; Condition codes from various operations
    CURerr          db      ?           ; initially 8087 exception flags clear
                                        ; this is the internal flag reset after
                                        ; each operation to detect per instruction
                                        ; errors
    CWmask          db      ?           ; exception masks
    CWcntl          db      ?           ; arithmetic control flags
    ErrMask         db      ?
    dummy           db      ?
EmulatorTebData ends

ENDstk          equ byte ptr INITstk + Reg87Len
LongStatusWord  equ dword ptr Einstall  ;Combine Einstall, CURerr, StatusWord
StatusWord      equ word ptr SWerr      ;Combine SWerr, SWcc
CurErrCond      equ word ptr SWcc       ;Combine SWcc, CURErr
LongControlWord equ dword ptr CWmask    ;Combine CWMask, CWcntl, ErrMask, dummy
ControlWord     equ word ptr CWmask     ;Combine CWMask, CWcntl

YFloatTemp      equ FloatTemp
YArgTemp        equ ArgTemp

.errnz (SWerr   - Einstall -1)
.errnz (SWcc    - Einstall -2)
.errnz (CURerr  - Einstall -3)
.errnz (CWcntl  - CWmask   -1)
.errnz (ErrMask - CWmask   -2)
.errnz (dummy   - CWmask   -3)


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


endif
