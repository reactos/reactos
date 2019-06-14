/*
 * This file contains NDIS5.X specific procedure definitions in NDIS driver.
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _PARA_NDIS5_H
#define _PARA_NDIS5_H

#include "ndis56common.h"


NDIS_STATUS NTAPI ParaNdis5_SetOID(IN NDIS_HANDLE MiniportAdapterContext,
                                            IN NDIS_OID Oid,
                                            IN PVOID InformationBuffer,
                                            IN ULONG InformationBufferLength,
                                            OUT PULONG BytesRead,
                                            OUT PULONG BytesNeeded);

NDIS_STATUS NTAPI ParaNdis5_QueryOID(IN NDIS_HANDLE  MiniportAdapterContext,
                                              IN NDIS_OID     Oid,
                                              IN PVOID        InformationBuffer,
                                              IN ULONG        InformationBufferLength,
                                              OUT PULONG      BytesWritten,
                                              OUT PULONG      BytesNeeded);


VOID NTAPI ParaNdis5_SendPackets(IN NDIS_HANDLE MiniportAdapterContext,
                               IN PPNDIS_PACKET PacketArray,
                               IN UINT NumberOfPackets);


VOID NTAPI ParaNdis5_ReturnPacket(IN NDIS_HANDLE  MiniportAdapterContext,IN PNDIS_PACKET Packet);

VOID ParaNdis5_IndicateConnect(PARANDIS_ADAPTER *pContext, BOOLEAN bConnected);


//NDIS 5.1 related functions
VOID NTAPI ParaNdis5_CancelSendPackets(IN NDIS_HANDLE MiniportAdapterContext,IN PVOID CancelId);

NDIS_STATUS ParaNdis5_StopSend(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN bStop,
    ONPAUSECOMPLETEPROC Callback);
NDIS_STATUS ParaNdis5_StopReceive(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN bStop,
    ONPAUSECOMPLETEPROC Callback
    );
VOID NTAPI ParaNdis5_HandleDPC(
    IN NDIS_HANDLE MiniportAdapterContext);

typedef struct _tagPowerWorkItem
{
    NDIS_WORK_ITEM              wi;
    PPARANDIS_ADAPTER           pContext;
    NDIS_DEVICE_POWER_STATE     state;
}tPowerWorkItem;

typedef struct _tagGeneralWorkItem
{
    NDIS_WORK_ITEM              wi;
    PPARANDIS_ADAPTER           pContext;
}tGeneralWorkItem;

#endif    // _PARA_NDIS5_H
