/******************************************************************************
 *
 * Module Name: dsdebug - Parser/Interpreter interface - debugging
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
#include "acdispat.h"
#include "acnamesp.h"
#include "acdisasm.h"
#include "acinterp.h"


#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsdebug")


#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DEBUGGER)

/* Local prototypes */

static void
AcpiDsPrintNodePathname (
    ACPI_NAMESPACE_NODE     *Node,
    const char              *Message);


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsPrintNodePathname
 *
 * PARAMETERS:  Node            - Object
 *              Message         - Prefix message
 *
 * DESCRIPTION: Print an object's full namespace pathname
 *              Manages allocation/freeing of a pathname buffer
 *
 ******************************************************************************/

static void
AcpiDsPrintNodePathname (
    ACPI_NAMESPACE_NODE     *Node,
    const char              *Message)
{
    ACPI_BUFFER             Buffer;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (DsPrintNodePathname);

    if (!Node)
    {
        ACPI_DEBUG_PRINT_RAW ((ACPI_DB_DISPATCH, "[NULL NAME]"));
        return_VOID;
    }

    /* Convert handle to full pathname and print it (with supplied message) */

    Buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

    Status = AcpiNsHandleToPathname (Node, &Buffer, TRUE);
    if (ACPI_SUCCESS (Status))
    {
        if (Message)
        {
            ACPI_DEBUG_PRINT_RAW ((ACPI_DB_DISPATCH, "%s ", Message));
        }

        ACPI_DEBUG_PRINT_RAW ((ACPI_DB_DISPATCH, "[%s] (Node %p)",
            (char *) Buffer.Pointer, Node));
        ACPI_FREE (Buffer.Pointer);
    }

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsDumpMethodStack
 *
 * PARAMETERS:  Status          - Method execution status
 *              WalkState       - Current state of the parse tree walk
 *              Op              - Executing parse op
 *
 * RETURN:      None
 *
 * DESCRIPTION: Called when a method has been aborted because of an error.
 *              Dumps the method execution stack.
 *
 ******************************************************************************/

void
AcpiDsDumpMethodStack (
    ACPI_STATUS             Status,
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_PARSE_OBJECT       *Next;
    ACPI_THREAD_STATE       *Thread;
    ACPI_WALK_STATE         *NextWalkState;
    ACPI_NAMESPACE_NODE     *PreviousMethod = NULL;
    ACPI_OPERAND_OBJECT     *MethodDesc;


    ACPI_FUNCTION_TRACE (DsDumpMethodStack);

    /* Ignore control codes, they are not errors */

    if ((Status & AE_CODE_MASK) == AE_CODE_CONTROL)
    {
        return_VOID;
    }

    /* We may be executing a deferred opcode */

    if (WalkState->DeferredNode)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "Executing subtree for Buffer/Package/Region\n"));
        return_VOID;
    }

    /*
     * If there is no Thread, we are not actually executing a method.
     * This can happen when the iASL compiler calls the interpreter
     * to perform constant folding.
     */
    Thread = WalkState->Thread;
    if (!Thread)
    {
        return_VOID;
    }

    /* Display exception and method name */

    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "\n**** Exception %s during execution of method ",
        AcpiFormatException (Status)));

    AcpiDsPrintNodePathname (WalkState->MethodNode, NULL);

    /* Display stack of executing methods */

    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_DISPATCH,
        "\n\nMethod Execution Stack:\n"));
    NextWalkState = Thread->WalkStateList;

    /* Walk list of linked walk states */

    while (NextWalkState)
    {
        MethodDesc = NextWalkState->MethodDesc;
        if (MethodDesc)
        {
            AcpiExStopTraceMethod (
                (ACPI_NAMESPACE_NODE *) MethodDesc->Method.Node,
                MethodDesc, WalkState);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "    Method [%4.4s] executing: ",
            AcpiUtGetNodeName (NextWalkState->MethodNode)));

        /* First method is the currently executing method */

        if (NextWalkState == WalkState)
        {
            if (Op)
            {
                /* Display currently executing ASL statement */

                Next = Op->Common.Next;
                Op->Common.Next = NULL;

#ifdef ACPI_DISASSEMBLER
                AcpiOsPrintf ("Failed at ");
                AcpiDmDisassemble (NextWalkState, Op, ACPI_UINT32_MAX);
#endif
                Op->Common.Next = Next;
            }
        }
        else
        {
            /*
             * This method has called another method
             * NOTE: the method call parse subtree is already deleted at
             * this point, so we cannot disassemble the method invocation.
             */
            ACPI_DEBUG_PRINT_RAW ((ACPI_DB_DISPATCH, "Call to method "));
            AcpiDsPrintNodePathname (PreviousMethod, NULL);
        }

        PreviousMethod = NextWalkState->MethodNode;
        NextWalkState = NextWalkState->Next;
        ACPI_DEBUG_PRINT_RAW ((ACPI_DB_DISPATCH, "\n"));
    }

    return_VOID;
}

#else

void
AcpiDsDumpMethodStack (
    ACPI_STATUS             Status,
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    return;
}

#endif
