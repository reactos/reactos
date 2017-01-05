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

PMSV1_0_AV_PAIR
NtlmAvlInit(IN void * pAvList)
{
    PMSV1_0_AV_PAIR pAvPair = (PMSV1_0_AV_PAIR)pAvList;
    pAvPair->AvId = MsvAvEOL;
    pAvPair->AvLen = 0;
    return pAvPair;
}

PMSV1_0_AV_PAIR
NtlmAvlGet(IN PMSV1_0_AV_PAIR pAvList,
           IN MSV1_0_AVID AvId,
           IN LONG cAvList)
{
    PMSV1_0_AV_PAIR pAvPair = pAvList;

    do
    {
        if (pAvPair->AvId == AvId)
            return pAvPair;
        if (pAvPair->AvId == MsvAvEOL)
            return NULL;
        cAvList -= (pAvPair->AvLen + sizeof(MSV1_0_AV_PAIR));
        if (cAvList <= 0)
            return NULL;
        pAvPair = (PMSV1_0_AV_PAIR)((PUCHAR)pAvPair + pAvPair->AvLen +
            sizeof(MSV1_0_AV_PAIR));
    }while(pAvPair);
    return NULL;
}

ULONG
NtlmAvlLen(IN PMSV1_0_AV_PAIR pAvList,
           IN LONG cAvList)
{
    PMSV1_0_AV_PAIR pCurPair = NtlmAvlGet(pAvList, MsvAvEOL, cAvList);

    if(!pCurPair)
        return 0;
    return (ULONG)(((PUCHAR)pCurPair - (PUCHAR)pAvList) + sizeof(MSV1_0_AV_PAIR));
}

PMSV1_0_AV_PAIR
NtlmAvlAdd(IN PMSV1_0_AV_PAIR pAvList,
           IN MSV1_0_AVID AvId,
           IN PUNICODE_STRING pString,
           IN LONG cAvList)
{
    PMSV1_0_AV_PAIR pCurPair = NtlmAvlGet(pAvList, MsvAvEOL, cAvList);

    if(!pCurPair)
        return NULL;

    pCurPair->AvId = (USHORT)AvId;
    pCurPair->AvLen = (USHORT)pString->Length;
    memcpy(pCurPair+1, pString->Buffer, pCurPair->AvLen);

    pCurPair = (PMSV1_0_AV_PAIR)((PUCHAR)pCurPair + sizeof(MSV1_0_AV_PAIR) + pCurPair->AvLen);
    pCurPair->AvId = MsvAvEOL;
    pCurPair->AvLen = 0;

    return pCurPair;
}

ULONG
NtlmAvlSize(IN ULONG Pairs,
            IN ULONG PairsLen)
{
    return(Pairs * sizeof(MSV1_0_AV_PAIR) +
           PairsLen + sizeof(MSV1_0_AV_PAIR));
}

