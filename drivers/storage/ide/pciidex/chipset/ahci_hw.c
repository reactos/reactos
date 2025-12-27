/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI hardware support
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * The PMP configuration code derives from the FreeBSD ATA driver
 * Copyright (c) 2009 Alexander Motin <mav@FreeBSD.org>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
AtaAhciPollRegister(
    _In_ PVOID IoBase,
    _In_ AHCI_PORT_REGISTER Register,
    _In_ ULONG Mask,
    _In_ ULONG Value,
    _In_range_(>, 0) ULONG TimeOut)
{
    ULONG i, Data;

    ASSUME(TimeOut > 0);

    /* Do a quick check first (100 us) */
    for (i = 0; i < 10; ++i)
    {
        Data = AHCI_PORT_READ(IoBase, Register);
        if ((Data & Mask) == Value)
            return TRUE;

        KeStallExecutionProcessor(10);
    }

    /* Retry after the time interval */
    for (i = 0; i < TimeOut; i++)
    {
        Data = AHCI_PORT_READ(IoBase, Register);
        if ((Data & Mask) == Value)
            return TRUE;

        AtaSleep();
    }

    return FALSE;
}

VOID
AtaAhciEnableInterrupts(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Enable)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;

    INFO("CH %lu: %sble interrupts\n", ChanData->Channel, Enable ? "Ena" : "Disa");

    /* Clear port interrupts */
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataError, 0xFFFFFFFF);
    AHCI_PORT_WRITE(ChanData->IoBase, PxInterruptStatus, 0xFFFFFFFF);
    AHCI_HBA_WRITE(ChanData->Controller->IoBase, HbaInterruptStatus, 1 << ChanData->Channel);

    AHCI_PORT_WRITE(ChanData->IoBase, PxInterruptEnable, Enable ? AHCI_PORT_INTERRUPT_MASK : 0);
}

static
BOOLEAN
AtaAhciIsHbaHotRemoved(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    PATA_CONTROLLER Controller = ChanData->Controller;
    BOOLEAN WasRemoved;

    /* Check if the HBA has been hot-removed to avoid timeouts on all ports */
    WasRemoved = (AHCI_HBA_READ(Controller->IoBase, HbaAhciVersion) == 0xFFFFFFFF);
    if (WasRemoved)
    {
        ERR("CH %lu: AHCI controller %04X:%04X is gone\n",
            ChanData->Channel,
            Controller->Pci.VendorID,
            Controller->Pci.DeviceID);
    }
    return WasRemoved;
}

static
VOID
AtaAhciSendComReset(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG SataControl;

    INFO("CH %lu: Transmit a COMRESET on the interface\n", ChanData->Channel);

    /* Clear errors */
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataError, 0xFFFFFFFF);

    SataControl = AHCI_PORT_READ(ChanData->IoBase, PxSataControl);
    SataControl &= ~AHCI_PXCTL_DET_MASK;
    SataControl |= AHCI_PXCTL_DET_RESET;
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataControl, SataControl);

    KeStallExecutionProcessor(1000);

    SataControl = AHCI_PORT_READ(ChanData->IoBase, PxSataControl);
    SataControl &= ~AHCI_PXCTL_DET_MASK;
    SataControl |= AHCI_PXCTL_DET_IDLE;
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataControl, SataControl);

    /* Clear errors */
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataError, 0xFFFFFFFF);
}

static
VOID
AtaAhciStopCommandListProcess(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    if (CmdStatus & AHCI_PXCMD_ST)
    {
        CmdStatus &= ~AHCI_PXCMD_ST;
        AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);
    }
}

static
VOID
AtaAhciStartCommandListProcess(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);

    /*
     * We are supposed to wait for the AHCI_PXCMD_CR bit to be set
     * before interacting with the port again,
     * but on some AHCI controllers this bit never becomes set.
     */
    KeStallExecutionProcessor(50);
}

static
BOOLEAN
AtaAhciStopCommandListProcessAndWait(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    AtaAhciStopCommandListProcess(ChanData);

    if (AtaAhciIsHbaHotRemoved(ChanData))
        return FALSE;

    return AtaAhciPollRegister(ChanData->IoBase,
                               PxCmdStatus,
                               AHCI_PXCMD_CR,
                               0,
                               AHCI_DELAY_CR_START_STOP);
}

static
VOID
AtaAhciStopFisReceiveProcess(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    if (CmdStatus & AHCI_PXCMD_FRE)
    {
        CmdStatus &= ~AHCI_PXCMD_FRE;
        AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);
    }
}

static
VOID
AtaAhciStartFisReceiveProcess(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_FRE;
    AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);
}

static
BOOLEAN
AtaAhciStopFisReceiveProcessAndWait(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    AtaAhciStopFisReceiveProcess(ChanData);

    if (AtaAhciIsHbaHotRemoved(ChanData))
        return FALSE;

    return AtaAhciPollRegister(ChanData->IoBase,
                               PxCmdStatus,
                               AHCI_PXCMD_FR,
                               0,
                               AHCI_DELAY_FR_START_STOP);
}

static
VOID
AtaAhciStartFisReceiveProcessAndWait(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    AtaAhciStartFisReceiveProcess(ChanData);

    if (!AtaAhciPollRegister(ChanData->IoBase,
                             PxCmdStatus,
                             AHCI_PXCMD_FR,
                             AHCI_PXCMD_FR,
                             AHCI_DELAY_FR_START_STOP))
    {
        /* Ignore timeouts, on some AHCI controllers the FR bit never becomes set */
        WARN("CH %lx: Failed to start the FIS Receive DMA engine %08lx\n",
             ChanData->Channel,
             AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus));
    }
}

static
VOID
AtaAhciPhyEnterListenMode(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CmdStatus, SataControl;

    if (!(ChanData->Controller->AhciCapabilities & AHCI_CAP_SSS))
        return;

    INFO("CH %lu: Enter listen mode\n", ChanData->Channel);

    AtaAhciStopCommandListProcessAndWait(ChanData);

    SataControl = AHCI_PORT_READ(ChanData->IoBase, PxSataControl);
    SataControl &= ~AHCI_PXCTL_DET_MASK;
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataControl, SataControl);

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    CmdStatus &= ~(AHCI_PXCMD_SUD | AHCI_PXCMD_ICC_MASK);
    AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);
}

static
BOOLEAN
AtaAhciPerformCommandListOverride(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CmdStatus;

    if (!(ChanData->Controller->AhciCapabilities & AHCI_CAP_SCLO))
      return TRUE;

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_CLO;
    AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);

    return AtaAhciPollRegister(ChanData->IoBase,
                               PxCmdStatus,
                               AHCI_PXCMD_CLO,
                               0,
                               AHCI_DELAY_CLO_CLEAR);
}

static
VOID
AtaAhciAtapiLedControl(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ BOOLEAN DoEnable)
{
    ULONG CmdStatus, NewCmdStatus;

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    if (DoEnable)
        NewCmdStatus = CmdStatus | AHCI_PXCMD_ATAPI;
    else
        NewCmdStatus = CmdStatus & ~AHCI_PXCMD_ATAPI;

    if (CmdStatus != NewCmdStatus)
        AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, NewCmdStatus);
}

static
VOID
AtaAhciFbsControl(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ BOOLEAN DoEnable)
{
    ULONG FbsControl, NewFbsControl;

    if (!(ChanData->ChanInfo & CHANNEL_FLAG_HAS_FBS))
        return;

    ChanData->LastFbsDeviceNumber = 0xFF;

    FbsControl = AHCI_PORT_READ(ChanData->IoBase, PxFisSwitchingControl);
    if (DoEnable)
    {
        NewFbsControl = FbsControl | AHCI_FBS_ENABLE;
    }
    else
    {
        NewFbsControl = FbsControl & ~AHCI_FBS_ENABLE;
        ChanData->ChanInfo &= ~CHANNEL_FLAG_FBS_ENABLED;
    }

    if ((FbsControl & NewFbsControl) ^ AHCI_FBS_ENABLE)
    {
        AHCI_PORT_WRITE(ChanData->IoBase, PxFisSwitchingControl, NewFbsControl);
    }

    if (!DoEnable)
        return;

    /* Make sure we can read back the new value */
    FbsControl = AHCI_PORT_READ(ChanData->IoBase, PxFisSwitchingControl);
    if (FbsControl & AHCI_FBS_ENABLE)
    {
        ChanData->ChanInfo |= CHANNEL_FLAG_FBS_ENABLED;
        INFO("CH %lu: FBS enabled\n", ChanData->Channel);
    }
    else
    {
        WARN("CH %lu: Unable to enable FIS-based switching\n", ChanData->Channel);
    }
}

static
BOOLEAN
AtaAhciEnterIdleState(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);

    /* Already in an idle state */
    if (!(CmdStatus & (AHCI_PXCMD_ST | AHCI_PXCMD_CR | AHCI_PXCMD_FRE | AHCI_PXCMD_FR)))
        return TRUE;

    /* Stop the command list DMA engine */
    if (!AtaAhciStopCommandListProcessAndWait(ChanData))
    {
        WARN("CH %lu: Failed to stop the command list DMA engine %08lx\n",
             ChanData->Channel,
             AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus));
        return FALSE;
    }

    AtaAhciPerformCommandListOverride(ChanData);

    /* Stop the FIS Receive DMA engine */
    if (!AtaAhciStopFisReceiveProcessAndWait(ChanData))
    {
        WARN("CH %lu: Failed to stop the FIS Receive DMA engine %08lx\n",
             ChanData->Channel,
             AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus));
        return FALSE;
    }

    return TRUE;
}

static
VOID
AtaAhciSetupDmaMemoryAddress(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    /* Physical address of the allocated command list */
    AHCI_PORT_WRITE(ChanData->IoBase, PxCommandListBaseLow, (ULONG)ChanData->Mem.CommandListPhys);
    if (ChanData->Controller->AhciCapabilities & AHCI_CAP_S64A)
    {
        AHCI_PORT_WRITE(ChanData->IoBase,
                        PxCommandListBaseHigh,
                        (ULONG)(ChanData->Mem.CommandListPhys >> 32));
    }

    /* Physical address of the allocated FIS receive area */
    AHCI_PORT_WRITE(ChanData->IoBase, PxFisBaseLow, (ULONG)ChanData->Mem.ReceivedFisPhys);
    if (ChanData->Controller->AhciCapabilities & AHCI_CAP_S64A)
    {
        AHCI_PORT_WRITE(ChanData->IoBase,
                        PxFisBaseHigh,
                        (ULONG)(ChanData->Mem.ReceivedFisPhys >> 32));
    }
}

static
VOID
AtaAhciSpinUp(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);

    /* Clear the PMA bit once the DMA engine has been stopped */
    CmdStatus &= ~AHCI_PXCMD_PMA;

    /* Move to the active interface state for the DET value be accurate */
    CmdStatus &= ~AHCI_PXCMD_ICC_MASK;
    CmdStatus |= AHCI_PXCMD_ICC_ACTIVE;

    AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);

    AtaAhciPollRegister(ChanData->IoBase,
                        PxCmdStatus,
                        AHCI_PXCMD_ICC_MASK,
                        AHCI_PXCMD_ICC_IDLE,
                        AHCI_DELAY_INTERFACE_CHANGE);

    /* Spin-up and power up the device */
    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    if (ChanData->Controller->AhciCapabilities & AHCI_CAP_SSS)
    {
        CmdStatus |= AHCI_PXCMD_SUD;
    }
    if (CmdStatus & AHCI_PXCMD_CPD)
    {
        CmdStatus |= AHCI_PXCMD_POD;
    }
    AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);
}

static
BOOLEAN
AtaAhciPhyCheckDevicePresence(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG i, SataStatus;

    /* Do a quick check first (100 us) */
    for (i = 0; i < 10; ++i)
    {
        SataStatus = AHCI_PORT_READ(ChanData->IoBase, PxSataStatus) & AHCI_PXSSTS_DET_MASK;
        if (SataStatus != AHCI_PXSSTS_DET_NO_DEVICE)
            return TRUE;

        KeStallExecutionProcessor(10);
    }

    /* Retry after the time interval */
    for (i = 0; i < AHCI_DELAY_DET_PRESENCE; ++i)
    {
        SataStatus = AHCI_PORT_READ(ChanData->IoBase, PxSataStatus) & AHCI_PXSSTS_DET_MASK;
        if (SataStatus != AHCI_PXSSTS_DET_NO_DEVICE)
            return TRUE;

        AtaSleep();
    }

    return FALSE;
}

static
BOOLEAN
AtaAhciPhyWaitForReady(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    return AtaAhciPollRegister(ChanData->IoBase,
                               PxSataStatus,
                               AHCI_PXSSTS_DET_MASK,
                               AHCI_PXSSTS_DET_PHY_OK,
                               AHCI_DELAY_DET_STABLE);
}

static
BOOLEAN
AtaAhciWaitForDeviceReady(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG TimeOut)
{
    ULONG i, TaskFileData;

    /* Do a quick check first (100 us) */
    for (i = 0; i < 10; ++i)
    {
        TaskFileData = AHCI_PORT_READ(ChanData->IoBase, PxTaskFileData);
        if (!(TaskFileData & (IDE_STATUS_BUSY | IDE_STATUS_DRQ)))
            return TRUE;

        KeStallExecutionProcessor(10);
    }

    /* Retry after the time interval */
    for (i = 0; i < TimeOut; ++i)
    {
        TaskFileData = AHCI_PORT_READ(ChanData->IoBase, PxTaskFileData);
        if (!(TaskFileData & (IDE_STATUS_BUSY | IDE_STATUS_DRQ)))
            return TRUE;

        /* Keep clearing the port errors after every 1 second interval */
        if (((i % AHCI_DELAY_1_SECOND) == 0) && (i != 0))
        {
            if (AtaAhciIsHbaHotRemoved(ChanData))
                return FALSE;

            WARN("CH %lu: Device is busy %08lx, trying to recover %lu\n",
                 ChanData->Channel,
                 TaskFileData,
                 i);

            AHCI_PORT_WRITE(ChanData->IoBase, PxSataError, 0xFFFFFFFF);
        }

        AtaSleep();
    }

    return FALSE;
}

static
UCHAR
AtaAhciPostRequestPolled(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_range_(>, 0) ULONG TimeOutMs)
{
    ULONG InterruptStatus, TaskFileData;
    UCHAR SrbStatus;

    ASSERT((ChanData->ActiveSlotsBitmap == 0) && (ChanData->ActiveQueuedSlotsBitmap == 0));

    AHCI_PORT_WRITE(ChanData->IoBase, PxSataError, 0xFFFFFFFF);

    AHCI_PORT_WRITE(ChanData->IoBase, PxCommandIssue, 1 << AHCI_INTERNAL_SLOT);
    if (!AtaAhciPollRegister(ChanData->IoBase,
                             PxCommandIssue,
                             1 << AHCI_INTERNAL_SLOT,
                             0,
                             TimeOutMs / PORT_TIMER_TICK_MS))
    {
        ERR("CH %lu: Internal request timed out\n", ChanData->Channel);

        /* Clear the active DMA command */
        AtaAhciStopCommandListProcessAndWait(ChanData);
        AtaAhciPerformCommandListOverride(ChanData);
        AtaAhciStartCommandListProcess(ChanData);

        SrbStatus = SRB_STATUS_TIMEOUT;
        goto Done;
    }

    InterruptStatus = AHCI_PORT_READ(ChanData->IoBase, PxInterruptStatus);

    /* Minimal basic recovery for port */
    if (InterruptStatus & AHCI_PXIRQ_FATAL_ERROR)
    {
        if (!AtaAhciStopCommandListProcessAndWait(ChanData))
        {
            ERR("CH %lu: Failed to stop the command list DMA engine\n", ChanData->Channel);
            SrbStatus = SRB_STATUS_TIMEOUT;
            goto Done;
        }

        TaskFileData = AHCI_PORT_READ(ChanData->IoBase, PxTaskFileData);
        if (TaskFileData & (IDE_STATUS_BUSY | IDE_STATUS_DRQ))
        {
            ERR("CH %lu: Busy TFD %08lx\n", ChanData->Channel, TaskFileData);
            SrbStatus = SRB_STATUS_TIMEOUT;
            goto Done;
        }

        AHCI_PORT_WRITE(ChanData->IoBase, PxSataError,
                        AHCI_PORT_READ(ChanData->IoBase, PxSataError));

        AtaAhciStartCommandListProcess(ChanData);

        SrbStatus = SRB_STATUS_ERROR;
        goto Done;
    }

    SrbStatus = SRB_STATUS_SUCCESS;

Done:
    if (SrbStatus == SRB_STATUS_SUCCESS)
        TRACE("CH %lu: Completed internal request\n", ChanData->Channel);
    else
        INFO("CH %lu: Internal request failed %02x\n", ChanData->Channel, SrbStatus);

    /* Clear port interrupts */
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataError, 0xFFFFFFFF);
    AHCI_PORT_WRITE(ChanData->IoBase, PxInterruptStatus, 0xFFFFFFFF);
    AHCI_HBA_WRITE(ChanData->Controller->IoBase, HbaInterruptStatus, 1 << ChanData->Channel);

    return SrbStatus;
}

static
UCHAR
AtaAhciSendResetFis(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ UCHAR PortNumber,
    _In_ BOOLEAN AssertSRST,
    _In_range_(>, 0) ULONG TimeOutMs)
{
    PAHCI_COMMAND_TABLE CommandTable = ChanData->CommandTable[AHCI_INTERNAL_SLOT];
    PAHCI_COMMAND_HEADER CommandHeader = &ChanData->CommandList->CommandHeader[AHCI_INTERNAL_SLOT];
    PAHCI_FIS_HOST_TO_DEVICE H2dFis = &CommandTable->HostToDeviceFis;

    RtlZeroMemory(H2dFis, sizeof(*H2dFis));
    H2dFis->Type = AHCI_FIS_REGISTER_HOST_TO_DEVICE;
    H2dFis->Flags = PortNumber;
    H2dFis->Control = IDE_DC_ALWAYS;

    CommandHeader->PrdByteCount = 0;
    CommandHeader->Control = (sizeof(*H2dFis) / sizeof(ULONG)) |
                             (PortNumber << AHCI_COMMAND_HEADER_PMP_SHIFT);

    if (AssertSRST)
    {
        H2dFis->Control |= IDE_DC_RESET_CONTROLLER;
        CommandHeader->Control |= AHCI_COMMAND_HEADER_RESET |
                                  AHCI_COMMAND_HEADER_CLEAR_BUSY_UPON_OK;
    }

    return AtaAhciPostRequestPolled(ChanData, TimeOutMs);
}

static
UCHAR
AtaAhciPmpRead(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ UCHAR PortNumber,
    _In_ USHORT Register,
    _Out_ PULONG Result)
{
    PAHCI_COMMAND_TABLE CommandTable = ChanData->CommandTable[AHCI_INTERNAL_SLOT];
    PAHCI_COMMAND_HEADER CommandHeader = &ChanData->CommandList->CommandHeader[AHCI_INTERNAL_SLOT];
    PAHCI_FIS_HOST_TO_DEVICE H2dFis = &CommandTable->HostToDeviceFis;
    PAHCI_FIS_DEVICE_TO_HOST D2hFis = &ChanData->ReceivedFis->DeviceToHostFis;
    UCHAR SrbStatus;

    RtlZeroMemory(H2dFis, sizeof(*H2dFis));
    H2dFis->Type = AHCI_FIS_REGISTER_HOST_TO_DEVICE;
    H2dFis->Flags = UPDATE_COMMAND | AHCI_PMP_CONTROL_PORT;
    H2dFis->Control = IDE_DC_ALWAYS;
    H2dFis->Device = IDE_DRIVE_SELECT | PortNumber;
    H2dFis->Features = (UCHAR)Register;
    H2dFis->FeaturesEx = (UCHAR)(Register >> 8);
    H2dFis->Command = IDE_COMMAND_READ_PORT_MULTIPLIER;

    CommandHeader->PrdByteCount = 0;
    CommandHeader->Control = (sizeof(*H2dFis) / sizeof(ULONG)) |
                             (AHCI_PMP_CONTROL_PORT << AHCI_COMMAND_HEADER_PMP_SHIFT);

    SrbStatus = AtaAhciPostRequestPolled(ChanData, 1000);

    *Result = D2hFis->SectorCount |
              (D2hFis->LbaLow << 8) |
              (D2hFis->LbaMid << 16) |
              (D2hFis->LbaHigh << 24);

    return SrbStatus;
}

static
UCHAR
AtaAhciPmpWrite(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ UCHAR PortNumber,
    _In_ USHORT Register,
    _In_ ULONG Value)
{
    PAHCI_COMMAND_TABLE CommandTable = ChanData->CommandTable[AHCI_INTERNAL_SLOT];
    PAHCI_COMMAND_HEADER CommandHeader = &ChanData->CommandList->CommandHeader[AHCI_INTERNAL_SLOT];
    PAHCI_FIS_HOST_TO_DEVICE H2dFis = &CommandTable->HostToDeviceFis;

    RtlZeroMemory(H2dFis, sizeof(*H2dFis));
    H2dFis->Type = AHCI_FIS_REGISTER_HOST_TO_DEVICE;
    H2dFis->Flags = UPDATE_COMMAND | AHCI_PMP_CONTROL_PORT;
    H2dFis->Control = IDE_DC_ALWAYS;
    H2dFis->Device = IDE_DRIVE_SELECT | PortNumber;
    H2dFis->Features = (UCHAR)Register;
    H2dFis->FeaturesEx = (UCHAR)(Register >> 8);
    H2dFis->SectorCount = (UCHAR)Value;
    H2dFis->LbaLow = (UCHAR)(Value >> 8);
    H2dFis->LbaMid = (UCHAR)(Value >> 16);
    H2dFis->LbaHigh = (UCHAR)(Value >> 24);
    H2dFis->Command = IDE_COMMAND_WRITE_PORT_MULTIPLIER;

    CommandHeader->PrdByteCount = 0;
    CommandHeader->Control = (sizeof(*H2dFis) / sizeof(ULONG)) |
                             (AHCI_PMP_CONTROL_PORT << AHCI_COMMAND_HEADER_PMP_SHIFT);

    return AtaAhciPostRequestPolled(ChanData, 1000);
}

static
BOOLEAN
AtaAhciPmpDisableSilXmitEarlyAck(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    UCHAR SrbStatus;
    ULONG Value;

    SrbStatus = AtaAhciPmpRead(ChanData, AHCI_PMP_CONTROL_PORT, 129, &Value);
    if (SrbStatus != SRB_STATUS_SUCCESS)
        return FALSE;

    if (Value & 1)
    {
        Value &= ~1;
        SrbStatus = AtaAhciPmpWrite(ChanData, AHCI_PMP_CONTROL_PORT, 129, Value);
        if (SrbStatus != SRB_STATUS_SUCCESS)
            return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
AtaAhciPmpIdentifyPmp(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _Out_ PULONG PortCountResult)
{
    ULONG ProductId, RevisionInfo, PortCount;
    UCHAR SrbStatus;

    *PortCountResult = 0;

    SrbStatus = AtaAhciPmpRead(ChanData, AHCI_PMP_CONTROL_PORT, PmpProductId, &ProductId);
    if (SrbStatus != SRB_STATUS_SUCCESS)
        return FALSE;

    SrbStatus = AtaAhciPmpRead(ChanData, AHCI_PMP_CONTROL_PORT, PmpRevisionInfo, &RevisionInfo);
    if (SrbStatus != SRB_STATUS_SUCCESS)
        return FALSE;

    SrbStatus = AtaAhciPmpRead(ChanData, AHCI_PMP_CONTROL_PORT, PmpPortInfo, &PortCount);
    if (SrbStatus != SRB_STATUS_SUCCESS)
        return FALSE;

    INFO("CH %lu: PMP %08lX:%08lX %lu ports\n",
         ChanData->Channel,
         ProductId,
         RevisionInfo,
         PortCount);

    PortCount &= AHCI_MAX_PMP_DEVICES;

    /* From FreeBSD: Hide pseudo ATA devices from the driver */
    switch (ProductId)
    {
        case 0x37261095:
        case 0x38261095:
        {
            if (!AtaAhciPmpDisableSilXmitEarlyAck(ChanData))
                return FALSE;

            if (PortCount == 6)
                PortCount = 5;
            break;
        }

        case 0x47261095:
            if (PortCount == 7)
                PortCount = 5;
            break;

        case 0x57231095:
        case 0x57331095:
        case 0x57341095:
        case 0x57441095:
            if (PortCount > 0)
                --PortCount;
            break;

        default:
            break;
    }
    *PortCountResult = PortCount;

    return TRUE;
}

static
ATA_CONNECTION_STATUS
AtaAhciPmpDetect(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ PBOOLEAN CheckForPmp)
{
    ULONG CmdStatus, Signature;
    UCHAR SrbStatus;

    /* Indicate a PMP */
    CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_PMA;
    AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);

    /* Turn off FIS-based switching prior to issuing the software reset */
    AtaAhciFbsControl(ChanData, FALSE);

    /* Start the command list DMA engine */
    AtaAhciPerformCommandListOverride(ChanData);
    AtaAhciStartCommandListProcess(ChanData);

    /* BSY and DRQ must be cleared prior to issuing the software reset */
    if (!AtaAhciWaitForDeviceReady(ChanData, AHCI_DELAY_PMP_READY_DRIVE))
    {
        WARN("CH %lu: Wait for ready failed %08lx\n",
             ChanData->Channel,
             AHCI_PORT_READ(ChanData->IoBase, PxTaskFileData));

        /* Transmit a COMRESET to recover */
        AtaAhciSendComReset(ChanData);

        return CONN_STATUS_FAILURE;
    }

    /*
     * Issue a software reset.
     *
     * Real PMP devices come up faster than ATA/ATAPI devices,
     * keep the timeout as small as possible.
     */
    SrbStatus = AtaAhciSendResetFis(ChanData, AHCI_PMP_CONTROL_PORT, TRUE, 1000);
    if (SrbStatus != SRB_STATUS_SUCCESS)
    {
        INFO("CH %lu: Soft reset failed %02x\n", ChanData->Channel, SrbStatus);

        /*
         * This command may timeout if a PMP-capable port indicates device presence
         * and the connected device is not a port multiplier.
         * As a result, the system boot time may increase by (ActivePortCount * TimeOutMs).
         * We skip PMP detection upon the first timeout, there is nothing more that can be done.
         */
        if (SrbStatus == SRB_STATUS_TIMEOUT)
        {
            /* Not a PMP device */
            *CheckForPmp = FALSE;
            INFO("CH %lu: Seems to have no PMP device\n", ChanData->Channel);
        }
        return CONN_STATUS_FAILURE;
    }

    /* SRST pulse width */
    KeStallExecutionProcessor(20);

    SrbStatus = AtaAhciSendResetFis(ChanData, AHCI_PMP_CONTROL_PORT, FALSE, 3000);
    if (SrbStatus != SRB_STATUS_SUCCESS)
    {
        INFO("CH %lu: Soft reset failed %02x\n", ChanData->Channel, SrbStatus);

        if (SrbStatus == SRB_STATUS_TIMEOUT)
        {
            /* Not a PMP device */
            *CheckForPmp = FALSE;
            INFO("CH %lu: Seems to have no PMP device\n", ChanData->Channel);
        }
        return CONN_STATUS_FAILURE;
    }

    KeStallExecutionProcessor(20);

    Signature = AHCI_PORT_READ(ChanData->IoBase, PxSignature);

    INFO("CH %lu: Received signature %08lx\n", ChanData->Channel, Signature);
    if (Signature != AHCI_PXSIG_PMP)
        return CONN_STATUS_NO_DEVICE;

    return CONN_STATUS_DEV_UNKNOWN;
}

static
ATA_CONNECTION_STATUS
AtaAhciPhyCheckConnection(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ATA_CONNECTION_STATUS ConnectionStatus;
    ULONG RetryCount;

    for (RetryCount = 0; RetryCount < 2; ++RetryCount)
    {
        /* Suspend DMA engine and wait for port idle */
        if (!AtaAhciEnterIdleState(ChanData))
        {
            /*
             * We can skip the ST bit clearing part while the port in a fatal error condition.
             * Transmit a COMRESET to recover.
             */
            AtaAhciSendComReset(ChanData);

            ConnectionStatus = CONN_STATUS_FAILURE;
            continue;
        }

        AtaAhciSetupDmaMemoryAddress(ChanData);
        AtaAhciSpinUp(ChanData);

        /* Enable FIS reception from the device */
        AtaAhciStartFisReceiveProcessAndWait(ChanData);

        /* Start device detection */
        AtaAhciSendComReset(ChanData);

        if (!AtaAhciPhyCheckDevicePresence(ChanData))
        {
            INFO("CH %lu: Device not connected %08lx\n",
                 ChanData->Channel,
                 AHCI_PORT_READ(ChanData->IoBase, PxSataStatus));

            ConnectionStatus = CONN_STATUS_NO_DEVICE;
            break;
        }

        INFO("CH %lu: Link up %08lx\n",
             ChanData->Channel,
             AHCI_PORT_READ(ChanData->IoBase, PxSataStatus));

        /* Determine if communication is established */
        if (!AtaAhciPhyWaitForReady(ChanData))
        {
            WARN("CH %lu: Unable to establish link %08lx\n",
                 ChanData->Channel,
                 AHCI_PORT_READ(ChanData->IoBase, PxSataStatus));

            ConnectionStatus = CONN_STATUS_FAILURE;
            continue;
        }

        /* Required for a D2H FIS */
        AHCI_PORT_WRITE(ChanData->IoBase, PxSataError, 0xFFFFFFFF);
        /* AtaChanEnableInterruptsSync(ChanData, TRUE); */

        if (!AtaAhciWaitForDeviceReady(ChanData, AHCI_DELAY_READY_DRIVE))
        {
            WARN("CH %lu: Wait for ready failed %08lx\n",
                 ChanData->Channel,
                 AHCI_PORT_READ(ChanData->IoBase, PxTaskFileData));

            ConnectionStatus = CONN_STATUS_FAILURE;
            continue;
        }

        INFO("CH %lu: Device is ready %08lx\n",
             ChanData->Channel,
             AHCI_PORT_READ(ChanData->IoBase, PxTaskFileData));

        ConnectionStatus = CONN_STATUS_DEV_UNKNOWN;
        break;
    }

    return ConnectionStatus;
}

ULONG
AtaAhciEnumerateChannel(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;
    ULONG RetryCount, PortCount;
    ATA_CONNECTION_STATUS ConnectionStatus;
    BOOLEAN CheckForPmp = TRUE;

    if (AtaAhciIsHbaHotRemoved(ChanData))
        return 0;

    AtaChanEnableInterruptsSync(ChanData, FALSE);

    /* COMRESET acts as a bus reset for the SATA port */
    ChanData->PortNotification(AtaResetDetected, ChanData->PortContext, 0xFFFFFFFF);

    for (RetryCount = 3; RetryCount > 0; RetryCount--)
    {
        ChanData->ChanInfo &= ~CHANNEL_FLAG_IS_PMP;

        ConnectionStatus = AtaAhciPhyCheckConnection(ChanData);
        if (ConnectionStatus != CONN_STATUS_DEV_UNKNOWN)
        {
            PortCount = 0;
            break;
        }

        /*
         * Check for a Port Multiplier.
         *
         * If the last discovery resulted in a failure,
         * threat this device as a non-port multiplier device
         * (i.e. a SATA device that is connected to device Port 0).
         */
        if ((ChanData->Controller->AhciCapabilities & AHCI_CAP_SPM) &&
            (RetryCount > 1) &&
            CheckForPmp)
        {
            INFO("CH %lu: Trying to detect PMP\n", ChanData->Channel);

            ConnectionStatus = AtaAhciPmpDetect(ChanData, &CheckForPmp);
            if (ConnectionStatus == CONN_STATUS_FAILURE)
                continue;

            if (ConnectionStatus == CONN_STATUS_DEV_UNKNOWN)
            {
                INFO("CH %lu: Discovered a Port Multiplier\n", ChanData->Channel);

                ChanData->ChanInfo |= CHANNEL_FLAG_IS_PMP;

                if (!AtaAhciPmpIdentifyPmp(ChanData, &PortCount))
                    continue;

                break;
            }
        }
        else
        {
            AtaAhciStartCommandListProcess(ChanData);
        }

        PortCount = 1;
        break;
    }

    if (ConnectionStatus == CONN_STATUS_FAILURE)
    {
        /* Channel reset failed, clear active DMA commands, so we can release pending IRPs safely */
        AtaAhciStopCommandListProcessAndWait(ChanData);
        AtaAhciPerformCommandListOverride(ChanData);
    }
    else if (PortCount == 0)
    {
        AtaAhciPhyEnterListenMode(ChanData);
    }

    AtaChanEnableInterruptsSync(ChanData, TRUE);

    ChanData->TotalPortCount = PortCount;
    return PortCount;
}

static
BOOLEAN
AtaAhciPmpCheckSendComReset(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG PortNumber)
{
    ULONG SataControl;
    UCHAR SrbStatus;

    INFO("CH %lu: Transmit a COMRESET on the interface\n", ChanData->Channel);

    /* Clear errors */
    SrbStatus = AtaAhciPmpWrite(ChanData, PortNumber, AHCI_PMP_SERROR, 0xFFFFFFFF);
    if (SrbStatus != SRB_STATUS_SUCCESS)
        return FALSE;

    SataControl = AHCI_PXCTL_DET_RESET;
    SrbStatus = AtaAhciPmpWrite(ChanData, PortNumber, AHCI_PMP_SCONTROL, SataControl);
    if (SrbStatus != SRB_STATUS_SUCCESS)
        return FALSE;

    KeStallExecutionProcessor(1000);

    SataControl &= ~AHCI_PXCTL_DET_MASK;
    SataControl |= AHCI_PXCTL_DET_IDLE;
    SrbStatus = AtaAhciPmpWrite(ChanData, PortNumber, AHCI_PMP_SCONTROL, SataControl);
    if (SrbStatus != SRB_STATUS_SUCCESS)
        return FALSE;

    /* Clear errors */
    SrbStatus = AtaAhciPmpWrite(ChanData, PortNumber, AHCI_PMP_SERROR, 0xFFFFFFFF);
    if (SrbStatus != SRB_STATUS_SUCCESS)
        return FALSE;

    return TRUE;
}

static
ATA_CONNECTION_STATUS
AtaAhciPmpPhyCheckDevicePresence(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG PortNumber)
{
    ULONG i, SataStatus;
    UCHAR SrbStatus;

    for (i = 0; i < AHCI_DELAY_PMP_DET_PRESENSE; ++i)
    {
        SrbStatus = AtaAhciPmpRead(ChanData, PortNumber, AHCI_PMP_SSTATUS, &SataStatus);
        if (SrbStatus == SRB_STATUS_ERROR)
            continue;
        else if (SrbStatus != SRB_STATUS_SUCCESS)
            return CONN_STATUS_FAILURE;

        if ((SataStatus & AHCI_PXSSTS_DET_MASK) != AHCI_PXSSTS_DET_NO_DEVICE)
        {
            INFO("CH %lu: PMP device %lu link up %08lx\n",
                 ChanData->Channel,
                 PortNumber,
                 SataStatus);
            return CONN_STATUS_DEV_UNKNOWN;
        }

        AtaSleep();
    }

    INFO("CH %lu: PMP device %lu not connected %08lx\n",
         ChanData->Channel,
         PortNumber,
         SataStatus);
    return CONN_STATUS_NO_DEVICE;
}

static
ATA_CONNECTION_STATUS
AtaAhciPmpPhyWaitForReady(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG PortNumber)
{
    ULONG i, SataStatus;
    UCHAR SrbStatus;

    for (i = 0; i < AHCI_DELAY_PMP_DET_PRESENSE; ++i)
    {
        SrbStatus = AtaAhciPmpRead(ChanData, PortNumber, AHCI_PMP_SSTATUS, &SataStatus);
        if (SrbStatus == SRB_STATUS_ERROR)
            continue;
        else if (SrbStatus != SRB_STATUS_SUCCESS)
            return CONN_STATUS_FAILURE;

        if ((SataStatus & AHCI_PXSSTS_DET_MASK) == AHCI_PXSSTS_DET_PHY_OK)
        {
            INFO("CH %lu: PMP device %lu link up %08lx\n",
                 ChanData->Channel,
                 PortNumber,
                 SataStatus);
            return CONN_STATUS_DEV_UNKNOWN;
        }

        AtaSleep();
    }

    INFO("CH %lu: PMP device %lu unable to establish link %08lx\n",
         ChanData->Channel,
         PortNumber,
         SataStatus);
    return CONN_STATUS_NO_DEVICE;
}

static
ATA_CONNECTION_STATUS
AtaPmpCheckConnection(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG PortNumber)
{
    ATA_CONNECTION_STATUS ConnectionStatus;

    INFO("CH %lu: Reset PMP port %lu\n", ChanData->Channel, PortNumber);

    ChanData->PortNotification(AtaResetDetected, ChanData->PortContext, 1 << PortNumber);

    if (!AtaAhciPmpCheckSendComReset(ChanData, PortNumber))
        return CONN_STATUS_FAILURE;

    ConnectionStatus = AtaAhciPmpPhyCheckDevicePresence(ChanData, PortNumber);
    if (ConnectionStatus != CONN_STATUS_DEV_UNKNOWN)
        return ConnectionStatus;

    ConnectionStatus = AtaAhciPmpPhyWaitForReady(ChanData, PortNumber);
    if (ConnectionStatus != CONN_STATUS_DEV_UNKNOWN)
        return ConnectionStatus;

    return CONN_STATUS_DEV_UNKNOWN;
}

static
ATA_CONNECTION_STATUS
AtaAhciPmpIdentifyDeviceBehindPmp(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG PortNumber)
{
    ATA_CONNECTION_STATUS ConnectionStatus;
    UCHAR SrbStatus;
    ULONG RetryCount;

    ASSERT(!(ChanData->ChanInfo & CHANNEL_FLAG_FBS_ENABLED));

    ConnectionStatus = AtaPmpCheckConnection(ChanData, PortNumber);
    if (ConnectionStatus != CONN_STATUS_DEV_UNKNOWN)
        return ConnectionStatus;

    /* Issue a soft reset */
    for (RetryCount = 0; RetryCount < 2; RetryCount++)
    {
        SrbStatus = AtaAhciPmpWrite(ChanData, PortNumber, AHCI_PMP_SERROR, 0xFFFFFFFF);
        if (SrbStatus == SRB_STATUS_ERROR)
            continue;
        else if (SrbStatus != SRB_STATUS_SUCCESS)
            goto Failure;

        SrbStatus = AtaAhciSendResetFis(ChanData, PortNumber, TRUE, 1000);
        if (SrbStatus == SRB_STATUS_ERROR)
            continue;
        else if (SrbStatus != SRB_STATUS_SUCCESS)
            goto Failure;

        /* SRST pulse width */
        KeStallExecutionProcessor(20);

        SrbStatus = AtaAhciSendResetFis(ChanData, PortNumber, FALSE, 1000);
        if (SrbStatus == SRB_STATUS_ERROR)
            continue;
        else if (SrbStatus != SRB_STATUS_SUCCESS)
            goto Failure;
    }

    return CONN_STATUS_DEV_UNKNOWN;

Failure:
    WARN("CH %lu: PMP port %lu soft reset failed %02x\n",
         ChanData->Channel, PortNumber, SrbStatus);
    return CONN_STATUS_FAILURE;
}

ATA_CONNECTION_STATUS
AtaAhciIdentifyDevice(
    _In_ PVOID ChannelContext,
    _In_ ULONG DeviceNumber)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;
    ULONG Signature;

    if (ChanData->ChanInfo & CHANNEL_FLAG_IS_PMP)
    {
        ATA_CONNECTION_STATUS ConnectionStatus;

        ConnectionStatus = AtaAhciPmpIdentifyDeviceBehindPmp(ChanData, DeviceNumber);

        /* Enable use of FIS-based switching once the port multiplier has enumerated */
        if ((ChanData->ChanInfo & CHANNEL_FLAG_HAS_FBS) &&
            (DeviceNumber == (ChanData->TotalPortCount - 1)))
        {
            AtaAhciStopCommandListProcessAndWait(ChanData);
            AtaAhciFbsControl(ChanData, TRUE);
            AtaAhciStartCommandListProcess(ChanData);
        }

        if (ConnectionStatus != CONN_STATUS_DEV_UNKNOWN)
            return ConnectionStatus;
    }
    else
    {
        ASSERT(DeviceNumber == 0);
    }

    Signature = AHCI_PORT_READ(ChanData->IoBase, PxSignature);
    INFO("CH %lu: Received signature %08lx\n", ChanData->Channel, Signature);

    if ((Signature & AHCI_PXSIG_MASK) == (AHCI_PXSIG_ATAPI & AHCI_PXSIG_MASK))
    {
        AtaAhciAtapiLedControl(ChanData, TRUE);
        return CONN_STATUS_DEV_ATAPI;
    }

    AtaAhciAtapiLedControl(ChanData, FALSE);
    return CONN_STATUS_DEV_ATA;
}

VOID
AtaAhciResetChannel(
    _In_ PVOID ChannelContext)
{
    /* The channel reset is done unconditionally in AtaAhciEnumerateChannel() */
    NOTHING;
}

/**
 * @brief Captures and saves a dump of the AHCI task file registers,
 * *except* the values in the Fis->Status and Fis->Error fields.
 */
static
VOID
AtaAhciSaveReceivedFisArea(
    _In_ AHCI_FIS_DEVICE_TO_HOST* __restrict Fis,
    _Inout_ ATA_DEVICE_REQUEST* __restrict Request)
{
    PATA_TASKFILE TaskFile = &Request->Output;

    TaskFile->SectorCount = Fis->SectorCount;
    TaskFile->LowLba = Fis->LbaLow;
    TaskFile->MidLba = Fis->LbaMid;
    TaskFile->HighLba = Fis->LbaHigh;
    TaskFile->DriveSelect = Fis->Device;

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        TaskFile->FeatureEx = 0; // FIS byte 11 is reserved
        TaskFile->SectorCountEx = Fis->SectorCountEx;
        TaskFile->LowLbaEx = Fis->LbaLowEx;
        TaskFile->MidLbaEx = Fis->LbaMidEx;
        TaskFile->HighLbaEx = Fis->LbaHighEx;
    }
}

VOID
AtaAhciSaveTaskFile(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _Inout_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN ProcessErrorStatus)
{
    union
    {
        PAHCI_FIS_PIO_SETUP PioSetup;
        PAHCI_FIS_DEVICE_TO_HOST DeviceToHost;
    } Fis;
    PAHCI_RECEIVED_FIS ReceivedFis;

    ReceivedFis = ChanData->ReceivedFis;

    if (ChanData->ChanInfo & CHANNEL_FLAG_FBS_ENABLED)
        ReceivedFis += DEV_NUMBER(Request->Device);

    Request->Flags |= REQUEST_FLAG_HAS_TASK_FILE;

    if (ProcessErrorStatus)
    {
        /*
         * Device to Host FIS on failed commands.
         * The Request->Output.Status and Request->Output.Error fields should have saved earlier.
         */
        Fis.DeviceToHost = &ReceivedFis->DeviceToHostFis;
    }
    else
    {
        /* Check for successful queued commands (see SATA 3.5 specification 11.14) */
        if (Request->Flags & REQUEST_FLAG_NCQ)
        {
            PAHCI_FIS_SET_DEVICE_BITS SetDeviceBitsFis;

            /*
             * The SDB FIS was received and we do not have enough information
             * for the rest of the registers. Emulate the content of the task file registers.
             */
            RtlCopyMemory(&Request->Output,
                          &Request->TaskFile,
                          sizeof(Request->TaskFile));

            SetDeviceBitsFis = &ReceivedFis->SetDeviceBitsFis;
            Request->Output.Status = SetDeviceBitsFis->Status;
            Request->Output.Error = SetDeviceBitsFis->Error;
            return;
        }
        /* Check for successful PIO Data-In commands (see SATA 3.5 specification 11.8) */
        else if (!(Request->Flags & REQUEST_FLAG_DMA) && (Request->Flags & REQUEST_FLAG_DATA_IN))
        {
            /* In this case we have received a PIO Setup FIS */
            Fis.PioSetup = &ReceivedFis->PioSetupFis;

            Request->Output.Status = Fis.PioSetup->EStatus;
            Request->Output.Error = Fis.PioSetup->Error;
        }
        else
        {
            ULONG TaskFileData;

            /* Otherwise, we have received a Device to Host FIS */
            Fis.DeviceToHost = &ReceivedFis->DeviceToHostFis;

            TaskFileData = AHCI_PORT_READ(ChanData->IoBase, PxTaskFileData);
            Request->Output.Status = (TaskFileData & AHCI_PXTFD_STATUS_MASK);
            Request->Output.Error =
                (TaskFileData & AHCI_PXTFD_ERROR_MASK) >> AHCI_PXTFD_ERROR_SHIFT;
        }
    }

    AtaAhciSaveReceivedFisArea(Fis.DeviceToHost, Request);
}

VOID
AtaAhciHandleFatalError(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG CurrentCommandSlot, TaskFileData;
    ULONG i, CmdStatus, SlotsBitmap, FailedSlot;
    PATA_DEVICE_REQUEST Request;

    Request = NULL;
    CurrentCommandSlot = AHCI_PXCMD_CCS(AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus));

    /* Clear the ST bit. This also clears PxCI, PxSACT, and PxCMD.CCS */
    AtaAhciStopCommandListProcess(ChanData);

    if (AtaAhciIsHbaHotRemoved(ChanData))
        goto Done;

    /* Wait for 500ms */
    for (i = 50000; i > 0; i--)
    {
        CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
        if (!(CmdStatus & AHCI_PXCMD_CR))
            break;

        KeStallExecutionProcessor(10);
    }
    if (i == 0)
    {
        /* The interface is hung */
        ERR("CH %lu: Failed to stop the command list DMA engine %08lx\n",
            ChanData->Channel,
            CmdStatus);
        goto Done;
    }

    TaskFileData = AHCI_PORT_READ(ChanData->IoBase, PxTaskFileData);
    if (TaskFileData & (IDE_STATUS_BUSY | IDE_STATUS_DRQ))
    {
        /* Put the device to the idle state */
        ERR("CH %lu: Busy TFD %08lx\n", ChanData->Channel, TaskFileData);
        goto Done;
    }

    /* Clear errors */
    AHCI_PORT_WRITE(ChanData->IoBase, PxSataError,
                    AHCI_PORT_READ(ChanData->IoBase, PxSataError));

    /* NCQ recovery is needed */
    if (ChanData->ActiveQueuedSlotsBitmap != 0)
    {
        AtaAhciStartCommandListProcess(ChanData);

        /* Find some active command */
        NT_VERIFY(_BitScanForward(&FailedSlot, ChanData->ActiveQueuedSlotsBitmap) != 0);

        Request = ChanData->Slots[FailedSlot];
        ASSERT(Request);
        ASSERT(Request->Slot == FailedSlot);
        ASSERT(Request->Flags & REQUEST_FLAG_NCQ);
        goto Done;
    }

    if (IsPowerOfTwo(ChanData->ActiveSlotsBitmap))
    {
        /* We have exactly one slot is outstanding */
        SlotsBitmap = ChanData->ActiveSlotsBitmap;
    }
    else if (ChanData->ActiveSlotsBitmap & (1 << CurrentCommandSlot))
    {
        SlotsBitmap = 1 << CurrentCommandSlot;
    }
    else
    {
        /* Indicates that the error bit is spurious *or* the exact slot is not known */
        if (ChanData->ActiveSlotsBitmap != 0)
        {
            ERR("CH %lu: Invalid slot received from the HBA %08lX --> %08lX\n",
                ChanData->Channel,
                ChanData->ActiveSlotsBitmap,
                1 << CurrentCommandSlot);
        }
        else
        {
            WARN("CH %lu: Spurious error interrupt %08lX\n",
                 ChanData->Channel,
                 AHCI_PORT_READ(ChanData->IoBase, PxInterruptStatus));
        }
        goto Done;
    }

    AtaAhciStartCommandListProcess(ChanData);

    NT_VERIFY(_BitScanForward(&FailedSlot, SlotsBitmap) != 0);

    Request = ChanData->Slots[FailedSlot];
    ASSERT(Request);
    ASSERT(Request->Slot == FailedSlot);

    /* Save the error */
    Request->SrbStatus = SRB_STATUS_ERROR;
    Request->Output.Status = (TaskFileData & AHCI_PXTFD_STATUS_MASK);
    Request->Output.Error = (TaskFileData & AHCI_PXTFD_ERROR_MASK) >> AHCI_PXTFD_ERROR_SHIFT;

    /* Save the current task file for the "ATA LBA field" (SAT-6 11.7) */
    if (!(Request->Device->TransportFlags & DEVICE_IS_ATAPI) ||
        (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE))
    {
        AtaAhciSaveTaskFile(ChanData, Request, TRUE);
    }

Done:
    /* Request arbitration from the port worker */
    ChanData->PortNotification(AtaRequestFailed, ChanData->PortContext, Request);
}

VOID
AtaAhciHandlePortStateChange(
    _In_ PCHANNEL_DATA_AHCI ChanData,
    _In_ ULONG InterruptStatus)
{
    ULONG SataStatus;

    if (InterruptStatus & AHCI_PXIRQ_DMPS)
    {
        ULONG CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);

        if (CmdStatus & AHCI_PXCMD_MPSP)
        {
            WARN("CH %lu: Mechanical presence switch has changed %08lx\n",
                 ChanData->Channel, CmdStatus);
            goto Notify;
        }
    }

    SataStatus = AHCI_PORT_READ(ChanData->IoBase, PxSataStatus) & AHCI_PXSSTS_DET_MASK;
    if (SataStatus == AHCI_PXSSTS_DET_PHY_OK)
        INFO("CH %lu: Device hot plug detected %08lx\n", ChanData->Channel, SataStatus);
    else
        INFO("CH %lu: Device removal detected %08lx\n", ChanData->Channel, SataStatus);

Notify:
    /* Schedule a port reset */
    ChanData->PortNotification(AtaBusChangeDetected, ChanData->PortContext);
}

BOOLEAN
AtaAhciDowngradeInterfaceSpeed(
    _In_ PCHANNEL_DATA_AHCI ChanData)
{
    ULONG SataSpeed, SataControl;

    SataSpeed = AHCI_PORT_READ(ChanData->IoBase, PxSataStatus) & AHCI_PXSSTS_SPD_MASK;
    if (SataSpeed != AHCI_PXSSTS_SPD_SATA1)
    {
        SataControl = AHCI_PORT_READ(ChanData->IoBase, PxSataControl);

        if ((SataControl & AHCI_PXCTL_SPD_MASK) != AHCI_PXCTL_SPD_LIMIT_NONE)
        {
            SataControl &= ~AHCI_PXCTL_SPD_MASK;
            SataControl |= SataSpeed - AHCI_PXCTL_SPD_LIMIT_LEVEL;

            WARN("CH %lu: Downgrading interface speed to %08lx\n", ChanData->Channel, SataControl);

            AHCI_PORT_WRITE(ChanData->IoBase, PxSataControl, SataControl);
            return TRUE;
        }
    }

    INFO("CH %lu: Unable to downgrade interface speed %08lx\n", ChanData->Channel, SataSpeed);
    return FALSE;
}

ULONG
AtaAhciChannelGetMaximumDeviceCount(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;

    if (Controller->AhciCapabilities & AHCI_CAP_SPM)
        return AHCI_MAX_PMP_DEVICES;

    return AHCI_MAX_PORT_DEVICES;
}
