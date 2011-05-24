
/******************************************************************************
 *
 * Module Name: hwxface - Public ACPICA hardware interfaces
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_HARDWARE
        ACPI_MODULE_NAME    ("hwxface")


/******************************************************************************
 *
 * FUNCTION:    AcpiReset
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Set reset register in memory or IO space. Note: Does not
 *              support reset register in PCI config space, this must be
 *              handled separately.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiReset (
    void)
{
    ACPI_GENERIC_ADDRESS    *ResetReg;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiReset);


    ResetReg = &AcpiGbl_FADT.ResetRegister;

    /* Check if the reset register is supported */

    if (!(AcpiGbl_FADT.Flags & ACPI_FADT_RESET_REGISTER) ||
        !ResetReg->Address)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    if (ResetReg->SpaceId == ACPI_ADR_SPACE_SYSTEM_IO)
    {
        /*
         * For I/O space, write directly to the OSL. This bypasses the port
         * validation mechanism, which may block a valid write to the reset
         * register.
         */
        Status = AcpiOsWritePort ((ACPI_IO_ADDRESS) ResetReg->Address,
                    AcpiGbl_FADT.ResetValue, ResetReg->BitWidth);
    }
    else
    {
        /* Write the reset value to the reset register */

        Status = AcpiHwWrite (AcpiGbl_FADT.ResetValue, ResetReg);
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiReset)


/******************************************************************************
 *
 * FUNCTION:    AcpiRead
 *
 * PARAMETERS:  Value               - Where the value is returned
 *              Reg                 - GAS register structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read from either memory or IO space.
 *
 * LIMITATIONS: <These limitations also apply to AcpiWrite>
 *      BitWidth must be exactly 8, 16, 32, or 64.
 *      SpaceID must be SystemMemory or SystemIO.
 *      BitOffset and AccessWidth are currently ignored, as there has
 *          not been a need to implement these.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRead (
    UINT64                  *ReturnValue,
    ACPI_GENERIC_ADDRESS    *Reg)
{
    UINT32                  Value;
    UINT32                  Width;
    UINT64                  Address;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_NAME (AcpiRead);


    if (!ReturnValue)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Validate contents of the GAS register. Allow 64-bit transfers */

    Status = AcpiHwValidateRegister (Reg, 64, &Address);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Width = Reg->BitWidth;
    if (Width == 64)
    {
        Width = 32; /* Break into two 32-bit transfers */
    }

    /* Initialize entire 64-bit return value to zero */

    *ReturnValue = 0;
    Value = 0;

    /*
     * Two address spaces supported: Memory or IO. PCI_Config is
     * not supported here because the GAS structure is insufficient
     */
    if (Reg->SpaceId == ACPI_ADR_SPACE_SYSTEM_MEMORY)
    {
        Status = AcpiOsReadMemory ((ACPI_PHYSICAL_ADDRESS)
                    Address, &Value, Width);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
        *ReturnValue = Value;

        if (Reg->BitWidth == 64)
        {
            /* Read the top 32 bits */

            Status = AcpiOsReadMemory ((ACPI_PHYSICAL_ADDRESS)
                        (Address + 4), &Value, 32);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }
            *ReturnValue |= ((UINT64) Value << 32);
        }
    }
    else /* ACPI_ADR_SPACE_SYSTEM_IO, validated earlier */
    {
        Status = AcpiHwReadPort ((ACPI_IO_ADDRESS)
                    Address, &Value, Width);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
        *ReturnValue = Value;

        if (Reg->BitWidth == 64)
        {
            /* Read the top 32 bits */

            Status = AcpiHwReadPort ((ACPI_IO_ADDRESS)
                        (Address + 4), &Value, 32);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }
            *ReturnValue |= ((UINT64) Value << 32);
        }
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_IO,
        "Read:  %8.8X%8.8X width %2d from %8.8X%8.8X (%s)\n",
        ACPI_FORMAT_UINT64 (*ReturnValue), Reg->BitWidth,
        ACPI_FORMAT_UINT64 (Address),
        AcpiUtGetRegionName (Reg->SpaceId)));

    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRead)


/******************************************************************************
 *
 * FUNCTION:    AcpiWrite
 *
 * PARAMETERS:  Value               - Value to be written
 *              Reg                 - GAS register structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write to either memory or IO space.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiWrite (
    UINT64                  Value,
    ACPI_GENERIC_ADDRESS    *Reg)
{
    UINT32                  Width;
    UINT64                  Address;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_NAME (AcpiWrite);


    /* Validate contents of the GAS register. Allow 64-bit transfers */

    Status = AcpiHwValidateRegister (Reg, 64, &Address);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Width = Reg->BitWidth;
    if (Width == 64)
    {
        Width = 32; /* Break into two 32-bit transfers */
    }

    /*
     * Two address spaces supported: Memory or IO. PCI_Config is
     * not supported here because the GAS structure is insufficient
     */
    if (Reg->SpaceId == ACPI_ADR_SPACE_SYSTEM_MEMORY)
    {
        Status = AcpiOsWriteMemory ((ACPI_PHYSICAL_ADDRESS)
                    Address, ACPI_LODWORD (Value), Width);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        if (Reg->BitWidth == 64)
        {
            Status = AcpiOsWriteMemory ((ACPI_PHYSICAL_ADDRESS)
                        (Address + 4), ACPI_HIDWORD (Value), 32);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }
        }
    }
    else /* ACPI_ADR_SPACE_SYSTEM_IO, validated earlier */
    {
        Status = AcpiHwWritePort ((ACPI_IO_ADDRESS)
                    Address, ACPI_LODWORD (Value), Width);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        if (Reg->BitWidth == 64)
        {
            Status = AcpiHwWritePort ((ACPI_IO_ADDRESS)
                        (Address + 4), ACPI_HIDWORD (Value), 32);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }
        }
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_IO,
        "Wrote: %8.8X%8.8X width %2d   to %8.8X%8.8X (%s)\n",
        ACPI_FORMAT_UINT64 (Value), Reg->BitWidth,
        ACPI_FORMAT_UINT64 (Address),
        AcpiUtGetRegionName (Reg->SpaceId)));

    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiWrite)


/*******************************************************************************
 *
 * FUNCTION:    AcpiReadBitRegister
 *
 * PARAMETERS:  RegisterId      - ID of ACPI Bit Register to access
 *              ReturnValue     - Value that was read from the register,
 *                                normalized to bit position zero.
 *
 * RETURN:      Status and the value read from the specified Register. Value
 *              returned is normalized to bit0 (is shifted all the way right)
 *
 * DESCRIPTION: ACPI BitRegister read function. Does not acquire the HW lock.
 *
 * SUPPORTS:    Bit fields in PM1 Status, PM1 Enable, PM1 Control, and
 *              PM2 Control.
 *
 * Note: The hardware lock is not required when reading the ACPI bit registers
 *       since almost all of them are single bit and it does not matter that
 *       the parent hardware register can be split across two physical
 *       registers. The only multi-bit field is SLP_TYP in the PM1 control
 *       register, but this field does not cross an 8-bit boundary (nor does
 *       it make much sense to actually read this field.)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiReadBitRegister (
    UINT32                  RegisterId,
    UINT32                  *ReturnValue)
{
    ACPI_BIT_REGISTER_INFO  *BitRegInfo;
    UINT32                  RegisterValue;
    UINT32                  Value;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_U32 (AcpiReadBitRegister, RegisterId);


    /* Get the info structure corresponding to the requested ACPI Register */

    BitRegInfo = AcpiHwGetBitRegisterInfo (RegisterId);
    if (!BitRegInfo)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Read the entire parent register */

    Status = AcpiHwRegisterRead (BitRegInfo->ParentRegister,
                &RegisterValue);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Normalize the value that was read, mask off other bits */

    Value = ((RegisterValue & BitRegInfo->AccessBitMask)
                >> BitRegInfo->BitPosition);

    ACPI_DEBUG_PRINT ((ACPI_DB_IO,
        "BitReg %X, ParentReg %X, Actual %8.8X, ReturnValue %8.8X\n",
        RegisterId, BitRegInfo->ParentRegister, RegisterValue, Value));

    *ReturnValue = Value;
    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiReadBitRegister)


/*******************************************************************************
 *
 * FUNCTION:    AcpiWriteBitRegister
 *
 * PARAMETERS:  RegisterId      - ID of ACPI Bit Register to access
 *              Value           - Value to write to the register, in bit
 *                                position zero. The bit is automaticallly
 *                                shifted to the correct position.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: ACPI Bit Register write function. Acquires the hardware lock
 *              since most operations require a read/modify/write sequence.
 *
 * SUPPORTS:    Bit fields in PM1 Status, PM1 Enable, PM1 Control, and
 *              PM2 Control.
 *
 * Note that at this level, the fact that there may be actually two
 * hardware registers (A and B - and B may not exist) is abstracted.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiWriteBitRegister (
    UINT32                  RegisterId,
    UINT32                  Value)
{
    ACPI_BIT_REGISTER_INFO  *BitRegInfo;
    ACPI_CPU_FLAGS          LockFlags;
    UINT32                  RegisterValue;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_U32 (AcpiWriteBitRegister, RegisterId);


    /* Get the info structure corresponding to the requested ACPI Register */

    BitRegInfo = AcpiHwGetBitRegisterInfo (RegisterId);
    if (!BitRegInfo)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    LockFlags = AcpiOsAcquireLock (AcpiGbl_HardwareLock);

    /*
     * At this point, we know that the parent register is one of the
     * following: PM1 Status, PM1 Enable, PM1 Control, or PM2 Control
     */
    if (BitRegInfo->ParentRegister != ACPI_REGISTER_PM1_STATUS)
    {
        /*
         * 1) Case for PM1 Enable, PM1 Control, and PM2 Control
         *
         * Perform a register read to preserve the bits that we are not
         * interested in
         */
        Status = AcpiHwRegisterRead (BitRegInfo->ParentRegister,
                    &RegisterValue);
        if (ACPI_FAILURE (Status))
        {
            goto UnlockAndExit;
        }

        /*
         * Insert the input bit into the value that was just read
         * and write the register
         */
        ACPI_REGISTER_INSERT_VALUE (RegisterValue, BitRegInfo->BitPosition,
            BitRegInfo->AccessBitMask, Value);

        Status = AcpiHwRegisterWrite (BitRegInfo->ParentRegister,
                    RegisterValue);
    }
    else
    {
        /*
         * 2) Case for PM1 Status
         *
         * The Status register is different from the rest. Clear an event
         * by writing 1, writing 0 has no effect. So, the only relevant
         * information is the single bit we're interested in, all others
         * should be written as 0 so they will be left unchanged.
         */
        RegisterValue = ACPI_REGISTER_PREPARE_BITS (Value,
            BitRegInfo->BitPosition, BitRegInfo->AccessBitMask);

        /* No need to write the register if value is all zeros */

        if (RegisterValue)
        {
            Status = AcpiHwRegisterWrite (ACPI_REGISTER_PM1_STATUS,
                        RegisterValue);
        }
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_IO,
        "BitReg %X, ParentReg %X, Value %8.8X, Actual %8.8X\n",
        RegisterId, BitRegInfo->ParentRegister, Value, RegisterValue));


UnlockAndExit:

    AcpiOsReleaseLock (AcpiGbl_HardwareLock, LockFlags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiWriteBitRegister)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetSleepTypeData
 *
 * PARAMETERS:  SleepState          - Numeric sleep state
 *              *SleepTypeA         - Where SLP_TYPa is returned
 *              *SleepTypeB         - Where SLP_TYPb is returned
 *
 * RETURN:      Status - ACPI status
 *
 * DESCRIPTION: Obtain the SLP_TYPa and SLP_TYPb values for the requested sleep
 *              state.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetSleepTypeData (
    UINT8                   SleepState,
    UINT8                   *SleepTypeA,
    UINT8                   *SleepTypeB)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_EVALUATE_INFO      *Info;


    ACPI_FUNCTION_TRACE (AcpiGetSleepTypeData);


    /* Validate parameters */

    if ((SleepState > ACPI_S_STATES_MAX) ||
        !SleepTypeA ||
        !SleepTypeB)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Allocate the evaluation information block */

    Info = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_EVALUATE_INFO));
    if (!Info)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Info->Pathname = ACPI_CAST_PTR (char, AcpiGbl_SleepStateNames[SleepState]);

    /* Evaluate the namespace object containing the values for this state */

    Status = AcpiNsEvaluate (Info);
    if (ACPI_FAILURE (Status))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "%s while evaluating SleepState [%s]\n",
            AcpiFormatException (Status), Info->Pathname));

        goto Cleanup;
    }

    /* Must have a return object */

    if (!Info->ReturnObject)
    {
        ACPI_ERROR ((AE_INFO, "No Sleep State object returned from [%s]",
            Info->Pathname));
        Status = AE_NOT_EXIST;
    }

    /* It must be of type Package */

    else if (Info->ReturnObject->Common.Type != ACPI_TYPE_PACKAGE)
    {
        ACPI_ERROR ((AE_INFO, "Sleep State return object is not a Package"));
        Status = AE_AML_OPERAND_TYPE;
    }

    /*
     * The package must have at least two elements. NOTE (March 2005): This
     * goes against the current ACPI spec which defines this object as a
     * package with one encoded DWORD element. However, existing practice
     * by BIOS vendors seems to be to have 2 or more elements, at least
     * one per sleep type (A/B).
     */
    else if (Info->ReturnObject->Package.Count < 2)
    {
        ACPI_ERROR ((AE_INFO,
            "Sleep State return package does not have at least two elements"));
        Status = AE_AML_NO_OPERAND;
    }

    /* The first two elements must both be of type Integer */

    else if (((Info->ReturnObject->Package.Elements[0])->Common.Type
                != ACPI_TYPE_INTEGER) ||
             ((Info->ReturnObject->Package.Elements[1])->Common.Type
                != ACPI_TYPE_INTEGER))
    {
        ACPI_ERROR ((AE_INFO,
            "Sleep State return package elements are not both Integers "
            "(%s, %s)",
            AcpiUtGetObjectTypeName (Info->ReturnObject->Package.Elements[0]),
            AcpiUtGetObjectTypeName (Info->ReturnObject->Package.Elements[1])));
        Status = AE_AML_OPERAND_TYPE;
    }
    else
    {
        /* Valid _Sx_ package size, type, and value */

        *SleepTypeA = (UINT8)
            (Info->ReturnObject->Package.Elements[0])->Integer.Value;
        *SleepTypeB = (UINT8)
            (Info->ReturnObject->Package.Elements[1])->Integer.Value;
    }

    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "While evaluating SleepState [%s], bad Sleep object %p type %s",
            Info->Pathname, Info->ReturnObject,
            AcpiUtGetObjectTypeName (Info->ReturnObject)));
    }

    AcpiUtRemoveReference (Info->ReturnObject);

Cleanup:
    ACPI_FREE (Info);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetSleepTypeData)
