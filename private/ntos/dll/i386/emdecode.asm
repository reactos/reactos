	subttl	emdecode.asm - Instruction decoding
	page
;***
;emdecode.asm - Instruction decoding
;
;	 Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;
;	 All Rights Reserved
;
;Purpose:
;	Further decoding of instructions done here.
;
;Revision History:
;
;    8/23/91  TP    Rewritten for 32 bits
;
;*******************************************************************************

;On entry, eax = r/m bits * 4.  This is used to jump directly to the
;correct instruction within the group.

GroupFCHS:
	jmp	tGroupFCHSdisp[eax]

GroupFLD1:
	jmp	tGroupFLD1disp[eax]

GroupF2XM1:
	jmp	tGroupF2XM1disp[eax]

GroupFPREM:
	jmp	tGroupFPREMdisp[eax]

GroupFENI:
	jmp	tGroupFENIdisp[eax]


