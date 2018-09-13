; ========================================================
;
; MODEL.INC
;
; Copyright (c) 1991 - Microsoft Corp.
; All rights reserved.
; Microsoft Confidential
;
;
; Include file which allows globally controlling the memory
; model of a library of assembly modules. Also contains
; macros for writing MASM 6.0 code which will compile
; under DOS 5.1.
;
;
; johnhe - 01-24-92
;
; ========================================================================
; Default to small memory model and C calling conventions if none
; were declared.
;
; ========================================================================


	; Convert old build switches to new type

IFDEF		MODEL_LARGE
  MEM_MODEL EQU <LARGE>
ELSEIFDEF	MODEL_MEDIUM
  MEM_MODEL EQU <MEDIUM> 
ELSEIFDEF	MODEL_COMPACT
  MEM_MODEL EQU <COMPACT>
ELSEIFDEF	MODEL_SMALL
  MEM_MODEL EQU <SMALL>
ENDIF


IFNDEF MEM_MODEL
  MEM_MODEL EQU <SMALL>
ENDIF

IFNDEF	CALL_TYPE
	CALL_TYPE EQU <C>
ENDIF

; ========================================================================
; The following defines the memory model based on one of four constants
; being defined. If none of the constants have been defined the model
; will default to MODEL_SMALL
; ========================================================================

DOSSEG

.MODEL	MEM_MODEL,CALL_TYPE


; ========================================================================
; This macro allows writing code which will assemble with MASM 5.1 or
; 6.0. MASM 6.0 requires an assume statement before using a register
; as the base pointer of a structure.
;
; RegPtr Register:STRUC, Register:STRUC, ...
;
; EXAMPLE:
;
;	RegPtr	DI:MyStruc
;
; ========================================================================

RegPtr MACRO  arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9

  IFB <arg1>
    %out <Assume Macro --> Requires at least 1 argument.>
  ENDIF

  IF @Version GE 600
    IRP X, <arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9>
      IFNB <x>
        ASSUME  x
      ENDIF
    ENDM
  ENDIF

ENDM

; ========================================================================
; Macro which exsplicitly calls a far or near procedure based on the
; current memory model.
;
; call_M PROCEDURE_NAME
;
; ========================================================================

call_M	MACRO	Function

IF 	@codesize			; Call adjusted for memory model

	call	FAR PTR Function	; FAR call if LARGE, MED, or HUGE
ELSE
	call	NEAR PTR Function	; Else NEAR call
ENDIF
ENDM

; ========================================================================
; DS_IF_DATASIZE
;
; Equate to allow DS to be pushed on entry to a function if ptrs are
; far. This macro should be used in conjunction with LoadPtr DS,x,x
; to ensure DS will be saved if its destroyed by a far ptr load.
;
; EXAMPLE:
;
; Function PROC USES DI SI ES FPTR_DS, Buffer:PTR, EndBuf:PTR
;
;	LoadPtr	ES,DI,EndBuf
;	LoadPTr	DS,SI,BUffer
;	...
; Function ENDP
;
; Function PROC USES_FPTR_DS, Buffer:PTR, EndBuf:PTR
;
;	LoadPtr	ES,DI,EndBuf
;	LoadPTr	DS,SI,BUffer
;	...
; Function ENDP
;
; ========================================================================

IF @DATASIZE
	FPTR_DS		EQU	<DS>
	USES_FPTR_DS	EQU	<USES DS>
	DS_IF_DATASIZE	EQU	<DS>
ELSE
	FPTR_DS EQU 		<>
	USES_FPTR_DS	EQU	<>
	DS_IF_DATASIZE	EQU	<>
ENDIF

; ========================================================================
; LoadPtr
;
;	Macro to load a pointer based on the memory model that is in use.
;	Argument checking is included in the macro and the appropriate
;	error message will be displayed.
;	
; LoadPtr MACRO vSegReg, vReg, vPtr
;
;	vSegReg		- Segment register (normally DS or ES but may be any)
;	vReg		- Any general purpose register (AX,BX,CX,DX,SI,DI)
;	vPtr		- Pointer to a memory location.
;			  The value will be treated as a dword ptr in
;			  LARGE/COMPACT model and word ptr in MEDIUM/SMALL.
;
; ========================================================================


LoadPtr MACRO vSegReg, vReg, vPtr

  SEG_OK = 0			;; Variables to specify different errors
  REG_OK = 0
  PTR_OK = 0

				;; Error checking on segment register
  IFNB <vSegReg>
    IRP x, <ES,DS,AX,BX,CX,DX,SI,DI>
      IFIDNI <x>,<vSegReg>
        SEG_OK = 1
      ENDIF
    ENDM 
  ENDIF

				;; Error checking on index register
  IFNB <vReg>
    IRP y, <AX,BX,CX,DX,SI,DI>
      IFIDNI <y>,<vReg>
        REG_OK = 1
      ENDIF
    ENDM 
  ENDIF

				;; Error checking on memory pointer
  IFNB <vPtr>
    PTR_OK = 1
  ENDIF

				;; If errs found display appropriate message
  IF (SEG_OK + REG_OK + PTR_OK) NE 3

    IF SEG_OK EQ 0
      %out <LoadPtr Macro --> Invalid segment register vSegReg was specified.>
    ENDIF

    IF REG_OK EQ 0
      %out <LoadPtr Macro --> Invalid register vReg was specified.>
    ENDIF

    IF PTR_OK EQ 0
      %out <LoadPtr Macro --> No memory pointer was specified.>
    ENDIF

    .ERR

  ELSE			;; Start of code in the macro since no errs found

    IF @DataSize	;; For LARGE & COMPACT models use LDS or LES  opcodes
			
      IFIDNI <vSegReg>,<DS>
        lds	vReg, vPtr
      ELSEIFIDNI <vSegReg>,<ES>
        les vReg, vPtr
      ELSE
        mov vSegReg, WORD PTR vPtr[2]
        mov vReg, WORD PTR vPtr
      ENDIF

    ELSE
			;; If vSegReg == ES we have to set ES to DATA SEG
      IFIDNI <vSegReg>,<ES>
        push	DS
        pop	ES
      ENDIF
			;; Load pointer into specified register
      mov  vReg, vPtr	

    ENDIF

  ENDIF

ENDM


