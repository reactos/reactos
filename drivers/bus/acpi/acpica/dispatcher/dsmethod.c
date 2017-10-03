/******************************************************************************
 *
 * Module Name: dsmethod - Parser/Interpreter interface - control method parsing
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
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "acparser.h"
#include "amlcode.h"
#include "acdebug.h"


#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsmethod")

/* Local prototypes */

static ACPI_STATUS
AcpiDsDetectNamedOpcodes (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       **OutOp);

static ACPI_STATUS
AcpiDsCreateMethodMutex (
    ACPI_OPERAND_OBJECT     *MethodDesc);


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsAutoSerializeMethod
 *
 * PARAMETERS:  Node                        - Namespace Node of the method
 *              ObjDesc                     - Method object attached to node
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse a control method AML to scan for control methods that
 *              need serialization due to the creation of named objects.
 *
 * NOTE: It is a bit of overkill to mark all such methods serialized, since
 * there is only a problem if the method actually blocks during execution.
 * A blocking operation is, for example, a Sleep() operation, or any access
 * to an operation region. However, it is probably not possible to easily
 * detect whether a method will block or not, so we simply mark all suspicious
 * methods as serialized.
 *
 * NOTE2: This code is essentially a generic routine for parsing a single
 * control method.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsAutoSerializeMethod (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_STATUS             Status;
    ACPI_PARSE_OBJECT       *Op = NULL;
    ACPI_WALK_STATE         *WalkState;


    ACPI_FUNCTION_TRACE_PTR (DsAutoSerializeMethod, Node);


    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
        "Method auto-serialization parse [%4.4s] %p\n",
        AcpiUtGetNodeName (Node), Node));

    /* Create/Init a root op for the method parse tree */

    Op = AcpiPsAllocOp (AML_METHOD_OP, ObjDesc->Method.AmlStart);
    if (!Op)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    AcpiPsSetName (Op, Node->Name.Integer);
    Op->Common.Node = Node;

    /* Create and initialize a new walk state */

    WalkState = AcpiDsCreateWalkState (Node->OwnerId, NULL, NULL, NULL);
    if (!WalkState)
    {
        AcpiPsFreeOp (Op);
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Status = AcpiDsInitAmlWalk (WalkState, Op, Node,
        ObjDesc->Method.AmlStart, ObjDesc->Method.AmlLength, NULL, 0);
    if (ACPI_FAILURE (Status))
    {
        AcpiDsDeleteWalkState (WalkState);
        AcpiPsFreeOp (Op);
        return_ACPI_STATUS (Status);
    }

    WalkState->DescendingCallback = AcpiDsDetectNamedOpcodes;

    /* Parse the method, scan for creation of named objects */

    Status = AcpiPsParseAml (WalkState);

    AcpiPsDeleteParseTree (Op);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsDetectNamedOpcodes
 *
 * PARAMETERS:  WalkState       - Current state of the parse tree walk
 *              OutOp           - Unused, required for parser interface
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Descending callback used during the loading of ACPI tables.
 *              Currently used to detect methods that must be marked serialized
 *              in order to avoid problems with the creation of named objects.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiDsDetectNamedOpcodes (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       **OutOp)
{

    ACPI_FUNCTION_NAME (AcpiDsDetectNamedOpcodes);


    /* We are only interested in opcodes that create a new name */

    if (!(WalkState->OpInfo->Flags & (AML_NAMED | AML_CREATE | AML_FIELD)))
    {
        return (AE_OK);
    }

    /*
     * At this point, we know we have a Named object opcode.
     * Mark the method as serialized. Later code will create a mutex for
     * this method to enforce serialization.
     *
     * Note, ACPI_METHOD_IGNORE_SYNC_LEVEL flag means that we will ignore the
     * Sync Level mechanism for this method, even though it is now serialized.
     * Otherwise, there can be conflicts with existing ASL code that actually
     * uses sync levels.
     */
    WalkState->MethodDesc->Method.SyncLevel = 0;
    WalkState->MethodDesc->Method.InfoFlags |=
        (ACPI_METHOD_SERIALIZED | ACPI_METHOD_IGNORE_SYNC_LEVEL);

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "Method serialized [%4.4s] %p - [%s] (%4.4X)\n",
        WalkState->MethodNode->Name.Ascii, WalkState->MethodNode,
        WalkState->OpInfo->Name, WalkState->Opcode));

    /* Abort the parse, no need to examine this method any further */

    return (AE_CTRL_TERMINATE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsMethodError
 *
 * PARAMETERS:  Status          - Execution status
 *              WalkState       - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Called on method error. Invoke the global exception handler if
 *              present, dump the method data if the debugger is configured
 *
 *              Note: Allows the exception handler to change the status code
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsMethodError (
    ACPI_STATUS             Status,
    ACPI_WALK_STATE         *WalkState)
{
    UINT32                  AmlOffset;
    ACPI_NAME               Name = 0;


    ACPI_FUNCTION_ENTRY ();


    /* Ignore AE_OK and control exception codes */

    if (ACPI_SUCCESS (Status) ||
        (Status & AE_CODE_CONTROL))
    {
        return (Status);
    }

    /* Invoke the global exception handler */

    if (AcpiGbl_ExceptionHandler)
    {
        /* Exit the interpreter, allow handler to execute methods */

        AcpiExExitInterpreter ();

        /*
         * Handler can map the exception code to anything it wants, including
         * AE_OK, in which case the executing method will not be aborted.
         */
        AmlOffset = (UINT32) ACPI_PTR_DIFF (WalkState->Aml,
            WalkState->ParserState.AmlStart);

        if (WalkState->MethodNode)
        {
            Name = WalkState->MethodNode->Name.Integer;
        }
        else if (WalkState->DeferredNode)
        {
            Name = WalkState->DeferredNode->Name.Integer;
        }

        Status = AcpiGbl_ExceptionHandler (Status, Name,
            WalkState->Opcode, AmlOffset, NULL);
        AcpiExEnterInterpreter ();
    }

    AcpiDsClearImplicitReturn (WalkState);

    if (ACPI_FAILURE (Status))
    {
        AcpiDsDumpMethodStack (Status, WalkState, WalkState->Op);

        /* Display method locals/args if debugger is present */

#ifdef ACPI_DEBUGGER
        AcpiDbDumpMethodInfo (Status, WalkState);
#endif
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateMethodMutex
 *
 * PARAMETERS:  ObjDesc             - The method object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a mutex object for a serialized control method
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiDsCreateMethodMutex (
    ACPI_OPERAND_OBJECT     *MethodDesc)
{
    ACPI_OPERAND_OBJECT     *MutexDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (DsCreateMethodMutex);


    /* Create the new mutex object */

    MutexDesc = AcpiUtCreateInternalObject (ACPI_TYPE_MUTEX);
    if (!MutexDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Create the actual OS Mutex */

    Status = AcpiOsCreateMutex (&MutexDesc->Mutex.OsMutex);
    if (ACPI_FAILURE (Status))
    {
        AcpiUtDeleteObjectDesc (MutexDesc);
        return_ACPI_STATUS (Status);
    }

    MutexDesc->Mutex.SyncLevel = MethodDesc->Method.SyncLevel;
    MethodDesc->Method.Mutex = MutexDesc;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsBeginMethodExecution
 *
 * PARAMETERS:  MethodNode          - Node of the method
 *              ObjDesc             - The method object
 *              WalkState           - current state, NULL if not yet executing
 *                                    a method.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Prepare a method for execution. Parses the method if necessary,
 *              increments the thread count, and waits at the method semaphore
 *              for clearance to execute.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsBeginMethodExecution (
    ACPI_NAMESPACE_NODE     *MethodNode,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_PTR (DsBeginMethodExecution, MethodNode);


    if (!MethodNode)
    {
        return_ACPI_STATUS (AE_NULL_ENTRY);
    }

    AcpiExStartTraceMethod (MethodNode, ObjDesc, WalkState);

    /* Prevent wraparound of thread count */

    if (ObjDesc->Method.ThreadCount == ACPI_UINT8_MAX)
    {
        ACPI_ERROR ((AE_INFO,
            "Method reached maximum reentrancy limit (255)"));
        return_ACPI_STATUS (AE_AML_METHOD_LIMIT);
    }

    /*
     * If this method is serialized, we need to acquire the method mutex.
     */
    if (ObjDesc->Method.InfoFlags & ACPI_METHOD_SERIALIZED)
    {
        /*
         * Create a mutex for the method if it is defined to be Serialized
         * and a mutex has not already been created. We defer the mutex creation
         * until a method is actually executed, to minimize the object count
         */
        if (!ObjDesc->Method.Mutex)
        {
            Status = AcpiDsCreateMethodMutex (ObjDesc);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }
        }

        /*
         * The CurrentSyncLevel (per-thread) must be less than or equal to
         * the sync level of the method. This mechanism provides some
         * deadlock prevention.
         *
         * If the method was auto-serialized, we just ignore the sync level
         * mechanism, because auto-serialization of methods can interfere
         * with ASL code that actually uses sync levels.
         *
         * Top-level method invocation has no walk state at this point
         */
        if (WalkState &&
            (!(ObjDesc->Method.InfoFlags & ACPI_METHOD_IGNORE_SYNC_LEVEL)) &&
            (WalkState->Thread->CurrentSyncLevel >
                ObjDesc->Method.Mutex->Mutex.SyncLevel))
        {
            ACPI_ERROR ((AE_INFO,
                "Cannot acquire Mutex for method [%4.4s]"
                ", current SyncLevel is too large (%u)",
                AcpiUtGetNodeName (MethodNode),
                WalkState->Thread->CurrentSyncLevel));

            return_ACPI_STATUS (AE_AML_MUTEX_ORDER);
        }

        /*
         * Obtain the method mutex if necessary. Do not acquire mutex for a
         * recursive call.
         */
        if (!WalkState ||
            !ObjDesc->Method.Mutex->Mutex.ThreadId ||
            (WalkState->Thread->ThreadId !=
                ObjDesc->Method.Mutex->Mutex.ThreadId))
        {
            /*
             * Acquire the method mutex. This releases the interpreter if we
             * block (and reacquires it before it returns)
             */
            Status = AcpiExSystemWaitMutex (
                ObjDesc->Method.Mutex->Mutex.OsMutex, ACPI_WAIT_FOREVER);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }

            /* Update the mutex and walk info and save the original SyncLevel */

            if (WalkState)
            {
                ObjDesc->Method.Mutex->Mutex.OriginalSyncLevel =
                    WalkState->Thread->CurrentSyncLevel;

                ObjDesc->Method.Mutex->Mutex.ThreadId =
                    WalkState->Thread->ThreadId;

                /*
                 * Update the current SyncLevel only if this is not an auto-
                 * serialized method. In the auto case, we have to ignore
                 * the sync level for the method mutex (created for the
                 * auto-serialization) because we have no idea of what the
                 * sync level should be. Therefore, just ignore it.
                 */
                if (!(ObjDesc->Method.InfoFlags &
                    ACPI_METHOD_IGNORE_SYNC_LEVEL))
                {
                    WalkState->Thread->CurrentSyncLevel =
                        ObjDesc->Method.SyncLevel;
                }
            }
            else
            {
                ObjDesc->Method.Mutex->Mutex.OriginalSyncLevel =
                    ObjDesc->Method.Mutex->Mutex.SyncLevel;

                ObjDesc->Method.Mutex->Mutex.ThreadId =
                    AcpiOsGetThreadId ();
            }
        }

        /* Always increase acquisition depth */

        ObjDesc->Method.Mutex->Mutex.AcquisitionDepth++;
    }

    /*
     * Allocate an Owner ID for this method, only if this is the first thread
     * to begin concurrent execution. We only need one OwnerId, even if the
     * method is invoked recursively.
     */
    if (!ObjDesc->Method.OwnerId)
    {
        Status = AcpiUtAllocateOwnerId (&ObjDesc->Method.OwnerId);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }
    }

    /*
     * Increment the method parse tree thread count since it has been
     * reentered one more time (even if it is the same thread)
     */
    ObjDesc->Method.ThreadCount++;
    AcpiMethodCount++;
    return_ACPI_STATUS (Status);


Cleanup:
    /* On error, must release the method mutex (if present) */

    if (ObjDesc->Method.Mutex)
    {
        AcpiOsReleaseMutex (ObjDesc->Method.Mutex->Mutex.OsMutex);
    }
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCallControlMethod
 *
 * PARAMETERS:  Thread              - Info for this thread
 *              ThisWalkState       - Current walk state
 *              Op                  - Current Op to be walked
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transfer execution to a called control method
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsCallControlMethod (
    ACPI_THREAD_STATE       *Thread,
    ACPI_WALK_STATE         *ThisWalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *MethodNode;
    ACPI_WALK_STATE         *NextWalkState = NULL;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_EVALUATE_INFO      *Info;
    UINT32                  i;


    ACPI_FUNCTION_TRACE_PTR (DsCallControlMethod, ThisWalkState);

    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "Calling method %p, currentstate=%p\n",
        ThisWalkState->PrevOp, ThisWalkState));

    /*
     * Get the namespace entry for the control method we are about to call
     */
    MethodNode = ThisWalkState->MethodCallNode;
    if (!MethodNode)
    {
        return_ACPI_STATUS (AE_NULL_ENTRY);
    }

    ObjDesc = AcpiNsGetAttachedObject (MethodNode);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NULL_OBJECT);
    }

    /* Init for new method, possibly wait on method mutex */

    Status = AcpiDsBeginMethodExecution (
        MethodNode, ObjDesc, ThisWalkState);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Begin method parse/execution. Create a new walk state */

    NextWalkState = AcpiDsCreateWalkState (
        ObjDesc->Method.OwnerId, NULL, ObjDesc, Thread);
    if (!NextWalkState)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /*
     * The resolved arguments were put on the previous walk state's operand
     * stack. Operands on the previous walk state stack always
     * start at index 0. Also, null terminate the list of arguments
     */
    ThisWalkState->Operands [ThisWalkState->NumOperands] = NULL;

    /*
     * Allocate and initialize the evaluation information block
     * TBD: this is somewhat inefficient, should change interface to
     * DsInitAmlWalk. For now, keeps this struct off the CPU stack
     */
    Info = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_EVALUATE_INFO));
    if (!Info)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    Info->Parameters = &ThisWalkState->Operands[0];

    Status = AcpiDsInitAmlWalk (NextWalkState, NULL, MethodNode,
        ObjDesc->Method.AmlStart, ObjDesc->Method.AmlLength,
        Info, ACPI_IMODE_EXECUTE);

    ACPI_FREE (Info);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /*
     * Delete the operands on the previous walkstate operand stack
     * (they were copied to new objects)
     */
    for (i = 0; i < ObjDesc->Method.ParamCount; i++)
    {
        AcpiUtRemoveReference (ThisWalkState->Operands [i]);
        ThisWalkState->Operands [i] = NULL;
    }

    /* Clear the operand stack */

    ThisWalkState->NumOperands = 0;

    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "**** Begin nested execution of [%4.4s] **** WalkState=%p\n",
        MethodNode->Name.Ascii, NextWalkState));

    /* Invoke an internal method if necessary */

    if (ObjDesc->Method.InfoFlags & ACPI_METHOD_INTERNAL_ONLY)
    {
        Status = ObjDesc->Method.Dispatch.Implementation (NextWalkState);
        if (Status == AE_OK)
        {
            Status = AE_CTRL_TERMINATE;
        }
    }

    return_ACPI_STATUS (Status);


Cleanup:

    /* On error, we must terminate the method properly */

    AcpiDsTerminateControlMethod (ObjDesc, NextWalkState);
    AcpiDsDeleteWalkState (NextWalkState);

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsRestartControlMethod
 *
 * PARAMETERS:  WalkState           - State for preempted method (caller)
 *              ReturnDesc          - Return value from the called method
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Restart a method that was preempted by another (nested) method
 *              invocation. Handle the return value (if any) from the callee.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsRestartControlMethod (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     *ReturnDesc)
{
    ACPI_STATUS             Status;
    int                     SameAsImplicitReturn;


    ACPI_FUNCTION_TRACE_PTR (DsRestartControlMethod, WalkState);


    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "****Restart [%4.4s] Op %p ReturnValueFromCallee %p\n",
        AcpiUtGetNodeName (WalkState->MethodNode),
        WalkState->MethodCallOp, ReturnDesc));

    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "    ReturnFromThisMethodUsed?=%X ResStack %p Walk %p\n",
        WalkState->ReturnUsed,
        WalkState->Results, WalkState));

    /* Did the called method return a value? */

    if (ReturnDesc)
    {
        /* Is the implicit return object the same as the return desc? */

        SameAsImplicitReturn = (WalkState->ImplicitReturnObj == ReturnDesc);

        /* Are we actually going to use the return value? */

        if (WalkState->ReturnUsed)
        {
            /* Save the return value from the previous method */

            Status = AcpiDsResultPush (ReturnDesc, WalkState);
            if (ACPI_FAILURE (Status))
            {
                AcpiUtRemoveReference (ReturnDesc);
                return_ACPI_STATUS (Status);
            }

            /*
             * Save as THIS method's return value in case it is returned
             * immediately to yet another method
             */
            WalkState->ReturnDesc = ReturnDesc;
        }

        /*
         * The following code is the optional support for the so-called
         * "implicit return". Some AML code assumes that the last value of the
         * method is "implicitly" returned to the caller, in the absence of an
         * explicit return value.
         *
         * Just save the last result of the method as the return value.
         *
         * NOTE: this is optional because the ASL language does not actually
         * support this behavior.
         */
        else if (!AcpiDsDoImplicitReturn (ReturnDesc, WalkState, FALSE) ||
                 SameAsImplicitReturn)
        {
            /*
             * Delete the return value if it will not be used by the
             * calling method or remove one reference if the explicit return
             * is the same as the implicit return value.
             */
            AcpiUtRemoveReference (ReturnDesc);
        }
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsTerminateControlMethod
 *
 * PARAMETERS:  MethodDesc          - Method object
 *              WalkState           - State associated with the method
 *
 * RETURN:      None
 *
 * DESCRIPTION: Terminate a control method. Delete everything that the method
 *              created, delete all locals and arguments, and delete the parse
 *              tree if requested.
 *
 * MUTEX:       Interpreter is locked
 *
 ******************************************************************************/

void
AcpiDsTerminateControlMethod (
    ACPI_OPERAND_OBJECT     *MethodDesc,
    ACPI_WALK_STATE         *WalkState)
{

    ACPI_FUNCTION_TRACE_PTR (DsTerminateControlMethod, WalkState);


    /* MethodDesc is required, WalkState is optional */

    if (!MethodDesc)
    {
        return_VOID;
    }

    if (WalkState)
    {
        /* Delete all arguments and locals */

        AcpiDsMethodDataDeleteAll (WalkState);

        /*
         * Delete any namespace objects created anywhere within the
         * namespace by the execution of this method. Unless:
         * 1) This method is a module-level executable code method, in which
         *    case we want make the objects permanent.
         * 2) There are other threads executing the method, in which case we
         *    will wait until the last thread has completed.
         */
        if (!(MethodDesc->Method.InfoFlags & ACPI_METHOD_MODULE_LEVEL) &&
             (MethodDesc->Method.ThreadCount == 1))
        {
            /* Delete any direct children of (created by) this method */

            (void) AcpiExExitInterpreter ();
            AcpiNsDeleteNamespaceSubtree (WalkState->MethodNode);
            (void) AcpiExEnterInterpreter ();

            /*
             * Delete any objects that were created by this method
             * elsewhere in the namespace (if any were created).
             * Use of the ACPI_METHOD_MODIFIED_NAMESPACE optimizes the
             * deletion such that we don't have to perform an entire
             * namespace walk for every control method execution.
             */
            if (MethodDesc->Method.InfoFlags & ACPI_METHOD_MODIFIED_NAMESPACE)
            {
                (void) AcpiExExitInterpreter ();
                AcpiNsDeleteNamespaceByOwner (MethodDesc->Method.OwnerId);
                (void) AcpiExEnterInterpreter ();
                MethodDesc->Method.InfoFlags &=
                    ~ACPI_METHOD_MODIFIED_NAMESPACE;
            }
        }

        /*
         * If method is serialized, release the mutex and restore the
         * current sync level for this thread
         */
        if (MethodDesc->Method.Mutex)
        {
            /* Acquisition Depth handles recursive calls */

            MethodDesc->Method.Mutex->Mutex.AcquisitionDepth--;
            if (!MethodDesc->Method.Mutex->Mutex.AcquisitionDepth)
            {
                WalkState->Thread->CurrentSyncLevel =
                    MethodDesc->Method.Mutex->Mutex.OriginalSyncLevel;

                AcpiOsReleaseMutex (
                    MethodDesc->Method.Mutex->Mutex.OsMutex);
                MethodDesc->Method.Mutex->Mutex.ThreadId = 0;
            }
        }
    }

    /* Decrement the thread count on the method */

    if (MethodDesc->Method.ThreadCount)
    {
        MethodDesc->Method.ThreadCount--;
    }
    else
    {
        ACPI_ERROR ((AE_INFO,
            "Invalid zero thread count in method"));
    }

    /* Are there any other threads currently executing this method? */

    if (MethodDesc->Method.ThreadCount)
    {
        /*
         * Additional threads. Do not release the OwnerId in this case,
         * we immediately reuse it for the next thread executing this method
         */
        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "*** Completed execution of one thread, %u threads remaining\n",
            MethodDesc->Method.ThreadCount));
    }
    else
    {
        /* This is the only executing thread for this method */

        /*
         * Support to dynamically change a method from NotSerialized to
         * Serialized if it appears that the method is incorrectly written and
         * does not support multiple thread execution. The best example of this
         * is if such a method creates namespace objects and blocks. A second
         * thread will fail with an AE_ALREADY_EXISTS exception.
         *
         * This code is here because we must wait until the last thread exits
         * before marking the method as serialized.
         */
        if (MethodDesc->Method.InfoFlags & ACPI_METHOD_SERIALIZED_PENDING)
        {
            if (WalkState)
            {
                ACPI_INFO ((
                    "Marking method %4.4s as Serialized "
                    "because of AE_ALREADY_EXISTS error",
                    WalkState->MethodNode->Name.Ascii));
            }

            /*
             * Method tried to create an object twice and was marked as
             * "pending serialized". The probable cause is that the method
             * cannot handle reentrancy.
             *
             * The method was created as NotSerialized, but it tried to create
             * a named object and then blocked, causing the second thread
             * entrance to begin and then fail. Workaround this problem by
             * marking the method permanently as Serialized when the last
             * thread exits here.
             */
            MethodDesc->Method.InfoFlags &=
                ~ACPI_METHOD_SERIALIZED_PENDING;

            MethodDesc->Method.InfoFlags |=
                (ACPI_METHOD_SERIALIZED | ACPI_METHOD_IGNORE_SYNC_LEVEL);
            MethodDesc->Method.SyncLevel = 0;
        }

        /* No more threads, we can free the OwnerId */

        if (!(MethodDesc->Method.InfoFlags & ACPI_METHOD_MODULE_LEVEL))
        {
            AcpiUtReleaseOwnerId (&MethodDesc->Method.OwnerId);
        }
    }

    AcpiExStopTraceMethod ((ACPI_NAMESPACE_NODE *) MethodDesc->Method.Node,
        MethodDesc, WalkState);

    return_VOID;
}
