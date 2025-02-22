/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power management
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
DcDownloadPatternFilter(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PDC_PATTERN_FILTER_BLOCK FilterBlock)
{
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < sizeof(*FilterBlock) / sizeof(ULONG); ++i)
    {
        DC_WRITE(Adapter, DcCsr1_WakeUpFilter, FilterBlock->AsULONG[i]);
    }
}

static
CODE_SEG("PAGE")
VOID
DcSetupWakeUpFilter(
    _In_ PDC21X4_ADAPTER Adapter)
{
    DC_PATTERN_FILTER_BLOCK FilterBlock;

    PAGED_CODE();

    /* Save the address filtering */
    NdisMoveMemory(Adapter->SetupFrameSaved, Adapter->SetupFrame, DC_SETUP_FRAME_SIZE);

    NdisZeroMemory(&FilterBlock, sizeof(FilterBlock));

    // TODO: Convert NDIS patterns to HW filter and prepare a setup frame

    DcDownloadPatternFilter(Adapter, &FilterBlock);
}

static
CODE_SEG("PAGE")
VOID
DcProgramWakeUpEvents(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG WakeUpControl;

    PAGED_CODE();

    /* Clear the wake-up events */
    WakeUpControl = (DC_WAKE_UP_STATUS_LINK_CHANGE |
                     DC_WAKE_UP_STATUS_MAGIC_PACKET |
                     DC_WAKE_UP_CONTROL_PATTERN_MATCH);

    /* Convert NDIS flags to hardware-specific values */
    if (Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_LINK_CHANGE)
        WakeUpControl |= DC_WAKE_UP_CONTROL_LINK_CHANGE;
    if (Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_MAGIC_PACKET)
        WakeUpControl |= DC_WAKE_UP_CONTROL_MAGIC_PACKET;
#if 0 // TODO: Pattern matching is not yet supported
    if (Adapter->WakeUpFlags & NDIS_PNP_WAKE_UP_PATTERN_MATCH)
        WakeUpControl |= DC_WAKE_UP_CONTROL_PATTERN_MATCH;
#endif

    DC_WRITE(Adapter, DcCsr2_WakeUpControl, WakeUpControl);
}

static
CODE_SEG("PAGE")
VOID
DcPowerDown(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG SiaState, SerialInterface;

    PAGED_CODE();

    /* Stop the receive and transmit processes */
    DcStopAdapter(Adapter, FALSE);

    Adapter->CurrentInterruptMask = 0;

    /* Enable the link integrity test bit */
    switch (Adapter->MediaNumber)
    {
        case MEDIA_AUI:
        case MEDIA_BNC:
        case MEDIA_HMR:
        {
            SiaState = DC_READ(Adapter, DcCsr14_SiaTxRx);
            if (!(SiaState & DC_SIA_TXRX_LINK_TEST))
            {
                SiaState |= DC_SIA_TXRX_LINK_TEST;
                DC_WRITE(Adapter, DcCsr14_SiaTxRx, SiaState);
            }
            break;
        }

        default:
            break;
    }

    /* Clear the MDC bit */
    SerialInterface = DC_READ(Adapter, DcCsr9_SerialInterface);
    if (SerialInterface & DC_SERIAL_MII_MDC)
    {
        SerialInterface &= ~DC_SERIAL_MII_MDC;
        DC_WRITE(Adapter, DcCsr9_SerialInterface, SerialInterface);
    }

    /* Unprotect PM access */
    DC_WRITE(Adapter, DcCsr0_BusMode, Adapter->BusMode | DC_BUS_MODE_ON_NOW_UNLOCK);

    /* Program the requested WOL events */
    DcSetupWakeUpFilter(Adapter);
    DcProgramWakeUpEvents(Adapter);

    /* Protect PM access */
    DC_WRITE(Adapter, DcCsr0_BusMode, Adapter->BusMode);
}

static
CODE_SEG("PAGE")
VOID
DcPowerUp(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PAGED_CODE();

    /* Restore the address filtering */
    NdisMoveMemory(Adapter->SetupFrame, Adapter->SetupFrameSaved, DC_SETUP_FRAME_SIZE);

    /* Re-initialize the chip to leave D3 state */
    if (Adapter->PrevPowerState == NdisDeviceStateD3)
    {
        NT_VERIFY(DcSetupAdapter(Adapter) == NDIS_STATUS_SUCCESS);
    }
    else
    {
        /* Start the transmit process */
        Adapter->OpMode |= DC_OPMODE_TX_ENABLE;
        DC_WRITE(Adapter, DcCsr6_OpMode, Adapter->OpMode);

        /* Load the address recognition RAM */
        NT_VERIFY(DcSetupFrameDownload(Adapter, TRUE) == TRUE);
    }

    DcStartAdapter(Adapter);
}

CODE_SEG("PAGE")
VOID
NTAPI
DcPowerWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context)
{
    PDC21X4_ADAPTER Adapter = Context;

    UNREFERENCED_PARAMETER(WorkItem);

    PAGED_CODE();

    if (Adapter->PowerState == NdisDeviceStateD0)
    {
        DcPowerUp(Adapter);
    }
    else
    {
        DcPowerDown(Adapter);
    }
    Adapter->PrevPowerState = Adapter->PowerState;

    NdisMSetInformationComplete(Adapter->AdapterHandle, NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
DcSetPower(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ NDIS_DEVICE_POWER_STATE PowerState)
{
    INFO("Power state %u\n", PowerState);

    Adapter->PowerState = PowerState;

    NdisScheduleWorkItem(&Adapter->PowerWorkItem);

    return NDIS_STATUS_PENDING;
}

NDIS_STATUS
DcRemoveWakeUpPattern(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern)
{
    // TODO: Not implemented
    ERR("FIXME: Not implemented\n");
    return NDIS_STATUS_NOT_SUPPORTED;
}

NDIS_STATUS
DcAddWakeUpPattern(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern)
{
    // TODO: Not implemented
    ERR("FIXME: Not implemented\n");
    return NDIS_STATUS_NOT_SUPPORTED;
}

VOID
DcPowerSave(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN Enable)
{
    ULONG ConfigValue;

    if (!(Adapter->Features & DC_HAS_POWER_SAVING))
        return;

    NdisReadPciSlotInformation(Adapter->AdapterHandle,
                               0,
                               DC_PCI_DEVICE_CONFIG,
                               &ConfigValue,
                               sizeof(ConfigValue));

    ConfigValue &= ~DC_PCI_DEVICE_CONFIG_SLEEP;

    if (Enable)
        ConfigValue |= DC_PCI_DEVICE_CONFIG_SNOOZE;
    else
        ConfigValue &= ~DC_PCI_DEVICE_CONFIG_SNOOZE;

    NdisWritePciSlotInformation(Adapter->AdapterHandle,
                                0,
                                DC_PCI_DEVICE_CONFIG,
                                &ConfigValue,
                                sizeof(ConfigValue));
}
