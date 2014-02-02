/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/lsasrv/utils.c
 * PURPOSE:         Utility functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "lsasrv.h"

#include <winuser.h>

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


PSID
LsapAppendRidToSid(
    PSID SrcSid,
    ULONG Rid)
{
    ULONG Rids[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    UCHAR RidCount;
    PSID DstSid;
    ULONG i;

    RidCount = *RtlSubAuthorityCountSid(SrcSid);
    if (RidCount >= 8)
        return NULL;

    for (i = 0; i < RidCount; i++)
        Rids[i] = *RtlSubAuthoritySid(SrcSid, i);

    Rids[RidCount] = Rid;
    RidCount++;

    RtlAllocateAndInitializeSid(RtlIdentifierAuthoritySid(SrcSid),
                                RidCount,
                                Rids[0],
                                Rids[1],
                                Rids[2],
                                Rids[3],
                                Rids[4],
                                Rids[5],
                                Rids[6],
                                Rids[7],
                                &DstSid);

    return DstSid;
}

/* EOF */
