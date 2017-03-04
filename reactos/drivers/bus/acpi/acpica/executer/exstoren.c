/******************************************************************************
 *
 * Module Name: exstoren - AML Interpreter object store support,
 *                        Store to Node (namespace object)
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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
#include "acinterp.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exstoren")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExResolveObject
 *
 * PARAMETERS:  SourceDescPtr       - Pointer to the source object
 *              TargetType          - Current type of the target
 *              WalkState           - Current walk state
 *
 * RETURN:      Status, resolved object in SourceDescPtr.
 *
 * DESCRIPTION: Resolve an object. If the object is a reference, dereference
 *              it and return the actual object in the SourceDescPtr.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExResolveObject (
    ACPI_OPERAND_OBJECT     **SourceDescPtr,
    ACPI_OBJECT_TYPE        TargetType,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *SourceDesc = *SourceDescPtr;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExResolveObject);


    /* Ensure we have a Target that can be stored to */

    switch (TargetType)
    {
    case ACPI_TYPE_BUFFER_FIELD:
    case ACPI_TYPE_LOCAL_REGION_FIELD:
    case ACPI_TYPE_LOCAL_BANK_FIELD:
    case ACPI_TYPE_LOCAL_INDEX_FIELD:
        /*
         * These cases all require only Integers or values that
         * can be converted to Integers (Strings or Buffers)
         */
    case ACPI_TYPE_INTEGER:
    case ACPI_TYPE_STRING:
    case ACPI_TYPE_BUFFER:
        /*
         * Stores into a Field/Region or into a Integer/Buffer/String
         * are all essentially the same. This case handles the
         * "interchangeable" types Integer, String, and Buffer.
         */
        if (SourceDesc->Common.Type == ACPI_TYPE_LOCAL_REFERENCE)
        {
            /* Resolve a reference object first */

            Status = AcpiExResolveToValue (SourceDescPtr, WalkState);
            if (ACPI_FAILURE (Status))
            {
                break;
            }
        }

        /* For CopyObject, no further validation necessary */

        if (WalkState->Opcode == AML_COPY_OBJECT_OP)
        {
            break;
        }

        /* Must have a Integer, Buffer, or String */

        if ((SourceDesc->Common.Type != ACPI_TYPE_INTEGER)    &&
            (SourceDesc->Common.Type != ACPI_TYPE_BUFFER)     &&
            (SourceDesc->Common.Type != ACPI_TYPE_STRING)     &&
            !((SourceDesc->Common.Type == ACPI_TYPE_LOCAL_REFERENCE) &&
                (SourceDesc->Reference.Class== ACPI_REFCLASS_TABLE)))
        {
            /* Conversion successful but still not a valid type */

            ACPI_ERROR ((AE_INFO,
                "Cannot assign type [%s] to [%s] (must be type Int/Str/Buf)",
                AcpiUtGetObjectTypeName (SourceDesc),
                AcpiUtGetTypeName (TargetType)));

            Status = AE_AML_OPERAND_TYPE;
        }
        break;

    case ACPI_TYPE_LOCAL_ALIAS:
    case ACPI_TYPE_LOCAL_METHOD_ALIAS:
        /*
         * All aliases should have been resolved earlier, during the
         * operand resolution phase.
         */
        ACPI_ERROR ((AE_INFO, "Store into an unresolved Alias object"));
        Status = AE_AML_INTERNAL;
        break;

    case ACPI_TYPE_PACKAGE:
    default:
        /*
         * All other types than Alias and the various Fields come here,
         * including the untyped case - ACPI_TYPE_ANY.
         */
        break;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExStoreObjectToObject
 *
 * PARAMETERS:  SourceDesc          - Object to store
 *              DestDesc            - Object to receive a copy of the source
 *              NewDesc             - New object if DestDesc is obsoleted
 *              WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: "Store" an object to another object. This may include
 *              converting the source type to the target type (implicit
 *              conversion), and a copy of the value of the source to
 *              the target.
 *
 *              The Assignment of an object to another (not named) object
 *              is handled here.
 *              The Source passed in will replace the current value (if any)
 *              with the input value.
 *
 *              When storing into an object the data is converted to the
 *              target object type then stored in the object. This means
 *              that the target object type (for an initialized target) will
 *              not be changed by a store operation.
 *
 *              This module allows destination types of Number, String,
 *              Buffer, and Package.
 *
 *              Assumes parameters are already validated. NOTE: SourceDesc
 *              resolution (from a reference object) must be performed by
 *              the caller if necessary.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExStoreObjectToObject (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *DestDesc,
    ACPI_OPERAND_OBJECT     **NewDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *ActualSrcDesc;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_PTR (ExStoreObjectToObject, SourceDesc);


    ActualSrcDesc = SourceDesc;
    if (!DestDesc)
    {
        /*
         * There is no destination object (An uninitialized node or
         * package element), so we can simply copy the source object
         * creating a new destination object
         */
        Status = AcpiUtCopyIobjectToIobject (ActualSrcDesc, NewDesc, WalkState);
        return_ACPI_STATUS (Status);
    }

    if (SourceDesc->Common.Type != DestDesc->Common.Type)
    {
        /*
         * The source type does not match the type of the destination.
         * Perform the "implicit conversion" of the source to the current type
         * of the target as per the ACPI specification.
         *
         * If no conversion performed, ActualSrcDesc = SourceDesc.
         * Otherwise, ActualSrcDesc is a temporary object to hold the
         * converted object.
         */
        Status = AcpiExConvertToTargetType (DestDesc->Common.Type,
            SourceDesc, &ActualSrcDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        if (SourceDesc == ActualSrcDesc)
        {
            /*
             * No conversion was performed. Return the SourceDesc as the
             * new object.
             */
            *NewDesc = SourceDesc;
            return_ACPI_STATUS (AE_OK);
        }
    }

    /*
     * We now have two objects of identical types, and we can perform a
     * copy of the *value* of the source object.
     */
    switch (DestDesc->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        DestDesc->Integer.Value = ActualSrcDesc->Integer.Value;

        /* Truncate value if we are executing from a 32-bit ACPI table */

        (void) AcpiExTruncateFor32bitTable (DestDesc);
        break;

    case ACPI_TYPE_STRING:

        Status = AcpiExStoreStringToString (ActualSrcDesc, DestDesc);
        break;

    case ACPI_TYPE_BUFFER:

        Status = AcpiExStoreBufferToBuffer (ActualSrcDesc, DestDesc);
        break;

    case ACPI_TYPE_PACKAGE:

        Status = AcpiUtCopyIobjectToIobject (ActualSrcDesc, &DestDesc,
                    WalkState);
        break;

    default:
        /*
         * All other types come here.
         */
        ACPI_WARNING ((AE_INFO, "Store into type [%s] not implemented",
            AcpiUtGetObjectTypeName (DestDesc)));

        Status = AE_NOT_IMPLEMENTED;
        break;
    }

    if (ActualSrcDesc != SourceDesc)
    {
        /* Delete the intermediate (temporary) source object */

        AcpiUtRemoveReference (ActualSrcDesc);
    }

    *NewDesc = DestDesc;
    return_ACPI_STATUS (Status);
}
