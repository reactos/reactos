/******************************************************************************
 *
 * Module Name: nswalk - Functions for walking the ACPI namespace
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
#include "acnamesp.h"


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nswalk")


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetNextNode
 *
 * PARAMETERS:  ParentNode          - Parent node whose children we are
 *                                    getting
 *              ChildNode           - Previous child that was found.
 *                                    The NEXT child will be returned
 *
 * RETURN:      ACPI_NAMESPACE_NODE - Pointer to the NEXT child or NULL if
 *                                    none is found.
 *
 * DESCRIPTION: Return the next peer node within the namespace. If Handle
 *              is valid, Scope is ignored. Otherwise, the first node
 *              within Scope is returned.
 *
 ******************************************************************************/

ACPI_NAMESPACE_NODE *
AcpiNsGetNextNode (
    ACPI_NAMESPACE_NODE     *ParentNode,
    ACPI_NAMESPACE_NODE     *ChildNode)
{
    ACPI_FUNCTION_ENTRY ();


    if (!ChildNode)
    {
        /* It's really the parent's _scope_ that we want */

        return (ParentNode->Child);
    }

    /* Otherwise just return the next peer */

    return (ChildNode->Peer);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetNextNodeTyped
 *
 * PARAMETERS:  Type                - Type of node to be searched for
 *              ParentNode          - Parent node whose children we are
 *                                    getting
 *              ChildNode           - Previous child that was found.
 *                                    The NEXT child will be returned
 *
 * RETURN:      ACPI_NAMESPACE_NODE - Pointer to the NEXT child or NULL if
 *                                    none is found.
 *
 * DESCRIPTION: Return the next peer node within the namespace. If Handle
 *              is valid, Scope is ignored. Otherwise, the first node
 *              within Scope is returned.
 *
 ******************************************************************************/

ACPI_NAMESPACE_NODE *
AcpiNsGetNextNodeTyped (
    ACPI_OBJECT_TYPE        Type,
    ACPI_NAMESPACE_NODE     *ParentNode,
    ACPI_NAMESPACE_NODE     *ChildNode)
{
    ACPI_NAMESPACE_NODE     *NextNode = NULL;


    ACPI_FUNCTION_ENTRY ();


    NextNode = AcpiNsGetNextNode (ParentNode, ChildNode);

    /* If any type is OK, we are done */

    if (Type == ACPI_TYPE_ANY)
    {
        /* NextNode is NULL if we are at the end-of-list */

        return (NextNode);
    }

    /* Must search for the node -- but within this scope only */

    while (NextNode)
    {
        /* If type matches, we are done */

        if (NextNode->Type == Type)
        {
            return (NextNode);
        }

        /* Otherwise, move on to the next peer node */

        NextNode = NextNode->Peer;
    }

    /* Not found */

    return (NULL);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsWalkNamespace
 *
 * PARAMETERS:  Type                - ACPI_OBJECT_TYPE to search for
 *              StartNode           - Handle in namespace where search begins
 *              MaxDepth            - Depth to which search is to reach
 *              Flags               - Whether to unlock the NS before invoking
 *                                    the callback routine
 *              DescendingCallback  - Called during tree descent
 *                                    when an object of "Type" is found
 *              AscendingCallback   - Called during tree ascent
 *                                    when an object of "Type" is found
 *              Context             - Passed to user function(s) above
 *              ReturnValue         - from the UserFunction if terminated
 *                                    early. Otherwise, returns NULL.
 * RETURNS:     Status
 *
 * DESCRIPTION: Performs a modified depth-first walk of the namespace tree,
 *              starting (and ending) at the node specified by StartHandle.
 *              The callback function is called whenever a node that matches
 *              the type parameter is found. If the callback function returns
 *              a non-zero value, the search is terminated immediately and
 *              this value is returned to the caller.
 *
 *              The point of this procedure is to provide a generic namespace
 *              walk routine that can be called from multiple places to
 *              provide multiple services; the callback function(s) can be
 *              tailored to each task, whether it is a print function,
 *              a compare function, etc.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsWalkNamespace (
    ACPI_OBJECT_TYPE        Type,
    ACPI_HANDLE             StartNode,
    UINT32                  MaxDepth,
    UINT32                  Flags,
    ACPI_WALK_CALLBACK      DescendingCallback,
    ACPI_WALK_CALLBACK      AscendingCallback,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_STATUS             Status;
    ACPI_STATUS             MutexStatus;
    ACPI_NAMESPACE_NODE     *ChildNode;
    ACPI_NAMESPACE_NODE     *ParentNode;
    ACPI_OBJECT_TYPE        ChildType;
    UINT32                  Level;
    BOOLEAN                 NodePreviouslyVisited = FALSE;


    ACPI_FUNCTION_TRACE (NsWalkNamespace);


    /* Special case for the namespace Root Node */

    if (StartNode == ACPI_ROOT_OBJECT)
    {
        StartNode = AcpiGbl_RootNode;
    }

    /* Null child means "get first node" */

    ParentNode = StartNode;
    ChildNode = AcpiNsGetNextNode (ParentNode, NULL);
    ChildType = ACPI_TYPE_ANY;
    Level = 1;

    /*
     * Traverse the tree of nodes until we bubble back up to where we
     * started. When Level is zero, the loop is done because we have
     * bubbled up to (and passed) the original parent handle (StartEntry)
     */
    while (Level > 0 && ChildNode)
    {
        Status = AE_OK;

        /* Found next child, get the type if we are not searching for ANY */

        if (Type != ACPI_TYPE_ANY)
        {
            ChildType = ChildNode->Type;
        }

        /*
         * Ignore all temporary namespace nodes (created during control
         * method execution) unless told otherwise. These temporary nodes
         * can cause a race condition because they can be deleted during
         * the execution of the user function (if the namespace is
         * unlocked before invocation of the user function.) Only the
         * debugger namespace dump will examine the temporary nodes.
         */
        if ((ChildNode->Flags & ANOBJ_TEMPORARY) &&
            !(Flags & ACPI_NS_WALK_TEMP_NODES))
        {
            Status = AE_CTRL_DEPTH;
        }

        /* Type must match requested type */

        else if (ChildType == Type)
        {
            /*
             * Found a matching node, invoke the user callback function.
             * Unlock the namespace if flag is set.
             */
            if (Flags & ACPI_NS_WALK_UNLOCK)
            {
                MutexStatus = AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
                if (ACPI_FAILURE (MutexStatus))
                {
                    return_ACPI_STATUS (MutexStatus);
                }
            }

            /*
             * Invoke the user function, either descending, ascending,
             * or both.
             */
            if (!NodePreviouslyVisited)
            {
                if (DescendingCallback)
                {
                    Status = DescendingCallback (ChildNode, Level,
                        Context, ReturnValue);
                }
            }
            else
            {
                if (AscendingCallback)
                {
                    Status = AscendingCallback (ChildNode, Level,
                        Context, ReturnValue);
                }
            }

            if (Flags & ACPI_NS_WALK_UNLOCK)
            {
                MutexStatus = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
                if (ACPI_FAILURE (MutexStatus))
                {
                    return_ACPI_STATUS (MutexStatus);
                }
            }

            switch (Status)
            {
            case AE_OK:
            case AE_CTRL_DEPTH:

                /* Just keep going */
                break;

            case AE_CTRL_TERMINATE:

                /* Exit now, with OK status */

                return_ACPI_STATUS (AE_OK);

            default:

                /* All others are valid exceptions */

                return_ACPI_STATUS (Status);
            }
        }

        /*
         * Depth first search: Attempt to go down another level in the
         * namespace if we are allowed to. Don't go any further if we have
         * reached the caller specified maximum depth or if the user
         * function has specified that the maximum depth has been reached.
         */
        if (!NodePreviouslyVisited &&
            (Level < MaxDepth) &&
            (Status != AE_CTRL_DEPTH))
        {
            if (ChildNode->Child)
            {
                /* There is at least one child of this node, visit it */

                Level++;
                ParentNode = ChildNode;
                ChildNode = AcpiNsGetNextNode (ParentNode, NULL);
                continue;
            }
        }

        /* No more children, re-visit this node */

        if (!NodePreviouslyVisited)
        {
            NodePreviouslyVisited = TRUE;
            continue;
        }

        /* No more children, visit peers */

        ChildNode = AcpiNsGetNextNode (ParentNode, ChildNode);
        if (ChildNode)
        {
            NodePreviouslyVisited = FALSE;
        }

        /* No peers, re-visit parent */

        else
        {
            /*
             * No more children of this node (AcpiNsGetNextNode failed), go
             * back upwards in the namespace tree to the node's parent.
             */
            Level--;
            ChildNode = ParentNode;
            ParentNode = ParentNode->Parent;

            NodePreviouslyVisited = TRUE;
        }
    }

    /* Complete walk, not terminated by user function */

    return_ACPI_STATUS (AE_OK);
}
