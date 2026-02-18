/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Global pageable data
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

ATAPORT_PAGED_DATA
static const struct
{
    PCSTR DeviceType;
    PCSTR GenericType;
    PCSTR PeripheralId;
} AtapGenericDeviceNames[] =
{
    {"Disk",       "GenDisk",       "DiskPeripheral"          }, // DIRECT_ACCESS_DEVICE
    {"Sequential", "GenSequential", "TapePeripheral"          }, // SEQUENTIAL_ACCESS_DEVICE
    {"Printer",    "GenPrinter",    "PrinterPeripheral"       }, // PRINTER_DEVICE
    {"Processor",  "GenProcessor",  "ProcessorPeripheral"     }, // PROCESSOR_DEVICE
    {"Worm",       "GenWorm",       "WormPeripheral"          }, // WRITE_ONCE_READ_MULTIPLE_DEVICE
    {"CdRom",      "GenCdRom",      "CdRomPeripheral"         }, // READ_ONLY_DIRECT_ACCESS_DEVICE
    {"Scanner",    "GenScanner",    "ScannerPeripheral"       }, // SCANNER_DEVICE
    {"Optical",    "GenOptical",    "OpticalDiskPeripheral"   }, // OPTICAL_DEVICE
    {"Changer",    "GenChanger",    "MediumChangerPeripheral" }, // MEDIUM_CHANGER
    {"Net",        "GenNet",        "CommunicationsPeripheral"}, // COMMUNICATION_DEVICE
    {"Other",      "IdeOther",      "OtherPeripheral"         }, // 10
};

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
PCSTR
AtaTypeCodeToName(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ DEVICE_TYPE_NAME Type)
{
    ULONG DeviceType;

    PAGED_CODE();

    DeviceType = DevExt->InquiryData.DeviceType;
    DeviceType = min(DeviceType, RTL_NUMBER_OF(AtapGenericDeviceNames) - 1);

    switch (Type)
    {
        case GetDeviceType:
            return AtapGenericDeviceNames[DeviceType].DeviceType;

        case GetPeripheralId:
            return AtapGenericDeviceNames[DeviceType].PeripheralId;

        case GetGenericType:
            if (DevExt->Device.DeviceFlags & DEVICE_IS_SUPER_FLOPPY)
                return "GenSFloppy"; // Install the Super Floppy storage class driver
            else
                return AtapGenericDeviceNames[DeviceType].GenericType;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
}
