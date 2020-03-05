/*
 * This file contains driver-related part of NDIS5.X adapter driver.
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

//#define NO_XP_POWER_MANAGEMENT

#ifdef WPP_EVENT_TRACING
#include "ParaNdis5-Driver.tmh"
#endif

static NDIS_HANDLE      DriverHandle;
static ULONG            gID = 0;

/******************************************************
Unload handler, only responsibility is cleanup WPP
*******************************************************/
static VOID NTAPI ParaVirtualNICUnload(IN  PDRIVER_OBJECT  pDriverObject)
{
    DEBUG_ENTRY(0);
    ParaNdis_DebugCleanup(pDriverObject);
}

/*************************************************************
Required NDIS function
Responsible to put the adapter to known (initial) hardware state

Do not call any NDIS functions
*************************************************************/
static VOID NTAPI ParaNdis5_Shutdown(IN NDIS_HANDLE MiniportAdapterContext)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    ParaNdis_OnShutdown(pContext);
}

/******************************************************
Required NDIS procedure
Allocates and initializes adapter context
Finally sets send and receive to Enabled state and reports connect
Returns:
NDIS_STATUS SUCCESS or some error code
*******************************************************/
static NDIS_STATUS NTAPI ParaNdis5_Initialize(OUT PNDIS_STATUS OpenErrorStatus,
                                    OUT PUINT SelectedMediumIndex,
                                    IN PNDIS_MEDIUM MediumArray,
                                    IN UINT MediumArraySize,
                                    IN NDIS_HANDLE MiniportAdapterHandle,
                                    IN NDIS_HANDLE WrapperConfigurationContext)
{
    NDIS_STATUS  status = NDIS_STATUS_UNSUPPORTED_MEDIA;
    PARANDIS_ADAPTER *pContext = NULL;
    UINT i;
    for(i = 0; i < MediumArraySize; ++i)
    {
        if(MediumArray[i] == NdisMedium802_3)
        {
            *SelectedMediumIndex = i;
            status = NDIS_STATUS_SUCCESS;
            break;
        }
    }

    if (status == NDIS_STATUS_SUCCESS)
    {
        pContext =
            (PARANDIS_ADAPTER *)ParaNdis_AllocateMemory(NULL, sizeof(PARANDIS_ADAPTER));
        if (!pContext)
        {
            status = NDIS_STATUS_RESOURCES;
        }
    }

    if (status == NDIS_STATUS_SUCCESS)
    {
        PVOID pResourceList = &status;
        UINT  uSize = 0;
        NdisZeroMemory(pContext, sizeof(PARANDIS_ADAPTER));
        pContext->ulUniqueID = NdisInterlockedIncrement(&gID);
        pContext->DriverHandle = DriverHandle;
        pContext->MiniportHandle = MiniportAdapterHandle;
        pContext->WrapperConfigurationHandle = WrapperConfigurationContext;
        NdisMQueryAdapterResources(&status, WrapperConfigurationContext, pResourceList, &uSize);
        if (uSize > 0)
            pResourceList = ParaNdis_AllocateMemory(MiniportAdapterHandle, uSize);
        else
            pResourceList = NULL;
        if (!pResourceList)
            status = uSize > 0 ? NDIS_STATUS_RESOURCES : NDIS_STATUS_FAILURE;
        else
        {
            ULONG attributes;
            attributes = NDIS_ATTRIBUTE_DESERIALIZE | NDIS_ATTRIBUTE_BUS_MASTER;
            // in XP SP2, if this flag is NOT set, the NDIS halts miniport
            // upon transition to S1..S4.
            // it seems that XP SP3 ignores it and always sends SET_POWER to D3
#ifndef NO_XP_POWER_MANAGEMENT
            attributes |= NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND;
#endif
            NdisMSetAttributesEx(
                MiniportAdapterHandle,
                pContext,
                0,
                attributes,
                NdisInterfacePci);
            NdisMQueryAdapterResources(&status, WrapperConfigurationContext, pResourceList, &uSize);
            status = ParaNdis_InitializeContext(pContext, (PNDIS_RESOURCE_LIST)pResourceList);
            NdisFreeMemory(pResourceList, 0, 0);
        }
    }

    if (status == NDIS_STATUS_SUCCESS)
    {
        status = ParaNdis_FinishInitialization(pContext);
        if (status == NDIS_STATUS_SUCCESS)
        {
            ParaNdis_DebugRegisterMiniport(pContext, TRUE);
            ParaNdis_IndicateConnect(pContext, FALSE, TRUE);
            ParaNdis5_StopSend(pContext, FALSE, NULL);
            ParaNdis5_StopReceive(pContext, FALSE, NULL);
            if (!pContext->ulMilliesToConnect)
            {
                ParaNdis_ReportLinkStatus(pContext, FALSE);
            }
            else
            {
                NdisSetTimer(&pContext->ConnectTimer, pContext->ulMilliesToConnect);
            }
        }
        else
        {
            ParaNdis_CleanupContext(pContext);
        }
    }

    if (status != NDIS_STATUS_SUCCESS && pContext)
    {
        NdisFreeMemory(pContext, 0, 0);
    }

    DEBUG_EXIT_STATUS(0, status);
    return status;
}


/*************************************************************
Callback of delayed receive pause procedure upon reset request
*************************************************************/
static void OnReceiveStoppedOnReset(VOID *p)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)p;
    DEBUG_ENTRY(0);
    NdisSetEvent(&pContext->ResetEvent);
}

/*************************************************************
Callback of delayed send pause procedure upon reset request
*************************************************************/
static void OnSendStoppedOnReset(VOID *p)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)p;
    DEBUG_ENTRY(0);
    NdisSetEvent(&pContext->ResetEvent);
}

VOID ParaNdis_Suspend(PARANDIS_ADAPTER *pContext)
{
    DEBUG_ENTRY(0);
    NdisResetEvent(&pContext->ResetEvent);
    if (NDIS_STATUS_PENDING != ParaNdis5_StopSend(pContext, TRUE, OnSendStoppedOnReset))
    {
        NdisSetEvent(&pContext->ResetEvent);
    }
    NdisWaitEvent(&pContext->ResetEvent, 0);
    NdisResetEvent(&pContext->ResetEvent);
    if (NDIS_STATUS_PENDING != ParaNdis5_StopReceive(pContext, TRUE, OnReceiveStoppedOnReset))
    {
        NdisSetEvent(&pContext->ResetEvent);
    }
    NdisWaitEvent(&pContext->ResetEvent, 0);
    NdisResetEvent(&pContext->ResetEvent);
    DEBUG_EXIT_STATUS(0, 0);
}

VOID ParaNdis_Resume(PARANDIS_ADAPTER *pContext)
{
    ParaNdis5_StopSend(pContext, FALSE, NULL);
    ParaNdis5_StopReceive(pContext, FALSE, NULL);
    DEBUG_EXIT_STATUS(0, 0);
}

static void NTAPI OnResetWorkItem(NDIS_WORK_ITEM * pWorkItem, PVOID  Context)
{
    tGeneralWorkItem *pwi = (tGeneralWorkItem *)pWorkItem;
    PARANDIS_ADAPTER *pContext = pwi->pContext;
    DEBUG_ENTRY(0);

    pContext->bResetInProgress = TRUE;
    ParaNdis_IndicateConnect(pContext, FALSE, FALSE);
    ParaNdis_Suspend(pContext);
    ParaNdis_Resume(pContext);
    pContext->bResetInProgress = FALSE;
    ParaNdis_ReportLinkStatus(pContext, FALSE);

    NdisFreeMemory(pwi, 0, 0);
    ParaNdis_DebugHistory(pContext, hopSysReset, NULL, 0, NDIS_STATUS_SUCCESS, 0);
    NdisMResetComplete(pContext->MiniportHandle, NDIS_STATUS_SUCCESS, TRUE);
}


/*************************************************************
Required NDIS procedure
Called when some procedure (like OID handler) returns PENDING and
does not complete or when CheckForHang return TRUE
*************************************************************/
static NDIS_STATUS NTAPI ParaNdis5_Reset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext)
{
    NDIS_STATUS status;
    tGeneralWorkItem *pwi;
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    DEBUG_ENTRY(0);
    ParaNdis_DebugHistory(pContext, hopSysReset, NULL, 1, 0, 0);
    status = NDIS_STATUS_FAILURE;
    pwi = ParaNdis_AllocateMemory(pContext, sizeof(tGeneralWorkItem));
    if (pwi)
    {
        pwi->pContext = pContext;
        NdisInitializeWorkItem(&pwi->wi, OnResetWorkItem, pwi);
        if (NdisScheduleWorkItem(&pwi->wi) == NDIS_STATUS_SUCCESS)
        {
            status = NDIS_STATUS_PENDING;
        }
        else
        {
            NdisFreeMemory(pwi, 0, 0);
        }
    }
    if (status != NDIS_STATUS_PENDING)
    {
        ParaNdis_DebugHistory(pContext, hopSysReset, NULL, 0, status, 0);
    }
    return status;
}

/*************************************************************
Callback of delayed receive pause procedure
*************************************************************/
static VOID OnReceiveStopped(VOID *p)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)p;
    DEBUG_ENTRY(0);
    NdisSetEvent(&pContext->HaltEvent);
}

/*************************************************************
Callback of delayed send pause procedure
*************************************************************/
static VOID OnSendStopped(VOID *p)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)p;
    DEBUG_ENTRY(0);
    NdisSetEvent(&pContext->HaltEvent);
}

static void WaitHaltEvent(PARANDIS_ADAPTER *pContext, const char *Reason)
{
    UINT ms = 5000;
    if (!NdisWaitEvent(&pContext->HaltEvent, 1))
    {
        while (!NdisWaitEvent(&pContext->HaltEvent, ms))
        {
            DPrintf(0, ("[%s]", __FUNCTION__));
        }
    }
}

/*************************************************************
Required NDIS procedure
Stops TX and RX path and finished the function of adapter
*************************************************************/
static VOID NTAPI ParaNdis5_Halt(
    IN NDIS_HANDLE MiniportAdapterContext)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    BOOLEAN bUnused;
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    DEBUG_ENTRY(0);

    ParaNdis_DebugHistory(pContext, hopHalt, NULL, 1, 0, 0);

    NdisCancelTimer(&pContext->ConnectTimer, &bUnused);
    NdisResetEvent(&pContext->HaltEvent);
    if (NDIS_STATUS_PENDING != ParaNdis5_StopSend(pContext, TRUE, OnSendStopped))
        NdisSetEvent(&pContext->HaltEvent);
    WaitHaltEvent(pContext, "Send");
    NdisResetEvent(&pContext->HaltEvent);
    if (NDIS_STATUS_PENDING != ParaNdis5_StopReceive(pContext, TRUE, OnReceiveStopped))
        NdisSetEvent(&pContext->HaltEvent);
    WaitHaltEvent(pContext, "Receive");
    ParaNdis_CleanupContext(pContext);
    NdisCancelTimer(&pContext->DPCPostProcessTimer, &bUnused);
    ParaNdis_DebugHistory(pContext, hopHalt, NULL, 0, 0, 0);
    ParaNdis_DebugRegisterMiniport(pContext, FALSE);
    NdisFreeMemory(pContext, 0, 0);
    DEBUG_EXIT_STATUS(0, status);
}


/*************************************************************
Called periodically (usually each 2 seconds)
*************************************************************/
static BOOLEAN NTAPI ParaNdis5_CheckForHang(IN NDIS_HANDLE MiniportAdapterContext)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    DEBUG_ENTRY(8);
    return ParaNdis_CheckForHang(pContext);
}

/*************************************************************
Required NDIS procedure
Responsible for hardware interrupt handling
*************************************************************/
static VOID NTAPI ParaNdis5_MiniportISR(OUT PBOOLEAN InterruptRecognized,
                               OUT PBOOLEAN QueueMiniportHandleInterrupt,
                               IN NDIS_HANDLE  MiniportAdapterContext)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    BOOLEAN b;
    *QueueMiniportHandleInterrupt = FALSE;
    b = ParaNdis_OnLegacyInterrupt(pContext, QueueMiniportHandleInterrupt);
    *InterruptRecognized = b;
    DEBUG_EXIT_STATUS(7, (ULONG)b);
}

/*************************************************************
Parameters:

Return value:

*************************************************************/
VOID NTAPI ParaNdis5_PnPEventNotify(IN NDIS_HANDLE MiniportAdapterContext,
                                  IN NDIS_DEVICE_PNP_EVENT PnPEvent,
                                  IN PVOID InformationBuffer,
                                  IN ULONG InformationBufferLength)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    ParaNdis_OnPnPEvent(pContext, PnPEvent, InformationBuffer, InformationBufferLength);
}

/*************************************************************
Driver's entry point
Parameters:
    as usual
Return value:
    SUCCESS or error code
*************************************************************/
NDIS_STATUS NTAPI DriverEntry(PVOID DriverObject,PVOID RegistryPath)
{
    NDIS_STATUS status;
    NDIS_MINIPORT_CHARACTERISTICS chars;
    ParaNdis_DebugInitialize(DriverObject, RegistryPath);

    status = NDIS_STATUS_FAILURE;

    DEBUG_ENTRY(0);
    _LogOutString(0, __DATE__ " " __TIME__);

    NdisMInitializeWrapper(&DriverHandle,
                           DriverObject,
                           RegistryPath,
                           NULL
                           );

    if (DriverHandle)
    {
        NdisZeroMemory(&chars, sizeof(chars));
        //NDIS version of the miniport
        chars.MajorNdisVersion          = NDIS_MINIPORT_MAJOR_VERSION;
        chars.MinorNdisVersion          = NDIS_MINIPORT_MINOR_VERSION;
        //Init and destruction
        chars.InitializeHandler         = ParaNdis5_Initialize;
        chars.HaltHandler               = ParaNdis5_Halt;

        //Interrupt and DPC handling
        chars.HandleInterruptHandler    = ParaNdis5_HandleDPC;
        chars.ISRHandler                = ParaNdis5_MiniportISR;

        //Packet transfer - send path and notification on the send packet
        chars.SendPacketsHandler        = ParaNdis5_SendPackets;
        chars.ReturnPacketHandler       = ParaNdis5_ReturnPacket;

        //OID set\get
        chars.SetInformationHandler     = ParaNdis5_SetOID;
        chars.QueryInformationHandler   = ParaNdis5_QueryOID;

        //Reset
        chars.ResetHandler              = ParaNdis5_Reset;
        chars.CheckForHangHandler       = ParaNdis5_CheckForHang; //optional

        chars.CancelSendPacketsHandler  = ParaNdis5_CancelSendPackets;
        chars.PnPEventNotifyHandler     = ParaNdis5_PnPEventNotify;
        chars.AdapterShutdownHandler    = ParaNdis5_Shutdown;

        status = NdisMRegisterMiniport(
            DriverHandle,
            &chars,
            sizeof(chars));
    }

    if (status == NDIS_STATUS_SUCCESS)
    {
        NdisMRegisterUnloadHandler(DriverHandle, ParaVirtualNICUnload);
    }
    else if (DriverHandle)
    {
        DPrintf(0, ("NdisMRegisterMiniport failed"));
        NdisTerminateWrapper(DriverHandle, NULL);
    }
    else
    {
        DPrintf(0, ("NdisMInitializeWrapper failed"));
    }

    DEBUG_EXIT_STATUS(status ? 0 : 4, status);
    return status;
}
