/*******************************************************************************
 *
 * Module Name: utresrc - Resource management utilities
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
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


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utresrc")


/*
 * Base sizes of the raw AML resource descriptors, indexed by resource type.
 * Zero indicates a reserved (and therefore invalid) resource type.
 */
const UINT8                 AcpiGbl_ResourceAmlSizes[] =
{
    /* Small descriptors */

    0,
    0,
    0,
    0,
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_IRQ),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_DMA),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_START_DEPENDENT),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_END_DEPENDENT),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_IO),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_FIXED_IO),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_FIXED_DMA),
    0,
    0,
    0,
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_VENDOR_SMALL),
    ACPI_AML_SIZE_SMALL (AML_RESOURCE_END_TAG),

    /* Large descriptors */

    0,
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_MEMORY24),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_GENERIC_REGISTER),
    0,
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_VENDOR_LARGE),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_MEMORY32),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_FIXED_MEMORY32),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_ADDRESS32),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_ADDRESS16),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_EXTENDED_IRQ),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_ADDRESS64),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_EXTENDED_ADDRESS64),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_GPIO),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_FUNCTION),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_COMMON_SERIALBUS),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_CONFIG),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_GROUP),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_GROUP_FUNCTION),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_PIN_GROUP_CONFIG),
};

const UINT8                 AcpiGbl_ResourceAmlSerialBusSizes[] =
{
    0,
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_I2C_SERIALBUS),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_SPI_SERIALBUS),
    ACPI_AML_SIZE_LARGE (AML_RESOURCE_UART_SERIALBUS),
};


/*
 * Resource types, used to validate the resource length field.
 * The length of fixed-length types must match exactly, variable
 * lengths must meet the minimum required length, etc.
 * Zero indicates a reserved (and therefore invalid) resource type.
 */
static const UINT8          AcpiGbl_ResourceTypes[] =
{
    /* Small descriptors */

    0,
    0,
    0,
    0,
    ACPI_SMALL_VARIABLE_LENGTH,     /* 04 IRQ */
    ACPI_FIXED_LENGTH,              /* 05 DMA */
    ACPI_SMALL_VARIABLE_LENGTH,     /* 06 StartDependentFunctions */
    ACPI_FIXED_LENGTH,              /* 07 EndDependentFunctions */
    ACPI_FIXED_LENGTH,              /* 08 IO */
    ACPI_FIXED_LENGTH,              /* 09 FixedIO */
    ACPI_FIXED_LENGTH,              /* 0A FixedDMA */
    0,
    0,
    0,
    ACPI_VARIABLE_LENGTH,           /* 0E VendorShort */
    ACPI_FIXED_LENGTH,              /* 0F EndTag */

    /* Large descriptors */

    0,
    ACPI_FIXED_LENGTH,              /* 01 Memory24 */
    ACPI_FIXED_LENGTH,              /* 02 GenericRegister */
    0,
    ACPI_VARIABLE_LENGTH,           /* 04 VendorLong */
    ACPI_FIXED_LENGTH,              /* 05 Memory32 */
    ACPI_FIXED_LENGTH,              /* 06 Memory32Fixed */
    ACPI_VARIABLE_LENGTH,           /* 07 Dword* address */
    ACPI_VARIABLE_LENGTH,           /* 08 Word* address */
    ACPI_VARIABLE_LENGTH,           /* 09 ExtendedIRQ */
    ACPI_VARIABLE_LENGTH,           /* 0A Qword* address */
    ACPI_FIXED_LENGTH,              /* 0B Extended* address */
    ACPI_VARIABLE_LENGTH,           /* 0C Gpio* */
    ACPI_VARIABLE_LENGTH,           /* 0D PinFunction */
    ACPI_VARIABLE_LENGTH,           /* 0E *SerialBus */
    ACPI_VARIABLE_LENGTH,           /* 0F PinConfig */
    ACPI_VARIABLE_LENGTH,           /* 10 PinGroup */
    ACPI_VARIABLE_LENGTH,           /* 11 PinGroupFunction */
    ACPI_VARIABLE_LENGTH,           /* 12 PinGroupConfig */
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtWalkAmlResources
 *
 * PARAMETERS:  WalkState           - Current walk info
 * PARAMETERS:  Aml                 - Pointer to the raw AML resource template
 *              AmlLength           - Length of the entire template
 *              UserFunction        - Called once for each descriptor found. If
 *                                    NULL, a pointer to the EndTag is returned
 *              Context             - Passed to UserFunction
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk a raw AML resource list(buffer). User function called
 *              once for each resource found.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtWalkAmlResources (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *Aml,
    ACPI_SIZE               AmlLength,
    ACPI_WALK_AML_CALLBACK  UserFunction,
    void                    **Context)
{
    ACPI_STATUS             Status;
    UINT8                   *EndAml;
    UINT8                   ResourceIndex;
    UINT32                  Length;
    UINT32                  Offset = 0;
    UINT8                   EndTag[2] = {0x79, 0x00};


    ACPI_FUNCTION_TRACE (UtWalkAmlResources);


    /* The absolute minimum resource template is one EndTag descriptor */

    if (AmlLength < sizeof (AML_RESOURCE_END_TAG))
    {
        return_ACPI_STATUS (AE_AML_NO_RESOURCE_END_TAG);
    }

    /* Point to the end of the resource template buffer */

    EndAml = Aml + AmlLength;

    /* Walk the byte list, abort on any invalid descriptor type or length */

    while (Aml < EndAml)
    {
        /* Validate the Resource Type and Resource Length */

        Status = AcpiUtValidateResource (WalkState, Aml, &ResourceIndex);
        if (ACPI_FAILURE (Status))
        {
            /*
             * Exit on failure. Cannot continue because the descriptor
             * length may be bogus also.
             */
            return_ACPI_STATUS (Status);
        }

        /* Get the length of this descriptor */

        Length = AcpiUtGetDescriptorLength (Aml);

        /* Invoke the user function */

        if (UserFunction)
        {
            Status = UserFunction (
                Aml, Length, Offset, ResourceIndex, Context);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }
        }

        /* An EndTag descriptor terminates this resource template */

        if (AcpiUtGetResourceType (Aml) == ACPI_RESOURCE_NAME_END_TAG)
        {
            /*
             * There must be at least one more byte in the buffer for
             * the 2nd byte of the EndTag
             */
            if ((Aml + 1) >= EndAml)
            {
                return_ACPI_STATUS (AE_AML_NO_RESOURCE_END_TAG);
            }

            /*
             * Don't attempt to perform any validation on the 2nd byte.
             * Although all known ASL compilers insert a zero for the 2nd
             * byte, it can also be a checksum (as per the ACPI spec),
             * and this is occasionally seen in the field. July 2017.
             */

            /* Return the pointer to the EndTag if requested */

            if (!UserFunction)
            {
                *Context = Aml;
            }

            /* Normal exit */

            return_ACPI_STATUS (AE_OK);
        }

        Aml += Length;
        Offset += Length;
    }

    /* Did not find an EndTag descriptor */

    if (UserFunction)
    {
        /* Insert an EndTag anyway. AcpiRsGetListLength always leaves room */

        (void) AcpiUtValidateResource (WalkState, EndTag, &ResourceIndex);
        Status = UserFunction (EndTag, 2, Offset, ResourceIndex, Context);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    return_ACPI_STATUS (AE_AML_NO_RESOURCE_END_TAG);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValidateResource
 *
 * PARAMETERS:  WalkState           - Current walk info
 *              Aml                 - Pointer to the raw AML resource descriptor
 *              ReturnIndex         - Where the resource index is returned. NULL
 *                                    if the index is not required.
 *
 * RETURN:      Status, and optionally the Index into the global resource tables
 *
 * DESCRIPTION: Validate an AML resource descriptor by checking the Resource
 *              Type and Resource Length. Returns an index into the global
 *              resource information/dispatch tables for later use.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtValidateResource (
    ACPI_WALK_STATE         *WalkState,
    void                    *Aml,
    UINT8                   *ReturnIndex)
{
    AML_RESOURCE            *AmlResource;
    UINT8                   ResourceType;
    UINT8                   ResourceIndex;
    ACPI_RS_LENGTH          ResourceLength;
    ACPI_RS_LENGTH          MinimumResourceLength;


    ACPI_FUNCTION_ENTRY ();


    /*
     * 1) Validate the ResourceType field (Byte 0)
     */
    ResourceType = ACPI_GET8 (Aml);

    /*
     * Byte 0 contains the descriptor name (Resource Type)
     * Examine the large/small bit in the resource header
     */
    if (ResourceType & ACPI_RESOURCE_NAME_LARGE)
    {
        /* Verify the large resource type (name) against the max */

        if (ResourceType > ACPI_RESOURCE_NAME_LARGE_MAX)
        {
            goto InvalidResource;
        }

        /*
         * Large Resource Type -- bits 6:0 contain the name
         * Translate range 0x80-0x8B to index range 0x10-0x1B
         */
        ResourceIndex = (UINT8) (ResourceType - 0x70);
    }
    else
    {
        /*
         * Small Resource Type -- bits 6:3 contain the name
         * Shift range to index range 0x00-0x0F
         */
        ResourceIndex = (UINT8)
            ((ResourceType & ACPI_RESOURCE_NAME_SMALL_MASK) >> 3);
    }

    /*
     * Check validity of the resource type, via AcpiGbl_ResourceTypes.
     * Zero indicates an invalid resource.
     */
    if (!AcpiGbl_ResourceTypes[ResourceIndex])
    {
        goto InvalidResource;
    }

    /*
     * Validate the ResourceLength field. This ensures that the length
     * is at least reasonable, and guarantees that it is non-zero.
     */
    ResourceLength = AcpiUtGetResourceLength (Aml);
    MinimumResourceLength = AcpiGbl_ResourceAmlSizes[ResourceIndex];

    /* Validate based upon the type of resource - fixed length or variable */

    switch (AcpiGbl_ResourceTypes[ResourceIndex])
    {
    case ACPI_FIXED_LENGTH:

        /* Fixed length resource, length must match exactly */

        if (ResourceLength != MinimumResourceLength)
        {
            goto BadResourceLength;
        }
        break;

    case ACPI_VARIABLE_LENGTH:

        /* Variable length resource, length must be at least the minimum */

        if (ResourceLength < MinimumResourceLength)
        {
            goto BadResourceLength;
        }
        break;

    case ACPI_SMALL_VARIABLE_LENGTH:

        /* Small variable length resource, length can be (Min) or (Min-1) */

        if ((ResourceLength > MinimumResourceLength) ||
            (ResourceLength < (MinimumResourceLength - 1)))
        {
            goto BadResourceLength;
        }
        break;

    default:

        /* Shouldn't happen (because of validation earlier), but be sure */

        goto InvalidResource;
    }

    AmlResource = ACPI_CAST_PTR (AML_RESOURCE, Aml);
    if (ResourceType == ACPI_RESOURCE_NAME_SERIAL_BUS)
    {
        /* Validate the BusType field */

        if ((AmlResource->CommonSerialBus.Type == 0) ||
            (AmlResource->CommonSerialBus.Type > AML_RESOURCE_MAX_SERIALBUSTYPE))
        {
            if (WalkState)
            {
                ACPI_ERROR ((AE_INFO,
                    "Invalid/unsupported SerialBus resource descriptor: BusType 0x%2.2X",
                    AmlResource->CommonSerialBus.Type));
            }
            return (AE_AML_INVALID_RESOURCE_TYPE);
        }
    }

    /* Optionally return the resource table index */

    if (ReturnIndex)
    {
        *ReturnIndex = ResourceIndex;
    }

    return (AE_OK);


InvalidResource:

    if (WalkState)
    {
        ACPI_ERROR ((AE_INFO,
            "Invalid/unsupported resource descriptor: Type 0x%2.2X",
            ResourceType));
    }
    return (AE_AML_INVALID_RESOURCE_TYPE);

BadResourceLength:

    if (WalkState)
    {
        ACPI_ERROR ((AE_INFO,
            "Invalid resource descriptor length: Type "
            "0x%2.2X, Length 0x%4.4X, MinLength 0x%4.4X",
            ResourceType, ResourceLength, MinimumResourceLength));
    }
    return (AE_AML_BAD_RESOURCE_LENGTH);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceType
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *
 * RETURN:      The Resource Type with no extraneous bits (except the
 *              Large/Small descriptor bit -- this is left alone)
 *
 * DESCRIPTION: Extract the Resource Type/Name from the first byte of
 *              a resource descriptor.
 *
 ******************************************************************************/

UINT8
AcpiUtGetResourceType (
    void                    *Aml)
{
    ACPI_FUNCTION_ENTRY ();


    /*
     * Byte 0 contains the descriptor name (Resource Type)
     * Examine the large/small bit in the resource header
     */
    if (ACPI_GET8 (Aml) & ACPI_RESOURCE_NAME_LARGE)
    {
        /* Large Resource Type -- bits 6:0 contain the name */

        return (ACPI_GET8 (Aml));
    }
    else
    {
        /* Small Resource Type -- bits 6:3 contain the name */

        return ((UINT8) (ACPI_GET8 (Aml) & ACPI_RESOURCE_NAME_SMALL_MASK));
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceLength
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *
 * RETURN:      Byte Length
 *
 * DESCRIPTION: Get the "Resource Length" of a raw AML descriptor. By
 *              definition, this does not include the size of the descriptor
 *              header or the length field itself.
 *
 ******************************************************************************/

UINT16
AcpiUtGetResourceLength (
    void                    *Aml)
{
    ACPI_RS_LENGTH          ResourceLength;


    ACPI_FUNCTION_ENTRY ();


    /*
     * Byte 0 contains the descriptor name (Resource Type)
     * Examine the large/small bit in the resource header
     */
    if (ACPI_GET8 (Aml) & ACPI_RESOURCE_NAME_LARGE)
    {
        /* Large Resource type -- bytes 1-2 contain the 16-bit length */

        ACPI_MOVE_16_TO_16 (&ResourceLength, ACPI_ADD_PTR (UINT8, Aml, 1));

    }
    else
    {
        /* Small Resource type -- bits 2:0 of byte 0 contain the length */

        ResourceLength = (UINT16) (ACPI_GET8 (Aml) &
            ACPI_RESOURCE_NAME_SMALL_LENGTH_MASK);
    }

    return (ResourceLength);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceHeaderLength
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *
 * RETURN:      Length of the AML header (depends on large/small descriptor)
 *
 * DESCRIPTION: Get the length of the header for this resource.
 *
 ******************************************************************************/

UINT8
AcpiUtGetResourceHeaderLength (
    void                    *Aml)
{
    ACPI_FUNCTION_ENTRY ();


    /* Examine the large/small bit in the resource header */

    if (ACPI_GET8 (Aml) & ACPI_RESOURCE_NAME_LARGE)
    {
        return (sizeof (AML_RESOURCE_LARGE_HEADER));
    }
    else
    {
        return (sizeof (AML_RESOURCE_SMALL_HEADER));
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetDescriptorLength
 *
 * PARAMETERS:  Aml             - Pointer to the raw AML resource descriptor
 *
 * RETURN:      Byte length
 *
 * DESCRIPTION: Get the total byte length of a raw AML descriptor, including the
 *              length of the descriptor header and the length field itself.
 *              Used to walk descriptor lists.
 *
 ******************************************************************************/

UINT32
AcpiUtGetDescriptorLength (
    void                    *Aml)
{
    ACPI_FUNCTION_ENTRY ();


    /*
     * Get the Resource Length (does not include header length) and add
     * the header length (depends on if this is a small or large resource)
     */
    return (AcpiUtGetResourceLength (Aml) +
        AcpiUtGetResourceHeaderLength (Aml));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceEndTag
 *
 * PARAMETERS:  ObjDesc         - The resource template buffer object
 *              EndTag          - Where the pointer to the EndTag is returned
 *
 * RETURN:      Status, pointer to the end tag
 *
 * DESCRIPTION: Find the EndTag resource descriptor in an AML resource template
 *              Note: allows a buffer length of zero.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtGetResourceEndTag (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT8                   **EndTag)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtGetResourceEndTag);


    /* Allow a buffer length of zero */

    if (!ObjDesc->Buffer.Length)
    {
        *EndTag = ObjDesc->Buffer.Pointer;
        return_ACPI_STATUS (AE_OK);
    }

    /* Validate the template and get a pointer to the EndTag */

    Status = AcpiUtWalkAmlResources (NULL, ObjDesc->Buffer.Pointer,
        ObjDesc->Buffer.Length, NULL, (void **) EndTag);

    return_ACPI_STATUS (Status);
}
