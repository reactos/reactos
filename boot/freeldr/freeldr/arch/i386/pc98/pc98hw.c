/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware detection routines for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <cportlib/cportlib.h>
#include <drivers/pc98/pit.h>
#include <drivers/pc98/fdc.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

/* Used for BIOS disks pre-enumeration performed when detecting the boot devices in InitializeBootDevices() */
extern UCHAR PcBiosDiskCount;

extern BOOLEAN HiResoMachine;

GET_HARDDISK_CONFIG_DATA GetHarddiskConfigurationData = NULL;

/* GLOBALS ********************************************************************/

#define MILLISEC     10
#define PRECISION    8
#define HZ           100

static unsigned int delay_count = 1;

PCHAR
GetHarddiskIdentifier(UCHAR DriveNumber);

/* FUNCTIONS ******************************************************************/

static VOID
__StallExecutionProcessor(ULONG Loops)
{
    register volatile unsigned int i;
    for (i = 0; i < Loops; i++);
}

VOID StallExecutionProcessor(ULONG Microseconds)
{
    ULONGLONG LoopCount = ((ULONGLONG)delay_count * (ULONGLONG)Microseconds) / 1000ULL;
    __StallExecutionProcessor((ULONG)LoopCount);
}

static VOID
WaitFor8253Wraparound(VOID)
{
    ULONG CurrentCount;
    ULONG PreviousCount = ~0;
    LONG Delta;

    CurrentCount = Read8253Timer(PitChannel0);

    do
    {
        PreviousCount = CurrentCount;
        CurrentCount = Read8253Timer(PitChannel0);
        Delta = CurrentCount - PreviousCount;
    }
    while (Delta < 300);
}

VOID
HalpCalibrateStallExecution(VOID)
{
    ULONG i;
    ULONG calib_bit;
    ULONG CurCount;
    TIMER_CONTROL_PORT_REGISTER TimerControl;
    USHORT Count = (*(PUCHAR)MEM_BIOS_FLAG1 & SYSTEM_CLOCK_8MHZ_FLAG) ?
                    (TIMER_FREQUENCY_1 / HZ) : (TIMER_FREQUENCY_2 / HZ);

    /* Initialize timer interrupt with MILLISECOND ms interval */
    TimerControl.BcdMode = FALSE;
    TimerControl.OperatingMode = PitOperatingMode2;
    TimerControl.AccessMode = PitAccessModeLowHigh;
    TimerControl.Channel = PitChannel0;
    Write8253Timer(TimerControl, Count);

    /* Stage 1: Coarse calibration */

    delay_count = 1;

    do
    {
        /* Next delay count to try */
        delay_count <<= 1;

        WaitFor8253Wraparound();

        /* Do the delay */
        __StallExecutionProcessor(delay_count);

        CurCount = Read8253Timer(PitChannel0);
    }
    while (CurCount > Count / 2);

    /* Get bottom value for delay */
    delay_count >>= 1;

    /* Stage 2: Fine calibration */

    /* Which bit are we going to test */
    calib_bit = delay_count;

    for (i = 0; i < PRECISION; i++)
    {
        /* Next bit to calibrate */
        calib_bit >>= 1;

        /* If we have done all bits, stop */
        if (!calib_bit)
            break;

        /* Set the bit in delay_count */
        delay_count |= calib_bit;

        WaitFor8253Wraparound();

        /* Do the delay */
        __StallExecutionProcessor(delay_count);

        CurCount = Read8253Timer(PitChannel0);

        /* If a tick has passed, turn the calibrated bit back off */
        if (CurCount <= Count / 2)
            delay_count &= ~calib_bit;
    }

    /* We're finished: Do the finishing touches */

    /* Calculate delay_count for 1ms */
    delay_count /= (MILLISEC / 2);
}

static UCHAR
GetFloppyType(UCHAR FloppyNumber)
{
    /* FIXME */
    return 5;
}

static VOID
DetectBiosFloppyPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_FLOPPY_DEVICE_DATA FloppyData;
    CHAR Identifier[20];
    PCONFIGURATION_COMPONENT_DATA PeripheralKey;
    ULONG Size;
    UCHAR FloppyNumber;
    UCHAR FloppyType;
    ULONG MaxDensity[6] = {0, 360, 1200, 720, 1440, 2880};

    for (FloppyNumber = 0; FloppyNumber < Pc98GetFloppyCount(); FloppyNumber++)
    {
        FloppyType = GetFloppyType(FloppyNumber);

        if ((FloppyType > 5) || (FloppyType == 0))
            continue;

        /* TODO: Properly detect */

        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "FLOPPY%d", FloppyNumber + 1);

        /* Set 'Configuration Data' value */
        Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
               sizeof(CM_FLOPPY_DEVICE_DATA);
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor! Ignoring remaining floppy peripherals. (FloppyNumber = %u, FloppyCount = %u)\n",
                FloppyNumber, Pc98GetFloppyCount());
            return;
        }
        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = 1;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_FLOPPY_DEVICE_DATA);

        /* FIXME: Don't use default parameters for 1.44 MB floppy */
        FloppyData = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));
        FloppyData->Version = 2;
        FloppyData->Revision = 0;
        FloppyData->MaxDensity = MaxDensity[FloppyType];
        FloppyData->MountDensity = 0;
        FloppyData->StepRateHeadUnloadTime = 175;
        FloppyData->HeadLoadTime = 2;
        FloppyData->MotorOffTime = 37;
        FloppyData->SectorLengthCode = 2;
        FloppyData->SectorPerTrack = 18;
        FloppyData->ReadWriteGapLength = 27;
        FloppyData->DataTransferLength = 255;
        FloppyData->FormatGapLength = 108;
        FloppyData->FormatFillCharacter = 0xF6;
        FloppyData->HeadSettleTime = 15;
        FloppyData->MotorSettleTime = 8;
        FloppyData->MaximumTrackValue = (FloppyType == 1) ? 39 : 79;
        FloppyData->DataTransferRate = 0;

        FldrCreateComponentKey(ControllerKey,
                               PeripheralClass,
                               FloppyDiskPeripheral,
                               Input | Output,
                               FloppyNumber,
                               0xFFFFFFFF,
                               Identifier,
                               PartialResourceList,
                               Size,
                               &PeripheralKey);
        TRACE("Created key: FloppyDiskPeripheral\\%d\n", FloppyNumber);
    }
}

static PCONFIGURATION_COMPONENT_DATA
DetectBiosFloppyController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    ULONG Size;
    ULONG FloppyCount;
    UCHAR i;
    UCHAR Index = 0;

    FloppyCount = Pc98GetFloppyCount();

    /* Always create a BIOS disk controller, no matter if we have floppy drives or not */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           6 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return NULL;
    }
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 7;

    /* Set I/O ports */
    for (i = 0; i < 3; i++)
    {
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypePort;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
        PartialDescriptor->u.Port.Start.LowPart = 0x90 + i * 2;
        PartialDescriptor->u.Port.Start.HighPart = 0;
        PartialDescriptor->u.Port.Length = 1;
    }
    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = 0xBE;
    PartialDescriptor->u.Port.Start.HighPart = 0;
    PartialDescriptor->u.Port.Length = 1;

    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = 0x4BE;
    PartialDescriptor->u.Port.Start.HighPart = 0;
    PartialDescriptor->u.Port.Length = 1;

    /* Set Interrupt */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypeInterrupt;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    PartialDescriptor->u.Interrupt.Level = 11;
    PartialDescriptor->u.Interrupt.Vector = 11;
    PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

    /* Set DMA channel */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypeDma;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = 0;
    PartialDescriptor->u.Dma.Channel = 2;
    PartialDescriptor->u.Dma.Port = 0;

    /* Create floppy disk controller */
    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           DiskController,
                           Output | Input,
                           0,
                           0xFFFFFFFF,
                           NULL,
                           PartialResourceList,
                           Size,
                           &ControllerKey);
    TRACE("Created key: DiskController\\0\n");

    if (FloppyCount)
        DetectBiosFloppyPeripheral(ControllerKey);

    return ControllerKey;
}

static PCM_PARTIAL_RESOURCE_LIST
Pc98GetHarddiskConfigurationData(UCHAR DriveNumber, ULONG* pSize)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
    GEOMETRY Geometry;
    ULONG Size;

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
    PartialResourceList->PartialDescriptors[0].Type = CmResourceTypeDeviceSpecific;
//  PartialResourceList->PartialDescriptors[0].ShareDisposition =
//  PartialResourceList->PartialDescriptors[0].Flags =
    PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
        sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);

    /* Get pointer to geometry data */
    DiskGeometry = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));

    /* Get the disk geometry. Extended geometry isn't supported by hardware */
    if (Pc98DiskGetDriveGeometry(DriveNumber, &Geometry))
    {
        DiskGeometry->BytesPerSector = Geometry.BytesPerSector;
        DiskGeometry->NumberOfCylinders = Geometry.Cylinders;
        DiskGeometry->SectorsPerTrack = Geometry.Sectors;
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

    *pSize = Size;
    return PartialResourceList;
}

VOID
DetectBiosDisks(
    PCONFIGURATION_COMPONENT_DATA SystemKey,
    PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCONFIGURATION_COMPONENT_DATA ControllerKey, DiskKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_INT13_DRIVE_PARAMETER Int13Drives;
    GEOMETRY Geometry;
    UCHAR DiskCount, DriveNumber;
    USHORT i;
    ULONG Size;

    /* The pre-enumeration of the BIOS disks was already done in InitializeBootDevices() */
    DiskCount = PcBiosDiskCount;

    /* Use the floppy disk controller as our controller */
    ControllerKey = DetectBiosFloppyController(BusKey);
    if (!ControllerKey)
    {
        ERR("Failed to detect BIOS disk controller\n");
        return;
    }

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;

    PartialResourceList->PartialDescriptors[0].Type = CmResourceTypeDeviceSpecific;
    PartialResourceList->PartialDescriptors[0].ShareDisposition = 0;
    PartialResourceList->PartialDescriptors[0].Flags = 0;
    PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
        sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;

    /* Get hard disk Int13 geometry data */
    Int13Drives = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));
    for (i = 0; i < DiskCount; i++)
    {
        DriveNumber = 0x80 + i;

        if (Pc98DiskGetDriveGeometry(DriveNumber, &Geometry))
        {
            Int13Drives[i].DriveSelect = DriveNumber;
            Int13Drives[i].MaxCylinders = Geometry.Cylinders - 1;
            Int13Drives[i].SectorsPerTrack = (USHORT)Geometry.Sectors;
            Int13Drives[i].MaxHeads = (USHORT)Geometry.Heads - 1;
            Int13Drives[i].NumberDrives = DiskCount;

            TRACE("Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
                  DriveNumber,
                  Geometry.Cylinders - 1,
                  Geometry.Heads - 1,
                  Geometry.Sectors,
                  Geometry.BytesPerSector);
        }
    }

    /* Update the 'System' key's configuration data with BIOS INT13h information */
    FldrSetConfigurationData(SystemKey, PartialResourceList, Size);

    /* Create and fill subkey for each harddisk */
    for (i = 0; i < DiskCount; i++)
    {
        PCHAR Identifier;

        DriveNumber = 0x80 + i;

        /* Get disk values */
        PartialResourceList = GetHarddiskConfigurationData(DriveNumber, &Size);
        Identifier = GetHarddiskIdentifier(DriveNumber);

        /* Create disk key */
        FldrCreateComponentKey(ControllerKey,
                               PeripheralClass,
                               DiskPeripheral,
                               Output | Input,
                               i,
                               0xFFFFFFFF,
                               Identifier,
                               PartialResourceList,
                               Size,
                               &DiskKey);
        TRACE("Created key: DiskPeripheral\\%d\n", i);
    }
}

static VOID
DetectPointerPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCONFIGURATION_COMPONENT_DATA PeripheralKey;
    ULONG Size;

    /* TODO: Properly detect */

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
                           "NEC PC-9800 BUS MOUSE",
                           PartialResourceList,
                           Size,
                           &PeripheralKey);
    TRACE("Created key: PointerPeripheral\\0\n");
}

static VOID
DetectPointerController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    ULONG Size;
    UCHAR i;
    UCHAR Index = 0;

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           (HiResoMachine ? 6 : 5) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = (HiResoMachine ? 7 : 6);

    /* Set I/O ports */
    for (i = 0; i < 4; i++)
    {
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypePort;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
        PartialDescriptor->u.Port.Start.LowPart = (HiResoMachine ? 0x61 : 0x7FD9) + i * 2;
        PartialDescriptor->u.Port.Start.HighPart = 0;
        PartialDescriptor->u.Port.Length = 1;
    }
    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = (HiResoMachine ? 0x869 : 0xBFDB);
    PartialDescriptor->u.Port.Start.HighPart = 0;
    PartialDescriptor->u.Port.Length = 1;
    if (HiResoMachine)
    {
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypePort;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
        PartialDescriptor->u.Port.Start.LowPart = 0x98D7;
        PartialDescriptor->u.Port.Start.HighPart = 0;
        PartialDescriptor->u.Port.Length = 1;
    }

    /* Set Interrupt */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypeInterrupt;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    PartialDescriptor->u.Interrupt.Level = 13;
    PartialDescriptor->u.Interrupt.Vector = 13;
    PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

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
    TRACE("Created key: PointerController\\0\n");

    DetectPointerPeripheral(ControllerKey);
}

static VOID
DetectKeyboardPeripheral(PCONFIGURATION_COMPONENT_DATA ControllerKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_KEYBOARD_DEVICE_DATA KeyboardData;
    PCONFIGURATION_COMPONENT_DATA PeripheralKey;
    CHAR Identifier[80];
    ULONG Size;
    REGS Regs;
    UCHAR KeyboardType = ((*(PUCHAR)MEM_KEYB_TYPE & 0x40) >> 5) |
                         ((*(PUCHAR)MEM_KEYB_TYPE & 0x08) >> 3);

    /* TODO: Properly detect */

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(CM_KEYBOARD_DEVICE_DATA);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;

    PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
    PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_KEYBOARD_DEVICE_DATA);

    /* Int 18h AH=02h
     * KEYBOARD - GET SHIFT FLAGS
     *
     * Return:
     * AL - shift flags
     */
    Regs.b.ah = 0x02;
    Int386(0x18, &Regs, &Regs);

    KeyboardData = (PCM_KEYBOARD_DEVICE_DATA)(PartialDescriptor + 1);
    KeyboardData->Version = 1;
    KeyboardData->Revision = 1;
    KeyboardData->Type = 7;
    KeyboardData->Subtype = 1;
    KeyboardData->KeyboardFlags = (Regs.b.al & 0x08) |
                                  ((Regs.b.al & 0x02) << 6) |
                                  ((Regs.b.al & 0x10) << 2) |
                                  ((Regs.b.al & 0x01) << 1);

    if (KeyboardType == 0)
        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "PC98_NmodeKEY");
    else if (KeyboardType == 2)
        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "PC98_106KEY");
    else
        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "PC98_LaptopKEY");

    /* Create controller key */
    FldrCreateComponentKey(ControllerKey,
                           PeripheralClass,
                           KeyboardPeripheral,
                           Input | ConsoleIn,
                           0,
                           0xFFFFFFFF,
                           Identifier,
                           PartialResourceList,
                           Size,
                           &PeripheralKey);
    TRACE("Created key: KeyboardPeripheral\\0\n");
}

static VOID
DetectKeyboardController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    ULONG Size;
    UCHAR i;

    if (!CpDoesPortExist((PUCHAR)0x41))
        return;

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 3;

    /* Set I/O ports */
    for (i = 0; i < 2; i++)
    {
        PartialDescriptor = &PartialResourceList->PartialDescriptors[i];
        PartialDescriptor->Type = CmResourceTypePort;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
        PartialDescriptor->u.Port.Start.LowPart = 0x41 + i * 2;
        PartialDescriptor->u.Port.Start.HighPart = 0;
        PartialDescriptor->u.Port.Length = 1;
    }

    /* Set Interrupt */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[2];
    PartialDescriptor->Type = CmResourceTypeInterrupt;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    PartialDescriptor->u.Interrupt.Level = 1;
    PartialDescriptor->u.Interrupt.Vector = 1;
    PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

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
    TRACE("Created key: KeyboardController\\0\n");

    DetectKeyboardPeripheral(ControllerKey);
}

static VOID
DetectParallelPorts(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    ULONG Size;
    UCHAR i;
    UCHAR Index = 0;

    /* TODO: Properly detect */

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           7 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 8;

    /* Set I/O ports */
    for (i = 0; i < 4; i++)
    {
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypePort;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
        PartialDescriptor->u.Port.Start.LowPart = 0x40 + i * 2;
        PartialDescriptor->u.Port.Start.HighPart = 0;
        PartialDescriptor->u.Port.Length = 3;
    }

    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = 0x140;
    PartialDescriptor->u.Port.Start.HighPart = 0;
    PartialDescriptor->u.Port.Length = 3;

    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = 0x149;
    PartialDescriptor->u.Port.Start.HighPart = 0;
    PartialDescriptor->u.Port.Length = 1;

    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypePort;
    PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
    PartialDescriptor->u.Port.Start.LowPart = 0x14B;
    PartialDescriptor->u.Port.Start.HighPart = 0;
    PartialDescriptor->u.Port.Length = 4;

    /* Set Interrupt */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
    PartialDescriptor->Type = CmResourceTypeInterrupt;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    PartialDescriptor->u.Interrupt.Level = 14;
    PartialDescriptor->u.Interrupt.Vector = 14;
    PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

    /* Create controller key */
    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           ParallelController,
                           Output,
                           0,
                           0xFFFFFFFF,
                           "PARALLEL1",
                           PartialResourceList,
                           Size,
                           &ControllerKey);
    TRACE("Created key: ParallelController\\0\n");
}

static VOID
DetectSerialPorts(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_SERIAL_DEVICE_DATA SerialDeviceData;
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    CHAR Identifier[80];
    UCHAR i;
    ULONG Size;
    UCHAR FifoStatus;
    BOOLEAN HasFifo;
    UCHAR Index = 0;
    ULONG ControllerNumber = 0;

    if (CpDoesPortExist((PUCHAR)0x30))
    {
        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "COM%d", ControllerNumber + 1);

        FifoStatus = READ_PORT_UCHAR((PUCHAR)0x136) & 0x40;
        StallExecutionProcessor(5);
        HasFifo = ((READ_PORT_UCHAR((PUCHAR)0x136) & 0x40) != FifoStatus);

        /* Set 'Configuration Data' value */
        Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
               (HasFifo ? 10 : 3) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
               sizeof(CM_SERIAL_DEVICE_DATA);
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }
        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = (HasFifo ? 11 : 4);

        /* Set I/O ports */
        for (i = 0; i < 2; i++)
        {
            PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
            PartialDescriptor->Type = CmResourceTypePort;
            PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
            PartialDescriptor->u.Port.Start.LowPart = 0x30 + i * 2;
            PartialDescriptor->u.Port.Start.HighPart = 0;
            PartialDescriptor->u.Port.Length = 1;
        }
        if (HasFifo)
        {
            for (i = 0; i < 7; i++)
            {
                PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
                PartialDescriptor->Type = CmResourceTypePort;
                PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
                PartialDescriptor->u.Port.Start.LowPart = 0x130 + i * 2;
                PartialDescriptor->u.Port.Start.HighPart = 0;
                PartialDescriptor->u.Port.Length = 1;
            }
        }

        /* Set Interrupt */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypeInterrupt;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        PartialDescriptor->u.Interrupt.Level = 4;
        PartialDescriptor->u.Interrupt.Vector = 4;
        PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

        /* Set serial data (device specific) */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->Flags = 0;
        PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_SERIAL_DEVICE_DATA);

        SerialDeviceData = (PCM_SERIAL_DEVICE_DATA)&PartialResourceList->PartialDescriptors[Index++];
        SerialDeviceData->BaudClock = (*(PUCHAR)MEM_BIOS_FLAG1 & SYSTEM_CLOCK_8MHZ_FLAG) ?
                                       TIMER_FREQUENCY_1 : TIMER_FREQUENCY_2;

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
        TRACE("Created key: SerialController\\%d\n", ControllerNumber);

        ++ControllerNumber;
    }

    if (CpDoesPortExist((PUCHAR)0x238))
    {
        Index = 0;

        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "COM%d", ControllerNumber + 1);

        /* Set 'Configuration Data' value */
        Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
               2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
               sizeof(CM_SERIAL_DEVICE_DATA);
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }
        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = 3;

        /* Set I/O ports */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypePort;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = CM_RESOURCE_PORT_IO;
        PartialDescriptor->u.Port.Start.LowPart = 0x238;
        PartialDescriptor->u.Port.Start.HighPart = 0;
        PartialDescriptor->u.Port.Length = 8;

        /* Set Interrupt */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypeInterrupt;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        PartialDescriptor->u.Interrupt.Level = 5;
        PartialDescriptor->u.Interrupt.Vector = 5;
        PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

        /* Set serial data (device specific) */
        PartialDescriptor = &PartialResourceList->PartialDescriptors[Index++];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->Flags = 0;
        PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(CM_SERIAL_DEVICE_DATA);

        SerialDeviceData = (PCM_SERIAL_DEVICE_DATA)&PartialResourceList->PartialDescriptors[Index++];
        SerialDeviceData->BaudClock = 1843200;

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
        TRACE("Created key: SerialController\\%d\n", ControllerNumber);

        ++ControllerNumber;
    }
}

static VOID
DetectCBusBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
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
    RtlZeroMemory(PartialResourceList, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 0;

    /* Create bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0x0,
                           0,
                           0xFFFFFFFF,
                           "ISA",
                           PartialResourceList,
                           Size,
                           &BusKey);

    /* Increment bus number */
    (*BusNumber)++;

    /* Detect C-bus/BIOS devices */
    DetectBiosDisks(SystemKey, BusKey);
    DetectSerialPorts(BusKey);
    DetectParallelPorts(BusKey);
    DetectKeyboardController(BusKey);
    DetectPointerController(BusKey);

    /* FIXME: Detect more C-bus devices */
}

static VOID
DetectNesaBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG Size;

    if (!((*(PUCHAR)MEM_BIOS_FLAG5) & NESA_BUS_FLAG))
        return;

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

    /* Create bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0x0,
                           0,
                           0xFFFFFFFF,
                           "EISA",
                           PartialResourceList,
                           Size,
                           &BusKey);

    /* Increment bus number */
    (*BusNumber)++;
}

// FIXME: Copied from machpc.c
static VOID
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
    RtlCopyMemory(Ptr, InstData, sizeof(CM_PNP_BIOS_INSTALLATION_CHECK));
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

            RtlCopyMemory(Ptr, DeviceNode, DeviceNode->Size);

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
                           0x0,
                           0x0,
                           0xFFFFFFFF,
                           "PNP BIOS",
                           PartialResourceList,
                           Size,
                           &BusKey);

    (*BusNumber)++;
}

PCONFIGURATION_COMPONENT_DATA
Pc98HwDetect(VOID)
{
    PCONFIGURATION_COMPONENT_DATA SystemKey;
    ULONG BusNumber = 0;

    TRACE("DetectHardware()\n");

    /* Create the 'System' key */
    FldrCreateSystemKey(&SystemKey);
    FldrSetIdentifier(SystemKey, "NEC PC-98");

    GetHarddiskConfigurationData = Pc98GetHarddiskConfigurationData;
    FindPciBios = PcFindPciBios;

    /* Detect buses */
    DetectPciBios(SystemKey, &BusNumber);
    DetectApmBios(SystemKey, &BusNumber);
    DetectPnpBios(SystemKey, &BusNumber);
    DetectNesaBios(SystemKey, &BusNumber);
    DetectCBusBios(SystemKey, &BusNumber);
    DetectAcpiBios(SystemKey, &BusNumber);
    // TODO: Detect more buses

    // TODO: Collect the ROM blocks and append their
    // CM_ROM_BLOCK data into the 'System' key's configuration data.

    TRACE("DetectHardware() Done\n");
    return SystemKey;
}

UCHAR
Pc98GetFloppyCount(VOID)
{
    USHORT DiskEquipment = *(PUSHORT)MEM_DISK_EQUIP & ~(*(PUCHAR)MEM_RDISK_EQUIP);
    UCHAR DiskMask;
    UCHAR FloppyCount = 0;

    for (DiskMask = 0x01; DiskMask != 0; DiskMask <<= 1)
    {
        if (FIRSTBYTE(DiskEquipment) & DiskMask)
            ++FloppyCount;
    }

    for (DiskMask = 0x10; DiskMask != 0; DiskMask <<= 1)
    {
        if (SECONDBYTE(DiskEquipment) & DiskMask)
            ++FloppyCount;
    }

    return FloppyCount;
}

VOID __cdecl DiskStopFloppyMotor(VOID)
{
    WRITE_PORT_UCHAR((PUCHAR)(FDC1_IO_BASE + FDC_o_CONTROL), 0);
    WRITE_PORT_UCHAR((PUCHAR)(FDC2_IO_BASE + FDC_o_CONTROL), 0);
}

// FIXME: 1) Copied from pchw.c 2) Should be done inside MachInit.
VOID
FrLdrCheckCpuCompatibility(VOID)
{
    INT CpuInformation[4] = {-1};
    ULONG NumberOfIds;

    /* Check if the processor first supports ID 1 */
    __cpuid(CpuInformation, 0);

    NumberOfIds = CpuInformation[0];

    if (NumberOfIds == 0)
    {
        FrLdrBugCheckWithMessage(MISSING_HARDWARE_REQUIREMENTS,
                                 __FILE__,
                                 __LINE__,
                                 "ReactOS requires the CPUID instruction to return "
                                 "more than one supported ID.\n\n");
    }

    /* NumberOfIds will be greater than 1 if the processor is new enough */
    if (NumberOfIds == 1)
    {
        INT ProcessorFamily;

        /* Get information */
        __cpuid(CpuInformation, 1);

        ProcessorFamily = (CpuInformation[0] >> 8) & 0xF;

        /* If it's Family 4 or lower, bugcheck */
        if (ProcessorFamily < 5)
        {
            FrLdrBugCheckWithMessage(MISSING_HARDWARE_REQUIREMENTS,
                                     __FILE__,
                                     __LINE__,
                                     "Processor is too old (family %u < 5)\n"
                                     "ReactOS requires a Pentium-level processor or newer.",
                                     ProcessorFamily);
        }
    }
}
