/*******************************************************************************
 *
 * Module Name: nsxfobj - Public interfaces to the ACPI subsystem
 *                         ACPI Object oriented interfaces
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2020, Intel Corp.
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

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsxfobj")

/*******************************************************************************
 *
 * FUNCTION:    AcpiGetType
 *
 * PARAMETERS:  Handle          - Handle of object whose type is desired
 *              RetType         - Where the type will be placed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This routine returns the type associated with a particular
 *              handle
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetType (
    ACPI_HANDLE             Handle,
    ACPI_OBJECT_TYPE        *RetType)
{
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    /* Parameter Validation */

    if (!RetType)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Special case for the predefined Root Node (return type ANY) */

    if (Handle == ACPI_ROOT_OBJECT)
    {
        *RetType = ACPI_TYPE_ANY;
        return (AE_OK);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Convert and validate the handle */

    Node = AcpiNsValidateHandle (Handle);
    if (!Node)
    {
        (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
        return (AE_BAD_PARAMETER);
    }

    *RetType = Node->Type;

    Status = AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetType)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetParent
 *
 * PARAMETERS:  Handle          - Handle of object whose parent is desired
 *              RetHandle       - Where the parent handle will be placed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Returns a handle to the parent of the object represented by
 *              Handle.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetParent (
    ACPI_HANDLE             Handle,
    ACPI_HANDLE             *RetHandle)
{
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_NAMESPACE_NODE     *ParentNode;
    ACPI_STATUS             Status;


    if (!RetHandle)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Special case for the predefined Root Node (no parent) */

    if (Handle == ACPI_ROOT_OBJECT)
    {
        return (AE_NULL_ENTRY);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Convert and validate the handle */

    Node = AcpiNsValidateHandle (Handle);
    if (!Node)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Get the parent entry */

    ParentNode = Node->Parent;
    *RetHandle = ACPI_CAST_PTR (ACPI_HANDLE, ParentNode);

    /* Return exception if parent is null */

    if (!ParentNode)
    {
        Status = AE_NULL_ENTRY;
    }


UnlockAndExit:

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetParent)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetNextObject
 *
 * PARAMETERS:  Type            - Type of object to be searched for
 *              Parent          - Parent object whose children we are getting
 *              LastChild       - Previous child that was found.
 *                                The NEXT child will be returned
 *              RetHandle       - Where handle to the next object is placed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Return the next peer object within the namespace. If Handle is
 *              valid, Scope is ignored. Otherwise, the first object within
 *              Scope is returned.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetNextObject (
    ACPI_OBJECT_TYPE        Type,
    ACPI_HANDLE             Parent,
    ACPI_HANDLE             Child,
    ACPI_HANDLE             *RetHandle)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_NAMESPACE_NODE     *ParentNode = NULL;
    ACPI_NAMESPACE_NODE     *ChildNode = NULL;


    /* Parameter validation */

    if (Type > ACPI_TYPE_EXTERNAL_MAX)
    {
        return (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* If null handle, use the parent */

    if (!Child)
    {
        /* Start search at the beginning of the specified scope */

        ParentNode = AcpiNsValidateHandle (Parent);
        if (!ParentNode)
        {
            Status = AE_BAD_PARAMETER;
            goto UnlockAndExit;
        }
    }
    else
    {
        /* Non-null handle, ignore the parent */
        /* Convert and validate the handle */

        ChildNode = AcpiNsValidateHandle (Child);
        if (!ChildNode)
        {
            Status = AE_BAD_PARAMETER;
            goto UnlockAndExit;
        }
    }

    /* Internal function does the real work */

    Node = AcpiNsGetNextNodeTyped (Type, ParentNode, ChildNode);
    if (!Node)
    {
        Status = AE_NOT_FOUND;
        goto UnlockAndExit;
    }

    if (RetHandle)
    {
        *RetHandle = ACPI_CAST_PTR (ACPI_HANDLE, Node);
    }


UnlockAndExit:

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetNextObject)
