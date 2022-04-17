/******************************************************************************
 *
 * Module Name: utpredef - support functions for predefined names
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
#include "acpredef.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utpredef")


/*
 * Names for the types that can be returned by the predefined objects.
 * Used for warning messages. Must be in the same order as the ACPI_RTYPEs
 */
static const char   *UtRtypeNames[] =
{
    "/Integer",
    "/String",
    "/Buffer",
    "/Package",
    "/Reference",
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetNextPredefinedMethod
 *
 * PARAMETERS:  ThisName            - Entry in the predefined method/name table
 *
 * RETURN:      Pointer to next entry in predefined table.
 *
 * DESCRIPTION: Get the next entry in the predefine method table. Handles the
 *              cases where a package info entry follows a method name that
 *              returns a package.
 *
 ******************************************************************************/

const ACPI_PREDEFINED_INFO *
AcpiUtGetNextPredefinedMethod (
    const ACPI_PREDEFINED_INFO  *ThisName)
{

    /*
     * Skip next entry in the table if this name returns a Package
     * (next entry contains the package info)
     */
    if ((ThisName->Info.ExpectedBtypes & ACPI_RTYPE_PACKAGE) &&
        (ThisName->Info.ExpectedBtypes != ACPI_RTYPE_ALL))
    {
        ThisName++;
    }

    ThisName++;
    return (ThisName);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtMatchPredefinedMethod
 *
 * PARAMETERS:  Name                - Name to find
 *
 * RETURN:      Pointer to entry in predefined table. NULL indicates not found.
 *
 * DESCRIPTION: Check an object name against the predefined object list.
 *
 ******************************************************************************/

const ACPI_PREDEFINED_INFO *
AcpiUtMatchPredefinedMethod (
    char                        *Name)
{
    const ACPI_PREDEFINED_INFO  *ThisName;


    /* Quick check for a predefined name, first character must be underscore */

    if (Name[0] != '_')
    {
        return (NULL);
    }

    /* Search info table for a predefined method/object name */

    ThisName = AcpiGbl_PredefinedMethods;
    while (ThisName->Info.Name[0])
    {
        if (ACPI_COMPARE_NAMESEG (Name, ThisName->Info.Name))
        {
            return (ThisName);
        }

        ThisName = AcpiUtGetNextPredefinedMethod (ThisName);
    }

    return (NULL); /* Not found */
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetExpectedReturnTypes
 *
 * PARAMETERS:  Buffer              - Where the formatted string is returned
 *              ExpectedBTypes      - Bitfield of expected data types
 *
 * RETURN:      Formatted string in Buffer.
 *
 * DESCRIPTION: Format the expected object types into a printable string.
 *
 ******************************************************************************/

void
AcpiUtGetExpectedReturnTypes (
    char                    *Buffer,
    UINT32                  ExpectedBtypes)
{
    UINT32                  ThisRtype;
    UINT32                  i;
    UINT32                  j;


    if (!ExpectedBtypes)
    {
        strcpy (Buffer, "NONE");
        return;
    }

    j = 1;
    Buffer[0] = 0;
    ThisRtype = ACPI_RTYPE_INTEGER;

    for (i = 0; i < ACPI_NUM_RTYPES; i++)
    {
        /* If one of the expected types, concatenate the name of this type */

        if (ExpectedBtypes & ThisRtype)
        {
            strcat (Buffer, &UtRtypeNames[i][j]);
            j = 0;              /* Use name separator from now on */
        }

        ThisRtype <<= 1;    /* Next Rtype */
    }
}


/*******************************************************************************
 *
 * The remaining functions are used by iASL and AcpiHelp only
 *
 ******************************************************************************/

#if (defined ACPI_ASL_COMPILER || defined ACPI_HELP_APP)

/* Local prototypes */

static UINT32
AcpiUtGetArgumentTypes (
    char                    *Buffer,
    UINT16                  ArgumentTypes);


/* Types that can be returned externally by a predefined name */

static const char   *UtExternalTypeNames[] = /* Indexed by ACPI_TYPE_* */
{
    ", Type_ANY",
    ", Integer",
    ", String",
    ", Buffer",
    ", Package"
};

/* Bit widths for resource descriptor predefined names */

static const char   *UtResourceTypeNames[] =
{
    "/1",
    "/2",
    "/3",
    "/8",
    "/16",
    "/32",
    "/64",
    "/variable",
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtMatchResourceName
 *
 * PARAMETERS:  Name                - Name to find
 *
 * RETURN:      Pointer to entry in the resource table. NULL indicates not
 *              found.
 *
 * DESCRIPTION: Check an object name against the predefined resource
 *              descriptor object list.
 *
 ******************************************************************************/

const ACPI_PREDEFINED_INFO *
AcpiUtMatchResourceName (
    char                        *Name)
{
    const ACPI_PREDEFINED_INFO  *ThisName;


    /*
     * Quick check for a predefined name, first character must
     * be underscore
     */
    if (Name[0] != '_')
    {
        return (NULL);
    }

    /* Search info table for a predefined method/object name */

    ThisName = AcpiGbl_ResourceNames;
    while (ThisName->Info.Name[0])
    {
        if (ACPI_COMPARE_NAMESEG (Name, ThisName->Info.Name))
        {
            return (ThisName);
        }

        ThisName++;
    }

    return (NULL); /* Not found */
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDisplayPredefinedMethod
 *
 * PARAMETERS:  Buffer              - Scratch buffer for this function
 *              ThisName            - Entry in the predefined method/name table
 *              MultiLine           - TRUE if output should be on >1 line
 *
 * RETURN:      None
 *
 * DESCRIPTION: Display information about a predefined method. Number and
 *              type of the input arguments, and expected type(s) for the
 *              return value, if any.
 *
 ******************************************************************************/

void
AcpiUtDisplayPredefinedMethod (
    char                        *Buffer,
    const ACPI_PREDEFINED_INFO  *ThisName,
    BOOLEAN                     MultiLine)
{
    UINT32                      ArgCount;

    /*
     * Get the argument count and the string buffer
     * containing all argument types
     */
    ArgCount = AcpiUtGetArgumentTypes (Buffer,
        ThisName->Info.ArgumentList);

    if (MultiLine)
    {
        printf ("      ");
    }

    printf ("%4.4s    Requires %s%u argument%s",
        ThisName->Info.Name,
        (ThisName->Info.ArgumentList & ARG_COUNT_IS_MINIMUM) ?
            "(at least) " : "",
        ArgCount, ArgCount != 1 ? "s" : "");

    /* Display the types for any arguments */

    if (ArgCount > 0)
    {
        printf (" (%s)", Buffer);
    }

    if (MultiLine)
    {
        printf ("\n    ");
    }

    /* Get the return value type(s) allowed */

    if (ThisName->Info.ExpectedBtypes)
    {
        AcpiUtGetExpectedReturnTypes (Buffer, ThisName->Info.ExpectedBtypes);
        printf ("  Return value types: %s\n", Buffer);
    }
    else
    {
        printf ("  No return value\n");
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetArgumentTypes
 *
 * PARAMETERS:  Buffer              - Where to return the formatted types
 *              ArgumentTypes       - Types field for this method
 *
 * RETURN:      Count - the number of arguments required for this method
 *
 * DESCRIPTION: Format the required data types for this method (Integer,
 *              String, Buffer, or Package) and return the required argument
 *              count.
 *
 ******************************************************************************/

static UINT32
AcpiUtGetArgumentTypes (
    char                    *Buffer,
    UINT16                  ArgumentTypes)
{
    UINT16                  ThisArgumentType;
    UINT16                  SubIndex;
    UINT16                  ArgCount;
    UINT32                  i;


    *Buffer = 0;
    SubIndex = 2;

    /* First field in the types list is the count of args to follow */

    ArgCount = METHOD_GET_ARG_COUNT (ArgumentTypes);
    if (ArgCount > METHOD_PREDEF_ARGS_MAX)
    {
        printf ("**** Invalid argument count (%u) "
            "in predefined info structure\n", ArgCount);
        return (ArgCount);
    }

    /* Get each argument from the list, convert to ascii, store to buffer */

    for (i = 0; i < ArgCount; i++)
    {
        ThisArgumentType = METHOD_GET_NEXT_TYPE (ArgumentTypes);

        if (ThisArgumentType > METHOD_MAX_ARG_TYPE)
        {
            printf ("**** Invalid argument type (%u) "
                "in predefined info structure\n", ThisArgumentType);
            return (ArgCount);
        }

        strcat (Buffer, UtExternalTypeNames[ThisArgumentType] + SubIndex);
        SubIndex = 0;
    }

    return (ArgCount);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetResourceBitWidth
 *
 * PARAMETERS:  Buffer              - Where the formatted string is returned
 *              Types               - Bitfield of expected data types
 *
 * RETURN:      Count of return types. Formatted string in Buffer.
 *
 * DESCRIPTION: Format the resource bit widths into a printable string.
 *
 ******************************************************************************/

UINT32
AcpiUtGetResourceBitWidth (
    char                    *Buffer,
    UINT16                  Types)
{
    UINT32                  i;
    UINT16                  SubIndex;
    UINT32                  Found;


    *Buffer = 0;
    SubIndex = 1;
    Found = 0;

    for (i = 0; i < NUM_RESOURCE_WIDTHS; i++)
    {
        if (Types & 1)
        {
            strcat (Buffer, &(UtResourceTypeNames[i][SubIndex]));
            SubIndex = 0;
            Found++;
        }

        Types >>= 1;
    }

    return (Found);
}
#endif
