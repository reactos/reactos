/*******************************************************************************
 *
 * Module Name: uterror - Various internal error/warning output functions
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
#include "acnamesp.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("uterror")


/*
 * This module contains internal error functions that may
 * be configured out.
 */
#if !defined (ACPI_NO_ERROR_MESSAGES)

/*******************************************************************************
 *
 * FUNCTION:    AcpiUtPredefinedWarning
 *
 * PARAMETERS:  ModuleName      - Caller's module name (for error output)
 *              LineNumber      - Caller's line number (for error output)
 *              Pathname        - Full pathname to the node
 *              NodeFlags       - From Namespace node for the method/object
 *              Format          - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Warnings for the predefined validation module. Messages are
 *              only emitted the first time a problem with a particular
 *              method/object is detected. This prevents a flood of error
 *              messages for methods that are repeatedly evaluated.
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiUtPredefinedWarning (
    const char              *ModuleName,
    UINT32                  LineNumber,
    char                    *Pathname,
    UINT8                   NodeFlags,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    /*
     * Warning messages for this method/object will be disabled after the
     * first time a validation fails or an object is successfully repaired.
     */
    if (NodeFlags & ANOBJ_EVALUATED)
    {
        return;
    }

    AcpiOsPrintf (ACPI_MSG_WARNING "%s: ", Pathname);

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtPredefinedInfo
 *
 * PARAMETERS:  ModuleName      - Caller's module name (for error output)
 *              LineNumber      - Caller's line number (for error output)
 *              Pathname        - Full pathname to the node
 *              NodeFlags       - From Namespace node for the method/object
 *              Format          - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Info messages for the predefined validation module. Messages
 *              are only emitted the first time a problem with a particular
 *              method/object is detected. This prevents a flood of
 *              messages for methods that are repeatedly evaluated.
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiUtPredefinedInfo (
    const char              *ModuleName,
    UINT32                  LineNumber,
    char                    *Pathname,
    UINT8                   NodeFlags,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    /*
     * Warning messages for this method/object will be disabled after the
     * first time a validation fails or an object is successfully repaired.
     */
    if (NodeFlags & ANOBJ_EVALUATED)
    {
        return;
    }

    AcpiOsPrintf (ACPI_MSG_INFO "%s: ", Pathname);

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtPredefinedBiosError
 *
 * PARAMETERS:  ModuleName      - Caller's module name (for error output)
 *              LineNumber      - Caller's line number (for error output)
 *              Pathname        - Full pathname to the node
 *              NodeFlags       - From Namespace node for the method/object
 *              Format          - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: BIOS error message for predefined names. Messages
 *              are only emitted the first time a problem with a particular
 *              method/object is detected. This prevents a flood of
 *              messages for methods that are repeatedly evaluated.
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiUtPredefinedBiosError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    char                    *Pathname,
    UINT8                   NodeFlags,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    /*
     * Warning messages for this method/object will be disabled after the
     * first time a validation fails or an object is successfully repaired.
     */
    if (NodeFlags & ANOBJ_EVALUATED)
    {
        return;
    }

    AcpiOsPrintf (ACPI_MSG_BIOS_ERROR "%s: ", Pathname);

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtPrefixedNamespaceError
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              PrefixScope         - Scope/Path that prefixes the internal path
 *              InternalPath        - Name or path of the namespace node
 *              LookupStatus        - Exception code from NS lookup
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print error message with the full pathname constructed this way:
 *
 *                  PrefixScopeNodeFullPath.ExternalizedInternalPath
 *
 * NOTE:        10/2017: Treat the major NsLookup errors as firmware errors
 *
 ******************************************************************************/

void
AcpiUtPrefixedNamespaceError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    ACPI_GENERIC_STATE      *PrefixScope,
    const char              *InternalPath,
    ACPI_STATUS             LookupStatus)
{
    char                    *FullPath;
    const char              *Message;


    /*
     * Main cases:
     * 1) Object creation, object must not already exist
     * 2) Object lookup, object must exist
     */
    switch (LookupStatus)
    {
    case AE_ALREADY_EXISTS:

        AcpiOsPrintf (ACPI_MSG_BIOS_ERROR);
        Message = "Failure creating";
        break;

    case AE_NOT_FOUND:

        AcpiOsPrintf (ACPI_MSG_BIOS_ERROR);
        Message = "Failure looking up";
        break;

    default:

        AcpiOsPrintf (ACPI_MSG_ERROR);
        Message = "Failure looking up";
        break;
    }

    /* Concatenate the prefix path and the internal path */

    FullPath = AcpiNsBuildPrefixedPathname (PrefixScope, InternalPath);

    AcpiOsPrintf ("%s [%s], %s", Message,
        FullPath ? FullPath : "Could not get pathname",
        AcpiFormatException (LookupStatus));

    if (FullPath)
    {
        ACPI_FREE (FullPath);
    }

    ACPI_MSG_SUFFIX;
}


#ifdef __OBSOLETE_FUNCTION
/*******************************************************************************
 *
 * FUNCTION:    AcpiUtNamespaceError
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              InternalName        - Name or path of the namespace node
 *              LookupStatus        - Exception code from NS lookup
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print error message with the full pathname for the NS node.
 *
 ******************************************************************************/

void
AcpiUtNamespaceError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *InternalName,
    ACPI_STATUS             LookupStatus)
{
    ACPI_STATUS             Status;
    UINT32                  BadName;
    char                    *Name = NULL;


    ACPI_MSG_REDIRECT_BEGIN;
    AcpiOsPrintf (ACPI_MSG_ERROR);

    if (LookupStatus == AE_BAD_CHARACTER)
    {
        /* There is a non-ascii character in the name */

        ACPI_MOVE_32_TO_32 (&BadName, ACPI_CAST_PTR (UINT32, InternalName));
        AcpiOsPrintf ("[0x%.8X] (NON-ASCII)", BadName);
    }
    else
    {
        /* Convert path to external format */

        Status = AcpiNsExternalizeName (
            ACPI_UINT32_MAX, InternalName, NULL, &Name);

        /* Print target name */

        if (ACPI_SUCCESS (Status))
        {
            AcpiOsPrintf ("[%s]", Name);
        }
        else
        {
            AcpiOsPrintf ("[COULD NOT EXTERNALIZE NAME]");
        }

        if (Name)
        {
            ACPI_FREE (Name);
        }
    }

    AcpiOsPrintf (" Namespace lookup failure, %s",
        AcpiFormatException (LookupStatus));

    ACPI_MSG_SUFFIX;
    ACPI_MSG_REDIRECT_END;
}
#endif

/*******************************************************************************
 *
 * FUNCTION:    AcpiUtMethodError
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Message             - Error message to use on failure
 *              PrefixNode          - Prefix relative to the path
 *              Path                - Path to the node (optional)
 *              MethodStatus        - Execution status
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print error message with the full pathname for the method.
 *
 ******************************************************************************/

void
AcpiUtMethodError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Message,
    ACPI_NAMESPACE_NODE     *PrefixNode,
    const char              *Path,
    ACPI_STATUS             MethodStatus)
{
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node = PrefixNode;


    ACPI_MSG_REDIRECT_BEGIN;
    AcpiOsPrintf (ACPI_MSG_ERROR);

    if (Path)
    {
        Status = AcpiNsGetNode (PrefixNode, Path,
            ACPI_NS_NO_UPSEARCH, &Node);
        if (ACPI_FAILURE (Status))
        {
            AcpiOsPrintf ("[Could not get node by pathname]");
        }
    }

    AcpiNsPrintNodePathname (Node, Message);
    AcpiOsPrintf (", %s", AcpiFormatException (MethodStatus));

    ACPI_MSG_SUFFIX;
    ACPI_MSG_REDIRECT_END;
}

#endif /* ACPI_NO_ERROR_MESSAGES */
