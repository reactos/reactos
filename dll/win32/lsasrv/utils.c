/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/lsasrv/utils.c
 * PURPOSE:         Utility functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


/* GLOBALS *****************************************************************/


/* FUNCTIONS ***************************************************************/

INT
LsapLoadString(HINSTANCE hInstance,
               UINT uId,
               LPWSTR lpBuffer,
               INT nBufferMax)
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    /* Use loword (incremented by 1) as resourceid */
    hrsrc = FindResourceW(hInstance,
                          MAKEINTRESOURCEW((LOWORD(uId) >> 4) + 1),
                          (LPWSTR)RT_STRING);
    if (!hrsrc)
        return 0;

    hmem = LoadResource(hInstance, hrsrc);
    if (!hmem)
        return 0;

    p = LockResource(hmem);
    string_num = uId & 0x000f;
    for (i = 0; i < string_num; i++)
        p += *p + 1;

    i = min(nBufferMax - 1, *p);
    if (i > 0)
    {
        memcpy(lpBuffer, p + 1, i * sizeof(WCHAR));
        lpBuffer[i] = 0;
    }
    else
    {
        if (nBufferMax > 1)
        {
            lpBuffer[0] = 0;
            return 0;
        }
    }

    return i;
}

/* EOF */
