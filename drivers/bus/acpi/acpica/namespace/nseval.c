/*******************************************************************************
 *
 * Module Name: nseval - Object evaluation, includes control method execution
 *
 ******************************************************************************/

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
#include "acparser.h"
#include "acinterp.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nseval")


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsEvaluate
 *
 * PARAMETERS:  Info            - Evaluation info block, contains these fields
 *                                and more:
 *                  PrefixNode      - Prefix or Method/Object Node to execute
 *                  RelativePath    - Name of method to execute, If NULL, the
 *                                    Node is the object to execute
 *                  Parameters      - List of parameters to pass to the method,
 *                                    terminated by NULL. Params itself may be
 *                                    NULL if no parameters are being passed.
 *                  ParameterType   - Type of Parameter list
 *                  ReturnObject    - Where to put method's return value (if
 *                                    any). If NULL, no value is returned.
 *                  Flags           - ACPI_IGNORE_RETURN_VALUE to delete return
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a control method or return the current value of an
 *              ACPI namespace object.
 *
 * MUTEX:       Locks interpreter
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsEvaluate (
    ACPI_EVALUATE_INFO      *Info)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (NsEvaluate);


    if (!Info)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (!Info->Node)
    {
        /*
         * Get the actual namespace node for the target object if we
         * need to. Handles these cases:
         *
         * 1) Null node, valid pathname from root (absolute path)
         * 2) Node and valid pathname (path relative to Node)
         * 3) Node, Null pathname
         */
        Status = AcpiNsGetNode (Info->PrefixNode, Info->RelativePathname,
            ACPI_NS_NO_UPSEARCH, &Info->Node);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * For a method alias, we must grab the actual method node so that
     * proper scoping context will be established before execution.
     */
    if (AcpiNsGetType (Info->Node) == ACPI_TYPE_LOCAL_METHOD_ALIAS)
    {
        Info->Node = ACPI_CAST_PTR (
            ACPI_NAMESPACE_NODE, Info->Node->Object);
    }

    /* Complete the info block initialization */

    Info->ReturnObject = NULL;
    Info->NodeFlags = Info->Node->Flags;
    Info->ObjDesc = AcpiNsGetAttachedObject (Info->Node);

    ACPI_DEBUG_PRINT ((ACPI_DB_NAMES, "%s [%p] Value %p\n",
        Info->RelativePathname, Info->Node,
        AcpiNsGetAttachedObject (Info->Node)));

    /* Get info if we have a predefined name (_HID, etc.) */

    Info->Predefined = AcpiUtMatchPredefinedMethod (Info->Node->Name.Ascii);

    /* Get the full pathname to the object, for use in warning messages */

    Info->FullPathname = AcpiNsGetNormalizedPathname (Info->Node, TRUE);
    if (!Info->FullPathname)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Optional object evaluation log */

    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_EVALUATION,
        "%-26s:  %s (%s)\n", "   Enter evaluation",
        &Info->FullPathname[1], AcpiUtGetTypeName (Info->Node->Type)));

    /* Count the number of arguments being passed in */

    Info->ParamCount = 0;
    if (Info->Parameters)
    {
        while (Info->Parameters[Info->ParamCount])
        {
            Info->ParamCount++;
        }

        /* Warn on impossible argument count */

        if (Info->ParamCount > ACPI_METHOD_NUM_ARGS)
        {
            ACPI_WARN_PREDEFINED ((AE_INFO, Info->FullPathname, ACPI_WARN_ALWAYS,
                "Excess arguments (%u) - using only %u",
                Info->ParamCount, ACPI_METHOD_NUM_ARGS));

            Info->ParamCount = ACPI_METHOD_NUM_ARGS;
        }
    }

    /*
     * For predefined names: Check that the declared argument count
     * matches the ACPI spec -- otherwise this is a BIOS error.
     */
    AcpiNsCheckAcpiCompliance (Info->FullPathname, Info->Node,
        Info->Predefined);

    /*
     * For all names: Check that the incoming argument count for
     * this method/object matches the actual ASL/AML definition.
     */
    AcpiNsCheckArgumentCount (Info->FullPathname, Info->Node,
        Info->ParamCount, Info->Predefined);

    /* For predefined names: Typecheck all incoming arguments */

    AcpiNsCheckArgumentTypes (Info);

    /*
     * Three major evaluation cases:
     *
     * 1) Object types that cannot be evaluated by definition
     * 2) The object is a control method -- execute it
     * 3) The object is not a method -- just return it's current value
     */
    switch (AcpiNsGetType (Info->Node))
    {
    case ACPI_TYPE_ANY:
    case ACPI_TYPE_DEVICE:
    case ACPI_TYPE_EVENT:
    case ACPI_TYPE_MUTEX:
    case ACPI_TYPE_REGION:
    case ACPI_TYPE_THERMAL:
    case ACPI_TYPE_LOCAL_SCOPE:
        /*
         * 1) Disallow evaluation of these object types. For these,
         *    object evaluation is undefined.
         */
        ACPI_ERROR ((AE_INFO,
            "%s: This object type [%s] "
            "never contains data and cannot be evaluated",
            Info->FullPathname, AcpiUtGetTypeName (Info->Node->Type)));

        Status = AE_TYPE;
        goto Cleanup;

    case ACPI_TYPE_METHOD:
        /*
         * 2) Object is a control method - execute it
         */

        /* Verify that there is a method object associated with this node */

        if (!Info->ObjDesc)
        {
            ACPI_ERROR ((AE_INFO, "%s: Method has no attached sub-object",
                Info->FullPathname));
            Status = AE_NULL_OBJECT;
            goto Cleanup;
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "**** Execute method [%s] at AML address %p length %X\n",
            Info->FullPathname,
            Info->ObjDesc->Method.AmlStart + 1,
            Info->ObjDesc->Method.AmlLength - 1));

        /*
         * Any namespace deletion must acquire both the namespace and
         * interpreter locks to ensure that no thread is using the portion of
         * the namespace that is being deleted.
         *
         * Execute the method via the interpreter. The interpreter is locked
         * here before calling into the AML parser
         */
        AcpiExEnterInterpreter ();
        Status = AcpiPsExecuteMethod (Info);
        AcpiExExitInterpreter ();
        break;

    default:
        /*
         * 3) All other non-method objects -- get the current object value
         */

        /*
         * Some objects require additional resolution steps (e.g., the Node
         * may be a field that must be read, etc.) -- we can't just grab
         * the object out of the node.
         *
         * Use ResolveNodeToValue() to get the associated value.
         *
         * NOTE: we can get away with passing in NULL for a walk state because
         * the Node is guaranteed to not be a reference to either a method
         * local or a method argument (because this interface is never called
         * from a running method.)
         *
         * Even though we do not directly invoke the interpreter for object
         * resolution, we must lock it because we could access an OpRegion.
         * The OpRegion access code assumes that the interpreter is locked.
         */
        AcpiExEnterInterpreter ();

        /* TBD: ResolveNodeToValue has a strange interface, fix */

        Info->ReturnObject = ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, Info->Node);

        Status = AcpiExResolveNodeToValue (ACPI_CAST_INDIRECT_PTR (
            ACPI_NAMESPACE_NODE, &Info->ReturnObject), NULL);
        AcpiExExitInterpreter ();

        if (ACPI_FAILURE (Status))
        {
            Info->ReturnObject = NULL;
            goto Cleanup;
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_NAMES, "Returned object %p [%s]\n",
            Info->ReturnObject,
            AcpiUtGetObjectTypeName (Info->ReturnObject)));

        Status = AE_CTRL_RETURN_VALUE; /* Always has a "return value" */
        break;
    }

    /*
     * For predefined names, check the return value against the ACPI
     * specification. Some incorrect return value types are repaired.
     */
    (void) AcpiNsCheckReturnValue (Info->Node, Info, Info->ParamCount,
        Status, &Info->ReturnObject);

    /* Check if there is a return value that must be dealt with */

    if (Status == AE_CTRL_RETURN_VALUE)
    {
        /* If caller does not want the return value, delete it */

        if (Info->Flags & ACPI_IGNORE_RETURN_VALUE)
        {
            AcpiUtRemoveReference (Info->ReturnObject);
            Info->ReturnObject = NULL;
        }

        /* Map AE_CTRL_RETURN_VALUE to AE_OK, we are done with it */

        Status = AE_OK;
    }
    else if (ACPI_FAILURE(Status))
    {
        /* If ReturnObject exists, delete it */

        if (Info->ReturnObject)
        {
            AcpiUtRemoveReference (Info->ReturnObject);
            Info->ReturnObject = NULL;
        }
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
        "*** Completed evaluation of object %s ***\n",
        Info->RelativePathname));

Cleanup:
    /* Optional object evaluation log */

    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_EVALUATION,
        "%-26s:  %s\n", "   Exit evaluation",
        &Info->FullPathname[1]));

    /*
     * Namespace was unlocked by the handling AcpiNs* function, so we
     * just free the pathname and return
     */
    ACPI_FREE (Info->FullPathname);
    Info->FullPathname = NULL;
    return_ACPI_STATUS (Status);
}
