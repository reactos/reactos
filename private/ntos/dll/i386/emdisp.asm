	subttl	emdisp.asm - Emulator Dispatch Tables
	page
;
;	 IBM/Microsoft Confidential
;
;	 Copyright (c) IBM Corporation 1987, 1989
;	 Copyright (c) Microsoft Corporation 1987, 1989
;
;	 All Rights Reserved
;
;Revision History:  (also see emulator.hst)
;
;    1/21/92  JWM   Minor modifications for DOSX32 emulator
;    8/23/91  TP    Direct dispatch off of 6-bit opcode
;   10/30/89  WAJ   Added this header.
;
;*******************************************************************************

;*********************************************************************;
;								      ;
;		Dispatch Tables 				      ;
;								      ;
;*********************************************************************;


;   These tables are based upon the layout of the 8087 instructions
;
;      8087 instruction fields:   |escape|MF|Arith|MOD|Op|r/m|disp1|disp2|
;	  field length in bits:       5    2   1    2	3   3	8     8
;
;   Disp1 and Disp2  are optional address bytes present only if MOD <> 11.
;   When (MOD <> 11) r/m describes which regs (SI,DI,BX,BP) are added to
;	Disp1 and Disp2 to calculate the effective address. This form
;	(memory format) is used for Loads, Stores, Compares, and Arithmetic
;   When using memory format MF determines the Type of the Memory operand
;	i.e. Single Real, Double real, Single Integer, or Double Integer
;   Arith is 0 for Arithmetic opetations (and compares), set to 1 otherwise
;   Op mostly determines which type of operation to do though when not in
;	memory format some of that is coded into MF and r/m
;   All of the tables are set up to do a jump based upon one or more of the
;	above fields. The outline for decoding instructions is:
;
;	    IF (memory format) THEN
;	       Assemble Effective Address (using MOD and r/m and EffectiveAddressTab)
;	       Jump through table to operation, using MF, Arith and Op bits
;	    ELSE (Register format)
;	       Jump through table to operation, using MF, Arith and Op bits

	ALIGN	4

;*********************************************************************;
;
; Memory address calculation tables

EA386Tab	label	dword			; Uses |r/m|MOD+1| for indexing
	dd	NoEffectiveAddress
	dd	Exx00			; eax
	dd	Exx01
	dd	Exx10
	dd	NoEffectiveAddress
	dd	Exx00			; ecx
	dd	Exx01
	dd	Exx10
	dd	NoEffectiveAddress
	dd	Exx00			; edx
	dd	Exx01
	dd	Exx10
	dd	NoEffectiveAddress
	dd	Exx00			; ebx
	dd	Exx01
	dd	Exx10
	dd	NoEffectiveAddress
	dd	SIB00			; esp (S-I-B follows)
	dd	SIB01
	dd	SIB10
	dd	NoEffectiveAddress
	dd	Direct386		; ebp (00 = direct addressing)
	dd	Exx01
	dd	Exx10
	dd	NoEffectiveAddress
	dd	Exx00			; esi
	dd	Exx01
	dd	Exx10
	dd	NoEffectiveAddress
	dd	Exx00			; edi
	dd	Exx01
	dd	Exx10

;*********************************************************************;
;
;Opcode dispatching tables
;Indexed by  | op1 | op2 |0 0|  (op1 = MF|Arith)

	public	tOpRegDisp
tOpRegDisp	label	dword
	dd	eFADDtop
	dd	eFMULtop
	dd	eFCOM
	dd	eFCOMP
	dd	eFSUBtop
	dd	eFSUBRtop
	dd	eFDIVtop
	dd	eFDIVRtop

	dd	eFLDreg
	dd	eFXCH
	dd	eFNOP		;UNDONE: also reserved on 387
	dd	eFSTP		;Special form 1
	dd	GroupFCHS	;FCHS,FABS,FTST,FXAM
	dd	GroupFLD1	;FLD1,FLDL2T,FLDL2E,FLDPI,FLDLG2,FLDLN2,FLDZ
	dd	GroupF2XM1	;F2XM1,FYL2X,FPTAN,FPATAN,FXTRACT,FPREM1,FDECSTP,FINCSTP
	dd	GroupFPREM	;FPREM,FYL2XP1,FSQRT,FSINCOS,FRNDINT,FSCALE,FSIN,FCOS

	dd	UNUSED
	dd	UNUSED
	dd	UNUSED
	dd	UNUSED
	dd	UNUSED
	dd	eFUCOMPP	;UNDONE: also reserved on 387
	dd	UNUSED
	dd	UNUSED

	dd	UNUSED
	dd	UNUSED
	dd	UNUSED
	dd	UNUSED
	dd	GroupFENI	;FENI,FDISI,FCLEX,FINIT
	dd	UNUSED
	dd	UNUSED
	dd	UNUSED

	dd	eFADDreg
	dd	eFMULreg
	dd	eFCOM		;Special form  2
	dd	eFCOMP		;Special form  3
	dd	eFSUBRreg
	dd	eFSUBreg
	dd	eFDIVRreg
	dd	eFDIVreg

	dd	eFFREE
	dd	eFXCH		;Special form 4
	dd	eFST
	dd	eFSTP
	dd	eFUCOM
	dd	eFUCOMP
	dd	UNUSED
	dd	UNUSED

	dd	eFADDPreg
	dd	eFMULPreg
	dd	eFCOMP		;Special form 5
	dd	eFCOMPP		;UNDONE: also reserved on 387
	dd	eFSUBRPreg
	dd	eFSUBPreg
	dd	eFDIVRPreg
	dd	eFDIVPreg

	dd	eFFREE		;Special form 6 UNDONE: "and pop stack"?
	dd	eFXCH		;Special form 7
	dd	eFSTP		;Special form 8
	dd	eFSTP		;Special form 9
	dd	eFSTSWax	;UNDONE: also reserved on 387
	dd	UNUSED
	dd	UNUSED
	dd	UNUSED


tOpMemDisp	label	dword
;MF = 00 (32-bit Real), Arith = 0
	dd	eFADD32
	dd	eFMUL32
	dd	eFCOM32
	dd	eFCOMP32
	dd	eFSUB32
	dd	eFSUBR32
	dd	eFDIV32
	dd	eFDIVR32
;MF = 00 (32-bit Real), Arith = 1
	dd	eFLD32
	dd	UNUSED
	dd	eFST32
	dd	eFSTP32
	dd	eFLDENV
	dd	eFLDCW
	dd	eFSTENV
	dd	eFSTCW
;MF = 01 (32-bit Int), Arith = 0
	dd	eFIADD32
	dd	eFIMUL32
	dd	eFICOM32
	dd	eFICOMP32
	dd	eFISUB32
	dd	eFISUBR32
	dd	eFIDIV32
	dd	eFIDIVR32
;MF = 01 (32-bit Int), Arith = 1
	dd	eFILD32
	dd	UNUSED
	dd	eFIST32
	dd	eFISTP32
	dd	UNUSED
	dd	eFLD80
	dd	UNUSED
	dd	eFSTP80
;MF = 10 (64-bit Real), Arith = 0
	dd	eFADD64
	dd	eFMUL64
	dd	eFCOM64
	dd	eFCOMP64
	dd	eFSUB64
	dd	eFSUBR64
	dd	eFDIV64
	dd	eFDIVR64
;MF = 10 (64-bit Real), Arith = 1
	dd	eFLD64
	dd	UNUSED
	dd	eFST64
	dd	eFSTP64
	dd	eFRSTOR
	dd	UNUSED
	dd	eFSAVE
	dd	eFSTSW
;MF = 11 (16-bit Int), Arith = 0
	dd	eFIADD16
	dd	eFIMUL16
	dd	eFICOM16
	dd	eFICOMP16
	dd	eFISUB16
	dd	eFISUBR16
	dd	eFIDIV16
	dd	eFIDIVR16
;MF = 11 (16-bit Int), Arith = 1
	dd	eFILD16
	dd	UNUSED
	dd	eFIST16
	dd	eFISTP16
	dd	eFBLD
	dd	eFILD64
	dd	eFBSTP
	dd	eFISTP64


tGroupFLD1disp	label	dword
	dd	eFLD1
	dd	eFLDL2T
	dd	eFLDL2E
	dd	eFLDPI
	dd	eFLDLG2
	dd	eFLDLN2
	dd	eFLDZ
	dd	UNUSED


tGroupF2XM1disp	label	dword
	dd	eF2XM1
	dd	eFYL2X
	dd	eFPTAN
	dd	eFPATAN
	dd	eFXTRACT
	dd	eFPREM1
	dd	eFDECSTP
	dd	eFINCSTP


tGroupFCHSdisp	label	dword
	dd	eFCHS
	dd	eFABS
	dd	UNUSED
	dd	UNUSED
	dd	eFTST
	dd	eFXAM
	dd	UNUSED
	dd	UNUSED


tGroupFPREMdisp	label	dword
	dd	eFPREM
	dd	eFYL2XP1
	dd	eFSQRT
	dd	eFSINCOS
	dd	eFRNDINT
	dd	eFSCALE
	dd	eFSIN
	dd	eFCOS


tGroupFENIdisp	label	dword
	dd	eFENI
	dd	eFDISI
	dd	eFCLEX
	dd	eFINIT
	dd	eFSETPM
	dd	UNUSED
	dd	UNUSED
	dd	UNUSED


