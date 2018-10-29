/******************************************************************************
 *
 * Module Name: exutils - interpreter/scanner utilities
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
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

/*
 * DEFINE_AML_GLOBALS is tested in amlcode.h
 * to determine whether certain global names should be "defined" or only
 * "declared" in the current compilation. This enhances maintainability
 * by enabling a single header file to embody all knowledge of the names
 * in question.
 *
 * Exactly one module of any executable should #define DEFINE_GLOBALS
 * before #including the header files which use this convention. The
 * names in question will be defined and initialized in that module,
 * and declared as extern in all other modules which #include those
 * header files.
 */

#define DEFINE_AML_GLOBALS

#include "acpi.h"
#include "accommon.h"
#include "acinterp.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exutils")

/* Local prototypes */

static UINT32
AcpiExDigitsNeeded (
    UINT64                  Value,
    UINT32                  Base);


#ifndef ACPI_NO_METHOD_EXECUTION
/*******************************************************************************
 *
 * FUNCTION:    AcpiExEnterInterpreter
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Enter the interpreter execution region. Failure to enter
 *              the interpreter region is a fatal system error. Used in
 *              conjunction with ExitInterpreter.
 *
 ******************************************************************************/

void
AcpiExEnterInterpreter (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (ExEnterInterpreter);


    Status = AcpiUtAcquireMutex (ACPI_MTX_INTERPRETER);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO, "Could not acquire AML Interpreter mutex"));
    }
    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO, "Could not acquire AML Namespace mutex"));
    }

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExExitInterpreter
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Exit the interpreter execution region. This is the top level
 *              routine used to exit the interpreter when all processing has
 *              been completed, or when the method blocks.
 *
 * Cases where the interpreter is unlocked internally:
 *      1) Method will be blocked on a Sleep() AML opcode
 *      2) Method will be blocked on an Acquire() AML opcode
 *      3) Method will be blocked on a Wait() AML opcode
 *      4) Method will be blocked to acquire the global lock
 *      5) Method will be blocked waiting to execute a serialized control
 *          method that is currently executing
 *      6) About to invoke a user-installed opregion handler
 *
 ******************************************************************************/

void
AcpiExExitInterpreter (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (ExExitInterpreter);


    Status = AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO, "Could not release AML Namespace mutex"));
    }
    Status = AcpiUtReleaseMutex (ACPI_MTX_INTERPRETER);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO, "Could not release AML Interpreter mutex"));
    }

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExTruncateFor32bitTable
 *
 * PARAMETERS:  ObjDesc         - Object to be truncated
 *
 * RETURN:      TRUE if a truncation was performed, FALSE otherwise.
 *
 * DESCRIPTION: Truncate an ACPI Integer to 32 bits if the execution mode is
 *              32-bit, as determined by the revision of the DSDT.
 *
 ******************************************************************************/

BOOLEAN
AcpiExTruncateFor32bitTable (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{

    ACPI_FUNCTION_ENTRY ();


    /*
     * Object must be a valid number and we must be executing
     * a control method. Object could be NS node for AML_INT_NAMEPATH_OP.
     */
    if ((!ObjDesc) ||
        (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) != ACPI_DESC_TYPE_OPERAND) ||
        (ObjDesc->Common.Type != ACPI_TYPE_INTEGER))
    {
        return (FALSE);
    }

    if ((AcpiGbl_IntegerByteWidth == 4) &&
        (ObjDesc->Integer.Value > (UINT64) ACPI_UINT32_MAX))
    {
        /*
         * We are executing in a 32-bit ACPI table. Truncate
         * the value to 32 bits by zeroing out the upper 32-bit field
         */
        ObjDesc->Integer.Value &= (UINT64) ACPI_UINT32_MAX;
        return (TRUE);
    }

    return (FALSE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExAcquireGlobalLock
 *
 * PARAMETERS:  FieldFlags            - Flags with Lock rule:
 *                                      AlwaysLock or NeverLock
 *
 * RETURN:      None
 *
 * DESCRIPTION: Obtain the ACPI hardware Global Lock, only if the field
 *              flags specifiy that it is to be obtained before field access.
 *
 ******************************************************************************/

void
AcpiExAcquireGlobalLock (
    UINT32                  FieldFlags)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (ExAcquireGlobalLock);


    /* Only use the lock if the AlwaysLock bit is set */

    if (!(FieldFlags & AML_FIELD_LOCK_RULE_MASK))
    {
        return_VOID;
    }

    /* Attempt to get the global lock, wait forever */

    Status = AcpiExAcquireMutexObject (ACPI_WAIT_FOREVER,
        AcpiGbl_GlobalLockMutex, AcpiOsGetThreadId ());

    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not acquire Global Lock"));
    }

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReleaseGlobalLock
 *
 * PARAMETERS:  FieldFlags            - Flags with Lock rule:
 *                                      AlwaysLock or NeverLock
 *
 * RETURN:      None
 *
 * DESCRIPTION: Release the ACPI hardware Global Lock
 *
 ******************************************************************************/

void
AcpiExReleaseGlobalLock (
    UINT32                  FieldFlags)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (ExReleaseGlobalLock);


    /* Only use the lock if the AlwaysLock bit is set */

    if (!(FieldFlags & AML_FIELD_LOCK_RULE_MASK))
    {
        return_VOID;
    }

    /* Release the global lock */

    Status = AcpiExReleaseMutexObject (AcpiGbl_GlobalLockMutex);
    if (ACPI_FAILURE (Status))
    {
        /* Report the error, but there isn't much else we can do */

        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not release Global Lock"));
    }

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDigitsNeeded
 *
 * PARAMETERS:  Value           - Value to be represented
 *              Base            - Base of representation
 *
 * RETURN:      The number of digits.
 *
 * DESCRIPTION: Calculate the number of digits needed to represent the Value
 *              in the given Base (Radix)
 *
 ******************************************************************************/

static UINT32
AcpiExDigitsNeeded (
    UINT64                  Value,
    UINT32                  Base)
{
    UINT32                  NumDigits;
    UINT64                  CurrentValue;


    ACPI_FUNCTION_TRACE (ExDigitsNeeded);


    /* UINT64 is unsigned, so we don't worry about a '-' prefix */

    if (Value == 0)
    {
        return_UINT32 (1);
    }

    CurrentValue = Value;
    NumDigits = 0;

    /* Count the digits in the requested base */

    while (CurrentValue)
    {
        (void) AcpiUtShortDivide (CurrentValue, Base, &CurrentValue, NULL);
        NumDigits++;
    }

    return_UINT32 (NumDigits);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExEisaIdToString
 *
 * PARAMETERS:  OutString       - Where to put the converted string (8 bytes)
 *              CompressedId    - EISAID to be converted
 *
 * RETURN:      None
 *
 * DESCRIPTION: Convert a numeric EISAID to string representation. Return
 *              buffer must be large enough to hold the string. The string
 *              returned is always exactly of length ACPI_EISAID_STRING_SIZE
 *              (includes null terminator). The EISAID is always 32 bits.
 *
 ******************************************************************************/

void
AcpiExEisaIdToString (
    char                    *OutString,
    UINT64                  CompressedId)
{
    UINT32                  SwappedId;


    ACPI_FUNCTION_ENTRY ();


    /* The EISAID should be a 32-bit integer */

    if (CompressedId > ACPI_UINT32_MAX)
    {
        ACPI_WARNING ((AE_INFO,
            "Expected EISAID is larger than 32 bits: "
            "0x%8.8X%8.8X, truncating",
            ACPI_FORMAT_UINT64 (CompressedId)));
    }

    /* Swap ID to big-endian to get contiguous bits */

    SwappedId = AcpiUtDwordByteSwap ((UINT32) CompressedId);

    /* First 3 bytes are uppercase letters. Next 4 bytes are hexadecimal */

    OutString[0] = (char) (0x40 + (((unsigned long) SwappedId >> 26) & 0x1F));
    OutString[1] = (char) (0x40 + ((SwappedId >> 21) & 0x1F));
    OutString[2] = (char) (0x40 + ((SwappedId >> 16) & 0x1F));
    OutString[3] = AcpiUtHexToAsciiChar ((UINT64) SwappedId, 12);
    OutString[4] = AcpiUtHexToAsciiChar ((UINT64) SwappedId, 8);
    OutString[5] = AcpiUtHexToAsciiChar ((UINT64) SwappedId, 4);
    OutString[6] = AcpiUtHexToAsciiChar ((UINT64) SwappedId, 0);
    OutString[7] = 0;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExIntegerToString
 *
 * PARAMETERS:  OutString       - Where to put the converted string. At least
 *                                21 bytes are needed to hold the largest
 *                                possible 64-bit integer.
 *              Value           - Value to be converted
 *
 * RETURN:      Converted string in OutString
 *
 * DESCRIPTION: Convert a 64-bit integer to decimal string representation.
 *              Assumes string buffer is large enough to hold the string. The
 *              largest string is (ACPI_MAX64_DECIMAL_DIGITS + 1).
 *
 ******************************************************************************/

void
AcpiExIntegerToString (
    char                    *OutString,
    UINT64                  Value)
{
    UINT32                  Count;
    UINT32                  DigitsNeeded;
    UINT32                  Remainder;


    ACPI_FUNCTION_ENTRY ();


    DigitsNeeded = AcpiExDigitsNeeded (Value, 10);
    OutString[DigitsNeeded] = 0;

    for (Count = DigitsNeeded; Count > 0; Count--)
    {
        (void) AcpiUtShortDivide (Value, 10, &Value, &Remainder);
        OutString[Count-1] = (char) ('0' + Remainder);\
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExPciClsToString
 *
 * PARAMETERS:  OutString       - Where to put the converted string (7 bytes)
 *              ClassCode       - PCI class code to be converted (3 bytes)
 *
 * RETURN:      Converted string in OutString
 *
 * DESCRIPTION: Convert 3-bytes PCI class code to string representation.
 *              Return buffer must be large enough to hold the string. The
 *              string returned is always exactly of length
 *              ACPI_PCICLS_STRING_SIZE (includes null terminator).
 *
 ******************************************************************************/

void
AcpiExPciClsToString (
    char                    *OutString,
    UINT8                   ClassCode[3])
{

    ACPI_FUNCTION_ENTRY ();


    /* All 3 bytes are hexadecimal */

    OutString[0] = AcpiUtHexToAsciiChar ((UINT64) ClassCode[0], 4);
    OutString[1] = AcpiUtHexToAsciiChar ((UINT64) ClassCode[0], 0);
    OutString[2] = AcpiUtHexToAsciiChar ((UINT64) ClassCode[1], 4);
    OutString[3] = AcpiUtHexToAsciiChar ((UINT64) ClassCode[1], 0);
    OutString[4] = AcpiUtHexToAsciiChar ((UINT64) ClassCode[2], 4);
    OutString[5] = AcpiUtHexToAsciiChar ((UINT64) ClassCode[2], 0);
    OutString[6] = 0;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiIsValidSpaceId
 *
 * PARAMETERS:  SpaceId             - ID to be validated
 *
 * RETURN:      TRUE if SpaceId is a valid/supported ID.
 *
 * DESCRIPTION: Validate an operation region SpaceID.
 *
 ******************************************************************************/

BOOLEAN
AcpiIsValidSpaceId (
    UINT8                   SpaceId)
{

    if ((SpaceId >= ACPI_NUM_PREDEFINED_REGIONS) &&
        (SpaceId < ACPI_USER_REGION_BEGIN) &&
        (SpaceId != ACPI_ADR_SPACE_DATA_TABLE) &&
        (SpaceId != ACPI_ADR_SPACE_FIXED_HARDWARE))
    {
        return (FALSE);
    }

    return (TRUE);
}

#endif
