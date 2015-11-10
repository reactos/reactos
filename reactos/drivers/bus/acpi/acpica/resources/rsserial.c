/*******************************************************************************
 *
 * Module Name: rsserial - GPIO/SerialBus resource descriptors
 *
 ******************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2015, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

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

ACPI_RSCONVERT_INFO     AcpiRsConvertI2cSerialBus[16] =
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

ACPI_RSCONVERT_INFO     AcpiRsConvertSpiSerialBus[20] =
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

ACPI_RSCONVERT_INFO     AcpiRsConvertUartSerialBus[22] =
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
