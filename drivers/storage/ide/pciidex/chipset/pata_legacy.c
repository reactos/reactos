/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Legacy (non-PnP) IDE controllers support
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
NTSTATUS
PataGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCHANNEL_DATA_PATA ChanData;
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(Controller->Flags & CTRL_FLAG_NON_PNP);

    Controller->MaxChannels = 1;
    Controller->ChannelBitmap = 0x1;
    Controller->Flags &= ~CTRL_FLAG_NATIVE_PCI;
    Controller->Flags |= CTRL_FLAG_MANUAL_RES;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    ChanData = Controller->Channels[0];
    ChanData->ChanInfo &= ~CHANNEL_FLAG_IO32;
    ChanData->TransferModeSupported = PIO_ALL;

    Status = PciIdeParseResources(ChanData, ResourcesTranslated);
    if (!NT_SUCCESS(Status))
        return Status;

#if defined(_M_IX86)
    if (ChanData->ChanInfo & CHANNEL_FLAG_CBUS)
    {
        UCHAR Value, Result;

        /* Check whether the 32-bit data port can be enabled on the southbridge */
        Value = READ_PORT_UCHAR((PUCHAR)PC98_ATA_BANK);
        WRITE_PORT_UCHAR((PUCHAR)PC98_ATA_BANK, Value ^ PC98_ATA_BANK_32BIT_PORT);
        Result = READ_PORT_UCHAR((PUCHAR)PC98_ATA_BANK);
        if ((Result ^ Value) & PC98_ATA_BANK_32BIT_PORT)
        {
            ChanData->ChanInfo |= CHANNEL_FLAG_IO32;
        }
    }
#endif

    return STATUS_SUCCESS;
}
