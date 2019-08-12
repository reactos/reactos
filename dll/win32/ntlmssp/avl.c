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

#include "ntlmssp.h"
#include "protocol.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

BOOL
NtlmAvlGet(IN PNTLM_DATABUF pAvData,
           IN MSV1_0_AVID AvId,
           OUT PVOID* pData,
           OUT PULONG pLen)
{
    PMSV1_0_AV_PAIR pAvPair;
    PBYTE ptr = pAvData->pData;
    PBYTE ptrend = ptr + pAvData->bUsed;

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
NtlmAvlAdd(IN PNTLM_DATABUF pAvData,
           IN MSV1_0_AVID AvId,
           IN void* data,
           IN ULONG len)
{
    PMSV1_0_AV_PAIR pNewPair;

    /* realloc not implemented ... */
    if (pAvData->bUsed + len + sizeof(MSV1_0_AV_PAIR) > pAvData->bAllocated)
    {
        ERR("NtlmAvlAdd - need more memory ...");
        return FALSE;
    }

    pNewPair = (PMSV1_0_AV_PAIR)(pAvData->pData + pAvData->bUsed);
    pNewPair->AvId = (USHORT)AvId;
    pNewPair->AvLen = (USHORT)len;
    pAvData->bUsed += sizeof(MSV1_0_AV_PAIR);

    memcpy(pAvData->pData + pAvData->bUsed, data, len);
    pAvData->bUsed += len;

    return TRUE;
}

ULONG
NtlmAvlSize(IN ULONG Pairs,
            IN ULONG PairsLen)
{
    return(Pairs * sizeof(MSV1_0_AV_PAIR) +
           PairsLen + sizeof(MSV1_0_AV_PAIR));
}

