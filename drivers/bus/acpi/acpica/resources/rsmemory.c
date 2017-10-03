/*******************************************************************************
 *
 * Module Name: rsmem24 - Memory resource descriptors
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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
        ACPI_MODULE_NAME    ("rsmemory")


/*******************************************************************************
 *
 * AcpiRsConvertMemory24
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertMemory24[4] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_MEMORY24,
                        ACPI_RS_SIZE (ACPI_RESOURCE_MEMORY24),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertMemory24)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_MEMORY24,
                        sizeof (AML_RESOURCE_MEMORY24),
                        0},

    /* Read/Write bit */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Memory24.WriteProtect),
                        AML_OFFSET (Memory24.Flags),
                        0},
    /*
     * These fields are contiguous in both the source and destination:
     * Minimum Base Address
     * Maximum Base Address
     * Address Base Alignment
     * Range Length
     */
    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.Memory24.Minimum),
                        AML_OFFSET (Memory24.Minimum),
                        4}
};


/*******************************************************************************
 *
 * AcpiRsConvertMemory32
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertMemory32[4] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_MEMORY32,
                        ACPI_RS_SIZE (ACPI_RESOURCE_MEMORY32),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertMemory32)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_MEMORY32,
                        sizeof (AML_RESOURCE_MEMORY32),
                        0},

    /* Read/Write bit */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Memory32.WriteProtect),
                        AML_OFFSET (Memory32.Flags),
                        0},
    /*
     * These fields are contiguous in both the source and destination:
     * Minimum Base Address
     * Maximum Base Address
     * Address Base Alignment
     * Range Length
     */
    {ACPI_RSC_MOVE32,   ACPI_RS_OFFSET (Data.Memory32.Minimum),
                        AML_OFFSET (Memory32.Minimum),
                        4}
};


/*******************************************************************************
 *
 * AcpiRsConvertFixedMemory32
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertFixedMemory32[4] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_FIXED_MEMORY32,
                        ACPI_RS_SIZE (ACPI_RESOURCE_FIXED_MEMORY32),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertFixedMemory32)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_FIXED_MEMORY32,
                        sizeof (AML_RESOURCE_FIXED_MEMORY32),
                        0},

    /* Read/Write bit */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.FixedMemory32.WriteProtect),
                        AML_OFFSET (FixedMemory32.Flags),
                        0},
    /*
     * These fields are contiguous in both the source and destination:
     * Base Address
     * Range Length
     */
    {ACPI_RSC_MOVE32,   ACPI_RS_OFFSET (Data.FixedMemory32.Address),
                        AML_OFFSET (FixedMemory32.Address),
                        2}
};


/*******************************************************************************
 *
 * AcpiRsGetVendorSmall
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsGetVendorSmall[3] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_VENDOR,
                        ACPI_RS_SIZE (ACPI_RESOURCE_VENDOR),
                        ACPI_RSC_TABLE_SIZE (AcpiRsGetVendorSmall)},

    /* Length of the vendor data (byte count) */

    {ACPI_RSC_COUNT16,  ACPI_RS_OFFSET (Data.Vendor.ByteLength),
                        0,
                        sizeof (UINT8)},

    /* Vendor data */

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Vendor.ByteData[0]),
                        sizeof (AML_RESOURCE_SMALL_HEADER),
                        0}
};


/*******************************************************************************
 *
 * AcpiRsGetVendorLarge
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsGetVendorLarge[3] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_VENDOR,
                        ACPI_RS_SIZE (ACPI_RESOURCE_VENDOR),
                        ACPI_RSC_TABLE_SIZE (AcpiRsGetVendorLarge)},

    /* Length of the vendor data (byte count) */

    {ACPI_RSC_COUNT16,  ACPI_RS_OFFSET (Data.Vendor.ByteLength),
                        0,
                        sizeof (UINT8)},

    /* Vendor data */

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Vendor.ByteData[0]),
                        sizeof (AML_RESOURCE_LARGE_HEADER),
                        0}
};


/*******************************************************************************
 *
 * AcpiRsSetVendor
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsSetVendor[7] =
{
    /* Default is a small vendor descriptor */

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_VENDOR_SMALL,
                        sizeof (AML_RESOURCE_SMALL_HEADER),
                        ACPI_RSC_TABLE_SIZE (AcpiRsSetVendor)},

    /* Get the length and copy the data */

    {ACPI_RSC_COUNT16,  ACPI_RS_OFFSET (Data.Vendor.ByteLength),
                        0,
                        0},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Vendor.ByteData[0]),
                        sizeof (AML_RESOURCE_SMALL_HEADER),
                        0},

    /*
     * All done if the Vendor byte length is 7 or less, meaning that it will
     * fit within a small descriptor
     */
    {ACPI_RSC_EXIT_LE,  0, 0, 7},

    /* Must create a large vendor descriptor */

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_VENDOR_LARGE,
                        sizeof (AML_RESOURCE_LARGE_HEADER),
                        0},

    {ACPI_RSC_COUNT16,  ACPI_RS_OFFSET (Data.Vendor.ByteLength),
                        0,
                        0},

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Vendor.ByteData[0]),
                        sizeof (AML_RESOURCE_LARGE_HEADER),
                        0}
};
