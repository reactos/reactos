/*
 *  FreeLoader
 *
 *  Copyright (C) 2003, 2004  Eric Kohl
 *  Copyright (C) 2009  Hervé Poussineau
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

#define MILLISEC     (10)
#define PRECISION    (8)

#if defined(SARCH_XBOX)
#define CLOCK_TICK_RATE 1125000
#else
#define CLOCK_TICK_RATE 1193182
#endif

#define HZ (100)
#define LATCH (CLOCK_TICK_RATE / HZ)

static unsigned int delay_count = 1;

/* Used for BIOS disks pre-enumeration performed when detecting the boot devices in InitializeBootDevices() */
extern UCHAR PcBiosDiskCount;

/* This function is slightly different in its PC and XBOX versions */
GET_HARDDISK_CONFIG_DATA GetHarddiskConfigurationData = NULL;

PCHAR
GetHarddiskIdentifier(UCHAR DriveNumber);

/* FUNCTIONS *****************************************************************/

static
VOID
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

static
ULONG
Read8254Timer(VOID)
{
    ULONG Count;

    WRITE_PORT_UCHAR((PUCHAR)0x43, 0x00);
    Count = READ_PORT_UCHAR((PUCHAR)0x40);
    Count |= READ_PORT_UCHAR((PUCHAR)0x40) << 8;

    return Count;
}

static
VOID
WaitFor8254Wraparound(VOID)
{
    ULONG CurCount;
    ULONG PrevCount = ~0;
    LONG Delta;

    CurCount = Read8254Timer();

    do
    {
        PrevCount = CurCount;
        CurCount = Read8254Timer();
        Delta = CurCount - PrevCount;

        /*
         * This limit for delta seems arbitrary, but it isn't, it's
         * slightly above the level of error a buggy Mercury/Neptune
         * chipset timer can cause.
         */
    }
    while (Delta < 300);
}

VOID
HalpCalibrateStallExecution(VOID)
{
    ULONG i;
    ULONG calib_bit;
    ULONG CurCount;

    /* Initialise timer interrupt with MILLISECOND ms interval        */
    WRITE_PORT_UCHAR((PUCHAR)0x43, 0x34);  /* binary, mode 2, LSB/MSB, ch 0 */
    WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH & 0xff); /* LSB */
    WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH >> 8); /* MSB */

    /* Stage 1:  Coarse calibration                                   */

    delay_count = 1;

    do
    {
        /* Next delay count to try */
        delay_count <<= 1;

        WaitFor8254Wraparound();

        /* Do the delay */
        __StallExecutionProcessor(delay_count);

        CurCount = Read8254Timer();
    }
    while (CurCount > LATCH / 2);

    /* Get bottom value for delay */
    delay_count >>= 1;

    /* Stage 2:  Fine calibration                                     */

    /* Which bit are we going to test */
    calib_bit = delay_count;

    for (i = 0; i < PRECISION; i++)
    {
        /* Next bit to calibrate */
        calib_bit >>= 1;

        /* If we have done all bits, stop */
        if (!calib_bit) break;

        /* Set the bit in delay_count */
        delay_count |= calib_bit;

        WaitFor8254Wraparound();

        /* Do the delay */
        __StallExecutionProcessor(delay_count);

        CurCount = Read8254Timer();
        /* If a tick has passed, turn the calibrated bit back off */
        if (CurCount <= LATCH / 2)
            delay_count &= ~calib_bit;
    }

    /* We're finished:  Do the finishing touches */

    /* Calculate delay_count for 1ms */
    delay_count /= (MILLISEC / 2);
}


static
UCHAR
GetFloppyType(UCHAR DriveNumber)
{
    UCHAR Data;

    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x10);
    Data = READ_PORT_UCHAR((PUCHAR)0x71);

    if (DriveNumber == 0)
        return Data >> 4;
    else if (DriveNumber == 1)
        return Data & 0x0F;

    return 0;
}

static
PVOID
GetInt1eTable(VOID)
{
    PUSHORT SegPtr = (PUSHORT)0x7A;
    PUSHORT OfsPtr = (PUSHORT)0x78;

    return (PVOID)((ULONG_PTR)(((ULONG)(*SegPtr)) << 4) + (ULONG)(*OfsPtr));
}

static
VOID
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
    PUCHAR Ptr;

    for (FloppyNumber = 0; FloppyNumber < 2; FloppyNumber++)
    {
        FloppyType = GetFloppyType(FloppyNumber);

        if ((FloppyType > 5) || (FloppyType == 0))
            continue;

        if (!DiskResetController(FloppyNumber))
            continue;

        Ptr = GetInt1eTable();

        /* Set 'Identifier' value */
        RtlStringCbPrintfA(Identifier, sizeof(Identifier), "FLOPPY%d", FloppyNumber + 1);

        Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
               sizeof(CM_FLOPPY_DEVICE_DATA);
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor! Ignoring remaining floppy peripherals. (FloppyNumber = %u)\n",
                FloppyNumber);
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

        FloppyData = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));
        FloppyData->Version = 2;
        FloppyData->Revision = 0;
        FloppyData->MaxDensity = MaxDensity[FloppyType];
        FloppyData->MountDensity = 0;
        RtlCopyMemory(&FloppyData->StepRateHeadUnloadTime, Ptr, 11);
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
    }
}

static
PCONFIGURATION_COMPONENT_DATA
DetectBiosFloppyController(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCONFIGURATION_COMPONENT_DATA ControllerKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    ULONG Size;
    ULONG FloppyCount;

    FloppyCount = MachGetFloppyCount();
    TRACE("Floppy count: %u\n", FloppyCount);

    /* Always create a BIOS disk controller, no matter if we have floppy drives or not */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return NULL;
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
    PartialDescriptor->u.Port.Start.LowPart = 0x03F0;
    PartialDescriptor->u.Port.Start.HighPart = 0x0;
    PartialDescriptor->u.Port.Length = 8;

    /* Set Interrupt */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
    PartialDescriptor->Type = CmResourceTypeInterrupt;
    PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
    PartialDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    PartialDescriptor->u.Interrupt.Level = 6;
    PartialDescriptor->u.Interrupt.Vector = 6;
    PartialDescriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

    /* Set DMA channel */
    PartialDescriptor = &PartialResourceList->PartialDescriptors[2];
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
                           0x0,
                           0xFFFFFFFF,
                           NULL,
                           PartialResourceList,
                           Size,
                           &ControllerKey);

    if (FloppyCount)
        DetectBiosFloppyPeripheral(ControllerKey);

    return ControllerKey;
}

VOID
DetectBiosDisks(PCONFIGURATION_COMPONENT_DATA SystemKey,
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

    /* Allocate resource descriptor */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;
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
    PartialResourceList->PartialDescriptors[0].Type = CmResourceTypeDeviceSpecific;
    PartialResourceList->PartialDescriptors[0].ShareDisposition = 0;
    PartialResourceList->PartialDescriptors[0].Flags = 0;
    PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
        sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;

    /* Get harddisk Int13 geometry data */
    Int13Drives = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));
    for (i = 0; i < DiskCount; i++)
    {
        DriveNumber = 0x80 + i;

        if (MachDiskGetDriveGeometry(DriveNumber, &Geometry))
        {
            Int13Drives[i].DriveSelect = DriveNumber;
            Int13Drives[i].MaxCylinders = Geometry.Cylinders - 1;
            Int13Drives[i].SectorsPerTrack = (USHORT)Geometry.SectorsPerTrack;
            Int13Drives[i].MaxHeads = (USHORT)Geometry.Heads - 1;
            Int13Drives[i].NumberDrives = DiskCount;

            TRACE("Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
                  DriveNumber,
                  Geometry.Cylinders - 1,
                  Geometry.Heads - 1,
                  Geometry.SectorsPerTrack,
                  Geometry.BytesPerSector);
        }
    }

    /* Update the 'System' key's configuration data with BIOS INT13h information */
    FldrSetConfigurationData(SystemKey, PartialResourceList, Size);

    /* Create and fill subkey for each harddisk */
    for (i = 0; i < DiskCount; i++)
    {
        PCSTR Identifier;

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
    }
}

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

/* EOF */
