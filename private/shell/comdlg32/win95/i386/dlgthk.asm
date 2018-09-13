	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Thu Aug 29 16:36:45 1996

;Command Line: ..\..\..\dev\tools\binr\thunk.exe -NC THUNK16B -o dlgthk ..\dlgthk.thk 

	TITLE	$dlgthk.asm

	.386
	OPTION READONLY
	OPTION OLDSTRUCTS

IFNDEF IS_16
IFNDEF IS_32
%out command line error: specify one of -DIS_16 -DIS_32
.err
ENDIF
ENDIF
IFDEF IS_32
IFDEF IS_16
%out command line error: you can't specify both -DIS_16 and -DIS_32
.err
ENDIF
	.model FLAT,STDCALL

externDef STDCALL DlgThunkInit@4:near32
externDef STDCALL ThkPathQualify@4:near32
externDef STDCALL ThkParseFile@8:near32
externDef STDCALL GetFileTitleI@16:near32
externDef STDCALL ThkCommDlgExtendedError@0:near32
externDef STDCALL ThkChooseFont@4:near32
externDef STDCALL ThkChooseColor@4:near32
externDef STDCALL ThkGetSaveFileName@4:near32
externDef STDCALL ThkGetOpenFileName@4:near32

externDef C Common32ThkLS:near32

	.data

pfndlgthkCommon	equ	<pfndlgthkTable>
public pfndlgthkTable
pfndlgthkTable	dword	0		; segmented common entry point
dlgthkConnectionNameLS	db	'dlgthkConnectionDataLS',0

public	dlgthkLSChecksum32
dlgthkLSChecksum32	equ	0b53eh

	.code 

;===========================================================================
; Worker routine to pass additional, internal params to the
; init routine in kernel32.
;
; Calling sequence:
;
; dlgthkConnectPeerLS proto near pszDll16:dword, pszDll32:dword
;
; pszDll16	db	'foo16.dll',0
; pszDll32	db	'foo32.dll',0
;
;	invoke	dlgthkConnectPeerLS, ADDR pszDll16, ADDR pszDll32
;	or	eax,eax
;	jz	failed
;	;success
;
public dlgthkConnectPeerLS@8
dlgthkConnectPeerLS@8:
externDef ThunkInitLS@20:near32
	pop	edx				;Shift return address
	push	dword ptr 0b53eh		;Checksum
	push	offset dlgthkConnectionNameLS	;Exported from peer
	push	offset pfndlgthkCommon		;Output param
	push	edx
	jmp	ThunkInitLS@20

;===========================================================================
; This is the table of 32-bit entry points.
DlgThunkInit@4 label near32
	mov	cl,0			; offset in jump table
	mov	edx, (1 SHL 16) + 0
PreEntryCommon32_1:
	mov	ch,0
	jmp	EntryCommon32
ThkPathQualify@4 label near32
	mov	cl,2			; offset in jump table
	mov	edx, (1 SHL 16) + 0
	jmp	PreEntryCommon32_1
ThkParseFile@8 label near32
	mov	cl,4			; offset in jump table
	mov	edx, (2 SHL 16) + 0
	jmp	PreEntryCommon32_1
GetFileTitleI@16 label near32
	mov	cl,6			; offset in jump table
	mov	edx, (4 SHL 16) + 65535
	jmp	PreEntryCommon32_1
ThkCommDlgExtendedError@0 label near32
	mov	cl,8			; offset in jump table
	mov	edx, (0 SHL 16) + 0
	jmp	PreEntryCommon32_1
ThkChooseFont@4 label near32
	mov	cl,10			; offset in jump table
	mov	edx, (1 SHL 16) + 0
	jmp	PreEntryCommon32_1
ThkChooseColor@4 label near32
	mov	cl,12			; offset in jump table
	mov	edx, (1 SHL 16) + 0
	jmp	PreEntryCommon32_1
ThkGetSaveFileName@4 label near32
	mov	cl,14			; offset in jump table
	mov	edx, (1 SHL 16) + 0
	jmp	PreEntryCommon32_1
ThkGetOpenFileName@4 label near32
	mov	cl,16			; offset in jump table
	mov	edx, (1 SHL 16) + 0
	jmp	PreEntryCommon32_1
;===========================================================================
; This is the common setup code for 32=>16 thunks.
; 
; Entry:  cx = offset in thunk table
;         dx = signed error return value
;         edx.hi = # of parameters (4 bytes per parameter)
; 
; Exit:   eax = 16:16 address of target common entry point
;
align
EntryCommon32:
ifdef FSAVEALL
.err	;32->16 thunks require FSAVEALL, else don't use -r
endif

	mov	eax, pfndlgthkCommon
	jmp	Common32ThkLS
THK_CODE32_SIZE label byte
ELSE	; IS_16
	OPTION SEGMENT:USE16
	.model LARGE,PASCAL

include thk.inc
include winerror.inc
include win31err.inc
include dlgthk.inc

externDef DlgThunkInit:far16
externDef PathQualify:far16
externDef ParseFileFrom32:far16
externDef GetFileTitleI:far16
externDef CommDlgExtendedError:far16
externDef ChooseFont:far16
externDef ChooseColor:far16
externDef GetSaveFileName:far16
externDef GetOpenFileName:far16
externDef UnMapStackAndMakeFlat:far16
externDef LinearToSelectorOffset:far16
externDef MapLS:far16
externDef UnmapLS:far16
externDef StackLinearToSegmented:far16
externDef SelectorOffsetToLinear:far16
externDef MapSL:far16
externDef AllocCallback:far16
externDef GetCSAlias:far16
externDef FreeCSAlias:far16
externDef OutputDebugString:far16
externDef SetLastError:far16
externDef THUNK16BCodeData:word
externDef FlatData:word
externDef dlgthkTable:dword
externDef dlgthkCommon:far16
externDef LocalAlloc:far16
externDef LocalLock:far16
externDef LocalUnlock:far16
externDef LocalFree:far16
MYLOCALLOCK macro x
	push ecx
	push ebx
	push x
	call LocalLock
	pop ebx
	pop ecx
	endm

externDef T_DlgThunkInit:near16
externDef T_ThkPathQualify:near16
externDef T_ThkParseFile:near16
externDef T_GetFileTitleI:near16
externDef T_ThkCommDlgExtendedError:near16
externDef T_ThkChooseFont:near16
externDef T_ThkChooseColor:near16
externDef T_ThkGetSaveFileName:near16
externDef T_ThkGetOpenFileName:near16

DATA16 SEGMENT WORD USE16 PUBLIC 'DATA'
DATA16 ENDS

	.code THUNK16B

public dlgthkConnectionDataLS	; export in def file
dlgthkConnectionDataLS	dd	0b53eh	;Checksum
			dw	offset dlgthkCommon
			dw	seg dlgthkCommon

;===========================================================================
; This is a jump table to API-specific 16-bit thunk code.
; Each entry is a word.

align
dlgthkTable16 label word
	dw	offset T_DlgThunkInit
	dw	offset T_ThkPathQualify
	dw	offset T_ThkParseFile
	dw	offset T_GetFileTitleI
	dw	offset T_ThkCommDlgExtendedError
	dw	offset T_ThkChooseFont
	dw	offset T_ThkChooseColor
	dw	offset T_ThkGetSaveFileName
	dw	offset T_ThkGetOpenFileName

;===========================================================================
; This is the common 16-bit entry point for 32=>16 thunks.  It:
;     1. saves sp in bp
;     2. sets ds
;     3. jumps to API-specific thunk code
;
; Entry:  di    == jump table offset
;         ss:sp == 16-bit stack pointer
align
dlgthkCommon label far16

	mov	bp,sp			; save sp in bp
	mov	ax,seg DATA16		; set ds
	mov	ds,ax
	jmp	word ptr cs:dlgthkTable16[di]  ; select specific thunk

;===========================================================================
; Macro sets extended error code if Win16 API returned an
; error value.
;
; Inputs:
;    errret:   32-bit API return value that signals an error
;    errle:    32-bit extended error code to pass to SetLastError()
;    exitlbl:  The Exit_n label to jump to.
;    eax:      The 32-bit API return value.
;
ERRCHK_EXIT	macro	errret,errle,exitlbl
	cmp	eax,&errret&
	jne	&exitlbl&
	push	eax	;Save return value.
	pushd	&errle&	;ARG: SetLastError(errorcode)
	call	SetLastError
	pop	eax	;Restore return value.
	jmp	&exitlbl&
endm ;ERRCHK_EXIT
;===========================================================================
; Common routines to restore the stack and registers
; and return to 32-bit code.  There is one for each
; size of 32-bit parameter list in this script.

align
Exit_0:
;--- No error checking.
	mov	bl,0		; parameter byte count
	mov	sp,bp		; point to ret addr
	retd			; 16:32 ret to dispatcher
align
Exit_4:
;--- No error checking.
	mov	bl,4		; parameter byte count
	mov	sp,bp		; point to ret addr
	retd			; 16:32 ret to dispatcher
align
Exit_8:
;--- No error checking.
	mov	bl,8		; parameter byte count
	mov	sp,bp		; point to ret addr
	retd			; 16:32 ret to dispatcher
align
Exit_16:
;--- No error checking.
	mov	bl,16		; parameter byte count
	mov	sp,bp		; point to ret addr
	retd			; 16:32 ret to dispatcher
;===========================================================================
T_DlgThunkInit label near16

; bp+40   pCB32Tab

	APILOGLS	DlgThunkInit

;-------------------------------------
; create new call frame and make the call

; pCB32Tab  from: unsigned long
	push	dword ptr [bp+40]	; to unsigned long

	call	DlgThunkInit		; call 16-bit version

; return code void --> void
; no conversion needed

;-------------------------------------
;--- No error checking.
	jmp	Exit_4


;===========================================================================
T_ThkPathQualify label near16

; bp+40   lpszFile

	APILOGLS	ThkPathQualify

;-------------------------------------
; Temp storage

	xor	eax,eax
	push	eax			; ptr param #1   lpszFile
;-------------------------------------
; *** BEGIN parameter packing

; lpszFile
; pointer char --> char
; same pointer types
	mov	eax,[bp+40]		; base address
	push	eax
	call	MapLS
	mov	[bp-4],eax
L0:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpszFile  from: char
	push	dword ptr [bp-4]	; to: char

	call	PathQualify		; call 16-bit version

; return code unsigned long --> unsigned long
	rol	eax,16
	xchg	ax,dx
	rol	eax,16

	push	eax
	push	dword ptr [bp - 4]
	call	UnmapLS
	pop	eax
;-------------------------------------
;--- No error checking.
	jmp	Exit_4


;===========================================================================
T_ThkParseFile label near16

; bp+40   lpszFile
; bp+44   dwFlags

	APILOGLS	ThkParseFile

;-------------------------------------
; Temp storage

	xor	eax,eax
	push	eax			; ptr param #1   lpszFile
;-------------------------------------
; *** BEGIN parameter packing

; lpszFile
; pointer char --> char
; same pointer types
	mov	eax,[bp+40]		; base address
	push	eax
	call	MapLS
	mov	[bp-4],eax
L1:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpszFile  from: char
	push	dword ptr [bp-4]	; to: char

; dwFlags  from: unsigned long
	push	dword ptr [bp+44]	; to unsigned long

	call	ParseFileFrom32		; call 16-bit version

; return code unsigned long --> unsigned long
	rol	eax,16
	xchg	ax,dx
	rol	eax,16

	push	eax
	push	dword ptr [bp - 4]
	call	UnmapLS
	pop	eax
;-------------------------------------
;--- No error checking.
	jmp	Exit_8


;===========================================================================
T_GetFileTitleI label near16

; bp+40   lpszFile
; bp+44   lpszTitle
; bp+48   wBufSize
; bp+52   dwFlags

	APILOGLS	GetFileTitleI

;-------------------------------------
; Temp storage

	xor	eax,eax
	push	eax			; ptr param #1   lpszFile
	push	eax			; ptr param #2   lpszTitle
;-------------------------------------
; *** BEGIN parameter packing

; lpszFile
; pointer char --> char
; same pointer types
	mov	eax,[bp+40]		; base address
	push	eax
	call	MapLS
	mov	[bp-4],eax
L2:

; lpszTitle
; pointer char --> char
; same pointer types
	mov	eax,[bp+44]		; base address
	push	eax
	call	MapLS
	mov	[bp-8],eax
L3:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpszFile  from: char
	push	dword ptr [bp-4]	; to: char

; lpszTitle  from: char
	push	dword ptr [bp-8]	; to: char

; wBufSize  from: unsigned short
	push	word ptr [bp+48]	; to unsigned short

; dwFlags  from: unsigned long
	push	dword ptr [bp+52]	; to unsigned long

	call	GetFileTitleI		; call 16-bit version

; return code short --> long
	cwde
	push	eax
	push	dword ptr [bp - 4]
	call	UnmapLS
	push	dword ptr [bp - 8]
	call	UnmapLS
	pop	eax
;-------------------------------------
;--- No error checking.
	jmp	Exit_16


;===========================================================================
T_ThkCommDlgExtendedError label near16


	APILOGLS	ThkCommDlgExtendedError

;-------------------------------------
; create new call frame and make the call

	call	CommDlgExtendedError		; call 16-bit version

; return code unsigned long --> unsigned long
	rol	eax,16
	xchg	ax,dx
	rol	eax,16

;-------------------------------------
;--- No error checking.
	jmp	Exit_0


;===========================================================================
T_ThkChooseFont label near16

; bp+40   lpcf

	APILOGLS	ThkChooseFont

;-------------------------------------
; Temp storage

	xor	eax,eax
	push	eax	;Storage for lpszStyleTemp
LOCAL__THKCHOOSEFONT_lpszStyleTemp	equ	<[bp-4]>
	push	eax	;Storage for lpTemplateNameTemp
LOCAL__THKCHOOSEFONT_lpTemplateNameTemp	equ	<[bp-8]>
	push	eax	;Storage for Flags
LOCAL__THKCHOOSEFONT_Flags	equ	<[bp-12]>
	push	eax			; ptr param #1   lpcf
;-------------------------------------
; *** BEGIN parameter packing

; lpcf
; pointer struct --> struct
	RAWPACK__THKCHOOSEFONT_lpcf 40, 16
L4:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpcf  from: struct
	push	dword ptr [bp-16]	; to: struct

	call	ChooseFont		; call 16-bit version

; return code short --> long
	cwde
;-------------------------------------
; *** BEGIN parameter unpacking

	push	eax			; save return code

; lpcf
	RAWUNPACK__THKCHOOSEFONT_lpcf 40, 16
L5:
	pop	eax			; restore return code

; *** END   parameter unpacking
;-------------------------------------
;--- No error checking.
	jmp	Exit_4


;===========================================================================
T_ThkChooseColor label near16

; bp+40   lpcc

	APILOGLS	ThkChooseColor

;-------------------------------------
; Temp storage

	xor	eax,eax
	push	eax	;Storage for lpTemplateNameSeg
LOCAL__THKCHOOSECOLOR_lpTemplateNameSeg	equ	<[bp-4]>
	push	eax	;Storage for lpCustColorsSeg
LOCAL__THKCHOOSECOLOR_lpCustColorsSeg	equ	<[bp-8]>
	push	eax	;Storage for Flags
LOCAL__THKCHOOSECOLOR_Flags	equ	<[bp-12]>
	push	eax			; ptr param #1   lpcc
;-------------------------------------
; *** BEGIN parameter packing

; lpcc
; pointer struct --> struct
	RAWPACK__THKCHOOSECOLOR_lpcc 40, 16
L6:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpcc  from: struct
	push	dword ptr [bp-16]	; to: struct

	call	ChooseColor		; call 16-bit version

; return code short --> long
	cwde
;-------------------------------------
; *** BEGIN parameter unpacking

	push	eax			; save return code

; lpcc
	RAWUNPACK__THKCHOOSECOLOR_lpcc 40, 16
L7:
	pop	eax			; restore return code

; *** END   parameter unpacking
;-------------------------------------
;--- No error checking.
	jmp	Exit_4


;===========================================================================
T_ThkGetSaveFileName label near16

; bp+40   lpOfn

	APILOGLS	ThkGetSaveFileName

;-------------------------------------
; Temp storage

	xor	eax,eax
	push	eax	;Storage for Flags
LOCAL__THKGETSAVEFILENAME_Flags	equ	<[bp-4]>
	push	eax			; ptr param #1   lpOfn
;-------------------------------------
; *** BEGIN parameter packing

; lpOfn
; pointer struct --> struct
	RAWPACK__THKGETSAVEFILENAME_lpOfn 40, 8
L8:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpOfn  from: struct
	push	dword ptr [bp-8]	; to: struct

	call	GetSaveFileName		; call 16-bit version

; return code short --> long
	cwde
;-------------------------------------
; *** BEGIN parameter unpacking

	push	eax			; save return code

; lpOfn
	RAWUNPACK__THKGETSAVEFILENAME_lpOfn 40, 8
L9:
	pop	eax			; restore return code

; *** END   parameter unpacking
;-------------------------------------
;--- No error checking.
	jmp	Exit_4


;===========================================================================
T_ThkGetOpenFileName label near16

; bp+40   lpOfn

	APILOGLS	ThkGetOpenFileName

;-------------------------------------
; Temp storage

	xor	eax,eax
	push	eax	;Storage for Flags
LOCAL__THKGETOPENFILENAME_Flags	equ	<[bp-4]>
	push	eax			; ptr param #1   lpOfn
;-------------------------------------
; *** BEGIN parameter packing

; lpOfn
; pointer struct --> struct
	RAWPACK__THKGETOPENFILENAME_lpOfn 40, 8
L10:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; lpOfn  from: struct
	push	dword ptr [bp-8]	; to: struct

	call	GetOpenFileName		; call 16-bit version

; return code short --> long
	cwde
;-------------------------------------
; *** BEGIN parameter unpacking

	push	eax			; save return code

; lpOfn
	RAWUNPACK__THKGETOPENFILENAME_lpOfn 40, 8
L11:
	pop	eax			; restore return code

; *** END   parameter unpacking
;-------------------------------------
;--- No error checking.
	jmp	Exit_4


THK_CODE16_SIZE label byte
ENDIF
END
