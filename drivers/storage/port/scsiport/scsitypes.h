
// See https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-scsi-devices
// and https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-ide-devices

struct
{
    PCSTR  DeviceType;
    PCSTR  GenericType;
    PCWSTR PeripheralType;
} DeviceTypeIds[] =
{
    {"Disk",        "GenDisk",          L"DiskPeripheral"               }, // DIRECT_ACCESS_DEVICE
    {"Sequential",  "",                 L"TapePeripheral"               }, // SEQUENTIAL_ACCESS_DEVICE
    {"Printer",     "GenPrinter",       L"PrinterPeripheral"            }, // PRINTER_DEVICE
    {"Processor",   "",                 L"OtherPeripheral"              }, // PROCESSOR_DEVICE
    {"Worm",        "GenWorm",          L"WormPeripheral"               }, // WRITE_ONCE_READ_MULTIPLE_DEVICE
    {"CdRom",       "GenCdRom",         L"CdRomPeripheral"              }, // READ_ONLY_DIRECT_ACCESS_DEVICE
    {"Scanner",     "GenScanner",       L"ScannerPeripheral"            }, // SCANNER_DEVICE
    {"Optical",     "GenOptical",       L"OpticalDiskPeripheral"        }, // OPTICAL_DEVICE
    {"Changer",     "ScsiChanger",      L"MediumChangerPeripheral"      }, // MEDIUM_CHANGER
    {"Net",         "ScsiNet",          L"CommunicationsPeripheral"     }, // COMMUNICATION_DEVICE
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral"}, // 10
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral"}, // 11
    {"Array",       "ScsiArray",        L"ArrayPeripheral"              }, // ARRAY_CONTROLLER_DEVICE
    {"Enclosure",   "ScsiEnclosure",    L"EnclosurePeripheral"          }, // SCSI_ENCLOSURE_DEVICE
    {"RBC",         "ScsiRBC",          L"RBCPeripheral"                }, // REDUCED_BLOCK_DEVICE
    {"CardReader",  "ScsiCardReader",   L"CardReaderPeripheral"         }, // OPTICAL_CARD_READER_WRITER_DEVICE
    {"Bridge",      "ScsiBridge",       L"BridgePeripheral"             }, // BRIDGE_CONTROLLER_DEVICE
    {"Other",       "ScsiOther",        L"OtherPeripheral"              }
};

FORCEINLINE
PCSTR
GetDeviceType(
    _In_ PINQUIRYDATA InquiryData)
{
    if (InquiryData->DeviceType < RTL_NUMBER_OF(DeviceTypeIds))
        return DeviceTypeIds[InquiryData->DeviceType].DeviceType;
    else
        return DeviceTypeIds[RTL_NUMBER_OF(DeviceTypeIds)-1].DeviceType;
}

FORCEINLINE
PCSTR
GetGenericType(
    _In_ PINQUIRYDATA InquiryData)
{
    if (InquiryData->DeviceType < RTL_NUMBER_OF(DeviceTypeIds))
        return DeviceTypeIds[InquiryData->DeviceType].GenericType;
    else
        return DeviceTypeIds[RTL_NUMBER_OF(DeviceTypeIds)-1].GenericType;
}

FORCEINLINE
PCWSTR
GetPeripheralTypeW(
    _In_ PINQUIRYDATA InquiryData)
{
    if (InquiryData->DeviceType < RTL_NUMBER_OF(DeviceTypeIds))
        return DeviceTypeIds[InquiryData->DeviceType].PeripheralType;
    else
        return DeviceTypeIds[RTL_NUMBER_OF(DeviceTypeIds)-1].PeripheralType;
}
