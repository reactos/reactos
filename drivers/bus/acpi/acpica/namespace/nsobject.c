/*******************************************************************************
 *
 * Module Name: nsobject - Utilities for objects attached to namespace
 *                         table entries
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


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsobject")


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsAttachObject
 *
 * PARAMETERS:  Node                - Parent Node
 *              Object              - Object to be attached
 *              Type                - Type of object, or ACPI_TYPE_ANY if not
 *                                    known
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Record the given object as the value associated with the
 *              name whose ACPI_HANDLE is passed. If Object is NULL
 *              and Type is ACPI_TYPE_ANY, set the name as having no value.
 *              Note: Future may require that the Node->Flags field be passed
 *              as a parameter.
 *
 * MUTEX:       Assumes namespace is locked
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsAttachObject (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OPERAND_OBJECT     *Object,
    ACPI_OBJECT_TYPE        Type)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *LastObjDesc;
    ACPI_OBJECT_TYPE        ObjectType = ACPI_TYPE_ANY;


    ACPI_FUNCTION_TRACE (NsAttachObject);


    /*
     * Parameter validation
     */
    if (!Node)
    {
        /* Invalid handle */

        ACPI_ERROR ((AE_INFO, "Null NamedObj handle"));
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (!Object && (ACPI_TYPE_ANY != Type))
    {
        /* Null object */

        ACPI_ERROR ((AE_INFO,
            "Null object, but type not ACPI_TYPE_ANY"));
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (ACPI_GET_DESCRIPTOR_TYPE (Node) != ACPI_DESC_TYPE_NAMED)
    {
        /* Not a name handle */

        ACPI_ERROR ((AE_INFO, "Invalid handle %p [%s]",
            Node, AcpiUtGetDescriptorName (Node)));
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Check if this object is already attached */

    if (Node->Object == Object)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "Obj %p already installed in NameObj %p\n",
            Object, Node));

        return_ACPI_STATUS (AE_OK);
    }

    /* If null object, we will just install it */

    if (!Object)
    {
        ObjDesc    = NULL;
        ObjectType = ACPI_TYPE_ANY;
    }

    /*
     * If the source object is a namespace Node with an attached object,
     * we will use that (attached) object
     */
    else if ((ACPI_GET_DESCRIPTOR_TYPE (Object) == ACPI_DESC_TYPE_NAMED) &&
            ((ACPI_NAMESPACE_NODE *) Object)->Object)
    {
        /*
         * Value passed is a name handle and that name has a
         * non-null value. Use that name's value and type.
         */
        ObjDesc = ((ACPI_NAMESPACE_NODE *) Object)->Object;
        ObjectType = ((ACPI_NAMESPACE_NODE *) Object)->Type;
    }

    /*
     * Otherwise, we will use the parameter object, but we must type
     * it first
     */
    else
    {
        ObjDesc = (ACPI_OPERAND_OBJECT  *) Object;

        /* Use the given type */

        ObjectType = Type;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Installing %p into Node %p [%4.4s]\n",
        ObjDesc, Node, AcpiUtGetNodeName (Node)));

    /* Detach an existing attached object if present */

    if (Node->Object)
    {
        AcpiNsDetachObject (Node);
    }

    if (ObjDesc)
    {
        /*
         * Must increment the new value's reference count
         * (if it is an internal object)
         */
        AcpiUtAddReference (ObjDesc);

        /*
         * Handle objects with multiple descriptors - walk
         * to the end of the descriptor list
         */
        LastObjDesc = ObjDesc;
        while (LastObjDesc->Common.NextObject)
        {
            LastObjDesc = LastObjDesc->Common.NextObject;
        }

        /* Install the object at the front of the object list */

        LastObjDesc->Common.NextObject = Node->Object;
    }

    Node->Type = (UINT8) ObjectType;
    Node->Object = ObjDesc;

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsDetachObject
 *
 * PARAMETERS:  Node           - A Namespace node whose object will be detached
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Detach/delete an object associated with a namespace node.
 *              if the object is an allocated object, it is freed.
 *              Otherwise, the field is simply cleared.
 *
 ******************************************************************************/

void
AcpiNsDetachObject (
    ACPI_NAMESPACE_NODE     *Node)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ACPI_FUNCTION_TRACE (NsDetachObject);


    ObjDesc = Node->Object;

    if (!ObjDesc ||
        (ObjDesc->Common.Type == ACPI_TYPE_LOCAL_DATA))
    {
        return_VOID;
    }

    if (Node->Flags & ANOBJ_ALLOCATED_BUFFER)
    {
        /* Free the dynamic aml buffer */

        if (ObjDesc->Common.Type == ACPI_TYPE_METHOD)
        {
            ACPI_FREE (ObjDesc->Method.AmlStart);
        }
    }

    if (ObjDesc->Common.Type == ACPI_TYPE_REGION)
    {
        AcpiUtRemoveAddressRange(ObjDesc->Region.SpaceId, Node);
    }

    /* Clear the Node entry in all cases */

    Node->Object = NULL;
    if (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) == ACPI_DESC_TYPE_OPERAND)
    {
        /* Unlink object from front of possible object list */

        Node->Object = ObjDesc->Common.NextObject;

        /* Handle possible 2-descriptor object */

        if (Node->Object &&
           (Node->Object->Common.Type != ACPI_TYPE_LOCAL_DATA))
        {
            Node->Object = Node->Object->Common.NextObject;
        }

        /*
         * Detach the object from any data objects (which are still held by
         * the namespace node)
         */
        if (ObjDesc->Common.NextObject &&
           ((ObjDesc->Common.NextObject)->Common.Type == ACPI_TYPE_LOCAL_DATA))
        {
           ObjDesc->Common.NextObject = NULL;
        }
    }

    /* Reset the node type to untyped */

    Node->Type = ACPI_TYPE_ANY;

    ACPI_DEBUG_PRINT ((ACPI_DB_NAMES, "Node %p [%4.4s] Object %p\n",
        Node, AcpiUtGetNodeName (Node), ObjDesc));

    /* Remove one reference on the object (and all subobjects) */

    AcpiUtRemoveReference (ObjDesc);
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetAttachedObject
 *
 * PARAMETERS:  Node             - Namespace node
 *
 * RETURN:      Current value of the object field from the Node whose
 *              handle is passed
 *
 * DESCRIPTION: Obtain the object attached to a namespace node.
 *
 ******************************************************************************/

ACPI_OPERAND_OBJECT *
AcpiNsGetAttachedObject (
    ACPI_NAMESPACE_NODE     *Node)
{
    ACPI_FUNCTION_TRACE_PTR (NsGetAttachedObject, Node);


    if (!Node)
    {
        ACPI_WARNING ((AE_INFO, "Null Node ptr"));
        return_PTR (NULL);
    }

    if (!Node->Object ||
            ((ACPI_GET_DESCRIPTOR_TYPE (Node->Object) != ACPI_DESC_TYPE_OPERAND) &&
             (ACPI_GET_DESCRIPTOR_TYPE (Node->Object) != ACPI_DESC_TYPE_NAMED))  ||
        ((Node->Object)->Common.Type == ACPI_TYPE_LOCAL_DATA))
    {
        return_PTR (NULL);
    }

    return_PTR (Node->Object);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetSecondaryObject
 *
 * PARAMETERS:  Node             - Namespace node
 *
 * RETURN:      Current value of the object field from the Node whose
 *              handle is passed.
 *
 * DESCRIPTION: Obtain a secondary object associated with a namespace node.
 *
 ******************************************************************************/

ACPI_OPERAND_OBJECT *
AcpiNsGetSecondaryObject (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_FUNCTION_TRACE_PTR (NsGetSecondaryObject, ObjDesc);


    if ((!ObjDesc)                                     ||
        (ObjDesc->Common.Type== ACPI_TYPE_LOCAL_DATA)  ||
        (!ObjDesc->Common.NextObject)                  ||
        ((ObjDesc->Common.NextObject)->Common.Type == ACPI_TYPE_LOCAL_DATA))
    {
        return_PTR (NULL);
    }

    return_PTR (ObjDesc->Common.NextObject);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsAttachData
 *
 * PARAMETERS:  Node            - Namespace node
 *              Handler         - Handler to be associated with the data
 *              Data            - Data to be attached
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Low-level attach data. Create and attach a Data object.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsAttachData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OBJECT_HANDLER     Handler,
    void                    *Data)
{
    ACPI_OPERAND_OBJECT     *PrevObjDesc;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *DataDesc;


    /* We only allow one attachment per handler */

    PrevObjDesc = NULL;
    ObjDesc = Node->Object;
    while (ObjDesc)
    {
        if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_DATA) &&
            (ObjDesc->Data.Handler == Handler))
        {
            return (AE_ALREADY_EXISTS);
        }

        PrevObjDesc = ObjDesc;
        ObjDesc = ObjDesc->Common.NextObject;
    }

    /* Create an internal object for the data */

    DataDesc = AcpiUtCreateInternalObject (ACPI_TYPE_LOCAL_DATA);
    if (!DataDesc)
    {
        return (AE_NO_MEMORY);
    }

    DataDesc->Data.Handler = Handler;
    DataDesc->Data.Pointer = Data;

    /* Install the data object */

    if (PrevObjDesc)
    {
        PrevObjDesc->Common.NextObject = DataDesc;
    }
    else
    {
        Node->Object = DataDesc;
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsDetachData
 *
 * PARAMETERS:  Node            - Namespace node
 *              Handler         - Handler associated with the data
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Low-level detach data. Delete the data node, but the caller
 *              is responsible for the actual data.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsDetachData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OBJECT_HANDLER     Handler)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *PrevObjDesc;


    PrevObjDesc = NULL;
    ObjDesc = Node->Object;
    while (ObjDesc)
    {
        if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_DATA) &&
            (ObjDesc->Data.Handler == Handler))
        {
            if (PrevObjDesc)
            {
                PrevObjDesc->Common.NextObject = ObjDesc->Common.NextObject;
            }
            else
            {
                Node->Object = ObjDesc->Common.NextObject;
            }

            AcpiUtRemoveReference (ObjDesc);
            return (AE_OK);
        }

        PrevObjDesc = ObjDesc;
        ObjDesc = ObjDesc->Common.NextObject;
    }

    return (AE_NOT_FOUND);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetAttachedData
 *
 * PARAMETERS:  Node            - Namespace node
 *              Handler         - Handler associated with the data
 *              Data            - Where the data is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Low level interface to obtain data previously associated with
 *              a namespace node.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsGetAttachedData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OBJECT_HANDLER     Handler,
    void                    **Data)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ObjDesc = Node->Object;
    while (ObjDesc)
    {
        if ((ObjDesc->Common.Type == ACPI_TYPE_LOCAL_DATA) &&
            (ObjDesc->Data.Handler == Handler))
        {
            *Data = ObjDesc->Data.Pointer;
            return (AE_OK);
        }

        ObjDesc = ObjDesc->Common.NextObject;
    }

    return (AE_NOT_FOUND);
}
