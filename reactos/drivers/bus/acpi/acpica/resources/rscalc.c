/*******************************************************************************
 *
 * Module Name: rscalc - Calculate stream and list lengths
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
#include "acnamesp.h"


#define _COMPONENT          ACPI_RESOURCES
        ACPI_MODULE_NAME    ("rscalc")


/* Local prototypes */

static UINT8
AcpiRsCountSetBits (
    UINT16                  BitField);

static ACPI_RS_LENGTH
AcpiRsStructOptionLength (
    ACPI_RESOURCE_SOURCE    *ResourceSource);

static UINT32
AcpiRsStreamOptionLength (
    UINT32                  ResourceLength,
    UINT32                  MinimumTotalLength);


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsCountSetBits
 *
 * PARAMETERS:  BitField        - Field in which to count bits
 *
 * RETURN:      Number of bits set within the field
 *
 * DESCRIPTION: Count the number of bits set in a resource field. Used for
 *              (Short descriptor) interrupt and DMA lists.
 *
 ******************************************************************************/

static UINT8
AcpiRsCountSetBits (
    UINT16                  BitField)
{
    UINT8                   BitsSet;


    ACPI_FUNCTION_ENTRY ();


    for (BitsSet = 0; BitField; BitsSet++)
    {
        /* Zero the least significant bit that is set */

        BitField &= (UINT16) (BitField - 1);
    }

    return (BitsSet);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsStructOptionLength
 *
 * PARAMETERS:  ResourceSource      - Pointer to optional descriptor field
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Common code to handle optional ResourceSourceIndex and
 *              ResourceSource fields in some Large descriptors. Used during
 *              list-to-stream conversion
 *
 ******************************************************************************/

static ACPI_RS_LENGTH
AcpiRsStructOptionLength (
    ACPI_RESOURCE_SOURCE    *ResourceSource)
{
    ACPI_FUNCTION_ENTRY ();


    /*
     * If the ResourceSource string is valid, return the size of the string
     * (StringLength includes the NULL terminator) plus the size of the
     * ResourceSourceIndex (1).
     */
    if (ResourceSource->StringPtr)
    {
        return ((ACPI_RS_LENGTH) (ResourceSource->StringLength + 1));
    }

    return (0);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsStreamOptionLength
 *
 * PARAMETERS:  ResourceLength      - Length from the resource header
 *              MinimumTotalLength  - Minimum length of this resource, before
 *                                    any optional fields. Includes header size
 *
 * RETURN:      Length of optional string (0 if no string present)
 *
 * DESCRIPTION: Common code to handle optional ResourceSourceIndex and
 *              ResourceSource fields in some Large descriptors. Used during
 *              stream-to-list conversion
 *
 ******************************************************************************/

static UINT32
AcpiRsStreamOptionLength (
    UINT32                  ResourceLength,
    UINT32                  MinimumAmlResourceLength)
{
    UINT32                  StringLength = 0;


    ACPI_FUNCTION_ENTRY ();


    /*
     * The ResourceSourceIndex and ResourceSource are optional elements of
     * some Large-type resource descriptors.
     */

    /*
     * If the length of the actual resource descriptor is greater than the
     * ACPI spec-defined minimum length, it means that a ResourceSourceIndex
     * exists and is followed by a (required) null terminated string. The
     * string length (including the null terminator) is the resource length
     * minus the minimum length, minus one byte for the ResourceSourceIndex
     * itself.
     */
    if (ResourceLength > MinimumAmlResourceLength)
    {
        /* Compute the length of the optional string */

        StringLength = ResourceLength - MinimumAmlResourceLength - 1;
    }

    /*
     * Round the length up to a multiple of the native word in order to
     * guarantee that the entire resource descriptor is native word aligned
     */
    return ((UINT32) ACPI_ROUND_UP_TO_NATIVE_WORD (StringLength));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetAmlLength
 *
 * PARAMETERS:  Resource            - Pointer to the resource linked list
 *              ResourceListSize    - Size of the resource linked list
 *              SizeNeeded          - Where the required size is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Takes a linked list of internal resource descriptors and
 *              calculates the size buffer needed to hold the corresponding
 *              external resource byte stream.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsGetAmlLength (
    ACPI_RESOURCE           *Resource,
    ACPI_SIZE               ResourceListSize,
    ACPI_SIZE               *SizeNeeded)
{
    ACPI_SIZE               AmlSizeNeeded = 0;
    ACPI_RESOURCE           *ResourceEnd;
    ACPI_RS_LENGTH          TotalSize;


    ACPI_FUNCTION_TRACE (RsGetAmlLength);


    /* Traverse entire list of internal resource descriptors */

    ResourceEnd = ACPI_ADD_PTR (ACPI_RESOURCE, Resource, ResourceListSize);
    while (Resource < ResourceEnd)
    {
        /* Validate the descriptor type */

        if (Resource->Type > ACPI_RESOURCE_TYPE_MAX)
        {
            return_ACPI_STATUS (AE_AML_INVALID_RESOURCE_TYPE);
        }

        /* Sanity check the length. It must not be zero, or we loop forever */

        if (!Resource->Length)
        {
            return_ACPI_STATUS (AE_AML_BAD_RESOURCE_LENGTH);
        }

        /* Get the base size of the (external stream) resource descriptor */

        TotalSize = AcpiGbl_AmlResourceSizes [Resource->Type];

        /*
         * Augment the base size for descriptors with optional and/or
         * variable-length fields
         */
        switch (Resource->Type)
        {
        case ACPI_RESOURCE_TYPE_IRQ:

            /* Length can be 3 or 2 */

            if (Resource->Data.Irq.DescriptorLength == 2)
            {
                TotalSize--;
            }
            break;


        case ACPI_RESOURCE_TYPE_START_DEPENDENT:

            /* Length can be 1 or 0 */

            if (Resource->Data.Irq.DescriptorLength == 0)
            {
                TotalSize--;
            }
            break;


        case ACPI_RESOURCE_TYPE_VENDOR:
            /*
             * Vendor Defined Resource:
             * For a Vendor Specific resource, if the Length is between 1 and 7
             * it will be created as a Small Resource data type, otherwise it
             * is a Large Resource data type.
             */
            if (Resource->Data.Vendor.ByteLength > 7)
            {
                /* Base size of a Large resource descriptor */

                TotalSize = sizeof (AML_RESOURCE_LARGE_HEADER);
            }

            /* Add the size of the vendor-specific data */

            TotalSize = (ACPI_RS_LENGTH)
                (TotalSize + Resource->Data.Vendor.ByteLength);
            break;


        case ACPI_RESOURCE_TYPE_END_TAG:
            /*
             * End Tag:
             * We are done -- return the accumulated total size.
             */
            *SizeNeeded = AmlSizeNeeded + TotalSize;

            /* Normal exit */

            return_ACPI_STATUS (AE_OK);


        case ACPI_RESOURCE_TYPE_ADDRESS16:
            /*
             * 16-Bit Address Resource:
             * Add the size of the optional ResourceSource info
             */
            TotalSize = (ACPI_RS_LENGTH) (TotalSize +
                AcpiRsStructOptionLength (
                    &Resource->Data.Address16.ResourceSource));
            break;


        case ACPI_RESOURCE_TYPE_ADDRESS32:
            /*
             * 32-Bit Address Resource:
             * Add the size of the optional ResourceSource info
             */
            TotalSize = (ACPI_RS_LENGTH) (TotalSize +
                AcpiRsStructOptionLength (
                    &Resource->Data.Address32.ResourceSource));
            break;


        case ACPI_RESOURCE_TYPE_ADDRESS64:
            /*
             * 64-Bit Address Resource:
             * Add the size of the optional ResourceSource info
             */
            TotalSize = (ACPI_RS_LENGTH) (TotalSize +
                AcpiRsStructOptionLength (
                    &Resource->Data.Address64.ResourceSource));
            break;


        case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            /*
             * Extended IRQ Resource:
             * Add the size of each additional optional interrupt beyond the
             * required 1 (4 bytes for each UINT32 interrupt number)
             */
            TotalSize = (ACPI_RS_LENGTH) (TotalSize +
                ((Resource->Data.ExtendedIrq.InterruptCount - 1) * 4) +

                /* Add the size of the optional ResourceSource info */

                AcpiRsStructOptionLength (
                    &Resource->Data.ExtendedIrq.ResourceSource));
            break;


        case ACPI_RESOURCE_TYPE_GPIO:

            TotalSize = (ACPI_RS_LENGTH) (TotalSize +
                (Resource->Data.Gpio.PinTableLength * 2) +
                Resource->Data.Gpio.ResourceSource.StringLength +
                Resource->Data.Gpio.VendorLength);

            break;


        case ACPI_RESOURCE_TYPE_SERIAL_BUS:

            TotalSize = AcpiGbl_AmlResourceSerialBusSizes [
                Resource->Data.CommonSerialBus.Type];

            TotalSize = (ACPI_RS_LENGTH) (TotalSize +
                Resource->Data.I2cSerialBus.ResourceSource.StringLength +
                Resource->Data.I2cSerialBus.VendorLength);

            break;

        default:

            break;
        }

        /* Update the total */

        AmlSizeNeeded += TotalSize;

        /* Point to the next object */

        Resource = ACPI_ADD_PTR (ACPI_RESOURCE, Resource, Resource->Length);
    }

    /* Did not find an EndTag resource descriptor */

    return_ACPI_STATUS (AE_AML_NO_RESOURCE_END_TAG);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetListLength
 *
 * PARAMETERS:  AmlBuffer           - Pointer to the resource byte stream
 *              AmlBufferLength     - Size of AmlBuffer
 *              SizeNeeded          - Where the size needed is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Takes an external resource byte stream and calculates the size
 *              buffer needed to hold the corresponding internal resource
 *              descriptor linked list.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsGetListLength (
    UINT8                   *AmlBuffer,
    UINT32                  AmlBufferLength,
    ACPI_SIZE               *SizeNeeded)
{
    ACPI_STATUS             Status;
    UINT8                   *EndAml;
    UINT8                   *Buffer;
    UINT32                  BufferSize;
    UINT16                  Temp16;
    UINT16                  ResourceLength;
    UINT32                  ExtraStructBytes;
    UINT8                   ResourceIndex;
    UINT8                   MinimumAmlResourceLength;
    AML_RESOURCE            *AmlResource;


    ACPI_FUNCTION_TRACE (RsGetListLength);


    *SizeNeeded = ACPI_RS_SIZE_MIN;         /* Minimum size is one EndTag */
    EndAml = AmlBuffer + AmlBufferLength;

    /* Walk the list of AML resource descriptors */

    while (AmlBuffer < EndAml)
    {
        /* Validate the Resource Type and Resource Length */

        Status = AcpiUtValidateResource (NULL, AmlBuffer, &ResourceIndex);
        if (ACPI_FAILURE (Status))
        {
            /*
             * Exit on failure. Cannot continue because the descriptor length
             * may be bogus also.
             */
            return_ACPI_STATUS (Status);
        }

        AmlResource = (void *) AmlBuffer;

        /* Get the resource length and base (minimum) AML size */

        ResourceLength = AcpiUtGetResourceLength (AmlBuffer);
        MinimumAmlResourceLength = AcpiGbl_ResourceAmlSizes[ResourceIndex];

        /*
         * Augment the size for descriptors with optional
         * and/or variable length fields
         */
        ExtraStructBytes = 0;
        Buffer = AmlBuffer + AcpiUtGetResourceHeaderLength (AmlBuffer);

        switch (AcpiUtGetResourceType (AmlBuffer))
        {
        case ACPI_RESOURCE_NAME_IRQ:
            /*
             * IRQ Resource:
             * Get the number of bits set in the 16-bit IRQ mask
             */
            ACPI_MOVE_16_TO_16 (&Temp16, Buffer);
            ExtraStructBytes = AcpiRsCountSetBits (Temp16);
            break;


        case ACPI_RESOURCE_NAME_DMA:
            /*
             * DMA Resource:
             * Get the number of bits set in the 8-bit DMA mask
             */
            ExtraStructBytes = AcpiRsCountSetBits (*Buffer);
            break;


        case ACPI_RESOURCE_NAME_VENDOR_SMALL:
        case ACPI_RESOURCE_NAME_VENDOR_LARGE:
            /*
             * Vendor Resource:
             * Get the number of vendor data bytes
             */
            ExtraStructBytes = ResourceLength;

            /*
             * There is already one byte included in the minimum
             * descriptor size. If there are extra struct bytes,
             * subtract one from the count.
             */
            if (ExtraStructBytes)
            {
                ExtraStructBytes--;
            }
            break;


        case ACPI_RESOURCE_NAME_END_TAG:
            /*
             * End Tag: This is the normal exit
             */
            return_ACPI_STATUS (AE_OK);


        case ACPI_RESOURCE_NAME_ADDRESS32:
        case ACPI_RESOURCE_NAME_ADDRESS16:
        case ACPI_RESOURCE_NAME_ADDRESS64:
            /*
             * Address Resource:
             * Add the size of the optional ResourceSource
             */
            ExtraStructBytes = AcpiRsStreamOptionLength (
                ResourceLength, MinimumAmlResourceLength);
            break;


        case ACPI_RESOURCE_NAME_EXTENDED_IRQ:
            /*
             * Extended IRQ Resource:
             * Using the InterruptTableLength, add 4 bytes for each additional
             * interrupt. Note: at least one interrupt is required and is
             * included in the minimum descriptor size (reason for the -1)
             */
            ExtraStructBytes = (Buffer[1] - 1) * sizeof (UINT32);

            /* Add the size of the optional ResourceSource */

            ExtraStructBytes += AcpiRsStreamOptionLength (
                ResourceLength - ExtraStructBytes, MinimumAmlResourceLength);
            break;

        case ACPI_RESOURCE_NAME_GPIO:

            /* Vendor data is optional */

            if (AmlResource->Gpio.VendorLength)
            {
                ExtraStructBytes +=
                    AmlResource->Gpio.VendorOffset -
                    AmlResource->Gpio.PinTableOffset +
                    AmlResource->Gpio.VendorLength;
            }
            else
            {
                ExtraStructBytes +=
                    AmlResource->LargeHeader.ResourceLength +
                    sizeof (AML_RESOURCE_LARGE_HEADER) -
                    AmlResource->Gpio.PinTableOffset;
            }
            break;

        case ACPI_RESOURCE_NAME_SERIAL_BUS:

            MinimumAmlResourceLength = AcpiGbl_ResourceAmlSerialBusSizes[
                AmlResource->CommonSerialBus.Type];
            ExtraStructBytes +=
                AmlResource->CommonSerialBus.ResourceLength -
                MinimumAmlResourceLength;
            break;

        default:

            break;
        }

        /*
         * Update the required buffer size for the internal descriptor structs
         *
         * Important: Round the size up for the appropriate alignment. This
         * is a requirement on IA64.
         */
        if (AcpiUtGetResourceType (AmlBuffer) ==
            ACPI_RESOURCE_NAME_SERIAL_BUS)
        {
            BufferSize = AcpiGbl_ResourceStructSerialBusSizes[
                AmlResource->CommonSerialBus.Type] + ExtraStructBytes;
        }
        else
        {
            BufferSize = AcpiGbl_ResourceStructSizes[ResourceIndex] +
                ExtraStructBytes;
        }

        BufferSize = (UINT32) ACPI_ROUND_UP_TO_NATIVE_WORD (BufferSize);
        *SizeNeeded += BufferSize;

        ACPI_DEBUG_PRINT ((ACPI_DB_RESOURCES,
            "Type %.2X, AmlLength %.2X InternalLength %.2X\n",
            AcpiUtGetResourceType (AmlBuffer),
            AcpiUtGetDescriptorLength (AmlBuffer), BufferSize));

        /*
         * Point to the next resource within the AML stream using the length
         * contained in the resource descriptor header
         */
        AmlBuffer += AcpiUtGetDescriptorLength (AmlBuffer);
    }

    /* Did not find an EndTag resource descriptor */

    return_ACPI_STATUS (AE_AML_NO_RESOURCE_END_TAG);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetPciRoutingTableLength
 *
 * PARAMETERS:  PackageObject           - Pointer to the package object
 *              BufferSizeNeeded        - UINT32 pointer of the size buffer
 *                                        needed to properly return the
 *                                        parsed data
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Given a package representing a PCI routing table, this
 *              calculates the size of the corresponding linked list of
 *              descriptions.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsGetPciRoutingTableLength (
    ACPI_OPERAND_OBJECT     *PackageObject,
    ACPI_SIZE               *BufferSizeNeeded)
{
    UINT32                  NumberOfElements;
    ACPI_SIZE               TempSizeNeeded = 0;
    ACPI_OPERAND_OBJECT     **TopObjectList;
    UINT32                  Index;
    ACPI_OPERAND_OBJECT     *PackageElement;
    ACPI_OPERAND_OBJECT     **SubObjectList;
    BOOLEAN                 NameFound;
    UINT32                  TableIndex;


    ACPI_FUNCTION_TRACE (RsGetPciRoutingTableLength);


    NumberOfElements = PackageObject->Package.Count;

    /*
     * Calculate the size of the return buffer.
     * The base size is the number of elements * the sizes of the
     * structures. Additional space for the strings is added below.
     * The minus one is to subtract the size of the UINT8 Source[1]
     * member because it is added below.
     *
     * But each PRT_ENTRY structure has a pointer to a string and
     * the size of that string must be found.
     */
    TopObjectList = PackageObject->Package.Elements;

    for (Index = 0; Index < NumberOfElements; Index++)
    {
        /* Dereference the subpackage */

        PackageElement = *TopObjectList;

        /* We must have a valid Package object */

        if (!PackageElement ||
            (PackageElement->Common.Type != ACPI_TYPE_PACKAGE))
        {
            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }

        /*
         * The SubObjectList will now point to an array of the
         * four IRQ elements: Address, Pin, Source and SourceIndex
         */
        SubObjectList = PackageElement->Package.Elements;

        /* Scan the IrqTableElements for the Source Name String */

        NameFound = FALSE;

        for (TableIndex = 0;
             TableIndex < PackageElement->Package.Count && !NameFound;
             TableIndex++)
        {
            if (*SubObjectList && /* Null object allowed */

                ((ACPI_TYPE_STRING ==
                    (*SubObjectList)->Common.Type) ||

                ((ACPI_TYPE_LOCAL_REFERENCE ==
                    (*SubObjectList)->Common.Type) &&

                    ((*SubObjectList)->Reference.Class ==
                        ACPI_REFCLASS_NAME))))
            {
                NameFound = TRUE;
            }
            else
            {
                /* Look at the next element */

                SubObjectList++;
            }
        }

        TempSizeNeeded += (sizeof (ACPI_PCI_ROUTING_TABLE) - 4);

        /* Was a String type found? */

        if (NameFound)
        {
            if ((*SubObjectList)->Common.Type == ACPI_TYPE_STRING)
            {
                /*
                 * The length String.Length field does not include the
                 * terminating NULL, add 1
                 */
                TempSizeNeeded += ((ACPI_SIZE)
                    (*SubObjectList)->String.Length + 1);
            }
            else
            {
                TempSizeNeeded += AcpiNsGetPathnameLength (
                    (*SubObjectList)->Reference.Node);
            }
        }
        else
        {
            /*
             * If no name was found, then this is a NULL, which is
             * translated as a UINT32 zero.
             */
            TempSizeNeeded += sizeof (UINT32);
        }

        /* Round up the size since each element must be aligned */

        TempSizeNeeded = ACPI_ROUND_UP_TO_64BIT (TempSizeNeeded);

        /* Point to the next ACPI_OPERAND_OBJECT */

        TopObjectList++;
    }

    /*
     * Add an extra element to the end of the list, essentially a
     * NULL terminator
     */
    *BufferSizeNeeded = TempSizeNeeded + sizeof (ACPI_PCI_ROUTING_TABLE);
    return_ACPI_STATUS (AE_OK);
}
