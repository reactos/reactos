/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/acpi/pcidiscovery.c
 * PURPOSE:         PCI Bus Hardware Detection for ACPI HAL
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/*
 * These includes are required to define
 * the ClassTable and VendorTable arrays.
 */
#define NEWLINE "\n"
#include "pci_classes.h"
#include "pci_vendors.h"

/* Interrupt vector redirect table (IRQ -> GSI mapping from MADT) */
extern ULONG HalpPicVectorRedirect[];

/* MCFG (PCI Express Enhanced Configuration Access Mechanism) table */
#define MCFG_SIGNATURE 'GFCM'

typedef struct _MCFG_ALLOCATION {
    ULONGLONG BaseAddress;
    USHORT PciSegmentGroup;
    UCHAR StartBusNumber;
    UCHAR EndBusNumber;
    ULONG Reserved;
} MCFG_ALLOCATION, *PMCFG_ALLOCATION;

/* PRIVATE FUNCTIONS **********************************************************/

CODE_SEG("INIT")
static
VOID
HalppShowSize(
    _In_ ULONGLONG Size)
{
    if (!Size) return;
    DbgPrint(" [size=");
    if (Size < 1024ULL)
    {
        DbgPrint("%u", (ULONG)Size);
    }
    else if (Size < 1048576ULL)
    {
        DbgPrint("%uK", (ULONG)(Size / 1024ULL));
    }
    else if (Size < 1073741824ULL)
    {
        DbgPrint("%uM", (ULONG)(Size / 1048576ULL));
    }
    else
    {
        DbgPrint("%uG", (ULONG)(Size / 1073741824ULL));
    }
    DbgPrint("]");
}

CODE_SEG("INIT")
static
VOID
HalppLookupClassName(
    _In_ UCHAR BaseClass,
    _In_ UCHAR SubClass,
    _Out_writes_z_(BufferSize) PCHAR Buffer,
    _In_ ULONG BufferSize)
{
    PCHAR ClassName, SubClassName, Boundary, p;
    CHAR LookupString[16];
    ULONG Length;

    /* Default */
    strncpy(Buffer, "Unknown", BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    /* Isolate the class name */
    sprintf(LookupString, "C %02x  ", BaseClass);
    ClassName = strstr((PCHAR)ClassTable, LookupString);
    if (!ClassName) return;

    /* Isolate the subclass name */
    ClassName += strlen("C 00  ");
    Boundary = strstr(ClassName, NEWLINE "C ");
    sprintf(LookupString, NEWLINE "\t%02x  ", SubClass);
    SubClassName = strstr(ClassName, LookupString);
    if (Boundary && SubClassName > Boundary)
    {
        SubClassName = NULL;
    }
    if (!SubClassName)
    {
        SubClassName = ClassName;
    }
    else
    {
        SubClassName += strlen(NEWLINE "\t00  ");
    }

    /* Copy the subclass name into our buffer */
    p = strpbrk(SubClassName, NEWLINE);
    if (!p) return;
    Length = (ULONG)(p - SubClassName);
    Length = min(Length, BufferSize - 1);
    strncpy(Buffer, SubClassName, Length);
    Buffer[Length] = '\0';
}

CODE_SEG("INIT")
static
VOID
HalppLookupVendorProduct(
    _In_ USHORT VendorId,
    _In_ USHORT DeviceId,
    _Out_writes_z_(VendorBufSize) PCHAR VendorBuf,
    _In_ ULONG VendorBufSize,
    _Out_writes_z_(ProductBufSize) PCHAR ProductBuf,
    _In_ ULONG ProductBufSize)
{
    PCHAR VendorName, ProductName, Boundary, p;
    CHAR LookupString[16];
    ULONG Length;

    /* Defaults */
    VendorBuf[0] = '\0';
    strncpy(ProductBuf, "Unknown device", ProductBufSize - 1);
    ProductBuf[ProductBufSize - 1] = '\0';

    /* Isolate the vendor name */
    sprintf(LookupString, NEWLINE "%04x  ", VendorId);
    VendorName = strstr((PCHAR)VendorTable, LookupString);
    if (!VendorName) return;

    /* Copy the vendor name into our buffer */
    VendorName += strlen(NEWLINE "0000  ");
    p = strpbrk(VendorName, NEWLINE);
    if (!p) return;
    Length = (ULONG)(p - VendorName);
    Length = min(Length, VendorBufSize - 1);
    strncpy(VendorBuf, VendorName, Length);
    VendorBuf[Length] = '\0';

    /* Find boundary for this vendor (next vendor entry) */
    p += strlen(NEWLINE);
    while (*p == '\t' || *p == '#')
    {
        p = strpbrk(p, NEWLINE);
        if (!p) return;
        p += strlen(NEWLINE);
    }
    Boundary = p;

    /* Isolate the product name */
    sprintf(LookupString, "\t%04x  ", DeviceId);
    ProductName = strstr(VendorName, LookupString);
    if (Boundary && ProductName >= Boundary)
    {
        ProductName = NULL;
    }
    if (!ProductName) return;

    /* Copy the product name into our buffer */
    ProductName += strlen("\t0000  ");
    p = strpbrk(ProductName, NEWLINE);
    if (!p) return;
    Length = (ULONG)(p - ProductName);
    Length = min(Length, ProductBufSize - 1);
    strncpy(ProductBuf, ProductName, Length);
    ProductBuf[Length] = '\0';
}

CODE_SEG("INIT")
static
VOID
HalppLookupSubsystem(
    _In_ USHORT VendorId,
    _In_ USHORT DeviceId,
    _In_ USHORT SubVendorId,
    _In_ USHORT SubSystemId,
    _Out_writes_z_(BufferSize) PCHAR Buffer,
    _In_ ULONG BufferSize)
{
    PCHAR VendorName, ProductName, SubVendorName, Boundary, p;
    CHAR LookupString[24];
    ULONG Length;

    /* Default */
    strncpy(Buffer, "Unknown", BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    /* Find vendor */
    sprintf(LookupString, NEWLINE "%04x  ", VendorId);
    VendorName = strstr((PCHAR)VendorTable, LookupString);
    if (!VendorName) return;
    VendorName += strlen(NEWLINE "0000  ");

    /* Find boundary for this vendor */
    p = strpbrk(VendorName, NEWLINE);
    if (!p) return;
    p += strlen(NEWLINE);
    while (*p == '\t' || *p == '#')
    {
        p = strpbrk(p, NEWLINE);
        if (!p) return;
        p += strlen(NEWLINE);
    }
    Boundary = p;

    /* Find device */
    sprintf(LookupString, "\t%04x  ", DeviceId);
    ProductName = strstr(VendorName, LookupString);
    if (!ProductName || (Boundary && ProductName >= Boundary)) return;
    ProductName += strlen("\t0000  ");

    /* Find boundary for this device (next device entry or next vendor) */
    p = strpbrk(ProductName, NEWLINE);
    if (!p) return;
    p += strlen(NEWLINE);
    while ((*p == '\t' && *(p + 1) == '\t') || *p == '#')
    {
        p = strpbrk(p, NEWLINE);
        if (!p) return;
        p += strlen(NEWLINE);
    }
    Boundary = p;

    /* Find subsystem */
    sprintf(LookupString, "\t\t%04x %04x  ", SubVendorId, SubSystemId);
    SubVendorName = strstr(ProductName, LookupString);
    if (!SubVendorName || (Boundary && SubVendorName >= Boundary)) return;

    /* Copy the subsystem name into our buffer */
    SubVendorName += strlen("\t\t0000 0000  ");
    p = strpbrk(SubVendorName, NEWLINE);
    if (!p) return;
    Length = (ULONG)(p - SubVendorName);
    Length = min(Length, BufferSize - 1);
    strncpy(Buffer, SubVendorName, Length);
    Buffer[Length] = '\0';
}

CODE_SEG("INIT")
static
VOID
HalppDumpInterruptRouting(
    _In_ ULONG Bus,
    _In_ PPCI_COMMON_CONFIG PciData)
{
    ULONG InterruptLine;
    ULONG Gsi;

    InterruptLine = PciData->u.type0.InterruptLine;
    if (PciData->u.type0.InterruptPin == 0 ||
        InterruptLine == 0 ||
        InterruptLine == 0xFF)
    {
        return;
    }

    /* Map through the PIC vector redirect table if within range */
    if (InterruptLine < 16)
    {
        Gsi = HalpPicVectorRedirect[InterruptLine];
    }
    else
    {
        Gsi = InterruptLine;
    }

    DbgPrint("    Interrupt: line %u -> GSI %u from firmware"
             " (seg 0 bus %u dev 255 fn 255 pin IN?)\n",
             InterruptLine, Gsi, Bus);
}

CODE_SEG("INIT")
static
VOID
HalppDumpBars(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_CONFIG PciData)
{
    UCHAR HeaderType;
    ULONG NumBars;
    ULONG b;
    ULONG Mem, PciBar, OriginalHigh;
    ULONG BarOffset;
    ULONGLONG Address64, Size64;
    ULONG PciBarHigh;

    HeaderType = PCI_CONFIGURATION_TYPE(PciData);
    if (HeaderType == PCI_DEVICE_TYPE)
        NumBars = PCI_TYPE0_ADDRESSES;
    else if (HeaderType == PCI_BRIDGE_TYPE)
        NumBars = PCI_TYPE1_ADDRESSES;
    else
        return;

    for (b = 0; b < NumBars; b++)
    {
        if (HeaderType != PCI_CARDBUS_BRIDGE_TYPE)
            Mem = PciData->u.type0.BaseAddresses[b];
        else
            Mem = 0;

        if (!Mem) continue;

        BarOffset = FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.BaseAddresses[b]);

        /* Probe BAR size */
        PciBar = 0xFFFFFFFF;
        HalpWritePCIConfig(BusHandler, PciSlot, &PciBar, BarOffset, sizeof(ULONG));
        HalpReadPCIConfig(BusHandler, PciSlot, &PciBar, BarOffset, sizeof(ULONG));
        HalpWritePCIConfig(BusHandler, PciSlot, &Mem, BarOffset, sizeof(ULONG));

        if (Mem & PCI_ADDRESS_IO_SPACE)
        {
            /* I/O BAR */
            Size64 = PciBar & ~(ULONG)0x3;
            Size64 = (~Size64 + 1) & 0xFFFF;
            DbgPrint("    I/O ports at %04lx", Mem & PCI_ADDRESS_IO_ADDRESS_MASK);
            HalppShowSize(Size64);
            DbgPrint("\n");
        }
        else
        {
            /* Memory BAR */
            ULONG MemType = (Mem & PCI_ADDRESS_MEMORY_TYPE_MASK);
            BOOLEAN Is64Bit = (MemType == PCI_TYPE_64BIT);
            BOOLEAN Prefetchable = (Mem & PCI_ADDRESS_MEMORY_PREFETCHABLE) ? TRUE : FALSE;

            if (Is64Bit && (b + 1) < NumBars)
            {
                /* 64-bit BAR: combine low and high parts */
                OriginalHigh = PciData->u.type0.BaseAddresses[b + 1];
                Address64 = ((ULONGLONG)OriginalHigh << 32) | (Mem & PCI_ADDRESS_MEMORY_ADDRESS_MASK);

                /* Probe high BAR */
                PciBarHigh = 0xFFFFFFFF;
                HalpWritePCIConfig(BusHandler, PciSlot,
                                   &PciBarHigh,
                                   BarOffset + sizeof(ULONG),
                                   sizeof(ULONG));
                HalpReadPCIConfig(BusHandler, PciSlot,
                                  &PciBarHigh,
                                  BarOffset + sizeof(ULONG),
                                  sizeof(ULONG));
                HalpWritePCIConfig(BusHandler, PciSlot,
                                   &OriginalHigh,
                                   BarOffset + sizeof(ULONG),
                                   sizeof(ULONG));

                Size64 = ((ULONGLONG)PciBarHigh << 32) | (PciBar & ~(ULONG)0xF);
                Size64 = ~Size64 + 1;

                DbgPrint("    Memory at %llx (64-bit, %sprefetchable)",
                         Address64,
                         Prefetchable ? "" : "non-");
                HalppShowSize(Size64);
                DbgPrint("\n");

                /* Skip the high BAR */
                b++;
            }
            else
            {
                /* 32-bit BAR */
                ULONG Size32 = PciBar & ~(ULONG)0xF;
                Size32 = ~Size32 + 1;

                DbgPrint("    Memory at %08lx (32-bit, %sprefetchable)",
                         Mem & PCI_ADDRESS_MEMORY_ADDRESS_MASK,
                         Prefetchable ? "" : "non-");
                HalppShowSize((ULONGLONG)Size32);
                DbgPrint("\n");
            }
        }
    }
}

CODE_SEG("INIT")
static
VOID
HalppDumpExpansionRom(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_CONFIG PciData)
{
    UCHAR HeaderType;
    ULONG RomOffset;
    ULONG RomValue, RomProbe, RomSize;
    ULONG RomAddress;
    BOOLEAN Enabled;

    HeaderType = PCI_CONFIGURATION_TYPE(PciData);
    if (HeaderType == PCI_DEVICE_TYPE)
    {
        RomOffset = FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.ROMBaseAddress);
        RomValue = PciData->u.type0.ROMBaseAddress;
    }
    else if (HeaderType == PCI_BRIDGE_TYPE)
    {
        RomOffset = FIELD_OFFSET(PCI_COMMON_HEADER, u.type1.ROMBaseAddress);
        RomValue = PciData->u.type1.ROMBaseAddress;
    }
    else
    {
        return;
    }

    RomAddress = RomValue & 0xFFFFF800;
    if (!RomAddress) return;

    Enabled = (RomValue & 1) ? TRUE : FALSE;

    /* Probe ROM size */
    RomProbe = 0xFFFFF800;
    HalpWritePCIConfig(BusHandler, PciSlot, &RomProbe, RomOffset, sizeof(ULONG));
    HalpReadPCIConfig(BusHandler, PciSlot, &RomProbe, RomOffset, sizeof(ULONG));
    HalpWritePCIConfig(BusHandler, PciSlot, &RomValue, RomOffset, sizeof(ULONG));

    RomProbe &= 0xFFFFF800;
    if (RomProbe)
    {
        RomSize = ~RomProbe + 1;
    }
    else
    {
        RomSize = 0;
    }

    DbgPrint("    Expansion ROM at %08lx [%s]",
             RomAddress,
             Enabled ? "enabled" : "disabled");
    HalppShowSize((ULONGLONG)RomSize);
    DbgPrint("\n");
}

CODE_SEG("INIT")
static
PCHAR
HalppGetExpressPortType(
    _In_ ULONG PortType)
{
    switch (PortType)
    {
        case 0:  return "Express Endpoint";
        case 1:  return "Legacy Endpoint";
        case 4:  return "Root Port of PCI Express Root Complex";
        case 5:  return "Upstream Port of PCI Express Switch";
        case 6:  return "Downstream Port of PCI Express Switch";
        case 7:  return "PCI Express to PCI/PCI-X Bridge";
        case 8:  return "PCI/PCI-X to PCI Express Bridge";
        case 9:  return "Root Complex Integrated Endpoint";
        case 10: return "Root Complex Event Collector";
        default: return "Unknown Express Device";
    }
}

CODE_SEG("INIT")
static
VOID
HalppDumpCapabilities(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_CONFIG PciData)
{
    UCHAR CapPtr, CapId, CapNext;
    USHORT MsgCtrl;
    ULONG CapCount;
    USHORT PmCaps;
    USHORT ExpCaps;
    ULONG PortType;
    ULONG Mmc, Mme;

    /* Check if capabilities list is supported */
    if (!(PciData->Status & PCI_STATUS_CAPABILITIES_LIST)) return;

    /* Get the capabilities pointer based on header type */
    if (PCI_CONFIGURATION_TYPE(PciData) == PCI_CARDBUS_BRIDGE_TYPE)
    {
        HalpReadPCIConfig(BusHandler, PciSlot, &CapPtr,
                          FIELD_OFFSET(PCI_COMMON_HEADER, u.type2.CapabilitiesPtr),
                          sizeof(UCHAR));
    }
    else
    {
        HalpReadPCIConfig(BusHandler, PciSlot, &CapPtr,
                          FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.CapabilitiesPtr),
                          sizeof(UCHAR));
    }

    /* Walk the capability list */
    CapCount = 0;
    while (CapPtr >= 0x40 && CapCount < 48)
    {
        /* Align to DWORD boundary */
        CapPtr &= ~(UCHAR)0x3;

        /* Read capability ID and next pointer */
        HalpReadPCIConfig(BusHandler, PciSlot, &CapId, CapPtr, sizeof(UCHAR));
        HalpReadPCIConfig(BusHandler, PciSlot, &CapNext, CapPtr + 1, sizeof(UCHAR));

        switch (CapId)
        {
            case 0x01: /* Power Management */
                HalpReadPCIConfig(BusHandler, PciSlot, &PmCaps,
                                  CapPtr + 2, sizeof(USHORT));
                DbgPrint("    Capabilities: [%02x] Power Management version %d\n",
                         CapPtr, PmCaps & 0x7);
                break;

            case 0x05: /* MSI */
                HalpReadPCIConfig(BusHandler, PciSlot, &MsgCtrl,
                                  CapPtr + 2, sizeof(USHORT));
                Mmc = 1 << ((MsgCtrl >> 1) & 0x7);
                Mme = 1 << ((MsgCtrl >> 4) & 0x7);
                DbgPrint("    Capabilities: [%02x] MSI: Enable%c Count=%d/%d"
                         " Maskable%c 64bit%c\n",
                         CapPtr,
                         (MsgCtrl & 0x0001) ? '+' : '-',
                         Mme, Mmc,
                         (MsgCtrl & 0x0100) ? '+' : '-',
                         (MsgCtrl & 0x0080) ? '+' : '-');
                break;

            case 0x09: /* Vendor Specific / Debug (VPD often shows as 09) */
                DbgPrint("    Capabilities: [%02x] Capability ID 09\n", CapPtr);
                break;

            case 0x10: /* PCI Express */
                HalpReadPCIConfig(BusHandler, PciSlot, &ExpCaps,
                                  CapPtr + 2, sizeof(USHORT));
                PortType = (ExpCaps >> 4) & 0xF;
                DbgPrint("    Capabilities: [%02x] Express %s, MSI %02x\n",
                         CapPtr,
                         HalppGetExpressPortType(PortType),
                         ExpCaps & 0xFF);
                break;

            case 0x11: /* MSI-X */
                HalpReadPCIConfig(BusHandler, PciSlot, &MsgCtrl,
                                  CapPtr + 2, sizeof(USHORT));
                DbgPrint("    Capabilities: [%02x] MSI-X: Enable%c Count=%d"
                         " Masked%c\n",
                         CapPtr,
                         (MsgCtrl & 0x8000) ? '+' : '-',
                         (MsgCtrl & 0x07FF) + 1,
                         (MsgCtrl & 0x4000) ? '+' : '-');
                break;

            case 0x12: /* Vendor Specific */
                DbgPrint("    Capabilities: [%02x] Capability ID 12\n", CapPtr);
                break;

            default:
                DbgPrint("    Capabilities: [%02x] Capability ID %02x\n",
                         CapPtr, CapId);
                break;
        }

        CapPtr = CapNext;
        CapCount++;

        if (CapPtr == 0) break;
    }
}

CODE_SEG("INIT")
static
VOID
HalppDumpExtendedCapabilities(
    _In_ PUCHAR EcamBase,
    _In_ ULONG Bus,
    _In_ ULONG Device,
    _In_ ULONG Function,
    _In_ UCHAR StartBus)
{
    PUCHAR DeviceBase;
    ULONG CapHeader;
    USHORT CapId;
    ULONG NextOffset;
    ULONG Offset;
    ULONG CapCount;
    ULONGLONG SerialNumber;
    PUCHAR Sn;

    /* Calculate device base in ECAM space */
    DeviceBase = EcamBase +
                 ((ULONG)(Bus - StartBus) << 20) +
                 ((ULONG)Device << 15) +
                 ((ULONG)Function << 12);

    /* Extended capabilities start at offset 0x100 */
    Offset = 0x100;
    CapCount = 0;

    while (Offset >= 0x100 && Offset < 0x1000 && CapCount < 48)
    {
        CapHeader = *(volatile ULONG *)(DeviceBase + Offset);
        if (CapHeader == 0 || CapHeader == 0xFFFFFFFF) break;

        CapId = (USHORT)(CapHeader & 0xFFFF);
        NextOffset = (CapHeader >> 20) & 0xFFC;

        switch (CapId)
        {
            case 0x0001: /* Advanced Error Reporting */
                DbgPrint("    Capabilities: [%03x] Advanced Error Reporting\n",
                         Offset);
                break;

            case 0x0003: /* Device Serial Number */
                SerialNumber = *(volatile ULONGLONG *)(DeviceBase + Offset + 4);
                Sn = (PUCHAR)&SerialNumber;
                DbgPrint("    Capabilities: [%03x] Device Serial Number"
                         " %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
                         Offset,
                         Sn[7], Sn[6], Sn[5], Sn[4],
                         Sn[3], Sn[2], Sn[1], Sn[0]);
                break;

            default:
                DbgPrint("    Capabilities: [%03x] Extended Capability ID %04x\n",
                         Offset, CapId);
                break;
        }

        CapCount++;
        if (NextOffset == 0) break;
        Offset = NextOffset;
    }
}

CODE_SEG("INIT")
static
VOID
HalppDumpDevice(
    _In_ PBUS_HANDLER BusHandler,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ ULONG Bus,
    _In_ ULONG Device,
    _In_ ULONG Function,
    _In_ PPCI_COMMON_CONFIG PciData,
    _In_opt_ PUCHAR EcamBase,
    _In_ UCHAR EcamStartBus)
{
    UCHAR HeaderType;
    CHAR bSubClassName[64];
    CHAR bVendorName[64];
    CHAR bProductName[128];
    CHAR bSubVendorName[128];

    HeaderType = PCI_CONFIGURATION_TYPE(PciData);

    /* Look up names from the PCI ID database */
    HalppLookupClassName(PciData->BaseClass, PciData->SubClass,
                         bSubClassName, sizeof(bSubClassName));
    HalppLookupVendorProduct(PciData->VendorID, PciData->DeviceID,
                             bVendorName, sizeof(bVendorName),
                             bProductName, sizeof(bProductName));

    /* Print device header line */
    DbgPrint("%02x:%02x.%x %s [%02x%02x]: %s %s [%04x:%04x] (rev %02x)\n",
             Bus, Device, Function,
             bSubClassName,
             PciData->BaseClass, PciData->SubClass,
             bVendorName, bProductName,
             PciData->VendorID, PciData->DeviceID,
             PciData->RevisionID);

    /* Print subsystem info for type 0 devices */
    if (HeaderType == PCI_DEVICE_TYPE)
    {
        HalppLookupSubsystem(PciData->VendorID, PciData->DeviceID,
                             PciData->u.type0.SubVendorID,
                             PciData->u.type0.SubSystemID,
                             bSubVendorName, sizeof(bSubVendorName));
        DbgPrint("    Subsystem: %s [%04x:%04x]\n",
                 bSubVendorName,
                 PciData->u.type0.SubVendorID,
                 PciData->u.type0.SubSystemID);
    }

    /* Print flags */
    DbgPrint("    Flags:");
    if (PciData->Command & PCI_ENABLE_BUS_MASTER) DbgPrint(" bus master,");
    if (PciData->Status & PCI_STATUS_66MHZ_CAPABLE) DbgPrint(" 66MHz,");
    if ((PciData->Status & PCI_STATUS_DEVSEL) == 0x000) DbgPrint(" fast devsel,");
    if ((PciData->Status & PCI_STATUS_DEVSEL) == 0x200) DbgPrint(" medium devsel,");
    if ((PciData->Status & PCI_STATUS_DEVSEL) == 0x400) DbgPrint(" slow devsel,");
    if ((PciData->Status & PCI_STATUS_DEVSEL) == 0x600) DbgPrint(" unknown devsel,");
    DbgPrint(" latency %d", PciData->LatencyTimer);
    if (PciData->u.type0.InterruptPin != 0 &&
        PciData->u.type0.InterruptLine != 0 &&
        PciData->u.type0.InterruptLine != 0xFF)
    {
        DbgPrint(", IRQ %d", PciData->u.type0.InterruptLine);
    }
    DbgPrint("\n");

    /* Print interrupt routing for devices with valid interrupt lines */
    HalppDumpInterruptRouting(Bus, PciData);

    /* Print bridge info */
    if (HeaderType == PCI_BRIDGE_TYPE)
    {
        DbgPrint("    Bridge:");
        DbgPrint(" primary bus %d,", PciData->u.type1.PrimaryBus);
        DbgPrint(" secondary bus %d,", PciData->u.type1.SecondaryBus);
        DbgPrint(" subordinate bus %d,", PciData->u.type1.SubordinateBus);
        DbgPrint(" secondary latency %d\n", PciData->u.type1.SecondaryLatency);
    }

    /* Dump BARs */
    HalppDumpBars(BusHandler, PciSlot, PciData);

    /* Dump Expansion ROM */
    HalppDumpExpansionRom(BusHandler, PciSlot, PciData);

    /* Dump PCI capabilities */
    HalppDumpCapabilities(BusHandler, PciSlot, PciData);

    /* Dump PCIe extended capabilities if ECAM is available */
    if (EcamBase)
    {
        HalppDumpExtendedCapabilities(EcamBase, Bus, Device, Function,
                                      EcamStartBus);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

CODE_SEG("INIT")
VOID
NTAPI
HalpAcpiPciDiscovery(VOID)
{
    ULONG Bus, Device, Function;
    PCI_SLOT_NUMBER PciSlot;
    UCHAR DataBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciData = (PPCI_COMMON_CONFIG)DataBuffer;
    BUS_HANDLER TempHandler;
    PDESCRIPTION_HEADER McfgHeader;
    PMCFG_ALLOCATION McfgAlloc;
    ULONG McfgEntries;
    PUCHAR EcamBase = NULL;
    UCHAR EcamStartBus = 0;
    ULONGLONG EcamPhysBase = 0;
    ULONG EcamPages = 0;
    PHYSICAL_ADDRESS PhysAddr;
    BOOLEAN WildcardSegment = FALSE;
    ULONG VendorDword;
    USHORT VendorWord;

    /* Verify PCI configuration is ready */
    if (!HalpPCIConfigInitialized)
    {
        DbgPrint("HAL: PCI config not initialized, skipping discovery.\n");
        return;
    }

    /* Try to locate the MCFG table for PCIe ECAM access */
    McfgHeader = HalpAcpiGetCachedTable(MCFG_SIGNATURE);
    if (McfgHeader)
    {
        /* MCFG layout: header + 8 reserved bytes + allocation array */
        McfgEntries = (McfgHeader->Length - sizeof(DESCRIPTION_HEADER) - 8) /
                      sizeof(MCFG_ALLOCATION);
        if (McfgEntries > 0)
        {
            McfgAlloc = (PMCFG_ALLOCATION)((PUCHAR)McfgHeader +
                         sizeof(DESCRIPTION_HEADER) + 8);

            /* Use the first allocation entry */
            EcamPhysBase = McfgAlloc->BaseAddress;
            EcamStartBus = McfgAlloc->StartBusNumber;

            /* Check if wildcard segment was used */
            if (McfgAlloc->PciSegmentGroup == 0)
            {
                WildcardSegment = TRUE;
            }

            /* Calculate pages needed: 4K per function, 256 buses max */
            EcamPages = (ULONG)(((ULONGLONG)(McfgAlloc->EndBusNumber -
                         McfgAlloc->StartBusNumber + 1) << 20) >> PAGE_SHIFT);

            /* Map the ECAM region */
            PhysAddr.QuadPart = EcamPhysBase;
            EcamBase = (PUCHAR)HalpMapPhysicalMemory64(PhysAddr, EcamPages);
            if (!EcamBase)
            {
                DbgPrint("HAL: Failed to map ECAM region at %llx\n",
                         EcamPhysBase);
            }
        }
    }

    /* Print banner */
    DbgPrint("\n====== PCI BUS HARDWARE DETECTION (ACPI HAL) =======\n\n");

    /* Enumerate all buses, devices, and functions */
    PciSlot.u.AsULONG = 0;
    for (Bus = HalpMinPciBus; Bus <= HalpMaxPciBus; Bus++)
    {
        /* Setup a temporary bus handler for this bus */
        RtlCopyMemory(&TempHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        TempHandler.BusNumber = Bus;

        for (Device = 0; Device < 32; Device++)
        {
            PciSlot.u.bits.DeviceNumber = Device;

            for (Function = 0; Function < 8; Function++)
            {
                PciSlot.u.bits.FunctionNumber = Function;

                /* Read the PCI configuration header */
                RtlZeroMemory(DataBuffer, PCI_COMMON_HDR_LENGTH);
                HalpReadPCIConfig(&TempHandler, PciSlot, PciData,
                                  0, PCI_COMMON_HDR_LENGTH);

                /* Skip invalid devices */
                if (PciData->VendorID == PCI_INVALID_VENDORID)
                {
                    /* If function 0 is invalid, skip remaining functions */
                    if (Function == 0) break;
                    continue;
                }

                /* For function 0, check if device is multi-function */
                if (Function == 0)
                {
                    if (!(PciData->HeaderType & PCI_MULTIFUNCTION))
                    {
                        /* Single-function device: dump it and move to next device */
                        HalppDumpDevice(&TempHandler, PciSlot, Bus, Device,
                                        Function, PciData, EcamBase, EcamStartBus);
                        break;
                    }
                }

                /* Dump this device/function */
                HalppDumpDevice(&TempHandler, PciSlot, Bus, Device, Function,
                                PciData, EcamBase, EcamStartBus);
            }
        }
    }

    /* Try to probe bus beyond the last known PCI bus */
    RtlCopyMemory(&TempHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    TempHandler.BusNumber = HalpMaxPciBus + 1;
    PciSlot.u.AsULONG = 0;

    /* Read the full DWORD at offset 0 to get vendor/device as 32 bits */
    VendorDword = 0xFFFFFFFF;
    HalpReadPCIConfig(&TempHandler, PciSlot, &VendorDword, 0, sizeof(ULONG));
    VendorWord = (USHORT)(VendorDword & 0xFFFF);

    if (VendorWord == PCI_INVALID_VENDORID)
    {
        DbgPrint("(%s:%d) HAL: Legacy config probe failed for bus %lu"
                 " (DWORD vendor=0x%08lx, WORD vendor=0x%04x)\n",
                 "\\hal\\halx86\\acpi\\pcidiscovery.c",
                 __LINE__,
                 HalpMaxPciBus + 1,
                 VendorDword,
                 VendorWord);
    }

    /* Print ECAM status if MCFG was found */
    if (McfgHeader)
    {
        DbgPrint("HAL: PCI Express MMCONFIG (ECAM) active for configuration space.\n");
        if (WildcardSegment)
        {
            DbgPrint("HAL:   ECAM note: callers used wildcard segment selection.\n");
        }
    }

    /* Unmap ECAM if it was mapped */
    if (EcamBase)
    {
        HalpUnmapVirtualAddress(EcamBase, EcamPages);
    }

    /* Print footer */
    DbgPrint("\n====== END PCI BUS DETECTION =======\n\n");
}

/* EOF */
