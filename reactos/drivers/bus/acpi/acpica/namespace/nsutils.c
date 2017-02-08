/******************************************************************************
 *
 * Module Name: nsutils - Utilities for accessing ACPI namespace, accessing
 *                        parents and siblings and Scope manipulation
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
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
#include "amlcode.h"

#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsutils")

/* Local prototypes */

#ifdef ACPI_OBSOLETE_FUNCTIONS
ACPI_NAME
AcpiNsFindParentName (
    ACPI_NAMESPACE_NODE     *NodeToSearch);
#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsPrintNodePathname
 *
 * PARAMETERS:  Node            - Object
 *              Message         - Prefix message
 *
 * DESCRIPTION: Print an object's full namespace pathname
 *              Manages allocation/freeing of a pathname buffer
 *
 ******************************************************************************/

void
AcpiNsPrintNodePathname (
    ACPI_NAMESPACE_NODE     *Node,
    const char              *Message)
{
    ACPI_BUFFER             Buffer;
    ACPI_STATUS             Status;


    if (!Node)
    {
        AcpiOsPrintf ("[NULL NAME]");
        return;
    }

    /* Convert handle to full pathname and print it (with supplied message) */

    Buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

    Status = AcpiNsHandleToPathname (Node, &Buffer, TRUE);
    if (ACPI_SUCCESS (Status))
    {
        if (Message)
        {
            AcpiOsPrintf ("%s ", Message);
        }

        AcpiOsPrintf ("[%s] (Node %p)", (char *) Buffer.Pointer, Node);
        ACPI_FREE (Buffer.Pointer);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetType
 *
 * PARAMETERS:  Node        - Parent Node to be examined
 *
 * RETURN:      Type field from Node whose handle is passed
 *
 * DESCRIPTION: Return the type of a Namespace node
 *
 ******************************************************************************/

ACPI_OBJECT_TYPE
AcpiNsGetType (
    ACPI_NAMESPACE_NODE     *Node)
{
    ACPI_FUNCTION_TRACE (NsGetType);


    if (!Node)
    {
        ACPI_WARNING ((AE_INFO, "Null Node parameter"));
        return_UINT8 (ACPI_TYPE_ANY);
    }

    return_UINT8 (Node->Type);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsLocal
 *
 * PARAMETERS:  Type        - A namespace object type
 *
 * RETURN:      LOCAL if names must be found locally in objects of the
 *              passed type, 0 if enclosing scopes should be searched
 *
 * DESCRIPTION: Returns scope rule for the given object type.
 *
 ******************************************************************************/

UINT32
AcpiNsLocal (
    ACPI_OBJECT_TYPE        Type)
{
    ACPI_FUNCTION_TRACE (NsLocal);


    if (!AcpiUtValidObjectType (Type))
    {
        /* Type code out of range  */

        ACPI_WARNING ((AE_INFO, "Invalid Object Type 0x%X", Type));
        return_UINT32 (ACPI_NS_NORMAL);
    }

    return_UINT32 (AcpiGbl_NsProperties[Type] & ACPI_NS_LOCAL);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetInternalNameLength
 *
 * PARAMETERS:  Info            - Info struct initialized with the
 *                                external name pointer.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Calculate the length of the internal (AML) namestring
 *              corresponding to the external (ASL) namestring.
 *
 ******************************************************************************/

void
AcpiNsGetInternalNameLength (
    ACPI_NAMESTRING_INFO    *Info)
{
    const char              *NextExternalChar;
    UINT32                  i;


    ACPI_FUNCTION_ENTRY ();


    NextExternalChar = Info->ExternalName;
    Info->NumCarats = 0;
    Info->NumSegments = 0;
    Info->FullyQualified = FALSE;

    /*
     * For the internal name, the required length is 4 bytes per segment,
     * plus 1 each for RootPrefix, MultiNamePrefixOp, segment count,
     * trailing null (which is not really needed, but no there's harm in
     * putting it there)
     *
     * strlen() + 1 covers the first NameSeg, which has no path separator
     */
    if (ACPI_IS_ROOT_PREFIX (*NextExternalChar))
    {
        Info->FullyQualified = TRUE;
        NextExternalChar++;

        /* Skip redundant RootPrefix, like \\_SB.PCI0.SBRG.EC0 */

        while (ACPI_IS_ROOT_PREFIX (*NextExternalChar))
        {
            NextExternalChar++;
        }
    }
    else
    {
        /* Handle Carat prefixes */

        while (ACPI_IS_PARENT_PREFIX (*NextExternalChar))
        {
            Info->NumCarats++;
            NextExternalChar++;
        }
    }

    /*
     * Determine the number of ACPI name "segments" by counting the number of
     * path separators within the string. Start with one segment since the
     * segment count is [(# separators) + 1], and zero separators is ok.
     */
    if (*NextExternalChar)
    {
        Info->NumSegments = 1;
        for (i = 0; NextExternalChar[i]; i++)
        {
            if (ACPI_IS_PATH_SEPARATOR (NextExternalChar[i]))
            {
                Info->NumSegments++;
            }
        }
    }

    Info->Length = (ACPI_NAME_SIZE * Info->NumSegments) +
        4 + Info->NumCarats;

    Info->NextExternalChar = NextExternalChar;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsBuildInternalName
 *
 * PARAMETERS:  Info            - Info struct fully initialized
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Construct the internal (AML) namestring
 *              corresponding to the external (ASL) namestring.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsBuildInternalName (
    ACPI_NAMESTRING_INFO    *Info)
{
    UINT32                  NumSegments = Info->NumSegments;
    char                    *InternalName = Info->InternalName;
    const char              *ExternalName = Info->NextExternalChar;
    char                    *Result = NULL;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (NsBuildInternalName);


    /* Setup the correct prefixes, counts, and pointers */

    if (Info->FullyQualified)
    {
        InternalName[0] = AML_ROOT_PREFIX;

        if (NumSegments <= 1)
        {
            Result = &InternalName[1];
        }
        else if (NumSegments == 2)
        {
            InternalName[1] = AML_DUAL_NAME_PREFIX;
            Result = &InternalName[2];
        }
        else
        {
            InternalName[1] = AML_MULTI_NAME_PREFIX_OP;
            InternalName[2] = (char) NumSegments;
            Result = &InternalName[3];
        }
    }
    else
    {
        /*
         * Not fully qualified.
         * Handle Carats first, then append the name segments
         */
        i = 0;
        if (Info->NumCarats)
        {
            for (i = 0; i < Info->NumCarats; i++)
            {
                InternalName[i] = AML_PARENT_PREFIX;
            }
        }

        if (NumSegments <= 1)
        {
            Result = &InternalName[i];
        }
        else if (NumSegments == 2)
        {
            InternalName[i] = AML_DUAL_NAME_PREFIX;
            Result = &InternalName[(ACPI_SIZE) i+1];
        }
        else
        {
            InternalName[i] = AML_MULTI_NAME_PREFIX_OP;
            InternalName[(ACPI_SIZE) i+1] = (char) NumSegments;
            Result = &InternalName[(ACPI_SIZE) i+2];
        }
    }

    /* Build the name (minus path separators) */

    for (; NumSegments; NumSegments--)
    {
        for (i = 0; i < ACPI_NAME_SIZE; i++)
        {
            if (ACPI_IS_PATH_SEPARATOR (*ExternalName) ||
               (*ExternalName == 0))
            {
                /* Pad the segment with underscore(s) if segment is short */

                Result[i] = '_';
            }
            else
            {
                /* Convert the character to uppercase and save it */

                Result[i] = (char) toupper ((int) *ExternalName);
                ExternalName++;
            }
        }

        /* Now we must have a path separator, or the pathname is bad */

        if (!ACPI_IS_PATH_SEPARATOR (*ExternalName) &&
            (*ExternalName != 0))
        {
            return_ACPI_STATUS (AE_BAD_PATHNAME);
        }

        /* Move on the next segment */

        ExternalName++;
        Result += ACPI_NAME_SIZE;
    }

    /* Terminate the string */

    *Result = 0;

    if (Info->FullyQualified)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Returning [%p] (abs) \"\\%s\"\n",
            InternalName, InternalName));
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Returning [%p] (rel) \"%s\"\n",
            InternalName, InternalName));
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsInternalizeName
 *
 * PARAMETERS:  *ExternalName           - External representation of name
 *              **Converted Name        - Where to return the resulting
 *                                        internal represention of the name
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an external representation (e.g. "\_PR_.CPU0")
 *              to internal form (e.g. 5c 2f 02 5f 50 52 5f 43 50 55 30)
 *
 *******************************************************************************/

ACPI_STATUS
AcpiNsInternalizeName (
    const char              *ExternalName,
    char                    **ConvertedName)
{
    char                    *InternalName;
    ACPI_NAMESTRING_INFO    Info;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (NsInternalizeName);


    if ((!ExternalName)      ||
        (*ExternalName == 0) ||
        (!ConvertedName))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Get the length of the new internal name */

    Info.ExternalName = ExternalName;
    AcpiNsGetInternalNameLength (&Info);

    /* We need a segment to store the internal  name */

    InternalName = ACPI_ALLOCATE_ZEROED (Info.Length);
    if (!InternalName)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Build the name */

    Info.InternalName = InternalName;
    Status = AcpiNsBuildInternalName (&Info);
    if (ACPI_FAILURE (Status))
    {
        ACPI_FREE (InternalName);
        return_ACPI_STATUS (Status);
    }

    *ConvertedName = InternalName;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsExternalizeName
 *
 * PARAMETERS:  InternalNameLength  - Lenth of the internal name below
 *              InternalName        - Internal representation of name
 *              ConvertedNameLength - Where the length is returned
 *              ConvertedName       - Where the resulting external name
 *                                    is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert internal name (e.g. 5c 2f 02 5f 50 52 5f 43 50 55 30)
 *              to its external (printable) form (e.g. "\_PR_.CPU0")
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsExternalizeName (
    UINT32                  InternalNameLength,
    const char              *InternalName,
    UINT32                  *ConvertedNameLength,
    char                    **ConvertedName)
{
    UINT32                  NamesIndex = 0;
    UINT32                  NumSegments = 0;
    UINT32                  RequiredLength;
    UINT32                  PrefixLength = 0;
    UINT32                  i = 0;
    UINT32                  j = 0;


    ACPI_FUNCTION_TRACE (NsExternalizeName);


    if (!InternalNameLength     ||
        !InternalName           ||
        !ConvertedName)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Check for a prefix (one '\' | one or more '^') */

    switch (InternalName[0])
    {
    case AML_ROOT_PREFIX:

        PrefixLength = 1;
        break;

    case AML_PARENT_PREFIX:

        for (i = 0; i < InternalNameLength; i++)
        {
            if (ACPI_IS_PARENT_PREFIX (InternalName[i]))
            {
                PrefixLength = i + 1;
            }
            else
            {
                break;
            }
        }

        if (i == InternalNameLength)
        {
            PrefixLength = i;
        }

        break;

    default:

        break;
    }

    /*
     * Check for object names. Note that there could be 0-255 of these
     * 4-byte elements.
     */
    if (PrefixLength < InternalNameLength)
    {
        switch (InternalName[PrefixLength])
        {
        case AML_MULTI_NAME_PREFIX_OP:

            /* <count> 4-byte names */

            NamesIndex = PrefixLength + 2;
            NumSegments = (UINT8)
                InternalName[(ACPI_SIZE) PrefixLength + 1];
            break;

        case AML_DUAL_NAME_PREFIX:

            /* Two 4-byte names */

            NamesIndex = PrefixLength + 1;
            NumSegments = 2;
            break;

        case 0:

            /* NullName */

            NamesIndex = 0;
            NumSegments = 0;
            break;

        default:

            /* one 4-byte name */

            NamesIndex = PrefixLength;
            NumSegments = 1;
            break;
        }
    }

    /*
     * Calculate the length of ConvertedName, which equals the length
     * of the prefix, length of all object names, length of any required
     * punctuation ('.') between object names, plus the NULL terminator.
     */
    RequiredLength = PrefixLength + (4 * NumSegments) +
        ((NumSegments > 0) ? (NumSegments - 1) : 0) + 1;

    /*
     * Check to see if we're still in bounds. If not, there's a problem
     * with InternalName (invalid format).
     */
    if (RequiredLength > InternalNameLength)
    {
        ACPI_ERROR ((AE_INFO, "Invalid internal name"));
        return_ACPI_STATUS (AE_BAD_PATHNAME);
    }

    /* Build the ConvertedName */

    *ConvertedName = ACPI_ALLOCATE_ZEROED (RequiredLength);
    if (!(*ConvertedName))
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    j = 0;

    for (i = 0; i < PrefixLength; i++)
    {
        (*ConvertedName)[j++] = InternalName[i];
    }

    if (NumSegments > 0)
    {
        for (i = 0; i < NumSegments; i++)
        {
            if (i > 0)
            {
                (*ConvertedName)[j++] = '.';
            }

            /* Copy and validate the 4-char name segment */

            ACPI_MOVE_NAME (&(*ConvertedName)[j],
                &InternalName[NamesIndex]);
            AcpiUtRepairName (&(*ConvertedName)[j]);

            j += ACPI_NAME_SIZE;
            NamesIndex += ACPI_NAME_SIZE;
        }
    }

    if (ConvertedNameLength)
    {
        *ConvertedNameLength = (UINT32) RequiredLength;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsValidateHandle
 *
 * PARAMETERS:  Handle          - Handle to be validated and typecast to a
 *                                namespace node.
 *
 * RETURN:      A pointer to a namespace node
 *
 * DESCRIPTION: Convert a namespace handle to a namespace node. Handles special
 *              cases for the root node.
 *
 * NOTE: Real integer handles would allow for more verification
 *       and keep all pointers within this subsystem - however this introduces
 *       more overhead and has not been necessary to this point. Drivers
 *       holding handles are typically notified before a node becomes invalid
 *       due to a table unload.
 *
 ******************************************************************************/

ACPI_NAMESPACE_NODE *
AcpiNsValidateHandle (
    ACPI_HANDLE             Handle)
{

    ACPI_FUNCTION_ENTRY ();


    /* Parameter validation */

    if ((!Handle) || (Handle == ACPI_ROOT_OBJECT))
    {
        return (AcpiGbl_RootNode);
    }

    /* We can at least attempt to verify the handle */

    if (ACPI_GET_DESCRIPTOR_TYPE (Handle) != ACPI_DESC_TYPE_NAMED)
    {
        return (NULL);
    }

    return (ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, Handle));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsTerminate
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: free memory allocated for namespace and ACPI table storage.
 *
 ******************************************************************************/

void
AcpiNsTerminate (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (NsTerminate);


#ifdef ACPI_EXEC_APP
    {
        ACPI_OPERAND_OBJECT     *Prev;
        ACPI_OPERAND_OBJECT     *Next;

        /* Delete any module-level code blocks */

        Next = AcpiGbl_ModuleCodeList;
        while (Next)
        {
            Prev = Next;
            Next = Next->Method.Mutex;
            Prev->Method.Mutex = NULL; /* Clear the Mutex (cheated) field */
            AcpiUtRemoveReference (Prev);
        }
    }
#endif

    /*
     * Free the entire namespace -- all nodes and all objects
     * attached to the nodes
     */
    AcpiNsDeleteNamespaceSubtree (AcpiGbl_RootNode);

    /* Delete any objects attached to the root node */

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_VOID;
    }

    AcpiNsDeleteNode (AcpiGbl_RootNode);
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "Namespace freed\n"));
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsOpensScope
 *
 * PARAMETERS:  Type        - A valid namespace type
 *
 * RETURN:      NEWSCOPE if the passed type "opens a name scope" according
 *              to the ACPI specification, else 0
 *
 ******************************************************************************/

UINT32
AcpiNsOpensScope (
    ACPI_OBJECT_TYPE        Type)
{
    ACPI_FUNCTION_ENTRY ();


    if (Type > ACPI_TYPE_LOCAL_MAX)
    {
        /* type code out of range  */

        ACPI_WARNING ((AE_INFO, "Invalid Object Type 0x%X", Type));
        return (ACPI_NS_NORMAL);
    }

    return (((UINT32) AcpiGbl_NsProperties[Type]) & ACPI_NS_NEWSCOPE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetNode
 *
 * PARAMETERS:  *Pathname   - Name to be found, in external (ASL) format. The
 *                            \ (backslash) and ^ (carat) prefixes, and the
 *                            . (period) to separate segments are supported.
 *              PrefixNode   - Root of subtree to be searched, or NS_ALL for the
 *                            root of the name space. If Name is fully
 *                            qualified (first INT8 is '\'), the passed value
 *                            of Scope will not be accessed.
 *              Flags       - Used to indicate whether to perform upsearch or
 *                            not.
 *              ReturnNode  - Where the Node is returned
 *
 * DESCRIPTION: Look up a name relative to a given scope and return the
 *              corresponding Node. NOTE: Scope can be null.
 *
 * MUTEX:       Locks namespace
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsGetNode (
    ACPI_NAMESPACE_NODE     *PrefixNode,
    const char              *Pathname,
    UINT32                  Flags,
    ACPI_NAMESPACE_NODE     **ReturnNode)
{
    ACPI_GENERIC_STATE      ScopeInfo;
    ACPI_STATUS             Status;
    char                    *InternalPath;


    ACPI_FUNCTION_TRACE_PTR (NsGetNode, ACPI_CAST_PTR (char, Pathname));


    /* Simplest case is a null pathname */

    if (!Pathname)
    {
        *ReturnNode = PrefixNode;
        if (!PrefixNode)
        {
            *ReturnNode = AcpiGbl_RootNode;
        }

        return_ACPI_STATUS (AE_OK);
    }

    /* Quick check for a reference to the root */

    if (ACPI_IS_ROOT_PREFIX (Pathname[0]) && (!Pathname[1]))
    {
        *ReturnNode = AcpiGbl_RootNode;
        return_ACPI_STATUS (AE_OK);
    }

    /* Convert path to internal representation */

    Status = AcpiNsInternalizeName (Pathname, &InternalPath);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Must lock namespace during lookup */

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /* Setup lookup scope (search starting point) */

    ScopeInfo.Scope.Node = PrefixNode;

    /* Lookup the name in the namespace */

    Status = AcpiNsLookup (&ScopeInfo, InternalPath, ACPI_TYPE_ANY,
        ACPI_IMODE_EXECUTE, (Flags | ACPI_NS_DONT_OPEN_SCOPE),
        NULL, ReturnNode);
    if (ACPI_FAILURE (Status))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "%s, %s\n",
            Pathname, AcpiFormatException (Status)));
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);

Cleanup:
    ACPI_FREE (InternalPath);
    return_ACPI_STATUS (Status);
}
