/*
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include <cportlib/cportlib.h>

#include "../ntldr/ntldropts.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);


/* Maximum number of COM and LPT ports */
#define MAX_COM_PORTS   4
#define MAX_LPT_PORTS   3

/* No Mouse */
#define MOUSE_TYPE_NONE            0
/* Microsoft Mouse with 2 buttons */
#define MOUSE_TYPE_MICROSOFT       1
/* Logitech Mouse with 3 buttons */
#define MOUSE_TYPE_LOGITECH        2
/* Microsoft Wheel Mouse (aka Z Mouse) */
#define MOUSE_TYPE_WHEELZ          3
/* Mouse Systems Mouse */
#define MOUSE_TYPE_MOUSESYSTEMS    4

#define INPORT_REGISTER_CONTROL    0x00
#define INPORT_REGISTER_DATA       0x01
#define INPORT_REGISTER_SIGNATURE  0x02

#define INPORT_REG_MODE            0x07
#define INPORT_RESET               0x80
#define INPORT_MODE_BASE           0x10
#define INPORT_TEST_IRQ            0x16
#define INPORT_SIGNATURE           0xDE

#define PIC1_CONTROL_PORT          0x20
#define PIC1_DATA_PORT             0x21
#define PIC2_CONTROL_PORT          0xA0
#define PIC2_DATA_PORT             0xA1

/* PS2 stuff */

/* Controller registers. */
#define CONTROLLER_REGISTER_STATUS                      0x64
#define CONTROLLER_REGISTER_CONTROL                     0x64
#define CONTROLLER_REGISTER_DATA                        0x60

/* Controller commands. */
#define CONTROLLER_COMMAND_READ_MODE                    0x20
#define CONTROLLER_COMMAND_WRITE_MODE                   0x60
#define CONTROLLER_COMMAND_GET_VERSION                  0xA1
#define CONTROLLER_COMMAND_MOUSE_DISABLE                0xA7
#define CONTROLLER_COMMAND_MOUSE_ENABLE                 0xA8
#define CONTROLLER_COMMAND_TEST_MOUSE                   0xA9
#define CONTROLLER_COMMAND_SELF_TEST                    0xAA
#define CONTROLLER_COMMAND_KEYBOARD_TEST                0xAB
#define CONTROLLER_COMMAND_KEYBOARD_DISABLE             0xAD
#define CONTROLLER_COMMAND_KEYBOARD_ENABLE              0xAE
#define CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER    0xD3
#define CONTROLLER_COMMAND_WRITE_MOUSE                  0xD4

/* Controller status */
#define CONTROLLER_STATUS_OUTPUT_BUFFER_FULL            0x01
#define CONTROLLER_STATUS_INPUT_BUFFER_FULL             0x02
#define CONTROLLER_STATUS_SELF_TEST                     0x04
#define CONTROLLER_STATUS_COMMAND                       0x08
#define CONTROLLER_STATUS_UNLOCKED                      0x10
#define CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL      0x20
#define CONTROLLER_STATUS_GENERAL_TIMEOUT               0x40
#define CONTROLLER_STATUS_PARITY_ERROR                  0x80
#define AUX_STATUS_OUTPUT_BUFFER_FULL                   (CONTROLLER_STATUS_OUTPUT_BUFFER_FULL | \
                                                         CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL)

/* Timeout in ms for sending to keyboard controller. */
#define CONTROLLER_TIMEOUT                              250


VOID
PcGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
    REGS BiosRegs;

    /* Get address and size of the extended BIOS data area */
    BiosRegs.d.eax = 0xC100;
    Int386(0x15, &BiosRegs, &BiosRegs);
    if (INT386_SUCCESS(BiosRegs))
    {
        *ExtendedBIOSDataArea = BiosRegs.w.es << 4;
        *ExtendedBIOSDataSize = 1024;
    }
    else
    {
        WARN("Int 15h AH=C1h call failed\n");
        *ExtendedBIOSDataArea = 0;
        *ExtendedBIOSDataSize = 0;
    }
}

// NOTE: Similar to machxbox.c!XboxGetHarddiskConfigurationData(),
// but with extended geometry support.
static
PCM_PARTIAL_RESOURCE_LIST
PcGetHarddiskConfigurationData(UCHAR DriveNumber, ULONG* pSize)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
    GEOMETRY Geometry;
    ULONG Size;

    /* Initialize returned size */
    *pSize = 0;

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return NULL;
    }

    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;
    PartialResourceList->PartialDescriptors[0].Type =
        CmResourceTypeDeviceSpecific;
//  PartialResourceList->PartialDescriptors[0].ShareDisposition =
//  PartialResourceList->PartialDescriptors[0].Flags =
    PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
        sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);

    /* Get pointer to geometry data */
    DiskGeometry = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));

    /* Get the disk geometry */
    if (PcDiskGetDriveGeometry(DriveNumber, &Geometry))
    {
        DiskGeometry->BytesPerSector = Geometry.BytesPerSector;
        DiskGeometry->NumberOfCylinders = Geometry.Cylinders;
        DiskGeometry->SectorsPerTrack = Geometry.SectorsPerTrack;
        DiskGeometry->NumberOfHeads = Geometry.Heads;
    }
    else
    {
        TRACE("Reading disk geometry failed\n");
        FrLdrHeapFree(PartialResourceList, TAG_HW_RESOURCE_LIST);
        return NULL;
    }
    TRACE("Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
          DriveNumber,
          DiskGeometry->NumberOfCylinders,
          DiskGeometry->NumberOfHeads,
          DiskGeometry->SectorsPerTrack,
          DiskGeometry->BytesPerSector);

    /* Return configuration data */
    *pSize = Size;
    return PartialResourceList;
}

static
VOID
DetectDockingStation(
    _Inout_ PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA PeripheralKey;
    PDOCKING_STATE_INFORMATION DockingState;
    ULONG Size, Result;

    Result = PnpBiosGetDockStationInformation(DiskReadBuffer);

    /* Build full device descriptor */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(DOCKING_STATE_INFORMATION);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 0;
    PartialResourceList->Revision = 0;
    PartialResourceList->Count = 1;

    /* Set device specific data */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = 0;
    PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(DOCKING_STATE_INFORMATION);

    DockingState = (PDOCKING_STATE_INFORMATION)&PartialResourceList->PartialDescriptors[1];
    DockingState->ReturnCode = Result;
    if (Result == 0)
    {
        /* FIXME: Add more device specific data */
        ERR("FIXME: System docked\n");
    }

    /* Create controller key */
    FldrCreateComponentKey(BusKey,
                           PeripheralClass,
                           DockingInformation,
                           0,
                           0,
                           0xFFFFFFFF,
                           "Docking State Information",
                           PartialResourceList,
                           Size,
                           &PeripheralKey);
}

static
VOID
DetectPnpBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PNP_BIOS_DEVICE_NODE DeviceNode;
    PCM_PNP_BIOS_INSTALLATION_CHECK InstData;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG x;
    ULONG NodeSize = 0;
    ULONG NodeCount = 0;
    UCHAR NodeNumber;
    ULONG FoundNodeCount;
    int i;
    ULONG PnpBufferSize;
    ULONG PnpBufferSizeLimit;
    ULONG Size;
    char *Ptr;

    InstData = (PCM_PNP_BIOS_INSTALLATION_CHECK)PnpBiosSupported();
    if (InstData == NULL || strncmp((CHAR*)InstData->Signature, "$PnP", 4))
    {
        TRACE("PnP-BIOS not supported\n");
        return;
    }

    TRACE("PnP-BIOS supported\n");
    TRACE("Signature '%c%c%c%c'\n",
          InstData->Signature[0], InstData->Signature[1],
          InstData->Signature[2], InstData->Signature[3]);

    x = PnpBiosGetDeviceNodeCount(&NodeSize, &NodeCount);
    if (x == 0x82)
    {
        TRACE("PnP-BIOS function 'Get Number of System Device Nodes' not supported\n");
        return;
    }

    NodeCount &= 0xFF; // needed since some fscked up BIOSes return
    // wrong info (e.g. Mac Virtual PC)
    // e.g. look: http://my.execpc.com/~geezer/osd/pnp/pnp16.c
    if (x != 0 || NodeSize == 0 || NodeCount == 0)
    {
        ERR("PnP-BIOS failed to enumerate device nodes\n");
        return;
    }
    TRACE("MaxNodeSize %u  NodeCount %u\n", NodeSize, NodeCount);
    TRACE("Estimated buffer size %u\n", NodeSize * NodeCount);

    /* Set 'Configuration Data' value */
    PnpBufferSizeLimit = sizeof(CM_PNP_BIOS_INSTALLATION_CHECK)
                         + (NodeSize * NodeCount);
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) + PnpBufferSizeLimit;
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;
    PartialResourceList->PartialDescriptors[0].Type =
        CmResourceTypeDeviceSpecific;
    PartialResourceList->PartialDescriptors[0].ShareDisposition =
        CmResourceShareUndetermined;

    /* The buffer starts after PartialResourceList->PartialDescriptors[0] */
    Ptr = (char *)(PartialResourceList + 1);

    /* Set installation check data */
    memcpy (Ptr, InstData, sizeof(CM_PNP_BIOS_INSTALLATION_CHECK));
    Ptr += sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);
    PnpBufferSize = sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);

    /* Copy device nodes */
    FoundNodeCount = 0;
    for (i = 0; i < 0xFF; i++)
    {
        NodeNumber = (UCHAR)i;

        x = PnpBiosGetDeviceNode(&NodeNumber, DiskReadBuffer);
        if (x == 0)
        {
            DeviceNode = (PCM_PNP_BIOS_DEVICE_NODE)DiskReadBuffer;

            TRACE("Node: %u  Size %u (0x%x)\n",
                  DeviceNode->Node,
                  DeviceNode->Size,
                  DeviceNode->Size);

            if (PnpBufferSize + DeviceNode->Size > PnpBufferSizeLimit)
            {
                ERR("Buffer too small! Ignoring remaining device nodes. (i = %d)\n", i);
                break;
            }

            memcpy(Ptr, DeviceNode, DeviceNode->Size);

            Ptr += DeviceNode->Size;
            PnpBufferSize += DeviceNode->Size;

            FoundNodeCount++;
            if (FoundNodeCount >= NodeCount)
                break;
        }
    }

    /* Set real data size */
    PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
        PnpBufferSize;
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) + PnpBufferSize;

    TRACE("Real buffer size: %u\n", PnpBufferSize);
    TRACE("Resource size: %u\n", Size);

    /* Create component key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0,
                           0,
                           0xFFFFFFFF,
                           "PNP BIOS",
                           PartialResourceList,
                           Size,
                           &BusKey);

    DetectDockingStation(BusKey);

    (*BusNumber)++;
}

static
VOID
InitializeSerialPort(PUCHAR Port,
                     UCHAR LineControl)
{
    WRITE_PORT_UCHAR(Port + 3, 0x80);  /* set DLAB on   */
    WRITE_PORT_UCHAR(Port,     0x60);  /* speed LO byte */
    WRITE_PORT_UCHAR(Port + 1, 0);     /* speed HI byte */
    WRITE_PORT_UCHAR(Port + 3, LineControl);
    WRITE_PORT_UCHAR(Port + 1, 0);     /* set comm and DLAB to 0 */
    WRITE_PORT_UCHAR(Port + 4, 0x09);  /* DR int enable */
    READ_PORT_UCHAR(Port + 5);  /* clear error bits */
}

static
ULONG
DetectSerialMouse(PUCHAR Port)
{
    CHAR Buffer[4];
    ULONG i;
    ULONG TimeOut;
    UCHAR LineControl;

    /* Shutdown mouse or something like that */
    LineControl = READ_PORT_UCHAR(Port + 4);
    WRITE_PORT_UCHAR(Port + 4, (LineControl & ~0x02) | 0x01);
    StallExecutionProcessor(100000);

    /*
     * Clear buffer
     * Maybe there is no serial port although BIOS reported one (this
     * is the case on Apple hardware), or the serial port is misbehaving,
     * therefore we must give up after some time.
     */
    TimeOut = 200;
    while (READ_PORT_UCHAR(Port + 5) & 0x01)
    {
        if (--TimeOut == 0)
            return MOUSE_TYPE_NONE;
        READ_PORT_UCHAR(Port);
    }

    /*
     * Send modem control with 'Data Terminal Ready', 'Request To Send' and
     * 'Output Line 2' message. This enables mouse to identify.
     */
    WRITE_PORT_UCHAR(Port + 4, 0x0b);

    /* Wait 10 milliseconds for the mouse getting ready */
    StallExecutionProcessor(10000);

    /* Read first four bytes, which contains Microsoft Mouse signs */
    TimeOut = 20;
    for (i = 0; i < 4; i++)
    {
        while ((READ_PORT_UCHAR(Port + 5) & 1) == 0)
        {
            StallExecutionProcessor(100);
            --TimeOut;
            if (TimeOut == 0)
                return MOUSE_TYPE_NONE;
        }
        Buffer[i] = READ_PORT_UCHAR(Port);
    }

    TRACE("Mouse data: %x %x %x %x\n",
          Buffer[0], Buffer[1], Buffer[2], Buffer[3]);

    /* Check that four bytes for signs */
    for (i = 0; i < 4; ++i)
    {
        if (Buffer[i] == 'B')
        {
            /* Sign for Microsoft Ballpoint */
//      DbgPrint("Microsoft Ballpoint device detected\n");
//      DbgPrint("THIS DEVICE IS NOT SUPPORTED, YET\n");
            return MOUSE_TYPE_NONE;
        }
        else if (Buffer[i] == 'M')
        {
            /* Sign for Microsoft Mouse protocol followed by button specifier */
            if (i == 3)
            {
                /* Overflow Error */
                return MOUSE_TYPE_NONE;
            }

            switch (Buffer[i + 1])
            {
            case '3':
                TRACE("Microsoft Mouse with 3-buttons detected\n");
                return MOUSE_TYPE_LOGITECH;

            case 'Z':
                TRACE("Microsoft Wheel Mouse detected\n");
                return MOUSE_TYPE_WHEELZ;

                /* case '2': */
            default:
                TRACE("Microsoft Mouse with 2-buttons detected\n");
                return MOUSE_TYPE_MICROSOFT;
            }
        }
    }

    return MOUSE_TYPE_NONE;
}

static ULONG
GetSerialMousePnpId(PUCHAR Port, char *Buffer)
{
    ULONG TimeOut;
    ULONG i = 0;
    char c;
    char x;

    WRITE_PORT_UCHAR(Port + 4, 0x09);

    /* Wait 10 milliseconds for the mouse getting ready */
    StallExecutionProcessor(10000);

    WRITE_PORT_UCHAR(Port + 4, 0x0b);

    StallExecutionProcessor(10000);

    for (;;)
    {
        TimeOut = 200;
        while (((READ_PORT_UCHAR(Port + 5) & 1) == 0) && (TimeOut > 0))
        {
            StallExecutionProcessor(1000);
            --TimeOut;
            if (TimeOut == 0)
            {
                return 0;
            }
        }

        c = READ_PORT_UCHAR(Port);
        if (c == 0x08 || c == 0x28)
            break;
    }

    Buffer[i++] = c;
    x = c + 1;

    for (;;)
    {
        TimeOut = 200;
        while (((READ_PORT_UCHAR(Port + 5) & 1) == 0) && (TimeOut > 0))
        {
            StallExecutionProcessor(1000);
            --TimeOut;
            if (TimeOut == 0)
                return 0;
        }
        c = READ_PORT_UCHAR(Port);
        Buffer[i++] = c;
        if (c == x)
            break;
        if (i >= 256)
            break;
    }

    return i;
}

static
VOID
DetectSerialPointerPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey,
                              PUCHAR Base)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    char Buffer[256];
    CHAR Identifier[256];
    PCONFIGURATION_COMPONENT_DATA PeripheralKey;
    ULONG MouseType;
    ULONG Size, Length;
    ULONG i;
    ULONG j;
    ULONG k;

    TRACE("DetectSerialPointerPeripheral()\n");

    Identifier[0] = 0;

    InitializeSerialPort(Base, 2);
    MouseType = DetectSerialMouse(Base);

    if (MouseType != MOUSE_TYPE_NONE)
    {
        Length = GetSerialMousePnpId(Base, Buffer);
        TRACE( "PnP ID length: %u\n", Length);

        if (Length != 0)
        {
            /* Convert PnP sting to ASCII */
            if (Buffer[0] == 0x08)
            {
                for (i = 0; i < Length; i++)
                    Buffer[i] += 0x20;
            }
            Buffer[Length] = 0;

            TRACE("PnP ID string: %s\n", Buffer);

            /* Copy PnpId string */
            for (i = 0; i < 7; i++)
            {
                Identifier[i] = Buffer[3 + i];
            }
            memcpy(&Identifier[7],
                   L" - ",
                   3 * sizeof(WCHAR));

            /* Skip device serial number */
            i = 10;
            if (Buffer[i] == '\\')
            {
                for (j = ++i; i < Length; ++i)
                {
                    if (Buffer[i] == '\\')
                        break;
                }
                if (i >= Length)
                    i -= 3;
            }

            /* Skip PnP class */
            if (Buffer[i] == '\\')
            {
                for (j = ++i; i < Length; ++i)
                {
                    if (Buffer[i] == '\\')
                        break;
                }

                if (i >= Length)
                    i -= 3;
            }

            /* Skip compatible PnP Id */
            if (Buffer[i] == '\\')
            {
                for (j = ++i; i < Length; ++i)
                {
                    if (Buffer[i] == '\\')
                        break;
                }
                if (Buffer[j] == '*')
                    ++j;
                if (i >= Length)
                    i -= 3;
            }

            /* Get product description */
            if (Buffer[i] == '\\')
            {
                for (j = ++i; i < Length; ++i)
                {
                    if (Buffer[i] == ';')
                        break;
                }
                if (i >= Length)
                    i -= 3;
                if (i > j + 1)
                {
                    for (k = 0; k < i - j; k++)
                    {
                        Identifier[k + 10] = Buffer[k + j];
                    }
                    Identifier[10 + (i - j)] = 0;
                }
            }

            TRACE("Identifier string: %s\n", Identifier);
        }

        if (Length == 0 || strlen(Identifier) < 11)
        {
            switch (MouseType)
            {
            case MOUSE_TYPE_LOGITECH:
                strcpy(Identifier, "LOGITECH SERIAL MOUSE");
                break;

            case MOUSE_TYPE_WHEELZ:
                strcpy(Identifier, "MICROSOFT SERIAL MOUSE WITH WHEEL");
                break;

            case MOUSE_TYPE_MICROSOFT:
            default:
                strcpy(Identifier, "MICROSOFT SERIAL MOUSE");
                break;
            }
        }

        /* Set 'Configuration Data' value */
        Size = sizeof(CM_PARTIAL_RESOURCE_LIST) -
               sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }

        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = 0;

        /* Create 'PointerPeripheral' key */
        FldrCreateComponentKey(ControllerKey,
                               PeripheralClass,
                               PointerPeripheral,
                               Input,
                               0,
                               0xFFFFFFFF,
                               Identifier,
                               PartialResourceList,
                               Size,
                               &PeripheralKey);
    }
}

static
ULONG
PcGetSerialPort(ULONG Index, PULONG Irq)
{
    static const ULONG PcIrq[MAX_COM_PORTS] = {4, 3, 4, 3};
    PUSHORT BasePtr;

    /*
     * The BIOS data area 0x400 holds the address of the first valid COM port.
     * Each COM port address is stored in a 2-byte field.
     * Infos at: http://www.bioscentral.com/misc/bda.htm
     */
    BasePtr = (PUSHORT)0x400;
    *Irq = PcIrq[Index];

    return (ULONG) *(BasePtr + Index);
}

/*
 * Parse the serial mouse detection options.
 * Format: /FASTDETECT
 * or: /NOSERIALMICE=COM[0-9],[0-9],[0-9]...
 * or: /NOSERIALMICE:COM[0-9]...
 * If we have /FASTDETECT, then nothing can be detected.
 */
static
ULONG
GetSerialMouseDetectionBitmap(
    _In_opt_ PCSTR Options)
{
    PCSTR Option, c;
    ULONG OptionLength, PortBitmap, i;

    if (NtLdrGetOption(Options, "FASTDETECT"))
        return (1 << MAX_COM_PORTS) - 1;

    Option = NtLdrGetOptionEx(Options, "NOSERIALMICE=", &OptionLength);
    if (!Option)
        Option = NtLdrGetOptionEx(Options, "NOSERIALMICE:", &OptionLength);

    if (!Option)
        return 0;

    /* Invalid port list */
    if (OptionLength < (sizeof("NOSERIALMICE=COM9") - 1))
        return (1 << MAX_COM_PORTS) - 1;

    /* Move to the port list */
    Option += sizeof("NOSERIALMICE=COM") - 1;
    OptionLength -= sizeof("NOSERIALMICE=COM") - 1;

    PortBitmap = 0;
    c = Option;
    for (i = 0; i < OptionLength; i += 2)
    {
        UCHAR PortNumber = *c - '0';

        if (PortNumber > 0 && PortNumber <= 9)
            PortBitmap |= 1 << (PortNumber - 1);

        c += 2;
    }

    return PortBitmap;
}

VOID
DetectSerialPorts(
    _In_opt_ PCSTR Options,
    _Inout_ PCONFIGURATION_COMPONENT_DATA BusKey,
    _In_ GET_SERIAL_PORT MachGetSerialPort,
    _In_ ULONG Count)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_SERIAL_DEVICE_DATA SerialDeviceData;
    ULONG Irq;
    ULONG Base;
    CHAR Identifier[80];
    ULONG ControllerNumber = 0;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    ULONG i;
    ULONG Size;
    ULONG PortBitmap;

    TRACE("DetectSerialPorts()\n");

    PortBitmap = GetSerialMouseDetectionBitmap(Options);

    for (i = 0; i < Count; i++)
    {
        Base = MachGetSerialPort(i, &Irq);
        if ((Base == 0) || !CpDoesPortExist(UlongToPtr(Base)))
            continue;

        TRACE("Found COM%u port at 0x%x\n", i + 1, Base);

        /* Set 'Identifier' value */
        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "COM%ld", i + 1);

        /* Build full device descriptor */
        Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
               2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
               sizeof(CM_SERIAL_DEVICE_DATA);
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor! Ignoring remaining serial ports. (i = %lu, Count = %lu)\n",
                i, Count);
            break;
        }

        /* Initialize resource descriptor */
        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = 3;

        /* Set IO Port */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypePort;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
        PartialDescriptor->u.Port.Start.LowPart = Base;
        PartialDescriptor->u.Port.Start.HighPart = 0x0;
        PartialDescriptor->u.Port.Length = 8;

        /* Set Interrupt */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
        PartialDescriptor->Type = CmResourceTypeInterrupt;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        PartialDescriptor->u.Interrupt.Level = Irq;
        PartialDescriptor->u.Interrupt.Vector = Irq;
        PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

        /* Set serial data (device specific) */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[2];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->Flags = 0;
        PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_SERIAL_DEVICE_DATA);

        SerialDeviceData =
            (PCM_SERIAL_DEVICE_DATA)&PartialResourceList->PartialDescriptors[3];
        SerialDeviceData->BaudClock = 1843200; /* UART Clock frequency (Hertz) */

        /* Create controller key */
        FldrCreateComponentKey(BusKey,
                               ControllerClass,
                               SerialController,
                               Output | Input | ConsoleIn | ConsoleOut,
                               ControllerNumber,
                               0xFFFFFFFF,
                               Identifier,
                               PartialResourceList,
                               Size,
                               &ControllerKey);

        if (!(PortBitmap & (1 << i)) && !Rs232PortInUse(UlongToPtr(Base)))
        {
            /* Detect serial mouse */
            DetectSerialPointerPeripheral(ControllerKey, UlongToPtr(Base));
        }

        ControllerNumber++;
    }
}

static VOID
DetectParallelPorts(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    ULONG Irq[MAX_LPT_PORTS] = {7, 5, (ULONG) - 1};
    CHAR Identifier[80];
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    PUSHORT BasePtr;
    ULONG Base;
    ULONG ControllerNumber = 0;
    ULONG i;
    ULONG Size;

    TRACE("DetectParallelPorts() called\n");

    /*
     * The BIOS data area 0x408 holds the address of the first valid LPT port.
     * Each LPT port address is stored in a 2-byte field.
     * Infos at: http://www.bioscentral.com/misc/bda.htm
     */
    BasePtr = (PUSHORT)0x408;

    for (i = 0; i < MAX_LPT_PORTS; i++, BasePtr++)
    {
        Base = (ULONG) * BasePtr;
        if (Base == 0)
            continue;

        TRACE("Parallel port %u: %x\n", ControllerNumber, Base);

        /* Set 'Identifier' value */
        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "PARALLEL%ld", i + 1);

        /* Build full device descriptor */
        Size = sizeof(CM_PARTIAL_RESOURCE_LIST);
        if (Irq[i] != (ULONG) - 1)
            Size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor! Ignoring remaining parallel ports. (i = %lu)\n", i);
            break;
        }

        /* Initialize resource descriptor */
        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = (Irq[i] != (ULONG) - 1) ? 2 : 1;

        /* Set IO Port */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypePort;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
        PartialDescriptor->u.Port.Start.LowPart = Base;
        PartialDescriptor->u.Port.Start.HighPart = 0x0;
        PartialDescriptor->u.Port.Length = 3;

        /* Set Interrupt */
        if (Irq[i] != (ULONG) - 1)
        {
            PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
            PartialDescriptor->Type = CmResourceTypeInterrupt;
            PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
            PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
            PartialDescriptor->u.Interrupt.Level = Irq[i];
            PartialDescriptor->u.Interrupt.Vector = Irq[i];
            PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;
        }

        /* Create controller key */
        FldrCreateComponentKey(BusKey,
                               ControllerClass,
                               ParallelController,
                               Output,
                               ControllerNumber,
                               0xFFFFFFFF,
                               Identifier,
                               PartialResourceList,
                               Size,
                               &ControllerKey);

        ControllerNumber++;
    }

    TRACE("DetectParallelPorts() done\n");
}

// static
BOOLEAN
DetectKeyboardDevice(VOID)
{
    UCHAR Status;
    UCHAR Scancode;
    ULONG Loops;
    BOOLEAN Result = TRUE;

    /* Identify device */
    WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA, 0xF2);

    /* Wait for reply */
    for (Loops = 0; Loops < 100; Loops++)
    {
        StallExecutionProcessor(10000);
        Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
        if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
            break;
    }

    if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) == 0)
    {
        /* PC/XT keyboard or no keyboard */
        Result = FALSE;
    }

    Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
    if (Scancode != 0xFA)
    {
        /* No ACK received */
        Result = FALSE;
    }

    StallExecutionProcessor(10000);

    Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
    if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) == 0)
    {
        /* Found AT keyboard */
        return Result;
    }

    Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
    if (Scancode != 0xAB)
    {
        /* No 0xAB received */
        Result = FALSE;
    }

    StallExecutionProcessor(10000);

    Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
    if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) == 0)
    {
        /* No byte in buffer */
        Result = FALSE;
    }

    Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
    if (Scancode != 0x41)
    {
        /* No 0x41 received */
        Result = FALSE;
    }

    /* Found MF-II keyboard */
    return Result;
}

static VOID
DetectKeyboardPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_KEYBOARD_DEVICE_DATA KeyboardData;
    PCONFIGURATION_COMPONENT_DATA PeripheralKey;
    ULONG Size;
    REGS Regs;

    /* HACK: don't call DetectKeyboardDevice() as it fails in Qemu 0.8.2
    if (DetectKeyboardDevice()) */
    {
        /* Set 'Configuration Data' value */
        Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
               sizeof(CM_KEYBOARD_DEVICE_DATA);
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }

        /* Initialize resource descriptor */
        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = 1;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_KEYBOARD_DEVICE_DATA);

        /* Int 16h AH=02h
         * KEYBOARD - GET SHIFT FLAGS
         *
         * Return:
         * AL - shift flags
         */
        Regs.b.ah = 0x02;
        Int386(0x16, &Regs, &Regs);

        KeyboardData = (PCM_KEYBOARD_DEVICE_DATA)(PartialDescriptor + 1);
        KeyboardData->Version = 1;
        KeyboardData->Revision = 1;
        KeyboardData->Type = 4;
        KeyboardData->Subtype = 0;
        KeyboardData->KeyboardFlags = Regs.b.al;

        /* Create controller key */
        FldrCreateComponentKey(ControllerKey,
                               PeripheralClass,
                               KeyboardPeripheral,
                               Input | ConsoleIn,
                               0,
                               0xFFFFFFFF,
                               "PCAT_ENHANCED",
                               PartialResourceList,
                               Size,
                               &PeripheralKey);
    }
}

static
VOID
DetectKeyboardController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    ULONG Size;

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 3;

    /* Set Interrupt */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeInterrupt;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    PartialDescriptor->u.Interrupt.Level = 1;
    PartialDescriptor->u.Interrupt.Vector = 1;
    PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

    /* Set IO Port 0x60 */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = 0x60;
    PartialDescriptor->u.Port.Start.HighPart = 0x0;
    PartialDescriptor->u.Port.Length = 1;

    /* Set IO Port 0x64 */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[2];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = 0x64;
    PartialDescriptor->u.Port.Start.HighPart = 0x0;
    PartialDescriptor->u.Port.Length = 1;

    /* Create controller key */
    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           KeyboardController,
                           Input | ConsoleIn,
                           0,
                           0xFFFFFFFF,
                           NULL,
                           PartialResourceList,
                           Size,
                           &ControllerKey);

    DetectKeyboardPeripheral(ControllerKey);
}

static
VOID
PS2ControllerWait(VOID)
{
    ULONG Timeout;
    UCHAR Status;

    for (Timeout = 0; Timeout < CONTROLLER_TIMEOUT; Timeout++)
    {
        Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
        if ((Status & CONTROLLER_STATUS_INPUT_BUFFER_FULL) == 0)
            return;

        /* Sleep for one millisecond */
        StallExecutionProcessor(1000);
    }
}

static
BOOLEAN
DetectPS2AuxPort(VOID)
{
#if 1
    /* Current detection is too unreliable. Just do as if
     * the PS/2 aux port is always present
     */
    return TRUE;
#else
    ULONG Loops;
    UCHAR Status;

    /* Put the value 0x5A in the output buffer using the
     * "WriteAuxiliary Device Output Buffer" command (0xD3).
     * Poll the Status Register for a while to see if the value really turns up
     * in the Data Register. If the KEYBOARD_STATUS_MOUSE_OBF bit is also set
     * to 1 in the Status Register, we assume this controller has an
     *  Auxiliary Port (a.k.a. Mouse Port).
     */
    PS2ControllerWait();
    WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_CONTROL,
                     CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER);
    PS2ControllerWait();

    /* 0x5A is a random dummy value */
    WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA,
                     0x5A);

    for (Loops = 0; Loops < 10; Loops++)
    {
        StallExecutionProcessor(10000);
        Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
        if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
            break;
    }

    READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);

    return (Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL);
#endif
}

static
BOOLEAN
DetectPS2AuxDevice(VOID)
{
    UCHAR Scancode;
    UCHAR Status;
    ULONG Loops;
    BOOLEAN Result = TRUE;

    PS2ControllerWait();
    WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_CONTROL,
                     CONTROLLER_COMMAND_WRITE_MOUSE);
    PS2ControllerWait();

    /* Identify device */
    WRITE_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA, 0xF2);

    /* Wait for reply */
    for (Loops = 0; Loops < 100; Loops++)
    {
        StallExecutionProcessor(10000);
        Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
        if ((Status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
            break;
    }

    Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
    if ((Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) == 0)
        Result = FALSE;

    Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
    if (Scancode != 0xFA)
        Result = FALSE;

    StallExecutionProcessor(10000);

    Status = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_STATUS);
    if ((Status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) == 0)
        Result = FALSE;

    Scancode = READ_PORT_UCHAR((PUCHAR)CONTROLLER_REGISTER_DATA);
    if (Scancode != 0x00)
        Result = FALSE;

    return Result;
}

// FIXME: Missing: DetectPS2Peripheral!! (for corresponding 'PointerPeripheral')

static
VOID
DetectPS2Mouse(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    PCONFIGURATION_COMPONENT_DATA PeripheralKey;
    ULONG Size;

    if (DetectPS2AuxPort())
    {
        TRACE("Detected PS2 port\n");

        PartialResourceList = FrLdrHeapAlloc(sizeof(CM_PARTIAL_RESOURCE_LIST), TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }

        /* Initialize resource descriptor */
        RtlZeroMemory(PartialResourceList, sizeof(CM_PARTIAL_RESOURCE_LIST));
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = 1;

        /* Set Interrupt */
        PartialResourceList->PartialDescriptors[0].Type = CmResourceTypeInterrupt;
        PartialResourceList->PartialDescriptors[0].ShareDisposition = CmResourceShareUndetermined;
        PartialResourceList->PartialDescriptors[0].Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        PartialResourceList->PartialDescriptors[0].u.Interrupt.Level = 12;
        PartialResourceList->PartialDescriptors[0].u.Interrupt.Vector = 12;
        PartialResourceList->PartialDescriptors[0].u.Interrupt.Affinity = 0xFFFFFFFF;

        /* Create controller key */
        FldrCreateComponentKey(BusKey,
                               ControllerClass,
                               PointerController,
                               Input,
                               0,
                               0xFFFFFFFF,
                               NULL,
                               PartialResourceList,
                               sizeof(CM_PARTIAL_RESOURCE_LIST),
                               &ControllerKey);

        if (DetectPS2AuxDevice())
        {
            TRACE("Detected PS2 mouse\n");

            /* Initialize resource descriptor */
            Size = sizeof(CM_PARTIAL_RESOURCE_LIST) -
                   sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (PartialResourceList == NULL)
            {
                ERR("Failed to allocate resource descriptor\n");
                return;
            }

            RtlZeroMemory(PartialResourceList, Size);
            PartialResourceList->Version = 1;
            PartialResourceList->Revision = 1;
            PartialResourceList->Count = 0;

            /* Create peripheral key */
            FldrCreateComponentKey(ControllerKey,
                                   ControllerClass,
                                   PointerPeripheral,
                                   Input,
                                   0,
                                   0xFFFFFFFF,
                                   "MICROSOFT PS2 MOUSE",
                                   PartialResourceList,
                                   Size,
                                   &PeripheralKey);
        }
    }
}

#if defined(_M_IX86)
static VOID
CreateBusMousePeripheralKey(
    _Inout_ PCONFIGURATION_COMPONENT_DATA BusKey,
    _In_ ULONG IoBase,
    _In_ ULONG Irq)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    PCONFIGURATION_COMPONENT_DATA PeripheralKey;
    ULONG Size;

    /* Set 'Configuration Data' value */
    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[2]);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 2;

    /* Set IO Port */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = IoBase;
    PartialDescriptor->u.Port.Start.HighPart = 0;
    PartialDescriptor->u.Port.Length = 4;

    /* Set Interrupt */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
    PartialDescriptor->Type = CmResourceTypeInterrupt;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    PartialDescriptor->u.Interrupt.Level = Irq;
    PartialDescriptor->u.Interrupt.Vector = Irq;
    PartialDescriptor->u.Interrupt.Affinity = (KAFFINITY)-1;

    /* Create controller key */
    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           PointerController,
                           Input,
                           0,
                           0xFFFFFFFF,
                           NULL,
                           PartialResourceList,
                           Size,
                           &ControllerKey);

    /* Set 'Configuration Data' value */
    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 0;

    /* Create peripheral key */
    FldrCreateComponentKey(ControllerKey,
                           ControllerClass,
                           PointerPeripheral,
                           Input,
                           0,
                           0xFFFFFFFF,
                           "MICROSOFT INPORT MOUSE",
                           PartialResourceList,
                           Size,
                           &PeripheralKey);
}

extern KIDTENTRY DECLSPEC_ALIGN(4) i386Idt[32];
VOID __cdecl HwIrqHandler(VOID);
extern volatile ULONG HwIrqCount;

static ULONG
DetectBusMouseTestIrq(
    _In_ ULONG IoBase,
    _In_ ULONG Irq)
{
    USHORT OldOffset, OldExtendedOffset;
    ULONG Vector, i;

    HwIrqCount = 0;

    /* Reset the device */
    WRITE_PORT_UCHAR((PUCHAR)IoBase + INPORT_REGISTER_CONTROL, INPORT_RESET);
    WRITE_PORT_UCHAR((PUCHAR)IoBase + INPORT_REGISTER_CONTROL, INPORT_REG_MODE);

    Vector = Irq + 8;

    /* Save the old interrupt vector and replace it by ours */
    OldOffset = i386Idt[Vector].Offset;
    OldExtendedOffset = i386Idt[Vector].ExtendedOffset;

    i386Idt[Vector].Offset = (ULONG)HwIrqHandler & 0xFFFF;
    i386Idt[Vector].ExtendedOffset = (ULONG)HwIrqHandler >> 16;

    /* Enable the requested IRQ on the master PIC */
    WRITE_PORT_UCHAR((PUCHAR)PIC1_DATA_PORT, ~(1 << Irq));

    _enable();

    /* Configure the device to generate interrupts */
    for (i = 0; i < 15; i++)
    {
        WRITE_PORT_UCHAR((PUCHAR)IoBase + INPORT_REGISTER_DATA, INPORT_MODE_BASE);
        WRITE_PORT_UCHAR((PUCHAR)IoBase + INPORT_REGISTER_DATA, INPORT_TEST_IRQ);
    }

    /* Disable the device */
    WRITE_PORT_UCHAR((PUCHAR)IoBase + INPORT_REGISTER_DATA, 0);

    _disable();

    i386Idt[Vector].Offset = OldOffset;
    i386Idt[Vector].ExtendedOffset = OldExtendedOffset;

    return (HwIrqCount != 0) ? Irq : 0;
}

static ULONG
DetectBusMouseIrq(
    _In_ ULONG IoBase)
{
    UCHAR Mask1, Mask2;
    ULONG Irq, Result;

    /* Save the current interrupt mask */
    Mask1 = READ_PORT_UCHAR(PIC1_DATA_PORT);
    Mask2 = READ_PORT_UCHAR(PIC2_DATA_PORT);

    /* Mask the interrupts on the slave PIC */
    WRITE_PORT_UCHAR(PIC2_DATA_PORT, 0xFF);

    /* Process IRQ detection: IRQ 5, 4, 3 */
    for (Irq = 5; Irq >= 3; Irq--)
    {
        Result = DetectBusMouseTestIrq(IoBase, Irq);
        if (Result != 0)
            break;
    }

    /* Restore the mask */
    WRITE_PORT_UCHAR(PIC1_DATA_PORT, Mask1);
    WRITE_PORT_UCHAR(PIC2_DATA_PORT, Mask2);

    return Result;
}

static VOID
DetectBusMouse(
    _Inout_ PCONFIGURATION_COMPONENT_DATA BusKey)
{
    ULONG IoBase, Irq, Signature1, Signature2, Signature3;

    /*
     * The bus mouse lives at one of these addresses: 0x230, 0x234, 0x238, 0x23C.
     * The 0x23C port is the most common I/O setting.
     */
    for (IoBase = 0x23C; IoBase >= 0x230; IoBase -= 4)
    {
        Signature1 = READ_PORT_UCHAR((PUCHAR)IoBase + INPORT_REGISTER_SIGNATURE);
        Signature2 = READ_PORT_UCHAR((PUCHAR)IoBase + INPORT_REGISTER_SIGNATURE);
        if (Signature1 == Signature2)
            continue;
        if (Signature1 != INPORT_SIGNATURE && Signature2 != INPORT_SIGNATURE)
            continue;

        Signature3 = READ_PORT_UCHAR((PUCHAR)IoBase + INPORT_REGISTER_SIGNATURE);
        if (Signature1 != Signature3)
            continue;

        Irq = DetectBusMouseIrq(IoBase);
        if (Irq == 0)
            continue;

        CreateBusMousePeripheralKey(BusKey, IoBase, Irq);
        break;
    }
}
#endif /* _M_IX86 */

// Implemented in pcvesa.c, returns the VESA version
USHORT  BiosIsVesaSupported(VOID);
BOOLEAN BiosIsVesaDdcSupported(VOID);
BOOLEAN BiosVesaReadEdid(VOID);

static VOID
DetectDisplayController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCSTR Identifier;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    USHORT VesaVersion;

    /* FIXME: Set 'ComponentInformation' value */

    VesaVersion = BiosIsVesaSupported();
    if (VesaVersion != 0)
    {
        TRACE("VESA version %c.%c\n",
              (VesaVersion >> 8) + '0',
              (VesaVersion & 0xFF) + '0');
    }
    else
    {
        TRACE("VESA not supported\n");
    }

    if (VesaVersion >= 0x0200)
        Identifier = "VBE Display";
    else
        Identifier = "VGA Display";

    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           DisplayController,
                           Output | ConsoleOut,
                           0,
                           0xFFFFFFFF,
                           Identifier,
                           NULL,
                           0,
                           &ControllerKey);

    /* FIXME: Add display peripheral (monitor) data */
    if (VesaVersion != 0)
    {
        if (BiosIsVesaDdcSupported())
        {
            TRACE("VESA/DDC supported!\n");
            if (BiosVesaReadEdid())
            {
                TRACE("EDID data read successfully!\n");
            }
        }
    }
}

static
VOID
DetectIsaBios(
    _In_opt_ PCSTR Options,
    _Inout_ PCONFIGURATION_COMPONENT_DATA SystemKey,
    _Out_ ULONG *BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG Size;

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) -
           sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 0;

    /* Create new bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0,
                           0,
                           0xFFFFFFFF,
                           "ISA",
                           PartialResourceList,
                           Size,
                           &BusKey);

    /* Increment bus number */
    (*BusNumber)++;

    /* Detect ISA/BIOS devices */
    DetectBiosDisks(SystemKey, BusKey);
    DetectSerialPorts(Options, BusKey, PcGetSerialPort, MAX_COM_PORTS);
    DetectParallelPorts(BusKey);
    DetectKeyboardController(BusKey);
    DetectPS2Mouse(BusKey);
#if defined(_M_IX86)
    DetectBusMouse(BusKey);
#endif
    DetectDisplayController(BusKey);

    /* FIXME: Detect more ISA devices */
}

/* FIXME: Abstract things better so we don't need to place define here */
#if !defined(SARCH_XBOX)
static
UCHAR
PcGetFloppyCount(VOID)
{
    UCHAR Data;

    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x10);
    Data = READ_PORT_UCHAR((PUCHAR)0x71);

    return ((Data & 0xF0) ? 1 : 0) + ((Data & 0x0F) ? 1 : 0);
}
#endif

PCONFIGURATION_COMPONENT_DATA
PcHwDetect(
    _In_opt_ PCSTR Options)
{
    PCONFIGURATION_COMPONENT_DATA SystemKey;
    ULONG BusNumber = 0;

    TRACE("DetectHardware()\n");

    /* Create the 'System' key */
    // TODO: Discover and set the other machine types
    FldrCreateSystemKey(&SystemKey, "AT/AT COMPATIBLE");

    GetHarddiskConfigurationData = PcGetHarddiskConfigurationData;
    FindPciBios = PcFindPciBios;

    /* Detect buses */
    DetectPciBios(SystemKey, &BusNumber);
    DetectApmBios(SystemKey, &BusNumber);
    DetectPnpBios(SystemKey, &BusNumber);
    DetectIsaBios(Options, SystemKey, &BusNumber); // TODO: Detect first EISA or MCA, before ISA
    DetectAcpiBios(SystemKey, &BusNumber);

    // TODO: Collect the ROM blocks from 0xC0000 to 0xF0000 and append their
    // CM_ROM_BLOCK data into the 'System' key's configuration data.

    TRACE("DetectHardware() Done\n");
    return SystemKey;
}

VOID
PcHwIdle(VOID)
{
    REGS Regs;

    /* Select APM 1.0+ function */
    Regs.b.ah = 0x53;

    /* Function 05h: CPU idle */
    Regs.b.al = 0x05;

    /* Call INT 15h */
    Int386(0x15, &Regs, &Regs);

    /* Check if successfull (CF set on error) */
    if (INT386_SUCCESS(Regs))
        return;

    /*
     * No futher processing here.
     * Optionally implement HLT instruction handling.
     */
}

VOID __cdecl ChainLoadBiosBootSectorCode(
    IN UCHAR BootDrive OPTIONAL,
    IN ULONG BootPartition OPTIONAL)
{
    REGS Regs;

    RtlZeroMemory(&Regs, sizeof(Regs));

    /* Set the boot drive and the boot partition */
    Regs.b.dl = (UCHAR)(BootDrive ? BootDrive : FrldrBootDrive);
    Regs.b.dh = (UCHAR)(BootPartition ? BootPartition : FrldrBootPartition);

    /*
     * Don't stop the floppy drive motor when we are just booting a bootsector,
     * a drive, or a partition. If we were to stop the floppy motor, the BIOS
     * wouldn't be informed and if the next read is to a floppy then the BIOS
     * will still think the motor is on and this will result in a read error.
     */
    // DiskStopFloppyMotor();

    Relocator16Boot(&Regs,
                    /* Stack segment:pointer */
                    0x0000, 0x7C00,
                    /* Code segment:pointer */
                    0x0000, 0x7C00);
}

/******************************************************************************/

/* FIXME: Abstract things better so we don't need to place define here */
#if !defined(SARCH_XBOX)
VOID
MachInit(const char *CmdLine)
{
    /* Setup vtbl */
    RtlZeroMemory(&MachVtbl, sizeof(MachVtbl));
    MachVtbl.ConsPutChar = PcConsPutChar;
    MachVtbl.ConsKbHit = PcConsKbHit;
    MachVtbl.ConsGetCh = PcConsGetCh;
    MachVtbl.VideoClearScreen = PcVideoClearScreen;
    MachVtbl.VideoSetDisplayMode = PcVideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = PcVideoGetDisplaySize;
    MachVtbl.VideoGetBufferSize = PcVideoGetBufferSize;
    MachVtbl.VideoGetFontsFromFirmware = PcVideoGetFontsFromFirmware;
    MachVtbl.VideoSetTextCursorPosition = PcVideoSetTextCursorPosition;
    MachVtbl.VideoHideShowTextCursor = PcVideoHideShowTextCursor;
    MachVtbl.VideoPutChar = PcVideoPutChar;
    MachVtbl.VideoCopyOffScreenBufferToVRAM = PcVideoCopyOffScreenBufferToVRAM;
    MachVtbl.VideoIsPaletteFixed = PcVideoIsPaletteFixed;
    MachVtbl.VideoSetPaletteColor = PcVideoSetPaletteColor;
    MachVtbl.VideoGetPaletteColor = PcVideoGetPaletteColor;
    MachVtbl.VideoSync = PcVideoSync;
    MachVtbl.Beep = PcBeep;
    MachVtbl.PrepareForReactOS = PcPrepareForReactOS;
    MachVtbl.GetMemoryMap = PcMemGetMemoryMap;
    MachVtbl.GetExtendedBIOSData = PcGetExtendedBIOSData;
    MachVtbl.GetFloppyCount = PcGetFloppyCount;
    MachVtbl.DiskReadLogicalSectors = PcDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = PcDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = PcDiskGetCacheableBlockCount;
    MachVtbl.GetTime = PcGetTime;
    MachVtbl.InitializeBootDevices = PcInitializeBootDevices;
    MachVtbl.HwDetect = PcHwDetect;
    MachVtbl.HwIdle = PcHwIdle;

    HalpCalibrateStallExecution();
}

VOID
PcPrepareForReactOS(VOID)
{
    /* On PC, prepare video and turn off the floppy motor */
    PcVideoPrepareForReactOS();
    DiskStopFloppyMotor();
}
#endif

/* EOF */
