/*******************************************************************************
 *
 * Module Name: nsnames - Name manipulation and search
 *
 ******************************************************************************/

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
#include "amlcode.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsnames")

/* Local Prototypes */

static void
AcpiNsNormalizePathname (
    char                    *OriginalPath);


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetExternalPathname
 *
 * PARAMETERS:  Node            - Namespace node whose pathname is needed
 *
 * RETURN:      Pointer to storage containing the fully qualified name of
 *              the node, In external format (name segments separated by path
 *              separators.)
 *
 * DESCRIPTION: Used to obtain the full pathname to a namespace node, usually
 *              for error and debug statements.
 *
 ******************************************************************************/

char *
AcpiNsGetExternalPathname (
    ACPI_NAMESPACE_NODE     *Node)
{
    char                    *NameBuffer;


    ACPI_FUNCTION_TRACE_PTR (NsGetExternalPathname, Node);


    NameBuffer = AcpiNsGetNormalizedPathname (Node, FALSE);
    return_PTR (NameBuffer);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetPathnameLength
 *
 * PARAMETERS:  Node        - Namespace node
 *
 * RETURN:      Length of path, including prefix
 *
 * DESCRIPTION: Get the length of the pathname string for this node
 *
 ******************************************************************************/

ACPI_SIZE
AcpiNsGetPathnameLength (
    ACPI_NAMESPACE_NODE     *Node)
{
    ACPI_SIZE               Size;


    /* Validate the Node */

    if (ACPI_GET_DESCRIPTOR_TYPE (Node) != ACPI_DESC_TYPE_NAMED)
    {
        ACPI_ERROR ((AE_INFO,
            "Invalid/cached reference target node: %p, descriptor type %d",
            Node, ACPI_GET_DESCRIPTOR_TYPE (Node)));
        return (0);
    }

    Size = AcpiNsBuildNormalizedPath (Node, NULL, 0, FALSE);
    return (Size);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsHandleToName
 *
 * PARAMETERS:  TargetHandle            - Handle of named object whose name is
 *                                        to be found
 *              Buffer                  - Where the name is returned
 *
 * RETURN:      Status, Buffer is filled with name if status is AE_OK
 *
 * DESCRIPTION: Build and return a full namespace name
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsHandleToName (
    ACPI_HANDLE             TargetHandle,
    ACPI_BUFFER             *Buffer)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;
    const char              *NodeName;


    ACPI_FUNCTION_TRACE_PTR (NsHandleToName, TargetHandle);


    Node = AcpiNsValidateHandle (TargetHandle);
    if (!Node)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Validate/Allocate/Clear caller buffer */

    Status = AcpiUtInitializeBuffer (Buffer, ACPI_PATH_SEGMENT_LENGTH);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Just copy the ACPI name from the Node and zero terminate it */

    NodeName = AcpiUtGetNodeName (Node);
    ACPI_MOVE_NAME (Buffer->Pointer, NodeName);
    ((char *) Buffer->Pointer) [ACPI_NAME_SIZE] = 0;

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "%4.4s\n", (char *) Buffer->Pointer));
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsHandleToPathname
 *
 * PARAMETERS:  TargetHandle            - Handle of named object whose name is
 *                                        to be found
 *              Buffer                  - Where the pathname is returned
 *              NoTrailing              - Remove trailing '_' for each name
 *                                        segment
 *
 * RETURN:      Status, Buffer is filled with pathname if status is AE_OK
 *
 * DESCRIPTION: Build and return a full namespace pathname
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsHandleToPathname (
    ACPI_HANDLE             TargetHandle,
    ACPI_BUFFER             *Buffer,
    BOOLEAN                 NoTrailing)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_SIZE               RequiredSize;


    ACPI_FUNCTION_TRACE_PTR (NsHandleToPathname, TargetHandle);


    Node = AcpiNsValidateHandle (TargetHandle);
    if (!Node)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Determine size required for the caller buffer */

    RequiredSize = AcpiNsBuildNormalizedPath (Node, NULL, 0, NoTrailing);
    if (!RequiredSize)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Validate/Allocate/Clear caller buffer */

    Status = AcpiUtInitializeBuffer (Buffer, RequiredSize);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Build the path in the caller buffer */

    (void) AcpiNsBuildNormalizedPath (Node, Buffer->Pointer,
        RequiredSize, NoTrailing);

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "%s [%X]\n",
        (char *) Buffer->Pointer, (UINT32) RequiredSize));
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsBuildNormalizedPath
 *
 * PARAMETERS:  Node        - Namespace node
 *              FullPath    - Where the path name is returned
 *              PathSize    - Size of returned path name buffer
 *              NoTrailing  - Remove trailing '_' from each name segment
 *
 * RETURN:      Return 1 if the AML path is empty, otherwise returning (length
 *              of pathname + 1) which means the 'FullPath' contains a trailing
 *              null.
 *
 * DESCRIPTION: Build and return a full namespace pathname.
 *              Note that if the size of 'FullPath' isn't large enough to
 *              contain the namespace node's path name, the actual required
 *              buffer length is returned, and it should be greater than
 *              'PathSize'. So callers are able to check the returning value
 *              to determine the buffer size of 'FullPath'.
 *
 ******************************************************************************/

UINT32
AcpiNsBuildNormalizedPath (
    ACPI_NAMESPACE_NODE     *Node,
    char                    *FullPath,
    UINT32                  PathSize,
    BOOLEAN                 NoTrailing)
{
    UINT32                  Length = 0, i;
    char                    Name[ACPI_NAME_SIZE];
    BOOLEAN                 DoNoTrailing;
    char                    c, *Left, *Right;
    ACPI_NAMESPACE_NODE     *NextNode;


    ACPI_FUNCTION_TRACE_PTR (NsBuildNormalizedPath, Node);


#define ACPI_PATH_PUT8(Path, Size, Byte, Length)    \
    do {                                            \
        if ((Length) < (Size))                      \
        {                                           \
            (Path)[(Length)] = (Byte);              \
        }                                           \
        (Length)++;                                 \
    } while (0)

    /*
     * Make sure the PathSize is correct, so that we don't need to
     * validate both FullPath and PathSize.
     */
    if (!FullPath)
    {
        PathSize = 0;
    }

    if (!Node)
    {
        goto BuildTrailingNull;
    }

    NextNode = Node;
    while (NextNode && NextNode != AcpiGbl_RootNode)
    {
        if (NextNode != Node)
        {
            ACPI_PATH_PUT8(FullPath, PathSize, AML_DUAL_NAME_PREFIX, Length);
        }

        ACPI_MOVE_32_TO_32 (Name, &NextNode->Name);
        DoNoTrailing = NoTrailing;
        for (i = 0; i < 4; i++)
        {
            c = Name[4-i-1];
            if (DoNoTrailing && c != '_')
            {
                DoNoTrailing = FALSE;
            }
            if (!DoNoTrailing)
            {
                ACPI_PATH_PUT8(FullPath, PathSize, c, Length);
            }
        }

        NextNode = NextNode->Parent;
    }

    ACPI_PATH_PUT8(FullPath, PathSize, AML_ROOT_PREFIX, Length);

    /* Reverse the path string */

    if (Length <= PathSize)
    {
        Left = FullPath;
        Right = FullPath+Length - 1;

        while (Left < Right)
        {
            c = *Left;
            *Left++ = *Right;
            *Right-- = c;
        }
    }

    /* Append the trailing null */

BuildTrailingNull:
    ACPI_PATH_PUT8 (FullPath, PathSize, '\0', Length);

#undef ACPI_PATH_PUT8

    return_UINT32 (Length);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsGetNormalizedPathname
 *
 * PARAMETERS:  Node            - Namespace node whose pathname is needed
 *              NoTrailing      - Remove trailing '_' from each name segment
 *
 * RETURN:      Pointer to storage containing the fully qualified name of
 *              the node, In external format (name segments separated by path
 *              separators.)
 *
 * DESCRIPTION: Used to obtain the full pathname to a namespace node, usually
 *              for error and debug statements. All trailing '_' will be
 *              removed from the full pathname if 'NoTrailing' is specified..
 *
 ******************************************************************************/

char *
AcpiNsGetNormalizedPathname (
    ACPI_NAMESPACE_NODE     *Node,
    BOOLEAN                 NoTrailing)
{
    char                    *NameBuffer;
    ACPI_SIZE               Size;


    ACPI_FUNCTION_TRACE_PTR (NsGetNormalizedPathname, Node);


    /* Calculate required buffer size based on depth below root */

    Size = AcpiNsBuildNormalizedPath (Node, NULL, 0, NoTrailing);
    if (!Size)
    {
        return_PTR (NULL);
    }

    /* Allocate a buffer to be returned to caller */

    NameBuffer = ACPI_ALLOCATE_ZEROED (Size);
    if (!NameBuffer)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not allocate %u bytes", (UINT32) Size));
        return_PTR (NULL);
    }

    /* Build the path in the allocated buffer */

    (void) AcpiNsBuildNormalizedPath (Node, NameBuffer, Size, NoTrailing);

    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_NAMES, "%s: Path \"%s\"\n",
        ACPI_GET_FUNCTION_NAME, NameBuffer));

    return_PTR (NameBuffer);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsBuildPrefixedPathname
 *
 * PARAMETERS:  PrefixScope         - Scope/Path that prefixes the internal path
 *              InternalPath        - Name or path of the namespace node
 *
 * RETURN:      None
 *
 * DESCRIPTION: Construct a fully qualified pathname from a concatenation of:
 *              1) Path associated with the PrefixScope namespace node
 *              2) External path representation of the Internal path
 *
 ******************************************************************************/

char *
AcpiNsBuildPrefixedPathname (
    ACPI_GENERIC_STATE      *PrefixScope,
    const char              *InternalPath)
{
    ACPI_STATUS             Status;
    char                    *FullPath = NULL;
    char                    *ExternalPath = NULL;
    char                    *PrefixPath = NULL;
    UINT32                  PrefixPathLength = 0;


    /* If there is a prefix, get the pathname to it */

    if (PrefixScope && PrefixScope->Scope.Node)
    {
        PrefixPath = AcpiNsGetNormalizedPathname (PrefixScope->Scope.Node, TRUE);
        if (PrefixPath)
        {
            PrefixPathLength = strlen (PrefixPath);
        }
    }

    Status = AcpiNsExternalizeName (ACPI_UINT32_MAX, InternalPath,
        NULL, &ExternalPath);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /* Merge the prefix path and the path. 2 is for one dot and trailing null */

    FullPath = ACPI_ALLOCATE_ZEROED (
        PrefixPathLength + strlen (ExternalPath) + 2);
    if (!FullPath)
    {
        goto Cleanup;
    }

    /* Don't merge if the External path is already fully qualified */

    if (PrefixPath &&
        (*ExternalPath != '\\') &&
        (*ExternalPath != '^'))
    {
        strcat (FullPath, PrefixPath);
        if (PrefixPath[1])
        {
            strcat (FullPath, ".");
        }
    }

    AcpiNsNormalizePathname (ExternalPath);
    strcat (FullPath, ExternalPath);

Cleanup:
    if (PrefixPath)
    {
        ACPI_FREE (PrefixPath);
    }
    if (ExternalPath)
    {
        ACPI_FREE (ExternalPath);
    }

    return (FullPath);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsNormalizePathname
 *
 * PARAMETERS:  OriginalPath        - Path to be normalized, in External format
 *
 * RETURN:      The original path is processed in-place
 *
 * DESCRIPTION: Remove trailing underscores from each element of a path.
 *
 *              For example:  \A___.B___.C___ becomes \A.B.C
 *
 ******************************************************************************/

static void
AcpiNsNormalizePathname (
    char                    *OriginalPath)
{
    char                    *InputPath = OriginalPath;
    char                    *NewPathBuffer;
    char                    *NewPath;
    UINT32                  i;


    /* Allocate a temp buffer in which to construct the new path */

    NewPathBuffer = ACPI_ALLOCATE_ZEROED (strlen (InputPath) + 1);
    NewPath = NewPathBuffer;
    if (!NewPathBuffer)
    {
        return;
    }

    /* Special characters may appear at the beginning of the path */

    if (*InputPath == '\\')
    {
        *NewPath = *InputPath;
        NewPath++;
        InputPath++;
    }

    while (*InputPath == '^')
    {
        *NewPath = *InputPath;
        NewPath++;
        InputPath++;
    }

    /* Remainder of the path */

    while (*InputPath)
    {
        /* Do one nameseg at a time */

        for (i = 0; (i < ACPI_NAME_SIZE) && *InputPath; i++)
        {
            if ((i == 0) || (*InputPath != '_')) /* First char is allowed to be underscore */
            {
                *NewPath = *InputPath;
                NewPath++;
            }

            InputPath++;
        }

        /* Dot means that there are more namesegs to come */

        if (*InputPath == '.')
        {
            *NewPath = *InputPath;
            NewPath++;
            InputPath++;
        }
    }

    *NewPath = 0;
    strcpy (OriginalPath, NewPathBuffer);
    ACPI_FREE (NewPathBuffer);
}
