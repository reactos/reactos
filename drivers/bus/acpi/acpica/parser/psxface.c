/******************************************************************************
 *
 * Module Name: psxface - Parser external interfaces
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
#include "acparser.h"
#include "acdispat.h"
#include "acinterp.h"
#include "actables.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psxface")

/* Local Prototypes */

static void
AcpiPsUpdateParameterList (
    ACPI_EVALUATE_INFO      *Info,
    UINT16                  Action);


/*******************************************************************************
 *
 * FUNCTION:    AcpiDebugTrace
 *
 * PARAMETERS:  MethodName      - Valid ACPI name string
 *              DebugLevel      - Optional level mask. 0 to use default
 *              DebugLayer      - Optional layer mask. 0 to use default
 *              Flags           - bit 1: one shot(1) or persistent(0)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: External interface to enable debug tracing during control
 *              method execution
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDebugTrace (
    const char              *Name,
    UINT32                  DebugLevel,
    UINT32                  DebugLayer,
    UINT32                  Flags)
{
    ACPI_STATUS             Status;


    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    AcpiGbl_TraceMethodName = Name;
    AcpiGbl_TraceFlags = Flags;
    AcpiGbl_TraceDbgLevel = DebugLevel;
    AcpiGbl_TraceDbgLayer = DebugLayer;
    Status = AE_OK;

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsExecuteMethod
 *
 * PARAMETERS:  Info            - Method info block, contains:
 *                  Node            - Method Node to execute
 *                  ObjDesc         - Method object
 *                  Parameters      - List of parameters to pass to the method,
 *                                    terminated by NULL. Params itself may be
 *                                    NULL if no parameters are being passed.
 *                  ReturnObject    - Where to put method's return value (if
 *                                    any). If NULL, no value is returned.
 *                  ParameterType   - Type of Parameter list
 *                  ReturnObject    - Where to put method's return value (if
 *                                    any). If NULL, no value is returned.
 *                  PassNumber      - Parse or execute pass
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a control method
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsExecuteMethod (
    ACPI_EVALUATE_INFO      *Info)
{
    ACPI_STATUS             Status;
    ACPI_PARSE_OBJECT       *Op;
    ACPI_WALK_STATE         *WalkState;


    ACPI_FUNCTION_TRACE (PsExecuteMethod);


    /* Quick validation of DSDT header */

    AcpiTbCheckDsdtHeader ();

    /* Validate the Info and method Node */

    if (!Info || !Info->Node)
    {
        return_ACPI_STATUS (AE_NULL_ENTRY);
    }

    /* Init for new method, wait on concurrency semaphore */

    Status = AcpiDsBeginMethodExecution (Info->Node, Info->ObjDesc, NULL);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * The caller "owns" the parameters, so give each one an extra reference
     */
    AcpiPsUpdateParameterList (Info, REF_INCREMENT);

    /*
     * Execute the method. Performs parse simultaneously
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
        "**** Begin Method Parse/Execute [%4.4s] **** Node=%p Obj=%p\n",
        Info->Node->Name.Ascii, Info->Node, Info->ObjDesc));

    /* Create and init a Root Node */

    Op = AcpiPsCreateScopeOp (Info->ObjDesc->Method.AmlStart);
    if (!Op)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Create and initialize a new walk state */

    Info->PassNumber = ACPI_IMODE_EXECUTE;
    WalkState = AcpiDsCreateWalkState (
        Info->ObjDesc->Method.OwnerId, NULL, NULL, NULL);
    if (!WalkState)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    Status = AcpiDsInitAmlWalk (WalkState, Op, Info->Node,
        Info->ObjDesc->Method.AmlStart,
        Info->ObjDesc->Method.AmlLength, Info, Info->PassNumber);
    if (ACPI_FAILURE (Status))
    {
        AcpiDsDeleteWalkState (WalkState);
        goto Cleanup;
    }

    if (Info->ObjDesc->Method.InfoFlags & ACPI_METHOD_MODULE_LEVEL)
    {
        WalkState->ParseFlags |= ACPI_PARSE_MODULE_LEVEL;
    }

    /* Invoke an internal method if necessary */

    if (Info->ObjDesc->Method.InfoFlags & ACPI_METHOD_INTERNAL_ONLY)
    {
        Status = Info->ObjDesc->Method.Dispatch.Implementation (WalkState);
        Info->ReturnObject = WalkState->ReturnDesc;

        /* Cleanup states */

        AcpiDsScopeStackClear (WalkState);
        AcpiPsCleanupScope (&WalkState->ParserState);
        AcpiDsTerminateControlMethod (WalkState->MethodDesc, WalkState);
        AcpiDsDeleteWalkState (WalkState);
        goto Cleanup;
    }

    /*
     * Start method evaluation with an implicit return of zero.
     * This is done for Windows compatibility.
     */
    if (AcpiGbl_EnableInterpreterSlack)
    {
        WalkState->ImplicitReturnObj =
            AcpiUtCreateIntegerObject ((UINT64) 0);
        if (!WalkState->ImplicitReturnObj)
        {
            Status = AE_NO_MEMORY;
            AcpiDsDeleteWalkState (WalkState);
            goto Cleanup;
        }
    }

    /* Parse the AML */

    Status = AcpiPsParseAml (WalkState);

    /* WalkState was deleted by ParseAml */

Cleanup:
    AcpiPsDeleteParseTree (Op);

    /* Take away the extra reference that we gave the parameters above */

    AcpiPsUpdateParameterList (Info, REF_DECREMENT);

    /* Exit now if error above */

    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * If the method has returned an object, signal this to the caller with
     * a control exception code
     */
    if (Info->ReturnObject)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "Method returned ObjDesc=%p\n",
            Info->ReturnObject));
        ACPI_DUMP_STACK_ENTRY (Info->ReturnObject);

        Status = AE_CTRL_RETURN_VALUE;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsExecuteTable
 *
 * PARAMETERS:  Info            - Method info block, contains:
 *              Node            - Node to where the is entered into the
 *                                namespace
 *              ObjDesc         - Pseudo method object describing the AML
 *                                code of the entire table
 *              PassNumber      - Parse or execute pass
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a table
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsExecuteTable (
    ACPI_EVALUATE_INFO      *Info)
{
    ACPI_STATUS             Status;
    ACPI_PARSE_OBJECT       *Op = NULL;
    ACPI_WALK_STATE         *WalkState = NULL;


    ACPI_FUNCTION_TRACE (PsExecuteTable);


    /* Create and init a Root Node */

    Op = AcpiPsCreateScopeOp (Info->ObjDesc->Method.AmlStart);
    if (!Op)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Create and initialize a new walk state */

    WalkState = AcpiDsCreateWalkState (
        Info->ObjDesc->Method.OwnerId, NULL, NULL, NULL);
    if (!WalkState)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    Status = AcpiDsInitAmlWalk (WalkState, Op, Info->Node,
        Info->ObjDesc->Method.AmlStart,
        Info->ObjDesc->Method.AmlLength, Info, Info->PassNumber);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    if (Info->ObjDesc->Method.InfoFlags & ACPI_METHOD_MODULE_LEVEL)
    {
        WalkState->ParseFlags |= ACPI_PARSE_MODULE_LEVEL;
    }

    /* Info->Node is the default location to load the table  */

    if (Info->Node && Info->Node != AcpiGbl_RootNode)
    {
        Status = AcpiDsScopeStackPush (
            Info->Node, ACPI_TYPE_METHOD, WalkState);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }
    }

    /*
     * Parse the AML, WalkState will be deleted by ParseAml
     */
    AcpiExEnterInterpreter ();
    Status = AcpiPsParseAml (WalkState);
    AcpiExExitInterpreter ();
    WalkState = NULL;

Cleanup:
    if (WalkState)
    {
        AcpiDsDeleteWalkState (WalkState);
    }
    if (Op)
    {
        AcpiPsDeleteParseTree (Op);
    }
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsUpdateParameterList
 *
 * PARAMETERS:  Info            - See ACPI_EVALUATE_INFO
 *                                (Used: ParameterType and Parameters)
 *              Action          - Add or Remove reference
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Update reference count on all method parameter objects
 *
 ******************************************************************************/

static void
AcpiPsUpdateParameterList (
    ACPI_EVALUATE_INFO      *Info,
    UINT16                  Action)
{
    UINT32                  i;


    if (Info->Parameters)
    {
        /* Update reference count for each parameter */

        for (i = 0; Info->Parameters[i]; i++)
        {
            /* Ignore errors, just do them all */

            (void) AcpiUtUpdateObjectReference (
                Info->Parameters[i], Action);
        }
    }
}
