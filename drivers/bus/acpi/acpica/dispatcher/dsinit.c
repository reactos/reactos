/******************************************************************************
 *
 * Module Name: dsinit - Object initialization namespace walk
 *
 *****************************************************************************/

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
#include "acdispat.h"
#include "acnamesp.h"
#include "actables.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsinit")


/* Local prototypes */

static ACPI_STATUS
AcpiDsInitOneObject (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsInitOneObject
 *
 * PARAMETERS:  ObjHandle       - Node for the object
 *              Level           - Current nesting level
 *              Context         - Points to a init info struct
 *              ReturnValue     - Not used
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Callback from AcpiWalkNamespace. Invoked for every object
 *              within the namespace.
 *
 *              Currently, the only objects that require initialization are:
 *              1) Methods
 *              2) Operation Regions
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiDsInitOneObject (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_INIT_WALK_INFO     *Info = (ACPI_INIT_WALK_INFO *) Context;
    ACPI_NAMESPACE_NODE     *Node = (ACPI_NAMESPACE_NODE *) ObjHandle;
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ACPI_FUNCTION_ENTRY ();


    /*
     * We are only interested in NS nodes owned by the table that
     * was just loaded
     */
    if (Node->OwnerId != Info->OwnerId)
    {
        return (AE_OK);
    }

    Info->ObjectCount++;

    /* And even then, we are only interested in a few object types */

    switch (AcpiNsGetType (ObjHandle))
    {
    case ACPI_TYPE_REGION:

        Status = AcpiDsInitializeRegion (ObjHandle);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "During Region initialization %p [%4.4s]",
                ObjHandle, AcpiUtGetNodeName (ObjHandle)));
        }

        Info->OpRegionCount++;
        break;

    case ACPI_TYPE_METHOD:
        /*
         * Auto-serialization support. We will examine each method that is
         * NotSerialized to determine if it creates any Named objects. If
         * it does, it will be marked serialized to prevent problems if
         * the method is entered by two or more threads and an attempt is
         * made to create the same named object twice -- which results in
         * an AE_ALREADY_EXISTS exception and method abort.
         */
        Info->MethodCount++;
        ObjDesc = AcpiNsGetAttachedObject (Node);
        if (!ObjDesc)
        {
            break;
        }

        /* Ignore if already serialized */

        if (ObjDesc->Method.InfoFlags & ACPI_METHOD_SERIALIZED)
        {
            Info->SerialMethodCount++;
            break;
        }

        if (AcpiGbl_AutoSerializeMethods)
        {
            /* Parse/scan method and serialize it if necessary */

            AcpiDsAutoSerializeMethod (Node, ObjDesc);
            if (ObjDesc->Method.InfoFlags & ACPI_METHOD_SERIALIZED)
            {
                /* Method was just converted to Serialized */

                Info->SerialMethodCount++;
                Info->SerializedMethodCount++;
                break;
            }
        }

        Info->NonSerialMethodCount++;
        break;

    case ACPI_TYPE_DEVICE:

        Info->DeviceCount++;
        break;

    default:

        break;
    }

    /*
     * We ignore errors from above, and always return OK, since
     * we don't want to abort the walk on a single error.
     */
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsInitializeObjects
 *
 * PARAMETERS:  TableDesc       - Descriptor for parent ACPI table
 *              StartNode       - Root of subtree to be initialized.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk the namespace starting at "StartNode" and perform any
 *              necessary initialization on the objects found therein
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsInitializeObjects (
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *StartNode)
{
    ACPI_STATUS             Status;
    ACPI_INIT_WALK_INFO     Info;
    ACPI_TABLE_HEADER       *Table;
    ACPI_OWNER_ID           OwnerId;


    ACPI_FUNCTION_TRACE (DsInitializeObjects);


    Status = AcpiTbGetOwnerId (TableIndex, &OwnerId);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "**** Starting initialization of namespace objects ****\n"));

    /* Set all init info to zero */

    memset (&Info, 0, sizeof (ACPI_INIT_WALK_INFO));

    Info.OwnerId = OwnerId;
    Info.TableIndex = TableIndex;

    /* Walk entire namespace from the supplied root */

    /*
     * We don't use AcpiWalkNamespace since we do not want to acquire
     * the namespace reader lock.
     */
    Status = AcpiNsWalkNamespace (ACPI_TYPE_ANY, StartNode, ACPI_UINT32_MAX,
        ACPI_NS_WALK_NO_UNLOCK, AcpiDsInitOneObject, NULL, &Info, NULL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "During WalkNamespace"));
    }

    Status = AcpiGetTableByIndex (TableIndex, &Table);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* DSDT is always the first AML table */

    if (ACPI_COMPARE_NAMESEG (Table->Signature, ACPI_SIG_DSDT))
    {
        ACPI_DEBUG_PRINT_RAW ((ACPI_DB_INIT,
            "\nACPI table initialization:\n"));
    }

    /* Summary of objects initialized */

    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_INIT,
        "Table [%4.4s: %-8.8s] (id %.2X) - %4u Objects with %3u Devices, "
        "%3u Regions, %4u Methods (%u/%u/%u Serial/Non/Cvt)\n",
        Table->Signature, Table->OemTableId, OwnerId, Info.ObjectCount,
        Info.DeviceCount,Info.OpRegionCount, Info.MethodCount,
        Info.SerialMethodCount, Info.NonSerialMethodCount,
        Info.SerializedMethodCount));

    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH, "%u Methods, %u Regions\n",
        Info.MethodCount, Info.OpRegionCount));

    return_ACPI_STATUS (AE_OK);
}
