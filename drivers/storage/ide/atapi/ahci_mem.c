/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI shared memory allocation
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
BOOLEAN
AtaAhciPortAllocateMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PATAPORT_PORT_DATA PortData,
    _Out_ PATAPORT_PORT_INFO PortInfo)
{
    PDMA_OPERATIONS DmaOperations;
    ULONG i, j, SlotNumber, CommandSlots, BlockSize;
    ULONG CommandListSize, CommandTableLength, CommandTablesPerPage;
    PVOID Buffer;
    ULONG_PTR BufferVa;
    ULONG64 BufferPa;
    PHYSICAL_ADDRESS PhysicalAddress;

    PAGED_CODE();

    DmaOperations = ChanExt->AdapterObject->DmaOperations;

    CommandSlots = ((ChanExt->AhciCapabilities & AHCI_CAP_NCS) >> 8) + 1;

    /* The maximum size of the command list is 1024 bytes */
    CommandListSize = FIELD_OFFSET(AHCI_COMMAND_LIST, CommandHeader[CommandSlots]);
    BlockSize = CommandListSize + (AHCI_COMMAND_LIST_ALIGNMENT - 1);

    /* Add the receive area structure (256 bytes) */
    if (!(PortData->PortFlags & PORT_FLAG_HAS_FBS))
    {
        BlockSize += sizeof(AHCI_RECEIVED_FIS);

        /* The command list is 1024-byte aligned, which saves us some bytes of allocation size */
        BlockSize += ALIGN_UP_BY(CommandListSize, AHCI_RECEIVED_FIS_ALIGNMENT) - CommandListSize;
    }

    /* Add the local buffer */
    BlockSize += ATA_LOCAL_BUFFER_SIZE;

    Buffer = DmaOperations->AllocateCommonBuffer(ChanExt->AdapterObject,
                                                 BlockSize,
                                                 &PhysicalAddress,
                                                 TRUE);
    if (!Buffer)
        return FALSE;
    RtlZeroMemory(Buffer, BlockSize);

    PortInfo->CommandListSize = BlockSize;
    PortInfo->CommandListOriginal = Buffer;
    PortInfo->CommandListPhysOriginal.QuadPart = PhysicalAddress.QuadPart;

    BufferVa = (ULONG_PTR)Buffer;
    BufferPa = PhysicalAddress.QuadPart;

    /* Command list */
    BufferVa = ALIGN_UP_BY(BufferVa, AHCI_COMMAND_LIST_ALIGNMENT);
    BufferPa = ALIGN_UP_BY(BufferPa, AHCI_COMMAND_LIST_ALIGNMENT);
    PortData->Ahci.CommandList = (PVOID)BufferVa;
    PortData->Ahci.CommandListPhys = BufferPa;
    BufferVa += CommandListSize;
    BufferPa += CommandListSize;

    /* Alignment requirement */
    ASSERT((ULONG_PTR)PortData->Ahci.CommandListPhys % AHCI_COMMAND_LIST_ALIGNMENT == 0);

    /* Received FIS structure */
    if (!(PortData->PortFlags & PORT_FLAG_HAS_FBS))
    {
        BufferVa = ALIGN_UP_BY(BufferVa, AHCI_RECEIVED_FIS_ALIGNMENT);
        BufferPa = ALIGN_UP_BY(BufferPa, AHCI_RECEIVED_FIS_ALIGNMENT);
        PortData->Ahci.ReceivedFis = (PVOID)BufferVa;
        PortData->Ahci.ReceivedFisPhys = BufferPa;
        BufferVa += sizeof(AHCI_RECEIVED_FIS);
        BufferPa += sizeof(AHCI_RECEIVED_FIS);

        /* Alignment requirement */
        ASSERT((ULONG_PTR)PortData->Ahci.ReceivedFisPhys % AHCI_RECEIVED_FIS_ALIGNMENT == 0);
    }

    /* Local buffer */
    PortInfo->LocalBuffer = (PVOID)BufferVa;
    PortData->Ahci.LocalSgList.Elements[0].Address.QuadPart = BufferPa;
    PortData->Ahci.LocalSgList.Elements[0].Length = ATA_LOCAL_BUFFER_SIZE;
    PortData->Ahci.LocalSgList.NumberOfElements = 1;

    if (PortData->PortFlags & PORT_FLAG_HAS_FBS)
    {
        /* The FBS receive area is 4kB, allocate a page which is also 4kB-aligned */
        BlockSize = PAGE_SIZE;

        /*
         * Some other architectures, like ia64, use a different page size,
         * that is a multiple of 4096.
         */
        C_ASSERT(PAGE_SIZE % AHCI_RECEIVED_FIS_FBS_ALIGNMENT == 0);

        Buffer = DmaOperations->AllocateCommonBuffer(ChanExt->AdapterObject,
                                                     BlockSize,
                                                     &PhysicalAddress,
                                                     TRUE);
        if (!Buffer)
            return FALSE;
        RtlZeroMemory(Buffer, BlockSize);

        PortInfo->ReceivedFisOriginal = Buffer;
        PortInfo->ReceivedFisPhysOriginal.QuadPart = PhysicalAddress.QuadPart;

        BufferVa = (ULONG_PTR)Buffer;
        BufferPa = PhysicalAddress.QuadPart;

        PortData->Ahci.ReceivedFis = (PVOID)BufferVa;
        PortData->Ahci.ReceivedFisPhys = BufferPa;

        /* Alignment requirement */
        ASSERT(BufferPa % AHCI_RECEIVED_FIS_FBS_ALIGNMENT == 0);
    }

    /* 32-bit DMA */
    if (!(ChanExt->AhciCapabilities & AHCI_CAP_S64A))
    {
        ASSERT((ULONG)(PortData->Ahci.CommandListPhys >> 32) == 0);
        ASSERT((ULONG)(PortData->Ahci.ReceivedFisPhys >> 32) == 0);
    }

    INFO("Command List PA %llx VA %p\n",
         PortData->Ahci.CommandListPhys, PortData->Ahci.CommandList);
    INFO("Received FIS PA %llx VA %p\n",
         PortData->Ahci.ReceivedFisPhys, PortData->Ahci.ReceivedFis);
    INFO("Local buffer PA %llx VA %p\n",
         PortData->Ahci.LocalSgList.Elements[0].Address.QuadPart, PortInfo->LocalBuffer);
    INFO("Allocated %lu PRD pages\n", ChanExt->MapRegisterCount);

    CommandTableLength = FIELD_OFFSET(AHCI_COMMAND_TABLE, PrdTable[ChanExt->MapRegisterCount]);

    ASSERT(ChanExt->MapRegisterCount != 0 &&
           ChanExt->MapRegisterCount <= AHCI_MAX_PRDT_ENTRIES);

    /*
     * See ATA_MAX_TRANSFER_LENGTH, currently the MapRegisterCount is restricted to
     * a maximum of (0x20000 / PAGE_SIZE) + 1 = 33 pages.
     * Each command table will require us 128 + 16 * 33 + (128 - 1) = 783 bytes of shared memory.
     */
    ASSERT(PAGE_SIZE > (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1)));

    /* Allocate one-page chunks to avoid having a large chunk of contiguous memory */
    CommandTablesPerPage = PAGE_SIZE / (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1));

    /* Command tables allocation loop */
    SlotNumber = 0;
    i = CommandSlots;
    while (i > 0)
    {
        ULONG TableCount;

        TableCount = min(i, CommandTablesPerPage);
        BlockSize = (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1)) * TableCount;

        /* Allocate a chunk of memory */
        Buffer = DmaOperations->AllocateCommonBuffer(ChanExt->AdapterObject,
                                                     BlockSize,
                                                     &PhysicalAddress,
                                                     TRUE);
        if (!Buffer)
            return FALSE;
        RtlZeroMemory(Buffer, BlockSize);

        PortInfo->CommandTableOriginal[SlotNumber] = Buffer;
        PortInfo->CommandTablePhysOriginal[SlotNumber].QuadPart = PhysicalAddress.QuadPart;
        PortInfo->CommandTableSize[SlotNumber] = BlockSize;

        BufferVa = (ULONG_PTR)Buffer;
        BufferPa = PhysicalAddress.QuadPart;

        /* Split the allocation into command tables */
        for (j = 0; j < TableCount; ++j)
        {
            PAHCI_COMMAND_HEADER CommandHeader;

            BufferVa = ALIGN_UP_BY(BufferVa, AHCI_COMMAND_TABLE_ALIGNMENT);
            BufferPa = ALIGN_UP_BY(BufferPa, AHCI_COMMAND_TABLE_ALIGNMENT);

            /* Alignment requirement */
            ASSERT(BufferPa % AHCI_COMMAND_TABLE_ALIGNMENT == 0);

            /* 32-bit DMA */
            if (!(ChanExt->AhciCapabilities & AHCI_CAP_S64A))
            {
                ASSERT((ULONG)(BufferPa >> 32) == 0);
            }

            PortData->Ahci.CommandTable[SlotNumber] = (PAHCI_COMMAND_TABLE)BufferVa;

            CommandHeader = &PortData->Ahci.CommandList->CommandHeader[SlotNumber];
            CommandHeader->CommandTableBaseLow = (ULONG)BufferPa;
            CommandHeader->CommandTableBaseHigh = (ULONG)(BufferPa >> 32);

            /* INFO("[%02lu] Command Table PA %llx VA %p\n", SlotNumber, BufferPa, BufferVa); */

            ++SlotNumber;
            BufferVa += CommandTableLength;
            BufferPa += CommandTableLength;
        }

        i -= TableCount;
    }

    return TRUE;
}

CODE_SEG("PAGE")
VOID
AtaFdoFreePortMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{

}
