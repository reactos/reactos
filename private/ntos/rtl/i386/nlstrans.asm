	title  "NLS Translation"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    nlstrans.asm
;
; Abstract:
;
;    This module implements the function to translate from Unicode
;    characters to ANSI and OEM characters.  The translation is based on
;    the installed ACP and OEMCP.
;
; Author:
;
;    Gregory Wilson  15 may 92
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
	.list

_DATA	SEGMENT DWORD PUBLIC 'DATA'

        extrn   _NlsUnicodeToAnsiData:DWORD
        extrn   _NlsUnicodeToMbAnsiData:DWORD
        extrn   _NlsMbCodePageTag:BYTE
        extrn   _NlsUnicodeToOemData:DWORD
        extrn   _NlsUnicodeToMbOemData:DWORD
        extrn   _NlsMbOemCodePageTag:BYTE

_DATA	ENDS

_TEXT	SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING

_MultiByteString$           equ   8
_MaxBytesInMultiByteString$ equ  12
_BytesInMultiByteString$    equ  16
_UnicodeString$             equ  20
_BytesInUnicodeString$      equ  24
_LoopCount$                 equ  -4
_MultiByteStringAnchor$     equ  -8
_CharsInUnicodeString$      equ -12

        align 4

        public  _RtlUnicodeToMultiByteN

; NTSTATUS
; RtlUnicodeToMultiByteN(
;     OUT PCH MultiByteString,
;     IN ULONG MaxBytesInMultiByteString,
;     OUT PULONG BytesInMultiByteString OPTIONAL,
;     IN PWCH UnicodeString,
;     IN ULONG BytesInUnicodeString)
;
; /*++
;
; Routine Description:
;
;     This functions converts the specified unicode source string into an
;     ansi string. The translation is done with respect to the
;     ANSI Code Page (ACP) loaded at boot time.
;
; Arguments:
;
;     MultiByteString - Returns an ansi string that is equivalent to the
;         unicode source string.  If the translation can not be done
;         because a character in the unicode string does not map to an
;         ansi character in the ACP, an error is returned.
;
;     MaxBytesInMultiByteString - Supplies the maximum number of bytes to be
;         written to MultiByteString.  If this causes MultiByteString to be a
;         truncated equivalent of UnicodeString, no error condition results.
;
;     BytesInMultiByteString - Returns the number of bytes in the returned
;         ansi string pointed to by MultiByteString.
;
;     UnicodeString - Supplies the unicode source string that is to be
;         converted to ansi.
;
;     BytesInUnicodeString - The number of bytes in the the string pointed to by
;         UnicodeString.
;
; Return Value:
;
;     SUCCESS - The conversion was successful
;
; --*/
;
_RtlUnicodeToMultiByteN proc
	push	ebp
	mov	ebp, esp
	sub	esp, 12
        push    ebx
        ;
        ; Save beginning position of MultiByteString ptr for later
        ; use in calculating number of characters translated.
        ;
	mov	eax, DWORD PTR _MultiByteString$[ebp]
	mov	DWORD PTR _MultiByteStringAnchor$[ebp], eax
        ;
        ; Convert BytesInUnicodeString to a character count and
        ; compare against the maximum number of characters we have
        ; room to translate.  Use the minimum for the loop count.
        ;
	mov	eax, DWORD PTR _BytesInUnicodeString$[ebp]
	shr	eax, 1
	mov	ecx, DWORD PTR _MaxBytesInMultiByteString$[ebp]
	sub	eax, ecx
	sbb	edx, edx
	and	eax, edx
	add	eax, ecx
	mov	DWORD PTR _LoopCount$[ebp], eax

        ;
        ; Set up registers such that:
        ;    ebx: UnicodeString
        ;    ecx: NlsUnicodeToAnsiData
        ;    edx: MultiByteString
        ;
	mov	edx, DWORD PTR _MultiByteString$[ebp]
	mov	ebx, DWORD PTR _UnicodeString$[ebp]
	mov	ecx, DWORD PTR _NlsUnicodeToAnsiData
        ;
        ; Determine if we're dealing with SBCS or MBCS.
        ;
	cmp	BYTE PTR _NlsMbCodePageTag, 0    ; 0 -> sbcs, 1 -> mbcs
	jne	$ACP_MBCS
        ;
        ; If the string to be translated does not contain a multiple
        ; of 16 characters then figure out where to jump into the
        ; translation loop to translate the left over characters first.
        ; From then on the loop only deals with 16 characters at a time.
        ;
	and	eax, 15	
	je	SHORT $ACP_TopOfSBLoop             ; already a multiple of 16 chars.

        push    eax                            ; save for indexing into jump table
	sub	DWORD PTR _LoopCount$[ebp], eax ; decrement LoopCount
        add     edx, eax                        ; increment MultiByteString ptr
	lea	eax, DWORD PTR [eax*2]
        add     ebx, eax                        ; increment UnicodeString ptr

        ;
        ; Use ACP_JumpTable to jump into the while loop at the appropriate
        ; spot to take care of the *extra* characters.
        ;
        pop     eax
	dec	eax
	jmp	DWORD PTR cs:$ACP_JumpTable[eax*4]

        ;
        ; Main translation loop.  Translates 16 characters on each iteration.
        ;
$ACP_TopOfSBLoop:
	cmp	DWORD PTR _LoopCount$[ebp], 0
	jbe	$ACP_FinishedTranslation
        ;
        ; Adjust pointers for next iteration
        ;
        add     edx, 16                        ; increment MultiByteString ptr
        add     ebx, 32                        ; increment UnicodeString ptr
	sub	DWORD PTR _LoopCount$[ebp], 16 ; decrement LoopCount

        ;
        ; begin translation
        ;
	movzx	eax, WORD PTR [ebx-32]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-16], al
$ACP_SBAdjust15:
	movzx	eax, WORD PTR [ebx-30]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-15], al
$ACP_SBAdjust14:
	movzx	eax, WORD PTR [ebx-28]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-14], al
$ACP_SBAdjust13:
	movzx	eax, WORD PTR [ebx-26]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-13], al
$ACP_SBAdjust12:
	movzx	eax, WORD PTR [ebx-24]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-12], al
$ACP_SBAdjust11:
	movzx	eax, WORD PTR [ebx-22]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-11], al
$ACP_SBAdjust10:
	movzx	eax, WORD PTR [ebx-20]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-10], al
$ACP_SBAdjust9:
	movzx	eax, WORD PTR [ebx-18]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-9], al
$ACP_SBAdjust8:
	movzx	eax, WORD PTR [ebx-16]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-8], al
$ACP_SBAdjust7:
	movzx	eax, WORD PTR [ebx-14]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-7], al
$ACP_SBAdjust6:
	movzx	eax, WORD PTR [ebx-12]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-6], al
$ACP_SBAdjust5:
	movzx	eax, WORD PTR [ebx-10]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-5], al
$ACP_SBAdjust4:
	movzx	eax, WORD PTR [ebx-8]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-4], al
$ACP_SBAdjust3:
	movzx	eax, WORD PTR [ebx-6]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-3], al
$ACP_SBAdjust2:
	movzx	eax, WORD PTR [ebx-4]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-2], al
$ACP_SBAdjust1:
	movzx	eax, WORD PTR [ebx-2]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-1], al

	jmp	$ACP_TopOfSBLoop

        ;
        ; The ACP is a multibyte code page.  Translation is done here.
        ;
        ; WARNING!! WARNING!!  No optimization has been done on this loop.
        ;
$ACP_MBCS:
	mov	eax, DWORD PTR _BytesInUnicodeString$[ebp]
	shr	eax, 1
	mov	DWORD PTR _CharsInUnicodeString$[ebp], eax
	dec	DWORD PTR _CharsInUnicodeString$[ebp]
	or	eax, eax
	mov	ecx, DWORD PTR _NlsUnicodeToMbAnsiData
	je	SHORT $ACP_FinishedTranslation
$ACP_TopOfMBLoop:
        ;
        ; Check to make sure we have room in the destination string.
        ;
	mov	eax, DWORD PTR _MaxBytesInMultiByteString$[ebp]
	dec	DWORD PTR _MaxBytesInMultiByteString$[ebp]
	or	eax, eax
	je	SHORT $ACP_FinishedTranslation
        ;
        ; Grab the multibyte character(s) from the translation table
        ; and increment the source string pointer.
        ;
	mov	eax, DWORD PTR _UnicodeString$[ebp]
	movzx	eax, WORD PTR [eax]
	mov	dx, WORD PTR [ecx+eax*2]
	add	DWORD PTR _UnicodeString$[ebp], 2
	mov	bl, dh
        ;
        ; Check for a lead byte.
        ;
	or	bl, bl
	je	SHORT $ACP_NoLeadByte
        ;
        ; There is a lead byte.  Make sure there's room in the
        ; destination buffer for both the lead byte and trail byte.
        ;
	mov	eax, DWORD PTR _MaxBytesInMultiByteString$[ebp]
	dec	DWORD PTR _MaxBytesInMultiByteString$[ebp]
	or	eax, eax
	je	SHORT $ACP_FinishedTranslation
        ;
        ; Store the lead byte in the destination buffer, increment
        ; the destination pointer and decrement the count of remaining
        ; space.
        ;
	mov	eax, DWORD PTR _MultiByteString$[ebp]
	mov	BYTE PTR [eax], bl
	inc	DWORD PTR _MultiByteString$[ebp]
	dec	DWORD PTR _MaxBytesInMultiByteString$[ebp]
        ;
        ; Store the single byte or trail byte.
        ;
$ACP_NoLeadByte:
	mov	eax, DWORD PTR _MultiByteString$[ebp]
	mov	BYTE PTR [eax], dl
	inc	DWORD PTR _MultiByteString$[ebp]
        ;
        ; Check to see if there are any more characters to translate.
        ;
	mov	eax, DWORD PTR _CharsInUnicodeString$[ebp]
	dec	DWORD PTR _CharsInUnicodeString$[ebp]
	or	eax, eax
	jne	SHORT $ACP_TopOfMBLoop
        ;
        ; We're finished translating for the multibyte case.
        ; Set up edx so we can calculate the number of characters
        ; written (if the user has requested it).
        ;
        mov     edx, DWORD PTR _MultiByteString$[ebp]

$ACP_FinishedTranslation:
	mov	eax, DWORD PTR _BytesInMultiByteString$[ebp]
	or	eax, eax
	je	SHORT $ACP_NoOptParam
	sub	edx, DWORD PTR _MultiByteStringAnchor$[ebp]
	mov	DWORD PTR [eax], edx
$ACP_NoOptParam:
	sub	eax, eax
        pop     ebx
	leave
	ret	0                        ; return STATUS_SUCCESS

$ACP_JumpTable:
	DD	OFFSET FLAT:$ACP_SBAdjust1
	DD	OFFSET FLAT:$ACP_SBAdjust2
	DD	OFFSET FLAT:$ACP_SBAdjust3
	DD	OFFSET FLAT:$ACP_SBAdjust4
	DD	OFFSET FLAT:$ACP_SBAdjust5
	DD	OFFSET FLAT:$ACP_SBAdjust6
	DD	OFFSET FLAT:$ACP_SBAdjust7
	DD	OFFSET FLAT:$ACP_SBAdjust8
	DD	OFFSET FLAT:$ACP_SBAdjust9
	DD	OFFSET FLAT:$ACP_SBAdjust10
	DD	OFFSET FLAT:$ACP_SBAdjust11
	DD	OFFSET FLAT:$ACP_SBAdjust12
	DD	OFFSET FLAT:$ACP_SBAdjust13
	DD	OFFSET FLAT:$ACP_SBAdjust14
	DD	OFFSET FLAT:$ACP_SBAdjust15

_RtlUnicodeToMultiByteN ENDP

_OemString$            equ   8
_MaxBytesInOemString$  equ  12
_BytesInOemString$     equ  16
_UnicodeString$        equ  20
_BytesInUnicodeString$ equ  24
_LoopCount$            equ  -4
_OemStringAnchor$      equ  -8
_CharsInUnicodeString$ equ -12

        public  _RtlUnicodeToOemN

; NTSTATUS
; RtlUnicodeToOemN(
;     OUT PCH OemString,
;     IN ULONG MaxBytesInOemString,
;     OUT PULONG BytesInOemString OPTIONAL,
;     IN PWCH UnicodeString,
;     IN ULONG BytesInUnicodeString)
;
; /*++
;
; Routine Description:
;
;     This functions converts the specified unicode source string into an
;     oem string. The translation is done with respect to the OEM Code
;     Page (OCP) loaded at boot time.
;
; Arguments:
;
;     OemString - Returns an oem string that is equivalent to the
;         unicode source string.  If the translation can not be done
;         because a character in the unicode string does not map to an
;         oem character in the OCP, an error is returned.
;
;     MaxBytesInOemString - Supplies the maximum number of bytes to be
;         written to OemString.  If this causes OemString to be a
;         truncated equivalent of UnicodeString, no error condition results.
;
;     BytesInOemString - Returns the number of bytes in the returned
;         oem string pointed to by OemString.
;
;     UnicodeString - Supplies the unicode source string that is to be
;         converted to oem.
;
;     BytesInUnicodeString - The number of bytes in the the string pointed to by
;         UnicodeString.
;
; Return Value:
;
;     SUCCESS - The conversion was successful
;
;     STATUS_BUFFER_OVERFLOW - MaxBytesInOemString was not enough to hold
;         the whole Oem string.  It was converted correct to the point though.
;
; --*/
_RtlUnicodeToOemN proc
	push	ebp
	mov	ebp, esp
	sub	esp, 12
        push    ebx
        ;
        ; Save beginning position of OemString ptr for later
        ; use in calculating number of characters translated.
        ;
	mov	eax, DWORD PTR _OemString$[ebp]
	mov	DWORD PTR _OemStringAnchor$[ebp], eax
        ;
        ; Convert BytesInUnicodeString to a character count and
        ; compare against the maximum number of characters we have
        ; room to translate.  Use the minimum for the loop count.
        ;
	mov	eax, DWORD PTR _BytesInUnicodeString$[ebp]
	shr	eax, 1
	mov	ecx, DWORD PTR _MaxBytesInOemString$[ebp]
	sub	eax, ecx
	sbb	edx, edx
	and	eax, edx
	add	eax, ecx
	mov	DWORD PTR _LoopCount$[ebp], eax
        ;
        ; Set up registers such that:
        ;    ebx: UnicodeString
        ;    ecx: NlsUnicodeToOemData
        ;    edx: OemString
        ;
	mov	edx, DWORD PTR _OemString$[ebp]
	mov	ebx, DWORD PTR _UnicodeString$[ebp]
	mov	ecx, DWORD PTR _NlsUnicodeToOemData
        ;
        ; Determine if we're dealing with SBCS or MBCS.
        ;
	cmp	BYTE PTR _NlsMbOemCodePageTag, 0    ; 0 -> sbcs, 1 -> mbcs
	jne	$OEMCP_MBCS
        ;
        ; If the string to be translated does not contain a multiple
        ; of 16 characters then figure out where to jump into the
        ; translation loop to translate the left over characters first.
        ; From then on the loop only deals with 16 characters at a time.
        ;
	and	eax, 15	
	je	SHORT $OEMCP_TopOfSBLoop        ; already a multiple of 16 chars.

        push    eax                             ; save for indexing into jump table
	sub	DWORD PTR _LoopCount$[ebp], eax ; decrement LoopCount
        add     edx, eax                        ; increment OemString ptr
	lea	eax, DWORD PTR [eax*2]
        add     ebx, eax                        ; increment UnicodeString ptr

        ;
        ; Use OEMCP_JumpTable to jump into the while loop at the appropriate
        ; spot to take care of the *extra* characters.
        ;
        pop     eax
	dec	eax
	jmp	DWORD PTR cs:$OEMCP_JumpTable[eax*4]

        ;
        ; Main translation loop.  Translates 16 characters on each iteration.
        ;
$OEMCP_TopOfSBLoop:
	cmp	DWORD PTR _LoopCount$[ebp], 0
	jbe	$OEMCP_FinishedTranslation
        ;
        ; Adjust pointers for next iteration
        ;
        add     edx, 16                        ; increment OemString ptr
        add     ebx, 32                        ; increment UnicodeString ptr
	sub	DWORD PTR _LoopCount$[ebp], 16 ; decrement LoopCount

        ;
        ; begin translation
        ;
	movzx	eax, WORD PTR [ebx-32]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-16], al
$OEMCP_SBAdjust15:
	movzx	eax, WORD PTR [ebx-30]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-15], al
$OEMCP_SBAdjust14:
	movzx	eax, WORD PTR [ebx-28]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-14], al
$OEMCP_SBAdjust13:
	movzx	eax, WORD PTR [ebx-26]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-13], al
$OEMCP_SBAdjust12:
	movzx	eax, WORD PTR [ebx-24]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-12], al
$OEMCP_SBAdjust11:
	movzx	eax, WORD PTR [ebx-22]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-11], al
$OEMCP_SBAdjust10:
	movzx	eax, WORD PTR [ebx-20]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-10], al
$OEMCP_SBAdjust9:
	movzx	eax, WORD PTR [ebx-18]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-9], al
$OEMCP_SBAdjust8:
	movzx	eax, WORD PTR [ebx-16]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-8], al
$OEMCP_SBAdjust7:
	movzx	eax, WORD PTR [ebx-14]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-7], al
$OEMCP_SBAdjust6:
	movzx	eax, WORD PTR [ebx-12]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-6], al
$OEMCP_SBAdjust5:
	movzx	eax, WORD PTR [ebx-10]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-5], al
$OEMCP_SBAdjust4:
	movzx	eax, WORD PTR [ebx-8]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-4], al
$OEMCP_SBAdjust3:
	movzx	eax, WORD PTR [ebx-6]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-3], al
$OEMCP_SBAdjust2:
	movzx	eax, WORD PTR [ebx-4]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-2], al
$OEMCP_SBAdjust1:
	movzx	eax, WORD PTR [ebx-2]
	mov	al, BYTE PTR [eax+ecx]
	mov	BYTE PTR [edx-1], al

	jmp	$OEMCP_TopOfSBLoop

        ;
        ; The OEMCP is a multibyte code page.  Translation is done here.
        ;
        ; WARNING!! WARNING!!  No optimization has been done on this loop.
        ;
$OEMCP_MBCS:
	mov	eax, DWORD PTR _BytesInUnicodeString$[ebp]
	shr	eax, 1
	mov	DWORD PTR _CharsInUnicodeString$[ebp], eax
	dec	DWORD PTR _CharsInUnicodeString$[ebp]
	or	eax, eax
	mov	ecx, DWORD PTR _NlsUnicodeToMbOemData
	je	SHORT $OEMCP_FinishedTranslation
$OEMCP_TopOfMBLoop:
        ;
        ; Check to make sure we have room in the destination string.
        ;
	mov	eax, DWORD PTR _MaxBytesInOemString$[ebp]
	dec	DWORD PTR _MaxBytesInOemString$[ebp]
	or	eax, eax
	je	SHORT $OEMCP_FinishedTranslation
        ;
        ; Grab the multibyte character(s) from the translation table
        ; and increment the source string pointer.
        ;
	mov	eax, DWORD PTR _UnicodeString$[ebp]
	movzx	eax, WORD PTR [eax]
	mov	dx, WORD PTR [ecx+eax*2]
	add	DWORD PTR _UnicodeString$[ebp], 2
	mov	bl, dh
        ;
        ; Check for a lead byte.
        ;
	or	bl, bl
	je	SHORT $OEMCP_NoLeadByte
        ;
        ; There is a lead byte.  Make sure there's room in the
        ; destination buffer for both the lead byte and trail byte.
        ;
	mov	eax, DWORD PTR _MaxBytesInOemString$[ebp]
	dec	DWORD PTR _MaxBytesInOemString$[ebp]
	or	eax, eax
	je	SHORT $OEMCP_FinishedTranslation
        ;
        ; Store the lead byte in the destination buffer, increment
        ; the destination pointer and decrement the count of remaining
        ; space.
        ;
	mov	eax, DWORD PTR _OemString$[ebp]
	mov	BYTE PTR [eax], bl
	inc	DWORD PTR _OemString$[ebp]
	dec	DWORD PTR _MaxBytesInOemString$[ebp]
        ;
        ; Store the single byte or trail byte.
        ;
$OEMCP_NoLeadByte:
	mov	eax, DWORD PTR _OemString$[ebp]
	mov	BYTE PTR [eax], dl
	inc	DWORD PTR _OemString$[ebp]
        ;
        ; Check to see if there are any more characters to translate.
        ;
	mov	eax, DWORD PTR _CharsInUnicodeString$[ebp]
	dec	DWORD PTR _CharsInUnicodeString$[ebp]
	or	eax, eax
	jne	SHORT $OEMCP_TopOfMBLoop
        ;
        ; We're finished translating for the multibyte case.
        ; Set up edx so we can calculate the number of characters
        ; written (if the user has requested it).
        ;
        mov     edx, DWORD PTR _OemString$[ebp]

$OEMCP_FinishedTranslation:
	mov	eax, DWORD PTR _BytesInOemString$[ebp]
	or	eax, eax
	je	SHORT $OEMCP_NoOptParam
	sub	edx, DWORD PTR _OemStringAnchor$[ebp]
	mov	DWORD PTR [eax], edx
$OEMCP_NoOptParam:
        ;
        ; If we ran out of space in the destination buffer before
        ; translating all of the Unicode characters then return
        ; STATUS_BUFFER_OVERFLOW.  Check is done by looking at
        ; # of chars in Unicode string left to translate.
        ;
;
; WARNING!
;
; we can't check CharsInUnicodeString since we determined the loop
; count above and don't modify CharsInUnicodeString anymore...
;
;	cmp	DWORD PTR _CharsInUnicodeString$[ebp], 1
;	cmc
;	sbb	eax, eax
;	and	eax, -2147483643    ; STATUS_BUFFER_OVERFLOW (80000005H)
        sub     eax, eax     ; return STATUS_SUCCESS
        pop     ebx
	leave
	ret	0

$OEMCP_JumpTable:
	DD	OFFSET FLAT:$OEMCP_SBAdjust1
	DD	OFFSET FLAT:$OEMCP_SBAdjust2
	DD	OFFSET FLAT:$OEMCP_SBAdjust3
	DD	OFFSET FLAT:$OEMCP_SBAdjust4
	DD	OFFSET FLAT:$OEMCP_SBAdjust5
	DD	OFFSET FLAT:$OEMCP_SBAdjust6
	DD	OFFSET FLAT:$OEMCP_SBAdjust7
	DD	OFFSET FLAT:$OEMCP_SBAdjust8
	DD	OFFSET FLAT:$OEMCP_SBAdjust9
	DD	OFFSET FLAT:$OEMCP_SBAdjust10
	DD	OFFSET FLAT:$OEMCP_SBAdjust11
	DD	OFFSET FLAT:$OEMCP_SBAdjust12
	DD	OFFSET FLAT:$OEMCP_SBAdjust13
	DD	OFFSET FLAT:$OEMCP_SBAdjust14
	DD	OFFSET FLAT:$OEMCP_SBAdjust15

_RtlUnicodeToOemN ENDP

_TEXT	ENDS
        end
