/*
 * Copyright 2011 Samuel Serapion
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#if 1
#include <precomp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);
#else
#include <apitest.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winnt.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>
#include <ntsam.h>

#include <dll/lsatest.h>
#include <dll/ntlmlib/calculations.h>
#include <dll/ntlmlib/util.h>
#include <dll/ntlmlib/ntlmlib.h>
#include <dll/ntlmlib/ciphers.h>
#include <shared/dbgutil.h>
#endif

BOOL
NtlmAvlGet(
    _In_ PSTRING pAvData,
    _In_ MSV1_0_AVID AvId,
    _Out_ PVOID* pData,
    _Out_ PULONG pLen)
{
    PMSV1_0_AV_PAIR pAvPair;
    PCHAR ptr = pAvData->Buffer;
    PCHAR ptrend = ptr + pAvData->Length;

    do
    {
        pAvPair = (PMSV1_0_AV_PAIR)ptr;
        ptr += sizeof(MSV1_0_AV_PAIR) + pAvPair->AvLen;

        /* check data end bound ... */
        if (ptr > ptrend)
            return FALSE;

        if (pAvPair->AvId == AvId)
        {
            *pLen = pAvPair->AvLen;
            *pData = (*pLen > 0) ? (PBYTE)(pAvPair+1) : NULL;
            return TRUE;
        }
        if (pAvPair->AvId == MsvAvEOL)
            return FALSE;

    } while (ptr < ptrend);

    return FALSE;
}

/*ULONG
NtlmAvlLen(IN PMSV1_0_AV_PAIR pAvList,
           IN LONG cAvList)
{
    PMSV1_0_AV_PAIR pCurPair = NtlmAvlGet(pAvList, MsvAvEOL, cAvList);

    if(!pCurPair)
        return 0;
    return (ULONG)(((PUCHAR)pCurPair - (PUCHAR)pAvList) + sizeof(MSV1_0_AV_PAIR));
}*/

BOOL
NtlmAvlAdd(
    _Inout_ PSTRING AvData,
    _In_ MSV1_0_AVID AvId,
    _In_ PVOID Data,
    _In_ ULONG DataLen)
{
    PMSV1_0_AV_PAIR NewPair;

    /* check available buffer size */
    if (AvData->Length + DataLen + sizeof(MSV1_0_AV_PAIR) > AvData->MaximumLength)
    {
        ERR("NtlmAvlAdd: Buffer to small!\n");
        return FALSE;
    }

    NewPair = (PMSV1_0_AV_PAIR)(AvData->Buffer + AvData->Length);
    NewPair->AvId = (USHORT)AvId;
    NewPair->AvLen = (USHORT)DataLen;
    AvData->Length += sizeof(MSV1_0_AV_PAIR);

    if (DataLen > 0)
        memcpy(AvData->Buffer + AvData->Length, Data, DataLen);
    AvData->Length += DataLen;

    return TRUE;
}

ULONG
NtlmAvlSize(IN ULONG Pairs,
            IN ULONG PairsLen)
{
    return(Pairs * sizeof(MSV1_0_AV_PAIR) +
           PairsLen + sizeof(MSV1_0_AV_PAIR));
}

