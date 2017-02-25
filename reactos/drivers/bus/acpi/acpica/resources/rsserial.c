/*******************************************************************************
 *
 * Module Name: rsserial - GPIO/SerialBus resource descriptors
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
        ACPI_MODULE_NAME    ("rsserial")


/*******************************************************************************
 *
 * AcpiRsConvertGpio
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertGpio[18] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_GPIO,
                        ACPI_RS_SIZE (ACPI_RESOURCE_GPIO),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertGpio)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_GPIO,
                        sizeof (AML_RESOURCE_GPIO),
                        0},

    /*
     * These fields are contiguous in both the source and destination:
     * RevisionId
     * ConnectionType
     */
    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Gpio.RevisionId),
                        AML_OFFSET (Gpio.RevisionId),
                        2},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Gpio.ProducerConsumer),
                        AML_OFFSET (Gpio.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Gpio.Sharable),
                        AML_OFFSET (Gpio.IntFlags),
                        3},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Gpio.WakeCapable),
                        AML_OFFSET (Gpio.IntFlags),
                        4},

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.Gpio.IoRestriction),
                        AML_OFFSET (Gpio.IntFlags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Gpio.Triggering),
                        AML_OFFSET (Gpio.IntFlags),
                        0},

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.Gpio.Polarity),
                        AML_OFFSET (Gpio.IntFlags),
                        1},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Gpio.PinConfig),
                        AML_OFFSET (Gpio.PinConfig),
                        1},

    /*
     * These fields are contiguous in both the source and destination:
     * DriveStrength
     * DebounceTimeout
     */
    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.Gpio.DriveStrength),
                        AML_OFFSET (Gpio.DriveStrength),
                        2},

    /* Pin Table */

    {ACPI_RSC_COUNT_GPIO_PIN, ACPI_RS_OFFSET (Data.Gpio.PinTableLength),
                        AML_OFFSET (Gpio.PinTableOffset),
                        AML_OFFSET (Gpio.ResSourceOffset)},

    {ACPI_RSC_MOVE_GPIO_PIN, ACPI_RS_OFFSET (Data.Gpio.PinTable),
                        AML_OFFSET (Gpio.PinTableOffset),
                        0},

    /* Resource Source */

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Gpio.ResourceSource.Index),
                        AML_OFFSET (Gpio.ResSourceIndex),
                        1},

    {ACPI_RSC_COUNT_GPIO_RES,  ACPI_RS_OFFSET (Data.Gpio.ResourceSource.StringLength),
                        AML_OFFSET (Gpio.ResSourceOffset),
                        AML_OFFSET (Gpio.VendorOffset)},

    {ACPI_RSC_MOVE_GPIO_RES,   ACPI_RS_OFFSET (Data.Gpio.ResourceSource.StringPtr),
                        AML_OFFSET (Gpio.ResSourceOffset),
                        0},

    /* Vendor Data */

    {ACPI_RSC_COUNT_GPIO_VEN,   ACPI_RS_OFFSET (Data.Gpio.VendorLength),
                        AML_OFFSET (Gpio.VendorLength),
                        1},

    {ACPI_RSC_MOVE_GPIO_RES,   ACPI_RS_OFFSET (Data.Gpio.VendorData),
                        AML_OFFSET (Gpio.VendorOffset),
                        0},
};


/*******************************************************************************
 *
 * AcpiRsConvertI2cSerialBus
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertI2cSerialBus[17] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_SERIAL_BUS,
                        ACPI_RS_SIZE (ACPI_RESOURCE_I2C_SERIALBUS),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertI2cSerialBus)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_SERIAL_BUS,
                        sizeof (AML_RESOURCE_I2C_SERIALBUS),
                        0},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.RevisionId),
                        AML_OFFSET (CommonSerialBus.RevisionId),
                        1},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.Type),
                        AML_OFFSET (CommonSerialBus.Type),
                        1},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.SlaveMode),
                        AML_OFFSET (CommonSerialBus.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.ProducerConsumer),
                        AML_OFFSET (CommonSerialBus.Flags),
                        1},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.ConnectionSharing),
                        AML_OFFSET (CommonSerialBus.Flags),
                        2},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.TypeRevisionId),
                        AML_OFFSET (CommonSerialBus.TypeRevisionId),
                        1},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.CommonSerialBus.TypeDataLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        1},

    /* Vendor data */

    {ACPI_RSC_COUNT_SERIAL_VEN, ACPI_RS_OFFSET (Data.CommonSerialBus.VendorLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        AML_RESOURCE_I2C_MIN_DATA_LEN},

    {ACPI_RSC_MOVE_SERIAL_VEN,  ACPI_RS_OFFSET (Data.CommonSerialBus.VendorData),
                        0,
                        sizeof (AML_RESOURCE_I2C_SERIALBUS)},

    /* Resource Source */

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.Index),
                        AML_OFFSET (CommonSerialBus.ResSourceIndex),
                        1},

    {ACPI_RSC_COUNT_SERIAL_RES, ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.StringLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        sizeof (AML_RESOURCE_COMMON_SERIALBUS)},

    {ACPI_RSC_MOVE_SERIAL_RES,  ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.StringPtr),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        sizeof (AML_RESOURCE_COMMON_SERIALBUS)},

    /* I2C bus type specific */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.I2cSerialBus.AccessMode),
                        AML_OFFSET (I2cSerialBus.TypeSpecificFlags),
                        0},

    {ACPI_RSC_MOVE32,   ACPI_RS_OFFSET (Data.I2cSerialBus.ConnectionSpeed),
                        AML_OFFSET (I2cSerialBus.ConnectionSpeed),
                        1},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.I2cSerialBus.SlaveAddress),
                        AML_OFFSET (I2cSerialBus.SlaveAddress),
                        1},
};


/*******************************************************************************
 *
 * AcpiRsConvertSpiSerialBus
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertSpiSerialBus[21] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_SERIAL_BUS,
                        ACPI_RS_SIZE (ACPI_RESOURCE_SPI_SERIALBUS),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertSpiSerialBus)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_SERIAL_BUS,
                        sizeof (AML_RESOURCE_SPI_SERIALBUS),
                        0},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.RevisionId),
                        AML_OFFSET (CommonSerialBus.RevisionId),
                        1},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.Type),
                        AML_OFFSET (CommonSerialBus.Type),
                        1},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.SlaveMode),
                        AML_OFFSET (CommonSerialBus.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.ProducerConsumer),
                        AML_OFFSET (CommonSerialBus.Flags),
                        1},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.ConnectionSharing),
                        AML_OFFSET (CommonSerialBus.Flags),
                        2},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.TypeRevisionId),
                        AML_OFFSET (CommonSerialBus.TypeRevisionId),
                        1},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.CommonSerialBus.TypeDataLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        1},

    /* Vendor data */

    {ACPI_RSC_COUNT_SERIAL_VEN, ACPI_RS_OFFSET (Data.CommonSerialBus.VendorLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        AML_RESOURCE_SPI_MIN_DATA_LEN},

    {ACPI_RSC_MOVE_SERIAL_VEN,  ACPI_RS_OFFSET (Data.CommonSerialBus.VendorData),
                        0,
                        sizeof (AML_RESOURCE_SPI_SERIALBUS)},

    /* Resource Source */

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.Index),
                        AML_OFFSET (CommonSerialBus.ResSourceIndex),
                        1},

    {ACPI_RSC_COUNT_SERIAL_RES, ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.StringLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        sizeof (AML_RESOURCE_COMMON_SERIALBUS)},

    {ACPI_RSC_MOVE_SERIAL_RES,  ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.StringPtr),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        sizeof (AML_RESOURCE_COMMON_SERIALBUS)},

    /* Spi bus type specific  */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.SpiSerialBus.WireMode),
                        AML_OFFSET (SpiSerialBus.TypeSpecificFlags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.SpiSerialBus.DevicePolarity),
                        AML_OFFSET (SpiSerialBus.TypeSpecificFlags),
                        1},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.SpiSerialBus.DataBitLength),
                        AML_OFFSET (SpiSerialBus.DataBitLength),
                        1},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.SpiSerialBus.ClockPhase),
                        AML_OFFSET (SpiSerialBus.ClockPhase),
                        1},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.SpiSerialBus.ClockPolarity),
                        AML_OFFSET (SpiSerialBus.ClockPolarity),
                        1},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.SpiSerialBus.DeviceSelection),
                        AML_OFFSET (SpiSerialBus.DeviceSelection),
                        1},

    {ACPI_RSC_MOVE32,   ACPI_RS_OFFSET (Data.SpiSerialBus.ConnectionSpeed),
                        AML_OFFSET (SpiSerialBus.ConnectionSpeed),
                        1},
};


/*******************************************************************************
 *
 * AcpiRsConvertUartSerialBus
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertUartSerialBus[23] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_SERIAL_BUS,
                        ACPI_RS_SIZE (ACPI_RESOURCE_UART_SERIALBUS),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertUartSerialBus)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_SERIAL_BUS,
                        sizeof (AML_RESOURCE_UART_SERIALBUS),
                        0},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.RevisionId),
                        AML_OFFSET (CommonSerialBus.RevisionId),
                        1},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.Type),
                        AML_OFFSET (CommonSerialBus.Type),
                        1},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.SlaveMode),
                        AML_OFFSET (CommonSerialBus.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.ProducerConsumer),
                        AML_OFFSET (CommonSerialBus.Flags),
                        1},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.CommonSerialBus.ConnectionSharing),
                        AML_OFFSET (CommonSerialBus.Flags),
                        2},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.TypeRevisionId),
                        AML_OFFSET (CommonSerialBus.TypeRevisionId),
                        1},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.CommonSerialBus.TypeDataLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        1},

    /* Vendor data */

    {ACPI_RSC_COUNT_SERIAL_VEN, ACPI_RS_OFFSET (Data.CommonSerialBus.VendorLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        AML_RESOURCE_UART_MIN_DATA_LEN},

    {ACPI_RSC_MOVE_SERIAL_VEN,  ACPI_RS_OFFSET (Data.CommonSerialBus.VendorData),
                        0,
                        sizeof (AML_RESOURCE_UART_SERIALBUS)},

    /* Resource Source */

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.Index),
                        AML_OFFSET (CommonSerialBus.ResSourceIndex),
                        1},

    {ACPI_RSC_COUNT_SERIAL_RES, ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.StringLength),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        sizeof (AML_RESOURCE_COMMON_SERIALBUS)},

    {ACPI_RSC_MOVE_SERIAL_RES,  ACPI_RS_OFFSET (Data.CommonSerialBus.ResourceSource.StringPtr),
                        AML_OFFSET (CommonSerialBus.TypeDataLength),
                        sizeof (AML_RESOURCE_COMMON_SERIALBUS)},

    /* Uart bus type specific  */

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.UartSerialBus.FlowControl),
                        AML_OFFSET (UartSerialBus.TypeSpecificFlags),
                        0},

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.UartSerialBus.StopBits),
                        AML_OFFSET (UartSerialBus.TypeSpecificFlags),
                        2},

    {ACPI_RSC_3BITFLAG, ACPI_RS_OFFSET (Data.UartSerialBus.DataBits),
                        AML_OFFSET (UartSerialBus.TypeSpecificFlags),
                        4},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.UartSerialBus.Endian),
                        AML_OFFSET (UartSerialBus.TypeSpecificFlags),
                        7},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.UartSerialBus.Parity),
                        AML_OFFSET (UartSerialBus.Parity),
                        1},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.UartSerialBus.LinesEnabled),
                        AML_OFFSET (UartSerialBus.LinesEnabled),
                        1},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.UartSerialBus.RxFifoSize),
                        AML_OFFSET (UartSerialBus.RxFifoSize),
                        1},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.UartSerialBus.TxFifoSize),
                        AML_OFFSET (UartSerialBus.TxFifoSize),
                        1},

    {ACPI_RSC_MOVE32,   ACPI_RS_OFFSET (Data.UartSerialBus.DefaultBaudRate),
                        AML_OFFSET (UartSerialBus.DefaultBaudRate),
                        1},
};
