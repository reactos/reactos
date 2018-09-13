/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WRES32.C
 *  WOW32 16-bit resource support
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wres32.c);

HANDLE APIENTRY W32FindResource(HANDLE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLang)
{
    PRES p;

    //
    // If hModule is not ours, then make Win32 call and return the
    // result to USER.
    //

    if (LOWORD (hModule) == 0) {
        return (FindResourceEx(hModule, lpType, lpName, wLang));
    }
    else {
        WOW32ASSERT(GETHMOD16(hModule));
        p = FindResource16(GETHMOD16(hModule), (LPSTR)lpName, (LPSTR)lpType);
        return HRES32(p);
    }

}

HANDLE APIENTRY W32LoadResource(HANDLE hModule, HANDLE hResInfo)
{
    PRES p;

    //
    // If hModule is not ours, then make Win32 call and return the
    // result to USER.
    //

    if (ISINST16(hModule) && ISRES16(hResInfo)) {
        WOW32ASSERT(GETHMOD16(hModule));
        p = LoadResource16(GETHMOD16(hModule), GETHRES16(hResInfo));
        return HRES32(p);
    }
    else {
        return LoadResource(hModule, hResInfo);
    }
}


BOOL APIENTRY W32FreeResource(HANDLE hResData, HANDLE hModule)
{

    //
    // If hModule is not ours, then make Win32 call and return the
    // result to USER.
    //

    if ((LOWORD (hModule) != 0) && ISRES16(hResData)) {
        return FreeResource16(GETHRES16(hResData));
    }
    else {
        return (FreeResource(hResData));
    }
}


LPSTR APIENTRY W32LockResource(HANDLE hResData, HANDLE hModule)
{

    //
    // If hModule is not ours, then make Win32 call and return the
    // result to USER.
    //

    if ((LOWORD (hModule) != 0) && ISRES16(hResData)) {
        return LockResource16(GETHRES16(hResData));
    }
    else {
        return (LockResource(hResData));
    }
}


BOOL APIENTRY W32UnlockResource(HANDLE hResData, HANDLE hModule)
{

    //
    // If hModule is not ours, then make Win32 call and return the
    // result to USER.
    //

    if ((LOWORD (hModule) != 0) && ISRES16(hResData)) {
        return UnlockResource16(GETHRES16(hResData));
    }
    else {
        return (UnlockResource(hResData));
    }
}


DWORD APIENTRY W32SizeofResource(HANDLE hModule, HANDLE hResInfo)
{

    //
    // If hModule is not ours, then make Win32 call and return the
    // result to USER.
    //

    if ((LOWORD (hModule) != 0) && ISINST16(hModule) && ISRES16(hResInfo)) {
        WOW32ASSERT(GETHMOD16(hModule));
        return SizeofResource16(GETHMOD16(hModule), GETHRES16(hResInfo));
    }
    else {
        return (SizeofResource(hModule, hResInfo));
    }
}
