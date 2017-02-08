/******************************************************************************
 *
 * Module Name: exoparg2 - AML execution - opcodes with 2 arguments
 *
 *****************************************************************************/

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
#include "acparser.h"
#include "acinterp.h"
#include "acevents.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exoparg2")


/*!
 * Naming convention for AML interpreter execution routines.
 *
 * The routines that begin execution of AML opcodes are named with a common
 * convention based upon the number of arguments, the number of target operands,
 * and whether or not a value is returned:
 *
 *      AcpiExOpcode_xA_yT_zR
 *
 * Where:
 *
 * xA - ARGUMENTS:    The number of arguments (input operands) that are
 *                    required for this opcode type (1 through 6 args).
 * yT - TARGETS:      The number of targets (output operands) that are required
 *                    for this opcode type (0, 1, or 2 targets).
 * zR - RETURN VALUE: Indicates whether this opcode type returns a value
 *                    as the function return (0 or 1).
 *
 * The AcpiExOpcode* functions are called via the Dispatcher component with
 * fully resolved operands.
!*/


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_2A_0T_0R
 *
 * PARAMETERS:  WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with two arguments, no target, and no return
 *              value.
 *
 * ALLOCATION:  Deletes both operands
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_2A_0T_0R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_NAMESPACE_NODE     *Node;
    UINT32                  Value;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_2A_0T_0R,
            AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Examine the opcode */

    switch (WalkState->Opcode)
    {
    case AML_NOTIFY_OP:         /* Notify (NotifyObject, NotifyValue) */

        /* The first operand is a namespace node */

        Node = (ACPI_NAMESPACE_NODE *) Operand[0];

        /* Second value is the notify value */

        Value = (UINT32) Operand[1]->Integer.Value;

        /* Are notifies allowed on this object? */

        if (!AcpiEvIsNotifyObject (Node))
        {
            ACPI_ERROR ((AE_INFO,
                "Unexpected notify object type [%s]",
                AcpiUtGetTypeName (Node->Type)));

            Status = AE_AML_OPERAND_TYPE;
            break;
        }

        /*
         * Dispatch the notify to the appropriate handler
         * NOTE: the request is queued for execution after this method
         * completes. The notify handlers are NOT invoked synchronously
         * from this thread -- because handlers may in turn run other
         * control methods.
         */
        Status = AcpiEvQueueNotifyRequest (Node, Value);
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_2A_2T_1R
 *
 * PARAMETERS:  WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a dyadic operator (2 operands) with 2 output targets
 *              and one implicit return value.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_2A_2T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *ReturnDesc1 = NULL;
    ACPI_OPERAND_OBJECT     *ReturnDesc2 = NULL;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_2A_2T_1R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Execute the opcode */

    switch (WalkState->Opcode)
    {
    case AML_DIVIDE_OP:

        /* Divide (Dividend, Divisor, RemainderResult QuotientResult) */

        ReturnDesc1 = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!ReturnDesc1)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        ReturnDesc2 = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!ReturnDesc2)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        /* Quotient to ReturnDesc1, remainder to ReturnDesc2 */

        Status = AcpiUtDivide (
            Operand[0]->Integer.Value,
            Operand[1]->Integer.Value,
            &ReturnDesc1->Integer.Value,
            &ReturnDesc2->Integer.Value);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));

        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }

    /* Store the results to the target reference operands */

    Status = AcpiExStore (ReturnDesc2, Operand[2], WalkState);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    Status = AcpiExStore (ReturnDesc1, Operand[3], WalkState);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

Cleanup:
    /*
     * Since the remainder is not returned indirectly, remove a reference to
     * it. Only the quotient is returned indirectly.
     */
    AcpiUtRemoveReference (ReturnDesc2);

    if (ACPI_FAILURE (Status))
    {
        /* Delete the return object */

        AcpiUtRemoveReference (ReturnDesc1);
    }

    /* Save return object (the remainder) on success */

    else
    {
        WalkState->ResultObj = ReturnDesc1;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_2A_1T_1R
 *
 * PARAMETERS:  WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with two arguments, one target, and a return
 *              value.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_2A_1T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *ReturnDesc = NULL;
    UINT64                  Index;
    ACPI_STATUS             Status = AE_OK;
    ACPI_SIZE               Length = 0;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_2A_1T_1R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Execute the opcode */

    if (WalkState->OpInfo->Flags & AML_MATH)
    {
        /* All simple math opcodes (add, etc.) */

        ReturnDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        ReturnDesc->Integer.Value = AcpiExDoMathOp (
            WalkState->Opcode,
            Operand[0]->Integer.Value,
            Operand[1]->Integer.Value);
        goto StoreResultToTarget;
    }

    switch (WalkState->Opcode)
    {
    case AML_MOD_OP: /* Mod (Dividend, Divisor, RemainderResult (ACPI 2.0) */

        ReturnDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        /* ReturnDesc will contain the remainder */

        Status = AcpiUtDivide (
            Operand[0]->Integer.Value,
            Operand[1]->Integer.Value,
            NULL,
            &ReturnDesc->Integer.Value);
        break;

    case AML_CONCAT_OP: /* Concatenate (Data1, Data2, Result) */

        Status = AcpiExDoConcatenate (
            Operand[0], Operand[1], &ReturnDesc, WalkState);
        break;

    case AML_TO_STRING_OP: /* ToString (Buffer, Length, Result) (ACPI 2.0) */
        /*
         * Input object is guaranteed to be a buffer at this point (it may have
         * been converted.)  Copy the raw buffer data to a new object of
         * type String.
         */

        /*
         * Get the length of the new string. It is the smallest of:
         * 1) Length of the input buffer
         * 2) Max length as specified in the ToString operator
         * 3) Length of input buffer up to a zero byte (null terminator)
         *
         * NOTE: A length of zero is ok, and will create a zero-length, null
         *       terminated string.
         */
        while ((Length < Operand[0]->Buffer.Length) &&
               (Length < Operand[1]->Integer.Value) &&
               (Operand[0]->Buffer.Pointer[Length]))
        {
            Length++;
        }

        /* Allocate a new string object */

        ReturnDesc = AcpiUtCreateStringObject (Length);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        /*
         * Copy the raw buffer data with no transform.
         * (NULL terminated already)
         */
        memcpy (ReturnDesc->String.Pointer,
            Operand[0]->Buffer.Pointer, Length);
        break;

    case AML_CONCAT_RES_OP:

        /* ConcatenateResTemplate (Buffer, Buffer, Result) (ACPI 2.0) */

        Status = AcpiExConcatTemplate (
            Operand[0], Operand[1], &ReturnDesc, WalkState);
        break;

    case AML_INDEX_OP:              /* Index (Source Index Result) */

        /* Create the internal return object */

        ReturnDesc = AcpiUtCreateInternalObject (ACPI_TYPE_LOCAL_REFERENCE);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        /* Initialize the Index reference object */

        Index = Operand[1]->Integer.Value;
        ReturnDesc->Reference.Value = (UINT32) Index;
        ReturnDesc->Reference.Class = ACPI_REFCLASS_INDEX;

        /*
         * At this point, the Source operand is a String, Buffer, or Package.
         * Verify that the index is within range.
         */
        switch ((Operand[0])->Common.Type)
        {
        case ACPI_TYPE_STRING:

            if (Index >= Operand[0]->String.Length)
            {
                Length = Operand[0]->String.Length;
                Status = AE_AML_STRING_LIMIT;
            }

            ReturnDesc->Reference.TargetType = ACPI_TYPE_BUFFER_FIELD;
            ReturnDesc->Reference.IndexPointer =
                &(Operand[0]->Buffer.Pointer [Index]);
            break;

        case ACPI_TYPE_BUFFER:

            if (Index >= Operand[0]->Buffer.Length)
            {
                Length = Operand[0]->Buffer.Length;
                Status = AE_AML_BUFFER_LIMIT;
            }

            ReturnDesc->Reference.TargetType = ACPI_TYPE_BUFFER_FIELD;
            ReturnDesc->Reference.IndexPointer =
                &(Operand[0]->Buffer.Pointer [Index]);
            break;

        case ACPI_TYPE_PACKAGE:

            if (Index >= Operand[0]->Package.Count)
            {
                Length = Operand[0]->Package.Count;
                Status = AE_AML_PACKAGE_LIMIT;
            }

            ReturnDesc->Reference.TargetType = ACPI_TYPE_PACKAGE;
            ReturnDesc->Reference.Where =
                &Operand[0]->Package.Elements [Index];
            break;

        default:

            Status = AE_AML_INTERNAL;
            goto Cleanup;
        }

        /* Failure means that the Index was beyond the end of the object */

        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Index (0x%X%8.8X) is beyond end of object (length 0x%X)",
                ACPI_FORMAT_UINT64 (Index), (UINT32) Length));
            goto Cleanup;
        }

        /*
         * Save the target object and add a reference to it for the life
         * of the index
         */
        ReturnDesc->Reference.Object = Operand[0];
        AcpiUtAddReference (Operand[0]);

        /* Store the reference to the Target */

        Status = AcpiExStore (ReturnDesc, Operand[2], WalkState);

        /* Return the reference */

        WalkState->ResultObj = ReturnDesc;
        goto Cleanup;

    default:

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
        break;
    }


StoreResultToTarget:

    if (ACPI_SUCCESS (Status))
    {
        /*
         * Store the result of the operation (which is now in ReturnDesc) into
         * the Target descriptor.
         */
        Status = AcpiExStore (ReturnDesc, Operand[2], WalkState);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        if (!WalkState->ResultObj)
        {
            WalkState->ResultObj = ReturnDesc;
        }
    }


Cleanup:

    /* Delete return object on error */

    if (ACPI_FAILURE (Status))
    {
        AcpiUtRemoveReference (ReturnDesc);
        WalkState->ResultObj = NULL;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_2A_0T_1R
 *
 * PARAMETERS:  WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with 2 arguments, no target, and a return value
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_2A_0T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *ReturnDesc = NULL;
    ACPI_STATUS             Status = AE_OK;
    BOOLEAN                 LogicalResult = FALSE;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_2A_0T_1R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Create the internal return object */

    ReturnDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
    if (!ReturnDesc)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Execute the Opcode */

    if (WalkState->OpInfo->Flags & AML_LOGICAL_NUMERIC)
    {
        /* LogicalOp  (Operand0, Operand1) */

        Status = AcpiExDoLogicalNumericOp (WalkState->Opcode,
            Operand[0]->Integer.Value, Operand[1]->Integer.Value,
            &LogicalResult);
        goto StoreLogicalResult;
    }
    else if (WalkState->OpInfo->Flags & AML_LOGICAL)
    {
        /* LogicalOp  (Operand0, Operand1) */

        Status = AcpiExDoLogicalOp (WalkState->Opcode, Operand[0],
            Operand[1], &LogicalResult);
        goto StoreLogicalResult;
    }

    switch (WalkState->Opcode)
    {
    case AML_ACQUIRE_OP:            /* Acquire (MutexObject, Timeout) */

        Status = AcpiExAcquireMutex (Operand[1], Operand[0], WalkState);
        if (Status == AE_TIME)
        {
            LogicalResult = TRUE;       /* TRUE = Acquire timed out */
            Status = AE_OK;
        }
        break;


    case AML_WAIT_OP:               /* Wait (EventObject, Timeout) */

        Status = AcpiExSystemWaitEvent (Operand[1], Operand[0]);
        if (Status == AE_TIME)
        {
            LogicalResult = TRUE;       /* TRUE, Wait timed out */
            Status = AE_OK;
        }
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));

        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }


StoreLogicalResult:
    /*
     * Set return value to according to LogicalResult. logical TRUE (all ones)
     * Default is FALSE (zero)
     */
    if (LogicalResult)
    {
        ReturnDesc->Integer.Value = ACPI_UINT64_MAX;
    }

Cleanup:

    /* Delete return object on error */

    if (ACPI_FAILURE (Status))
    {
        AcpiUtRemoveReference (ReturnDesc);
    }

    /* Save return object on success */

    else
    {
        WalkState->ResultObj = ReturnDesc;
    }

    return_ACPI_STATUS (Status);
}
