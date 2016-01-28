/*******************************************************************************
 *
 * Module Name: rsaddr - Address resource descriptors (16/32/64)
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
        ACPI_MODULE_NAME    ("rsaddr")


/*******************************************************************************
 *
 * AcpiRsConvertAddress16 - All WORD (16-bit) address resources
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertAddress16[5] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_ADDRESS16,
                        ACPI_RS_SIZE (ACPI_RESOURCE_ADDRESS16),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertAddress16)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_ADDRESS16,
                        sizeof (AML_RESOURCE_ADDRESS16),
                        0},

    /* Resource Type, General Flags, and Type-Specific Flags */

    {ACPI_RSC_ADDRESS,  0, 0, 0},

    /*
     * These fields are contiguous in both the source and destination:
     * Address Granularity
     * Address Range Minimum
     * Address Range Maximum
     * Address Translation Offset
     * Address Length
     */
    {ACPI_RSC_MOVE16,   ACPI_RS_OFFSET (Data.Address16.Address.Granularity),
                        AML_OFFSET (Address16.Granularity),
                        5},

    /* Optional ResourceSource (Index and String) */

    {ACPI_RSC_SOURCE,   ACPI_RS_OFFSET (Data.Address16.ResourceSource),
                        0,
                        sizeof (AML_RESOURCE_ADDRESS16)}
};


/*******************************************************************************
 *
 * AcpiRsConvertAddress32 - All DWORD (32-bit) address resources
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertAddress32[5] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_ADDRESS32,
                        ACPI_RS_SIZE (ACPI_RESOURCE_ADDRESS32),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertAddress32)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_ADDRESS32,
                        sizeof (AML_RESOURCE_ADDRESS32),
                        0},

    /* Resource Type, General Flags, and Type-Specific Flags */

    {ACPI_RSC_ADDRESS,  0, 0, 0},

    /*
     * These fields are contiguous in both the source and destination:
     * Address Granularity
     * Address Range Minimum
     * Address Range Maximum
     * Address Translation Offset
     * Address Length
     */
    {ACPI_RSC_MOVE32,   ACPI_RS_OFFSET (Data.Address32.Address.Granularity),
                        AML_OFFSET (Address32.Granularity),
                        5},

    /* Optional ResourceSource (Index and String) */

    {ACPI_RSC_SOURCE,   ACPI_RS_OFFSET (Data.Address32.ResourceSource),
                        0,
                        sizeof (AML_RESOURCE_ADDRESS32)}
};


/*******************************************************************************
 *
 * AcpiRsConvertAddress64 - All QWORD (64-bit) address resources
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertAddress64[5] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_ADDRESS64,
                        ACPI_RS_SIZE (ACPI_RESOURCE_ADDRESS64),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertAddress64)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_ADDRESS64,
                        sizeof (AML_RESOURCE_ADDRESS64),
                        0},

    /* Resource Type, General Flags, and Type-Specific Flags */

    {ACPI_RSC_ADDRESS,  0, 0, 0},

    /*
     * These fields are contiguous in both the source and destination:
     * Address Granularity
     * Address Range Minimum
     * Address Range Maximum
     * Address Translation Offset
     * Address Length
     */
    {ACPI_RSC_MOVE64,   ACPI_RS_OFFSET (Data.Address64.Address.Granularity),
                        AML_OFFSET (Address64.Granularity),
                        5},

    /* Optional ResourceSource (Index and String) */

    {ACPI_RSC_SOURCE,   ACPI_RS_OFFSET (Data.Address64.ResourceSource),
                        0,
                        sizeof (AML_RESOURCE_ADDRESS64)}
};


/*******************************************************************************
 *
 * AcpiRsConvertExtAddress64 - All Extended (64-bit) address resources
 *
 ******************************************************************************/

ACPI_RSCONVERT_INFO     AcpiRsConvertExtAddress64[5] =
{
    {ACPI_RSC_INITGET,  ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64,
                        ACPI_RS_SIZE (ACPI_RESOURCE_EXTENDED_ADDRESS64),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertExtAddress64)},

    {ACPI_RSC_INITSET,  ACPI_RESOURCE_NAME_EXTENDED_ADDRESS64,
                        sizeof (AML_RESOURCE_EXTENDED_ADDRESS64),
                        0},

    /* Resource Type, General Flags, and Type-Specific Flags */

    {ACPI_RSC_ADDRESS,  0, 0, 0},

    /* Revision ID */

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.ExtAddress64.RevisionID),
                        AML_OFFSET (ExtAddress64.RevisionID),
                        1},
    /*
     * These fields are contiguous in both the source and destination:
     * Address Granularity
     * Address Range Minimum
     * Address Range Maximum
     * Address Translation Offset
     * Address Length
     * Type-Specific Attribute
     */
    {ACPI_RSC_MOVE64,   ACPI_RS_OFFSET (Data.ExtAddress64.Address.Granularity),
                        AML_OFFSET (ExtAddress64.Granularity),
                        6}
};


/*******************************************************************************
 *
 * AcpiRsConvertGeneralFlags - Flags common to all address descriptors
 *
 ******************************************************************************/

static ACPI_RSCONVERT_INFO  AcpiRsConvertGeneralFlags[6] =
{
    {ACPI_RSC_FLAGINIT, 0, AML_OFFSET (Address.Flags),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertGeneralFlags)},

    /* Resource Type (Memory, Io, BusNumber, etc.) */

    {ACPI_RSC_MOVE8,    ACPI_RS_OFFSET (Data.Address.ResourceType),
                        AML_OFFSET (Address.ResourceType),
                        1},

    /* General Flags - Consume, Decode, MinFixed, MaxFixed */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Address.ProducerConsumer),
                        AML_OFFSET (Address.Flags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Address.Decode),
                        AML_OFFSET (Address.Flags),
                        1},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Address.MinAddressFixed),
                        AML_OFFSET (Address.Flags),
                        2},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Address.MaxAddressFixed),
                        AML_OFFSET (Address.Flags),
                        3}
};


/*******************************************************************************
 *
 * AcpiRsConvertMemFlags - Flags common to Memory address descriptors
 *
 ******************************************************************************/

static ACPI_RSCONVERT_INFO  AcpiRsConvertMemFlags[5] =
{
    {ACPI_RSC_FLAGINIT, 0, AML_OFFSET (Address.SpecificFlags),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertMemFlags)},

    /* Memory-specific flags */

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Address.Info.Mem.WriteProtect),
                        AML_OFFSET (Address.SpecificFlags),
                        0},

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.Address.Info.Mem.Caching),
                        AML_OFFSET (Address.SpecificFlags),
                        1},

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.Address.Info.Mem.RangeType),
                        AML_OFFSET (Address.SpecificFlags),
                        3},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Address.Info.Mem.Translation),
                        AML_OFFSET (Address.SpecificFlags),
                        5}
};


/*******************************************************************************
 *
 * AcpiRsConvertIoFlags - Flags common to I/O address descriptors
 *
 ******************************************************************************/

static ACPI_RSCONVERT_INFO  AcpiRsConvertIoFlags[4] =
{
    {ACPI_RSC_FLAGINIT, 0, AML_OFFSET (Address.SpecificFlags),
                        ACPI_RSC_TABLE_SIZE (AcpiRsConvertIoFlags)},

    /* I/O-specific flags */

    {ACPI_RSC_2BITFLAG, ACPI_RS_OFFSET (Data.Address.Info.Io.RangeType),
                        AML_OFFSET (Address.SpecificFlags),
                        0},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Address.Info.Io.Translation),
                        AML_OFFSET (Address.SpecificFlags),
                        4},

    {ACPI_RSC_1BITFLAG, ACPI_RS_OFFSET (Data.Address.Info.Io.TranslationType),
                        AML_OFFSET (Address.SpecificFlags),
                        5}
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetAddressCommon
 *
 * PARAMETERS:  Resource            - Pointer to the internal resource struct
 *              Aml                 - Pointer to the AML resource descriptor
 *
 * RETURN:      TRUE if the ResourceType field is OK, FALSE otherwise
 *
 * DESCRIPTION: Convert common flag fields from a raw AML resource descriptor
 *              to an internal resource descriptor
 *
 ******************************************************************************/

BOOLEAN
AcpiRsGetAddressCommon (
    ACPI_RESOURCE           *Resource,
    AML_RESOURCE            *Aml)
{
    ACPI_FUNCTION_ENTRY ();


    /* Validate the Resource Type */

    if ((Aml->Address.ResourceType > 2) &&
        (Aml->Address.ResourceType < 0xC0))
    {
        return (FALSE);
    }

    /* Get the Resource Type and General Flags */

    (void) AcpiRsConvertAmlToResource (
        Resource, Aml, AcpiRsConvertGeneralFlags);

    /* Get the Type-Specific Flags (Memory and I/O descriptors only) */

    if (Resource->Data.Address.ResourceType == ACPI_MEMORY_RANGE)
    {
        (void) AcpiRsConvertAmlToResource (
            Resource, Aml, AcpiRsConvertMemFlags);
    }
    else if (Resource->Data.Address.ResourceType == ACPI_IO_RANGE)
    {
        (void) AcpiRsConvertAmlToResource (
            Resource, Aml, AcpiRsConvertIoFlags);
    }
    else
    {
        /* Generic resource type, just grab the TypeSpecific byte */

        Resource->Data.Address.Info.TypeSpecific =
            Aml->Address.SpecificFlags;
    }

    return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsSetAddressCommon
 *
 * PARAMETERS:  Aml                 - Pointer to the AML resource descriptor
 *              Resource            - Pointer to the internal resource struct
 *
 * RETURN:      None
 *
 * DESCRIPTION: Convert common flag fields from a resource descriptor to an
 *              AML descriptor
 *
 ******************************************************************************/

void
AcpiRsSetAddressCommon (
    AML_RESOURCE            *Aml,
    ACPI_RESOURCE           *Resource)
{
    ACPI_FUNCTION_ENTRY ();


    /* Set the Resource Type and General Flags */

    (void) AcpiRsConvertResourceToAml (
        Resource, Aml, AcpiRsConvertGeneralFlags);

    /* Set the Type-Specific Flags (Memory and I/O descriptors only) */

    if (Resource->Data.Address.ResourceType == ACPI_MEMORY_RANGE)
    {
        (void) AcpiRsConvertResourceToAml (
            Resource, Aml, AcpiRsConvertMemFlags);
    }
    else if (Resource->Data.Address.ResourceType == ACPI_IO_RANGE)
    {
        (void) AcpiRsConvertResourceToAml (
            Resource, Aml, AcpiRsConvertIoFlags);
    }
    else
    {
        /* Generic resource type, just copy the TypeSpecific byte */

        Aml->Address.SpecificFlags =
            Resource->Data.Address.Info.TypeSpecific;
    }
}
