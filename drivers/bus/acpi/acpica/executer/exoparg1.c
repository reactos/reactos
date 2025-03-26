/******************************************************************************
 *
 * Module Name: exoparg1 - AML execution - opcodes with 1 argument
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2022, Intel Corp.
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
#include "acparser.h"
#include "acdispat.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exoparg1")


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
 *                    required for this opcode type (0 through 6 args).
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
 * FUNCTION:    AcpiExOpcode_0A_0T_1R
 *
 * PARAMETERS:  WalkState           - Current state (contains AML opcode)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute operator with no operands, one return value
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_0A_0T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_OPERAND_OBJECT     *ReturnDesc = NULL;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_0A_0T_1R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Examine the AML opcode */

    switch (WalkState->Opcode)
    {
    case AML_TIMER_OP:      /*  Timer () */

        /* Create a return object of type Integer */

        ReturnDesc = AcpiUtCreateIntegerObject (AcpiOsGetTimer ());
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }
        break;

    default:                /*  Unknown opcode  */

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
        break;
    }

Cleanup:

    /* Delete return object on error */

    if ((ACPI_FAILURE (Status)) || WalkState->ResultObj)
    {
        AcpiUtRemoveReference (ReturnDesc);
        WalkState->ResultObj = NULL;
    }
    else
    {
        /* Save the return value */

        WalkState->ResultObj = ReturnDesc;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_1A_0T_0R
 *
 * PARAMETERS:  WalkState           - Current state (contains AML opcode)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Type 1 monadic operator with numeric operand on
 *              object stack
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_1A_0T_0R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_1A_0T_0R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Examine the AML opcode */

    switch (WalkState->Opcode)
    {
    case AML_RELEASE_OP:    /*  Release (MutexObject) */

        Status = AcpiExReleaseMutex (Operand[0], WalkState);
        break;

    case AML_RESET_OP:      /*  Reset (EventObject) */

        Status = AcpiExSystemResetEvent (Operand[0]);
        break;

    case AML_SIGNAL_OP:     /*  Signal (EventObject) */

        Status = AcpiExSystemSignalEvent (Operand[0]);
        break;

    case AML_SLEEP_OP:      /*  Sleep (MsecTime) */

        Status = AcpiExSystemDoSleep (Operand[0]->Integer.Value);
        break;

    case AML_STALL_OP:      /*  Stall (UsecTime) */

        Status = AcpiExSystemDoStall ((UINT32) Operand[0]->Integer.Value);
        break;

    case AML_UNLOAD_OP:     /*  Unload (Handle) */

        Status = AcpiExUnloadTable (Operand[0]);
        break;

    default:                /*  Unknown opcode  */

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
        break;
    }

    return_ACPI_STATUS (Status);
}


#ifdef _OBSOLETE_CODE /* Was originally used for Load() operator */
/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_1A_1T_0R
 *
 * PARAMETERS:  WalkState           - Current state (contains AML opcode)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with one argument, one target, and no
 *              return value.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_1A_1T_0R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];


    ACPI_FUNCTION_TRACE_STR (ExOpcode_1A_1T_0R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Examine the AML opcode */

    switch (WalkState->Opcode)
    {
#ifdef _OBSOLETE_CODE
    case AML_LOAD_OP:

        Status = AcpiExLoadOp (Operand[0], Operand[1], WalkState);
        break;
#endif

    default:                        /* Unknown opcode */

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }


Cleanup:

    return_ACPI_STATUS (Status);
}
#endif

/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_1A_1T_1R
 *
 * PARAMETERS:  WalkState           - Current state (contains AML opcode)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with one argument, one target, and a
 *              return value.
 *              January 2022: Added Load operator, with new ACPI 6.4
 *              semantics.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_1A_1T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *ReturnDesc = NULL;
    ACPI_OPERAND_OBJECT     *ReturnDesc2 = NULL;
    UINT32                  Temp32;
    UINT32                  i;
    UINT64                  PowerOfTen;
    UINT64                  Digit;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_1A_1T_1R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Examine the AML opcode */

    switch (WalkState->Opcode)
    {
    case AML_BIT_NOT_OP:
    case AML_FIND_SET_LEFT_BIT_OP:
    case AML_FIND_SET_RIGHT_BIT_OP:
    case AML_FROM_BCD_OP:
    case AML_LOAD_OP:
    case AML_TO_BCD_OP:
    case AML_CONDITIONAL_REF_OF_OP:

        /* Create a return object of type Integer for these opcodes */

        ReturnDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        switch (WalkState->Opcode)
        {
        case AML_BIT_NOT_OP:            /* Not (Operand, Result)  */

            ReturnDesc->Integer.Value = ~Operand[0]->Integer.Value;
            break;

        case AML_FIND_SET_LEFT_BIT_OP:  /* FindSetLeftBit (Operand, Result) */

            ReturnDesc->Integer.Value = Operand[0]->Integer.Value;

            /*
             * Acpi specification describes Integer type as a little
             * endian unsigned value, so this boundary condition is valid.
             */
            for (Temp32 = 0; ReturnDesc->Integer.Value &&
                    Temp32 < ACPI_INTEGER_BIT_SIZE; ++Temp32)
            {
                ReturnDesc->Integer.Value >>= 1;
            }

            ReturnDesc->Integer.Value = Temp32;
            break;

        case AML_FIND_SET_RIGHT_BIT_OP: /* FindSetRightBit (Operand, Result) */

            ReturnDesc->Integer.Value = Operand[0]->Integer.Value;

            /*
             * The Acpi specification describes Integer type as a little
             * endian unsigned value, so this boundary condition is valid.
             */
            for (Temp32 = 0; ReturnDesc->Integer.Value &&
                     Temp32 < ACPI_INTEGER_BIT_SIZE; ++Temp32)
            {
                ReturnDesc->Integer.Value <<= 1;
            }

            /* Since the bit position is one-based, subtract from 33 (65) */

            ReturnDesc->Integer.Value =
                Temp32 == 0 ? 0 : (ACPI_INTEGER_BIT_SIZE + 1) - Temp32;
            break;

        case AML_FROM_BCD_OP:           /* FromBcd (BCDValue, Result)  */
            /*
             * The 64-bit ACPI integer can hold 16 4-bit BCD characters
             * (if table is 32-bit, integer can hold 8 BCD characters)
             * Convert each 4-bit BCD value
             */
            PowerOfTen = 1;
            ReturnDesc->Integer.Value = 0;
            Digit = Operand[0]->Integer.Value;

            /* Convert each BCD digit (each is one nybble wide) */

            for (i = 0; (i < AcpiGbl_IntegerNybbleWidth) && (Digit > 0); i++)
            {
                /* Get the least significant 4-bit BCD digit */

                Temp32 = ((UINT32) Digit) & 0xF;

                /* Check the range of the digit */

                if (Temp32 > 9)
                {
                    ACPI_ERROR ((AE_INFO,
                        "BCD digit too large (not decimal): 0x%X",
                        Temp32));

                    Status = AE_AML_NUMERIC_OVERFLOW;
                    goto Cleanup;
                }

                /* Sum the digit into the result with the current power of 10 */

                ReturnDesc->Integer.Value +=
                    (((UINT64) Temp32) * PowerOfTen);

                /* Shift to next BCD digit */

                Digit >>= 4;

                /* Next power of 10 */

                PowerOfTen *= 10;
            }
            break;

        case AML_LOAD_OP:               /* Result1 = Load (Operand[0], Result1) */

            ReturnDesc->Integer.Value = 0;
            Status = AcpiExLoadOp (Operand[0], ReturnDesc, WalkState);
            if (ACPI_SUCCESS (Status))
            {
                /* Return -1 (non-zero) indicates success */

                ReturnDesc->Integer.Value = 0xFFFFFFFFFFFFFFFF;
            }
            break;

        case AML_TO_BCD_OP:             /* ToBcd (Operand, Result)  */

            ReturnDesc->Integer.Value = 0;
            Digit = Operand[0]->Integer.Value;

            /* Each BCD digit is one nybble wide */

            for (i = 0; (i < AcpiGbl_IntegerNybbleWidth) && (Digit > 0); i++)
            {
                (void) AcpiUtShortDivide (Digit, 10, &Digit, &Temp32);

                /*
                 * Insert the BCD digit that resides in the
                 * remainder from above
                 */
                ReturnDesc->Integer.Value |=
                    (((UINT64) Temp32) << ACPI_MUL_4 (i));
            }

            /* Overflow if there is any data left in Digit */

            if (Digit > 0)
            {
                ACPI_ERROR ((AE_INFO,
                    "Integer too large to convert to BCD: 0x%8.8X%8.8X",
                    ACPI_FORMAT_UINT64 (Operand[0]->Integer.Value)));
                Status = AE_AML_NUMERIC_OVERFLOW;
                goto Cleanup;
            }
            break;

        case AML_CONDITIONAL_REF_OF_OP:     /* CondRefOf (SourceObject, Result)  */
            /*
             * This op is a little strange because the internal return value is
             * different than the return value stored in the result descriptor
             * (There are really two return values)
             */
            if ((ACPI_NAMESPACE_NODE *) Operand[0] == AcpiGbl_RootNode)
            {
                /*
                 * This means that the object does not exist in the namespace,
                 * return FALSE
                 */
                ReturnDesc->Integer.Value = 0;
                goto Cleanup;
            }

            /* Get the object reference, store it, and remove our reference */

            Status = AcpiExGetObjectReference (Operand[0],
                &ReturnDesc2, WalkState);
            if (ACPI_FAILURE (Status))
            {
                goto Cleanup;
            }

            Status = AcpiExStore (ReturnDesc2, Operand[1], WalkState);
            AcpiUtRemoveReference (ReturnDesc2);

            /* The object exists in the namespace, return TRUE */

            ReturnDesc->Integer.Value = ACPI_UINT64_MAX;
            goto Cleanup;


        default:

            /* No other opcodes get here */

            break;
        }
        break;

    case AML_STORE_OP:              /* Store (Source, Target) */
        /*
         * A store operand is typically a number, string, buffer or lvalue
         * Be careful about deleting the source object,
         * since the object itself may have been stored.
         */
        Status = AcpiExStore (Operand[0], Operand[1], WalkState);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        /* It is possible that the Store already produced a return object */

        if (!WalkState->ResultObj)
        {
            /*
             * Normally, we would remove a reference on the Operand[0]
             * parameter; But since it is being used as the internal return
             * object (meaning we would normally increment it), the two
             * cancel out, and we simply don't do anything.
             */
            WalkState->ResultObj = Operand[0];
            WalkState->Operands[0] = NULL;  /* Prevent deletion */
        }
        return_ACPI_STATUS (Status);

    /*
     * ACPI 2.0 Opcodes
     */
    case AML_COPY_OBJECT_OP:        /* CopyObject (Source, Target) */

        Status = AcpiUtCopyIobjectToIobject (
            Operand[0], &ReturnDesc, WalkState);
        break;

    case AML_TO_DECIMAL_STRING_OP:  /* ToDecimalString (Data, Result) */

        Status = AcpiExConvertToString (
            Operand[0], &ReturnDesc, ACPI_EXPLICIT_CONVERT_DECIMAL);
        if (ReturnDesc == Operand[0])
        {
            /* No conversion performed, add ref to handle return value */

            AcpiUtAddReference (ReturnDesc);
        }
        break;

    case AML_TO_HEX_STRING_OP:      /* ToHexString (Data, Result) */

        Status = AcpiExConvertToString (
            Operand[0], &ReturnDesc, ACPI_EXPLICIT_CONVERT_HEX);
        if (ReturnDesc == Operand[0])
        {
            /* No conversion performed, add ref to handle return value */

            AcpiUtAddReference (ReturnDesc);
        }
        break;

    case AML_TO_BUFFER_OP:          /* ToBuffer (Data, Result) */

        Status = AcpiExConvertToBuffer (Operand[0], &ReturnDesc);
        if (ReturnDesc == Operand[0])
        {
            /* No conversion performed, add ref to handle return value */

            AcpiUtAddReference (ReturnDesc);
        }
        break;

    case AML_TO_INTEGER_OP:         /* ToInteger (Data, Result) */

        /* Perform "explicit" conversion */

        Status = AcpiExConvertToInteger (Operand[0], &ReturnDesc, 0);
        if (ReturnDesc == Operand[0])
        {
            /* No conversion performed, add ref to handle return value */

            AcpiUtAddReference (ReturnDesc);
        }
        break;

    case AML_SHIFT_LEFT_BIT_OP:     /* ShiftLeftBit (Source, BitNum)  */
    case AML_SHIFT_RIGHT_BIT_OP:    /* ShiftRightBit (Source, BitNum) */

        /* These are two obsolete opcodes */

        ACPI_ERROR ((AE_INFO,
            "%s is obsolete and not implemented",
            AcpiPsGetOpcodeName (WalkState->Opcode)));
        Status = AE_SUPPORT;
        goto Cleanup;

    default:                        /* Unknown opcode */

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }

    if (ACPI_SUCCESS (Status))
    {
        /* Store the return value computed above into the target object */

        Status = AcpiExStore (ReturnDesc, Operand[1], WalkState);
    }


Cleanup:

    /* Delete return object on error */

    if (ACPI_FAILURE (Status))
    {
        AcpiUtRemoveReference (ReturnDesc);
    }

    /* Save return object on success */

    else if (!WalkState->ResultObj)
    {
        WalkState->ResultObj = ReturnDesc;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_1A_0T_1R
 *
 * PARAMETERS:  WalkState           - Current state (contains AML opcode)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with one argument, no target, and a return value
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_1A_0T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *TempDesc;
    ACPI_OPERAND_OBJECT     *ReturnDesc = NULL;
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Type;
    UINT64                  Value;


    ACPI_FUNCTION_TRACE_STR (ExOpcode_1A_0T_1R,
        AcpiPsGetOpcodeName (WalkState->Opcode));


    /* Examine the AML opcode */

    switch (WalkState->Opcode)
    {
    case AML_LOGICAL_NOT_OP:        /* LNot (Operand) */

        ReturnDesc = AcpiUtCreateIntegerObject ((UINT64) 0);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        /*
         * Set result to ONES (TRUE) if Value == 0. Note:
         * ReturnDesc->Integer.Value is initially == 0 (FALSE) from above.
         */
        if (!Operand[0]->Integer.Value)
        {
            ReturnDesc->Integer.Value = ACPI_UINT64_MAX;
        }
        break;

    case AML_DECREMENT_OP:          /* Decrement (Operand)  */
    case AML_INCREMENT_OP:          /* Increment (Operand)  */
        /*
         * Create a new integer. Can't just get the base integer and
         * increment it because it may be an Arg or Field.
         */
        ReturnDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        /*
         * Since we are expecting a Reference operand, it can be either a
         * NS Node or an internal object.
         */
        TempDesc = Operand[0];
        if (ACPI_GET_DESCRIPTOR_TYPE (TempDesc) == ACPI_DESC_TYPE_OPERAND)
        {
            /* Internal reference object - prevent deletion */

            AcpiUtAddReference (TempDesc);
        }

        /*
         * Convert the Reference operand to an Integer (This removes a
         * reference on the Operand[0] object)
         *
         * NOTE:  We use LNOT_OP here in order to force resolution of the
         * reference operand to an actual integer.
         */
        Status = AcpiExResolveOperands (AML_LOGICAL_NOT_OP,
            &TempDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "While resolving operands for [%s]",
                AcpiPsGetOpcodeName (WalkState->Opcode)));

            goto Cleanup;
        }

        /*
         * TempDesc is now guaranteed to be an Integer object --
         * Perform the actual increment or decrement
         */
        if (WalkState->Opcode == AML_INCREMENT_OP)
        {
            ReturnDesc->Integer.Value = TempDesc->Integer.Value + 1;
        }
        else
        {
            ReturnDesc->Integer.Value = TempDesc->Integer.Value - 1;
        }

        /* Finished with this Integer object */

        AcpiUtRemoveReference (TempDesc);

        /*
         * Store the result back (indirectly) through the original
         * Reference object
         */
        Status = AcpiExStore (ReturnDesc, Operand[0], WalkState);
        break;

    case AML_OBJECT_TYPE_OP:            /* ObjectType (SourceObject) */
        /*
         * Note: The operand is not resolved at this point because we want to
         * get the associated object, not its value. For example, we don't
         * want to resolve a FieldUnit to its value, we want the actual
         * FieldUnit object.
         */

        /* Get the type of the base object */

        Status = AcpiExResolveMultiple (WalkState, Operand[0], &Type, NULL);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        /* Allocate a descriptor to hold the type. */

        ReturnDesc = AcpiUtCreateIntegerObject ((UINT64) Type);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }
        break;

    case AML_SIZE_OF_OP:            /* SizeOf (SourceObject)  */
        /*
         * Note: The operand is not resolved at this point because we want to
         * get the associated object, not its value.
         */

        /* Get the base object */

        Status = AcpiExResolveMultiple (
            WalkState, Operand[0], &Type, &TempDesc);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        /*
         * The type of the base object must be integer, buffer, string, or
         * package. All others are not supported.
         *
         * NOTE: Integer is not specifically supported by the ACPI spec,
         * but is supported implicitly via implicit operand conversion.
         * rather than bother with conversion, we just use the byte width
         * global (4 or 8 bytes).
         */
        switch (Type)
        {
        case ACPI_TYPE_INTEGER:

            Value = AcpiGbl_IntegerByteWidth;
            break;

        case ACPI_TYPE_STRING:

            Value = TempDesc->String.Length;
            break;

        case ACPI_TYPE_BUFFER:

            /* Buffer arguments may not be evaluated at this point */

            Status = AcpiDsGetBufferArguments (TempDesc);
            Value = TempDesc->Buffer.Length;
            break;

        case ACPI_TYPE_PACKAGE:

            /* Package arguments may not be evaluated at this point */

            Status = AcpiDsGetPackageArguments (TempDesc);
            Value = TempDesc->Package.Count;
            break;

        default:

            ACPI_ERROR ((AE_INFO,
                "Operand must be Buffer/Integer/String/Package"
                " - found type %s",
                AcpiUtGetTypeName (Type)));

            Status = AE_AML_OPERAND_TYPE;
            goto Cleanup;
        }

        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        /*
         * Now that we have the size of the object, create a result
         * object to hold the value
         */
        ReturnDesc = AcpiUtCreateIntegerObject (Value);
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }
        break;


    case AML_REF_OF_OP:             /* RefOf (SourceObject) */

        Status = AcpiExGetObjectReference (
            Operand[0], &ReturnDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }
        break;


    case AML_DEREF_OF_OP:           /* DerefOf (ObjReference | String) */

        /* Check for a method local or argument, or standalone String */

        if (ACPI_GET_DESCRIPTOR_TYPE (Operand[0]) == ACPI_DESC_TYPE_NAMED)
        {
            TempDesc = AcpiNsGetAttachedObject (
                (ACPI_NAMESPACE_NODE *) Operand[0]);
            if (TempDesc &&
                 ((TempDesc->Common.Type == ACPI_TYPE_STRING) ||
                  (TempDesc->Common.Type == ACPI_TYPE_LOCAL_REFERENCE)))
            {
                Operand[0] = TempDesc;
                AcpiUtAddReference (TempDesc);
            }
            else
            {
                Status = AE_AML_OPERAND_TYPE;
                goto Cleanup;
            }
        }
        else
        {
            switch ((Operand[0])->Common.Type)
            {
            case ACPI_TYPE_LOCAL_REFERENCE:
                /*
                 * This is a DerefOf (LocalX | ArgX)
                 *
                 * Must resolve/dereference the local/arg reference first
                 */
                switch (Operand[0]->Reference.Class)
                {
                case ACPI_REFCLASS_LOCAL:
                case ACPI_REFCLASS_ARG:

                    /* Set Operand[0] to the value of the local/arg */

                    Status = AcpiDsMethodDataGetValue (
                        Operand[0]->Reference.Class,
                        Operand[0]->Reference.Value,
                        WalkState, &TempDesc);
                    if (ACPI_FAILURE (Status))
                    {
                        goto Cleanup;
                    }

                    /*
                     * Delete our reference to the input object and
                     * point to the object just retrieved
                     */
                    AcpiUtRemoveReference (Operand[0]);
                    Operand[0] = TempDesc;
                    break;

                case ACPI_REFCLASS_REFOF:

                    /* Get the object to which the reference refers */

                    TempDesc = Operand[0]->Reference.Object;
                    AcpiUtRemoveReference (Operand[0]);
                    Operand[0] = TempDesc;
                    break;

                default:

                    /* Must be an Index op - handled below */
                    break;
                }
                break;

            case ACPI_TYPE_STRING:

                break;

            default:

                Status = AE_AML_OPERAND_TYPE;
                goto Cleanup;
            }
        }

        if (ACPI_GET_DESCRIPTOR_TYPE (Operand[0]) != ACPI_DESC_TYPE_NAMED)
        {
            if ((Operand[0])->Common.Type == ACPI_TYPE_STRING)
            {
                /*
                 * This is a DerefOf (String). The string is a reference
                 * to a named ACPI object.
                 *
                 * 1) Find the owning Node
                 * 2) Dereference the node to an actual object. Could be a
                 *    Field, so we need to resolve the node to a value.
                 */
                Status = AcpiNsGetNodeUnlocked (WalkState->ScopeInfo->Scope.Node,
                    Operand[0]->String.Pointer,
                    ACPI_NS_SEARCH_PARENT,
                    ACPI_CAST_INDIRECT_PTR (
                        ACPI_NAMESPACE_NODE, &ReturnDesc));
                if (ACPI_FAILURE (Status))
                {
                    goto Cleanup;
                }

                Status = AcpiExResolveNodeToValue (
                    ACPI_CAST_INDIRECT_PTR (
                        ACPI_NAMESPACE_NODE, &ReturnDesc),
                    WalkState);
                goto Cleanup;
            }
        }

        /* Operand[0] may have changed from the code above */

        if (ACPI_GET_DESCRIPTOR_TYPE (Operand[0]) == ACPI_DESC_TYPE_NAMED)
        {
            /*
             * This is a DerefOf (ObjectReference)
             * Get the actual object from the Node (This is the dereference).
             * This case may only happen when a LocalX or ArgX is
             * dereferenced above, or for references to device and
             * thermal objects.
             */
            switch (((ACPI_NAMESPACE_NODE *) Operand[0])->Type)
            {
            case ACPI_TYPE_DEVICE:
            case ACPI_TYPE_THERMAL:

                /* These types have no node subobject, return the NS node */

                ReturnDesc = Operand[0];
                break;

            default:
                /* For most types, get the object attached to the node */

                ReturnDesc = AcpiNsGetAttachedObject (
                    (ACPI_NAMESPACE_NODE *) Operand[0]);
                AcpiUtAddReference (ReturnDesc);
                break;
            }
        }
        else
        {
            /*
             * This must be a reference object produced by either the
             * Index() or RefOf() operator
             */
            switch (Operand[0]->Reference.Class)
            {
            case ACPI_REFCLASS_INDEX:
                /*
                 * The target type for the Index operator must be
                 * either a Buffer or a Package
                 */
                switch (Operand[0]->Reference.TargetType)
                {
                case ACPI_TYPE_BUFFER_FIELD:

                    TempDesc = Operand[0]->Reference.Object;

                    /*
                     * Create a new object that contains one element of the
                     * buffer -- the element pointed to by the index.
                     *
                     * NOTE: index into a buffer is NOT a pointer to a
                     * sub-buffer of the main buffer, it is only a pointer to a
                     * single element (byte) of the buffer!
                     *
                     * Since we are returning the value of the buffer at the
                     * indexed location, we don't need to add an additional
                     * reference to the buffer itself.
                     */
                    ReturnDesc = AcpiUtCreateIntegerObject ((UINT64)
                        TempDesc->Buffer.Pointer[Operand[0]->Reference.Value]);
                    if (!ReturnDesc)
                    {
                        Status = AE_NO_MEMORY;
                        goto Cleanup;
                    }
                    break;

                case ACPI_TYPE_PACKAGE:
                    /*
                     * Return the referenced element of the package. We must
                     * add another reference to the referenced object, however.
                     */
                    ReturnDesc = *(Operand[0]->Reference.Where);
                    if (!ReturnDesc)
                    {
                        /*
                         * Element is NULL, do not allow the dereference.
                         * This provides compatibility with other ACPI
                         * implementations.
                         */
                        return_ACPI_STATUS (AE_AML_UNINITIALIZED_ELEMENT);
                    }

                    AcpiUtAddReference (ReturnDesc);
                    break;

                default:

                    ACPI_ERROR ((AE_INFO,
                        "Unknown Index TargetType 0x%X in reference object %p",
                        Operand[0]->Reference.TargetType, Operand[0]));

                    Status = AE_AML_OPERAND_TYPE;
                    goto Cleanup;
                }
                break;

            case ACPI_REFCLASS_REFOF:

                ReturnDesc = Operand[0]->Reference.Object;

                if (ACPI_GET_DESCRIPTOR_TYPE (ReturnDesc) ==
                    ACPI_DESC_TYPE_NAMED)
                {
                    ReturnDesc = AcpiNsGetAttachedObject (
                        (ACPI_NAMESPACE_NODE *) ReturnDesc);
                    if (!ReturnDesc)
                    {
                        break;
                    }

                   /*
                    * June 2013:
                    * BufferFields/FieldUnits require additional resolution
                    */
                    switch (ReturnDesc->Common.Type)
                    {
                    case ACPI_TYPE_BUFFER_FIELD:
                    case ACPI_TYPE_LOCAL_REGION_FIELD:
                    case ACPI_TYPE_LOCAL_BANK_FIELD:
                    case ACPI_TYPE_LOCAL_INDEX_FIELD:

                        Status = AcpiExReadDataFromField (
                            WalkState, ReturnDesc, &TempDesc);
                        if (ACPI_FAILURE (Status))
                        {
                            return_ACPI_STATUS (Status);
                        }

                        ReturnDesc = TempDesc;
                        break;

                    default:

                        /* Add another reference to the object */

                        AcpiUtAddReference (ReturnDesc);
                        break;
                    }
                }
                break;

            default:

                ACPI_ERROR ((AE_INFO,
                    "Unknown class in reference(%p) - 0x%2.2X",
                    Operand[0], Operand[0]->Reference.Class));

                Status = AE_TYPE;
                goto Cleanup;
            }
        }
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            WalkState->Opcode));

        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
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
