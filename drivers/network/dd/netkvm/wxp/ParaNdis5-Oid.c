/*
 * This file contains NDIS5.X implementation of
 * OID-related adapter driver procedures
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
#include "ParaNdis5.h"
#include "ParaNdis-Oid.h"

#ifdef WPP_EVENT_TRACING
#include "ParaNdis5-Oid.tmh"
#endif

#define OIDENTRY(oid, el, xfl, xokl, flags) \
{ oid, el, xfl, xokl, flags, NULL }
#define OIDENTRYPROC(oid, el, xfl, xokl, flags, setproc) \
{ oid, el, xfl, xokl, flags, setproc }

static NDIS_TASK_OFFLOAD_HEADER ReservedHeader =
{
    NDIS_TASK_OFFLOAD_VERSION,
    sizeof(NDIS_TASK_OFFLOAD_HEADER),
    0,
    0,
    { IEEE_802_3_Encapsulation, { 1, 0 }, 0 }
};


static NDIS_OID SupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_VLAN_ID,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_TRANSMIT_QUEUE_LENGTH,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAC_OPTIONS,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_802_3_XMIT_DEFERRED,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_802_3_RCV_OVERRUN,
    OID_802_3_XMIT_UNDERRUN,
    OID_802_3_XMIT_HEARTBEAT_FAILURE,
    OID_802_3_XMIT_TIMES_CRS_LOST,
    OID_802_3_XMIT_LATE_COLLISIONS,
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ADD_WAKE_UP_PATTERN,
    OID_PNP_REMOVE_WAKE_UP_PATTERN,
    OID_PNP_ENABLE_WAKE_UP,
    OID_TCP_TASK_OFFLOAD
};

static NDIS_STATUS OnOidSetNdis5Offload(PARANDIS_ADAPTER *pContext, tOidDesc *pOid);
static NDIS_STATUS CreateOffloadInfo5ForQuery(PARANDIS_ADAPTER *pContext, tOidDesc *pOid, PVOID *ppInfo, PULONG pulSize);
static NDIS_STATUS CreateOffloadInfo5Internal(PARANDIS_ADAPTER *pContext, PVOID *ppInfo, PULONG pulSize, PCCHAR reason, NDIS_TASK_OFFLOAD_HEADER *pHeader);

/**********************************************************
Structure defining how to process all the oids
***********************************************************/
// oid                                          e f ok flags      set procedure
static const tOidWhatToDo OidsDB[] =
{
OIDENTRY(OID_GEN_SUPPORTED_LIST,                2,2,4, ohfQueryStat     ),
OIDENTRY(OID_GEN_HARDWARE_STATUS,               2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_MEDIA_SUPPORTED,               2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_MEDIA_IN_USE,                  2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_MAXIMUM_LOOKAHEAD,             2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_MAXIMUM_FRAME_SIZE,            2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_LINK_SPEED,                    6,0,6, ohfQuery         ),
OIDENTRY(OID_GEN_TRANSMIT_BUFFER_SPACE,         2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_RECEIVE_BUFFER_SPACE,          2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_TRANSMIT_BLOCK_SIZE,           2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_RECEIVE_BLOCK_SIZE,            2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_VENDOR_ID,                     2,0,4, ohfQueryStat     ),
OIDENTRY(OID_GEN_VENDOR_DESCRIPTION,            2,2,4, ohfQuery         ),
OIDENTRYPROC(OID_GEN_CURRENT_PACKET_FILTER,     2,0,4, ohfQuerySet, ParaNdis_OnSetPacketFilter),
OIDENTRYPROC(OID_GEN_CURRENT_LOOKAHEAD,         2,0,4, ohfQuerySet, ParaNdis_OnSetLookahead),
OIDENTRY(OID_GEN_DRIVER_VERSION,                2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_MAXIMUM_TOTAL_SIZE,            2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_PROTOCOL_OPTIONS,              2,0,4, 0                ),
OIDENTRY(OID_GEN_MAC_OPTIONS,                   2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_MEDIA_CONNECT_STATUS,          6,0,6, ohfQuery         ),
OIDENTRY(OID_GEN_MAXIMUM_SEND_PACKETS,          2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_VENDOR_DRIVER_VERSION,         2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_SUPPORTED_GUIDS,               2,2,4, 0                ),
OIDENTRY(OID_GEN_TRANSPORT_HEADER_OFFSET,       2,4,4, 0                ),
OIDENTRY(OID_GEN_MEDIA_CAPABILITIES,            2,4,4, 0                ),
OIDENTRY(OID_GEN_PHYSICAL_MEDIUM,               2,4,4, 0                ),
OIDENTRY(OID_GEN_XMIT_OK,                       6,0,6, ohfQuery3264     ),
OIDENTRY(OID_GEN_RCV_OK,                        6,0,4, ohfQuery3264     ),
OIDENTRY(OID_GEN_XMIT_ERROR,                    6,0,6, ohfQuery3264     ),
OIDENTRY(OID_GEN_RCV_ERROR,                     6,0,6, ohfQuery3264     ),
OIDENTRY(OID_GEN_RCV_NO_BUFFER,                 2,0,4, ohfQuery3264     ),
OIDENTRY(OID_GEN_DIRECTED_BYTES_XMIT,           2,4,4, 0                ),
OIDENTRY(OID_GEN_DIRECTED_FRAMES_XMIT,          2,4,4, 0                ),
OIDENTRY(OID_GEN_MULTICAST_BYTES_XMIT,          2,4,4, 0                ),
OIDENTRY(OID_GEN_MULTICAST_FRAMES_XMIT,         2,4,4, 0                ),
OIDENTRY(OID_GEN_BROADCAST_BYTES_XMIT,          2,4,4, 0                ),
OIDENTRY(OID_GEN_BROADCAST_FRAMES_XMIT,         2,4,4, 0                ),
OIDENTRY(OID_GEN_DIRECTED_BYTES_RCV,            2,4,4, 0                ),
OIDENTRY(OID_GEN_DIRECTED_FRAMES_RCV,           2,4,4, 0                ),
OIDENTRY(OID_GEN_MULTICAST_BYTES_RCV,           2,4,4, 0                ),
OIDENTRY(OID_GEN_MULTICAST_FRAMES_RCV,          2,4,4, 0                ),
OIDENTRY(OID_GEN_BROADCAST_BYTES_RCV,           2,4,4, 0                ),
OIDENTRY(OID_GEN_BROADCAST_FRAMES_RCV,          2,4,4, 0                ),
OIDENTRY(OID_GEN_RCV_CRC_ERROR,                 2,0,4, ohfQuery3264     ),
OIDENTRY(OID_GEN_TRANSMIT_QUEUE_LENGTH,         2,0,4, ohfQuery         ),
OIDENTRY(OID_GEN_GET_TIME_CAPS,                 2,4,4, 0                ),
OIDENTRY(OID_GEN_GET_NETCARD_TIME,              2,4,4, 0                ),
OIDENTRY(OID_GEN_NETCARD_LOAD,                  2,4,4, 0                ),
OIDENTRY(OID_GEN_DEVICE_PROFILE,                2,4,4, 0                ),
OIDENTRY(OID_GEN_INIT_TIME_MS,                  2,4,4, 0                ),
OIDENTRY(OID_GEN_RESET_COUNTS,                  2,4,4, 0                ),
OIDENTRY(OID_GEN_MEDIA_SENSE_COUNTS,            2,4,4, 0                ),
OIDENTRY(OID_PNP_CAPABILITIES,                  2,0,4, ohfQuery         ),
OIDENTRY(OID_PNP_QUERY_POWER,                   2,0,4, ohfQuery         ),
OIDENTRY(OID_802_3_PERMANENT_ADDRESS,           2,0,4, ohfQueryStat     ),
OIDENTRY(OID_802_3_CURRENT_ADDRESS,             2,0,4, ohfQueryStat     ),
OIDENTRY(OID_802_3_MAXIMUM_LIST_SIZE,           2,0,4, ohfQuery         ),
OIDENTRY(OID_802_3_MAC_OPTIONS,                 2,4,4, ohfQuery         ),
OIDENTRY(OID_802_3_RCV_ERROR_ALIGNMENT,         2,0,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_XMIT_ONE_COLLISION,          2,4,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_XMIT_MORE_COLLISIONS,        2,4,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_XMIT_DEFERRED,               2,0,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_XMIT_MAX_COLLISIONS,         2,0,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_RCV_OVERRUN,                 2,0,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_XMIT_UNDERRUN,               2,0,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_XMIT_HEARTBEAT_FAILURE,      2,0,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_XMIT_TIMES_CRS_LOST,         2,0,4, ohfQuery3264     ),
OIDENTRY(OID_802_3_XMIT_LATE_COLLISIONS,        2,0,4, ohfQuery3264     ),
OIDENTRY(OID_GEN_MACHINE_NAME,                  2,4,4, 0                ),
OIDENTRY(OID_IP4_OFFLOAD_STATS,                 4,4,4, 0                ),
OIDENTRY(OID_IP6_OFFLOAD_STATS,                 4,4,4, 0                ),
OIDENTRY(OID_802_11_CAPABILITY,                 4,4,4, 0                ),
OIDENTRYPROC(OID_PNP_ADD_WAKE_UP_PATTERN,       2,0,4, ohfSet,          ParaNdis_OnAddWakeupPattern),
OIDENTRYPROC(OID_PNP_REMOVE_WAKE_UP_PATTERN,    2,0,4, ohfSet,          ParaNdis_OnRemoveWakeupPattern),
OIDENTRYPROC(OID_PNP_ENABLE_WAKE_UP,            2,0,4, ohfQuerySet,     ParaNdis_OnEnableWakeup),
OIDENTRYPROC(OID_PNP_SET_POWER,                 2,0,4, ohfSet | ohfSetMoreOK,   ParaNdis_OnSetPower),
OIDENTRYPROC(OID_GEN_CURRENT_LOOKAHEAD,         2,0,4, ohfQuerySet,     ParaNdis_OnSetLookahead),
OIDENTRYPROC(OID_GEN_CURRENT_PACKET_FILTER,     2,0,4, ohfQuerySet,     ParaNdis_OnSetPacketFilter),
OIDENTRYPROC(OID_802_3_MULTICAST_LIST,          2,0,4, ohfQuerySet,     ParaNdis_OnOidSetMulticastList),
OIDENTRY(OID_FFP_SUPPORT,                       2,4,4, 0                ),
OIDENTRYPROC(OID_TCP_TASK_OFFLOAD,              0,0,0, ohfQuerySet, OnOidSetNdis5Offload),
OIDENTRYPROC(OID_GEN_VLAN_ID,                   0,4,4, ohfQuerySet, ParaNdis_OnSetVlanId),
OIDENTRY(0x00010203 /*(OID_GEN_RECEIVE_SCALE_CAPABILITIES)*/, 2,4,4, 0  ),
OIDENTRY(0x0001021F /*(OID_GEN_RECEIVE_HASH)*/, 2,4,4, 0                ),
OIDENTRY(0,                                     4,4,4, 0),
};

/**********************************************************
Returns to common query processor the array of supported oids
***********************************************************/
void ParaNdis_GetSupportedOid(PVOID *pOidsArray, PULONG pLength)
{
    *pOidsArray     = SupportedOids;
    *pLength        = sizeof(SupportedOids);
}


/*****************************************************************
Handles NDIS5 specific OID, all the rest handled by common handler
*****************************************************************/
static NDIS_STATUS ParaNdis_OidQuery(PARANDIS_ADAPTER *pContext, tOidDesc *pOid)
{
    NDIS_STATUS status;
    BOOLEAN bFreeInfo = FALSE;
    PVOID pInfo = NULL;
    ULONG ulSize = 0;
    ULONG ulLinkSpeed = 0;

    switch(pOid->Oid)
    {
        case OID_TCP_TASK_OFFLOAD:
            status = CreateOffloadInfo5ForQuery(pContext, pOid, &pInfo, &ulSize);
            bFreeInfo = pInfo != NULL;
            break;
        case OID_GEN_LINK_SPEED:
            {
                /* units are 100 bps */
                ulLinkSpeed = (ULONG)(PARANDIS_FORMAL_LINK_SPEED / 100);
                pInfo = &ulLinkSpeed;
                ulSize = sizeof(ulLinkSpeed);
                status = NDIS_STATUS_SUCCESS;
            }
            break;
        default:
            return ParaNdis_OidQueryCommon(pContext, pOid);
    }
    if (status == NDIS_STATUS_SUCCESS)
    {
        status = ParaNdis_OidQueryCopy(pOid, pInfo, ulSize, bFreeInfo);
    }
    else if (bFreeInfo)
    {
        NdisFreeMemory(pInfo, 0, 0);
    }
    return status;
}

/**********************************************************
NDIS required procedure of OID QUERY
Just passes all the supported oids to common query procedure
Return value:
    NDIS_STATUS                 as returned from common code
    NDIS_STATUS_NOT_SUPPORTED   if suppressed in the table
***********************************************************/
NDIS_STATUS NTAPI ParaNdis5_QueryOID(IN NDIS_HANDLE MiniportAdapterContext,
                                    IN NDIS_OID Oid,
                                    IN PVOID InformationBuffer,
                                    IN ULONG InformationBufferLength,
                                    OUT PULONG BytesWritten,
                                    OUT PULONG BytesNeeded)
{
    NDIS_STATUS  status = NDIS_STATUS_NOT_SUPPORTED;
    tOidWhatToDo Rules;
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    tOidDesc _oid;
    ParaNdis_GetOidSupportRules(Oid, &Rules, OidsDB);
    _oid.ulToDoFlags = Rules.Flags;
    *BytesWritten = 0;
    *BytesNeeded  = 0;
    ParaNdis_DebugHistory(pContext, hopOidRequest, NULL, Oid, 0, 1);
    DPrintf(Rules.nEntryLevel, ("[%s], id 0x%X(%s) of %d", __FUNCTION__,
                Oid,
                Rules.name,
                InformationBufferLength));
    _oid.Oid = Oid;
    _oid.InformationBuffer = InformationBuffer;
    _oid.InformationBufferLength = InformationBufferLength;
    _oid.pBytesNeeded = (PUINT)BytesNeeded;
    _oid.pBytesRead   = (PUINT)BytesWritten;
    _oid.pBytesWritten = (PUINT)BytesWritten;
    if (pContext->bSurprizeRemoved) status = NDIS_STATUS_NOT_ACCEPTED;
    else if (Rules.Flags & ohfQuery) status = ParaNdis_OidQuery(pContext, &_oid);


    ParaNdis_DebugHistory(pContext, hopOidRequest, NULL, Oid, status, 0);
    DPrintf((status != NDIS_STATUS_SUCCESS) ? Rules.nExitFailLevel : Rules.nExitOKLevel,
        ("[%s] , id 0x%X(%s) (%X), written %d, needed %d",
        __FUNCTION__,
        Rules.oid,
        Rules.name,
        status,
        *BytesWritten,
        *BytesNeeded));
    return status;

}

/**********************************************************
NDIS required procedure of OID SET
Just passes all the supported oids to common set procedure
Return value:
    NDIS_STATUS                 as returned from set procedure
    NDIS_STATUS_NOT_SUPPORTED   if support not defined in the table
***********************************************************/
NDIS_STATUS NTAPI ParaNdis5_SetOID(IN NDIS_HANDLE MiniportAdapterContext,
                                  IN NDIS_OID Oid,
                                  IN PVOID InformationBuffer,
                                  IN ULONG InformationBufferLength,
                                  OUT PULONG BytesRead,
                                  OUT PULONG BytesNeeded)
{
    NDIS_STATUS  status = NDIS_STATUS_NOT_SUPPORTED;
    tOidWhatToDo Rules;
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    tOidDesc _oid;
    ParaNdis_GetOidSupportRules(Oid, &Rules, OidsDB);
    _oid.ulToDoFlags = Rules.Flags;
    *BytesRead = 0;
    *BytesNeeded = 0;
    ParaNdis_DebugHistory(pContext, hopOidRequest, NULL, Oid, 1, 1);
    DPrintf(Rules.nEntryLevel, ("[%s], id 0x%X(%s) of %d", __FUNCTION__,
                Oid,
                Rules.name,
                InformationBufferLength));
    _oid.Oid = Oid;
    _oid.InformationBuffer = InformationBuffer;
    _oid.InformationBufferLength = InformationBufferLength;
    _oid.pBytesNeeded = (PUINT)BytesNeeded;
    _oid.pBytesRead   = (PUINT)BytesRead;
    _oid.pBytesWritten = (PUINT)BytesRead;
    if (pContext->bSurprizeRemoved) status = NDIS_STATUS_NOT_ACCEPTED;
    else if (Rules.Flags & ohfSet)
    {
        if (Rules.OidSetProc) status = Rules.OidSetProc(pContext, &_oid);
        else
        {
            DPrintf(0, ("[%s] ERROR in OID redirection table", __FUNCTION__));
            status = NDIS_STATUS_INVALID_OID;
        }
    }
    ParaNdis_DebugHistory(pContext, hopOidRequest, NULL, Oid, status, 0);
    if  (status != NDIS_STATUS_PENDING)
    {
        DPrintf((status != NDIS_STATUS_SUCCESS) ? Rules.nExitFailLevel : Rules.nExitOKLevel,
            ("[%s] , id 0x%X(%s) (%X), read %d, needed %d", __FUNCTION__,
            Rules.oid, Rules.name, status, *BytesRead, *BytesNeeded));
    }
    return status;
}

static void NTAPI OnSetPowerWorkItem(NDIS_WORK_ITEM * pWorkItem, PVOID  Context)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    tPowerWorkItem *pwi = (tPowerWorkItem *)pWorkItem;
    PARANDIS_ADAPTER *pContext = pwi->pContext;
    if (pwi->state == (NDIS_DEVICE_POWER_STATE)NetDeviceStateD0)
    {
        status = ParaNdis_PowerOn(pContext);
    }
    else
    {
        ParaNdis_PowerOff(pContext);
    }
    NdisFreeMemory(pwi, 0, 0);
    ParaNdis_DebugHistory(pContext, hopOidRequest, NULL, OID_PNP_SET_POWER, 0, 2);
    NdisMSetInformationComplete(pContext->MiniportHandle, status);
}

/**********************************************************
NDIS5.X handler of power management
***********************************************************/
NDIS_STATUS ParaNdis_OnSetPower(PARANDIS_ADAPTER *pContext, tOidDesc *pOid)
{
    NDIS_STATUS status;
    NDIS_DEVICE_POWER_STATE newState;
    DEBUG_ENTRY(0);
    status = ParaNdis_OidSetCopy(pOid, &newState, sizeof(newState));
    if (status == NDIS_STATUS_SUCCESS)
    {
        tPowerWorkItem *pwi = ParaNdis_AllocateMemory(pContext, sizeof(tPowerWorkItem));
        status = NDIS_STATUS_FAILURE;
        if (pwi)
        {
            pwi->pContext = pContext;
            pwi->state    = newState;
            NdisInitializeWorkItem(&pwi->wi, OnSetPowerWorkItem, pwi);
            if (NdisScheduleWorkItem(&pwi->wi) == NDIS_STATUS_SUCCESS)
            {
                status = NDIS_STATUS_PENDING;
            }
            else
                NdisFreeMemory(pwi, 0, 0);
        }
    }
    return status;
}

/***************************************************
check that the incoming NDIS_TASK_TCP_IP_CHECKSUM
does not enable options which we do not support
***************************************************/
static BOOLEAN IsValidPcs(  PARANDIS_ADAPTER *pContext, NDIS_TASK_TCP_IP_CHECKSUM *pcs)
{
    tOffloadSettingsFlags f;
    BOOLEAN bInvalid = FALSE;
    ParaNdis_ResetOffloadSettings(pContext, &f, NULL);
    bInvalid |= pcs->V4Receive.IpChecksum && !f.fRxIPChecksum;
    bInvalid |= pcs->V4Receive.IpOptionsSupported && !f.fRxIPOptions;
    bInvalid |= pcs->V4Receive.TcpChecksum && !f.fRxTCPChecksum;
    bInvalid |= pcs->V4Receive.TcpOptionsSupported && !f.fRxTCPOptions;
    bInvalid |= pcs->V4Receive.UdpChecksum && !f.fRxUDPChecksum;

    bInvalid |= pcs->V4Transmit.IpChecksum && !f.fTxIPChecksum;
    bInvalid |= pcs->V4Transmit.IpOptionsSupported && !f.fTxIPOptions;
    bInvalid |= pcs->V4Transmit.TcpChecksum && !f.fTxTCPChecksum;
    bInvalid |= pcs->V4Transmit.TcpOptionsSupported && !f.fTxTCPOptions;
    bInvalid |= pcs->V4Transmit.UdpChecksum && !f.fTxUDPChecksum;
    return !bInvalid;
}

/***************************************************
check that the incoming NDIS_TASK_TCP_LARGE_SEND
does not enable options which we do not support
***************************************************/
static BOOLEAN IsValidPls(  PARANDIS_ADAPTER *pContext, NDIS_TASK_TCP_LARGE_SEND *pls)
{
    tOffloadSettingsFlags f;
    BOOLEAN bInvalid = FALSE;
    ParaNdis_ResetOffloadSettings(pContext, &f, NULL);
    bInvalid |= pls->Version != NDIS_TASK_TCP_LARGE_SEND_V0;
    bInvalid |= pls->IpOptions && !f.fTxLsoIP;
    bInvalid |= pls->TcpOptions && !f.fTxLsoTCP;
    bInvalid |= (pls->IpOptions || pls->TcpOptions || pls->MaxOffLoadSize) && !f.fTxLso;
    bInvalid |= pls->MinSegmentCount < PARANDIS_MIN_LSO_SEGMENTS;
    return !bInvalid;
}

static NDIS_STATUS ParseOffloadTask(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN bApply, /* for 'set'*/
    NDIS_TASK_OFFLOAD *pto,
    ULONG offset,
    ULONG maxSize)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    NDIS_TASK_TCP_IP_CHECKSUM *pcs = NULL;
    NDIS_TASK_TCP_LARGE_SEND  *pls = NULL;
    NDIS_TASK_IPSEC *pips = NULL;
    LPCSTR sName = NULL;
    ULONG TaskBufferSize = 0, tailOffset = 0;
    switch(pto->Task)
    {
    case TcpIpChecksumNdisTask:
        pcs = (NDIS_TASK_TCP_IP_CHECKSUM *)pto->TaskBuffer;
        TaskBufferSize = sizeof(*pcs);
        sName = "TcpIpChecksumNdisTask";
        break;
    case TcpLargeSendNdisTask:
        pls = (NDIS_TASK_TCP_LARGE_SEND *)pto->TaskBuffer;
        TaskBufferSize = sizeof(*pls);
        sName = "TcpLargeSendNdisTask";
        break;
    case IpSecNdisTask:
        pips = (NDIS_TASK_IPSEC *)pto->TaskBuffer;
        TaskBufferSize = sizeof(*pips);
        sName = "IpSecNdisTask";
        break;
    default:
        break;
    }
    tailOffset = offset + RtlPointerToOffset(pto, &pto->TaskBuffer) + TaskBufferSize;
    if (!TaskBufferSize)
    {
        DPrintf(0, ("[%s], unknown offload task %d", __FUNCTION__, pto->Task));
    }
    else if (tailOffset > maxSize)
    {
        DPrintf(0, ("[%s], can not parse %s at offset %d, tail at %d", __FUNCTION__, sName, offset, tailOffset));
        status = NDIS_STATUS_BUFFER_TOO_SHORT;
    }
    else if (TaskBufferSize > pto->TaskBufferLength)
    {
        DPrintf(0, ("[%s], invalid size of %s", __FUNCTION__, sName));
        status = NDIS_STATUS_BUFFER_TOO_SHORT;
    }
    else if (pcs)
    {
        DPrintf(0, ("[%s], parsing %s", __FUNCTION__, sName));
        DPrintf(0, ("Rx4: checksum IP(%d),TCP(%d),UDP(%d), options IP(%d),TCP(%d)",
            pcs->V4Receive.IpChecksum, pcs->V4Receive.TcpChecksum, pcs->V4Receive.UdpChecksum,
            pcs->V4Receive.IpOptionsSupported, pcs->V4Receive.TcpOptionsSupported
            ));
        DPrintf(0, ("Tx4: checksum IP(%d),TCP(%d),UDP(%d), options IP(%d),TCP(%d)",
            pcs->V4Transmit.IpChecksum, pcs->V4Transmit.TcpChecksum, pcs->V4Transmit.UdpChecksum,
            pcs->V4Transmit.IpOptionsSupported, pcs->V4Transmit.TcpOptionsSupported
            ));
        if (bApply)
        {
            if (IsValidPcs(pContext, pcs))
            {
                tOffloadSettingsFlags *pf = &pContext->Offload.flags;
                pf->fTxIPChecksum = !!pcs->V4Transmit.IpChecksum;
                pf->fTxTCPChecksum = !!pcs->V4Transmit.TcpChecksum;
                pf->fTxUDPChecksum = !!pcs->V4Transmit.UdpChecksum;
                pf->fTxTCPOptions = !!pcs->V4Transmit.TcpOptionsSupported;
                pf->fTxIPOptions = !!pcs->V4Transmit.IpOptionsSupported;
                pf->fRxIPChecksum = !!pcs->V4Receive.IpChecksum;
                pf->fRxIPOptions = !!pcs->V4Receive.IpOptionsSupported;
                pf->fRxTCPChecksum = !!pcs->V4Receive.TcpChecksum;
                pf->fRxTCPOptions = !!pcs->V4Receive.TcpOptionsSupported;
                pf->fRxUDPChecksum = !!pcs->V4Receive.UdpChecksum;
            }
            else
                status = STATUS_NOT_SUPPORTED;
        }
    }
    else if (pls)
    {
        DPrintf(0, ("[%s], parsing %s version %d", __FUNCTION__, sName, pls->Version));
        DPrintf(0, ("options IP(%d),TCP(%d),MaxOffload %d, MinSegments %d",
            pls->IpOptions, pls->TcpOptions, pls->MaxOffLoadSize, pls->MinSegmentCount));
        if (bApply)
        {
            if (IsValidPls(pContext, pls))
            {
                tOffloadSettingsFlags *pf = &pContext->Offload.flags;
                pf->fTxLsoIP = !!pls->IpOptions;
                pf->fTxLsoTCP = !!pls->TcpOptions;
                pf->fTxLso = 1;
            }
            else
                status = STATUS_NOT_SUPPORTED;
        }
    }
    else if (pips)
    {
        DPrintf(0, ("[%s], parsing %s", __FUNCTION__, sName));
    }
    return status;
}

static FORCEINLINE BOOLEAN ValidateOffloadHeader(NDIS_TASK_OFFLOAD_HEADER *pth)
{
    return
        pth->EncapsulationFormat.Encapsulation == IEEE_802_3_Encapsulation &&
        pth->Version == NDIS_TASK_OFFLOAD_VERSION &&
        pth->Size == sizeof(*pth);
}

static NDIS_STATUS ParseOffload(
    PARANDIS_ADAPTER *pContext,
    NDIS_TASK_OFFLOAD_HEADER *pth,
    ULONG size,
    BOOLEAN bApply,
    PCCHAR reason,
    BOOLEAN headerOnly)
{
    NDIS_STATUS status = NDIS_STATUS_NOT_SUPPORTED;
    BOOLEAN bReset = FALSE;
    ULONG ulNoCapabilities = 0;
    DPrintf(0, ("[%s](%s), format %d", __FUNCTION__, reason,
        pth->EncapsulationFormat.Encapsulation));
    if (ValidateOffloadHeader(pth))
    {
        PUCHAR p = (PUCHAR)pth;
        LONG  offset = (LONG)pth->OffsetFirstTask;
        status = NDIS_STATUS_SUCCESS;
        DPrintf(0, ("[%s], header version %d, ip header at %d, fixed %d, first at %d", __FUNCTION__,
            pth->Version,
            pth->EncapsulationFormat.EncapsulationHeaderSize,
            pth->EncapsulationFormat.Flags.FixedHeaderSize,
            offset));
        if (!offset && bApply)
        {
            /* disable all the capabilities */
            // according to DDK, 0 at first task offset means disabling all the capabilities
            DPrintf(0, ("[%s] RESETTING offload capabilities", __FUNCTION__));
            ParaNdis_ResetOffloadSettings(pContext, NULL, &ulNoCapabilities);
            bReset = TRUE;
        }
        while (!headerOnly && offset > 0 && (offset + sizeof(NDIS_TASK_OFFLOAD)) < size)
        {
            NDIS_TASK_OFFLOAD *pto = (NDIS_TASK_OFFLOAD *)(p + offset);
            if (pto->Version != NDIS_TASK_OFFLOAD_VERSION)
            {
                DPrintf(0, ("[%s], unexpected TO version %d at %d",
                    __FUNCTION__, pto->Version, offset));
                status = NDIS_STATUS_INVALID_DATA;
                break;
            }
            status = ParseOffloadTask(pContext, bApply, pto, offset, size);
            if (!pto->OffsetNextTask || status != NDIS_STATUS_SUCCESS)
                break;
            offset += pto->OffsetNextTask;
        }
    }
    if (status == STATUS_SUCCESS && bApply)
        pContext->Offload.ipHeaderOffset = bReset ? 0: pth->EncapsulationFormat.EncapsulationHeaderSize;
    return status;
}

/********************************************************
Fill offload query structure according to our capabilities
********************************************************/
static BOOLEAN GetTcpIpCheckSumCapabilities(
    PARANDIS_ADAPTER *pContext,
    NDIS_TASK_TCP_IP_CHECKSUM *pcs)
{
    tOffloadSettingsFlags f;
    NdisZeroMemory(pcs, sizeof(*pcs));
    ParaNdis_ResetOffloadSettings(pContext, &f, NULL);
    pcs->V4Transmit.IpChecksum = !!f.fTxIPChecksum;
    pcs->V4Transmit.TcpChecksum = !!f.fTxTCPChecksum;
    pcs->V4Transmit.UdpChecksum = !!f.fTxUDPChecksum;
    pcs->V4Transmit.IpOptionsSupported = !!f.fTxIPOptions;
    pcs->V4Transmit.TcpOptionsSupported = !!f.fTxTCPOptions;
    pcs->V4Receive.IpChecksum = !!f.fRxIPChecksum;
    pcs->V4Receive.IpOptionsSupported = !!f.fRxIPOptions;
    pcs->V4Receive.TcpChecksum = !!f.fRxTCPChecksum;
    pcs->V4Receive.TcpOptionsSupported = !!f.fRxTCPOptions;
    pcs->V4Receive.UdpChecksum = !!f.fRxUDPChecksum;

    return
        pcs->V4Transmit.IpChecksum ||
        pcs->V4Transmit.TcpChecksum ||
        pcs->V4Transmit.UdpChecksum ||
        pcs->V4Receive.IpChecksum ||
        pcs->V4Receive.TcpChecksum ||
        pcs->V4Receive.UdpChecksum;
}

/********************************************************
Fill offload query structure according to our capabilities
********************************************************/
static BOOLEAN GetLargeSendCapabilities(
    PARANDIS_ADAPTER *pContext,
    NDIS_TASK_TCP_LARGE_SEND  *pls)
{
    tOffloadSettingsFlags f;
    NdisZeroMemory(pls, sizeof(*pls));
    ParaNdis_ResetOffloadSettings(pContext, &f, NULL);
    pls->Version = NDIS_TASK_TCP_LARGE_SEND_V0;
    pls->IpOptions = !!f.fTxLsoIP;
    pls->TcpOptions = !!f.fTxLsoTCP;
    pls->MinSegmentCount = PARANDIS_MIN_LSO_SEGMENTS;
    pls->MaxOffLoadSize = pContext->Offload.maxPacketSize;
    return f.fTxLso != 0;
}

/********************************************************
Allocate and fill our capabilities, dependent on registry setting
Note than NDIS test of WLK1.2 and 1.3 fail (offloadmisc)
if CS capability indicated and passes if only LSO indicated
********************************************************/
NDIS_STATUS CreateOffloadInfo5Internal(
    PARANDIS_ADAPTER *pContext,
    PVOID *ppInfo,
    PULONG pulSize,
    PCCHAR reason,
    NDIS_TASK_OFFLOAD_HEADER *pHeader)
{
    NDIS_STATUS status = NDIS_STATUS_RESOURCES;
    ULONG size =
        sizeof(NDIS_TASK_OFFLOAD_HEADER) +
        sizeof(NDIS_TASK_OFFLOAD) + sizeof(NDIS_TASK_TCP_IP_CHECKSUM) +
        sizeof(NDIS_TASK_OFFLOAD) + sizeof(NDIS_TASK_TCP_LARGE_SEND);
    *ppInfo = ParaNdis_AllocateMemory(pContext, size);
    if (*ppInfo)
    {
        ULONG flags = 0;
        NDIS_TASK_TCP_IP_CHECKSUM cs;
        NDIS_TASK_TCP_LARGE_SEND lso;
        flags |= GetTcpIpCheckSumCapabilities(pContext, &cs) ? 2 : 0;
        flags |= GetLargeSendCapabilities(pContext, &lso) ? 1 : 0;
        if (flags)
        {
            NDIS_TASK_OFFLOAD_HEADER *ph;
            NDIS_TASK_OFFLOAD *pto;
            UINT i = 0;
            ULONG *pOffset;
            PVOID base;
            *pulSize = size;
            NdisZeroMemory(*ppInfo, size);
            ph = (NDIS_TASK_OFFLOAD_HEADER *)*ppInfo;
            *ph = *pHeader;
            pto = (NDIS_TASK_OFFLOAD *)(ph + 1);
            base = ph;
            pOffset = &ph->OffsetFirstTask;
            ph->OffsetFirstTask = 0;
            do
            {
                if (flags & (1 << i))
                {
                    flags &= ~(1 << i);
                    pto->Version = NDIS_TASK_OFFLOAD_VERSION;
                    pto->Size = sizeof(*pto);
                    *pOffset = RtlPointerToOffset(base, pto);
                    base = pto;
                    pOffset = &pto->OffsetNextTask;
                    switch(i)
                    {
                        case 1:
                        {
                            NDIS_TASK_TCP_IP_CHECKSUM *pcs = (NDIS_TASK_TCP_IP_CHECKSUM *)pto->TaskBuffer;
                            pto->Task = TcpIpChecksumNdisTask;
                            pto->TaskBufferLength = sizeof(*pcs);
                            NdisMoveMemory(pcs, &cs, sizeof(cs));
                            pto = (NDIS_TASK_OFFLOAD *)(pcs + 1);
                            break;
                        }
                        case 0:
                        {
                            NDIS_TASK_TCP_LARGE_SEND  *pls = (NDIS_TASK_TCP_LARGE_SEND *)pto->TaskBuffer;
                            pto->Task = TcpLargeSendNdisTask;
                            pto->TaskBufferLength = sizeof(*pls);
                            NdisMoveMemory(pls, &lso, sizeof(lso));
                            pto = (NDIS_TASK_OFFLOAD *)(pls + 1);
                            break;
                        }
                        default:
                            break;
                    }
                }
                ++i;
            } while (flags);
            status = ParseOffload(pContext, ph, size, FALSE, reason, FALSE);
        }
        else
        {
            NdisFreeMemory(*ppInfo, 0, 0);
            *ppInfo = NULL;
            status = NDIS_STATUS_NOT_SUPPORTED;
        }
    }
    return status;
}


NDIS_STATUS CreateOffloadInfo5ForQuery(
    PARANDIS_ADAPTER *pContext,
    tOidDesc *pOid,
    PVOID *ppInfo,
    PULONG pulSize)
{
    NDIS_TASK_OFFLOAD_HEADER *pth = (NDIS_TASK_OFFLOAD_HEADER *)pOid->InformationBuffer;
    NDIS_STATUS status;
    *ppInfo  = NULL;
    *pulSize = 0;
    if (pOid->InformationBufferLength < sizeof(*pth)) pth = &ReservedHeader;
    status = ParseOffload(pContext, pth, pOid->InformationBufferLength, FALSE, "query enter", TRUE);
    if (status == NDIS_STATUS_SUCCESS)
    {
        CreateOffloadInfo5Internal(pContext, ppInfo, pulSize, "QUERY", pth);
    }
    return status;
}

NDIS_STATUS OnOidSetNdis5Offload(PARANDIS_ADAPTER *pContext, tOidDesc *pOid)
{
    NDIS_STATUS status;
    status = ParseOffload(pContext, (NDIS_TASK_OFFLOAD_HEADER *)pOid->InformationBuffer,
        pOid->InformationBufferLength, TRUE, "SET", FALSE);
    if (status == STATUS_SUCCESS)
    {
#if 0   // only for logging after SET
        PVOID pInfo = NULL;
        ULONG dummy = 0;
        CreateOffloadInfo5Internal(pContext, &pInfo, &dummy, "UPDATED", &ReservedHeader);
        if (pInfo) NdisFreeMemory(pInfo, 0, 0);
#endif
        *pOid->pBytesRead = pOid->InformationBufferLength;
    }
    else
    {
        DPrintf(0, ("[%s], restoring after unsuccessful set", __FUNCTION__));
        pContext->Offload = pContext->Offload;
    }
    return status;
}
