/*******************************************************************************
 *
 * Module Name: rsdumpinfo - Tables used to display resource descriptors.
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "acpi.h"
#include "accommon.h"
#include "acresrc.h"

#define _COMPONENT          ACPI_RESOURCES
        ACPI_MODULE_NAME    ("rsdumpinfo")


#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DISASSEMBLER) || defined(ACPI_DEBUGGER)


#define ACPI_RSD_OFFSET(f)          (UINT8) ACPI_OFFSET (ACPI_RESOURCE_DATA,f)
#define ACPI_PRT_OFFSET(f)          (UINT8) ACPI_OFFSET (ACPI_PCI_ROUTING_TABLE,f)
#define ACPI_RSD_TABLE_SIZE(name)   (sizeof(name) / sizeof (ACPI_RSDUMP_INFO))


/*******************************************************************************
 *
 * Resource Descriptor info tables
 *
 * Note: The first table entry must be a Title or Literal and must contain
 * the table length (number of table entries)
 *
 ******************************************************************************/

ACPI_RSDUMP_INFO        AcpiRsDumpIrq[7] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpIrq),                "IRQ",                      NULL},
    {ACPI_RSD_UINT8 ,   ACPI_RSD_OFFSET (Irq.DescriptorLength),             "Descriptor Length",        NULL},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Irq.Triggering),                   "Triggering",               AcpiGbl_HeDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Irq.Polarity),                     "Polarity",                 AcpiGbl_LlDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Irq.Sharable),                     "Sharing",                  AcpiGbl_ShrDecode},
    {ACPI_RSD_UINT8 ,   ACPI_RSD_OFFSET (Irq.InterruptCount),               "Interrupt Count",          NULL},
    {ACPI_RSD_SHORTLIST,ACPI_RSD_OFFSET (Irq.Interrupts[0]),                "Interrupt List",           NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpDma[6] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpDma),                "DMA",                      NULL},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Dma.Type),                         "Speed",                    AcpiGbl_TypDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Dma.BusMaster),                    "Mastering",                AcpiGbl_BmDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Dma.Transfer),                     "Transfer Type",            AcpiGbl_SizDecode},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (Dma.ChannelCount),                 "Channel Count",            NULL},
    {ACPI_RSD_SHORTLIST,ACPI_RSD_OFFSET (Dma.Channels[0]),                  "Channel List",             NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpStartDpf[4] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpStartDpf),           "Start-Dependent-Functions",NULL},
    {ACPI_RSD_UINT8 ,   ACPI_RSD_OFFSET (StartDpf.DescriptorLength),        "Descriptor Length",        NULL},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (StartDpf.CompatibilityPriority),   "Compatibility Priority",   AcpiGbl_ConfigDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (StartDpf.PerformanceRobustness),   "Performance/Robustness",   AcpiGbl_ConfigDecode}
};

ACPI_RSDUMP_INFO        AcpiRsDumpEndDpf[1] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpEndDpf),             "End-Dependent-Functions",  NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpIo[6] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpIo),                 "I/O",                      NULL},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Io.IoDecode),                      "Address Decoding",         AcpiGbl_IoDecode},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Io.Minimum),                       "Address Minimum",          NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Io.Maximum),                       "Address Maximum",          NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (Io.Alignment),                     "Alignment",                NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (Io.AddressLength),                 "Address Length",           NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpFixedIo[3] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpFixedIo),            "Fixed I/O",                NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (FixedIo.Address),                  "Address",                  NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (FixedIo.AddressLength),            "Address Length",           NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpVendor[3] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpVendor),             "Vendor Specific",          NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Vendor.ByteLength),                "Length",                   NULL},
    {ACPI_RSD_LONGLIST, ACPI_RSD_OFFSET (Vendor.ByteData[0]),               "Vendor Data",              NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpEndTag[1] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpEndTag),             "EndTag",                   NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpMemory24[6] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpMemory24),           "24-Bit Memory Range",      NULL},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Memory24.WriteProtect),            "Write Protect",            AcpiGbl_RwDecode},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Memory24.Minimum),                 "Address Minimum",          NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Memory24.Maximum),                 "Address Maximum",          NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Memory24.Alignment),               "Alignment",                NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Memory24.AddressLength),           "Address Length",           NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpMemory32[6] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpMemory32),           "32-Bit Memory Range",      NULL},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Memory32.WriteProtect),            "Write Protect",            AcpiGbl_RwDecode},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Memory32.Minimum),                 "Address Minimum",          NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Memory32.Maximum),                 "Address Maximum",          NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Memory32.Alignment),               "Alignment",                NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Memory32.AddressLength),           "Address Length",           NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpFixedMemory32[4] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpFixedMemory32),      "32-Bit Fixed Memory Range",NULL},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (FixedMemory32.WriteProtect),       "Write Protect",            AcpiGbl_RwDecode},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (FixedMemory32.Address),            "Address",                  NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (FixedMemory32.AddressLength),      "Address Length",           NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpAddress16[8] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpAddress16),          "16-Bit WORD Address Space",NULL},
    {ACPI_RSD_ADDRESS,  0,                                                  NULL,                       NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Address16.Address.Granularity),    "Granularity",              NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Address16.Address.Minimum),        "Address Minimum",          NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Address16.Address.Maximum),        "Address Maximum",          NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Address16.Address.TranslationOffset),
                                                                            "Translation Offset",       NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Address16.Address.AddressLength),  "Address Length",           NULL},
    {ACPI_RSD_SOURCE,   ACPI_RSD_OFFSET (Address16.ResourceSource),         NULL,                       NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpAddress32[8] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpAddress32),         "32-Bit DWORD Address Space", NULL},
    {ACPI_RSD_ADDRESS,  0,                                                  NULL,                       NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Address32.Address.Granularity),    "Granularity",              NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Address32.Address.Minimum),        "Address Minimum",          NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Address32.Address.Maximum),        "Address Maximum",          NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Address32.Address.TranslationOffset),
                                                                            "Translation Offset",       NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (Address32.Address.AddressLength),  "Address Length",           NULL},
    {ACPI_RSD_SOURCE,   ACPI_RSD_OFFSET (Address32.ResourceSource),         NULL,                       NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpAddress64[8] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpAddress64),          "64-Bit QWORD Address Space", NULL},
    {ACPI_RSD_ADDRESS,  0,                                                  NULL,                       NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (Address64.Address.Granularity),    "Granularity",              NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (Address64.Address.Minimum),        "Address Minimum",          NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (Address64.Address.Maximum),        "Address Maximum",          NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (Address64.Address.TranslationOffset),
                                                                            "Translation Offset",       NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (Address64.Address.AddressLength),  "Address Length",           NULL},
    {ACPI_RSD_SOURCE,   ACPI_RSD_OFFSET (Address64.ResourceSource),         NULL,                       NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpExtAddress64[8] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpExtAddress64),       "64-Bit Extended Address Space", NULL},
    {ACPI_RSD_ADDRESS,  0,                                                  NULL,                       NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (ExtAddress64.Address.Granularity), "Granularity",              NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (ExtAddress64.Address.Minimum),     "Address Minimum",          NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (ExtAddress64.Address.Maximum),     "Address Maximum",          NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (ExtAddress64.Address.TranslationOffset),
                                                                            "Translation Offset",       NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (ExtAddress64.Address.AddressLength),
                                                                            "Address Length",           NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (ExtAddress64.TypeSpecific),        "Type-Specific Attribute",  NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpExtIrq[8] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpExtIrq),             "Extended IRQ",             NULL},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (ExtendedIrq.ProducerConsumer),     "Type",                     AcpiGbl_ConsumeDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (ExtendedIrq.Triggering),           "Triggering",               AcpiGbl_HeDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (ExtendedIrq.Polarity),             "Polarity",                 AcpiGbl_LlDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (ExtendedIrq.Sharable),             "Sharing",                  AcpiGbl_ShrDecode},
    {ACPI_RSD_SOURCE,   ACPI_RSD_OFFSET (ExtendedIrq.ResourceSource),       NULL,                       NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (ExtendedIrq.InterruptCount),       "Interrupt Count",          NULL},
    {ACPI_RSD_DWORDLIST,ACPI_RSD_OFFSET (ExtendedIrq.Interrupts[0]),        "Interrupt List",           NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpGenericReg[6] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpGenericReg),         "Generic Register",         NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (GenericReg.SpaceId),               "Space ID",                 NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (GenericReg.BitWidth),              "Bit Width",                NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (GenericReg.BitOffset),             "Bit Offset",               NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (GenericReg.AccessSize),            "Access Size",              NULL},
    {ACPI_RSD_UINT64,   ACPI_RSD_OFFSET (GenericReg.Address),               "Address",                  NULL}
};

ACPI_RSDUMP_INFO        AcpiRsDumpGpio[16] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpGpio),               "GPIO",                     NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (Gpio.RevisionId),                  "RevisionId",               NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (Gpio.ConnectionType),              "ConnectionType",           AcpiGbl_CtDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Gpio.ProducerConsumer),            "ProducerConsumer",         AcpiGbl_ConsumeDecode},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (Gpio.PinConfig),                   "PinConfig",                AcpiGbl_PpcDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Gpio.Sharable),                    "Sharing",                  AcpiGbl_ShrDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Gpio.IoRestriction),               "IoRestriction",            AcpiGbl_IorDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Gpio.Triggering),                  "Triggering",               AcpiGbl_HeDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Gpio.Polarity),                    "Polarity",                 AcpiGbl_LlDecode},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Gpio.DriveStrength),               "DriveStrength",            NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Gpio.DebounceTimeout),             "DebounceTimeout",          NULL},
    {ACPI_RSD_SOURCE,   ACPI_RSD_OFFSET (Gpio.ResourceSource),              "ResourceSource",           NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Gpio.PinTableLength),              "PinTableLength",           NULL},
    {ACPI_RSD_WORDLIST, ACPI_RSD_OFFSET (Gpio.PinTable),                    "PinTable",                 NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (Gpio.VendorLength),                "VendorLength",             NULL},
    {ACPI_RSD_SHORTLISTX,ACPI_RSD_OFFSET (Gpio.VendorData),                 "VendorData",               NULL},
};

ACPI_RSDUMP_INFO        AcpiRsDumpFixedDma[4] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpFixedDma),           "FixedDma",                 NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (FixedDma.RequestLines),            "RequestLines",             NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (FixedDma.Channels),                "Channels",                 NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (FixedDma.Width),                   "TransferWidth",            AcpiGbl_DtsDecode},
};

#define ACPI_RS_DUMP_COMMON_SERIAL_BUS \
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (CommonSerialBus.RevisionId),       "RevisionId",               NULL}, \
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (CommonSerialBus.Type),             "Type",                     AcpiGbl_SbtDecode}, \
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (CommonSerialBus.ProducerConsumer), "ProducerConsumer",         AcpiGbl_ConsumeDecode}, \
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (CommonSerialBus.SlaveMode),        "SlaveMode",                AcpiGbl_SmDecode}, \
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (CommonSerialBus.ConnectionSharing),"ConnectionSharing",        AcpiGbl_ShrDecode}, \
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (CommonSerialBus.TypeRevisionId),   "TypeRevisionId",           NULL}, \
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (CommonSerialBus.TypeDataLength),   "TypeDataLength",           NULL}, \
    {ACPI_RSD_SOURCE,   ACPI_RSD_OFFSET (CommonSerialBus.ResourceSource),   "ResourceSource",           NULL}, \
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (CommonSerialBus.VendorLength),     "VendorLength",             NULL}, \
    {ACPI_RSD_SHORTLISTX,ACPI_RSD_OFFSET (CommonSerialBus.VendorData),      "VendorData",               NULL},

ACPI_RSDUMP_INFO        AcpiRsDumpCommonSerialBus[11] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpCommonSerialBus),    "Common Serial Bus",        NULL},
    ACPI_RS_DUMP_COMMON_SERIAL_BUS
};

ACPI_RSDUMP_INFO        AcpiRsDumpI2cSerialBus[14] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpI2cSerialBus),       "I2C Serial Bus",           NULL},
    ACPI_RS_DUMP_COMMON_SERIAL_BUS
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (I2cSerialBus.AccessMode),          "AccessMode",               AcpiGbl_AmDecode},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (I2cSerialBus.ConnectionSpeed),     "ConnectionSpeed",          NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (I2cSerialBus.SlaveAddress),        "SlaveAddress",             NULL},
};

ACPI_RSDUMP_INFO        AcpiRsDumpSpiSerialBus[18] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpSpiSerialBus),       "Spi Serial Bus",           NULL},
    ACPI_RS_DUMP_COMMON_SERIAL_BUS
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (SpiSerialBus.WireMode),            "WireMode",                 AcpiGbl_WmDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (SpiSerialBus.DevicePolarity),      "DevicePolarity",           AcpiGbl_DpDecode},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (SpiSerialBus.DataBitLength),       "DataBitLength",            NULL},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (SpiSerialBus.ClockPhase),          "ClockPhase",               AcpiGbl_CphDecode},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (SpiSerialBus.ClockPolarity),       "ClockPolarity",            AcpiGbl_CpoDecode},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (SpiSerialBus.DeviceSelection),     "DeviceSelection",          NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (SpiSerialBus.ConnectionSpeed),     "ConnectionSpeed",          NULL},
};

ACPI_RSDUMP_INFO        AcpiRsDumpUartSerialBus[20] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpUartSerialBus),       "Uart Serial Bus",         NULL},
    ACPI_RS_DUMP_COMMON_SERIAL_BUS
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (UartSerialBus.FlowControl),         "FlowControl",             AcpiGbl_FcDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (UartSerialBus.StopBits),            "StopBits",                AcpiGbl_SbDecode},
    {ACPI_RSD_3BITFLAG, ACPI_RSD_OFFSET (UartSerialBus.DataBits),            "DataBits",                AcpiGbl_BpbDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (UartSerialBus.Endian),              "Endian",                  AcpiGbl_EdDecode},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (UartSerialBus.Parity),              "Parity",                  AcpiGbl_PtDecode},
    {ACPI_RSD_UINT8,    ACPI_RSD_OFFSET (UartSerialBus.LinesEnabled),        "LinesEnabled",            NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (UartSerialBus.RxFifoSize),          "RxFifoSize",              NULL},
    {ACPI_RSD_UINT16,   ACPI_RSD_OFFSET (UartSerialBus.TxFifoSize),          "TxFifoSize",              NULL},
    {ACPI_RSD_UINT32,   ACPI_RSD_OFFSET (UartSerialBus.DefaultBaudRate),     "ConnectionSpeed",         NULL},
};

/*
 * Tables used for common address descriptor flag fields
 */
ACPI_RSDUMP_INFO        AcpiRsDumpGeneralFlags[5] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpGeneralFlags),       NULL,                       NULL},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Address.ProducerConsumer),         "Consumer/Producer",        AcpiGbl_ConsumeDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Address.Decode),                   "Address Decode",           AcpiGbl_DecDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Address.MinAddressFixed),          "Min Relocatability",       AcpiGbl_MinDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Address.MaxAddressFixed),          "Max Relocatability",       AcpiGbl_MaxDecode}
};

ACPI_RSDUMP_INFO        AcpiRsDumpMemoryFlags[5] =
{
    {ACPI_RSD_LITERAL,  ACPI_RSD_TABLE_SIZE (AcpiRsDumpMemoryFlags),        "Resource Type",            (void *) "Memory Range"},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Address.Info.Mem.WriteProtect),    "Write Protect",            AcpiGbl_RwDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Address.Info.Mem.Caching),         "Caching",                  AcpiGbl_MemDecode},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Address.Info.Mem.RangeType),       "Range Type",               AcpiGbl_MtpDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Address.Info.Mem.Translation),     "Translation",              AcpiGbl_TtpDecode}
};

ACPI_RSDUMP_INFO        AcpiRsDumpIoFlags[4] =
{
    {ACPI_RSD_LITERAL,  ACPI_RSD_TABLE_SIZE (AcpiRsDumpIoFlags),            "Resource Type",            (void *) "I/O Range"},
    {ACPI_RSD_2BITFLAG, ACPI_RSD_OFFSET (Address.Info.Io.RangeType),        "Range Type",               AcpiGbl_RngDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Address.Info.Io.Translation),      "Translation",              AcpiGbl_TtpDecode},
    {ACPI_RSD_1BITFLAG, ACPI_RSD_OFFSET (Address.Info.Io.TranslationType),  "Translation Type",         AcpiGbl_TrsDecode}
};


/*
 * Table used to dump _PRT contents
 */
ACPI_RSDUMP_INFO        AcpiRsDumpPrt[5] =
{
    {ACPI_RSD_TITLE,    ACPI_RSD_TABLE_SIZE (AcpiRsDumpPrt),                NULL,                       NULL},
    {ACPI_RSD_UINT64,   ACPI_PRT_OFFSET (Address),                          "Address",                  NULL},
    {ACPI_RSD_UINT32,   ACPI_PRT_OFFSET (Pin),                              "Pin",                      NULL},
    {ACPI_RSD_STRING,   ACPI_PRT_OFFSET (Source[0]),                        "Source",                   NULL},
    {ACPI_RSD_UINT32,   ACPI_PRT_OFFSET (SourceIndex),                      "Source Index",             NULL}
};

#endif
