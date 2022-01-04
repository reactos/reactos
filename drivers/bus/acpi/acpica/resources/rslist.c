/*******************************************************************************
 *
 * Module Name: rslist - Linked list utilities
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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
        ACPI_MODULE_NAME    ("rslist")


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsConvertAmlToResources
 *
 * PARAMETERS:  ACPI_WALK_AML_CALLBACK
 *              ResourcePtr             - Pointer to the buffer that will
 *                                        contain the output structures
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an AML resource to an internal representation of the
 *              resource that is aligned and easier to access.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsConvertAmlToResources (
    UINT8                   *Aml,
    UINT32                  Length,
    UINT32                  Offset,
    UINT8                   ResourceIndex,
    void                    **Context)
{
    ACPI_RESOURCE           **ResourcePtr = ACPI_CAST_INDIRECT_PTR (
                                ACPI_RESOURCE, Context);
    ACPI_RESOURCE           *Resource;
    AML_RESOURCE            *AmlResource;
    ACPI_RSCONVERT_INFO     *ConversionTable;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (RsConvertAmlToResources);


    /*
     * Check that the input buffer and all subsequent pointers into it
     * are aligned on a native word boundary. Most important on IA64
     */
    Resource = *ResourcePtr;
    if (ACPI_IS_MISALIGNED (Resource))
    {
        ACPI_WARNING ((AE_INFO,
            "Misaligned resource pointer %p", Resource));
    }

    /* Get the appropriate conversion info table */

    AmlResource = ACPI_CAST_PTR (AML_RESOURCE, Aml);

    if (AcpiUtGetResourceType (Aml) ==
        ACPI_RESOURCE_NAME_SERIAL_BUS)
    {
        if (AmlResource->CommonSerialBus.Type >
            AML_RESOURCE_MAX_SERIALBUSTYPE)
        {
            ConversionTable = NULL;
        }
        else
        {
            /* This is an I2C, SPI, UART, or CSI2 SerialBus descriptor */

            ConversionTable = AcpiGbl_ConvertResourceSerialBusDispatch [
                AmlResource->CommonSerialBus.Type];
        }
    }
    else
    {
        ConversionTable = AcpiGbl_GetResourceDispatch[ResourceIndex];
    }

    if (!ConversionTable)
    {
        ACPI_ERROR ((AE_INFO,
            "Invalid/unsupported resource descriptor: Type 0x%2.2X",
            ResourceIndex));
        return_ACPI_STATUS (AE_AML_INVALID_RESOURCE_TYPE);
    }

     /* Convert the AML byte stream resource to a local resource struct */

    Status = AcpiRsConvertAmlToResource (
        Resource, AmlResource, ConversionTable);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not convert AML resource (Type 0x%X)", *Aml));
        return_ACPI_STATUS (Status);
    }

    if (!Resource->Length)
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Zero-length resource returned from RsConvertAmlToResource"));
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_RESOURCES,
        "Type %.2X, AmlLength %.2X InternalLength %.2X\n",
        AcpiUtGetResourceType (Aml), Length,
        Resource->Length));

    /* Point to the next structure in the output buffer */

    *ResourcePtr = ACPI_NEXT_RESOURCE (Resource);
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsConvertResourcesToAml
 *
 * PARAMETERS:  Resource            - Pointer to the resource linked list
 *              AmlSizeNeeded       - Calculated size of the byte stream
 *                                    needed from calling AcpiRsGetAmlLength()
 *                                    The size of the OutputBuffer is
 *                                    guaranteed to be >= AmlSizeNeeded
 *              OutputBuffer        - Pointer to the buffer that will
 *                                    contain the byte stream
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Takes the resource linked list and parses it, creating a
 *              byte stream of resources in the caller's output buffer
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsConvertResourcesToAml (
    ACPI_RESOURCE           *Resource,
    ACPI_SIZE               AmlSizeNeeded,
    UINT8                   *OutputBuffer)
{
    UINT8                   *Aml = OutputBuffer;
    UINT8                   *EndAml = OutputBuffer + AmlSizeNeeded;
    ACPI_RSCONVERT_INFO     *ConversionTable;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (RsConvertResourcesToAml);


    /* Walk the resource descriptor list, convert each descriptor */

    while (Aml < EndAml)
    {
        /* Validate the (internal) Resource Type */

        if (Resource->Type > ACPI_RESOURCE_TYPE_MAX)
        {
            ACPI_ERROR ((AE_INFO,
                "Invalid descriptor type (0x%X) in resource list",
                Resource->Type));
            return_ACPI_STATUS (AE_BAD_DATA);
        }

        /* Sanity check the length. It must not be zero, or we loop forever */

        if (!Resource->Length)
        {
            ACPI_ERROR ((AE_INFO,
                "Invalid zero length descriptor in resource list\n"));
            return_ACPI_STATUS (AE_AML_BAD_RESOURCE_LENGTH);
        }

        /* Perform the conversion */

        if (Resource->Type == ACPI_RESOURCE_TYPE_SERIAL_BUS)
        {
            if (Resource->Data.CommonSerialBus.Type >
                AML_RESOURCE_MAX_SERIALBUSTYPE)
            {
                ConversionTable = NULL;
            }
            else
            {
                /* This is an I2C, SPI, UART or CSI2 SerialBus descriptor */

                ConversionTable = AcpiGbl_ConvertResourceSerialBusDispatch[
                    Resource->Data.CommonSerialBus.Type];
            }
        }
        else
        {
            ConversionTable = AcpiGbl_SetResourceDispatch[Resource->Type];
        }

        if (!ConversionTable)
        {
            ACPI_ERROR ((AE_INFO,
                "Invalid/unsupported resource descriptor: Type 0x%2.2X",
                Resource->Type));
            return_ACPI_STATUS (AE_AML_INVALID_RESOURCE_TYPE);
        }

        Status = AcpiRsConvertResourceToAml (Resource,
            ACPI_CAST_PTR (AML_RESOURCE, Aml), ConversionTable);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not convert resource (type 0x%X) to AML",
                Resource->Type));
            return_ACPI_STATUS (Status);
        }

        /* Perform final sanity check on the new AML resource descriptor */

        Status = AcpiUtValidateResource (
            NULL, ACPI_CAST_PTR (AML_RESOURCE, Aml), NULL);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        /* Check for end-of-list, normal exit */

        if (Resource->Type == ACPI_RESOURCE_TYPE_END_TAG)
        {
            /* An End Tag indicates the end of the input Resource Template */

            return_ACPI_STATUS (AE_OK);
        }

        /*
         * Extract the total length of the new descriptor and set the
         * Aml to point to the next (output) resource descriptor
         */
        Aml += AcpiUtGetDescriptorLength (Aml);

        /* Point to the next input resource descriptor */

        Resource = ACPI_NEXT_RESOURCE (Resource);
    }

    /* Completed buffer, but did not find an EndTag resource descriptor */

    return_ACPI_STATUS (AE_AML_NO_RESOURCE_END_TAG);
}
