/*******************************************************************************
 *
 * Module Name: rsio - IO and DMA resource descriptors
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
        ACPI_MODULE_NAME    ("rsio")


/*******************************************************************************
 *
 * AcpiRsConvertIo
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertIo[5] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_IO,
                        ACPI_RS_SIZE (ACPI_RESOURCE_IO),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertIo)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_IO,
                        sizeof (AML_RESOURCE_IO),
                        0},

    /* Decode flag */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Io.IoDecode),
                        AML_OFFSET (Io.Flags),
                        0},
    /*
     * These fields are contiguous in both the source and destination:
     * Address Alignment
     * Length
     * Minimum Base Address
     * Maximum Base Address
     */
    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Io.Alignment),
                        AML_OFFSET (Io.Alignment),
                        2},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.Io.Minimum),
                        AML_OFFSET (Io.Minimum),
                        2}
};


/*******************************************************************************
 *
 * AcpiRsConvertFixedIo
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertFixedIo[4] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_FIXED_IO,
                        ACPI_RS_SIZE (ACPI_RESOURCE_FIXED_IO),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertFixedIo)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_FIXED_IO,
                        sizeof (AML_RESOURCE_FIXED_IO),
                        0},
    /*
     * These fields are contiguous in both the source and destination:
     * Base Address
     * Length
     */
    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.FixedIo.AddressLength),
                        AML_OFFSET (FixedIo.AddressLength),
                        1},

    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.FixedIo.Address),
                        AML_OFFSET (FixedIo.Address),
                        1}
};


/*******************************************************************************
 *
 * AcpiRsConvertGenericReg
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertGenericReg[4] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_GENERIC_REGISTER,
                        ACPI_RS_SIZE (ACPI_RESOURCE_GENERIC_REGISTER),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertGenericReg)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_GENERIC_REGISTER,
                        sizeof (AML_RESOURCE_GENERIC_REGISTER),
                        0},
    /*
     * These fields are contiguous in both the source and destination:
     * Address Space ID
     * Register Bit Width
     * Register Bit Offset
     * Access Size
     */
    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.GenericReg.SpaceId),
                        AML_OFFSET (GenericReg.AddressSpaceId),
                        4},

    /* Get the Register Address */

    {ACPI_RSC_MOVE64,   ACPI_RS_OFFSET (Data.GenericReg.Address),
                        AML_OFFSET (GenericReg.Address),
                        1}
};


/*******************************************************************************
 *
 * AcpiRsConvertEndDpf
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO   AcpiRsConvertEndDpf[2] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_END_DEPENDENT,
                        ACPI_RS_SIZE_MIN,
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertEndDpf)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_END_DEPENDENT,
                        sizeof (AML_RESOURCE_END_DEPENDENT),
                        0}
};


/*******************************************************************************
 *
 * AcpiRsConvertEndTag
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO   AcpiRsConvertEndTag[2] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_END_TAG,
                        ACPI_RS_SIZE_MIN,
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertEndTag)},

    /*
     * Note: The checksum field is set to zero, meaning that the resource
     * data is treated as if the checksum operation succeeded.
     * (ACPI Spec 1.0b Section 6.4.2.8)
     */
    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_END_TAG,
                        sizeof (AML_RESOURCE_END_TAG),
                        0}
};


/*******************************************************************************
 *
 * AcpiRsGetStartDpf
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO   AcpiRsGetStartDpf[6] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_START_DEPENDENT,
                        ACPI_RS_SIZE (ACPI_RESOURCE_START_DEPENDENT),
                        ACPI_RSC_TABLE_SIZE (AcpiRsGetStartDpf)},

    /* Defaults for Compatibility and Performance priorities */

    {ACPI_RSC_SET8,     ACPI_RS_OFFSET (Data.StartDpf.CompatibilityPriority),
                        ACPI_ACCEPTABLE_CONFIGURATION,
                        2},

    /* Get the descriptor length (0 or 1 for Start Dpf descriptor) */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.StartDpf.DescriptorLength),
                        AML_OFFSET (StartDpf.DescriptorType),
                        0},

    /* All done if there is no flag byte present in the descriptor */

    {ACPI_RSC_EXIT_NE,  ACPI_RSC_COMPARE_AML_LENGTH, 0, 1},

    /* Flag byte is present, get the flags */

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.StartDpf.CompatibilityPriority),
                        AML_OFFSET (StartDpf.Flags),
                        0},

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.StartDpf.PerformanceRobustness),
                        AML_OFFSET (StartDpf.Flags),
                        2}
};


/*******************************************************************************
 *
 * AcpiRsSetStartDpf
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO   AcpiRsSetStartDpf[10] =
{
    /* Start with a default descriptor of length 1 */

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_START_DEPENDENT,
                        sizeof (AML_RESOURCE_START_DEPENDENT),
                        ACPI_RSC_TABLE_SIZE (AcpiRsSetStartDpf)},

    /* Set the default flag values */

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.StartDpf.CompatibilityPriority),
                        AML_OFFSET (StartDpf.Flags),
                        0},

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.StartDpf.PerformanceRobustness),
                        AML_OFFSET (StartDpf.Flags),
                        2},
    /*
     * All done if the output descriptor length is required to be 1
     * (i.e., optimization to 0 bytes cannot be attempted)
     */
    {ACPI_RSC_EXIT_EQ,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET(Data.StartDpf.DescriptorLength),
                        1},

    /* Set length to 0 bytes (no flags byte) */

    {ACPI_RSC_LENGTH,   0, 0, sizeof (AML_RESOURCE_START_DEPENDENT_NOPRIO)},

    /*
     * All done if the output descriptor length is required to be 0.
     *
     * TBD: Perhaps we should check for error if input flags are not
     * compatible with a 0-byte descriptor.
     */
    {ACPI_RSC_EXIT_EQ,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET(Data.StartDpf.DescriptorLength),
                        0},

    /* Reset length to 1 byte (descriptor with flags byte) */

    {ACPI_RSC_LENGTH,   0, 0, sizeof (AML_RESOURCE_START_DEPENDENT)},


    /*
     * All done if flags byte is necessary -- if either priority value
     * is not ACPI_ACCEPTABLE_CONFIGURATION
     */
    {ACPI_RSC_EXIT_NE,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET (Data.StartDpf.CompatibilityPriority),
                        ACPI_ACCEPTABLE_CONFIGURATION},

    {ACPI_RSC_EXIT_NE,  ACPI_RSC_COMPARE_VALUE,
                        ACPI_RS_OFFSET (Data.StartDpf.PerformanceRobustness),
                        ACPI_ACCEPTABLE_CONFIGURATION},

    /* Flag byte is not necessary */

    {ACPI_RSC_LENGTH,   0, 0, sizeof (AML_RESOURCE_START_DEPENDENT_NOPRIO)}
};
