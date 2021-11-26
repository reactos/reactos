/*******************************************************************************
 *
 * Module Name: nsaccess - Top-level functions for accessing ACPI namespace
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
#include "amlcode.h"
#include "acnamesp.h"
#include "acdispat.h"

#ifdef ACPI_ASL_COMPILER
    #include "acdisasm.h"
#endif

#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsaccess")


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsRootInitialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate and initialize the default root named objects
 *
 * MUTEX:       Locks namespace for entire execution
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsRootInitialize (
    void)
{
    ACPI_STATUS                 Status;
    const ACPI_PREDEFINED_NAMES *InitVal = NULL;
    ACPI_NAMESPACE_NODE         *NewNode;
    ACPI_NAMESPACE_NODE         *PrevNode = NULL;
    ACPI_OPERAND_OBJECT         *ObjDesc;
    ACPI_STRING                 Val = NULL;


    ACPI_FUNCTION_TRACE (NsRootInitialize);


    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * The global root ptr is initially NULL, so a non-NULL value indicates
     * that AcpiNsRootInitialize() has already been called; just return.
     */
    if (AcpiGbl_RootNode)
    {
        Status = AE_OK;
        goto UnlockAndExit;
    }

    /*
     * Tell the rest of the subsystem that the root is initialized
     * (This is OK because the namespace is locked)
     */
    AcpiGbl_RootNode = &AcpiGbl_RootNodeStruct;

    /* Enter the predefined names in the name table */

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "Entering predefined entries into namespace\n"));

    /*
     * Create the initial (default) namespace.
     * This namespace looks like something similar to this:
     *
     *   ACPI Namespace (from Namespace Root):
     *    0  _GPE Scope        00203160 00
     *    0  _PR_ Scope        002031D0 00
     *    0  _SB_ Device       00203240 00 Notify Object: 0020ADD8
     *    0  _SI_ Scope        002032B0 00
     *    0  _TZ_ Device       00203320 00
     *    0  _REV Integer      00203390 00 = 0000000000000002
     *    0  _OS_ String       00203488 00 Len 14 "Microsoft Windows NT"
     *    0  _GL_ Mutex        00203580 00 Object 002035F0
     *    0  _OSI Method       00203678 00 Args 1 Len 0000 Aml 00000000
     */
    for (InitVal = AcpiGbl_PreDefinedNames; InitVal->Name; InitVal++)
    {
        Status = AE_OK;

        /* _OSI is optional for now, will be permanent later */

        if (!strcmp (InitVal->Name, "_OSI") && !AcpiGbl_CreateOsiMethod)
        {
            continue;
        }

        /*
         * Create, init, and link the new predefined name
         * Note: No need to use AcpiNsLookup here because all the
         * predefined names are at the root level. It is much easier to
         * just create and link the new node(s) here.
         */
        NewNode = AcpiNsCreateNode (*ACPI_CAST_PTR (UINT32, InitVal->Name));
        if (!NewNode)
        {
            Status = AE_NO_MEMORY;
            goto UnlockAndExit;
        }

        NewNode->DescriptorType = ACPI_DESC_TYPE_NAMED;
        NewNode->Type = InitVal->Type;

        if (!PrevNode)
        {
            AcpiGbl_RootNodeStruct.Child = NewNode;
        }
        else
        {
            PrevNode->Peer = NewNode;
        }

        NewNode->Parent = &AcpiGbl_RootNodeStruct;
        PrevNode = NewNode;

        /*
         * Name entered successfully. If entry in PreDefinedNames[] specifies
         * an initial value, create the initial value.
         */
        if (InitVal->Val)
        {
            Status = AcpiOsPredefinedOverride (InitVal, &Val);
            if (ACPI_FAILURE (Status))
            {
                ACPI_ERROR ((AE_INFO,
                    "Could not override predefined %s",
                    InitVal->Name));
            }

            if (!Val)
            {
                Val = InitVal->Val;
            }

            /*
             * Entry requests an initial value, allocate a
             * descriptor for it.
             */
            ObjDesc = AcpiUtCreateInternalObject (InitVal->Type);
            if (!ObjDesc)
            {
                Status = AE_NO_MEMORY;
                goto UnlockAndExit;
            }

            /*
             * Convert value string from table entry to
             * internal representation. Only types actually
             * used for initial values are implemented here.
             */
            switch (InitVal->Type)
            {
            case ACPI_TYPE_METHOD:

                ObjDesc->Method.ParamCount = (UINT8) ACPI_TO_INTEGER (Val);
                ObjDesc->Common.Flags |= AOPOBJ_DATA_VALID;

#if defined (ACPI_ASL_COMPILER)

                /* Save the parameter count for the iASL compiler */

                NewNode->Value = ObjDesc->Method.ParamCount;
#else
                /* Mark this as a very SPECIAL method (_OSI) */

                ObjDesc->Method.InfoFlags = ACPI_METHOD_INTERNAL_ONLY;
                ObjDesc->Method.Dispatch.Implementation = AcpiUtOsiImplementation;
#endif
                break;

            case ACPI_TYPE_INTEGER:

                ObjDesc->Integer.Value = ACPI_TO_INTEGER (Val);
                break;

            case ACPI_TYPE_STRING:

                /* Build an object around the static string */

                ObjDesc->String.Length = (UINT32) strlen (Val);
                ObjDesc->String.Pointer = Val;
                ObjDesc->Common.Flags |= AOPOBJ_STATIC_POINTER;
                break;

            case ACPI_TYPE_MUTEX:

                ObjDesc->Mutex.Node = NewNode;
                ObjDesc->Mutex.SyncLevel = (UINT8) (ACPI_TO_INTEGER (Val) - 1);

                /* Create a mutex */

                Status = AcpiOsCreateMutex (&ObjDesc->Mutex.OsMutex);
                if (ACPI_FAILURE (Status))
                {
                    AcpiUtRemoveReference (ObjDesc);
                    goto UnlockAndExit;
                }

                /* Special case for ACPI Global Lock */

                if (strcmp (InitVal->Name, "_GL_") == 0)
                {
                    AcpiGbl_GlobalLockMutex = ObjDesc;

                    /* Create additional counting semaphore for global lock */

                    Status = AcpiOsCreateSemaphore (
                        1, 0, &AcpiGbl_GlobalLockSemaphore);
                    if (ACPI_FAILURE (Status))
                    {
                        AcpiUtRemoveReference (ObjDesc);
                        goto UnlockAndExit;
                    }
                }
                break;

            default:

                ACPI_ERROR ((AE_INFO, "Unsupported initial type value 0x%X",
                    InitVal->Type));
                AcpiUtRemoveReference (ObjDesc);
                ObjDesc = NULL;
                continue;
            }

            /* Store pointer to value descriptor in the Node */

            Status = AcpiNsAttachObject (NewNode, ObjDesc,
                ObjDesc->Common.Type);

            /* Remove local reference to the object */

            AcpiUtRemoveReference (ObjDesc);
        }
    }

UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);

    /* Save a handle to "_GPE", it is always present */

    if (ACPI_SUCCESS (Status))
    {
        Status = AcpiNsGetNode (NULL, "\\_GPE", ACPI_NS_NO_UPSEARCH,
            &AcpiGbl_FadtGpeDevice);
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsLookup
 *
 * PARAMETERS:  ScopeInfo       - Current scope info block
 *              Pathname        - Search pathname, in internal format
 *                                (as represented in the AML stream)
 *              Type            - Type associated with name
 *              InterpreterMode - IMODE_LOAD_PASS2 => add name if not found
 *              Flags           - Flags describing the search restrictions
 *              WalkState       - Current state of the walk
 *              ReturnNode      - Where the Node is placed (if found
 *                                or created successfully)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Find or enter the passed name in the name space.
 *              Log an error if name not found in Exec mode.
 *
 * MUTEX:       Assumes namespace is locked.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsLookup (
    ACPI_GENERIC_STATE      *ScopeInfo,
    char                    *Pathname,
    ACPI_OBJECT_TYPE        Type,
    ACPI_INTERPRETER_MODE   InterpreterMode,
    UINT32                  Flags,
    ACPI_WALK_STATE         *WalkState,
    ACPI_NAMESPACE_NODE     **ReturnNode)
{
    ACPI_STATUS             Status;
    char                    *Path = Pathname;
    char                    *ExternalPath;
    ACPI_NAMESPACE_NODE     *PrefixNode;
    ACPI_NAMESPACE_NODE     *CurrentNode = NULL;
    ACPI_NAMESPACE_NODE     *ThisNode = NULL;
    UINT32                  NumSegments;
    UINT32                  NumCarats;
    ACPI_NAME               SimpleName;
    ACPI_OBJECT_TYPE        TypeToCheckFor;
    ACPI_OBJECT_TYPE        ThisSearchType;
    UINT32                  SearchParentFlag = ACPI_NS_SEARCH_PARENT;
    UINT32                  LocalFlags;
    ACPI_INTERPRETER_MODE   LocalInterpreterMode;


    ACPI_FUNCTION_TRACE (NsLookup);


    if (!ReturnNode)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    LocalFlags = Flags &
        ~(ACPI_NS_ERROR_IF_FOUND | ACPI_NS_OVERRIDE_IF_FOUND |
          ACPI_NS_SEARCH_PARENT);
    *ReturnNode = ACPI_ENTRY_NOT_FOUND;
    AcpiGbl_NsLookupCount++;

    if (!AcpiGbl_RootNode)
    {
        return_ACPI_STATUS (AE_NO_NAMESPACE);
    }

    /* Get the prefix scope. A null scope means use the root scope */

    if ((!ScopeInfo) ||
        (!ScopeInfo->Scope.Node))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
            "Null scope prefix, using root node (%p)\n",
            AcpiGbl_RootNode));

        PrefixNode = AcpiGbl_RootNode;
    }
    else
    {
        PrefixNode = ScopeInfo->Scope.Node;
        if (ACPI_GET_DESCRIPTOR_TYPE (PrefixNode) != ACPI_DESC_TYPE_NAMED)
        {
            ACPI_ERROR ((AE_INFO, "%p is not a namespace node [%s]",
                PrefixNode, AcpiUtGetDescriptorName (PrefixNode)));
            return_ACPI_STATUS (AE_AML_INTERNAL);
        }

        if (!(Flags & ACPI_NS_PREFIX_IS_SCOPE))
        {
            /*
             * This node might not be a actual "scope" node (such as a
             * Device/Method, etc.)  It could be a Package or other object
             * node. Backup up the tree to find the containing scope node.
             */
            while (!AcpiNsOpensScope (PrefixNode->Type) &&
                    PrefixNode->Type != ACPI_TYPE_ANY)
            {
                PrefixNode = PrefixNode->Parent;
            }
        }
    }

    /* Save type. TBD: may be no longer necessary */

    TypeToCheckFor = Type;

    /*
     * Begin examination of the actual pathname
     */
    if (!Pathname)
    {
        /* A Null NamePath is allowed and refers to the root */

        NumSegments = 0;
        ThisNode = AcpiGbl_RootNode;
        Path = "";

        ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
            "Null Pathname (Zero segments), Flags=%X\n", Flags));
    }
    else
    {
        /*
         * Name pointer is valid (and must be in internal name format)
         *
         * Check for scope prefixes:
         *
         * As represented in the AML stream, a namepath consists of an
         * optional scope prefix followed by a name segment part.
         *
         * If present, the scope prefix is either a Root Prefix (in
         * which case the name is fully qualified), or one or more
         * Parent Prefixes (in which case the name's scope is relative
         * to the current scope).
         */
        if (*Path == (UINT8) AML_ROOT_PREFIX)
        {
            /* Pathname is fully qualified, start from the root */

            ThisNode = AcpiGbl_RootNode;
            SearchParentFlag = ACPI_NS_NO_UPSEARCH;

            /* Point to name segment part */

            Path++;

            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "Path is absolute from root [%p]\n", ThisNode));
        }
        else
        {
            /* Pathname is relative to current scope, start there */

            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "Searching relative to prefix scope [%4.4s] (%p)\n",
                AcpiUtGetNodeName (PrefixNode), PrefixNode));

            /*
             * Handle multiple Parent Prefixes (carat) by just getting
             * the parent node for each prefix instance.
             */
            ThisNode = PrefixNode;
            NumCarats = 0;
            while (*Path == (UINT8) AML_PARENT_PREFIX)
            {
                /* Name is fully qualified, no search rules apply */

                SearchParentFlag = ACPI_NS_NO_UPSEARCH;

                /*
                 * Point past this prefix to the name segment
                 * part or the next Parent Prefix
                 */
                Path++;

                /* Backup to the parent node */

                NumCarats++;
                ThisNode = ThisNode->Parent;
                if (!ThisNode)
                {
                    /*
                     * Current scope has no parent scope. Externalize
                     * the internal path for error message.
                     */
                    Status = AcpiNsExternalizeName (ACPI_UINT32_MAX, Pathname,
                        NULL, &ExternalPath);
                    if (ACPI_SUCCESS (Status))
                    {
                        ACPI_ERROR ((AE_INFO,
                            "%s: Path has too many parent prefixes (^)",
                            ExternalPath));

                        ACPI_FREE (ExternalPath);
                    }

                    return_ACPI_STATUS (AE_NOT_FOUND);
                }
            }

            if (SearchParentFlag == ACPI_NS_NO_UPSEARCH)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                    "Search scope is [%4.4s], path has %u carat(s)\n",
                    AcpiUtGetNodeName (ThisNode), NumCarats));
            }
        }

        /*
         * Determine the number of ACPI name segments in this pathname.
         *
         * The segment part consists of either:
         *  - A Null name segment (0)
         *  - A DualNamePrefix followed by two 4-byte name segments
         *  - A MultiNamePrefix followed by a byte indicating the
         *      number of segments and the segments themselves.
         *  - A single 4-byte name segment
         *
         * Examine the name prefix opcode, if any, to determine the number of
         * segments.
         */
        switch (*Path)
        {
        case 0:
            /*
             * Null name after a root or parent prefixes. We already
             * have the correct target node and there are no name segments.
             */
            NumSegments  = 0;
            Type = ThisNode->Type;

            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "Prefix-only Pathname (Zero name segments), Flags=%X\n",
                Flags));
            break;

        case AML_DUAL_NAME_PREFIX:

            /* More than one NameSeg, search rules do not apply */

            SearchParentFlag = ACPI_NS_NO_UPSEARCH;

            /* Two segments, point to first name segment */

            NumSegments = 2;
            Path++;

            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "Dual Pathname (2 segments, Flags=%X)\n", Flags));
            break;

        case AML_MULTI_NAME_PREFIX:

            /* More than one NameSeg, search rules do not apply */

            SearchParentFlag = ACPI_NS_NO_UPSEARCH;

            /* Extract segment count, point to first name segment */

            Path++;
            NumSegments = (UINT32) (UINT8) *Path;
            Path++;

            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "Multi Pathname (%u Segments, Flags=%X)\n",
                NumSegments, Flags));
            break;

        default:
            /*
             * Not a Null name, no Dual or Multi prefix, hence there is
             * only one name segment and Pathname is already pointing to it.
             */
            NumSegments = 1;

            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "Simple Pathname (1 segment, Flags=%X)\n", Flags));
            break;
        }

        ACPI_DEBUG_EXEC (AcpiNsPrintPathname (NumSegments, Path));
    }


    /*
     * Search namespace for each segment of the name. Loop through and
     * verify (or add to the namespace) each name segment.
     *
     * The object type is significant only at the last name
     * segment. (We don't care about the types along the path, only
     * the type of the final target object.)
     */
    ThisSearchType = ACPI_TYPE_ANY;
    CurrentNode = ThisNode;

    while (NumSegments && CurrentNode)
    {
        NumSegments--;
        if (!NumSegments)
        {
            /* This is the last segment, enable typechecking */

            ThisSearchType = Type;

            /*
             * Only allow automatic parent search (search rules) if the caller
             * requested it AND we have a single, non-fully-qualified NameSeg
             */
            if ((SearchParentFlag != ACPI_NS_NO_UPSEARCH) &&
                (Flags & ACPI_NS_SEARCH_PARENT))
            {
                LocalFlags |= ACPI_NS_SEARCH_PARENT;
            }

            /* Set error flag according to caller */

            if (Flags & ACPI_NS_ERROR_IF_FOUND)
            {
                LocalFlags |= ACPI_NS_ERROR_IF_FOUND;
            }

            /* Set override flag according to caller */

            if (Flags & ACPI_NS_OVERRIDE_IF_FOUND)
            {
                LocalFlags |= ACPI_NS_OVERRIDE_IF_FOUND;
            }
        }

        /* Handle opcodes that create a new NameSeg via a full NamePath */

        LocalInterpreterMode = InterpreterMode;
        if ((Flags & ACPI_NS_PREFIX_MUST_EXIST) && (NumSegments > 0))
        {
            /* Every element of the path must exist (except for the final NameSeg) */

            LocalInterpreterMode = ACPI_IMODE_EXECUTE;
        }

        /* Extract one ACPI name from the front of the pathname */

        ACPI_MOVE_32_TO_32 (&SimpleName, Path);

        /* Try to find the single (4 character) ACPI name */

        Status = AcpiNsSearchAndEnter (SimpleName, WalkState, CurrentNode,
            LocalInterpreterMode, ThisSearchType, LocalFlags, &ThisNode);
        if (ACPI_FAILURE (Status))
        {
            if (Status == AE_NOT_FOUND)
            {
#if !defined ACPI_ASL_COMPILER /* Note: iASL reports this error by itself, not needed here */
                if (Flags & ACPI_NS_PREFIX_MUST_EXIST)
                {
                    AcpiOsPrintf (ACPI_MSG_BIOS_ERROR
                        "Object does not exist: %4.4s\n", (char *) &SimpleName);
                }
#endif
                /* Name not found in ACPI namespace */

                ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                    "Name [%4.4s] not found in scope [%4.4s] %p\n",
                    (char *) &SimpleName, (char *) &CurrentNode->Name,
                    CurrentNode));
            }

#ifdef ACPI_EXEC_APP
            if ((Status == AE_ALREADY_EXISTS) &&
                (ThisNode->Flags & ANOBJ_NODE_EARLY_INIT))
            {
                ThisNode->Flags &= ~ANOBJ_NODE_EARLY_INIT;
                Status = AE_OK;
            }
#endif

#ifdef ACPI_ASL_COMPILER
            /*
             * If this ACPI name already exists within the namespace as an
             * external declaration, then mark the external as a conflicting
             * declaration and proceed to process the current node as if it did
             * not exist in the namespace. If this node is not processed as
             * normal, then it could cause improper namespace resolution
             * by failing to open a new scope.
             */
            if (AcpiGbl_DisasmFlag &&
                (Status == AE_ALREADY_EXISTS) &&
                ((ThisNode->Flags & ANOBJ_IS_EXTERNAL) ||
                    (WalkState && WalkState->Opcode == AML_EXTERNAL_OP)))
            {
                ThisNode->Flags &= ~ANOBJ_IS_EXTERNAL;
                ThisNode->Type = (UINT8)ThisSearchType;
                if (WalkState->Opcode != AML_EXTERNAL_OP)
                {
                    AcpiDmMarkExternalConflict (ThisNode);
                }
                break;
            }
#endif

            *ReturnNode = ThisNode;
            return_ACPI_STATUS (Status);
        }

        /* More segments to follow? */

        if (NumSegments > 0)
        {
            /*
             * If we have an alias to an object that opens a scope (such as a
             * device or processor), we need to dereference the alias here so
             * that we can access any children of the original node (via the
             * remaining segments).
             */
            if (ThisNode->Type == ACPI_TYPE_LOCAL_ALIAS)
            {
                if (!ThisNode->Object)
                {
                    return_ACPI_STATUS (AE_NOT_EXIST);
                }

                if (AcpiNsOpensScope (((ACPI_NAMESPACE_NODE *)
                        ThisNode->Object)->Type))
                {
                    ThisNode = (ACPI_NAMESPACE_NODE *) ThisNode->Object;
                }
            }
        }

        /* Special handling for the last segment (NumSegments == 0) */

        else
        {
            /*
             * Sanity typecheck of the target object:
             *
             * If 1) This is the last segment (NumSegments == 0)
             *    2) And we are looking for a specific type
             *       (Not checking for TYPE_ANY)
             *    3) Which is not an alias
             *    4) Which is not a local type (TYPE_SCOPE)
             *    5) And the type of target object is known (not TYPE_ANY)
             *    6) And target object does not match what we are looking for
             *
             * Then we have a type mismatch. Just warn and ignore it.
             */
            if ((TypeToCheckFor != ACPI_TYPE_ANY)                   &&
                (TypeToCheckFor != ACPI_TYPE_LOCAL_ALIAS)           &&
                (TypeToCheckFor != ACPI_TYPE_LOCAL_METHOD_ALIAS)    &&
                (TypeToCheckFor != ACPI_TYPE_LOCAL_SCOPE)           &&
                (ThisNode->Type != ACPI_TYPE_ANY)                   &&
                (ThisNode->Type != TypeToCheckFor))
            {
                /* Complain about a type mismatch */

                ACPI_WARNING ((AE_INFO,
                    "NsLookup: Type mismatch on %4.4s (%s), searching for (%s)",
                    ACPI_CAST_PTR (char, &SimpleName),
                    AcpiUtGetTypeName (ThisNode->Type),
                    AcpiUtGetTypeName (TypeToCheckFor)));
            }

            /*
             * If this is the last name segment and we are not looking for a
             * specific type, but the type of found object is known, use that
             * type to (later) see if it opens a scope.
             */
            if (Type == ACPI_TYPE_ANY)
            {
                Type = ThisNode->Type;
            }
        }

        /* Point to next name segment and make this node current */

        Path += ACPI_NAMESEG_SIZE;
        CurrentNode = ThisNode;
    }

    /* Always check if we need to open a new scope */

    if (!(Flags & ACPI_NS_DONT_OPEN_SCOPE) && (WalkState))
    {
        /*
         * If entry is a type which opens a scope, push the new scope on the
         * scope stack.
         */
        if (AcpiNsOpensScope (Type))
        {
            Status = AcpiDsScopeStackPush (ThisNode, Type, WalkState);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }
        }
    }

#ifdef ACPI_EXEC_APP
    if (Flags & ACPI_NS_EARLY_INIT)
    {
        ThisNode->Flags |= ANOBJ_NODE_EARLY_INIT;
    }
#endif

    *ReturnNode = ThisNode;
    return_ACPI_STATUS (AE_OK);
}
