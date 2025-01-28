
// see https://learn.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-scsi-devices
// and https://learn.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-ide-devices

FORCEINLINE
PCSTR
GetDeviceType(
    _In_ PINQUIRYDATA InquiryData)
{
    switch (InquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return "Disk";
        case SEQUENTIAL_ACCESS_DEVICE:
            return "Sequential";
        case PRINTER_DEVICE:
            return "Printer";
        case PROCESSOR_DEVICE:
            return "Processor";
        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return "Worm";
        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return "CdRom";
        case SCANNER_DEVICE:
            return "Scanner";
        case OPTICAL_DEVICE:
            return "Optical";
        case MEDIUM_CHANGER:
            return "Changer";
        case COMMUNICATION_DEVICE:
            return "Net";
        case ARRAY_CONTROLLER_DEVICE:
            return "Array";
        case SCSI_ENCLOSURE_DEVICE:
            return "Enclosure";
        case REDUCED_BLOCK_DEVICE:
            return "RBC";
        case OPTICAL_CARD_READER_WRITER_DEVICE:
            return "CardReader";
        case BRIDGE_CONTROLLER_DEVICE:
            return "Bridge";
        default:
            return "Other";
    }
}

FORCEINLINE
PCSTR
GetGenericType(
    _In_ PINQUIRYDATA InquiryData)
{
    switch (InquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return "GenDisk";
        case PRINTER_DEVICE:
            return "GenPrinter";
        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return "GenWorm";
        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return "GenCdRom";
        case SCANNER_DEVICE:
            return "GenScanner";
        case OPTICAL_DEVICE:
            return "GenOptical";
        case MEDIUM_CHANGER:
            return "ScsiChanger";
        case COMMUNICATION_DEVICE:
            return "ScsiNet";
        case ARRAY_CONTROLLER_DEVICE:
            return "ScsiArray";
        case SCSI_ENCLOSURE_DEVICE:
            return "ScsiEnclosure";
        case REDUCED_BLOCK_DEVICE:
            return "ScsiRBC";
        case OPTICAL_CARD_READER_WRITER_DEVICE:
            return "ScsiCardReader";
        case BRIDGE_CONTROLLER_DEVICE:
            return "ScsiBridge";
        default:
            return "ScsiOther";
    }
}

FORCEINLINE
PCWSTR
GetPeripheralTypeW(
    _In_ PINQUIRYDATA InquiryData)
{
    switch (InquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return L"DiskPeripheral";
        case SEQUENTIAL_ACCESS_DEVICE:
            return L"TapePeripheral";
        case PRINTER_DEVICE:
            return L"PrinterPeripheral";
        // case 3: "ProcessorPeripheral", classified as 'other': fall back to default case.
        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return L"WormPeripheral";
        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return L"CdRomPeripheral";
        case SCANNER_DEVICE:
            return L"ScannerPeripheral";
        case OPTICAL_DEVICE:
            return L"OpticalDiskPeripheral";
        case MEDIUM_CHANGER:
            return L"MediumChangerPeripheral";
        case COMMUNICATION_DEVICE:
            return L"CommunicationsPeripheral";
        case ARRAY_CONTROLLER_DEVICE:
            return L"ArrayPeripheral";
        case SCSI_ENCLOSURE_DEVICE:
            return L"EnclosurePeripheral";
        case REDUCED_BLOCK_DEVICE:
            return L"RBCPeripheral";
        case OPTICAL_CARD_READER_WRITER_DEVICE:
            return L"CardReaderPeripheral";
        case BRIDGE_CONTROLLER_DEVICE:
            return L"BridgePeripheral";
        default:
            return L"OtherPeripheral";
    }
}
