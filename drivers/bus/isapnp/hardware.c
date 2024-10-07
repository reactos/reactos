/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Hardware support code
 * COPYRIGHT:       Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *                  Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 *                  Copyright 2021 Dmitry Borisov <di.sean@protonmail.com>
 */

#ifndef UNIT_TEST

#include "isapnp.h"

#define NDEBUG
#include <debug.h>

#ifdef _MSC_VER
#pragma warning(disable:28138) /* ISA bus always uses hardcoded port addresses */
#endif

#endif /* UNIT_TEST */

ULONG IsaConfigPorts[2] =
{
    ISAPNP_WRITE_DATA_PCAT,
    ISAPNP_ADDRESS_PCAT
};

static
inline
VOID
WriteAddress(
    _In_ UCHAR Address)
{
    WRITE_PORT_UCHAR(UlongToPtr(IsaConfigPorts[1]), Address);
}

static
inline
VOID
WriteData(
    _In_ UCHAR Data)
{
    WRITE_PORT_UCHAR(UlongToPtr(IsaConfigPorts[0]), Data);
}

static
inline
UCHAR
ReadData(
    _In_ PUCHAR ReadDataPort)
{
    return READ_PORT_UCHAR(ReadDataPort);
}

static
CODE_SEG("PAGE")
VOID
WriteByte(
    _In_ UCHAR Address,
    _In_ UCHAR Value)
{
    PAGED_CODE();

    WriteAddress(Address);
    WriteData(Value);
}

static
inline
VOID
WriteWord(
    _In_ UCHAR Address,
    _In_ USHORT Value)
{
    WriteByte(Address + 1, (UCHAR)Value);
    WriteByte(Address, Value >> 8);
}

static
inline
VOID
WriteDoubleWord(
    _In_ UCHAR Address,
    _In_ ULONG Value)
{
    WriteWord(Address + 2, (USHORT)Value);
    WriteWord(Address, Value >> 16);
}

static
CODE_SEG("PAGE")
UCHAR
ReadByte(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Address)
{
    PAGED_CODE();

    WriteAddress(Address);
    return ReadData(ReadDataPort);
}

static
inline
USHORT
ReadWord(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Address)
{
    return ((ReadByte(ReadDataPort, Address) << 8) |
            (ReadByte(ReadDataPort, Address + 1)));
}

static
inline
ULONG
ReadDoubleWord(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Address)
{
    return ((ReadWord(ReadDataPort, Address) << 16) |
            (ReadWord(ReadDataPort, Address + 2)));
}

static
inline
VOID
SetReadDataPort(
    _In_ PUCHAR ReadDataPort)
{
    WriteByte(ISAPNP_READPORT, (UCHAR)((ULONG_PTR)ReadDataPort >> 2));
}

static
inline
VOID
EnterIsolationState(VOID)
{
    WriteAddress(ISAPNP_SERIALISOLATION);
}

static
inline
VOID
WaitForKey(VOID)
{
    WriteByte(ISAPNP_CONFIGCONTROL, ISAPNP_CONFIG_WAIT_FOR_KEY);
}

static
inline
VOID
Wake(
    _In_ UCHAR Csn)
{
    WriteByte(ISAPNP_WAKE, Csn);
}

static
inline
UCHAR
ReadResourceData(
    _In_ PUCHAR ReadDataPort)
{
    return ReadByte(ReadDataPort, ISAPNP_RESOURCEDATA);
}

static
inline
UCHAR
ReadStatus(
    _In_ PUCHAR ReadDataPort)
{
    return ReadByte(ReadDataPort, ISAPNP_STATUS);
}

static
inline
VOID
WriteCsn(
    _In_ UCHAR Csn)
{
    WriteByte(ISAPNP_CARDSELECTNUMBER, Csn);
}

static
inline
VOID
WriteLogicalDeviceNumber(
    _In_ UCHAR LogDev)
{
    WriteByte(ISAPNP_LOGICALDEVICENUMBER, LogDev);
}

static
inline
VOID
ActivateDevice(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR LogDev)
{
    WriteLogicalDeviceNumber(LogDev);

    WriteByte(ISAPNP_IORANGECHECK,
              ReadByte(ReadDataPort, ISAPNP_IORANGECHECK) & ~2);

    WriteByte(ISAPNP_ACTIVATE, 1);
}

static
inline
VOID
DeactivateDevice(
    _In_ UCHAR LogDev)
{
    WriteLogicalDeviceNumber(LogDev);
    WriteByte(ISAPNP_ACTIVATE, 0);
}

static
inline
USHORT
ReadIoBase(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadWord(ReadDataPort, ISAPNP_IOBASE(Index));
}

static
inline
UCHAR
ReadIrqNo(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadByte(ReadDataPort, ISAPNP_IRQNO(Index)) & 0x0F;
}

static
inline
UCHAR
ReadIrqType(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadByte(ReadDataPort, ISAPNP_IRQTYPE(Index));
}

static
inline
UCHAR
ReadDmaChannel(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadByte(ReadDataPort, ISAPNP_DMACHANNEL(Index)) & 0x07;
}

static
inline
USHORT
ReadMemoryBase(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadWord(ReadDataPort, ISAPNP_MEMBASE(Index));
}

static
inline
UCHAR
ReadMemoryControl(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadByte(ReadDataPort, ISAPNP_MEMCONTROL(Index));
}

static
inline
USHORT
ReadMemoryLimit(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadWord(ReadDataPort, ISAPNP_MEMLIMIT(Index));
}

static
inline
ULONG
ReadMemoryBase32(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadDoubleWord(ReadDataPort, ISAPNP_MEMBASE32(Index));
}

static
inline
UCHAR
ReadMemoryControl32(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadByte(ReadDataPort, ISAPNP_MEMCONTROL32(Index));
}

static
inline
ULONG
ReadMemoryLimit32(
    _In_ PUCHAR ReadDataPort,
    _In_ UCHAR Index)
{
    return ReadDoubleWord(ReadDataPort, ISAPNP_MEMLIMIT32(Index));
}

static
inline
UCHAR
NextLFSR(
    _In_ UCHAR Lfsr,
    _In_ UCHAR InputBit)
{
    UCHAR NextLfsr = Lfsr >> 1;

    NextLfsr |= (((Lfsr ^ NextLfsr) ^ InputBit)) << 7;

    return NextLfsr;
}

static
CODE_SEG("PAGE")
VOID
SendKey(VOID)
{
    UCHAR i, Lfsr;

    PAGED_CODE();

    WriteAddress(0x00);
    WriteAddress(0x00);

    Lfsr = ISAPNP_LFSR_SEED;
    for (i = 0; i < 32; i++)
    {
        WriteAddress(Lfsr);
        Lfsr = NextLFSR(Lfsr, 0);
    }
}

static
CODE_SEG("PAGE")
UCHAR
PeekByte(
    _In_ PUCHAR ReadDataPort)
{
    UCHAR i;

    PAGED_CODE();

    for (i = 0; i < 20; i++)
    {
        if (ReadStatus(ReadDataPort) & 0x01)
            return ReadResourceData(ReadDataPort);

        KeStallExecutionProcessor(1000);
    }

    return 0xFF;
}

static
CODE_SEG("PAGE")
VOID
Peek(
    _In_ PUCHAR ReadDataPort,
    _Out_writes_bytes_all_opt_(Length) PVOID Buffer,
    _In_ USHORT Length)
{
    USHORT i;

    PAGED_CODE();

    for (i = 0; i < Length; i++)
    {
        UCHAR Byte = PeekByte(ReadDataPort);

        if (Buffer)
            ((PUCHAR)Buffer)[i] = Byte;
    }
}

static
CODE_SEG("PAGE")
UCHAR
IsaPnpChecksum(
    _In_ PISAPNP_IDENTIFIER Identifier)
{
    UCHAR i, j, Lfsr;

    PAGED_CODE();

    Lfsr = ISAPNP_LFSR_SEED;
    for (i = 0; i < FIELD_OFFSET(ISAPNP_IDENTIFIER, Checksum); i++)
    {
        UCHAR Byte = ((PUCHAR)Identifier)[i];

        for (j = 0; j < RTL_BITS_OF(Byte); j++)
        {
            Lfsr = NextLFSR(Lfsr, Byte);
            Byte >>= 1;
        }
    }

    return Lfsr;
}

static
CODE_SEG("PAGE")
VOID
IsaPnpExtractAscii(
    _Out_writes_all_(3) PUCHAR Buffer,
    _In_ USHORT CompressedData)
{
    PAGED_CODE();

    Buffer[0] = ((CompressedData >> 2) & 0x1F) + 'A' - 1;
    Buffer[1] = (((CompressedData & 0x3) << 3) | ((CompressedData >> 13) & 0x7)) + 'A' - 1;
    Buffer[2] = ((CompressedData >> 8) & 0x1F) + 'A' - 1;
}

static
CODE_SEG("PAGE")
NTSTATUS
ReadTags(
    _In_ PUCHAR ReadDataPort,
    _Out_writes_(ISAPNP_MAX_RESOURCEDATA) PUCHAR Buffer,
    _In_ ULONG MaxLength,
    _Out_ PUSHORT MaxLogDev,
    _Out_ PULONG MaxTagsPerDevice)
{
    ULONG TagCount = 0;

    PAGED_CODE();

    *MaxLogDev = 0;
    *MaxTagsPerDevice = 0;

    while (TRUE)
    {
        UCHAR Tag;
        USHORT TagLen;

        ++TagCount;

        if (MaxLength < 1)
        {
            DPRINT("Too small tag\n");
            return STATUS_BUFFER_OVERFLOW;
        }

        Tag = PeekByte(ReadDataPort);
        if (Tag == 0)
        {
            DPRINT("Invalid tag\n");
            return STATUS_INVALID_PARAMETER_1;
        }
        *Buffer++ = Tag;
        --MaxLength;

        if (ISAPNP_IS_SMALL_TAG(Tag))
        {
            TagLen = ISAPNP_SMALL_TAG_LEN(Tag);
            Tag = ISAPNP_SMALL_TAG_NAME(Tag);
        }
        else
        {
            UCHAR Temp[2];

            if (MaxLength < sizeof(Temp))
            {
                DPRINT("Too small tag\n");
                return STATUS_BUFFER_OVERFLOW;
            }

            Peek(ReadDataPort, &Temp, sizeof(Temp));
            *Buffer++ = Temp[0];
            *Buffer++ = Temp[1];
            MaxLength -= sizeof(Temp);

            TagLen = Temp[0] + (Temp[1] << 8);
            Tag = ISAPNP_LARGE_TAG_NAME(Tag);
        }

        if (Tag == 0xFF && TagLen == 0xFFFF)
        {
            DPRINT("Invalid tag\n");
            return STATUS_INVALID_PARAMETER_2;
        }

        if (TagLen > MaxLength)
        {
            DPRINT("Too large resource data structure\n");
            return STATUS_BUFFER_OVERFLOW;
        }

        Peek(ReadDataPort, Buffer, TagLen);
        MaxLength -= TagLen;
        Buffer += TagLen;

        if (Tag == ISAPNP_TAG_LOGDEVID)
        {
            /* Attempt to guess the allocation size based on the tags available */
            *MaxTagsPerDevice = max(*MaxTagsPerDevice, TagCount);
            TagCount = 0;

            (*MaxLogDev)++;
        }
        else if (Tag == ISAPNP_TAG_END)
        {
            *MaxTagsPerDevice = max(*MaxTagsPerDevice, TagCount);
            break;
        }
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
FreeLogicalDevice(
    _In_ __drv_freesMem(Mem) PISAPNP_LOGICAL_DEVICE LogDevice)
{
    PLIST_ENTRY Entry;

    PAGED_CODE();

    if (LogDevice->FriendlyName)
        ExFreePoolWithTag(LogDevice->FriendlyName, TAG_ISAPNP);

    if (LogDevice->Resources)
        ExFreePoolWithTag(LogDevice->Resources, TAG_ISAPNP);

    Entry = LogDevice->CompatibleIdList.Flink;
    while (Entry != &LogDevice->CompatibleIdList)
    {
        PISAPNP_COMPATIBLE_ID_ENTRY CompatibleId =
            CONTAINING_RECORD(Entry, ISAPNP_COMPATIBLE_ID_ENTRY, IdLink);

        RemoveEntryList(&CompatibleId->IdLink);

        Entry = Entry->Flink;

        ExFreePoolWithTag(CompatibleId, TAG_ISAPNP);
    }

    ExFreePoolWithTag(LogDevice, TAG_ISAPNP);
}

static
CODE_SEG("PAGE")
NTSTATUS
ParseTags(
    _In_ PUCHAR ResourceData,
    _In_ USHORT LogDevToParse,
    _Inout_ PISAPNP_LOGICAL_DEVICE LogDevice)
{
    USHORT LogDev;
    ISAPNP_DEPENDENT_FUNCTION_STATE DfState = dfNotStarted;
    PISAPNP_RESOURCE Resource = LogDevice->Resources;
    PUCHAR IdStrPos = NULL;
    USHORT IdStrLen = 0;

    PAGED_CODE();

    DPRINT("%s for CSN %u, LDN %u\n", __FUNCTION__, LogDevice->CSN, LogDevice->LDN);

    LogDev = LogDevToParse + 1;

    while (TRUE)
    {
        UCHAR Tag;
        USHORT TagLen;

        Tag = *ResourceData++;

        if (ISAPNP_IS_SMALL_TAG(Tag))
        {
            TagLen = ISAPNP_SMALL_TAG_LEN(Tag);
            Tag = ISAPNP_SMALL_TAG_NAME(Tag);
        }
        else
        {
            TagLen = *ResourceData++;
            TagLen += *ResourceData++ << 8;

            Tag = ISAPNP_LARGE_TAG_NAME(Tag);
        }

        switch (Tag)
        {
            case ISAPNP_TAG_LOGDEVID:
            {
                ISAPNP_LOGDEVID Temp;

                --LogDev;

                if (LogDev != 0 ||
                    (TagLen > sizeof(ISAPNP_LOGDEVID) ||
                     TagLen < (sizeof(ISAPNP_LOGDEVID) - 1)))
                {
                    goto SkipTag;
                }

                RtlCopyMemory(&Temp, ResourceData, TagLen);
                ResourceData += TagLen;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  VendorId 0x%04X\n"
                       "  ProdId   0x%04X\n",
                       Tag, TagLen,
                       Temp.VendorId,
                       Temp.ProdId);

                IsaPnpExtractAscii(LogDevice->LogVendorId, Temp.VendorId);
                LogDevice->LogProdId = RtlUshortByteSwap(Temp.ProdId);

                break;
            }

            case ISAPNP_TAG_COMPATDEVID:
            {
                ISAPNP_COMPATID Temp;
                PISAPNP_COMPATIBLE_ID_ENTRY CompatibleId;

                if (LogDev != 0 || TagLen != sizeof(ISAPNP_COMPATID))
                    goto SkipTag;

                CompatibleId = ExAllocatePoolWithTag(PagedPool,
                                                     sizeof(ISAPNP_COMPATIBLE_ID_ENTRY),
                                                     TAG_ISAPNP);
                if (!CompatibleId)
                    return STATUS_INSUFFICIENT_RESOURCES;

                RtlCopyMemory(&Temp, ResourceData, TagLen);
                ResourceData += TagLen;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  VendorId 0x%04X\n"
                       "  ProdId   0x%04X\n",
                       Tag, TagLen,
                       Temp.VendorId,
                       Temp.ProdId);

                IsaPnpExtractAscii(CompatibleId->VendorId, Temp.VendorId);
                CompatibleId->ProdId = RtlUshortByteSwap(Temp.ProdId);

                InsertTailList(&LogDevice->CompatibleIdList, &CompatibleId->IdLink);

                break;
            }

            case ISAPNP_TAG_IRQ:
            {
                PISAPNP_IRQ_DESCRIPTION Description;

                if (LogDev != 0)
                    goto SkipTag;

                if (TagLen > sizeof(ISAPNP_IRQ_DESCRIPTION) ||
                    TagLen < (sizeof(ISAPNP_IRQ_DESCRIPTION) - 1))
                {
                    DPRINT1("Invalid tag %x\n", ISAPNP_TAG_IRQ);
                    return STATUS_UNSUCCESSFUL;
                }

                Resource->Type = ISAPNP_RESOURCE_TYPE_IRQ;
                Description = &Resource->IrqDescription;
                ++Resource;

                RtlCopyMemory(Description, ResourceData, TagLen);
                ResourceData += TagLen;

                if (TagLen == (sizeof(ISAPNP_IRQ_DESCRIPTION) - 1))
                    Description->Information = 0x01;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  Mask        0x%X\n"
                       "  Information 0x%X\n",
                       Tag, TagLen,
                       Description->Mask,
                       Description->Information);

                break;
            }

            case ISAPNP_TAG_DMA:
            {
                PISAPNP_DMA_DESCRIPTION Description;

                if (LogDev != 0)
                    goto SkipTag;

                if (TagLen != sizeof(ISAPNP_DMA_DESCRIPTION))
                {
                    DPRINT1("Invalid tag %x\n", ISAPNP_TAG_DMA);
                    return STATUS_UNSUCCESSFUL;
                }

                Resource->Type = ISAPNP_RESOURCE_TYPE_DMA;
                Description = &Resource->DmaDescription;
                ++Resource;

                RtlCopyMemory(Description, ResourceData, TagLen);
                ResourceData += TagLen;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  Mask        0x%X\n"
                       "  Information 0x%X\n",
                       Tag, TagLen,
                       Description->Mask,
                       Description->Information);

                break;
            }

            case ISAPNP_TAG_STARTDEP:
            {
                if (LogDev != 0)
                    goto SkipTag;

                if (TagLen > 1)
                {
                    DPRINT1("Invalid tag %x\n", ISAPNP_TAG_STARTDEP);
                    return STATUS_UNSUCCESSFUL;
                }

                if (DfState == dfNotStarted)
                {
                    DfState = dfStarted;
                }
                else if (DfState != dfStarted)
                {
                    goto SkipTag;
                }

                Resource->Type = ISAPNP_RESOURCE_TYPE_START_DEPENDENT;
                ++Resource;

                if (TagLen != 1)
                {
                    Resource->Priority = 1;
                }
                else
                {
                    RtlCopyMemory(&Resource->Priority, ResourceData, TagLen);
                    ResourceData += TagLen;
                }

                DPRINT("*** Start dependent set, priority %u ***\n",
                       Resource->Priority);

                break;
            }

            case ISAPNP_TAG_ENDDEP:
            {
                if (LogDev != 0 || DfState != dfStarted)
                    goto SkipTag;

                Resource->Type = ISAPNP_RESOURCE_TYPE_END_DEPENDENT;
                ++Resource;

                DfState = dfDone;

                ResourceData += TagLen;

                DPRINT("*** End of dependent set ***\n");

                break;
            }

            case ISAPNP_TAG_IOPORT:
            {
                PISAPNP_IO_DESCRIPTION Description;

                if (LogDev != 0)
                    goto SkipTag;

                if (TagLen != sizeof(ISAPNP_IO_DESCRIPTION))
                {
                    DPRINT1("Invalid tag %x\n", ISAPNP_TAG_IOPORT);
                    return STATUS_UNSUCCESSFUL;
                }

                Resource->Type = ISAPNP_RESOURCE_TYPE_IO;
                Description = &Resource->IoDescription;
                ++Resource;

                RtlCopyMemory(Description, ResourceData, TagLen);
                ResourceData += TagLen;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  Information 0x%X\n"
                       "  Minimum     0x%X\n"
                       "  Maximum     0x%X\n"
                       "  Alignment   0x%X\n"
                       "  Length      0x%X\n",
                       Tag, TagLen,
                       Description->Information,
                       Description->Minimum,
                       Description->Maximum,
                       Description->Alignment,
                       Description->Length);

                break;
            }

            case ISAPNP_TAG_FIXEDIO:
            {
                ISAPNP_FIXED_IO_DESCRIPTION Temp;
                PISAPNP_IO_DESCRIPTION Description;

                if (LogDev != 0)
                    goto SkipTag;

                if (TagLen != sizeof(ISAPNP_FIXED_IO_DESCRIPTION))
                {
                    DPRINT1("Invalid tag %x\n", ISAPNP_TAG_FIXEDIO);
                    return STATUS_UNSUCCESSFUL;
                }

                Resource->Type = ISAPNP_RESOURCE_TYPE_IO;
                Description = &Resource->IoDescription;
                ++Resource;

                RtlCopyMemory(&Temp, ResourceData, TagLen);
                ResourceData += TagLen;

                /* Save the address bits [0:9] */
                Temp.IoBase &= ((1 << 10) - 1);

                Description->Information = 0;
                Description->Minimum =
                Description->Maximum = Temp.IoBase;
                Description->Alignment = 1;
                Description->Length = Temp.Length;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  IoBase 0x%X\n"
                       "  Length 0x%X\n",
                       Tag, TagLen,
                       Temp.IoBase,
                       Temp.Length);

                break;
            }

            case ISAPNP_TAG_END:
            {
                if (IdStrPos)
                {
                    PSTR End;

                    LogDevice->FriendlyName = ExAllocatePoolWithTag(PagedPool,
                                                                    IdStrLen + sizeof(ANSI_NULL),
                                                                    TAG_ISAPNP);
                    if (!LogDevice->FriendlyName)
                        return STATUS_INSUFFICIENT_RESOURCES;

                    RtlCopyMemory(LogDevice->FriendlyName, IdStrPos, IdStrLen);

                    End = LogDevice->FriendlyName + IdStrLen - 1;
                    while (End > LogDevice->FriendlyName && *End == ' ')
                    {
                        --End;
                    }
                    *++End = ANSI_NULL;
                }

                Resource->Type = ISAPNP_RESOURCE_TYPE_END;

                return STATUS_SUCCESS;
            }

            case ISAPNP_TAG_MEMRANGE:
            {
                PISAPNP_MEMRANGE_DESCRIPTION Description;

                if (LogDev != 0)
                    goto SkipTag;

                if (TagLen != sizeof(ISAPNP_MEMRANGE_DESCRIPTION))
                {
                    DPRINT1("Invalid tag %x\n", ISAPNP_TAG_MEMRANGE);
                    return STATUS_UNSUCCESSFUL;
                }

                LogDevice->Flags |= ISAPNP_HAS_MEM24_DECODER;
                ASSERT(!(LogDevice->Flags & ISAPNP_HAS_MEM32_DECODER));

                Resource->Type = ISAPNP_RESOURCE_TYPE_MEMRANGE;
                Description = &Resource->MemRangeDescription;
                ++Resource;

                RtlCopyMemory(Description, ResourceData, TagLen);
                ResourceData += TagLen;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  Information 0x%X\n"
                       "  Minimum     0x%X\n"
                       "  Maximum     0x%X\n"
                       "  Alignment   0x%X\n"
                       "  Length      0x%X\n",
                       Tag, TagLen,
                       Description->Information,
                       Description->Minimum,
                       Description->Maximum,
                       Description->Alignment,
                       Description->Length);

                break;
            }

            case ISAPNP_TAG_ANSISTR:
            {
                /* If logical device identifier is not supplied, use card identifier */
                if (LogDev == LogDevToParse + 1 || LogDev == 0)
                {
                    IdStrPos = ResourceData;
                    IdStrLen = TagLen;

                    ResourceData += TagLen;

                    DPRINT("Found tag 0x%X (len %u)\n"
                           "  '%.*s'\n",
                           Tag, TagLen,
                           IdStrLen,
                           IdStrPos);
                }
                else
                {
                    goto SkipTag;
                }

                break;
            }

            case ISAPNP_TAG_MEM32RANGE:
            {
                PISAPNP_MEMRANGE32_DESCRIPTION Description;

                if (LogDev != 0)
                    goto SkipTag;

                if (TagLen != sizeof(ISAPNP_MEMRANGE32_DESCRIPTION))
                {
                    DPRINT1("Invalid tag %x\n", ISAPNP_TAG_MEM32RANGE);
                    return STATUS_UNSUCCESSFUL;
                }

                LogDevice->Flags |= ISAPNP_HAS_MEM32_DECODER;
                ASSERT(!(LogDevice->Flags & ISAPNP_HAS_MEM24_DECODER));

                Resource->Type = ISAPNP_RESOURCE_TYPE_MEMRANGE32;
                Description = &Resource->MemRange32Description;
                ++Resource;

                RtlCopyMemory(Description, ResourceData, TagLen);
                ResourceData += TagLen;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  Information 0x%X\n"
                       "  Minimum     0x%08lX\n"
                       "  Maximum     0x%08lX\n"
                       "  Alignment   0x%08lX\n"
                       "  Length      0x%08lX\n",
                       Tag, TagLen,
                       Description->Information,
                       Description->Minimum,
                       Description->Maximum,
                       Description->Alignment,
                       Description->Length);

                break;
            }

            case ISAPNP_TAG_FIXEDMEM32RANGE:
            {
                ISAPNP_FIXEDMEMRANGE_DESCRIPTION Temp;
                PISAPNP_MEMRANGE32_DESCRIPTION Description;

                if (LogDev != 0)
                    goto SkipTag;

                if (TagLen != sizeof(ISAPNP_FIXEDMEMRANGE_DESCRIPTION))
                {
                    DPRINT1("Invalid tag %x\n", ISAPNP_TAG_FIXEDMEM32RANGE);
                    return STATUS_UNSUCCESSFUL;
                }

                LogDevice->Flags |= ISAPNP_HAS_MEM32_DECODER;
                ASSERT(!(LogDevice->Flags & ISAPNP_HAS_MEM24_DECODER));

                Resource->Type = ISAPNP_RESOURCE_TYPE_MEMRANGE32;
                Description = &Resource->MemRange32Description;
                ++Resource;

                RtlCopyMemory(&Temp, ResourceData, TagLen);
                ResourceData += TagLen;

                Description->Information = Temp.Information;
                Description->Minimum =
                Description->Maximum = Temp.MemoryBase;
                Description->Alignment = 1;
                Description->Length = Temp.Length;

                DPRINT("Found tag 0x%X (len %u)\n"
                       "  Information 0x%X\n"
                       "  MemoryBase  0x%08lX\n"
                       "  Length      0x%08lX\n",
                       Tag, TagLen,
                       Temp.Information,
                       Temp.MemoryBase,
                       Temp.Length);

                break;
            }

SkipTag:
            default:
            {
                if (LogDev == 0)
                    DPRINT("Found unknown tag 0x%X (len %u)\n", Tag, TagLen);

                /* We don't want to read informations on this
                 * logical device, or we don't know the tag. */
                ResourceData += TagLen;
                break;
            }
        }
    }
}

static
CODE_SEG("PAGE")
BOOLEAN
ReadCurrentResources(
    _In_ PUCHAR ReadDataPort,
    _Inout_ PISAPNP_LOGICAL_DEVICE LogDevice)
{
    UCHAR i;

    PAGED_CODE();

    DPRINT("%s for CSN %u, LDN %u\n", __FUNCTION__, LogDevice->CSN, LogDevice->LDN);

    WriteLogicalDeviceNumber(LogDevice->LDN);

    /* If the device is not activated by BIOS then the device has no boot resources */
    if (!(ReadByte(ReadDataPort, ISAPNP_ACTIVATE) & 1))
    {
        LogDevice->Flags &= ~ISAPNP_HAS_RESOURCES;
        return FALSE;
    }

    for (i = 0; i < RTL_NUMBER_OF(LogDevice->Io); i++)
    {
        LogDevice->Io[i].CurrentBase = ReadIoBase(ReadDataPort, i);

        /* Skip empty descriptors */
        if (!LogDevice->Io[i].CurrentBase)
            break;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDevice->Irq); i++)
    {
        LogDevice->Irq[i].CurrentNo = ReadIrqNo(ReadDataPort, i);

        if (!LogDevice->Irq[i].CurrentNo)
            break;

        LogDevice->Irq[i].CurrentType = ReadIrqType(ReadDataPort, i);
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDevice->Dma); i++)
    {
        LogDevice->Dma[i].CurrentChannel = ReadDmaChannel(ReadDataPort, i);

        if (LogDevice->Dma[i].CurrentChannel == DMACHANNEL_NONE)
            break;
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDevice->MemRange); i++)
    {
        LogDevice->MemRange[i].CurrentBase = ReadMemoryBase(ReadDataPort, i) << 8;

        if (!LogDevice->MemRange[i].CurrentBase)
            break;

        LogDevice->MemRange[i].CurrentLength = ReadMemoryLimit(ReadDataPort, i) << 8;

        if (ReadMemoryControl(ReadDataPort, i) & MEMORY_UPPER_LIMIT)
        {
            LogDevice->MemRange[i].CurrentLength -= LogDevice->MemRange[i].CurrentBase;
        }
        else
        {
            LogDevice->MemRange[i].CurrentLength =
                RANGE_LENGTH_TO_LENGTH(LogDevice->MemRange[i].CurrentLength);
        }
    }
    for (i = 0; i < RTL_NUMBER_OF(LogDevice->MemRange32); i++)
    {
        LogDevice->MemRange32[i].CurrentBase = ReadMemoryBase32(ReadDataPort, i);

        if (!LogDevice->MemRange32[i].CurrentBase)
            break;

        LogDevice->MemRange32[i].CurrentLength = ReadMemoryLimit32(ReadDataPort, i);

        if (ReadMemoryControl32(ReadDataPort, i) & MEMORY_UPPER_LIMIT)
        {
            LogDevice->MemRange32[i].CurrentLength -= LogDevice->MemRange32[i].CurrentBase;
        }
        else
        {
            LogDevice->MemRange32[i].CurrentLength =
                RANGE_LENGTH_TO_LENGTH(LogDevice->MemRange32[i].CurrentLength);
        }
    }

    LogDevice->Flags |= ISAPNP_HAS_RESOURCES;
    return TRUE;
}

static
CODE_SEG("PAGE")
VOID
IsaProgramIoDecoder(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Index)
{
    PAGED_CODE();

    ASSERT(Descriptor->u.Port.Start.QuadPart <= 0xFFFF);

    WriteWord(ISAPNP_IOBASE(Index), Descriptor->u.Port.Start.LowPart);
}

static
CODE_SEG("PAGE")
VOID
IsaProgramIrqSelect(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Index)
{
    UCHAR TypeSelect;

    PAGED_CODE();

    ASSERT(Descriptor->u.Interrupt.Level <= 15);

    if (Descriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
        TypeSelect = IRQTYPE_HIGH_EDGE;
    else
        TypeSelect = IRQTYPE_LOW_LEVEL;

    WriteByte(ISAPNP_IRQNO(Index), Descriptor->u.Interrupt.Level);
    WriteByte(ISAPNP_IRQTYPE(Index), TypeSelect);
}

static
CODE_SEG("PAGE")
VOID
IsaProgramDmaSelect(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Index)
{
    PAGED_CODE();

    ASSERT(Descriptor->u.Dma.Channel <= 7);

    WriteByte(ISAPNP_DMACHANNEL(Index), Descriptor->u.Dma.Channel);
}

static
CODE_SEG("PAGE")
NTSTATUS
IsaProgramMemoryDecoder(
    _In_ PUCHAR ReadDataPort,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ BOOLEAN IsMemory32,
    _In_ UCHAR Information,
    _In_ UCHAR Index)
{
    UCHAR MemoryControl;
    ULONG LengthLimit;

    PAGED_CODE();

    if (!IsMemory32)
    {
        /* The 24-bit memory address decoder always considers bits 0:7 to be zeros */
        if (Descriptor->u.Memory.Start.LowPart & 0xFF)
            return STATUS_INVALID_PARAMETER;

        if (Information & MEMRANGE_16_BIT_MEMORY_MASK)
            MemoryControl = MEMORY_USE_16_BIT_DECODER;
        else
            MemoryControl = MEMORY_USE_8_BIT_DECODER;

        if (ReadMemoryControl(ReadDataPort, Index) & MEMORY_UPPER_LIMIT)
        {
            MemoryControl |= MEMORY_UPPER_LIMIT;
            LengthLimit = Descriptor->u.Memory.Start.LowPart + Descriptor->u.Memory.Length;
        }
        else
        {
            LengthLimit = LENGTH_TO_RANGE_LENGTH(Descriptor->u.Memory.Length);
        }
        LengthLimit >>= 8;

        WriteWord(ISAPNP_MEMBASE(Index), Descriptor->u.Memory.Start.LowPart >> 8);
        WriteByte(ISAPNP_MEMCONTROL(Index), MemoryControl);
        WriteWord(ISAPNP_MEMLIMIT(Index), LengthLimit);
    }
    else
    {
        if ((Information & MEMRANGE_16_BIT_MEMORY_MASK) == MEMRANGE_32_BIT_MEMORY_ONLY)
            MemoryControl = MEMORY_USE_32_BIT_DECODER;
        else if (Information & MEMRANGE_16_BIT_MEMORY_MASK)
            MemoryControl = MEMORY_USE_16_BIT_DECODER;
        else
            MemoryControl = MEMORY_USE_8_BIT_DECODER;

        if (ReadMemoryControl32(ReadDataPort, Index) & MEMORY_UPPER_LIMIT)
        {
            MemoryControl |= MEMORY_UPPER_LIMIT;
            LengthLimit = Descriptor->u.Memory.Start.LowPart + Descriptor->u.Memory.Length;
        }
        else
        {
            LengthLimit = LENGTH_TO_RANGE_LENGTH(Descriptor->u.Memory.Length);
        }

        WriteDoubleWord(ISAPNP_MEMBASE32(Index), Descriptor->u.Memory.Start.LowPart);
        WriteByte(ISAPNP_MEMCONTROL32(Index), MemoryControl);
        WriteDoubleWord(ISAPNP_MEMLIMIT32(Index), LengthLimit);
    }

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
UCHAR
IsaHwTryReadDataPort(
    _In_ PUCHAR ReadDataPort)
{
    ULONG NumberOfRead = 0;
    UCHAR Csn = 0;

    PAGED_CODE();

    DPRINT("Setting read data port: 0x%p\n", ReadDataPort);

    SendKey();

    WriteByte(ISAPNP_CONFIGCONTROL,
              ISAPNP_CONFIG_RESET_CSN | ISAPNP_CONFIG_WAIT_FOR_KEY);
    KeStallExecutionProcessor(2000);

    SendKey();

    Wake(0x00);
    KeStallExecutionProcessor(1000);

    SetReadDataPort(ReadDataPort);

    Wake(0x00);

    while (TRUE)
    {
        ISAPNP_IDENTIFIER Identifier;
        UCHAR i, j;
        BOOLEAN Seen55aa = FALSE;

        EnterIsolationState();
        KeStallExecutionProcessor(1000);

        RtlZeroMemory(&Identifier, sizeof(Identifier));

        for (i = 0; i < sizeof(Identifier); i++)
        {
            UCHAR Byte = 0;

            for (j = 0; j < RTL_BITS_OF(Byte); j++)
            {
                USHORT Data;

                Data = ReadData(ReadDataPort) << 8;
                KeStallExecutionProcessor(250);
                Data |= ReadData(ReadDataPort);
                KeStallExecutionProcessor(250);

                Byte >>= 1;

                if (Data == 0x55AA)
                {
                    Byte |= 0x80;
                    Seen55aa = TRUE;
                }
            }

            ((PUCHAR)&Identifier)[i] = Byte;
        }

        ++NumberOfRead;

        if (Identifier.Checksum != 0x00 &&
            Identifier.Checksum != IsaPnpChecksum(&Identifier))
        {
            DPRINT("Bad checksum\n");
            break;
        }

        if (!Seen55aa)
        {
            DPRINT("Saw no sign of life\n");
            break;
        }

        Csn++;

        WriteCsn(Csn);
        KeStallExecutionProcessor(1000);

        Wake(0x00);
    }

    Wake(0x00);

    if (NumberOfRead == 1)
    {
        DPRINT("Trying next read data port\n");
        return 0;
    }
    else
    {
        DPRINT("Found %u cards at read port 0x%p\n", Csn, ReadDataPort);
        return Csn;
    }
}

_Requires_lock_held_(FdoExt->DeviceSyncEvent)
CODE_SEG("PAGE")
NTSTATUS
IsaHwFillDeviceList(
    _In_ PISAPNP_FDO_EXTENSION FdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDevice;
    UCHAR Csn;
    PLIST_ENTRY Entry;
    PUCHAR ResourceData;

    PAGED_CODE();
    ASSERT(FdoExt->ReadDataPort);

    DPRINT("%s for read port 0x%p\n", __FUNCTION__, FdoExt->ReadDataPort);

    ResourceData = ExAllocatePoolWithTag(PagedPool, ISAPNP_MAX_RESOURCEDATA, TAG_ISAPNP);
    if (!ResourceData)
    {
        DPRINT1("Failed to allocate memory for cache data\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (Entry = FdoExt->DeviceListHead.Flink;
         Entry != &FdoExt->DeviceListHead;
         Entry = Entry->Flink)
    {
        LogDevice = CONTAINING_RECORD(Entry, ISAPNP_LOGICAL_DEVICE, DeviceLink);

        LogDevice->Flags &= ~ISAPNP_PRESENT;
    }

    for (Csn = 1; Csn <= FdoExt->Cards; Csn++)
    {
        NTSTATUS Status;
        UCHAR TempId[3], LogDev;
        ISAPNP_IDENTIFIER Identifier;
        ULONG MaxTagsPerDevice;
        USHORT MaxLogDev;

        Wake(Csn);

        Peek(FdoExt->ReadDataPort, &Identifier, sizeof(Identifier));

        IsaPnpExtractAscii(TempId, Identifier.VendorId);
        Identifier.ProdId = RtlUshortByteSwap(Identifier.ProdId);

        Status = ReadTags(FdoExt->ReadDataPort,
                          ResourceData,
                          ISAPNP_MAX_RESOURCEDATA,
                          &MaxLogDev,
                          &MaxTagsPerDevice);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to read tags with status 0x%08lx, CSN %u\n", Status, Csn);
            continue;
        }

        DPRINT("Detected ISA PnP device - VID: '%.3s' PID: 0x%04x SN: 0x%08lX\n",
               TempId, Identifier.ProdId, Identifier.Serial);

        for (LogDev = 0; LogDev < MaxLogDev; LogDev++)
        {
            BOOLEAN IsAlreadyEnumerated = FALSE;

#ifndef UNIT_TEST
            for (Entry = FdoExt->DeviceListHead.Flink;
                 Entry != &FdoExt->DeviceListHead;
                 Entry = Entry->Flink)
            {
                LogDevice = CONTAINING_RECORD(Entry, ISAPNP_LOGICAL_DEVICE, DeviceLink);

                /* This logical device has already been enumerated */
                if ((LogDevice->SerialNumber == Identifier.Serial) &&
                    (RtlCompareMemory(LogDevice->VendorId, TempId, 3) == 3) &&
                    (LogDevice->ProdId == Identifier.ProdId) &&
                    (LogDevice->LDN == LogDev))
                {
                    LogDevice->Flags |= ISAPNP_PRESENT;

                    /* Assign a new CSN */
                    LogDevice->CSN = Csn;

                    if (LogDevice->Pdo)
                    {
                        PISAPNP_PDO_EXTENSION PdoExt = LogDevice->Pdo->DeviceExtension;

                        if (PdoExt->Common.State == dsStarted)
                            ActivateDevice(FdoExt->ReadDataPort, LogDev);
                    }

                    DPRINT("Skip CSN %u, LDN %u\n", LogDevice->CSN, LogDevice->LDN);
                    IsAlreadyEnumerated = TRUE;
                    break;
                }
            }
#endif /* UNIT_TEST */

            if (IsAlreadyEnumerated)
                continue;

            LogDevice = ExAllocatePoolZero(NonPagedPool, sizeof(ISAPNP_LOGICAL_DEVICE), TAG_ISAPNP);
            if (!LogDevice)
            {
                DPRINT1("Failed to allocate logical device!\n");
                goto Deactivate;
            }

            InitializeListHead(&LogDevice->CompatibleIdList);

            LogDevice->CSN = Csn;
            LogDevice->LDN = LogDev;

            LogDevice->Resources = ExAllocatePoolWithTag(PagedPool,
                                                         MaxTagsPerDevice * sizeof(ISAPNP_RESOURCE),
                                                         TAG_ISAPNP);
            if (!LogDevice->Resources)
            {
                DPRINT1("Failed to allocate the resources array\n");
                FreeLogicalDevice(LogDevice);
                goto Deactivate;
            }

            Status = ParseTags(ResourceData, LogDev, LogDevice);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to parse tags with status 0x%08lx, CSN %u, LDN %u\n",
                        Status, LogDevice->CSN, LogDevice->LDN);
                FreeLogicalDevice(LogDevice);
                goto Deactivate;
            }

            if (!ReadCurrentResources(FdoExt->ReadDataPort, LogDevice))
                DPRINT("Unable to read boot resources\n");

            IsaPnpExtractAscii(LogDevice->VendorId, Identifier.VendorId);
            LogDevice->ProdId = Identifier.ProdId;
            LogDevice->SerialNumber = Identifier.Serial;

            if (MaxLogDev > 1)
                LogDevice->Flags |= ISAPNP_HAS_MULTIPLE_LOGDEVS;

            LogDevice->Flags |= ISAPNP_PRESENT;

            InsertTailList(&FdoExt->DeviceListHead, &LogDevice->DeviceLink);
            FdoExt->DeviceCount++;

            /* Now we wait for the start device IRP */
Deactivate:
            DeactivateDevice(LogDev);
        }
    }

    ExFreePoolWithTag(ResourceData, TAG_ISAPNP);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
IsaHwConfigureDevice(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice,
    _In_ PCM_RESOURCE_LIST Resources)
{
    ULONG i;
    UCHAR NumberOfIo = 0,
          NumberOfIrq = 0,
          NumberOfDma = 0,
          NumberOfMemory = 0,
          NumberOfMemory32 = 0;

    PAGED_CODE();

    if (!Resources)
        return STATUS_INSUFFICIENT_RESOURCES;

    WriteLogicalDeviceNumber(LogicalDevice->LDN);

    for (i = 0; i < Resources->List[0].PartialResourceList.Count; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor =
            &Resources->List[0].PartialResourceList.PartialDescriptors[i];

        switch (Descriptor->Type)
        {
            case CmResourceTypePort:
            {
                if (NumberOfIo >= RTL_NUMBER_OF(LogicalDevice->Io))
                    return STATUS_INVALID_PARAMETER_1;

                IsaProgramIoDecoder(Descriptor, NumberOfIo++);
                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (NumberOfIrq >= RTL_NUMBER_OF(LogicalDevice->Irq))
                    return STATUS_INVALID_PARAMETER_2;

                IsaProgramIrqSelect(Descriptor, NumberOfIrq++);
                break;
            }

            case CmResourceTypeDma:
            {
                if (NumberOfDma >= RTL_NUMBER_OF(LogicalDevice->Dma))
                    return STATUS_INVALID_PARAMETER_3;

                IsaProgramDmaSelect(Descriptor, NumberOfDma++);
                break;
            }

            case CmResourceTypeMemory:
            {
                BOOLEAN IsMemory32;
                UCHAR Index, Information;
                NTSTATUS Status;

                if ((NumberOfMemory + NumberOfMemory32) >= RTL_NUMBER_OF(LogicalDevice->MemRange))
                    return STATUS_INVALID_PARAMETER_4;

                /*
                 * The PNP ROM provides an information byte for each memory descriptor
                 * which is then used to program the memory control register.
                 */
                if (!FindMemoryDescriptor(LogicalDevice,
                                          Descriptor->u.Memory.Start.LowPart,
                                          Descriptor->u.Memory.Start.LowPart +
                                          Descriptor->u.Memory.Length - 1,
                                          &Information))
                {
                    return STATUS_RESOURCE_DATA_NOT_FOUND;
                }

                /* We can have a 24- or 32-bit memory decoder, but not both */
                IsMemory32 = !!(LogicalDevice->Flags & ISAPNP_HAS_MEM32_DECODER);

                if (IsMemory32)
                    Index = NumberOfMemory32++;
                else
                    Index = NumberOfMemory++;

                Status = IsaProgramMemoryDecoder(FdoExt->ReadDataPort,
                                                 Descriptor,
                                                 IsMemory32,
                                                 Information,
                                                 Index);
                if (!NT_SUCCESS(Status))
                    return Status;

                break;
            }

            default:
                break;
        }
    }

    /* Disable the unclaimed device resources */
    for (i = NumberOfIo; i < RTL_NUMBER_OF(LogicalDevice->Io); i++)
    {
        WriteWord(ISAPNP_IOBASE(i), 0);
    }
    for (i = NumberOfIrq; i < RTL_NUMBER_OF(LogicalDevice->Irq); i++)
    {
        WriteByte(ISAPNP_IRQNO(i), 0);
        WriteByte(ISAPNP_IRQTYPE(i), 0);
    }
    for (i = NumberOfDma; i < RTL_NUMBER_OF(LogicalDevice->Dma); i++)
    {
        WriteByte(ISAPNP_DMACHANNEL(i), DMACHANNEL_NONE);
    }
    for (i = NumberOfMemory; i < RTL_NUMBER_OF(LogicalDevice->MemRange); i++)
    {
        WriteWord(ISAPNP_MEMBASE(i), 0);
        WriteByte(ISAPNP_MEMCONTROL(i), 0);
        WriteWord(ISAPNP_MEMLIMIT(i), 0);
    }
    for (i = NumberOfMemory32; i < RTL_NUMBER_OF(LogicalDevice->MemRange32); i++)
    {
        WriteDoubleWord(ISAPNP_MEMBASE32(i), 0);
        WriteByte(ISAPNP_MEMCONTROL32(i), 0);
        WriteDoubleWord(ISAPNP_MEMLIMIT32(i), 0);
    }

    KeStallExecutionProcessor(10000);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
IsaHwWakeDevice(
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice)
{
    PAGED_CODE();

    SendKey();
    Wake(LogicalDevice->CSN);
}

CODE_SEG("PAGE")
VOID
IsaHwActivateDevice(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice)
{
    PAGED_CODE();

    ActivateDevice(FdoExt->ReadDataPort, LogicalDevice->LDN);
}

#ifndef UNIT_TEST
CODE_SEG("PAGE")
VOID
IsaHwDeactivateDevice(
    _In_ PISAPNP_LOGICAL_DEVICE LogicalDevice)
{
    PAGED_CODE();

    DeactivateDevice(LogicalDevice->LDN);
}
#endif /* UNIT_TEST */

CODE_SEG("PAGE")
VOID
IsaHwWaitForKey(VOID)
{
    PAGED_CODE();

    WaitForKey();
}
