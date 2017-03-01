/******************************************************************************
 *
 * Module Name: exoparg3 - AML execution - opcodes with 3 arguments
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
#include "acparser.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exoparg3")


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
 * FUNCTION:    AcpiExOpcode_3A_0T_0R
 *
 * PARAMETERS:  WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Triadic operator (3 operands)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_3A_0T_0R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_SIGNAL_FATAL_INFO  *Fatal;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_3A_0T_0R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    switch (WalkState->Opcode)
    {
    case AML_FATAL_OP:          /* Fatal (FatalType  FatalCode  FatalArg) */

        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "FatalOp: Type %X Code %X Arg %X "
            "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n",
            (UINT32) Operand[0]->Integer.Value,
            (UINT32) Operand[1]->Integer.Value,
            (UINT32) Operand[2]->Integer.Value));

        Fatal = ACPI_ALLOCATE (sizeof (ACPI_SIGNAL_FATAL_INFO));
        if (Fatal)
        {
            Fatal->Type = (UINT32) Operand[0]->Integer.Value;
            Fatal->Code = (UINT32) Operand[1]->Integer.Value;
            Fatal->Argument = (UINT32) Operand[2]->Integer.Value;
        }

        /* Always signal the OS! */

        Status = AcpiOsSignal (ACPI_SIGNAL_FATAL, Fatal);

        /* Might return while OS is shutting down, just continue */

        ACPI_FREE (Fatal);
        goto Cleanup;

    case AML_EXTERNAL_OP:
        /*
         * If the interpreter sees this opcode, just ignore it. The External
         * op is intended for use by disassemblers in order to properly
         * disassemble control method invocations. The opcode or group of
         * opcodes should be surrounded by an "if (0)" clause to ensure that
         * AML interpreters never see the opcode. Thus, something is
         * wrong if an external opcode ever gets here.
         */
        ACPI_ERROR ((AE_INFO, "Executed External Op"));
        Status = AE_OK;
        goto Cleanup;

    default:

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));

        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }


Cleanup:

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_3A_1T_1R
 *
 * PARAMETERS:  WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Triadic operator (3 operands)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_3A_1T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *ReturnDesc = NULL;
    char                    *Buffer = NULL;
    ACPI_STATUS             Status = AE_OK;
    UINT64                  Index;
    ACPI_SIZE               Length;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_3A_1T_1R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    switch (WalkState->Opcode)
    {
    case AML_MID_OP:    /* Mid (Source[0], Index[1], Length[2], Result[3]) */
        /*
         * Create the return object. The Source operand is guaranteed to be
         * either a String or a Buffer, so just use its type.
         */
        ReturnDesc = AcpiUtCreateInternalObject (
            (Operand[0])->Common.Type);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        /* Get the Integer values from the objects */

        Index = Operand[1]->Integer.Value;
        Length = (ACPI_SIZE) Operand[2]->Integer.Value;

        /*
         * If the index is beyond the length of the String/Buffer, or if the
         * requested length is zero, return a zero-length String/Buffer
         */
        if (Index >= Operand[0]->String.Length)
        {
            Length = 0;
        }

        /* Truncate request if larger than the actual String/Buffer */

        else if ((Index + Length) > Operand[0]->String.Length)
        {
            Length =
                (ACPI_SIZE) Operand[0]->String.Length - (ACPI_SIZE) Index;
        }

        /* Strings always have a sub-pointer, not so for buffers */

        switch ((Operand[0])->Common.Type)
        {
        case ACPI_TYPE_STRING:

            /* Always allocate a new buffer for the String */

            Buffer = ACPI_ALLOCATE_ZEROED ((ACPI_SIZE) Length + 1);
            if (!Buffer)
            {
                Status = AE_NO_MEMORY;
                goto Cleanup;
            }
            break;

        case ACPI_TYPE_BUFFER:

            /* If the requested length is zero, don't allocate a buffer */

            if (Length > 0)
            {
                /* Allocate a new buffer for the Buffer */

                Buffer = ACPI_ALLOCATE_ZEROED (Length);
                if (!Buffer)
                {
                    Status = AE_NO_MEMORY;
                    goto Cleanup;
                }
            }
            break;

        default:                        /* Should not happen */

            Status = AE_AML_OPERAND_TYPE;
            goto Cleanup;
        }

        if (Buffer)
        {
            /* We have a buffer, copy the portion requested */

            memcpy (Buffer,
                Operand[0]->String.Pointer + Index, Length);
        }

        /* Set the length of the new String/Buffer */

        ReturnDesc->String.Pointer = Buffer;
        ReturnDesc->String.Length = (UINT32) Length;

        /* Mark buffer initialized */

        ReturnDesc->Buffer.Flags |= AOPOBJ_DATA_VALID;
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));

        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }

    /* Store the result in the target */

    Status = AcpiExStore (ReturnDesc, Operand[3], WalkState);

Cleanup:

    /* Delete return object on error */

    if (ACPI_FAILURE (Status) || WalkState->ResultObj)
    {
        AcpiUtRemoveReference (ReturnDesc);
        WalkState->ResultObj = NULL;
    }
    else
    {
        /* Set the return object and exit */

        WalkState->ResultObj = ReturnDesc;
    }

    return_ACPI_STATUS (Status);
}
