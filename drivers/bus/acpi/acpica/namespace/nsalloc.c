/*******************************************************************************
 *
 * Module Name: nsalloc - Namespace allocation and deletion utilities
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2020, Intel Corp.
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
        ACPI_MODULE_NAME    ("nsalloc")


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsCreateNode
 *
 * PARAMETERS:  Name            - Name of the new node (4 char ACPI name)
 *
 * RETURN:      New namespace node (Null on failure)
 *
 * DESCRIPTION: Create a namespace node
 *
 ******************************************************************************/

ACPI_NAMESPACE_NODE *
AcpiNsCreateNode (
    UINT32                  Name)
{
    ACPI_NAMESPACE_NODE     *Node;
#ifdef ACPI_DBG_TRACK_ALLOCATIONS
    UINT32                  Temp;
#endif


    ACPI_FUNCTION_TRACE (NsCreateNode);


    Node = AcpiOsAcquireObject (AcpiGbl_NamespaceCache);
    if (!Node)
    {
        return_PTR (NULL);
    }

    ACPI_MEM_TRACKING (AcpiGbl_NsNodeList->TotalAllocated++);

#ifdef ACPI_DBG_TRACK_ALLOCATIONS
        Temp = AcpiGbl_NsNodeList->TotalAllocated -
            AcpiGbl_NsNodeList->TotalFreed;
        if (Temp > AcpiGbl_NsNodeList->MaxOccupied)
        {
            AcpiGbl_NsNodeList->MaxOccupied = Temp;
        }
#endif

    Node->Name.Integer = Name;
    ACPI_SET_DESCRIPTOR_TYPE (Node, ACPI_DESC_TYPE_NAMED);
    return_PTR (Node);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsDeleteNode
 *
 * PARAMETERS:  Node            - Node to be deleted
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete a namespace node. All node deletions must come through
 *              here. Detaches any attached objects, including any attached
 *              data. If a handler is associated with attached data, it is
 *              invoked before the node is deleted.
 *
 ******************************************************************************/

void
AcpiNsDeleteNode (
    ACPI_NAMESPACE_NODE     *Node)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *NextDesc;


    ACPI_FUNCTION_NAME (NsDeleteNode);


    if (!Node)
    {
        return_VOID;
    }

    /* Detach an object if there is one */

    AcpiNsDetachObject (Node);

    /*
     * Delete an attached data object list if present (objects that were
     * attached via AcpiAttachData). Note: After any normal object is
     * detached above, the only possible remaining object(s) are data
     * objects, in a linked list.
     */
    ObjDesc = Node->Object;
    while (ObjDesc &&
        (ObjDesc->Common.Type == ACPI_TYPE_LOCAL_DATA))
    {
        /* Invoke the attached data deletion handler if present */

        if (ObjDesc->Data.Handler)
        {
            ObjDesc->Data.Handler (Node, ObjDesc->Data.Pointer);
        }

        NextDesc = ObjDesc->Common.NextObject;
        AcpiUtRemoveReference (ObjDesc);
        ObjDesc = NextDesc;
    }

    /* Special case for the statically allocated root node */

    if (Node == AcpiGbl_RootNode)
    {
        return;
    }

    /* Now we can delete the node */

    (void) AcpiOsReleaseObject (AcpiGbl_NamespaceCache, Node);

    ACPI_MEM_TRACKING (AcpiGbl_NsNodeList->TotalFreed++);
    ACPI_DEBUG_PRINT ((ACPI_DB_ALLOCATIONS, "Node %p, Remaining %X\n",
        Node, AcpiGbl_CurrentNodeCount));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsRemoveNode
 *
 * PARAMETERS:  Node            - Node to be removed/deleted
 *
 * RETURN:      None
 *
 * DESCRIPTION: Remove (unlink) and delete a namespace node
 *
 ******************************************************************************/

void
AcpiNsRemoveNode (
    ACPI_NAMESPACE_NODE     *Node)
{
    ACPI_NAMESPACE_NODE     *ParentNode;
    ACPI_NAMESPACE_NODE     *PrevNode;
    ACPI_NAMESPACE_NODE     *NextNode;


    ACPI_FUNCTION_TRACE_PTR (NsRemoveNode, Node);


    ParentNode = Node->Parent;

    PrevNode = NULL;
    NextNode = ParentNode->Child;

    /* Find the node that is the previous peer in the parent's child list */

    while (NextNode != Node)
    {
        PrevNode = NextNode;
        NextNode = NextNode->Peer;
    }

    if (PrevNode)
    {
        /* Node is not first child, unlink it */

        PrevNode->Peer = Node->Peer;
    }
    else
    {
        /*
         * Node is first child (has no previous peer).
         * Link peer list to parent
         */
        ParentNode->Child = Node->Peer;
    }

    /* Delete the node and any attached objects */

    AcpiNsDeleteNode (Node);
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsInstallNode
 *
 * PARAMETERS:  WalkState       - Current state of the walk
 *              ParentNode      - The parent of the new Node
 *              Node            - The new Node to install
 *              Type            - ACPI object type of the new Node
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize a new namespace node and install it amongst
 *              its peers.
 *
 *              Note: Current namespace lookup is linear search. This appears
 *              to be sufficient as namespace searches consume only a small
 *              fraction of the execution time of the ACPI subsystem.
 *
 ******************************************************************************/

void
AcpiNsInstallNode (
    ACPI_WALK_STATE         *WalkState,
    ACPI_NAMESPACE_NODE     *ParentNode,    /* Parent */
    ACPI_NAMESPACE_NODE     *Node,          /* New Child*/
    ACPI_OBJECT_TYPE        Type)
{
    ACPI_OWNER_ID           OwnerId = 0;
    ACPI_NAMESPACE_NODE     *ChildNode;


    ACPI_FUNCTION_TRACE (NsInstallNode);


    if (WalkState)
    {
        /*
         * Get the owner ID from the Walk state. The owner ID is used to
         * track table deletion and deletion of objects created by methods.
         */
        OwnerId = WalkState->OwnerId;

        if ((WalkState->MethodDesc) &&
            (ParentNode != WalkState->MethodNode))
        {
            /*
             * A method is creating a new node that is not a child of the
             * method (it is non-local). Mark the executing method as having
             * modified the namespace. This is used for cleanup when the
             * method exits.
             */
            WalkState->MethodDesc->Method.InfoFlags |=
                ACPI_METHOD_MODIFIED_NAMESPACE;
        }
    }

    /* Link the new entry into the parent and existing children */

    Node->Peer = NULL;
    Node->Parent = ParentNode;
    ChildNode = ParentNode->Child;

    if (!ChildNode)
    {
        ParentNode->Child = Node;
    }
    else
    {
        /* Add node to the end of the peer list */

        while (ChildNode->Peer)
        {
            ChildNode = ChildNode->Peer;
        }

        ChildNode->Peer = Node;
    }

    /* Init the new entry */

    Node->OwnerId = OwnerId;
    Node->Type = (UINT8) Type;

    ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
        "%4.4s (%s) [Node %p Owner %3.3X] added to %4.4s (%s) [Node %p]\n",
        AcpiUtGetNodeName (Node), AcpiUtGetTypeName (Node->Type), Node, OwnerId,
        AcpiUtGetNodeName (ParentNode), AcpiUtGetTypeName (ParentNode->Type),
        ParentNode));

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsDeleteChildren
 *
 * PARAMETERS:  ParentNode      - Delete this objects children
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete all children of the parent object. In other words,
 *              deletes a "scope".
 *
 ******************************************************************************/

void
AcpiNsDeleteChildren (
    ACPI_NAMESPACE_NODE     *ParentNode)
{
    ACPI_NAMESPACE_NODE     *NextNode;
    ACPI_NAMESPACE_NODE     *NodeToDelete;


    ACPI_FUNCTION_TRACE_PTR (NsDeleteChildren, ParentNode);


    if (!ParentNode)
    {
        return_VOID;
    }

    /* Deallocate all children at this level */

    NextNode = ParentNode->Child;
    while (NextNode)
    {
        /* Grandchildren should have all been deleted already */

        if (NextNode->Child)
        {
            ACPI_ERROR ((AE_INFO, "Found a grandchild! P=%p C=%p",
                ParentNode, NextNode));
        }

        /*
         * Delete this child node and move on to the next child in the list.
         * No need to unlink the node since we are deleting the entire branch.
         */
        NodeToDelete = NextNode;
        NextNode = NextNode->Peer;
        AcpiNsDeleteNode (NodeToDelete);
    }

    /* Clear the parent's child pointer */

    ParentNode->Child = NULL;
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsDeleteNamespaceSubtree
 *
 * PARAMETERS:  ParentNode      - Root of the subtree to be deleted
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a subtree of the namespace. This includes all objects
 *              stored within the subtree.
 *
 ******************************************************************************/

void
AcpiNsDeleteNamespaceSubtree (
    ACPI_NAMESPACE_NODE     *ParentNode)
{
    ACPI_NAMESPACE_NODE     *ChildNode = NULL;
    UINT32                  Level = 1;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (NsDeleteNamespaceSubtree);


    if (!ParentNode)
    {
        return_VOID;
    }

    /* Lock namespace for possible update */

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_VOID;
    }

    /*
     * Traverse the tree of objects until we bubble back up
     * to where we started.
     */
    while (Level > 0)
    {
        /* Get the next node in this scope (NULL if none) */

        ChildNode = AcpiNsGetNextNode (ParentNode, ChildNode);
        if (ChildNode)
        {
            /* Found a child node - detach any attached object */

            AcpiNsDetachObject (ChildNode);

            /* Check if this node has any children */

            if (ChildNode->Child)
            {
                /*
                 * There is at least one child of this node,
                 * visit the node
                 */
                Level++;
                ParentNode = ChildNode;
                ChildNode  = NULL;
            }
        }
        else
        {
            /*
             * No more children of this parent node.
             * Move up to the grandparent.
             */
            Level--;

            /*
             * Now delete all of the children of this parent
             * all at the same time.
             */
            AcpiNsDeleteChildren (ParentNode);

            /* New "last child" is this parent node */

            ChildNode = ParentNode;

            /* Move up the tree to the grandparent */

            ParentNode = ParentNode->Parent;
        }
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsDeleteNamespaceByOwner
 *
 * PARAMETERS:  OwnerId     - All nodes with this owner will be deleted
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete entries within the namespace that are owned by a
 *              specific ID. Used to delete entire ACPI tables. All
 *              reference counts are updated.
 *
 * MUTEX:       Locks namespace during deletion walk.
 *
 ******************************************************************************/

void
AcpiNsDeleteNamespaceByOwner (
    ACPI_OWNER_ID            OwnerId)
{
    ACPI_NAMESPACE_NODE     *ChildNode;
    ACPI_NAMESPACE_NODE     *DeletionNode;
    ACPI_NAMESPACE_NODE     *ParentNode;
    UINT32                  Level;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_U32 (NsDeleteNamespaceByOwner, OwnerId);


    if (OwnerId == 0)
    {
        return_VOID;
    }

    /* Lock namespace for possible update */

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_VOID;
    }

    DeletionNode = NULL;
    ParentNode = AcpiGbl_RootNode;
    ChildNode = NULL;
    Level = 1;

    /*
     * Traverse the tree of nodes until we bubble back up
     * to where we started.
     */
    while (Level > 0)
    {
        /*
         * Get the next child of this parent node. When ChildNode is NULL,
         * the first child of the parent is returned
         */
        ChildNode = AcpiNsGetNextNode (ParentNode, ChildNode);

        if (DeletionNode)
        {
            AcpiNsDeleteChildren (DeletionNode);
            AcpiNsRemoveNode (DeletionNode);
            DeletionNode = NULL;
        }

        if (ChildNode)
        {
            if (ChildNode->OwnerId == OwnerId)
            {
                /* Found a matching child node - detach any attached object */

                AcpiNsDetachObject (ChildNode);
            }

            /* Check if this node has any children */

            if (ChildNode->Child)
            {
                /*
                 * There is at least one child of this node,
                 * visit the node
                 */
                Level++;
                ParentNode = ChildNode;
                ChildNode  = NULL;
            }
            else if (ChildNode->OwnerId == OwnerId)
            {
                DeletionNode = ChildNode;
            }
        }
        else
        {
            /*
             * No more children of this parent node.
             * Move up to the grandparent.
             */
            Level--;
            if (Level != 0)
            {
                if (ParentNode->OwnerId == OwnerId)
                {
                    DeletionNode = ParentNode;
                }
            }

            /* New "last child" is this parent node */

            ChildNode = ParentNode;

            /* Move up the tree to the grandparent */

            ParentNode = ParentNode->Parent;
        }
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_VOID;
}
