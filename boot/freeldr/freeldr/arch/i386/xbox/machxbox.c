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

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);


BOOLEAN
XboxFindPciBios(PPCI_REGISTRY_INFO BusData)
{
    /* We emulate PCI BIOS here, there are 2 known working PCI buses on an original Xbox */

    BusData->NoBuses = 2;
    BusData->MajorRevision = 1;
    BusData->MinorRevision = 0;
    BusData->HardwareMechanism = 1;
    return TRUE;
}

extern
VOID
DetectSerialPointerPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey, PUCHAR Base);

static
ULONG
XboxGetSerialPort(ULONG Index, PULONG Irq)
{
    /*
     * Xbox may have maximum two Serial COM ports
     * if the Super I/O chip is connected via LPC
     */
    static const UCHAR Device[MAX_XBOX_COM_PORTS] = {LPC_DEVICE_SERIAL_PORT_1, LPC_DEVICE_SERIAL_PORT_2};
    ULONG ComBase = 0;

    // Enter Configuration
    WRITE_PORT_UCHAR(LPC_IO_BASE, LPC_ENTER_CONFIG_KEY);

    // Select serial device
    WRITE_PORT_UCHAR(LPC_IO_BASE, LPC_CONFIG_DEVICE_NUMBER);
    WRITE_PORT_UCHAR(LPC_IO_BASE + 1, Device[Index]);

    // Check if selected device is active
    WRITE_PORT_UCHAR(LPC_IO_BASE, LPC_CONFIG_DEVICE_ACTIVATE);
    if (READ_PORT_UCHAR(LPC_IO_BASE + 1) == 1)
    {
        // Read LSB
        WRITE_PORT_UCHAR(LPC_IO_BASE, LPC_CONFIG_DEVICE_BASE_ADDRESS_LOW);
        ComBase = READ_PORT_UCHAR(LPC_IO_BASE + 1);
        // Read MSB
        WRITE_PORT_UCHAR(LPC_IO_BASE, LPC_CONFIG_DEVICE_BASE_ADDRESS_HIGH);
        ComBase |= (READ_PORT_UCHAR(LPC_IO_BASE + 1) << 8);
        // Read IRQ
        WRITE_PORT_UCHAR(LPC_IO_BASE, LPC_CONFIG_DEVICE_INTERRUPT);
        *Irq = READ_PORT_UCHAR(LPC_IO_BASE + 1);
    }

    // Exit Configuration
    WRITE_PORT_UCHAR(LPC_IO_BASE, LPC_EXIT_CONFIG_KEY);

    return ComBase;
}

extern
VOID
DetectSerialPorts(PCONFIGURATION_COMPONENT_DATA BusKey, GET_SERIAL_PORT MachGetSerialPort, ULONG Count);

VOID
XboxGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize)
{
    TRACE("XboxGetExtendedBIOSData(): UNIMPLEMENTED\n");
    *ExtendedBIOSDataArea = 0;
    *ExtendedBIOSDataSize = 0;
}

// NOTE: Similar to machpc.c!PcGetHarddiskConfigurationData(),
// but without extended geometry support.
static
PCM_PARTIAL_RESOURCE_LIST
XboxGetHarddiskConfigurationData(UCHAR DriveNumber, ULONG* pSize)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
    //EXTENDED_GEOMETRY ExtGeometry;
    GEOMETRY Geometry;
    ULONG Size;

    //
    // Initialize returned size
    //
    *pSize = 0;

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate a full resource descriptor\n");
        return NULL;
    }

    memset(PartialResourceList, 0, Size);
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
    //ExtGeometry.Size = sizeof(EXTENDED_GEOMETRY);

    if (XboxDiskGetDriveGeometry(DriveNumber, &Geometry))
    {
        DiskGeometry->BytesPerSector = Geometry.BytesPerSector;
        DiskGeometry->NumberOfCylinders = Geometry.Cylinders;
        DiskGeometry->SectorsPerTrack = Geometry.Sectors;
        DiskGeometry->NumberOfHeads = Geometry.Heads;
    }
    else
    {
        ERR("Reading disk geometry failed\n");
        FrLdrHeapFree(PartialResourceList, TAG_HW_RESOURCE_LIST);
        return NULL;
    }
    TRACE("Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
          DriveNumber,
          DiskGeometry->NumberOfCylinders,
          DiskGeometry->NumberOfHeads,
          DiskGeometry->SectorsPerTrack,
          DiskGeometry->BytesPerSector);

    //
    // Return configuration data
    //
    *pSize = Size;
    return PartialResourceList;
}

static
VOID
DetectIsaBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
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
        TRACE("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    memset(PartialResourceList, 0, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 0;

    /* Create new bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0x0,
                           0x0,
                           0xFFFFFFFF,
                           "ISA",
                           PartialResourceList,
                           Size,
                           &BusKey);

    /* Increment bus number */
    (*BusNumber)++;

    /* Detect ISA/BIOS devices */
    DetectBiosDisks(SystemKey, BusKey);
    DetectSerialPorts(BusKey, XboxGetSerialPort, MAX_XBOX_COM_PORTS);

    /* FIXME: Detect more ISA devices */
}

static
UCHAR
XboxGetFloppyCount(VOID)
{
    /* On a PC we use CMOS/RTC I/O ports 0x70 and 0x71 to detect floppies.
     * However an Xbox CMOS memory range [0x10, 0x70) and [0x80, 0x100)
     * is filled with 0x55 0xAA 0x55 0xAA ... byte pattern which is used
     * to validate the date/time settings by Xbox OS.
     *
     * Technically it's possible to connect a floppy drive to Xbox, but
     * CMOS detection method should not be used here. */

    WARN("XboxGetFloppyCount() is UNIMPLEMENTED, returning 0\n");
    return 0;
}

PCONFIGURATION_COMPONENT_DATA
XboxHwDetect(VOID)
{
    PCONFIGURATION_COMPONENT_DATA SystemKey;
    ULONG BusNumber = 0;

    TRACE("DetectHardware()\n");

    /* Create the 'System' key */
    FldrCreateSystemKey(&SystemKey);

    GetHarddiskConfigurationData = XboxGetHarddiskConfigurationData;
    FindPciBios = XboxFindPciBios;

    /* TODO: Build actual xbox's hardware configuration tree */
    DetectPciBios(SystemKey, &BusNumber);
    DetectIsaBios(SystemKey, &BusNumber);

    TRACE("DetectHardware() Done\n");
    return SystemKey;
}

VOID XboxHwIdle(VOID)
{
    /* UNIMPLEMENTED */
}


/******************************************************************************/

VOID
MachInit(const char *CmdLine)
{
    ULONG PciId;

    memset(&MachVtbl, 0, sizeof(MACHVTBL));

    /* Check for Xbox by identifying device at PCI 0:0:0, if it's
     * 0x10DE/0x02A5 then we're running on an Xbox */
    WRITE_PORT_ULONG((ULONG *)0xCF8, CONFIG_CMD(0, 0, 0));
    PciId = READ_PORT_ULONG((ULONG *)0xCFC);
    if (PciId != 0x02A510DE)
    {
        ERR("This is not original Xbox!\n");

        /* Disable and halt the CPU */
        _disable();
        __halt();

        while (TRUE);
    }

    /* Set LEDs to red before anything is initialized */
    XboxSetLED("rrrr");

    /* Initialize our stuff */
    XboxMemInit();
    XboxVideoInit();

    /* Setup vtbl */
    MachVtbl.ConsPutChar = XboxConsPutChar;
    MachVtbl.ConsKbHit = XboxConsKbHit;
    MachVtbl.ConsGetCh = XboxConsGetCh;
    MachVtbl.VideoClearScreen = XboxVideoClearScreen;
    MachVtbl.VideoSetDisplayMode = XboxVideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = XboxVideoGetDisplaySize;
    MachVtbl.VideoGetBufferSize = XboxVideoGetBufferSize;
    MachVtbl.VideoGetFontsFromFirmware = XboxVideoGetFontsFromFirmware;
    MachVtbl.VideoSetTextCursorPosition = XboxVideoSetTextCursorPosition;
    MachVtbl.VideoHideShowTextCursor = XboxVideoHideShowTextCursor;
    MachVtbl.VideoPutChar = XboxVideoPutChar;
    MachVtbl.VideoCopyOffScreenBufferToVRAM = XboxVideoCopyOffScreenBufferToVRAM;
    MachVtbl.VideoIsPaletteFixed = XboxVideoIsPaletteFixed;
    MachVtbl.VideoSetPaletteColor = XboxVideoSetPaletteColor;
    MachVtbl.VideoGetPaletteColor = XboxVideoGetPaletteColor;
    MachVtbl.VideoSync = XboxVideoSync;
    MachVtbl.Beep = PcBeep;
    MachVtbl.PrepareForReactOS = XboxPrepareForReactOS;
    MachVtbl.GetMemoryMap = XboxMemGetMemoryMap;
    MachVtbl.GetExtendedBIOSData = XboxGetExtendedBIOSData;
    MachVtbl.GetFloppyCount = XboxGetFloppyCount;
    MachVtbl.DiskReadLogicalSectors = XboxDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = XboxDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = XboxDiskGetCacheableBlockCount;
    MachVtbl.GetTime = XboxGetTime;
    MachVtbl.InitializeBootDevices = PcInitializeBootDevices;
    MachVtbl.HwDetect = XboxHwDetect;
    MachVtbl.HwIdle = XboxHwIdle;

    /* Set LEDs to orange after init */
    XboxSetLED("oooo");

    HalpCalibrateStallExecution();
}

VOID
XboxPrepareForReactOS(VOID)
{
    /* On Xbox, prepare video and disk support */
    XboxVideoPrepareForReactOS();
    XboxDiskInit(FALSE);
    DiskStopFloppyMotor();
}

/* EOF */
