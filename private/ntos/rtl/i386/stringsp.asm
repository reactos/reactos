        TITLE   "String support routines"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    stringsup.asm
;
; Abstract:
;
;    This module implements suplimentary routines for performing string
;    operations.
;
; Author:
;
;    Larry Osterman (larryo) 18-Sep-1991
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--

.386p

include callconv.inc            ; calling convention macros


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "RtlInitAnsiString"
;++
;
; VOID
; RtlInitAnsiString(
;     OUT PANSI_STRING DestinationString,
;     IN PSZ SourceString OPTIONAL
;     )
;
;
; Routine Description:
;
;    The RtlInitAnsiString function initializes an NT counted string.
;    The DestinationString is initialized to point to the SourceString
;    and the Length and MaximumLength fields of DestinationString are
;    initialized to the length of the SourceString, which is zero if
;    SourceString is not specified.
;
; Arguments:
;
;    (TOS+4) = DestinationString - Pointer to the counted string to initialize
;
;    (TOS+8) = SourceString - Optional pointer to a null terminated string that
;        the counted string is to point to.
;
;
; Return Value:
;
;    None.
;
; NOTE:
;       This routine assumes that the string is less than 64K in size.
;
;--

cPublicProc _RtlInitString ,2
cPublicFpo 2,2
        push    edi
        mov     edi,[esp]+8+4           ; (edi)= SourceString
        mov     edx,[esp]+4+4           ; (esi)= DestinationString
        mov     DWORD PTR [edx], 0      ; (Destination).Length = (Destination).MaximumLength = 0
        mov     DWORD PTR [edx]+4, edi  ; (Destination).Buffer = 0
        or      edi, edi
        jz      @f
        or      ecx, -1
        xor     eax, eax
        repne   scasb
        not     ecx
        mov     [edx]+2, cx             ; Save maximum length
        dec     ecx
        mov     [edx], cx               ; Save length
@@:     pop     edi
        stdRET    _RtlInitString

stdENDP _RtlInitString


cPublicProc _RtlInitAnsiString ,2
cPublicFpo 2,2
        push    edi
        mov     edi,[esp]+8+4           ; (edi)= SourceString
        mov     edx,[esp]+4+4           ; (esi)= DestinationString
        mov     DWORD PTR [edx], 0      ; (Destination).Length = (Destination).MaximumLength = 0
        mov     DWORD PTR [edx]+4, edi  ; (Destination).Buffer = 0
        or      edi, edi
        jz      @f
        or      ecx, -1
        xor     eax, eax
        repne   scasb
        not     ecx
        mov     [edx]+2, cx             ; Save maximum length
        dec     ecx
        mov     [edx], cx               ; Save length
@@:     pop     edi
        stdRET    _RtlInitAnsiString

stdENDP _RtlInitAnsiString


        page
        subttl  "RtlInitAnsiString"
;++
;
; VOID
; RtlInitAnsiString(
;     OUT PANSI_STRING DestinationString,
;     IN PSZ SourceString OPTIONAL
;     )
;
;
; Routine Description:
;
;    The RtlInitUnicodeString function initializes an NT counted string.
;    The DestinationString is initialized to point to the SourceString
;    and the Length and MaximumLength fields of DestinationString are
;    initialized to the length of the SourceString, which is zero if
;    SourceString is not specified.
;
; Arguments:
;
;    (TOS+4) = DestinationString - Pointer to the counted string to initialize
;
;    (TOS+8) = SourceString - Optional pointer to a null terminated string that
;        the counted string is to point to.
;
;
; Return Value:
;
;    None.
;
; NOTE:
;       This routine assumes that the string is less than 64K in size.
;
;--

cPublicProc _RtlInitUnicodeString ,2
cPublicFpo 2,2
        push    edi
        mov     edi,[esp]+8+4           ; (edi)= SourceString
        mov     edx,[esp]+4+4           ; (esi)= DestinationString
        mov     DWORD PTR [edx], 0      ; (Destination).Length = (Destination).MaximumLength = 0
        mov     DWORD PTR [edx]+4, edi  ; (Destination).Buffer = 0
        or      edi, edi
        jz      @f
        or      ecx, -1
        xor     eax, eax
        repne   scasw
        not     ecx
        shl     ecx,1
        mov     [edx]+2, cx             ; Save maximum length
        dec     ecx
        dec     ecx
        mov     [edx], cx               ; Save length
@@:     pop     edi
        stdRET    _RtlInitUnicodeString

stdENDP _RtlInitUnicodeString

_TEXT   ends
        end
