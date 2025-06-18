/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI hardware support
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * The PMP configuration code derives from the FreeBSD ATA driver
 * Copyright (c) 2009 Alexander Motin <mav@FreeBSD.org>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
AtaAhciPollRegister(
    _In_ PULONG IoBase,
    _In_ AHCI_PORT_REGISTER Register,
    _In_ ULONG Mask,
    _In_ ULONG Value,
    _In_range_(>, 0) ULONG TimeOut)
{
    ULONG i, Data;

    ASSUME(TimeOut > 0);

    for (i = 0; i < TimeOut; i++)
    {
        Data = AHCI_PORT_READ(IoBase, Register);

        if ((Data & Mask) == Value)
            return TRUE;

        KeStallExecutionProcessor(10);
    }

    return FALSE;
}

static
VOID
AtaAhciSendComReset(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG SataControl;

    INFO("PORT %lu: Transmit a COMRESET on the interface\n", PortData->PortNumber);

    /* Clear errors */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, 0xFFFFFFFF);

    SataControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataControl);
    SataControl &= ~AHCI_PXCTL_DET_MASK;
    SataControl |= AHCI_PXCTL_DET_RESET;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataControl, SataControl);

    KeStallExecutionProcessor(1000);

    SataControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataControl);
    SataControl &= ~AHCI_PXCTL_DET_MASK;
    SataControl |= AHCI_PXCTL_DET_IDLE;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataControl, SataControl);
}

static
VOID
AtaAhciStopCommandListProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    if (CmdStatus & AHCI_PXCMD_ST)
    {
        CmdStatus &= ~AHCI_PXCMD_ST;
        AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);
    }
}

static
VOID
AtaAhciStartCommandListProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_ST;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    /*
     * We are supposed to wait for the AHCI_PXCMD_CR bit to be set
     * before interacting with the port again,
     * but on some AHCI controllers this bit never becomes set.
     */
}

static
BOOLEAN
AtaAhciPerformCommandListOverride(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    if (!(PortData->ChanExt->AhciCapabilities & AHCI_CAP_SCLO))
        return TRUE;

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_CLO;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    return AtaAhciPollRegister(PortData->Ahci.IoBase,
                               PxCmdStatus,
                               AHCI_PXCMD_CLO,
                               0,
                               AHCI_DELAY_CLO_CLEAR);
}

static
VOID
AtaAhciSetupDmaMemoryAddress(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Physical address of the allocated command list */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase,
                    PxCommandListBaseLow,
                    (ULONG)PortData->Ahci.CommandListPhys);
    if (PortData->ChanExt->AhciCapabilities & AHCI_CAP_S64A)
    {
        AHCI_PORT_WRITE(PortData->Ahci.IoBase,
                        PxCommandListBaseHigh,
                        (ULONG)(PortData->Ahci.CommandListPhys >> 32));
    }

    /* Physical address of the allocated FIS receive area */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase,
                    PxFisBaseLow,
                    (ULONG)PortData->Ahci.ReceivedFisPhys);
    if (PortData->ChanExt->AhciCapabilities & AHCI_CAP_S64A)
    {
        AHCI_PORT_WRITE(PortData->Ahci.IoBase,
                        PxFisBaseHigh,
                        (ULONG)(PortData->Ahci.ReceivedFisPhys >> 32));
    }
}

static
VOID
AtaAhciFbsControl(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ BOOLEAN DoEnable)
{
    ULONG FbsControl, NewFbsControl;

    if (!(PortData->PortFlags & PORT_FLAG_HAS_FBS))
        return;

    FbsControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxFisSwitchingControl);
    if (DoEnable)
    {
        NewFbsControl = FbsControl | AHCI_FBS_ENABLE;
    }
    else
    {
        NewFbsControl = FbsControl & ~AHCI_FBS_ENABLE;
        PortData->PortFlags &= ~PORT_FLAG_FBS_ENABLED;
    }

    if ((FbsControl & NewFbsControl) ^ AHCI_FBS_ENABLE)
    {
        AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxFisSwitchingControl, NewFbsControl);
    }

    if (!DoEnable)
        return;

    /* Make sure we can read back the new value */
    FbsControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxFisSwitchingControl);
    if (FbsControl & AHCI_FBS_ENABLE)
    {
        PortData->PortFlags |= PORT_FLAG_FBS_ENABLED;
        INFO("PORT %lu: FBS enabled\n", PortData->PortNumber);
    }
}

VOID
AtaAhciEnterPhyListenMode(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus, SataControl;

    if (!(PortData->ChanExt->AhciCapabilities & AHCI_CAP_SSS))
        return;

    INFO("PORT %lu: Enter listen mode\n", PortData->PortNumber);

    AtaAhciStopCommandListProcess(PortData);

    SataControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataControl);
    SataControl &= ~AHCI_PXCTL_DET_MASK;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataControl, SataControl);

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus &= ~(AHCI_PXCMD_SUD | AHCI_PXCMD_ICC_MASK);
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);
}

VOID
AtaAhciEnableInterrupts(
    _In_ PATAPORT_PORT_DATA PortData)
{
    INFO("PORT %lu: Enable interrupts\n", PortData->PortNumber);

    /* Clear errors */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, 0xFFFFFFFF);

    /* Clear interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptStatus, 0xFFFFFFFF);

    /* Re-enable interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptEnable, AHCI_PORT_INTERRUPT_MASK);
}

static
VOID
AtaPmpRead(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ UCHAR PortNumber,
    _In_ USHORT Register)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    PortData->Worker.InternalDevice.PmpNumber = AHCI_PMP_CONTROL_PORT;

    Request->TimeOut = 3;
    Request->Flags = REQUEST_FLAG_LBA48 |
                     REQUEST_FLAG_SET_DEVICE_REGISTER |
                     REQUEST_FLAG_SAVE_TASK_FILE;

    RtlZeroMemory(TaskFile, sizeof(*TaskFile));
    TaskFile->DriveSelect = IDE_LBA_MODE | PortNumber;
    TaskFile->Feature = (UCHAR)Register;
    TaskFile->FeatureEx = (UCHAR)(Register >> 8);
    TaskFile->Command = IDE_COMMAND_READ_PORT_MULTIPLIER;
    AtaFsmIssueCommand(&PortData->Worker);
}

static
VOID
AtaPmpWrite(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ UCHAR PortNumber,
    _In_ USHORT Register,
    _In_ ULONG Value)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    PortData->Worker.InternalDevice.PmpNumber = AHCI_PMP_CONTROL_PORT;

    Request->TimeOut = 3;
    Request->Flags = REQUEST_FLAG_LBA48 |
                     REQUEST_FLAG_SET_DEVICE_REGISTER;

    RtlZeroMemory(TaskFile, sizeof(*TaskFile));
    TaskFile->DriveSelect = IDE_LBA_MODE | PortNumber;
    TaskFile->Feature = (UCHAR)Register;
    TaskFile->FeatureEx = (UCHAR)(Register >> 8);
    TaskFile->SectorCount = (UCHAR)Value;
    TaskFile->LowLba = (UCHAR)(Value >> 8);
    TaskFile->MidLba = (UCHAR)(Value >> 16);
    TaskFile->HighLba = (UCHAR)(Value >> 24);
    TaskFile->Command = IDE_COMMAND_WRITE_PORT_MULTIPLIER;
    AtaFsmIssueCommand(&PortData->Worker);
}

static
ULONG
AtaPmpGetResult(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;
    PATA_TASKFILE TaskFile = &Request->Output;

    return TaskFile->SectorCount |
           (TaskFile->LowLba << 8) |
           (TaskFile->MidLba << 16) |
           (TaskFile->HighLba << 24);
}

static
VOID
AtaDeviceSendResetFis(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ UCHAR PortNumber,
    _In_ BOOLEAN AssertSRST)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;

    PortData->Worker.InternalDevice.PmpNumber = PortNumber;

    /*
     * Real PMP devices come up faster than ATA/ATAPI devices,
     * keep the timeout as small as possible.
     */
    Request->TimeOut = 2;
    Request->Flags = REQUEST_FLAG_RST_COMMAND | REQUEST_FLAG_POLL;

    Request->TaskFile.Command = AssertSRST ? IDE_DC_RESET_CONTROLLER : 0;
    AtaFsmIssueCommand(&PortData->Worker);
}

static
VOID
AtaAhciFsmResetPort(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ BOOLEAN DoComReset)
{
    /*
     * Something bad happened to the port device.
     * First check to see if the HBA has been hot-removed.
     */
    if (AHCI_HBA_READ(PortData->ChanExt->IoBase, HbaAhciVersion) == 0xFFFFFFFF)
    {
        ERR("PORT %lu: AHCI controller is gone\n", PortData->PortNumber);
        goto Fail;
    }

    if (++PortData->Worker.ResetRetryCount > PORT_MAX_RESET_RETRY_COUNT)
    {
        ERR("PORT %lu: Too many reset attempts\n", PortData->PortNumber);
        goto Fail;
    }

    /*
     * It's allowed to skip the ST bit clearing part
     * while the port in a fatal error condition. Transmit a COMRESET to recover.
     */
    if (DoComReset)
        AtaAhciSendComReset(PortData);

    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_RESET);
    return;

Fail:
    AtaFsmCompletePortEnumEvent(PortData, 0);
}

static
VOID
AtaAhciHandleCheckForIdle(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    PortData->PortFlags &= ~PORT_FLAG_IS_PMP;
    PortData->Worker.Flags &= ~WORKER_FLAG_MANUAL_PORT_RECOVERY;

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);

    /* Already in an idle state */
    if (!(CmdStatus & (AHCI_PXCMD_ST | AHCI_PXCMD_CR | AHCI_PXCMD_FRE | AHCI_PXCMD_FR)))
    {
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_SPINUP);
    }
    else
    {
        AtaAhciStopCommandListProcess(PortData);

        PortData->Worker.TimeOut = AHCI_DELAY_CR_START_STOP;
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_STOP_CMD_ENGINE_WAIT);
    }
}

static
VOID
AtaAhciHandleWaitForStopCommandListProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Do a quick check first (100 us) */
    if (AtaAhciPollRegister(PortData->Ahci.IoBase, PxCmdStatus, AHCI_PXCMD_CR, 0, 10))
    {
        AtaAhciPerformCommandListOverride(PortData);
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_STOP_FIS_ENGINE);
        return;
    }

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    WARN("PORT %lu: Failed to stop the command list DMA engine %08lx\n",
         PortData->PortNumber,
         AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus));

    AtaAhciFsmResetPort(PortData, TRUE);
}

static
VOID
AtaAhciHandleStopFisReceiveProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    if (CmdStatus & AHCI_PXCMD_FRE)
    {
        CmdStatus &= ~AHCI_PXCMD_FRE;
        AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);
    }

    PortData->Worker.TimeOut = AHCI_DELAY_FR_START_STOP;
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_STOP_FIS_ENGINE_WAIT);
}

static
VOID
AtaAhciHandleWaitForStopFisReceiveProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Do a quick check first (100 us) */
    if (AtaAhciPollRegister(PortData->Ahci.IoBase, PxCmdStatus, AHCI_PXCMD_FR, 0, 10))
    {
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_SPINUP);
        return;
    }

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    WARN("PORT %lu: Failed to stop the FIS Receive DMA engine %08lx\n",
         PortData->PortNumber,
         AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus));

    AtaAhciFsmResetPort(PortData, TRUE);
}

static
VOID
AtaAhciHandleSpinUp(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    AtaAhciSetupDmaMemoryAddress(PortData);

    /* Move to the active interface state for the DET value be accurate */
    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus &= ~AHCI_PXCMD_ICC_MASK;
    CmdStatus |= AHCI_PXCMD_ICC_ACTIVE;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);
    AtaAhciPollRegister(PortData->Ahci.IoBase,
                        PxCmdStatus,
                        AHCI_PXCMD_ICC_MASK,
                        AHCI_PXCMD_ICC_IDLE,
                        AHCI_DELAY_INTERFACE_CHANGE);

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);

    /* Clear the PMA bit once the DMA engine has been stopped */
    CmdStatus &= ~AHCI_PXCMD_PMA;

    /* Spin-up and power up the device */
    if (PortData->ChanExt->AhciCapabilities & AHCI_CAP_SSS)
    {
        CmdStatus |= AHCI_PXCMD_SUD;
    }
    if (CmdStatus & AHCI_PXCMD_CPD)
    {
        CmdStatus |= AHCI_PXCMD_POD;
    }
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    /* Enable FIS reception from the device */
    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
    CmdStatus |= AHCI_PXCMD_FRE;
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

    PortData->Worker.TimeOut = AHCI_DELAY_FR_START_STOP;
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_START_FIS_ENGINE_WAIT);
}

static
VOID
AtaAhciHandleWaitForStartFisReceiveProcess(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Do a quick check first (100 us) */
    if (AtaAhciPollRegister(PortData->Ahci.IoBase, PxCmdStatus, AHCI_PXCMD_FR, AHCI_PXCMD_FR, 10))
        goto Exit;

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    WARN("PORT %lu: Failed to start the FIS Receive DMA engine %08lx\n",
         PortData->PortNumber,
         AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus));

    /* Ignore timeouts, on some AHCI controllers the FR bit never becomes set */
Exit:
    /* Start device detection */
    AtaAhciSendComReset(PortData);

    PortData->Worker.TimeOut = AHCI_DELAY_DET_PRESENCE;
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_DET_PRESENCE_WAIT);
}

static
VOID
AtaAhciHandleWaitForDevicePresence(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG i, SataStatus;

    /* Do a quick check first (100 us) */
    for (i = 0; i < 10; ++i)
    {
        SataStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataStatus) & AHCI_PXSSTS_DET_MASK;
        if (SataStatus != AHCI_PXSSTS_DET_NO_DEVICE)
        {
            AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PHY_READY_WAIT);
            return;
        }

        KeStallExecutionProcessor(10);
    }

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    INFO("PORT %lu: Link down %08lx\n",
         PortData->PortNumber,
         AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataStatus));

    AtaFsmCompletePortEnumEvent(PortData, 0);
}

static
VOID
AtaAhciHandleWaitForPhyReady(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Do a quick check first (100 us) */
    if (AtaAhciPollRegister(PortData->Ahci.IoBase,
                            PxSataStatus,
                            AHCI_PXSSTS_DET_MASK,
                            AHCI_PXSSTS_DET_PHY_OK,
                            10))
    {
        INFO("PORT %lu: Link up %08lx\n",
             PortData->PortNumber,
             AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataStatus));

        /* Required for a D2H FIS */
        AtaAhciEnableInterrupts(PortData);

        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_CHECK);
        return;
    }

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    INFO("PORT %lu: Link down %08lx\n",
         PortData->PortNumber,
         AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataStatus));

    AtaFsmCompletePortEnumEvent(PortData, 0);
}

static
VOID
AtaAhciHandleWaitForDeviceReady(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Do a quick check first (100 us) */
    if (AtaAhciPollRegister(PortData->Ahci.IoBase,
                            PxTaskFileData,
                            IDE_STATUS_BUSY | IDE_STATUS_DRQ,
                            0,
                            10))
    {
        INFO("PORT %lu: Device is ready %08lx\n",
             PortData->PortNumber,
             AHCI_PORT_READ(PortData->Ahci.IoBase, PxTaskFileData));

        AtaAhciStartCommandListProcess(PortData);
        AtaFsmCompletePortEnumEvent(PortData, 1);
        return;
    }

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        /* Keep clearing the PHY errors after every 1 second interval */
        if ((PortData->Worker.TimeOut % AHCI_DELAY_1_SECOND) == 0)
        {
            INFO("PORT %lu: Device is busy %lu\n",
                 PortData->PortNumber,
                 PortData->Worker.TimeOut);

            AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, 0xFFFFFFFF);
        }

        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    WARN("PORT %lu: Wait for ready failed %08lx\n",
         PortData->PortNumber,
         AHCI_PORT_READ(PortData->Ahci.IoBase, PxTaskFileData));

    AtaFsmCompletePortEnumEvent(PortData, 0);
}

static
VOID
AtaAhciHandlePmpCheck(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CmdStatus;

    /*
     * Check for a Port Multiplier.
     *
     * If the last discovery resulted in a failure,
     * threat this device as a non-port multiplier device
     * (i.e. a SATA device that is connected to device Port 0).
     */
    if ((PortData->ChanExt->AhciCapabilities & AHCI_CAP_SPM) &&
        (PortData->Worker.PmpDetectRetryCount < PORT_MAX_PMP_DETECT_RETRY_COUNT))
    {
        TRACE("PORT %lu: Trying to detect PMP\n", PortData->PortNumber);

        /* Indicate a PMP */
        CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);
        CmdStatus |= AHCI_PXCMD_PMA;
        AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxCmdStatus, CmdStatus);

        /* Turn off FIS-based switching prior to issuing the software reset */
        AtaAhciFbsControl(PortData, FALSE);

        /* Start the command list DMA engine */
        AtaAhciPerformCommandListOverride(PortData);
        AtaAhciStartCommandListProcess(PortData);

        /* BSY and DRQ must be cleared prior to issuing the software reset */
        PortData->Worker.TimeOut = AHCI_DELAY_PMP_READY_DRIVE;
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_READY_WAIT);
    }
    else
    {
        PortData->Worker.TimeOut = AHCI_DELAY_READY_DRIVE;
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_DEVICE_READY_WAIT);
    }
}

static
VOID
AtaAhciHandlePmpWaitForDeviceReady(
    _In_ PATAPORT_PORT_DATA PortData)
{
    /* Do a quick check first (100 us) */
    if (AtaAhciPollRegister(PortData->Ahci.IoBase,
                            PxTaskFileData,
                            IDE_STATUS_BUSY | IDE_STATUS_DRQ,
                            0,
                            10))
    {
        INFO("PORT %lu: Device is ready %08lx\n",
             PortData->PortNumber,
             AHCI_PORT_READ(PortData->Ahci.IoBase, PxTaskFileData));

        /* Issue a software reset */
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_RESET_1);
        return;
    }

    /* Retry after the time interval */
    if (--PortData->Worker.TimeOut != 0)
    {
        /* Keep clearing the PHY errors after every 1 second interval */
        if ((PortData->Worker.TimeOut % AHCI_DELAY_1_SECOND) == 0)
        {
            INFO("PORT %lu: Device is busy %lu\n",
                 PortData->PortNumber,
                 PortData->Worker.TimeOut);

            AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, 0xFFFFFFFF);
        }

        AtaFsmRequestTimer(&PortData->Worker);
        return;
    }

    WARN("PORT %lu: Wait for ready failed %08lx\n",
         PortData->PortNumber,
         AHCI_PORT_READ(PortData->Ahci.IoBase, PxTaskFileData));

    ++PortData->Worker.PmpDetectRetryCount;

    AtaAhciFsmResetPort(PortData, TRUE);
}

static
VOID
AtaAhciHandlePmpAssertSrst(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaDeviceSendResetFis(PortData, AHCI_PMP_CONTROL_PORT, TRUE);
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_RESET_2);

    /*
     * This command will timeout if a PMP-capable port indicates device presence
     * and the connected device is not a port multiplier.
     * As a result, the system boot time may increase by (Request->TimeOut * ActivePortCount).
     * We skip PMP detection upon the first timeout, there is nothing more that can be done.
     */
    PortData->Worker.Flags |= WORKER_FLAG_MANUAL_PORT_RECOVERY;
}

static
VOID
AtaAhciHandlePmpDeAssertSrst(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        /* Not a PMP device */
        PortData->Worker.PmpDetectRetryCount = PORT_MAX_PMP_DETECT_RETRY_COUNT;

        INFO("PORT %lu: Seems to have no PMP device\n", PortData->PortNumber);
        AtaAhciFsmResetPort(PortData, FALSE);
        return;
    }

    /* SRST pulse width */
    KeStallExecutionProcessor(20);

    AtaDeviceSendResetFis(PortData, AHCI_PMP_CONTROL_PORT, FALSE);
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_CHECK_SIG);
}

static
VOID
AtaAhciHandlePmpCheckSignature(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;
    ULONG Signature;

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        ++PortData->Worker.PmpDetectRetryCount;
        AtaAhciFsmResetPort(PortData, FALSE);
        return;
    }

    Signature = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSignature);

    TRACE("PORT %lu: Received signature %08lx\n", PortData->PortNumber, Signature);

    if (Signature == AHCI_PXSIG_PMP)
    {
        INFO("PORT %lu: Discovered a Port Multiplier\n", PortData->PortNumber);

        PortData->PortFlags |= PORT_FLAG_IS_PMP;

        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_ID_PRODUCT_ID);
    }
    else
    {
        PortData->Worker.Flags &= ~WORKER_FLAG_MANUAL_PORT_RECOVERY;

        PortData->Worker.TimeOut = AHCI_DELAY_READY_DRIVE;
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_DEVICE_READY_WAIT);
    }
}

static
VOID
AtaAhciHandlePmpReadProductId(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaPmpRead(PortData, AHCI_PMP_CONTROL_PORT, PmpProductId);
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_ID_REV_INFO);
}

static
VOID
AtaAhciHandlePmpReadRevisionInfo(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        ++PortData->Worker.PmpDetectRetryCount;
        AtaAhciFsmResetPort(PortData, FALSE);
        return;
    }

    PortData->Worker.PmpProductId = AtaPmpGetResult(PortData);

    INFO("PORT %lu: PMP PID %08lx\n", PortData->PortNumber, PortData->Worker.PmpProductId);

    AtaPmpRead(PortData, AHCI_PMP_CONTROL_PORT, PmpRevisionInfo);
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_ID_PORT_INFO);
}

static
VOID
AtaAhciHandlePmpReadPortInfo(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        ++PortData->Worker.PmpDetectRetryCount;
        AtaAhciFsmResetPort(PortData, FALSE);
        return;
    }

    INFO("PORT %lu: PMP REV %08lx\n", PortData->PortNumber, AtaPmpGetResult(PortData));

    AtaPmpRead(PortData, AHCI_PMP_CONTROL_PORT, PmpPortInfo);
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_ID_PORT_INFO_RESULT);
}

static
VOID
AtaAhciHandlePmpReadPortInfoResult(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        ++PortData->Worker.PmpDetectRetryCount;
        AtaAhciFsmResetPort(PortData, FALSE);
        return;
    }

    PortData->Worker.Flags &= ~WORKER_FLAG_MANUAL_PORT_RECOVERY;

    PortData->Worker.PmpPortCount = AtaPmpGetResult(PortData) & AHCI_MAX_PMP_DEVICES;

    INFO("PORT %lu: PMP %lu ports\n", PortData->PortNumber, PortData->Worker.PmpPortCount);

    /* From FreeBSD: Hide pseudo ATA devices from the driver */
    switch (PortData->Worker.PmpProductId)
    {
        case 0x37261095:
        case 0x38261095:
        {
            if (PortData->Worker.PmpPortCount == 6)
                PortData->Worker.PmpPortCount = 5;
            break;
        }

        case 0x47261095:
            if (PortData->Worker.PmpPortCount == 7)
                PortData->Worker.PmpPortCount = 5;
            break;

        case 0x57231095:
        case 0x57331095:
        case 0x57341095:
        case 0x57441095:
            if (PortData->Worker.PmpPortCount > 0)
                --PortData->Worker.PmpPortCount;
            break;

        default:
            break;
    }

    if (PortData->Worker.PmpPortCount == 0)
    {
        AtaFsmCompletePortEnumEvent(PortData, 0);
    }
    else
    {
        /* Reset each port */
        PortData->Worker.CurrentPmpPort = 0;
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_COMRESET_1);
    }
}

static
VOID
AtaAhciHandlePmpClearErrors(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaPmpWrite(PortData, PortData->Worker.CurrentPmpPort, AHCI_PMP_SERROR, 0xFFFFFFFF);
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_COMRESET_2);
}

static
VOID
AtaAhciHandlePmpAssertReset(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaPmpWrite(PortData, PortData->Worker.CurrentPmpPort, AHCI_PMP_SCONTROL, AHCI_PXCTL_DET_RESET);
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_COMRESET_3);
}

static
VOID
AtaAhciHandlePmpDeAssertReset(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaPmpWrite(PortData, PortData->Worker.CurrentPmpPort, AHCI_PMP_SCONTROL, AHCI_PXCTL_DET_IDLE);

    if (PortData->Worker.CurrentPmpPort++ < PortData->Worker.PmpPortCount)
    {
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_COMRESET_3);
    }
    else
    {
        /* Determine if communication is established */
        PortData->Worker.PmpActivePorts = 0;
        PortData->Worker.CurrentPmpPort = 0;
        PortData->Worker.TimeOut = AHCI_DELAY_PMP_DET_STABLE;
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_PHY_READY_WAIT);
    }
}

static
VOID
AtaAhciHandlePmpWaitForPhyReady(
    _In_ PATAPORT_PORT_DATA PortData)
{
    AtaPmpRead(PortData, PortData->Worker.CurrentPmpPort, AHCI_PMP_SSTATUS);
    AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_PHY_READY_WAIT_RESULT);
}

static
VOID
AtaAhciHandlePmpWaitForPhyReadyResult(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;
    ULONG SataStatus;

    SataStatus = AtaPmpGetResult(PortData);

    if ((Request->SrbStatus != SRB_STATUS_SUCCESS) ||
        ((SataStatus & AHCI_PXSSTS_DET_MASK) != AHCI_PXSSTS_DET_PHY_OK))
    {
        if (--PortData->Worker.TimeOut != 0)
        {
            AtaFsmRequestTimer(&PortData->Worker);
            AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_PHY_READY_WAIT);
            return;
        }

        INFO("PORT %lu: PMP device %lu link down %08lx\n",
             PortData->PortNumber,
             PortData->Worker.CurrentPmpPort,
             SataStatus);

        if (PortData->Worker.CurrentPmpPort++ < PortData->Worker.PmpPortCount)
        {
            PortData->Worker.TimeOut = AHCI_DELAY_PMP_DET_STABLE;
            AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_PHY_READY_WAIT);
        }
        else
        {
            AtaFsmCompletePortEnumEvent(PortData, PortData->Worker.PmpActivePorts);
        }
    }
    else
    {
        INFO("PORT %lu: PMP device %lu link up %08lx\n",
             PortData->PortNumber,
             PortData->Worker.CurrentPmpPort,
             SataStatus);
        PortData->Worker.PmpActivePorts |= 1 << PortData->Worker.CurrentPmpPort;

        AtaPmpWrite(PortData, PortData->Worker.CurrentPmpPort, AHCI_PMP_SERROR, 0xFFFFFFFF);
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_CLEAR_ERRORS_RESULT);
    }
}

static
VOID
AtaAhciHandlePmpClearDeviceErrorsResult(
    _In_ PATAPORT_PORT_DATA PortData)
{
    if (PortData->Worker.CurrentPmpPort++ < PortData->Worker.PmpPortCount)
    {
        PortData->Worker.TimeOut = AHCI_DELAY_PMP_DET_STABLE;
        AtaFsmSetLocalState(&PortData->Worker, AHCI_STATE_ENUM_PMP_PHY_READY_WAIT);
    }
    else
    {
        AtaFsmCompletePortEnumEvent(PortData, PortData->Worker.PmpActivePorts);
    }
}

VOID
AtaAhciRunStateMachine(
    _In_ PATAPORT_PORT_DATA PortData)
{
    switch (PortData->Worker.LocalState)
    {
        case AHCI_STATE_ENUM_RESET:
            AtaAhciHandleCheckForIdle(PortData);
            break;
        case AHCI_STATE_ENUM_STOP_CMD_ENGINE_WAIT:
            AtaAhciHandleWaitForStopCommandListProcess(PortData);
            break;
        case AHCI_STATE_ENUM_STOP_FIS_ENGINE:
            AtaAhciHandleStopFisReceiveProcess(PortData);
            break;
        case AHCI_STATE_ENUM_STOP_FIS_ENGINE_WAIT:
            AtaAhciHandleWaitForStopFisReceiveProcess(PortData);
            break;
        case AHCI_STATE_ENUM_SPINUP:
            AtaAhciHandleSpinUp(PortData);
            break;
        case AHCI_STATE_ENUM_START_FIS_ENGINE_WAIT:
            AtaAhciHandleWaitForStartFisReceiveProcess(PortData);
            break;
        case AHCI_STATE_ENUM_DET_PRESENCE_WAIT:
            AtaAhciHandleWaitForDevicePresence(PortData);
            break;
        case AHCI_STATE_ENUM_PHY_READY_WAIT:
            AtaAhciHandleWaitForPhyReady(PortData);
            break;
        case AHCI_STATE_ENUM_DEVICE_READY_WAIT:
            AtaAhciHandleWaitForDeviceReady(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_CHECK:
            AtaAhciHandlePmpCheck(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_READY_WAIT:
            AtaAhciHandlePmpWaitForDeviceReady(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_RESET_1:
            AtaAhciHandlePmpAssertSrst(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_RESET_2:
            AtaAhciHandlePmpDeAssertSrst(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_CHECK_SIG:
            AtaAhciHandlePmpCheckSignature(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_ID_PRODUCT_ID:
            AtaAhciHandlePmpReadProductId(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_ID_REV_INFO:
            AtaAhciHandlePmpReadRevisionInfo(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_ID_PORT_INFO:
            AtaAhciHandlePmpReadPortInfo(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_ID_PORT_INFO_RESULT:
            AtaAhciHandlePmpReadPortInfoResult(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_COMRESET_1:
            AtaAhciHandlePmpClearErrors(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_COMRESET_2:
            AtaAhciHandlePmpAssertReset(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_COMRESET_3:
            AtaAhciHandlePmpDeAssertReset(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_PHY_READY_WAIT:
            AtaAhciHandlePmpWaitForPhyReady(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_PHY_READY_WAIT_RESULT:
            AtaAhciHandlePmpWaitForPhyReadyResult(PortData);
            break;
        case AHCI_STATE_ENUM_PMP_CLEAR_ERRORS_RESULT:
            AtaAhciHandlePmpClearDeviceErrorsResult(PortData);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
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
    _In_ PATAPORT_PORT_DATA PortData,
    _Inout_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN ProcessErrorStatus)
{
    union
    {
        PAHCI_FIS_PIO_SETUP PioSetup;
        PAHCI_FIS_DEVICE_TO_HOST DeviceToHost;
    } Fis;
    PAHCI_RECEIVED_FIS ReceivedFis;

    ReceivedFis = PortData->Ahci.ReceivedFis;

    if (PortData->PortFlags & PORT_FLAG_FBS_ENABLED)
        ReceivedFis += Request->Device->PmpNumber;

    Request->Flags |= REQUEST_FLAG_HAS_TASK_FILE;

    if (ProcessErrorStatus)
    {
        /* Device to Host FIS on failed commands */
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

            TaskFileData = AHCI_PORT_READ(PortData->Ahci.IoBase, PxTaskFileData);
            Request->Output.Status = (TaskFileData & AHCI_PXTFD_STATUS_MASK);
            Request->Output.Error =
                (TaskFileData & AHCI_PXTFD_ERROR_MASK) >> AHCI_PXTFD_ERROR_SHIFT;
        }
    }

    AtaAhciSaveReceivedFisArea(Fis.DeviceToHost, Request);
}

BOOLEAN
AtaAhciDowngradeInterfaceSpeed(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG SataSpeed, SataControl;

    SataSpeed = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataStatus) & AHCI_PXSSTS_SPD_MASK;

    if (SataSpeed != AHCI_PXSSTS_SPD_SATA1)
    {
        SataControl = AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataControl);

        if ((SataControl & AHCI_PXCTL_SPD_MASK) != AHCI_PXCTL_SPD_LIMIT_NONE)
        {
            SataControl &= ~AHCI_PXCTL_SPD_MASK;
            SataControl |= SataSpeed - AHCI_PXCTL_SPD_LIMIT_LEVEL;

            WARN("Downgrading interface speed to %08lx\n", SataControl);

            AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataControl, SataControl);
            return TRUE;
        }
    }

    return FALSE;
}

VOID
AtaAhciHandlePortChange(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG InterruptStatus)
{


}

VOID
AtaAhciHandleFatalError(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG CurrentCommandSlot, TaskFileData;
    ULONG SlotsBitmap, FailedSlot;
    PATA_DEVICE_REQUEST Request = NULL;

    CurrentCommandSlot = AHCI_PXCMD_CCS(AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus));

    /* Clear the ST bit. This also clears PxCI, PxSACT, and PxCMD.CCS */
    AtaAhciStopCommandListProcess(PortData);
    if (!AtaAhciPollRegister(PortData->Ahci.IoBase,
                             PxCmdStatus,
                             AHCI_PXCMD_CR,
                             0,
                             500000 / 10))
    {
        /* The interface is hung */
        WARN("Failed to stop the command list DMA engine\n");
        goto Done;
    }

    TaskFileData = AHCI_PORT_READ(PortData->Ahci.IoBase, PxTaskFileData);
    if (TaskFileData & (IDE_STATUS_BUSY | IDE_STATUS_DRQ))
    {
        /* Put the device to the idle state */
        ERR("Busy TFD %08lx\n", TaskFileData);
        goto Done;
    }

    /* Clear errors */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError,
                    AHCI_PORT_READ(PortData->Ahci.IoBase, PxSataError));

    /* NCQ recovery is needed */
    if (PortData->ActiveQueuedSlotsBitmap != 0)
    {
        AtaAhciStartCommandListProcess(PortData);

        /* Dummy value to distinguish the port reset */
        Request = UlongToPtr(1);
        goto Done;
    }

    if (IsPowerOfTwo(PortData->ActiveSlotsBitmap))
    {
        /* We have exactly one slot is outstanding */
        SlotsBitmap = PortData->ActiveSlotsBitmap;
    }
    else if (PortData->ActiveSlotsBitmap & (1 << CurrentCommandSlot))
    {
        SlotsBitmap = 1 << CurrentCommandSlot;
    }
    else
    {
        /* Indicates that the error bit is spurious *or* the exact slot is not known */
        if (PortData->ActiveSlotsBitmap != 0)
        {
            ERR("Invalid slot received from the HBA %08lX --> %08lX\n",
                PortData->ActiveSlotsBitmap, 1 << CurrentCommandSlot);
        }
        else
        {
            WARN("Spurious error interrupt %08lX\n",
                 AHCI_PORT_READ(PortData->Ahci.IoBase, PxInterruptStatus));
        }

        goto Done;
    }

    AtaAhciStartCommandListProcess(PortData);

    NT_VERIFY(_BitScanForward(&FailedSlot, SlotsBitmap) != 0);

    Request = PortData->Slots[FailedSlot];
    ASSERT(Request);
    ASSERT(Request->Slot == FailedSlot);

    /* Save the error */
    Request->SrbStatus = SRB_STATUS_ERROR;
    Request->Output.Status = (TaskFileData & AHCI_PXTFD_STATUS_MASK);
    Request->Output.Error = (TaskFileData & AHCI_PXTFD_ERROR_MASK) >> AHCI_PXTFD_ERROR_SHIFT;

    /* Save the current task file for the "ATA LBA field" (SAT-5 11.7) */
    if (!IS_ATAPI(Request->Device) || (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE))
    {
        AtaAhciSaveTaskFile(PortData, Request, TRUE);
    }

Done:
    /* Request arbitration from the port worker */
    AtaPortRecoveryFromError(PortData, ACTION_DEVICE_ERROR, Request);
}

CODE_SEG("PAGE")
BOOLEAN
AtaAhciPortInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PATAPORT_PORT_DATA PortData,
    _Out_ PATAPORT_PORT_INFO PortInfo)
{
    ULONG CmdStatus;

    PAGED_CODE();

    /* Disable interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptEnable, 0);

    /* Clear interrupts */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxInterruptStatus, 0xFFFFFFFF);

    /* Clear the error register */
    AHCI_PORT_WRITE(PortData->Ahci.IoBase, PxSataError, 0xFFFFFFFF);

    /* Begin the process of stopping the command list DMA engine for later initialization */
    AtaAhciStopCommandListProcess(PortData);

    CmdStatus = AHCI_PORT_READ(PortData->Ahci.IoBase, PxCmdStatus);

    /* Check for the FIS-based switching feature support */
    if ((ChanExt->AhciCapabilities & AHCI_CAP_SPM) &&
        (ChanExt->AhciCapabilities & AHCI_CAP_FBSS) &&
        (CmdStatus & AHCI_PXCMD_FBSCP))
    {
        INFO("PORT %lu: FBS supported\n", PortData->PortNumber);
        PortData->PortFlags |= PORT_FLAG_HAS_FBS;
    }

    if (AtaAhciIsPortRemovable(ChanExt->AhciCapabilities, CmdStatus))
    {
        INFO("PORT %lu: Port is external\n", PortData->PortNumber);
        PortData->PortFlags |= PORT_FLAG_IS_EXTERNAL;
    }

    /* Allocate shared memory */
    if (!AtaAhciPortAllocateMemory(ChanExt, PortData, PortInfo))
        return FALSE;

    return TRUE;
}

CODE_SEG("PAGE")
NTSTATUS
AtaAhciInitHba(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    ULONG i, GlobalControl, AhciVersion;

    PAGED_CODE();

    TRACE("Initializing HBA [%04X:%04X]\n", ChanExt->DeviceID, ChanExt->VendorID);

    /* Set AE before accessing other AHCI registers */
    GlobalControl = AHCI_HBA_READ(ChanExt->IoBase, HbaGlobalControl);
    GlobalControl |= AHCI_GHC_AE;
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    AhciVersion = AHCI_HBA_READ(ChanExt->IoBase, HbaAhciVersion);
    if (AhciVersion >= AHCI_VERSION_1_2)
    {
        ChanExt->AhciCapabilitiesEx = AHCI_HBA_READ(ChanExt->IoBase, HbaCapabilitiesEx);

        /* if (ChanExt->AhciCapabilitiesEx & AHCI_CAP2_BOH) */ // todo
    }

    ChanExt->AhciCapabilities = AHCI_HBA_READ(ChanExt->IoBase, HbaCapabilities);
    ChanExt->PortBitmap = AHCI_HBA_READ(ChanExt->IoBase, HbaPortBitmap);
    ChanExt->NumberOfPorts = CountSetBits(ChanExt->PortBitmap);

    if (ChanExt->NumberOfPorts == 0)
    {
        ASSERT(ChanExt->NumberOfPorts == 0);
        return STATUS_DEVICE_HARDWARE_ERROR;
    }

    /* Reset the HBA */
    GlobalControl |= AHCI_GHC_HR;
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    /* HBA reset may take up to 1 second */
    for (i = 100000; i > 0; i--)
    {
        GlobalControl = AHCI_HBA_READ(ChanExt->IoBase, HbaGlobalControl);
        if (!(GlobalControl & AHCI_GHC_HR))
            break;

        KeStallExecutionProcessor(10);
    }
    if (i == 0)
    {
        ERR("HBA reset failed\n");
        return STATUS_IO_TIMEOUT;
    }

    /* Disable interrupts and re-enable AE */
    GlobalControl |= AHCI_GHC_AE;
    GlobalControl &= ~AHCI_GHC_IE;
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);

    /* Clear interrupts */
    AHCI_HBA_WRITE(ChanExt->IoBase, HbaInterruptStatus, 0xFFFFFFFF);

    INFO("Version %08lx, PI %08lx, CAP %08lx, CAP2 %08lx\n",
         AhciVersion,
         ChanExt->PortBitmap,
         ChanExt->AhciCapabilities,
         ChanExt->AhciCapabilitiesEx);

    return STATUS_SUCCESS;
}
