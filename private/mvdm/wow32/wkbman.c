/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKBMAN.C
 *  WOW32 16-bit Keyboard API support (manually-coded thunks)
 *
 *  History:
 *  Created  27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Modified 13-Jan-1992 by Nandurir . Added all the code.
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wkbman.c);

ULONG FASTCALL WKB32ToAscii(PVDMFRAME pFrame)
{
    ULONG ul;
    register PTOASCII16 parg16;
    LPSTR lpstrT;
    LPVOID lpvoidT;

    GETARGPTR(pFrame, sizeof(TOASCII16), parg16);
    GETPSZPTR(parg16->f3, lpstrT);
    GETMISCPTR(parg16->f4, lpvoidT);

    ul = ToAscii((WORD)(parg16->f1),
                 (WORD)(parg16->f2),
                 lpstrT,
                 lpvoidT,
                 (WORD)(parg16->f5));


    FREEPSZPTR(lpstrT);
    FLUSHVDMPTR(parg16->f4, 4, lpvoidT);
    FREEMISCPTR(lpvoidT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WKB32OemKeyScan(PVDMFRAME pFrame)
{
    ULONG ul;
    register POEMKEYSCAN16 parg16;

    GETARGPTR(pFrame, sizeof(OEMKEYSCAN16), parg16);

    ul = (ULONG)OemKeyScan((WORD)(parg16->f1));

    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WKB32VkKeyScan(PVDMFRAME pFrame)
{
    ULONG ul;
    register PVKKEYSCAN16 parg16;

    GETARGPTR(pFrame, sizeof(VKKEYSCAN16), parg16);

    ul = (ULONG)VkKeyScan((CHAR)(parg16->f1));

    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WKB32GetKeyboardType(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETKEYBOARDTYPE16 parg16;

    GETARGPTR(pFrame, sizeof(GETKEYBOARDTYPE16), parg16);

    ul = GetKeyboardType(INT32(parg16->f1));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WKB32MapVirtualKey(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMAPVIRTUALKEY16 parg16;

    GETARGPTR(pFrame, sizeof(MAPVIRTUALKEY16), parg16);

    ul = MapVirtualKey((UINT)(parg16->f1), (UINT)(parg16->f2));

    // MapVirtualKey sets the high bit (Win16 & Win32) to indicate diacritic
    if (ul & 0x80000000) {
        ul |= 0x8000;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WKB32GetKBCodePage(PVDMFRAME pFrame)
{
    ULONG ul;

    ul = (ULONG)GetKBCodePage();

    RETURN(ul);
}


ULONG FASTCALL WKB32GetKeyNameText(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETKEYNAMETEXT16 parg16;
    LPSTR lpstrT;

    GETARGPTR(pFrame, sizeof(GETKEYNAMETEXT16), parg16);
    GETPSZPTR(parg16->f2, lpstrT);

    ul = (ULONG)GetKeyNameText(DWORD32(parg16->f1), lpstrT,
                                                      (INT)(WORD)(parg16->f3));

    FLUSHVDMPTR(parg16->f2, (WORD) (parg16->f3), lpstrT);
    FREEPSZPTR(lpstrT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WKB32AnsiToOemBuff(PVDMFRAME pFrame)
{
    ULONG ul;
    register PANSITOOEMBUFF16 parg16;
    LPSTR  lpstrAnsi, lpstrOem;

    GETARGPTR(pFrame, sizeof(ANSITOOEMBUFF16), parg16);
    GETPSZPTR(parg16->f1, lpstrAnsi);
    GETPSZPTR(parg16->f2, lpstrOem);

    ul = AnsiToOemBuff(lpstrAnsi, lpstrOem, (DWORD)(WORD)(parg16->f3));

    FLUSHVDMPTR(parg16->f2, ((parg16->f3) ? (parg16->f3) : 0xFFFF), lpstrOem);
    FREEPSZPTR(lpstrAnsi);
    FREEPSZPTR(lpstrOem);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WKB32OemToAnsiBuff(PVDMFRAME pFrame)
{
    ULONG ul;
    register POEMTOANSIBUFF16 parg16;
    LPSTR  lpstrAnsi, lpstrOem;

    GETARGPTR(pFrame, sizeof(OEMTOANSIBUFF16), parg16);
    GETPSZPTR(parg16->f1, lpstrOem);
    GETPSZPTR(parg16->f2, lpstrAnsi);

    ul = (ULONG)OemToAnsiBuff(lpstrOem, lpstrAnsi, (DWORD)(WORD)(parg16->f3));

    FLUSHVDMPTR(parg16->f2, ((parg16->f3) ? (parg16->f3) : 0xFFFF), lpstrAnsi);
    FREEPSZPTR(lpstrOem);
    FREEPSZPTR(lpstrAnsi);
    FREEARGPTR(parg16);
    RETURN(ul);
}
