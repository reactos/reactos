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
        subttl  "RtlAnsiCharToUnicodeChar"
;++
;
; WCHAR
;RtlAnsiCharToUnicodeChar(
;    IN OUT PCHAR *SourceCharacter
;    )
;
;
; Routine Description:
;
;    This function translates the specified ansi character to unicode and
;    returns the unicode value.  The purpose for this routine is to allow
;    for character by character ansi to unicode translation.  The
;    translation is done with respect to the current system locale
;    information.
;
;
; Arguments:
;
;    (TOS+4) = SourceCharacter - Supplies a pointer to an ansi character pointer.
;        Through two levels of indirection, this supplies an ansi
;        character that is to be translated to unicode.  After
;        translation, the ansi character pointer is modified to point to
;        the next character to be converted.  This is done to allow for
;        dbcs ansi characters.
;
; Return Value:
;
;    Returns the unicode equivalent of the specified ansi character.
;
; Note:
;
;    This routine will have to be converted to use the proper unicode mapping
;    tables.
;
;--
cPublicProc _RtlAnsiCharToUnicodeChar ,1
cPublicFpo 1,2
        push    esi
        mov     esi, [esp+8]
        push    dword ptr [esi]         ; Save the old input string
        inc     dword ptr [esi]         ; Bump the input string
        pop     esi                     ; (ESI) = input string
        xor     eax, eax                ; Zero out the EAX register.
        lodsb                           ; Grab the first character from string
        pop     esi
        stdRET    _RtlAnsiCharToUnicodeChar

stdENDP _RtlAnsiCharToUnicodeChar

        page
        subttl  "RtlpAnsiPszToUnicodePsz"
;++
;
; VOID
; RtlpAnsiPszToUnicodePsz(
;    IN PCHAR AnsiString,
;    IN PWCHAR UnicodeString,
;    IN USHORT AnsiStringLength
;    )
;
;
; Routine Description:
;
;    This function translates the specified ansi character to unicode and
;    returns the unicode value.  The purpose for this routine is to allow
;    for character by character ansi to unicode translation.  The
;    translation is done with respect to the current system locale
;    information.
;
;
; Arguments:
;
;    (ESP+4) = AnsiString - Supplies a pointer to the ANSI string to convert to unicode.
;    (ESP+8) = UnicodeString - Supplies a pointer to a buffer to hold the unicode string
;    (ESP+12) = AnsiStringLength - Supplies the length of the ANSI string.
;
; Return Value:
;
;    None.
;
;
; Note:
;
;    This routine will have to be converted to use the proper unicode mapping
;    tables.
;--

cPublicProc _RtlpAnsiPszToUnicodePsz ,3
cPublicFpo 3,2
        push    esi
        push    edi
        xor     ecx, ecx
        mov     cx,  [esp]+12+8
        jecxz   raptup9
        mov     esi, [esp]+4+8
        mov     edi, [esp]+8+8
        xor     ah, ah
@@:     lodsb
        stosw
        loop    @b
        xor     eax, eax
        stosw                           ; Don't forget to stick the null at end
raptup9:pop     edi
        pop     esi
        stdRET    _RtlpAnsiPszToUnicodePsz

stdENDP _RtlpAnsiPszToUnicodePsz

_TEXT   ends
        end
