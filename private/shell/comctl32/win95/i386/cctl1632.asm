	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Tue Jul 09 16:59:49 1996

;Command Line: ..\..\..\dev\tools\binr\thunk.exe -NC _TEXT -o Cctl1632 ..\Cctl1632.thk 

	TITLE	$Cctl1632.asm

	.386
	OPTION READONLY
	OPTION OLDSTRUCTS

IFNDEF IS_16
IFNDEF IS_32
%out command line error: specify one of -DIS_16, -DIS_32
.err
ENDIF  ;IS_32
ENDIF  ;IS_16


IFDEF IS_32
IFDEF IS_16
%out command line error: you can't specify both -DIS_16 and -DIS_32
.err
ENDIF ;IS_16
;************************* START OF 32-BIT CODE *************************


	.model FLAT,STDCALL


;-- Import common flat thunk routines (in k32)

externDef AllocMappedBuffer	:near32
externDef FreeMappedBuffer		:near32
externDef MapHInstLS	:near32
externDef MapHInstLS_PN	:near32
externDef MapHInstSL	:near32
externDef MapHInstSL_PN	:near32
externDef FT_Prolog	:near32
externDef FT_Thunk	:near32
externDef QT_Thunk	:near32
externDef FT_Exit0	:near32
externDef FT_Exit4	:near32
externDef FT_Exit8	:near32
externDef FT_Exit12	:near32
externDef FT_Exit16	:near32
externDef FT_Exit20	:near32
externDef FT_Exit24	:near32
externDef FT_Exit28	:near32
externDef FT_Exit32	:near32
externDef FT_Exit36	:near32
externDef FT_Exit40	:near32
externDef FT_Exit44	:near32
externDef FT_Exit48	:near32
externDef FT_Exit52	:near32
externDef FT_Exit56	:near32
externDef SMapLS	:near32
externDef SUnMapLS	:near32
externDef SMapLS_IP_EBP_8	:near32
externDef SUnMapLS_IP_EBP_8	:near32
externDef SMapLS_IP_EBP_12	:near32
externDef SUnMapLS_IP_EBP_12	:near32
externDef SMapLS_IP_EBP_16	:near32
externDef SUnMapLS_IP_EBP_16	:near32
externDef SMapLS_IP_EBP_20	:near32
externDef SUnMapLS_IP_EBP_20	:near32
externDef SMapLS_IP_EBP_24	:near32
externDef SUnMapLS_IP_EBP_24	:near32
externDef SMapLS_IP_EBP_28	:near32
externDef SUnMapLS_IP_EBP_28	:near32
externDef SMapLS_IP_EBP_32	:near32
externDef SUnMapLS_IP_EBP_32	:near32
externDef SMapLS_IP_EBP_36	:near32
externDef SUnMapLS_IP_EBP_36	:near32
externDef SMapLS_IP_EBP_40	:near32
externDef SUnMapLS_IP_EBP_40	:near32

MapLS	PROTO NEAR STDCALL :DWORD
UnMapLS	PROTO NEAR STDCALL :DWORD
MapSL	PROTO NEAR STDCALL p32:DWORD

;***************** START OF KERNEL32-ONLY SECTION ******************
; Hacks for kernel32 initialization.

IFDEF FT_DEFINEFTCOMMONROUTINES

	.data
public FT_Cctl1632TargetTable	;Flat address of target table in 16-bit module.

public FT_Cctl1632Checksum32
FT_Cctl1632Checksum32	dd	01e9ah


ENDIF ;FT_DEFINEFTCOMMONROUTINES
;***************** END OF KERNEL32-ONLY SECTION ******************



	.code 

;************************* COMMON PER-MODULE ROUTINES *************************

	.data

; The next two symbols must be exported so BBT knows not to optimize
; them.  (We tag them as KeepTogetherRange in the BBT config file.)

public Cctl1632_ThunkData32	;This symbol must be exported.
public FT_Prolog_Cctl1632

Cctl1632_ThunkData32 label dword
	dd	3130534ch	;Protocol 'LS01'
	dd	01e9ah	;Checksum
	dd	0	;Jump table address.
	dd	3130424ch	;'LB01'
	dd	0	;Flags
	dd	0	;Reserved (MUST BE 0)
	dd	0	;Reserved (MUST BE 0)
	dd	offset QT_Thunk_Cctl1632 - offset Cctl1632_ThunkData32
	dd	offset FT_Prolog_Cctl1632 - offset Cctl1632_ThunkData32



	.code 


externDef ThunkConnect32@24:near32

public Cctl1632_ThunkConnect32@16
Cctl1632_ThunkConnect32@16:
	pop	edx
	push	offset Cctl1632_ThkData16
	push	offset Cctl1632_ThunkData32
	push	edx
	jmp	ThunkConnect32@24
Cctl1632_ThkData16 label byte
	db	"Cctl1632_ThunkData16",0


		


pfnQT_Thunk_Cctl1632	dd offset QT_Thunk_Cctl1632
pfnFT_Prolog_Cctl1632	dd offset FT_Prolog_Cctl1632
	.data
QT_Thunk_Cctl1632 label byte
	db	32 dup(0cch)	;Patch space.

FT_Prolog_Cctl1632 label byte
	db	32 dup(0cch)	;Patch space.


	.code 




ebp_top		equ	<[ebp + 8]>	;First api parameter
ebp_retval	equ	<[ebp + -64]>	;Api return value
FT_ESPFIXUP	macro	dwSpOffset
	or	dword ptr [ebp + -20], 1 SHL ((dwSpOffset) SHR 1)
endm


ebp_qttop	equ	<[ebp + 8]>


include fltthk.inc	;Support definitions
include Cctl1632.inc



;************************ START OF THUNK BODIES************************




;
public DestroyPropertySheetPage16@4
DestroyPropertySheetPage16@4:
	FAPILOG16	35
	mov	cl,2
; DestroyPropertySheetPage(16) = DestroyPropertySheetPage16(32) {}
;
; dword ptr [ebp+8]:  hpage
;
public IIDestroyPropertySheetPage16@4
IIDestroyPropertySheetPage16@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;hpage: dword->dword
	call	dword ptr [pfnQT_Thunk_Cctl1632]
	cwde
	leave
	retn	4





;
public CreatePage16@8
CreatePage16@8:
	FAPILOG16	18
	mov	cl,1
; CreatePage(16) = CreatePage16(32) {}
;
; dword ptr [ebp+8]:  hpage
; dword ptr [ebp+12]:  hwndParent
;
public IICreatePage16@8
IICreatePage16@8:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	dword ptr [ebp+8]	;hpage: dword->dword
	push	word ptr [ebp+12]	;hwndParent: dword->word
	call	dword ptr [pfnQT_Thunk_Cctl1632]
	movzx	eax,ax
	leave
	retn	8






;
public GetPageInfoME@24
GetPageInfoME@24:
	FAPILOG16	0
	mov	cx, (6 SHL 10) + (0 SHL 8) + 0
; GetPageInfo(16) = GetPageInfoME(32) {}
;
; dword ptr [ebp+8]:  hpage
; dword ptr [ebp+12]:  pszCaption
; dword ptr [ebp+16]:  cbCaption
; dword ptr [ebp+20]:  ppt
; dword ptr [ebp+24]:  phIcon
; dword ptr [ebp+28]:  pb
;
public IIGetPageInfoME@24
IIGetPageInfoME@24:
	call	dword ptr [pfnFT_Prolog_Cctl1632]
	xor	eax,eax
	push	eax
	push	eax
	sub	esp,4
	mov	esi,[ebp+20]
	or	esi,esi
	jz	@F
	or	byte ptr [esi], 0
	or	byte ptr [esi + 7], 0
@@:
	mov	edx, dword ptr [ebp+24]
	or	edx,edx
	jz	@F
	or	dword ptr [edx], 0
@@:
	mov	edx, dword ptr [ebp+28]
	or	edx,edx
	jz	@F
	or	dword ptr [edx], 0
@@:
	push	dword ptr [ebp+8]	;hpage: dword->dword
	call	SMapLS_IP_EBP_12
	push	eax
	push	word ptr [ebp+16]	;cbCaption: dword->word
	mov	esi,[ebp+20]
	or	esi,esi
	jnz	M0
	push	esi
	jmp	M1
M0:
	lea	edi,[ebp-76]
	push	edi	;ppt: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],010h	;Set flag to fixup ESP-rel argument.
M1:
	mov	eax, dword ptr [ebp+24]
	call	SMapLS
	mov	[ebp-68],edx
	push	eax
	mov	eax, dword ptr [ebp+28]
	call	SMapLS
	mov	[ebp-72],edx
	push	eax
	call	FT_Thunk
	movsx	ebx,ax
	call	SUnMapLS_IP_EBP_12
	mov	edi,[ebp+20]
	or	edi,edi
	jz	M2
	lea	esi,[ebp-76]	;ppt  Struct16->Struct32
	lodsw
	cwde
	stosd
	lodsw
	cwde
	stosd
M2:
	mov	edx, dword ptr [ebp+24]
	or	edx,edx
	jz	M3
	mov	word ptr [edx+2], 0
M3:
	mov	ecx, dword ptr [ebp-68]
	call	SUnMapLS
	mov	edx, dword ptr [ebp+28]
	or	edx,edx
	jz	M4
	mov	word ptr [edx+2], 0
M4:
	mov	ecx, dword ptr [ebp-72]
	call	SUnMapLS
	jmp	FT_Exit24




;
public GetPageInfo16@20
GetPageInfo16@20:
	FAPILOG16	0
	mov	cx, (5 SHL 10) + (0 SHL 8) + 0
; GetPageInfo(16) = GetPageInfo16(32) {}
;
; dword ptr [ebp+8]:  hpage
; dword ptr [ebp+12]:  pszCaption
; dword ptr [ebp+16]:  cbCaption
; dword ptr [ebp+20]:  ppt
; dword ptr [ebp+24]:  phIcon
;
public IIGetPageInfo16@20
IIGetPageInfo16@20:
	call	dword ptr [pfnFT_Prolog_Cctl1632]
	xor	eax,eax
	push	eax
	sub	esp,4
	mov	esi,[ebp+20]
	or	esi,esi
	jz	@F
	or	byte ptr [esi], 0
	or	byte ptr [esi + 7], 0
@@:
	mov	edx, dword ptr [ebp+24]
	or	edx,edx
	jz	@F
	or	dword ptr [edx], 0
@@:
	push	dword ptr [ebp+8]	;hpage: dword->dword
	call	SMapLS_IP_EBP_12
	push	eax
	push	word ptr [ebp+16]	;cbCaption: dword->word
	mov	esi,[ebp+20]
	or	esi,esi
	jnz	L0
	push	esi
	jmp	L1
L0:
	lea	edi,[ebp-72]
	push	edi	;ppt: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],04h	;Set flag to fixup ESP-rel argument.
L1:
	mov	eax, dword ptr [ebp+24]
	call	SMapLS
	mov	[ebp-68],edx
	push	eax
	call	FT_Thunk
	movsx	ebx,ax
	call	SUnMapLS_IP_EBP_12
	mov	edi,[ebp+20]
	or	edi,edi
	jz	L2
	lea	esi,[ebp-72]	;ppt  Struct16->Struct32
	lodsw
	cwde
	stosd
	lodsw
	cwde
	stosd
L2:
	mov	edx, dword ptr [ebp+24]
	or	edx,edx
	jz	L3
	mov	word ptr [edx+2], 0
L3:
	mov	ecx, dword ptr [ebp-68]
	call	SUnMapLS
	jmp	FT_Exit20




;-----------------------------------------------------------
ifdef DEBUG
FT_ThunkLogNames label byte
	db	'[F] GetPageInfo16',0
	db	'[F] CreatePage16',0
	db	'[F] DestroyPropertySheetPage16',0
endif ;DEBUG
;-----------------------------------------------------------



ELSE
;************************* START OF 16-BIT CODE *************************




	OPTION SEGMENT:USE16
	.model LARGE,PASCAL


	.code	_TEXT



externDef GetPageInfo:far16
externDef CreatePage:far16
externDef DestroyPropertySheetPage:far16


FT_Cctl1632TargetTable label word
	dw	offset GetPageInfo
	dw	   seg GetPageInfo
	dw	offset CreatePage
	dw	   seg CreatePage
	dw	offset DestroyPropertySheetPage
	dw	   seg DestroyPropertySheetPage




	.data

public Cctl1632_ThunkData16	;This symbol must be exported.
Cctl1632_ThunkData16	dd	3130534ch	;Protocol 'LS01'
	dd	01e9ah	;Checksum
	dw	offset FT_Cctl1632TargetTable
	dw	seg    FT_Cctl1632TargetTable
	dd	0	;First-time flag.



	.code _TEXT


externDef ThunkConnect16:far16

public Cctl1632_ThunkConnect16
Cctl1632_ThunkConnect16:
	pop	ax
	pop	dx
	push	seg    Cctl1632_ThunkData16
	push	offset Cctl1632_ThunkData16
	push	seg    Cctl1632_ThkData32
	push	offset Cctl1632_ThkData32
	push	cs
	push	dx
	push	ax
	jmp	ThunkConnect16
Cctl1632_ThkData32 label byte
	db	"Cctl1632_ThunkData32",0





ENDIF
END
