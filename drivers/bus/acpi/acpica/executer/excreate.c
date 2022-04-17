/******************************************************************************
 *
 * Module Name: excreate - Named object creation
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
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("excreate")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCreateAlias
 *
 * PARAMETERS:  WalkState            - Current state, contains operands
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new named alias
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExCreateAlias (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_NAMESPACE_NODE     *TargetNode;
    ACPI_NAMESPACE_NODE     *AliasNode;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExCreateAlias);


    /* Get the source/alias operands (both namespace nodes) */

    AliasNode =  (ACPI_NAMESPACE_NODE *) WalkState->Operands[0];
    TargetNode = (ACPI_NAMESPACE_NODE *) WalkState->Operands[1];

    if ((TargetNode->Type == ACPI_TYPE_LOCAL_ALIAS)  ||
        (TargetNode->Type == ACPI_TYPE_LOCAL_METHOD_ALIAS))
    {
        /*
         * Dereference an existing alias so that we don't create a chain
         * of aliases. With this code, we guarantee that an alias is
         * always exactly one level of indirection away from the
         * actual aliased name.
         */
        TargetNode = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, TargetNode->Object);
    }

    /* Ensure that the target node is valid */

    if (!TargetNode)
    {
        return_ACPI_STATUS (AE_NULL_OBJECT);
    }

    /* Construct the alias object (a namespace node) */

    switch (TargetNode->Type)
    {
    case ACPI_TYPE_METHOD:
        /*
         * Control method aliases need to be differentiated with
         * a special type
         */
        AliasNode->Type = ACPI_TYPE_LOCAL_METHOD_ALIAS;
        break;

    default:
        /*
         * All other object types.
         *
         * The new alias has the type ALIAS and points to the original
         * NS node, not the object itself.
         */
        AliasNode->Type = ACPI_TYPE_LOCAL_ALIAS;
        AliasNode->Object = ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, TargetNode);
        break;
    }

    /* Since both operands are Nodes, we don't need to delete them */

    AliasNode->Object = ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, TargetNode);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCreateEvent
 *
 * PARAMETERS:  WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new event object
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExCreateEvent (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ACPI_FUNCTION_TRACE (ExCreateEvent);


    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_EVENT);
    if (!ObjDesc)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /*
     * Create the actual OS semaphore, with zero initial units -- meaning
     * that the event is created in an unsignalled state
     */
    Status = AcpiOsCreateSemaphore (ACPI_NO_UNIT_LIMIT, 0,
        &ObjDesc->Event.OsSemaphore);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /* Attach object to the Node */

    Status = AcpiNsAttachObject (
        (ACPI_NAMESPACE_NODE *) WalkState->Operands[0],
        ObjDesc, ACPI_TYPE_EVENT);

Cleanup:
    /*
     * Remove local reference to the object (on error, will cause deletion
     * of both object and semaphore if present.)
     */
    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCreateMutex
 *
 * PARAMETERS:  WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new mutex object
 *
 *              Mutex (Name[0], SyncLevel[1])
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExCreateMutex (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ACPI_FUNCTION_TRACE_PTR (ExCreateMutex, ACPI_WALK_OPERANDS);


    /* Create the new mutex object */

    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_MUTEX);
    if (!ObjDesc)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Create the actual OS Mutex */

    Status = AcpiOsCreateMutex (&ObjDesc->Mutex.OsMutex);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /* Init object and attach to NS node */

    ObjDesc->Mutex.SyncLevel = (UINT8) WalkState->Operands[1]->Integer.Value;
    ObjDesc->Mutex.Node = (ACPI_NAMESPACE_NODE *) WalkState->Operands[0];

    Status = AcpiNsAttachObject (
        ObjDesc->Mutex.Node, ObjDesc, ACPI_TYPE_MUTEX);


Cleanup:
    /*
     * Remove local reference to the object (on error, will cause deletion
     * of both object and semaphore if present.)
     */
    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCreateRegion
 *
 * PARAMETERS:  AmlStart            - Pointer to the region declaration AML
 *              AmlLength           - Max length of the declaration AML
 *              SpaceId             - Address space ID for the region
 *              WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new operation region object
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExCreateRegion (
    UINT8                   *AmlStart,
    UINT32                  AmlLength,
    UINT8                   SpaceId,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_OPERAND_OBJECT     *RegionObj2;


    ACPI_FUNCTION_TRACE (ExCreateRegion);


    /* Get the Namespace Node */

    Node = WalkState->Op->Common.Node;

    /*
     * If the region object is already attached to this node,
     * just return
     */
    if (AcpiNsGetAttachedObject (Node))
    {
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * Space ID must be one of the predefined IDs, or in the user-defined
     * range
     */
    if (!AcpiIsValidSpaceId (SpaceId))
    {
        /*
         * Print an error message, but continue. We don't want to abort
         * a table load for this exception. Instead, if the region is
         * actually used at runtime, abort the executing method.
         */
        ACPI_ERROR ((AE_INFO,
            "Invalid/unknown Address Space ID: 0x%2.2X", SpaceId));
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_LOAD, "Region Type - %s (0x%X)\n",
        AcpiUtGetRegionName (SpaceId), SpaceId));

    /* Create the region descriptor */

    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_REGION);
    if (!ObjDesc)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /*
     * Remember location in AML stream of address & length
     * operands since they need to be evaluated at run time.
     */
    RegionObj2 = AcpiNsGetSecondaryObject (ObjDesc);
    RegionObj2->Extra.AmlStart = AmlStart;
    RegionObj2->Extra.AmlLength = AmlLength;
    RegionObj2->Extra.Method_REG = NULL;
    if (WalkState->ScopeInfo)
    {
        RegionObj2->Extra.ScopeNode = WalkState->ScopeInfo->Scope.Node;
    }
    else
    {
        RegionObj2->Extra.ScopeNode = Node;
    }

    /* Init the region from the operands */

    ObjDesc->Region.SpaceId = SpaceId;
    ObjDesc->Region.Address = 0;
    ObjDesc->Region.Length = 0;
    ObjDesc->Region.Node = Node;
    ObjDesc->Region.Handler = NULL;
    ObjDesc->Common.Flags &=
        ~(AOPOBJ_SETUP_COMPLETE | AOPOBJ_REG_CONNECTED |
          AOPOBJ_OBJECT_INITIALIZED);

    /* Install the new region object in the parent Node */

    Status = AcpiNsAttachObject (Node, ObjDesc, ACPI_TYPE_REGION);


Cleanup:

    /* Remove local reference to the object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCreateProcessor
 *
 * PARAMETERS:  WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new processor object and populate the fields
 *
 *              Processor (Name[0], CpuID[1], PblockAddr[2], PblockLength[3])
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExCreateProcessor (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (ExCreateProcessor, WalkState);


    /* Create the processor object */

    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_PROCESSOR);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Initialize the processor object from the operands */

    ObjDesc->Processor.ProcId = (UINT8) Operand[1]->Integer.Value;
    ObjDesc->Processor.Length = (UINT8) Operand[3]->Integer.Value;
    ObjDesc->Processor.Address = (ACPI_IO_ADDRESS) Operand[2]->Integer.Value;

    /* Install the processor object in the parent Node */

    Status = AcpiNsAttachObject ((ACPI_NAMESPACE_NODE *) Operand[0],
        ObjDesc, ACPI_TYPE_PROCESSOR);

    /* Remove local reference to the object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCreatePowerResource
 *
 * PARAMETERS:  WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new PowerResource object and populate the fields
 *
 *              PowerResource (Name[0], SystemLevel[1], ResourceOrder[2])
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExCreatePowerResource (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ACPI_FUNCTION_TRACE_PTR (ExCreatePowerResource, WalkState);


    /* Create the power resource object */

    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_POWER);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Initialize the power object from the operands */

    ObjDesc->PowerResource.SystemLevel = (UINT8) Operand[1]->Integer.Value;
    ObjDesc->PowerResource.ResourceOrder = (UINT16) Operand[2]->Integer.Value;

    /* Install the  power resource object in the parent Node */

    Status = AcpiNsAttachObject ((ACPI_NAMESPACE_NODE *) Operand[0],
        ObjDesc, ACPI_TYPE_POWER);

    /* Remove local reference to the object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCreateMethod
 *
 * PARAMETERS:  AmlStart        - First byte of the method's AML
 *              AmlLength       - AML byte count for this method
 *              WalkState       - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new method object
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExCreateMethod (
    UINT8                   *AmlStart,
    UINT32                  AmlLength,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;
    UINT8                   MethodFlags;


    ACPI_FUNCTION_TRACE_PTR (ExCreateMethod, WalkState);


    /* Create a new method object */

    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_METHOD);
    if (!ObjDesc)
    {
       Status = AE_NO_MEMORY;
       goto Exit;
    }

    /* Save the method's AML pointer and length  */

    ObjDesc->Method.AmlStart = AmlStart;
    ObjDesc->Method.AmlLength = AmlLength;
    ObjDesc->Method.Node = Operand[0];

    /*
     * Disassemble the method flags. Split off the ArgCount, Serialized
     * flag, and SyncLevel for efficiency.
     */
    MethodFlags = (UINT8) Operand[1]->Integer.Value;
    ObjDesc->Method.ParamCount = (UINT8)
        (MethodFlags & AML_METHOD_ARG_COUNT);

    /*
     * Get the SyncLevel. If method is serialized, a mutex will be
     * created for this method when it is parsed.
     */
    if (MethodFlags & AML_METHOD_SERIALIZED)
    {
        ObjDesc->Method.InfoFlags = ACPI_METHOD_SERIALIZED;

        /*
         * ACPI 1.0: SyncLevel = 0
         * ACPI 2.0: SyncLevel = SyncLevel in method declaration
         */
        ObjDesc->Method.SyncLevel = (UINT8)
            ((MethodFlags & AML_METHOD_SYNC_LEVEL) >> 4);
    }

    /* Attach the new object to the method Node */

    Status = AcpiNsAttachObject ((ACPI_NAMESPACE_NODE *) Operand[0],
        ObjDesc, ACPI_TYPE_METHOD);

    /* Remove local reference to the object */

    AcpiUtRemoveReference (ObjDesc);

Exit:
    /* Remove a reference to the operand */

    AcpiUtRemoveReference (Operand[1]);
    return_ACPI_STATUS (Status);
}
