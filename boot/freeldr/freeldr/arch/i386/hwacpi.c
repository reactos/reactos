/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        boot/freeldr/freeldr/arch/i386/hwacpi.c
 * PURPOSE:     ACPI detection and firmware watchdog shutdown
 * COPYRIGHT:   Copyright 2004 Eric Kohl
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Eric Kohl
 *              Ryan Wilson <ryan@verosnt.com>
 */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

BOOLEAN AcpiPresent = FALSE;

BOOLEAN IsAcpiPresent(VOID)
{
    return AcpiPresent;
}

static PRSDP_DESCRIPTOR
FindAcpiBios(VOID)
{
    PUCHAR Ptr;

    /* Find the 'Root System Descriptor Table Pointer' */
    Ptr = (PUCHAR)0xE0000;
    while ((ULONG_PTR)Ptr < 0x100000)
    {
        if (!memcmp(Ptr, "RSD PTR ", 8))
        {
            TRACE("ACPI supported\n");

            return (PRSDP_DESCRIPTOR)Ptr;
        }

        Ptr = (PUCHAR)((ULONG_PTR)Ptr + 0x10);
    }

    TRACE("ACPI not supported\n");

    return NULL;
}


VOID
DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PRSDP_DESCRIPTOR Rsdp;
    PACPI_BIOS_DATA AcpiBiosData;
    ULONG TableSize, Size;

    Rsdp = FindAcpiBios();

    if (Rsdp)
    {
        /* Set up the flag in the loader block */
        AcpiPresent = TRUE;

        /* Calculate the table size */
        TableSize = PcBiosMapCount * sizeof(BIOS_MEMORY_MAP) +
            sizeof(ACPI_BIOS_DATA) - sizeof(BIOS_MEMORY_MAP);

        /* Set 'Configuration Data' value */
        Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[1]) + TableSize;
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }

        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 0;
        PartialResourceList->Revision = 0;
        PartialResourceList->Count = 1;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->u.DeviceSpecificData.DataSize = TableSize;

        /* Fill the table */
        AcpiBiosData = (PACPI_BIOS_DATA)(PartialDescriptor + 1);

        if (Rsdp->revision > 0)
        {
            TRACE("ACPI >1.0, using XSDT address\n");
            AcpiBiosData->RSDTAddress.QuadPart = Rsdp->xsdt_physical_address;
        }
        else
        {
            TRACE("ACPI 1.0, using RSDT address\n");
            AcpiBiosData->RSDTAddress.LowPart = Rsdp->rsdt_physical_address;
        }

        AcpiBiosData->Count = PcBiosMapCount;
        RtlCopyMemory(AcpiBiosData->MemoryMap, PcBiosMemoryMap,
                      PcBiosMapCount * sizeof(BIOS_MEMORY_MAP));

        TRACE("RSDT %p, data size %x\n", Rsdp->rsdt_physical_address,
            TableSize);

        /* Create new bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0,
                               0,
                               0xFFFFFFFF,
                               "ACPI BIOS",
                               PartialResourceList,
                               Size,
                               &BiosKey);

        /* Increment bus number */
        (*BusNumber)++;
    }
}

#include <pshpack1.h>
typedef struct
{
    CHAR Signature[4];
    ULONG Length;
    UCHAR Revision;
    UCHAR Checksum;
    CHAR OemId[6];
    CHAR OemTableId[8];
    ULONG OemRevision;
    CHAR CreatorId[4];
    ULONG CreatorRevision;
} PC_ACPI_HEADER;

typedef struct
{
    UCHAR SpaceId;
    UCHAR BitWidth;
    UCHAR BitOffset;
    UCHAR AccessWidth;
    ULONGLONG Address;
} PC_ACPI_GAS;

typedef struct
{
    PC_ACPI_HEADER Header;
    ULONG HeaderLength;
    USHORT PciSegment;
    UCHAR PciBus;
    UCHAR PciDevice;
    UCHAR PciFunction;
    UCHAR Reserved[3];
    ULONG TimerPeriod;
    ULONG MaxCount;
    ULONG MinCount;
    UCHAR Flags;
    UCHAR Reserved2[3];
    ULONG Entries;
} PC_WDAT_TABLE;

typedef struct
{
    UCHAR Action;
    UCHAR Instruction;
    USHORT Reserved;
    PC_ACPI_GAS RegisterRegion;
    ULONG Value;
    ULONG Mask;
} PC_WDAT_ENTRY;
#include <poppack.h>

#define PC_WDAT_SET_COUNTDOWN 6
#define PC_WDAT_SET_STOPPED   11
#define PC_WDAT_SET_STATUS    33
#define PC_WDAT_WRITE_VALUE     2
#define PC_WDAT_WRITE_COUNTDOWN 3
#define PC_WDAT_PRESERVE        0x80
#define PC_WDAT_SPACE_MEMORY 0
#define PC_WDAT_SPACE_IO     1

#ifdef _M_AMD64
static DECLSPEC_ALIGN(PAGE_SIZE) UCHAR WdatPd[3][PAGE_SIZE];

/*
 * The real-mode startup map covers only the first 1 GB. The Acer WDAT
 * table lives just below 2 GB, so give that 2 MB page a large-page entry.
 */
static BOOLEAN
PcMapPhysical2Mb(ULONGLONG Physical)
{
    PULONGLONG Pdp = (PULONGLONG)(ULONG_PTR)PDP_ADDRESS;
    ULONG Slot = (ULONG)((Physical >> 30) & 0x1FF);
    ULONG Index = (ULONG)((Physical >> 21) & 0x1FF);
    ULONGLONG Base = Physical & ~0x1FFFFFULL;
    PULONGLONG Pd;

    if (Slot == 0)
        return TRUE;
    if (Slot > 3)
        return FALSE;

    Pd = (PULONGLONG)WdatPd[Slot - 1];
    if ((Pdp[Slot] & ~0xFFFULL) != (ULONGLONG)(ULONG_PTR)Pd)
        RtlZeroMemory(Pd, PAGE_SIZE);

    Pd[Index] = Base | 0x8F;
    Pdp[Slot] = ((ULONGLONG)(ULONG_PTR)Pd) | 0x0F;
    __writecr3(__readcr3());
    return TRUE;
}
#else
static BOOLEAN
PcMapPhysical2Mb(ULONGLONG Physical)
{
    return Physical < 0x100000000ULL;
}
#endif

static UCHAR
PcGasBytes(const PC_ACPI_GAS *Gas)
{
    if (Gas->AccessWidth == 1)
        return 1;
    if (Gas->AccessWidth == 2)
        return 2;
    if (Gas->AccessWidth == 3 || Gas->AccessWidth == 4)
        return 4;
    if (Gas->BitWidth <= 8)
        return 1;
    if (Gas->BitWidth <= 16)
        return 2;
    return 4;
}

static BOOLEAN
PcReadRegister(const PC_ACPI_GAS *Gas, PULONG Value)
{
    UCHAR Bytes = PcGasBytes(Gas);
    ULONG_PTR Address = (ULONG_PTR)Gas->Address;

    if (Gas->SpaceId == PC_WDAT_SPACE_MEMORY)
    {
        if (!PcMapPhysical2Mb(Gas->Address))
            return FALSE;
        if (Bytes == 1)
            *Value = *(volatile UCHAR *)Address;
        else if (Bytes == 2)
            *Value = *(volatile USHORT *)Address;
        else
            *Value = *(volatile ULONG *)Address;
        return TRUE;
    }

    if (Gas->SpaceId != PC_WDAT_SPACE_IO)
        return FALSE;

    if (Bytes == 1)
        *Value = READ_PORT_UCHAR((PUCHAR)Address);
    else if (Bytes == 2)
        *Value = READ_PORT_USHORT((PUSHORT)Address);
    else
        *Value = READ_PORT_ULONG((PULONG)Address);
    return TRUE;
}

static BOOLEAN
PcWriteRegister(const PC_ACPI_GAS *Gas, ULONG Value)
{
    UCHAR Bytes = PcGasBytes(Gas);
    ULONG_PTR Address = (ULONG_PTR)Gas->Address;

    if (Gas->SpaceId == PC_WDAT_SPACE_MEMORY)
    {
        if (!PcMapPhysical2Mb(Gas->Address))
            return FALSE;
        if (Bytes == 1)
            *(volatile UCHAR *)Address = (UCHAR)Value;
        else if (Bytes == 2)
            *(volatile USHORT *)Address = (USHORT)Value;
        else
            *(volatile ULONG *)Address = Value;
        return TRUE;
    }

    if (Gas->SpaceId != PC_WDAT_SPACE_IO)
        return FALSE;

    if (Bytes == 1)
        WRITE_PORT_UCHAR((PUCHAR)Address, (UCHAR)Value);
    else if (Bytes == 2)
        WRITE_PORT_USHORT((PUSHORT)Address, (USHORT)Value);
    else
        WRITE_PORT_ULONG((PULONG)Address, Value);
    return TRUE;
}

static BOOLEAN
PcRsdpChecksumOk(const RSDP_DESCRIPTOR *Rsdp)
{
    const UCHAR *Bytes = (const UCHAR *)Rsdp;
    ULONG Sum = 0;
    ULONG Index;
    ULONG Length;

    for (Index = 0; Index < 20; Index++)
        Sum += Bytes[Index];
    if ((Sum & 0xFF) != 0)
        return FALSE;
    if (Rsdp->revision == 0)
        return TRUE;

    Length = Rsdp->length;
    if (Length < sizeof(RSDP_DESCRIPTOR) || Length > 256)
        return FALSE;
    Sum = 0;
    for (Index = 0; Index < Length; Index++)
        Sum += Bytes[Index];
    return (Sum & 0xFF) == 0;
}

static ULONG
PcPciRead32(UCHAR Bus, UCHAR Device, UCHAR Function, UCHAR Offset)
{
    ULONG Address;

    Address = 0x80000000UL |
              ((ULONG)Bus << 16) |
              ((ULONG)Device << 11) |
              ((ULONG)Function << 8) |
              (Offset & 0xFC);
    WRITE_PORT_ULONG((PULONG)0xCF8, Address);
    return READ_PORT_ULONG((PULONG)0xCFC);
}

/*
 * NM10 (the Acer LPC, 8086:27BC) counts in the ICH7 TCO block. A BIOS call
 * reloads it. The first expiry raises an SMI; if that SMI sees the halt bit
 * already set, Insyde spins and the second expiry resets the CPU. Silence
 * the SMI, clear both timeout bits, then halt. QEMU has no such device.
 *
 * TCO1_STS + 0x04 bit 0x0008, TCO2_STS + 0x06 bits 0x0002 and 0x0004, and
 * TCO1_CNT + 0x08 bit 0x0800 are in Intel AP-727:
 * https://uefi.org/sites/default/files/resources/Watchdog%20Descriptor%20Table.pdf
 * TCOv2_TMR + 0x12, SMI_EN at PMBASE + 0x30 bit 13, and TCO base PMBASE + 0x60
 * are in https://github.com/torvalds/linux/blob/master/drivers/watchdog/iTCO_wdt.c
 * That file names Intel document 322896-001 for NM10.
 */
static VOID
PcHaltIchTco(VOID)
{
    ULONG Id;
    ULONG PmBase;
    USHORT TcoBase;
    USHORT Value;
    ULONG SmiEn;

    Id = PcPciRead32(0, 31, 0, 0);
    if ((USHORT)Id != 0x8086)
        return;
    if ((USHORT)(Id >> 16) < 0x27B8 || (USHORT)(Id >> 16) > 0x27BD)
        return;

    PmBase = PcPciRead32(0, 31, 0, 0x40) & 0xFF80;
    if (PmBase < 0x80 || PmBase > 0xFF00)
        return;

    TcoBase = (USHORT)(PmBase + 0x60);

    SmiEn = READ_PORT_ULONG((PULONG)(ULONG_PTR)(PmBase + 0x30));
    SmiEn &= ~0x2000UL;
    WRITE_PORT_ULONG((PULONG)(ULONG_PTR)(PmBase + 0x30), SmiEn);

    WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)(TcoBase + 0x04), 0x0008);
    WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)(TcoBase + 0x06), 0x0002);
    WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)(TcoBase + 0x06), 0x0004);

    Value = READ_PORT_USHORT((PUSHORT)(ULONG_PTR)(TcoBase + 0x12));
    Value = (Value & 0xFC00) | 0x03FF;
    WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)(TcoBase + 0x12), Value);

    Value = READ_PORT_USHORT((PUSHORT)(ULONG_PTR)(TcoBase + 0x08));
    Value |= 0x0800;
    WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)(TcoBase + 0x08), Value);
}

static VOID
PcRunWdatAction(PC_WDAT_TABLE *Table, UCHAR Action, ULONG Countdown)
{
    PC_WDAT_ENTRY *Entry;
    ULONG Count;
    ULONG Index;
    ULONG Start;

    Start = sizeof(PC_WDAT_TABLE);
    if (Table->HeaderLength > Start && Table->HeaderLength < Table->Header.Length)
        Start = Table->HeaderLength;

    Count = (Table->Header.Length - Start) / sizeof(PC_WDAT_ENTRY);
    if (Count > Table->Entries)
        Count = Table->Entries;

    Entry = (PC_WDAT_ENTRY *)((PUCHAR)Table + Start);
    for (Index = 0; Index < Count; Index++)
    {
        UCHAR Instruction = Entry[Index].Instruction;
        ULONG Mask = Entry[Index].Mask;
        ULONG Written;
        ULONG Current;
        UCHAR Shift = Entry[Index].RegisterRegion.BitOffset;

        if (Entry[Index].Action != Action)
            continue;
        if (Mask == 0)
        {
            UCHAR Bits = PcGasBytes(&Entry[Index].RegisterRegion) * 8;
            Mask = (Bits >= 32) ? 0xFFFFFFFF : (0xFFFFFFFF >> (32 - Bits));
        }
        if (Shift >= 32)
            continue;

        Instruction &= ~PC_WDAT_PRESERVE;
        if (Instruction == PC_WDAT_WRITE_COUNTDOWN)
            Written = Countdown & Mask;
        else if (Instruction == PC_WDAT_WRITE_VALUE)
            Written = Entry[Index].Value & Mask;
        else
            continue;

        Current = 0;
        if (Entry[Index].Instruction & PC_WDAT_PRESERVE)
        {
            if (!PcReadRegister(&Entry[Index].RegisterRegion, &Current))
                continue;
            Current &= ~(Mask << Shift);
        }

        Current |= Written << Shift;
        PcWriteRegister(&Entry[Index].RegisterRegion, Current);
    }
}

VOID
PcStopFirmwareWatchdog(VOID)
{
    PUCHAR Ptr;
    PRSDP_DESCRIPTOR Rsdp;
    PC_ACPI_HEADER *Root;
    PC_WDAT_TABLE *Wdat;
    ULONG EntrySize;
    ULONG Count;
    ULONG Index;

    /* Port halt first. A bad ACPI map must not skip it. */
    PcHaltIchTco();

    Rsdp = NULL;
    Ptr = (PUCHAR)0xE0000;
    while ((ULONG_PTR)Ptr < 0x100000)
    {
        if (!memcmp(Ptr, "RSD PTR ", 8) &&
            PcRsdpChecksumOk((PRSDP_DESCRIPTOR)Ptr))
        {
            Rsdp = (PRSDP_DESCRIPTOR)Ptr;
            break;
        }
        Ptr += 0x10;
    }
    if (Rsdp == NULL)
        return;

    if (Rsdp->revision > 0 && Rsdp->xsdt_physical_address != 0)
    {
        if (!PcMapPhysical2Mb(Rsdp->xsdt_physical_address))
            return;
        Root = (PC_ACPI_HEADER *)(ULONG_PTR)Rsdp->xsdt_physical_address;
        EntrySize = sizeof(ULONGLONG);
    }
    else
    {
        if (!PcMapPhysical2Mb(Rsdp->rsdt_physical_address))
            return;
        Root = (PC_ACPI_HEADER *)(ULONG_PTR)Rsdp->rsdt_physical_address;
        EntrySize = sizeof(ULONG);
    }

    if (Root->Length < sizeof(PC_ACPI_HEADER) || Root->Length > 0x10000)
        return;
    if (memcmp(Root->Signature, EntrySize == sizeof(ULONGLONG) ? "XSDT" : "RSDT", 4) != 0)
        return;

    Count = (Root->Length - sizeof(PC_ACPI_HEADER)) / EntrySize;
    Wdat = NULL;
    for (Index = 0; Index < Count; Index++)
    {
        ULONGLONG TablePa;
        PC_ACPI_HEADER *Table;

        if (EntrySize == sizeof(ULONGLONG))
            TablePa = ((PULONGLONG)(Root + 1))[Index];
        else
            TablePa = ((PULONG)(Root + 1))[Index];

        if (!PcMapPhysical2Mb(TablePa))
            continue;
        Table = (PC_ACPI_HEADER *)(ULONG_PTR)TablePa;
        if (memcmp(Table->Signature, "WDAT", 4) == 0)
        {
            Wdat = (PC_WDAT_TABLE *)Table;
            break;
        }
    }

    if (Wdat == NULL || Wdat->Header.Length < sizeof(PC_WDAT_TABLE))
        return;

    TRACE("WDAT at %p, period %lu ms, max %lu, flags 0x%x\n",
          Wdat, Wdat->TimerPeriod, Wdat->MaxCount, Wdat->Flags);
    /*
     * Status clear, then halt. Do not reload the countdown: that write
     * starts the count again, and the port halt below has to win.
     */
    PcRunWdatAction(Wdat, PC_WDAT_SET_STATUS, 0);
    PcRunWdatAction(Wdat, PC_WDAT_SET_STOPPED, 0);
    PcHaltIchTco();
    TRACE("WDAT stopped\n");
}

VOID
PcUnmapReservedBootRanges(VOID)
{
#ifdef _M_AMD64
    ULONG Index;
    PULONGLONG Pd = (PULONGLONG)(ULONG_PTR)PD_ADDRESS;

    for (Index = 0; Index < PcBiosMapCount; Index++)
    {
        ULONGLONG Start;
        ULONGLONG End;
        ULONGLONG Page;

        if (PcBiosMemoryMap[Index].Type != BiosMemoryReserved &&
            PcBiosMemoryMap[Index].Type != BiosMemoryUnusable)
        {
            continue;
        }

        Start = PcBiosMemoryMap[Index].BaseAddress;
        End = Start + PcBiosMemoryMap[Index].Length;
        if (Start >= 0x40000000ULL)
            continue;
        if (End > 0x40000000ULL)
            End = 0x40000000ULL;

        /* First 2 MB page wholly inside the reserved run. */
        Page = (Start + 0x1FFFFFULL) & ~0x1FFFFFULL;
        while (Page + 0x200000ULL <= End)
        {
            Pd[Page >> 21] = 0;
            Page += 0x200000ULL;
        }
    }

    __writecr3(__readcr3());
#endif
}

/* EOF */
