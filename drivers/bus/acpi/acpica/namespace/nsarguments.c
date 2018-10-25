/******************************************************************************
 *
 * Module Name: nsarguments - Validation of args for ACPI predefined methods
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

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "acpredef.h"


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsarguments")


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsCheckArgumentTypes
 *
 * PARAMETERS:  Info            - Method execution information block
 *
 * RETURN:      None
 *
 * DESCRIPTION: Check the incoming argument count and all argument types
 *              against the argument type list for a predefined name.
 *
 ******************************************************************************/

void
AcpiNsCheckArgumentTypes (
    ACPI_EVALUATE_INFO          *Info)
{
    UINT16                      ArgTypeList;
    UINT8                       ArgCount;
    UINT8                       ArgType;
    UINT8                       UserArgType;
    UINT32                      i;


    /*
     * If not a predefined name, cannot typecheck args, because
     * we have no idea what argument types are expected.
     * Also, ignore typecheck if warnings/errors if this method
     * has already been evaluated at least once -- in order
     * to suppress repetitive messages.
     */
    if (!Info->Predefined || (Info->Node->Flags & ANOBJ_EVALUATED))
    {
        return;
    }

    ArgTypeList = Info->Predefined->Info.ArgumentList;
    ArgCount = METHOD_GET_ARG_COUNT (ArgTypeList);

    /* Typecheck all arguments */

    for (i = 0; ((i < ArgCount) && (i < Info->ParamCount)); i++)
    {
        ArgType = METHOD_GET_NEXT_TYPE (ArgTypeList);
        UserArgType = Info->Parameters[i]->Common.Type;

        if (UserArgType != ArgType)
        {
            ACPI_WARN_PREDEFINED ((AE_INFO, Info->FullPathname, ACPI_WARN_ALWAYS,
                "Argument #%u type mismatch - "
                "Found [%s], ACPI requires [%s]", (i + 1),
                AcpiUtGetTypeName (UserArgType),
                AcpiUtGetTypeName (ArgType)));

            /* Prevent any additional typechecking for this method */

            Info->Node->Flags |= ANOBJ_EVALUATED;
        }
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsCheckAcpiCompliance
 *
 * PARAMETERS:  Pathname        - Full pathname to the node (for error msgs)
 *              Node            - Namespace node for the method/object
 *              Predefined      - Pointer to entry in predefined name table
 *
 * RETURN:      None
 *
 * DESCRIPTION: Check that the declared parameter count (in ASL/AML) for a
 *              predefined name is what is expected (matches what is defined in
 *              the ACPI specification for this predefined name.)
 *
 ******************************************************************************/

void
AcpiNsCheckAcpiCompliance (
    char                        *Pathname,
    ACPI_NAMESPACE_NODE         *Node,
    const ACPI_PREDEFINED_INFO  *Predefined)
{
    UINT32                      AmlParamCount;
    UINT32                      RequiredParamCount;


    if (!Predefined || (Node->Flags & ANOBJ_EVALUATED))
    {
        return;
    }

    /* Get the ACPI-required arg count from the predefined info table */

    RequiredParamCount =
        METHOD_GET_ARG_COUNT (Predefined->Info.ArgumentList);

    /*
     * If this object is not a control method, we can check if the ACPI
     * spec requires that it be a method.
     */
    if (Node->Type != ACPI_TYPE_METHOD)
    {
        if (RequiredParamCount > 0)
        {
            /* Object requires args, must be implemented as a method */

            ACPI_BIOS_ERROR_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
                "Object (%s) must be a control method with %u arguments",
                AcpiUtGetTypeName (Node->Type), RequiredParamCount));
        }
        else if (!RequiredParamCount && !Predefined->Info.ExpectedBtypes)
        {
            /* Object requires no args and no return value, must be a method */

            ACPI_BIOS_ERROR_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
                "Object (%s) must be a control method "
                "with no arguments and no return value",
                AcpiUtGetTypeName (Node->Type)));
        }

        return;
    }

    /*
     * This is a control method.
     * Check that the ASL/AML-defined parameter count for this method
     * matches the ACPI-required parameter count
     *
     * Some methods are allowed to have a "minimum" number of args (_SCP)
     * because their definition in ACPI has changed over time.
     *
     * Note: These are BIOS errors in the declaration of the object
     */
    AmlParamCount = Node->Object->Method.ParamCount;

    if (AmlParamCount < RequiredParamCount)
    {
        ACPI_BIOS_ERROR_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
            "Insufficient arguments - "
            "ASL declared %u, ACPI requires %u",
            AmlParamCount, RequiredParamCount));
    }
    else if ((AmlParamCount > RequiredParamCount) &&
        !(Predefined->Info.ArgumentList & ARG_COUNT_IS_MINIMUM))
    {
        ACPI_BIOS_ERROR_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
            "Excess arguments - "
            "ASL declared %u, ACPI requires %u",
            AmlParamCount, RequiredParamCount));
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsCheckArgumentCount
 *
 * PARAMETERS:  Pathname        - Full pathname to the node (for error msgs)
 *              Node            - Namespace node for the method/object
 *              UserParamCount  - Number of args passed in by the caller
 *              Predefined      - Pointer to entry in predefined name table
 *
 * RETURN:      None
 *
 * DESCRIPTION: Check that incoming argument count matches the declared
 *              parameter count (in the ASL/AML) for an object.
 *
 ******************************************************************************/

void
AcpiNsCheckArgumentCount (
    char                        *Pathname,
    ACPI_NAMESPACE_NODE         *Node,
    UINT32                      UserParamCount,
    const ACPI_PREDEFINED_INFO  *Predefined)
{
    UINT32                      AmlParamCount;
    UINT32                      RequiredParamCount;


    if (Node->Flags & ANOBJ_EVALUATED)
    {
        return;
    }

    if (!Predefined)
    {
        /*
         * Not a predefined name. Check the incoming user argument count
         * against the count that is specified in the method/object.
         */
        if (Node->Type != ACPI_TYPE_METHOD)
        {
            if (UserParamCount)
            {
                ACPI_INFO_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
                    "%u arguments were passed to a non-method ACPI object (%s)",
                    UserParamCount, AcpiUtGetTypeName (Node->Type)));
            }

            return;
        }

        /*
         * This is a control method. Check the parameter count.
         * We can only check the incoming argument count against the
         * argument count declared for the method in the ASL/AML.
         *
         * Emit a message if too few or too many arguments have been passed
         * by the caller.
         *
         * Note: Too many arguments will not cause the method to
         * fail. However, the method will fail if there are too few
         * arguments and the method attempts to use one of the missing ones.
         */
        AmlParamCount = Node->Object->Method.ParamCount;

        if (UserParamCount < AmlParamCount)
        {
            ACPI_WARN_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
                "Insufficient arguments - "
                "Caller passed %u, method requires %u",
                UserParamCount, AmlParamCount));
        }
        else if (UserParamCount > AmlParamCount)
        {
            ACPI_INFO_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
                "Excess arguments - "
                "Caller passed %u, method requires %u",
                UserParamCount, AmlParamCount));
        }

        return;
    }

    /*
     * This is a predefined name. Validate the user-supplied parameter
     * count against the ACPI specification. We don't validate against
     * the method itself because what is important here is that the
     * caller is in conformance with the spec. (The arg count for the
     * method was checked against the ACPI spec earlier.)
     *
     * Some methods are allowed to have a "minimum" number of args (_SCP)
     * because their definition in ACPI has changed over time.
     */
    RequiredParamCount =
        METHOD_GET_ARG_COUNT (Predefined->Info.ArgumentList);

    if (UserParamCount < RequiredParamCount)
    {
        ACPI_WARN_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
            "Insufficient arguments - "
            "Caller passed %u, ACPI requires %u",
            UserParamCount, RequiredParamCount));
    }
    else if ((UserParamCount > RequiredParamCount) &&
        !(Predefined->Info.ArgumentList & ARG_COUNT_IS_MINIMUM))
    {
        ACPI_INFO_PREDEFINED ((AE_INFO, Pathname, ACPI_WARN_ALWAYS,
            "Excess arguments - "
            "Caller passed %u, ACPI requires %u",
            UserParamCount, RequiredParamCount));
    }
}
