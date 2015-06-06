////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*
    Module:
            Namesup.cpp

    Abstract: FileName support routines
*/

#include "udffs.h"

//  '\dfdf\aaa\ffg'  -->  '\aaa\ffg'
//       '\aaa\ffg'  -->  '\ffg'
PWCHAR
__fastcall
UDFDissectName(
    IN  PWCHAR   Buffer,
    OUT PUSHORT  Length
    )
{

    USHORT  i;

//#ifdef _X86_
#ifdef _MSC_VER

    PWCHAR retval;

    __asm push  ebx
    __asm push  ecx

    __asm mov   ebx,Buffer
    __asm xor   ecx,ecx
Remove_leading_slash:
    __asm cmp   [word ptr ebx],L'\\'
    __asm jne   No_IncPointer
    __asm add   ebx,2
    __asm jmp   Remove_leading_slash
No_IncPointer:
    __asm cmp   [word ptr ebx],L':'
    __asm jne   Scan_1
    __asm add   ebx,2
    __asm inc   ecx
    __asm jmp   EO_Dissect
Scan_1:
    __asm mov   ax,[word ptr ebx]
    __asm cmp   ax,L'\\'
    __asm je    EO_Dissect
    __asm or    ax,ax
    __asm jz    EO_Dissect
    __asm cmp   ax,L':'
    __asm jne   Cont_scan
    __asm or    ecx,ecx
    __asm jnz   EO_Dissect
Cont_scan:
    __asm inc   ecx
    __asm add   ebx,2
    __asm jmp   Scan_1
EO_Dissect:
    __asm mov   retval,ebx
    __asm mov   i,cx

    __asm pop   ecx
    __asm pop   ebx

    *Length = i;
    return retval;

#else   // NO X86 optimization , use generic C/C++

    while (Buffer[0] == L'\\') {
        Buffer++;
    }
    if (Buffer[0] == L':') {
        *Length = 1;
        return &(Buffer[1]);
    }
    for(i = 0; (  Buffer[i] != L'\\' &&
                ((Buffer[i] != L':') || !i) &&
                  Buffer[i]);  i++);
    *Length = i;
    return &(Buffer[i]);

#endif // _X86_

} // end UDFDissectName()

BOOLEAN
__fastcall
UDFIsNameValid(
    IN PUNICODE_STRING SearchPattern,
    OUT BOOLEAN* StreamOpen,
    OUT ULONG* SNameIndex
) {
    LONG   Index, l;
    BOOLEAN _StreamOpen = FALSE;
    PWCHAR Buffer;
    WCHAR c, c0;

    if(StreamOpen) (*StreamOpen) = FALSE;
    // We can't create nameless file or too long path
    if(!(l = SearchPattern->Length/sizeof(WCHAR)) ||
        (l>UDF_X_PATH_LEN)) return FALSE;
    Buffer = SearchPattern->Buffer;
    for(Index = 0; Index<l; Index++, Buffer++) {
        // Check for disallowed characters
        c = (*Buffer);
        if((c == L'*') ||
           (c == L'>') ||
           (c == L'\"') ||
           (c == L'/') ||
           (c == L'<') ||
           (c == L'|') ||
           ((c >= 0x0000) && (c <= 0x001f)) ||
           (c == L'?')) return FALSE;
        // check if this a Stream path (& validate it)
        if(!(_StreamOpen) && // sub-streams are not allowed
            (Index<(l-1)) && // stream name must be specified
           ((_StreamOpen) = (c == L':'))) {
            if(StreamOpen) (*StreamOpen) = TRUE;
            if(SNameIndex) (*SNameIndex) = Index;
        }
        // According to NT IFS documentation neither SPACE nor DOT can be
        // a trailing character
        if(Index && (c == L'\\') ) {
           if((c0 == L' ') ||
              (_StreamOpen) || // stream is not a directory
              (c0 == L'.')) return FALSE;
        }
        c0 = c;
    }
    // According to NT IFS documentation neither SPACE nor DOT can be
    // a trailing character
    if((c0 == L' ') ||
       (c0 == L'.')) return FALSE;
    return TRUE;
} // end UDFIsNameValid()


#ifndef _CONSOLE
/*

Routine Description:

    This routine will compare two Unicode strings.
    PtrSearchPattern may contain wildcards

Return Value:

    BOOLEAN - TRUE if the expressions match, FALSE otherwise.

*/
BOOLEAN
UDFIsNameInExpression(
    IN PVCB Vcb,
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING PtrSearchPattern,
    OUT PBOOLEAN DosOpen,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ContainsWC,
    IN BOOLEAN CanBe8dot3,
    IN BOOLEAN KeepIntact // passed to UDFDOSName
    )
{
    BOOLEAN             Match = TRUE;
    UNICODE_STRING      ShortName;
    WCHAR               Buffer[13];

    if(!PtrSearchPattern) return TRUE;
    // we try to open file by LFN by default
    if(DosOpen) (*DosOpen) = FALSE;
    //  If there are wildcards in the expression then we call the
    //  appropriate FsRtlRoutine.
    if(ContainsWC) {
        Match = FsRtlIsNameInExpression( PtrSearchPattern, FileName, IgnoreCase, NULL );
    //  Otherwise do a direct memory comparison for the name string.
    } else if (RtlCompareUnicodeString(FileName, PtrSearchPattern, IgnoreCase)) {
        Match = FALSE;
    }

    if(Match) return TRUE;

    // check if SFN can match this pattern
    if(!CanBe8dot3)
        return FALSE;

    // try to open by SFN
    ShortName.Buffer = (PWCHAR)(&Buffer);
    ShortName.MaximumLength = 13*sizeof(WCHAR);
    UDFDOSName(Vcb, &ShortName, FileName, KeepIntact);

    // PtrSearchPattern is upcased if we are called with IgnoreCase=TRUE
    // DOSName is always upcased
    // thus, we can use case-sensetive compare here to improve performance
    if(ContainsWC) {
        Match = FsRtlIsNameInExpression( PtrSearchPattern, &ShortName, FALSE, NULL );
    //  Otherwise do a direct memory comparison for the name string.
    } else if (!RtlCompareUnicodeString(&ShortName, PtrSearchPattern, FALSE)) {
        Match = TRUE;
    }
    if(DosOpen && Match) {
        // remember that we've opened file by SFN
        (*DosOpen) = TRUE;
    }
    return Match;
} // end UDFIsNameInExpression()

#endif

BOOLEAN
__fastcall
UDFIsMatchAllMask(
    IN PUNICODE_STRING Name,
   OUT BOOLEAN* DosOpen
    )
{
    USHORT i;
    PWCHAR Buffer;

    if(DosOpen)
        *DosOpen = FALSE;
    Buffer = Name->Buffer;
    if(Name->Length == sizeof(WCHAR)) {
        // Win32-style wildcard
        if((*Buffer) != L'*')
            return FALSE;
        return TRUE;
    } else
    if(Name->Length == sizeof(WCHAR)*(8+1+3)) {
        // DOS-style wildcard
        for(i=0;i<8;i++,Buffer++) {
            if((*Buffer) != DOS_QM)
                return FALSE;
        }
        if((*Buffer) != DOS_DOT)
            return FALSE;
        Buffer++;
        for(i=9;i<12;i++,Buffer++) {
            if((*Buffer) != DOS_QM)
                return FALSE;
        }
        if(*DosOpen)
            *DosOpen = TRUE;
        return TRUE;
    } else
    if(Name->Length == sizeof(WCHAR)*(3)) {
        // DOS-style wildcard
        if(Buffer[0] != DOS_STAR)
            return FALSE;
        if(Buffer[1] != DOS_DOT)
            return FALSE;
        if(Buffer[2] != DOS_STAR)
            return FALSE;
        if(*DosOpen)
            *DosOpen = TRUE;
        return TRUE;
    } else {
        return FALSE;
    }
} // end UDFIsMatchAllMask()

BOOLEAN
__fastcall
UDFCanNameBeA8dot3(
    IN PUNICODE_STRING Name
    )
{
    if(Name->Length >= 13 * sizeof(WCHAR))
        return FALSE;

    ULONG i,l;
    ULONG dot_pos=0;
    ULONG ext_len=0;
    PWCHAR buff = Name->Buffer;

    l = Name->Length / sizeof(WCHAR);

    for(i=0; i<l; i++, buff++) {
        if( ((*buff) == L'.') ||
            ((*buff) == DOS_DOT) ) {
            if(dot_pos)
                return FALSE;
            dot_pos = i+1;
        } else
        if(dot_pos) {
            ext_len++;
            if(ext_len > 3)
                return FALSE;
        } else
        if(i >= 8) {
            return FALSE;
        }
    }
    return TRUE;
} // end UDFCanNameBeA8dot3()
