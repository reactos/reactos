/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/route.c
 * PURPOSE:     Route cache
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:       The route cache is implemented as a binary search
 *              tree to obtain fast searches
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <route.h>
#include <router.h>


/* This RCN is shared by all external nodes. It complicates things,
   but the memory.requirements are reduced by approximately 50%.
   The RCN is protected by the route cache spin lock */
PROUTE_CACHE_NODE ExternalRCN;
PROUTE_CACHE_NODE RouteCache;
KSPIN_LOCK RouteCacheLock;


#if DBG
VOID PrintTree(
    PROUTE_CACHE_NODE Node)
/*
 * FUNCTION: Prints all nodes on tree
 * ARGUMENTS:
 *     Node = Pointer to root node of tree
 * NOTES:
 *     This function must be called with the route cache lock held.
 */
{
    if (IsInternalRCN(Node)) {
        /* Traverse left subtree */
        PrintTree(Node->Left);

        /* Traverse right subtree */
        PrintTree(Node->Right);

        /* Finally check the node itself */
        TI_DbgPrint(MIN_TRACE, ("(Internal) Self,Parent,Left,Right,Data = (%08X, %08X, %08X, %08X, %08X).\n",
            Node, Node->Parent, Node->Left, Node->Right, (ULONG_PTR)Node->Destination.Address.IPv4Address));
    } else
        TI_DbgPrint(MIN_TRACE, ("(External) Self,Parent,Left,Right = (%08X, %08X, %08X, %08X).\n",
            Node, Node->Parent, Node->Left, Node->Right));
}
#endif


VOID RemoveAboveExternal(VOID)
/*
 * FUNCTION: Removes the parent node of the selected external node from the route cache tree
 * NOTES:
 *     This function must be called with the route cache lock held.
 *     ExternalRCN->Parent must be initialized
 */
{
    PROUTE_CACHE_NODE Parent;
    PROUTE_CACHE_NODE Sibling;

    TI_DbgPrint(DEBUG_RCACHE, ("Called.\n"));

#if 0
    TI_DbgPrint(MIN_TRACE, ("Displaying tree (before).\n"));
    PrintTree(RouteCache);
#endif

    Parent = ExternalRCN->Parent;
    /* Find sibling of external node */
    if (ExternalRCN == Parent->Left)
        Sibling = Parent->Right;
    else
        Sibling = Parent->Left;

    /* Replace parent node with sibling of external node */
    if (Parent != RouteCache) {
        if (Parent->Parent->Left == Parent)
            Parent->Parent->Left = Sibling;
        else
            Parent->Parent->Right = Sibling;
        /* Give sibling a new parent */
        Sibling->Parent = Parent->Parent;
    } else {
        /* This is the root we're removing */
        RouteCache      = Sibling;
        Sibling->Parent = NULL;
    }

    DereferenceObject(Parent);

#if 0
    TI_DbgPrint(MIN_TRACE, ("Displaying tree (after).\n"));
    PrintTree(RouteCache);
#endif
}


PROUTE_CACHE_NODE SearchRouteCache(
    PIP_ADDRESS Destination,
    PROUTE_CACHE_NODE Node)
/*
 * FUNCTION: Searches route cache for a RCN for a destination address
 * ARGUMENTS:
 *     Destination = Pointer to destination address (key)
 *     Node        = Pointer to start route cache node
 * NOTES:
 *     This function must be called with the route cache lock held
 * RETURNS:
 *     Pointer to internal node if a matching node was found, or
 *     external node where it should be if none was found
 */
{
    INT Value;

    TI_DbgPrint(DEBUG_RCACHE, ("Called. Destination (0x%X)  Node (0x%X)\n", Destination, Node));

    /* Is this an external node? */
    if (IsExternalRCN(Node))
        return Node;

    /* Is it this node we are looking for? */
    Value = AddrCompare(Destination, &Node->Destination);
    if (Value == 0)
        return Node;

    /* Traverse down the left subtree if the key is smaller than
       the key of the node, otherwise traverse the right subtree */
    if (Value < 0) {
        Node->Left->Parent = Node;
        ExternalRCN->Left  = (PROUTE_CACHE_NODE)&Node->Left;
        return SearchRouteCache(Destination, Node->Left);
    } else {
        Node->Right->Parent = Node;
        ExternalRCN->Left   = (PROUTE_CACHE_NODE)&Node->Right;
        return SearchRouteCache(Destination, Node->Right);
    }
}


PROUTE_CACHE_NODE ExpandExternalRCN(VOID)
/*
 * FUNCTION: Expands an external route cache node
 * NOTES:
 *     This function must be called with the route cache lock held.
 *     We cheat a little here to save memory. We don't actually allocate memory
 *     for external nodes. We wait until they're turned into internal nodes.
 *     ExternalRCN->Parent must be initialized
 *     ExternalRCN->Left must be a pointer to the correct child link of it's parent
 * RETURNS:
 *     Pointer to new internal node if the external node was expanded, NULL if not
 */
{
    PROUTE_CACHE_NODE RCN;

    TI_DbgPrint(DEBUG_RCACHE, ("Called.\n"));

    RCN = PoolAllocateBuffer(sizeof(ROUTE_CACHE_NODE));
    if (!RCN) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    if (ExternalRCN->Left)
        /* Register RCN as a child with it's parent */
        *(PROUTE_CACHE_NODE*)ExternalRCN->Left = RCN;

    RCN->Parent = ExternalRCN->Parent;
    RCN->Left   = ExternalRCN;
    RCN->Right  = ExternalRCN;

    return RCN;
}

#if 0
VOID SwapRCN(
    PROUTE_CACHE_NODE *Node1,
    PROUTE_CACHE_NODE *Node2)
/*
 * FUNCTION: Swaps two nodes
 * ARGUMENTS:
 *     Node1 = Address of pointer to first node
 *     Node2 = Address of pointer to second node
 */
{
    PROUTE_CACHE_NODE Temp;

    Temp  = *Node2;
    *Node2 = *Node1;
    *Node1 = Temp;
}
#endif

/*
 * FUNCTION: Removes a route to a destination
 * ARGUMENTS:
 *     RCN = Pointer to route cache node to remove
 * NOTES:
 *     Internal version. Route cache lock must be held
 */
VOID RemoveRouteToDestination(
    PROUTE_CACHE_NODE RCN)
{
    PROUTE_CACHE_NODE RemNode, Parent, SwapNode;

    TI_DbgPrint(DEBUG_RCACHE, ("Called. RCN (0x%X).\n", RCN));

    if (IsExternalRCN(RCN->Left)) {
        /* Left node is external */
        RemNode         = RCN->Left;
        RemNode->Parent = RCN;
    } else if (IsExternalRCN(RCN->Right)) {
        /* Right node is external */
        RemNode         = RCN->Right;
        RemNode->Parent = RCN;
    } else {
        /* The node has internal children */
#if 0
        /* Normally we would replace
           the item of RCN with the item of the leftmost external
           node on the right subtree of RCN. This we cannot do here
           because there may be references directly to that node.
           Instead we swap pointer values (parent, left and right)
           of the two nodes */
#endif
        RemNode = RCN->Right;
        do {
            Parent  = RemNode;
            RemNode = RemNode->Left;
        } while (IsInternalRCN(RemNode));
        RemNode->Parent = Parent;

        SwapNode = RemNode->Parent;
#if 0
        if (RCN != RouteCache) {
            /* Set SwapNode to be child of RCN's parent instead of RCN */
            Parent = RCN->Parent;
            if (RCN == Parent->Left)
                Parent->Left = SwapNode;
            else
                Parent->Right = SwapNode;
        } else
            /* SwapNode is the new cache root */
            RouteCache = SwapNode;

        /* Set RCN to be child of SwapNode's parent instead of SwapNode */
        Parent = SwapNode->Parent;
        if (SwapNode == Parent->Left)
            Parent->Left = RCN;
        else
            Parent->Right = RCN;

        /* Swap parents */
        SwapRCN(&SwapNode->Parent, &RCN->Parent);
        /* Swap children */
        SwapRCN(&SwapNode->Left, &RCN->Left);
        SwapRCN(&SwapNode->Right, &RCN->Right);
#endif
    }
    
    /* Dereference NTE and NCE */
    DereferenceObject(RCN->NTE);
    DereferenceObject(RCN->NCE);

    ExternalRCN->Parent = RemNode->Parent;

    RemoveAboveExternal();
}


VOID InvalidateNTEOnSubtree(
    PNET_TABLE_ENTRY NTE,
    PROUTE_CACHE_NODE Node)
/*
 * FUNCTION: Removes all RCNs with references to an NTE on a subtree
 * ARGUMENNTS:
 *     NTE  = Pointer to NTE to invalidate
 *     Node = Pointer to RCN to start removing nodes at
 * NOTES:
 *     This function must be called with the route cache lock held.
 */
{
    TI_DbgPrint(DEBUG_RCACHE, ("Called. NTE (0x%X)  Node (0x%X).\n", NTE, Node));

    if (IsInternalRCN(Node)) {
        /* Traverse left subtree */
        InvalidateNTEOnSubtree(NTE, Node->Left);

        /* Traverse right subtree */
        InvalidateNTEOnSubtree(NTE, Node->Right);

        /* Finally check the node itself */
        if (Node->NTE == NTE)
            RemoveRouteToDestination(Node);
    }
}


VOID InvalidateNCEOnSubtree(
    PNEIGHBOR_CACHE_ENTRY NCE,
    PROUTE_CACHE_NODE Node)
/*
 * FUNCTION: Removes all RCNs with references to an NCE on a subtree
 * ARGUMENNTS:
 *     NCE  = Pointer to NCE to invalidate
 *     Node = Pointer to RCN to start removing nodes at
 * NOTES:
 *     This function must be called with the route cache lock held
 */
{
    TI_DbgPrint(DEBUG_RCACHE, ("Called. NCE (0x%X)  Node (0x%X).\n", NCE, Node));

    if (IsInternalRCN(Node)) {
        /* Traverse left subtree */
        InvalidateNCEOnSubtree(NCE, Node->Left);

        /* Traverse right subtree */
        InvalidateNCEOnSubtree(NCE, Node->Right);

        /* Finally check the node itself */
        if (Node->NCE == NCE)
            RemoveRouteToDestination(Node);
    }
}


VOID RemoveSubtree(
    PROUTE_CACHE_NODE Node)
/*
 * FUNCTION: Removes a subtree from the tree using recursion
 * ARGUMENNTS:
 *     Node = Pointer to RCN to start removing nodes at
 * NOTES:
 *     This function must be called with the route cache lock held
 */
{
    TI_DbgPrint(DEBUG_RCACHE, ("Called. Node (0x%X).\n", Node));

    if (IsInternalRCN(Node)) {
        /* Traverse left subtree */
        RemoveSubtree(Node->Left);

        /* Traverse right subtree */
        RemoveSubtree(Node->Right);

        /* Finally remove the node itself */

        /* It's an internal node, so dereference NTE and NCE */
        DereferenceObject(Node->NTE);
        DereferenceObject(Node->NCE);

#if DBG
        if (Node->RefCount != 1)
            TI_DbgPrint(MIN_TRACE, ("RCN at (0x%X) has (%d) references (should be 1).\n", Node, Node->RefCount));
#endif

        /* Remove reference for being alive */
        DereferenceObject(Node);
    }
}


NTSTATUS RouteStartup(
    VOID)
/*
 * FUNCTION: Initializes the routing subsystem
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_RCACHE, ("Called.\n"));

    /* Initialize the pseudo external route cache node */
    ExternalRCN = PoolAllocateBuffer(sizeof(ROUTE_CACHE_NODE));
    if (!ExternalRCN) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExternalRCN->Parent = NULL;
    ExternalRCN->Left   = NULL;
    ExternalRCN->Right  = NULL;

    /* Initialize the route cache root */
    RouteCache = ExternalRCN;

    KeInitializeSpinLock(&RouteCacheLock);

#if 0
    TI_DbgPrint(MIN_TRACE, ("Displaying tree.\n"));
    PrintTree(RouteCache);
#endif
    return STATUS_SUCCESS;
}


NTSTATUS RouteShutdown(
    VOID)
/*
 * FUNCTION: Shuts down the routing subsystem
 * RETURNS:
 *     Status of operation
 */
{
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_RCACHE, ("Called.\n"));

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
#if 0
    TI_DbgPrint(MIN_TRACE, ("Displaying tree.\n"));
    PrintTree(RouteCache);
#endif
    /* Clear route cache */
    RemoveSubtree(RouteCache);

    PoolFreeBuffer(ExternalRCN);

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);

    return STATUS_SUCCESS;
}


UINT RouteGetRouteToDestination(
    PIP_ADDRESS Destination,
    PNET_TABLE_ENTRY NTE,
    PROUTE_CACHE_NODE *RCN)
/*
 * FUNCTION: Locates an RCN describing a route to a destination address
 * ARGUMENTS:
 *     Destination = Pointer to destination address to find route to
 *     NTE         = Pointer to NTE describing net to send on
 *                   (NULL means routing module choose NTE to send on)
 *     RCN         = Address of pointer to an RCN
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     The RCN is referenced for the caller. The caller is responsible
 *     for dereferencing it after use
 */
{
    KIRQL OldIrql;
    PROUTE_CACHE_NODE RCN2;
    PNEIGHBOR_CACHE_ENTRY NCE;
    PIP_INTERFACE Interface;

    TI_DbgPrint(DEBUG_RCACHE, ("Called. Destination (0x%X)  NTE (0x%X).\n", Destination, NTE));

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);

#if 0
    TI_DbgPrint(MIN_TRACE, ("Displaying tree (before).\n"));
    PrintTree(RouteCache);
#endif

    ExternalRCN->Left = NULL;
    RCN2 = SearchRouteCache(Destination, RouteCache);
    if (IsExternalRCN(RCN2)) {
        /* No route was found in the cache */

        /* Check if the destination is on-link */
        Interface = RouterFindOnLinkInterface(Destination, NTE);
        if (Interface) {
            if (!NTE) {
                NTE = RouterFindBestNTE(Interface, Destination);
                if (!NTE) {
                    /* We cannot get to the specified destination. Return error */
                    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                    return IP_NO_ROUTE_TO_DESTINATION;
                }
            } else
                ReferenceObject(NTE);

            /* The destination address is on-link. Check our neighbor cache */
            NCE = NBFindOrCreateNeighbor(Interface, Destination);
            if (!NCE) {
                DereferenceObject(NTE);
                KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                return IP_NO_RESOURCES;
            }
        } else {
            /* Destination is not on any subnets we're on. Find a router to use */
            NCE = RouterGetRoute(Destination, NTE);
            if (!NCE) {
                /* We cannot get to the specified destination. Return error */
                KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                return IP_NO_ROUTE_TO_DESTINATION;
            }
        }

        /* Add the new route to the route cache */
        if (RCN2 == RouteCache) {
            RCN2       = ExpandExternalRCN();
            RouteCache = RCN2;
        } else
            RCN2 = ExpandExternalRCN();
        if (!RCN2) {
            DereferenceObject(NTE);
            DereferenceObject(NCE);
            KeReleaseSpinLock(&RouteCacheLock, OldIrql);
            return IP_NO_RESOURCES;
        }

        RCN2->RefCount    = 1;
        RCN2->State       = RCN_STATE_COMPUTED;
        RCN2->NTE         = NTE;
        RtlCopyMemory(&RCN2->Destination, Destination, sizeof(IP_ADDRESS));
        RCN2->PathMTU     = NCE->Interface->MTU;
        RCN2->NCE         = NCE;

        /* The route cache node references the NTE and the NCE. The
           NTE was referenced before and NCE is already referenced by
           RouteGetRoute() or NBFindOrCreateNeighbor() so we don't
           reference them here */
    }

    /* Reference the RCN for the user */
    ReferenceObject(RCN2);

#if 0
    TI_DbgPrint(MIN_TRACE, ("Displaying tree (after).\n"));
    PrintTree(RouteCache);
#endif

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);

    *RCN = RCN2;

    return IP_SUCCESS;
}


PROUTE_CACHE_NODE RouteAddRouteToDestination(
    PIP_ADDRESS Destination,
    PNET_TABLE_ENTRY NTE,
    PIP_INTERFACE IF,
    PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Adds a (permanent) route to a destination
 * ARGUMENTS:
 *     Destination = Pointer to destination address
 *     NTE         = Pointer to net table entry
 *     IF          = Pointer to interface to use
 *     NCE         = Pointer to first hop to destination
 * RETURNS:
 *     Pointer to RCN if the route was added, NULL if not.
 *     There can be at most one RCN per destination address / interface pair
 */
{
    KIRQL OldIrql;
    PROUTE_CACHE_NODE RCN;

    TI_DbgPrint(DEBUG_RCACHE, ("Called. Destination (0x%X)  NTE (0x%X)  IF (0x%X)  NCE (0x%X).\n", Destination, NTE, IF, NCE));

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);

    /* Locate an external RCN we can expand */
    RCN               = RouteCache;
    ExternalRCN->Left = NULL;
    for (;;) {
        RCN = SearchRouteCache(Destination, RCN);
        if (IsInternalRCN(RCN)) {
            ExternalRCN->Left = (PROUTE_CACHE_NODE)&RCN->Right;
            /* This is an internal node, continue the search to the right */
            RCN = RCN->Right;
        } else
            /* This is an external node, we've found an empty spot */
            break;
    }

    /* Expand the external node */
    if (RCN == RouteCache) {
        RCN        = ExpandExternalRCN();
        RouteCache = RCN;
    } else
        RCN = ExpandExternalRCN();
    if (!RCN) {
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
        return NULL;
    }

    /* Initialize the newly created internal node */

    /* Reference once for beeing alive */
    RCN->RefCount    = 1;
    RCN->State       = RCN_STATE_PERMANENT;
    RCN->NTE         = NTE;
    RtlCopyMemory(&RCN->Destination, Destination, sizeof(IP_ADDRESS));
    RCN->PathMTU     = IF->MTU;
    RCN->NCE         = NCE;

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);

    /* The route cache node references the NTE and the NCE */
    ReferenceObject(NTE);
    if (NCE)
        ReferenceObject(NCE);

#if 0
    TI_DbgPrint(MIN_TRACE, ("Displaying tree.\n"));
    PrintTree(RouteCache);
#endif

    return RCN;
}


VOID RouteRemoveRouteToDestination(
    PROUTE_CACHE_NODE RCN)
/*
 * FUNCTION: Removes a route to a destination
 * ARGUMENTS:
 *     RCN = Pointer to route cache node to remove
 */
{
    KIRQL OldIrql;
 
    TI_DbgPrint(DEBUG_RCACHE, ("Called. RCN (0x%X).\n", RCN));

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);

    RemoveRouteToDestination(RCN);

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}


VOID RouteInvalidateNTE(
    PNET_TABLE_ENTRY NTE)
/*
 * FUNCTION: Removes all RCNs with references to an NTE
 * ARGUMENTS:
 *     NTE = Pointer to net table entry to invalidate
 */
{
    KIRQL OldIrql;
 
    TI_DbgPrint(DEBUG_RCACHE, ("Called. NTE (0x%X).\n", NTE));

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
    InvalidateNTEOnSubtree(NTE, RouteCache);
    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}


VOID RouteInvalidateNCE(
    PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Removes all RCNs with references to an NCE
 * ARGUMENTS:
 *     NCE = Pointer to neighbor cache entry to invalidate
 */
{
    KIRQL OldIrql;
 
    TI_DbgPrint(DEBUG_RCACHE, ("Called. NCE (0x%X).\n", NCE));

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
    InvalidateNCEOnSubtree(NCE, RouteCache);
    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}

/* EOF */
