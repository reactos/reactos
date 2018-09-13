/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WULANG.C
 *  WOW32 16-bit User API support
 *
 *
 *  It thunks the win 3.x language functions to NT. These functions are
 *  mainly used by the programs that are ported to various international
 *  languages.
 *
 *  History:
 *  Created 19-April-1992 by Chandan Chauhan (ChandanC)
 *
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wulang.c);


/*++
    LPSTR AnsiLower(<lpString>)
    LPSTR <lpString>;

    The %AnsiLower% function converts the given character string to
    lowercase. The conversion is made by the language driver based on the
    criteria of the current language selected by the user at setup or with the
    Control Panel.

    <lpString>
        Points to a null-terminated string or specifies single character. If
        lpString specifies single character, that character is in the low-order
        byte of the low-order word, and the high-order word is zero.

    The return value points to a converted character string if the function
    parameter is a character string. Otherwise, it is a 32-bit value that
    contains the converted character in the low-order byte of the low-order
    word.
--*/

ULONG FASTCALL WU32AnsiLower(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    register PANSILOWER16 parg16;

    GETARGPTR(pFrame, sizeof(ANSILOWER16), parg16);
    GETPSZIDPTR(parg16->f1, psz1);

    ul = GETLPSTRBOGUS(AnsiLower(psz1));

    if (HIWORD(psz1)) {
        ul = parg16->f1;
    }

    FREEPSZIDPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    WORD AnsiLowerBuff(<lpString>, <nLength>)
    LPSTR <lpString>;
    WORD <nLength>;

    The %AnsiLowerBuff% function converts character string in a buffer to
    lowercase. The conversion is made by the language driver based on the
    criteria of the current language selected by the user at setup or with the
    Control Panel.

    <lpString>
        Points to a buffer containing one or more characters.

    <nLength>
        Specifies the number of characters in the buffer identified by
        the <lpString> parameter. If <nLength> is zero, the length is
        64K (65,536).

    The return value specifies the length of the converted string.
--*/

ULONG FASTCALL WU32AnsiLowerBuff(PVDMFRAME pFrame)
{
    ULONG ul;
    PBYTE pb1;
    register PANSILOWERBUFF16 parg16;

    GETARGPTR(pFrame, sizeof(ANSILOWERBUFF16), parg16);
    GETVDMPTR(parg16->f1, SIZETO64K(parg16->f2), pb1);

    ul = GETWORD16(AnsiLowerBuff(pb1, SIZETO64K(parg16->f2)));

    FLUSHVDMPTR(parg16->f1, SIZETO64K(parg16->f2), pb1);
    FREEVDMPTR(pb1);
    FREEARGPTR(parg16);
    RETURN(ul);
}



/*++
    LPSTR AnsiNext(<lpCurrentChar>)
    LPSTR <lpCurrentChar>;

    The %AnsiNext% function moves to the next character in a string.

    <lpCurrentChar>
        Points to a character in a null-terminated string.

    The return value points to the next character in the string, or, if there is
    no next character, to the null character at the end of the string.

    The %AnsiNext% function is used to move through strings whose characters are
    two or more bytes each (for example, strings that contain characters from a
    Japanese character set).
--*/

ULONG FASTCALL WU32AnsiNext(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    register PANSINEXT16 parg16;
    DWORD ret;

    GETARGPTR(pFrame, sizeof(ANSINEXT16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    ul = (ULONG) AnsiNext(psz1);

    ul = ul - (ULONG) psz1;

    ret = FETCHDWORD(parg16->f1);

    ul = MAKELONG((LOWORD(ret) + ul),HIWORD(ret));

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    LPSTR AnsiPrev(<lpStart>, <lpCurrentChar>)
    LPSTR <lpStart>;
    LPSTR <lpCurrentChar>;

    The %AnsiPrev% function moves to the previous character in a string.

    <lpStart>
        Points to the beginning of the string.

    <lpCurrentChar>
        Points to a character in a null-terminated string.

    The return value points to the previous character in the string, or to the
    first character in the string if the <lpCurrentChar> parameter is equal to
    the <lpStart> parameter.

    The %AnsiPrev% function is used to move through strings whose characters are
    two or more bytes each (for example, strings that contain characters from a
    Japanese character set).
--*/

ULONG FASTCALL WU32AnsiPrev(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    register PANSIPREV16 parg16;
    DWORD ret;
#ifdef FE_SB
    PSZ lpCurrent;
#endif // FE_SB

    GETARGPTR(pFrame, sizeof(ANSIPREV16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);

#ifdef FE_SB
    if (GetSystemDefaultLangID() == 0x411) {
        lpCurrent = psz2;

        // New Win32 CharPrev code for SUR-FE
        // The following code is correct.
        // But some Japanese Windows application does not work
        // with it.
        // Jpanese WOW uses old code with bug.
        //
        // if (psz1 > psz2)
        //     return psz1

        if (psz1 == psz2) {
            ul = (ULONG)psz1;
            goto PrevExit;
        }

        if (--lpCurrent == psz1) {
            ul = (ULONG)psz1;
            goto PrevExit;
        }

        // we assume lpCurrentChar never points the second byte
        // of double byte character
        // this check makes things a little bit faster [takaok]
        if (IsDBCSLeadByte(*lpCurrent)) {
            ul = (ULONG)lpCurrent-1;
            goto PrevExit;
        }

        do {
            lpCurrent--;
            if (!IsDBCSLeadByte(*lpCurrent)) {
                lpCurrent++;
                break;
            }
        } while(lpCurrent != psz1);

        ul = (ULONG)(psz2 - (((psz2 - lpCurrent) & 1) ? 1 : 2));
    }
    else
        ul = (ULONG) AnsiPrev(psz1, psz2);
PrevExit:
#else // !FE_SB
    ul = (ULONG) AnsiPrev(psz1, psz2);
#endif // !FE_SB

    ul = (ULONG) psz2 - ul;

    ret = FETCHDWORD(parg16->f2);

    ul = MAKELONG((LOWORD(ret) - ul),HIWORD(ret));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    LPSTR AnsiUpper(<lpString>)
    LPSTR <lpString>;

    The %AnsiUpper% function converts the given character string to
    uppercase. The conversion is made by the language driver based on the
    criteria of the current language selected by the user at setup or with the
    Control Panel.

    <lpString>
        Points to a null-terminated string or specifies single character. If
        lpString specifies a single character, that character is in the
        low-order byte of the low-order word, and the high-order word is zero.

    The return value points to a converted character string if the function
    parameter is a character string; otherwise, it is a 32-bit value that
    contains the converted character in the low-order byte of the low-order
    word.
--*/

ULONG FASTCALL WU32AnsiUpper(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    register PANSIUPPER16 parg16;

    GETARGPTR(pFrame, sizeof(ANSIUPPER16), parg16);
    GETPSZIDPTR(parg16->f1, psz1);

    ul = GETLPSTRBOGUS(AnsiUpper(psz1));

    if (HIWORD(psz1)) {
        ul = parg16->f1;
    }

    FREEPSZIDPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    WORD AnsiUpperBuff(<lpString>, <nLength>)
    LPSTR <lpString>;
    WORD <nLength>;

    The %AnsiUpperBuff% function converts a character string in a buffer to
    uppercase. The conversion is made by the language driver based on the
    criteria of the current language selected by the user at setup or with the
    Control Panel.

    <lpString>
        Points to a buffer containing one or more characters.

    <nLength>
        Specifies the number of characters in the buffer identified by
        the <lpString> parameter. If <nLength> is zero, the length is 64K
        (65,536).

    The return value specifies the length of the converted string.
--*/

ULONG FASTCALL WU32AnsiUpperBuff(PVDMFRAME pFrame)
{
    ULONG ul;
    PBYTE pb1;
    register PANSIUPPERBUFF16 parg16;

    GETARGPTR(pFrame, sizeof(ANSIUPPERBUFF16), parg16);
    GETVDMPTR(parg16->f1, SIZETO64K(parg16->f2), pb1);

    ul = GETWORD16(AnsiUpperBuff(pb1, SIZETO64K(parg16->f2)));

    FLUSHVDMPTR(parg16->f1, SIZETO64K(parg16->f2), pb1);
    FREEVDMPTR(pb1);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32lstrcmp(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    register PLSTRCMP16 parg16;

    GETARGPTR(pFrame, sizeof(LSTRCMP16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);

    ul = GETINT16(lstrcmp(psz1, psz2));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32lstrcmpi(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    register PLSTRCMPI16 parg16;

    GETARGPTR(pFrame, sizeof(LSTRCMPI16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);

    ul = GETINT16(lstrcmpi(psz1, psz2));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32IsCharAlpha(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISCHARALPHA16 parg16;

    GETARGPTR(pFrame, sizeof(ISCHARALPHA16), parg16);

    ul = GETBOOL16(IsCharAlpha(CHAR32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32IsCharAlphaNumeric(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISCHARALPHANUMERIC16 parg16;

    GETARGPTR(pFrame, sizeof(ISCHARALPHANUMERIC16), parg16);

    ul = GETBOOL16(IsCharAlphaNumeric(CHAR32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32IsCharLower(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISCHARLOWER16 parg16;

    GETARGPTR(pFrame, sizeof(ISCHARLOWER16), parg16);

    ul = GETBOOL16(IsCharLower(CHAR32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32IsCharUpper(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISCHARUPPER16 parg16;

    GETARGPTR(pFrame, sizeof(ISCHARUPPER16), parg16);

    ul = GETBOOL16(IsCharUpper(CHAR32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}
