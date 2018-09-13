	subttl	emfinit.asm - Emulator initialization and FINIT instruction
        page
;*******************************************************************************
;emfinit.asm - Emulator initialization and FINIT instruction
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


EM_ENTRY eEmulatorInit
EmulatorInit:
EM_ENTRY eFINIT
eFINIT:
	mov	esi,BEGstk
	mov	EMSEG:[CURstk],INITstk
	mov	ecx,Numlev
	xor	eax,eax

EmInitLoop:
	mov	EMSEG:[esi].ExpSgn,bTAG_EMPTY		;Exponent and sign are zero
	mov	EMSEG:[esi].lManHi,eax
	mov	EMSEG:[esi].lManLo,eax

	add	esi, Reg87Len
	loop	EmInitLoop

	mov	EMSEG:[StatusWord],ax			; clear status word
	mov	[esp+4].OldStatus,ax			; clear saved status word.
	mov	EMSEG:[PrevCodeOff],eax
	mov	EMSEG:[PrevDataOff],eax
	mov	EMSEG:[LongControlWord],InitControlWord
	mov	eax,offset Round64near
	mov	EMSEG:[RoundMode],eax			;Address of round routine
	mov	EMSEG:[TransRound],eax			;Address of round routine
	mov	EMSEG:[SavedRoundMode],eax
	mov	EMSEG:[ZeroVector],offset SaveResult
	mov	EMSEG:[Einstall], 1
	ret
