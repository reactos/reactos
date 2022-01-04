/*******************************************************************************
 *
 * Module Name: rsutils - Utilities for the resource manager
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
#include "acnamesp.h"
#include "acresrc.h"


#define _COMPONENT          ACPI_RESOURCES
        ACPI_MODULE_NAME    ("rsutils")


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsDecodeBitmask
 *
 * PARAMETERS:  Mask            - Bitmask to decode
 *              List            - Where the converted list is returned
 *
 * RETURN:      Count of bits set (length of list)
 *
 * DESCRIPTION: Convert a bit mask into a list of values
 *
 ******************************************************************************/

UINT8
AcpiRsDecodeBitmask (
    UINT16                  Mask,
    UINT8                   *List)
{
    UINT8                   i;
    UINT8                   BitCount;


    ACPI_FUNCTION_ENTRY ();


    /* Decode the mask bits */

    for (i = 0, BitCount = 0; Mask; i++)
    {
        if (Mask & 0x0001)
        {
            List[BitCount] = i;
            BitCount++;
        }

        Mask >>= 1;
    }

    return (BitCount);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsEncodeBitmask
 *
 * PARAMETERS:  List            - List of values to encode
 *              Count           - Length of list
 *
 * RETURN:      Encoded bitmask
 *
 * DESCRIPTION: Convert a list of values to an encoded bitmask
 *
 ******************************************************************************/

UINT16
AcpiRsEncodeBitmask (
    UINT8                   *List,
    UINT8                   Count)
{
    UINT32                  i;
    UINT16                  Mask;


    ACPI_FUNCTION_ENTRY ();


    /* Encode the list into a single bitmask */

    for (i = 0, Mask = 0; i < Count; i++)
    {
        Mask |= (0x1 << List[i]);
    }

    return (Mask);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsMoveData
 *
 * PARAMETERS:  Destination         - Pointer to the destination descriptor
 *              Source              - Pointer to the source descriptor
 *              ItemCount           - How many items to move
 *              MoveType            - Byte width
 *
 * RETURN:      None
 *
 * DESCRIPTION: Move multiple data items from one descriptor to another. Handles
 *              alignment issues and endian issues if necessary, as configured
 *              via the ACPI_MOVE_* macros. (This is why a memcpy is not used)
 *
 ******************************************************************************/

void
AcpiRsMoveData (
    void                    *Destination,
    void                    *Source,
    UINT16                  ItemCount,
    UINT8                   MoveType)
{
    UINT32                  i;


    ACPI_FUNCTION_ENTRY ();


    /* One move per item */

    for (i = 0; i < ItemCount; i++)
    {
        switch (MoveType)
        {
        /*
         * For the 8-bit case, we can perform the move all at once
         * since there are no alignment or endian issues
         */
        case ACPI_RSC_MOVE8:
        case ACPI_RSC_MOVE_GPIO_RES:
        case ACPI_RSC_MOVE_SERIAL_VEN:
        case ACPI_RSC_MOVE_SERIAL_RES:

            memcpy (Destination, Source, ItemCount);
            return;

        /*
         * 16-, 32-, and 64-bit cases must use the move macros that perform
         * endian conversion and/or accommodate hardware that cannot perform
         * misaligned memory transfers
         */
        case ACPI_RSC_MOVE16:
        case ACPI_RSC_MOVE_GPIO_PIN:

            ACPI_MOVE_16_TO_16 (
                &ACPI_CAST_PTR (UINT16, Destination)[i],
                &ACPI_CAST_PTR (UINT16, Source)[i]);
            break;

        case ACPI_RSC_MOVE32:

            ACPI_MOVE_32_TO_32 (
                &ACPI_CAST_PTR (UINT32, Destination)[i],
                &ACPI_CAST_PTR (UINT32, Source)[i]);
            break;

        case ACPI_RSC_MOVE64:

            ACPI_MOVE_64_TO_64 (
                &ACPI_CAST_PTR (UINT64, Destination)[i],
                &ACPI_CAST_PTR (UINT64, Source)[i]);
            break;

        default:

            return;
        }
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsSetResourceLength
 *
 * PARAMETERS:  TotalLength         - Length of the AML descriptor, including
 *                                    the header and length fields.
 *              Aml                 - Pointer to the raw AML descriptor
 *
 * RETURN:      None
 *
 * DESCRIPTION: Set the ResourceLength field of an AML
 *              resource descriptor, both Large and Small descriptors are
 *              supported automatically. Note: Descriptor Type field must
 *              be valid.
 *
 ******************************************************************************/

void
AcpiRsSetResourceLength (
    ACPI_RSDESC_SIZE        TotalLength,
    AML_RESOURCE            *Aml)
{
    ACPI_RS_LENGTH          ResourceLength;


    ACPI_FUNCTION_ENTRY ();


    /* Length is the total descriptor length minus the header length */

    ResourceLength = (ACPI_RS_LENGTH)
        (TotalLength - AcpiUtGetResourceHeaderLength (Aml));

    /* Length is stored differently for large and small descriptors */

    if (Aml->SmallHeader.DescriptorType & ACPI_RESOURCE_NAME_LARGE)
    {
        /* Large descriptor -- bytes 1-2 contain the 16-bit length */

        ACPI_MOVE_16_TO_16 (
            &Aml->LargeHeader.ResourceLength, &ResourceLength);
    }
    else
    {
        /*
         * Small descriptor -- bits 2:0 of byte 0 contain the length
         * Clear any existing length, preserving descriptor type bits
         */
        Aml->SmallHeader.DescriptorType = (UINT8)
            ((Aml->SmallHeader.DescriptorType &
                ~ACPI_RESOURCE_NAME_SMALL_LENGTH_MASK)
            | ResourceLength);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsSetResourceHeader
 *
 * PARAMETERS:  DescriptorType      - Byte to be inserted as the type
 *              TotalLength         - Length of the AML descriptor, including
 *                                    the header and length fields.
 *              Aml                 - Pointer to the raw AML descriptor
 *
 * RETURN:      None
 *
 * DESCRIPTION: Set the DescriptorType and ResourceLength fields of an AML
 *              resource descriptor, both Large and Small descriptors are
 *              supported automatically
 *
 ******************************************************************************/

void
AcpiRsSetResourceHeader (
    UINT8                   DescriptorType,
    ACPI_RSDESC_SIZE        TotalLength,
    AML_RESOURCE            *Aml)
{
    ACPI_FUNCTION_ENTRY ();


    /* Set the Resource Type */

    Aml->SmallHeader.DescriptorType = DescriptorType;

    /* Set the Resource Length */

    AcpiRsSetResourceLength (TotalLength, Aml);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsStrcpy
 *
 * PARAMETERS:  Destination         - Pointer to the destination string
 *              Source              - Pointer to the source string
 *
 * RETURN:      String length, including NULL terminator
 *
 * DESCRIPTION: Local string copy that returns the string length, saving a
 *              strcpy followed by a strlen.
 *
 ******************************************************************************/

static UINT16
AcpiRsStrcpy (
    char                    *Destination,
    char                    *Source)
{
    UINT16                  i;


    ACPI_FUNCTION_ENTRY ();


    for (i = 0; Source[i]; i++)
    {
        Destination[i] = Source[i];
    }

    Destination[i] = 0;

    /* Return string length including the NULL terminator */

    return ((UINT16) (i + 1));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetResourceSource
 *
 * PARAMETERS:  ResourceLength      - Length field of the descriptor
 *              MinimumLength       - Minimum length of the descriptor (minus
 *                                    any optional fields)
 *              ResourceSource      - Where the ResourceSource is returned
 *              Aml                 - Pointer to the raw AML descriptor
 *              StringPtr           - (optional) where to store the actual
 *                                    ResourceSource string
 *
 * RETURN:      Length of the string plus NULL terminator, rounded up to native
 *              word boundary
 *
 * DESCRIPTION: Copy the optional ResourceSource data from a raw AML descriptor
 *              to an internal resource descriptor
 *
 ******************************************************************************/

ACPI_RS_LENGTH
AcpiRsGetResourceSource (
    ACPI_RS_LENGTH          ResourceLength,
    ACPI_RS_LENGTH          MinimumLength,
    ACPI_RESOURCE_SOURCE    *ResourceSource,
    AML_RESOURCE            *Aml,
    char                    *StringPtr)
{
    ACPI_RSDESC_SIZE        TotalLength;
    UINT8                   *AmlResourceSource;


    ACPI_FUNCTION_ENTRY ();


    TotalLength = ResourceLength + sizeof (AML_RESOURCE_LARGE_HEADER);
    AmlResourceSource = ACPI_ADD_PTR (UINT8, Aml, MinimumLength);

    /*
     * ResourceSource is present if the length of the descriptor is longer
     * than the minimum length.
     *
     * Note: Some resource descriptors will have an additional null, so
     * we add 1 to the minimum length.
     */
    if (TotalLength > (ACPI_RSDESC_SIZE) (MinimumLength + 1))
    {
        /* Get the ResourceSourceIndex */

        ResourceSource->Index = AmlResourceSource[0];

        ResourceSource->StringPtr = StringPtr;
        if (!StringPtr)
        {
            /*
             * String destination pointer is not specified; Set the String
             * pointer to the end of the current ResourceSource structure.
             */
            ResourceSource->StringPtr = ACPI_ADD_PTR (
                char, ResourceSource, sizeof (ACPI_RESOURCE_SOURCE));
        }

        /*
         * In order for the Resource length to be a multiple of the native
         * word, calculate the length of the string (+1 for NULL terminator)
         * and expand to the next word multiple.
         *
         * Zero the entire area of the buffer.
         */
        TotalLength = (UINT32) strlen (
            ACPI_CAST_PTR (char, &AmlResourceSource[1])) + 1;

        TotalLength = (UINT32) ACPI_ROUND_UP_TO_NATIVE_WORD (TotalLength);

        memset (ResourceSource->StringPtr, 0, TotalLength);

        /* Copy the ResourceSource string to the destination */

        ResourceSource->StringLength = AcpiRsStrcpy (
            ResourceSource->StringPtr,
            ACPI_CAST_PTR (char, &AmlResourceSource[1]));

        return ((ACPI_RS_LENGTH) TotalLength);
    }

    /* ResourceSource is not present */

    ResourceSource->Index = 0;
    ResourceSource->StringLength = 0;
    ResourceSource->StringPtr = NULL;
    return (0);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsSetResourceSource
 *
 * PARAMETERS:  Aml                 - Pointer to the raw AML descriptor
 *              MinimumLength       - Minimum length of the descriptor (minus
 *                                    any optional fields)
 *              ResourceSource      - Internal ResourceSource

 *
 * RETURN:      Total length of the AML descriptor
 *
 * DESCRIPTION: Convert an optional ResourceSource from internal format to a
 *              raw AML resource descriptor
 *
 ******************************************************************************/

ACPI_RSDESC_SIZE
AcpiRsSetResourceSource (
    AML_RESOURCE            *Aml,
    ACPI_RS_LENGTH          MinimumLength,
    ACPI_RESOURCE_SOURCE    *ResourceSource)
{
    UINT8                   *AmlResourceSource;
    ACPI_RSDESC_SIZE        DescriptorLength;


    ACPI_FUNCTION_ENTRY ();


    DescriptorLength = MinimumLength;

    /* Non-zero string length indicates presence of a ResourceSource */

    if (ResourceSource->StringLength)
    {
        /* Point to the end of the AML descriptor */

        AmlResourceSource = ACPI_ADD_PTR (UINT8, Aml, MinimumLength);

        /* Copy the ResourceSourceIndex */

        AmlResourceSource[0] = (UINT8) ResourceSource->Index;

        /* Copy the ResourceSource string */

        strcpy (ACPI_CAST_PTR (char, &AmlResourceSource[1]),
            ResourceSource->StringPtr);

        /*
         * Add the length of the string (+ 1 for null terminator) to the
         * final descriptor length
         */
        DescriptorLength += ((ACPI_RSDESC_SIZE)
            ResourceSource->StringLength + 1);
    }

    /* Return the new total length of the AML descriptor */

    return (DescriptorLength);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetPrtMethodData
 *
 * PARAMETERS:  Node            - Device node
 *              RetBuffer       - Pointer to a buffer structure for the
 *                                results
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get the _PRT value of an object
 *              contained in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsGetPrtMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (RsGetPrtMethodData);


    /* Parameters guaranteed valid by caller */

    /* Execute the method, no parameters */

    Status = AcpiUtEvaluateObject (
        Node, METHOD_NAME__PRT, ACPI_BTYPE_PACKAGE, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Create a resource linked list from the byte stream buffer that comes
     * back from the _CRS method execution.
     */
    Status = AcpiRsCreatePciRoutingTable (ObjDesc, RetBuffer);

    /* On exit, we must delete the object returned by EvaluateObject */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetCrsMethodData
 *
 * PARAMETERS:  Node            - Device node
 *              RetBuffer       - Pointer to a buffer structure for the
 *                                results
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get the _CRS value of an object
 *              contained in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsGetCrsMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (RsGetCrsMethodData);


    /* Parameters guaranteed valid by caller */

    /* Execute the method, no parameters */

    Status = AcpiUtEvaluateObject (
        Node, METHOD_NAME__CRS, ACPI_BTYPE_BUFFER, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Make the call to create a resource linked list from the
     * byte stream buffer that comes back from the _CRS method
     * execution.
     */
    Status = AcpiRsCreateResourceList (ObjDesc, RetBuffer);

    /* On exit, we must delete the object returned by evaluateObject */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetPrsMethodData
 *
 * PARAMETERS:  Node            - Device node
 *              RetBuffer       - Pointer to a buffer structure for the
 *                                results
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get the _PRS value of an object
 *              contained in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsGetPrsMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (RsGetPrsMethodData);


    /* Parameters guaranteed valid by caller */

    /* Execute the method, no parameters */

    Status = AcpiUtEvaluateObject (
        Node, METHOD_NAME__PRS, ACPI_BTYPE_BUFFER, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Make the call to create a resource linked list from the
     * byte stream buffer that comes back from the _CRS method
     * execution.
     */
    Status = AcpiRsCreateResourceList (ObjDesc, RetBuffer);

    /* On exit, we must delete the object returned by evaluateObject */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetAeiMethodData
 *
 * PARAMETERS:  Node            - Device node
 *              RetBuffer       - Pointer to a buffer structure for the
 *                                results
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get the _AEI value of an object
 *              contained in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsGetAeiMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (RsGetAeiMethodData);


    /* Parameters guaranteed valid by caller */

    /* Execute the method, no parameters */

    Status = AcpiUtEvaluateObject (
        Node, METHOD_NAME__AEI, ACPI_BTYPE_BUFFER, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Make the call to create a resource linked list from the
     * byte stream buffer that comes back from the _CRS method
     * execution.
     */
    Status = AcpiRsCreateResourceList (ObjDesc, RetBuffer);

    /* On exit, we must delete the object returned by evaluateObject */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsGetMethodData
 *
 * PARAMETERS:  Handle          - Handle to the containing object
 *              Path            - Path to method, relative to Handle
 *              RetBuffer       - Pointer to a buffer structure for the
 *                                results
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get the _CRS or _PRS value of an
 *              object contained in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsGetMethodData (
    ACPI_HANDLE             Handle,
    const char              *Path,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (RsGetMethodData);


    /* Parameters guaranteed valid by caller */

    /* Execute the method, no parameters */

    Status = AcpiUtEvaluateObject (
        ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, Handle),
        Path, ACPI_BTYPE_BUFFER, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Make the call to create a resource linked list from the
     * byte stream buffer that comes back from the method
     * execution.
     */
    Status = AcpiRsCreateResourceList (ObjDesc, RetBuffer);

    /* On exit, we must delete the object returned by EvaluateObject */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsSetSrsMethodData
 *
 * PARAMETERS:  Node            - Device node
 *              InBuffer        - Pointer to a buffer structure of the
 *                                parameter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to set the _SRS of an object contained
 *              in an object specified by the handle passed in
 *
 *              If the function fails an appropriate status will be returned
 *              and the contents of the callers buffer is undefined.
 *
 * Note: Parameters guaranteed valid by caller
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRsSetSrsMethodData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_BUFFER             *InBuffer)
{
    ACPI_EVALUATE_INFO      *Info;
    ACPI_OPERAND_OBJECT     *Args[2];
    ACPI_STATUS             Status;
    ACPI_BUFFER             Buffer;


    ACPI_FUNCTION_TRACE (RsSetSrsMethodData);


    /* Allocate and initialize the evaluation information block */

    Info = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_EVALUATE_INFO));
    if (!Info)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Info->PrefixNode = Node;
    Info->RelativePathname = METHOD_NAME__SRS;
    Info->Parameters = Args;
    Info->Flags = ACPI_IGNORE_RETURN_VALUE;

    /*
     * The InBuffer parameter will point to a linked list of
     * resource parameters. It needs to be formatted into a
     * byte stream to be sent in as an input parameter to _SRS
     *
     * Convert the linked list into a byte stream
     */
    Buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;
    Status = AcpiRsCreateAmlResources (InBuffer, &Buffer);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /* Create and initialize the method parameter object */

    Args[0] = AcpiUtCreateInternalObject (ACPI_TYPE_BUFFER);
    if (!Args[0])
    {
        /*
         * Must free the buffer allocated above (otherwise it is freed
         * later)
         */
        ACPI_FREE (Buffer.Pointer);
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    Args[0]->Buffer.Length  = (UINT32) Buffer.Length;
    Args[0]->Buffer.Pointer = Buffer.Pointer;
    Args[0]->Common.Flags   = AOPOBJ_DATA_VALID;
    Args[1] = NULL;

    /* Execute the method, no return value is expected */

    Status = AcpiNsEvaluate (Info);

    /* Clean up and return the status from AcpiNsEvaluate */

    AcpiUtRemoveReference (Args[0]);

Cleanup:
    ACPI_FREE (Info);
    return_ACPI_STATUS (Status);
}
