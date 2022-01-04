/*******************************************************************************
 *
 * Module Name: rsxface - Public interfaces to the resource manager
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

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"
#include "acresrc.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_RESOURCES
        ACPI_MODULE_NAME    ("rsxface")

/* Local macros for 16,32-bit to 64-bit conversion */

#define ACPI_COPY_FIELD(Out, In, Field)  ((Out)->Field = (In)->Field)
#define ACPI_COPY_ADDRESS(Out, In)                       \
    ACPI_COPY_FIELD(Out, In, ResourceType);              \
    ACPI_COPY_FIELD(Out, In, ProducerConsumer);          \
    ACPI_COPY_FIELD(Out, In, Decode);                    \
    ACPI_COPY_FIELD(Out, In, MinAddressFixed);           \
    ACPI_COPY_FIELD(Out, In, MaxAddressFixed);           \
    ACPI_COPY_FIELD(Out, In, Info);                      \
    ACPI_COPY_FIELD(Out, In, Address.Granularity);       \
    ACPI_COPY_FIELD(Out, In, Address.Minimum);           \
    ACPI_COPY_FIELD(Out, In, Address.Maximum);           \
    ACPI_COPY_FIELD(Out, In, Address.TranslationOffset); \
    ACPI_COPY_FIELD(Out, In, Address.AddressLength);     \
    ACPI_COPY_FIELD(Out, In, ResourceSource);


/* Local prototypes */

static ACPI_STATUS
AcpiRsMatchVendorResource (
    ACPI_RESOURCE           *Resource,
    void                    *Context);

static ACPI_STATUS
AcpiRsValidateParameters (
    ACPI_HANDLE             DeviceHandle,
    ACPI_BUFFER             *Buffer,
    ACPI_NAMESPACE_NODE     **ReturnNode);


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsValidateParameters
 *
 * PARAMETERS:  DeviceHandle    - Handle to a device
 *              Buffer          - Pointer to a data buffer
 *              ReturnNode      - Pointer to where the device node is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Common parameter validation for resource interfaces
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiRsValidateParameters (
    ACPI_HANDLE             DeviceHandle,
    ACPI_BUFFER             *Buffer,
    ACPI_NAMESPACE_NODE     **ReturnNode)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;


    ACPI_FUNCTION_TRACE (RsValidateParameters);


    /*
     * Must have a valid handle to an ACPI device
     */
    if (!DeviceHandle)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Node = AcpiNsValidateHandle (DeviceHandle);
    if (!Node)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (Node->Type != ACPI_TYPE_DEVICE)
    {
        return_ACPI_STATUS (AE_TYPE);
    }

    /*
     * Validate the user buffer object
     *
     * if there is a non-zero buffer length we also need a valid pointer in
     * the buffer. If it's a zero buffer length, we'll be returning the
     * needed buffer size (later), so keep going.
     */
    Status = AcpiUtValidateBuffer (Buffer);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    *ReturnNode = Node;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetIrqRoutingTable
 *
 * PARAMETERS:  DeviceHandle    - Handle to the Bus device we are querying
 *              RetBuffer       - Pointer to a buffer to receive the
 *                                current resources for the device
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get the IRQ routing table for a
 *              specific bus. The caller must first acquire a handle for the
 *              desired bus. The routine table is placed in the buffer pointed
 *              to by the RetBuffer variable parameter.
 *
 *              If the function fails an appropriate status will be returned
 *              and the value of RetBuffer is undefined.
 *
 *              This function attempts to execute the _PRT method contained in
 *              the object indicated by the passed DeviceHandle.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetIrqRoutingTable  (
    ACPI_HANDLE             DeviceHandle,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;


    ACPI_FUNCTION_TRACE (AcpiGetIrqRoutingTable);


    /* Validate parameters then dispatch to internal routine */

    Status = AcpiRsValidateParameters (DeviceHandle, RetBuffer, &Node);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiRsGetPrtMethodData (Node, RetBuffer);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetIrqRoutingTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetCurrentResources
 *
 * PARAMETERS:  DeviceHandle    - Handle to the device object for the
 *                                device we are querying
 *              RetBuffer       - Pointer to a buffer to receive the
 *                                current resources for the device
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get the current resources for a
 *              specific device. The caller must first acquire a handle for
 *              the desired device. The resource data is placed in the buffer
 *              pointed to by the RetBuffer variable parameter.
 *
 *              If the function fails an appropriate status will be returned
 *              and the value of RetBuffer is undefined.
 *
 *              This function attempts to execute the _CRS method contained in
 *              the object indicated by the passed DeviceHandle.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetCurrentResources (
    ACPI_HANDLE             DeviceHandle,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;


    ACPI_FUNCTION_TRACE (AcpiGetCurrentResources);


    /* Validate parameters then dispatch to internal routine */

    Status = AcpiRsValidateParameters (DeviceHandle, RetBuffer, &Node);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiRsGetCrsMethodData (Node, RetBuffer);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetCurrentResources)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetPossibleResources
 *
 * PARAMETERS:  DeviceHandle    - Handle to the device object for the
 *                                device we are querying
 *              RetBuffer       - Pointer to a buffer to receive the
 *                                resources for the device
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get a list of the possible resources
 *              for a specific device. The caller must first acquire a handle
 *              for the desired device. The resource data is placed in the
 *              buffer pointed to by the RetBuffer variable.
 *
 *              If the function fails an appropriate status will be returned
 *              and the value of RetBuffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetPossibleResources (
    ACPI_HANDLE             DeviceHandle,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;


    ACPI_FUNCTION_TRACE (AcpiGetPossibleResources);


    /* Validate parameters then dispatch to internal routine */

    Status = AcpiRsValidateParameters (DeviceHandle, RetBuffer, &Node);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiRsGetPrsMethodData (Node, RetBuffer);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetPossibleResources)


/*******************************************************************************
 *
 * FUNCTION:    AcpiSetCurrentResources
 *
 * PARAMETERS:  DeviceHandle    - Handle to the device object for the
 *                                device we are setting resources
 *              InBuffer        - Pointer to a buffer containing the
 *                                resources to be set for the device
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to set the current resources for a
 *              specific device. The caller must first acquire a handle for
 *              the desired device. The resource data is passed to the routine
 *              the buffer pointed to by the InBuffer variable.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiSetCurrentResources (
    ACPI_HANDLE             DeviceHandle,
    ACPI_BUFFER             *InBuffer)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;


    ACPI_FUNCTION_TRACE (AcpiSetCurrentResources);


    /* Validate the buffer, don't allow zero length */

    if ((!InBuffer) ||
        (!InBuffer->Pointer) ||
        (!InBuffer->Length))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Validate parameters then dispatch to internal routine */

    Status = AcpiRsValidateParameters (DeviceHandle, InBuffer, &Node);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiRsSetSrsMethodData (Node, InBuffer);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiSetCurrentResources)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetEventResources
 *
 * PARAMETERS:  DeviceHandle    - Handle to the device object for the
 *                                device we are getting resources
 *              InBuffer        - Pointer to a buffer containing the
 *                                resources to be set for the device
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to get the event resources for a
 *              specific device. The caller must first acquire a handle for
 *              the desired device. The resource data is passed to the routine
 *              the buffer pointed to by the InBuffer variable. Uses the
 *              _AEI method.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetEventResources (
    ACPI_HANDLE             DeviceHandle,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;


    ACPI_FUNCTION_TRACE (AcpiGetEventResources);


    /* Validate parameters then dispatch to internal routine */

    Status = AcpiRsValidateParameters (DeviceHandle, RetBuffer, &Node);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiRsGetAeiMethodData (Node, RetBuffer);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetEventResources)


/******************************************************************************
 *
 * FUNCTION:    AcpiResourceToAddress64
 *
 * PARAMETERS:  Resource        - Pointer to a resource
 *              Out             - Pointer to the users's return buffer
 *                                (a struct acpi_resource_address64)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: If the resource is an address16, address32, or address64,
 *              copy it to the address64 return buffer. This saves the
 *              caller from having to duplicate code for different-sized
 *              addresses.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiResourceToAddress64 (
    ACPI_RESOURCE               *Resource,
    ACPI_RESOURCE_ADDRESS64     *Out)
{
    ACPI_RESOURCE_ADDRESS16     *Address16;
    ACPI_RESOURCE_ADDRESS32     *Address32;


    if (!Resource || !Out)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Convert 16 or 32 address descriptor to 64 */

    switch (Resource->Type)
    {
    case ACPI_RESOURCE_TYPE_ADDRESS16:

        Address16 = ACPI_CAST_PTR (
            ACPI_RESOURCE_ADDRESS16, &Resource->Data);
        ACPI_COPY_ADDRESS (Out, Address16);
        break;

    case ACPI_RESOURCE_TYPE_ADDRESS32:

        Address32 = ACPI_CAST_PTR (
            ACPI_RESOURCE_ADDRESS32, &Resource->Data);
        ACPI_COPY_ADDRESS (Out, Address32);
        break;

    case ACPI_RESOURCE_TYPE_ADDRESS64:

        /* Simple copy for 64 bit source */

        memcpy (Out, &Resource->Data, sizeof (ACPI_RESOURCE_ADDRESS64));
        break;

    default:

        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiResourceToAddress64)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetVendorResource
 *
 * PARAMETERS:  DeviceHandle    - Handle for the parent device object
 *              Name            - Method name for the parent resource
 *                                (METHOD_NAME__CRS or METHOD_NAME__PRS)
 *              Uuid            - Pointer to the UUID to be matched.
 *                                includes both subtype and 16-byte UUID
 *              RetBuffer       - Where the vendor resource is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk a resource template for the specified device to find a
 *              vendor-defined resource that matches the supplied UUID and
 *              UUID subtype. Returns a ACPI_RESOURCE of type Vendor.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetVendorResource (
    ACPI_HANDLE             DeviceHandle,
    char                    *Name,
    ACPI_VENDOR_UUID        *Uuid,
    ACPI_BUFFER             *RetBuffer)
{
    ACPI_VENDOR_WALK_INFO   Info;
    ACPI_STATUS             Status;


    /* Other parameters are validated by AcpiWalkResources */

    if (!Uuid || !RetBuffer)
    {
        return (AE_BAD_PARAMETER);
    }

    Info.Uuid = Uuid;
    Info.Buffer = RetBuffer;
    Info.Status = AE_NOT_EXIST;

    /* Walk the _CRS or _PRS resource list for this device */

    Status = AcpiWalkResources (
        DeviceHandle, Name, AcpiRsMatchVendorResource, &Info);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    return (Info.Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetVendorResource)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRsMatchVendorResource
 *
 * PARAMETERS:  ACPI_WALK_RESOURCE_CALLBACK
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Match a vendor resource via the ACPI 3.0 UUID
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiRsMatchVendorResource (
    ACPI_RESOURCE           *Resource,
    void                    *Context)
{
    ACPI_VENDOR_WALK_INFO       *Info = Context;
    ACPI_RESOURCE_VENDOR_TYPED  *Vendor;
    ACPI_BUFFER                 *Buffer;
    ACPI_STATUS                 Status;


    /* Ignore all descriptors except Vendor */

    if (Resource->Type != ACPI_RESOURCE_TYPE_VENDOR)
    {
        return (AE_OK);
    }

    Vendor = &Resource->Data.VendorTyped;

    /*
     * For a valid match, these conditions must hold:
     *
     * 1) Length of descriptor data must be at least as long as a UUID struct
     * 2) The UUID subtypes must match
     * 3) The UUID data must match
     */
    if ((Vendor->ByteLength < (ACPI_UUID_LENGTH + 1)) ||
        (Vendor->UuidSubtype != Info->Uuid->Subtype)  ||
        (memcmp (Vendor->Uuid, Info->Uuid->Data, ACPI_UUID_LENGTH)))
    {
        return (AE_OK);
    }

    /* Validate/Allocate/Clear caller buffer */

    Buffer = Info->Buffer;
    Status = AcpiUtInitializeBuffer (Buffer, Resource->Length);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Found the correct resource, copy and return it */

    memcpy (Buffer->Pointer, Resource, Resource->Length);
    Buffer->Length = Resource->Length;

    /* Found the desired descriptor, terminate resource walk */

    Info->Status = AE_OK;
    return (AE_CTRL_TERMINATE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiWalkResourceBuffer
 *
 * PARAMETERS:  Buffer          - Formatted buffer returned by one of the
 *                                various Get*Resource functions
 *              UserFunction    - Called for each resource
 *              Context         - Passed to UserFunction
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walks the input resource template. The UserFunction is called
 *              once for each resource in the list.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiWalkResourceBuffer (
    ACPI_BUFFER                 *Buffer,
    ACPI_WALK_RESOURCE_CALLBACK UserFunction,
    void                        *Context)
{
    ACPI_STATUS                 Status = AE_OK;
    ACPI_RESOURCE               *Resource;
    ACPI_RESOURCE               *ResourceEnd;


    ACPI_FUNCTION_TRACE (AcpiWalkResourceBuffer);


    /* Parameter validation */

    if (!Buffer || !Buffer->Pointer || !UserFunction)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Buffer contains the resource list and length */

    Resource = ACPI_CAST_PTR (ACPI_RESOURCE, Buffer->Pointer);
    ResourceEnd = ACPI_ADD_PTR (
        ACPI_RESOURCE, Buffer->Pointer, Buffer->Length);

    /* Walk the resource list until the EndTag is found (or buffer end) */

    while (Resource < ResourceEnd)
    {
        /* Sanity check the resource type */

        if (Resource->Type > ACPI_RESOURCE_TYPE_MAX)
        {
            Status = AE_AML_INVALID_RESOURCE_TYPE;
            break;
        }

        /* Sanity check the length. It must not be zero, or we loop forever */

        if (!Resource->Length)
        {
            return_ACPI_STATUS (AE_AML_BAD_RESOURCE_LENGTH);
        }

        /* Invoke the user function, abort on any error returned */

        Status = UserFunction (Resource, Context);
        if (ACPI_FAILURE (Status))
        {
            if (Status == AE_CTRL_TERMINATE)
            {
                /* This is an OK termination by the user function */

                Status = AE_OK;
            }
            break;
        }

        /* EndTag indicates end-of-list */

        if (Resource->Type == ACPI_RESOURCE_TYPE_END_TAG)
        {
            break;
        }

        /* Get the next resource descriptor */

        Resource = ACPI_NEXT_RESOURCE (Resource);
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiWalkResourceBuffer)


/*******************************************************************************
 *
 * FUNCTION:    AcpiWalkResources
 *
 * PARAMETERS:  DeviceHandle    - Handle to the device object for the
 *                                device we are querying
 *              Name            - Method name of the resources we want.
 *                                (METHOD_NAME__CRS, METHOD_NAME__PRS, or
 *                                METHOD_NAME__AEI or METHOD_NAME__DMA)
 *              UserFunction    - Called for each resource
 *              Context         - Passed to UserFunction
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieves the current or possible resource list for the
 *              specified device. The UserFunction is called once for
 *              each resource in the list.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiWalkResources (
    ACPI_HANDLE                 DeviceHandle,
    char                        *Name,
    ACPI_WALK_RESOURCE_CALLBACK UserFunction,
    void                        *Context)
{
    ACPI_STATUS                 Status;
    ACPI_BUFFER                 Buffer;


    ACPI_FUNCTION_TRACE (AcpiWalkResources);


    /* Parameter validation */

    if (!DeviceHandle || !UserFunction || !Name ||
        (!ACPI_COMPARE_NAMESEG (Name, METHOD_NAME__CRS) &&
         !ACPI_COMPARE_NAMESEG (Name, METHOD_NAME__PRS) &&
         !ACPI_COMPARE_NAMESEG (Name, METHOD_NAME__AEI) &&
         !ACPI_COMPARE_NAMESEG (Name, METHOD_NAME__DMA)))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Get the _CRS/_PRS/_AEI/_DMA resource list */

    Buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;
    Status = AcpiRsGetMethodData (DeviceHandle, Name, &Buffer);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Walk the resource list and cleanup */

    Status = AcpiWalkResourceBuffer (&Buffer, UserFunction, Context);
    ACPI_FREE (Buffer.Pointer);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiWalkResources)
