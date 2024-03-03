/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PATA request handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

BOOLEAN
AtaPataPreparePioDataTransfer(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT(!IS_AHCI(DevExt));
    ASSERT(Request->Flags & (REQUEST_FLAG_DMA | REQUEST_FLAG_PROGRAM_DMA));

    /* ATAPI commands */
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
    {
        Request->Flags &= ~(REQUEST_FLAG_DMA | REQUEST_FLAG_PROGRAM_DMA);
        return TRUE;
    }

    /*
     * For ATA commands there's no simple way to achieve this,
     * we have to fix the command opcode.
     */
    if (Request->Flags & REQUEST_FLAG_READ_WRITE)
    {
        Request->TaskFile.Command = AtaReadWriteCommand(Request, Request->DevExt);
        if (Request->TaskFile.Command == 0)
            return FALSE;

        Request->Flags &= ~(REQUEST_FLAG_DMA | REQUEST_FLAG_PROGRAM_DMA);
        return TRUE;
    }

    /* PIO is not available */
    return FALSE;
}

VOID
NTAPI
AtaPataPollingTimerDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{

}

VOID
NTAPI
AtaPataIoTimer(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{

}

BOOLEAN
NTAPI
AtaPciIdeChannelIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    return TRUE;
}

BOOLEAN
NTAPI
AtaPataChannelIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    return TRUE;
}
