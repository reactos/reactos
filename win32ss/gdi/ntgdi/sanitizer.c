/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/sanitizer.c
 * PURPOSE:         Address sanitizer
 * PROGRAMMERS:     Copyright 2022 Katayama Hirofumi MZ.
 */

/** Includes ******************************************************************/

#include <win32k.h>

/* FUNCTIONS ******************************************************************/

VOID FASTCALL ValidateReadPtr(IN LPCVOID lp, IN UINT_PTR ucb)
{
    memcmp(lp, lp, ucb);
}

VOID FASTCALL ValidateWritePtr(IN LPVOID lp, IN UINT_PTR ucb)
{
    memcpy(lp, lp, ucb);
}

VOID FASTCALL ValidateStringPtrA(IN LPCSTR lpsz)
{
    while (*lpsz)
    {
        *lpsz = *lpsz;
        ++lpsz;
    }
}

VOID FASTCALL ValidateStringPtrW(IN LPCWSTR lpsz)
{
    while (*lpsz)
    {
        *lpsz = *lpsz;
        ++lpsz;
    }
}
