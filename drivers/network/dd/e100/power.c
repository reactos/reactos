/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Power management
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
EeProgramWakeUpEvents(
    _In_ PE100_ADAPTER Adapter)
{
    PFXP_CB_CONFIGURE Config = &Adapter->ControlBlock->Configuration;

    PAGED_CODE();

    if (Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_LINK_CHANGE)
        Config->LinkStatusChangeWakeEnable = 1;

    if (Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_MAGIC_PACKET)
    {
        ASSERT(Adapter->Flags & E100_FLAG_HAS_WOL);
        Config->MagicPacketWakeDisable = 0;
    }

    EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_CONFIG, &Config->Header);
}

static
CODE_SEG("PAGE")
VOID
EePowerDown(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    EeStopAdapter(Adapter);
    EeSoftReset(Adapter, FALSE);

    EeFlushTransmitQueue(Adapter);

    if (Adapter->WakeUpFlags != 0)
    {
        EePhyPowerSaveDowngradeLinkSpeed(Adapter);
        EeProgramWakeUpEvents(Adapter);
    }
    else
    {
        EePhySetPower(Adapter, FALSE);
    }
}

static
CODE_SEG("PAGE")
VOID
EePowerUp(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    if (Adapter->WakeUpFlags != 0)
    {
        /* Clear wakeup events */
        CSR_WRITE_8(Adapter, FXP_CSR_PMDR, CSR_READ_8(Adapter, FXP_CSR_PMDR));
    }
    else
    {
        EePhySetPower(Adapter, TRUE);
    }

    EeSoftReset(Adapter, FALSE);

    EeSetupAdapter(Adapter);

    /* Restore the RX filter */
    EeApplyPacketFilter(Adapter);

    EeStartAdapter(Adapter);
}

CODE_SEG("PAGE")
VOID
NTAPI
EePowerWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context)
{
    PE100_ADAPTER Adapter = Context;

    UNREFERENCED_PARAMETER(WorkItem);

    PAGED_CODE();

    INFO("Power state %lu\n", Adapter->PowerState);

    ASSERT(Adapter->RevisionID != FXP_REV_82557);

    if (Adapter->PowerState == NdisDeviceStateD0)
    {
        EePowerUp(Adapter);
    }
    else
    {
        EePowerDown(Adapter);
    }
    Adapter->PrevPowerState = Adapter->PowerState;

    NdisMSetInformationComplete(Adapter->AdapterHandle, NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
EeRemoveWakeUpPattern(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern)
{
    ASSERT(Adapter->Flags & E100_FLAG_HAS_WOL);

    // TODO: Not implemented
    ERR("FIXME: Not implemented\n");
    return NDIS_STATUS_NOT_SUPPORTED;
}

NDIS_STATUS
EeAddWakeUpPattern(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern)
{
    ASSERT(Adapter->Flags & E100_FLAG_HAS_WOL);

    // TODO: Not implemented
    ERR("FIXME: Not implemented\n");
    return NDIS_STATUS_NOT_SUPPORTED;
}

NDIS_STATUS
EeGetPowerManagementCapabilities(
    _In_ PE100_ADAPTER Adapter,
    _Out_ PNDIS_PNP_CAPABILITIES Capabilities)
{
    /* The 82557 has no power management support */
    if (Adapter->RevisionID == FXP_REV_82557)
        return NDIS_STATUS_NOT_SUPPORTED;

    if (!(Adapter->Flags & E100_FLAG_HAS_WOL))
    {
        Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
        Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateUnspecified;

        /* The 82559ER can only wake on link status change */
        if (Adapter->RevisionID == FXP_REV_82559S_A)
            Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateD3;
        else
            Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
    }
    else if (Adapter->RevisionID == FXP_REV_82558_A4)
    {
        Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateD1;
        Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateD1;
        Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateUnspecified;
    }
    else
    {
        ASSERT(Adapter->RevisionID >= FXP_REV_82558_B0);

        Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateD3;
        Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateD3;
        Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateD3;
    }

    return NDIS_STATUS_SUCCESS;
}
