/******************************************************************************
 *
 * Module Name: dspkginit - Completion of deferred package initialization
 *
 *****************************************************************************/

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
#include "acnamesp.h"
#include "amlcode.h"
#include "acdispat.h"
#include "acinterp.h"
#include "acparser.h"


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("dspkginit")


/* Local prototypes */

static void
AcpiDsResolvePackageElement (
    ACPI_OPERAND_OBJECT     **Element);


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsBuildInternalPackageObj
 *
 * PARAMETERS:  WalkState       - Current walk state
 *              Op              - Parser object to be translated
 *              ElementCount    - Number of elements in the package - this is
 *                                the NumElements argument to Package()
 *              ObjDescPtr      - Where the ACPI internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate a parser Op package object to the equivalent
 *              namespace object
 *
 * NOTE: The number of elements in the package will be always be the NumElements
 * count, regardless of the number of elements in the package list. If
 * NumElements is smaller, only that many package list elements are used.
 * if NumElements is larger, the Package object is padded out with
 * objects of type Uninitialized (as per ACPI spec.)
 *
 * Even though the ASL compilers do not allow NumElements to be smaller
 * than the Package list length (for the fixed length package opcode), some
 * BIOS code modifies the AML on the fly to adjust the NumElements, and
 * this code compensates for that. This also provides compatibility with
 * other AML interpreters.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsBuildInternalPackageObj (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  ElementCount,
    ACPI_OPERAND_OBJECT     **ObjDescPtr)
{
    ACPI_PARSE_OBJECT       *Arg;
    ACPI_PARSE_OBJECT       *Parent;
    ACPI_OPERAND_OBJECT     *ObjDesc = NULL;
    ACPI_STATUS             Status = AE_OK;
    BOOLEAN                 ModuleLevelCode = FALSE;
    UINT16                  ReferenceCount;
    UINT32                  Index;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (DsBuildInternalPackageObj);


    /* Check if we are executing module level code */

    if (WalkState->ParseFlags & ACPI_PARSE_MODULE_LEVEL)
    {
        ModuleLevelCode = TRUE;
    }

    /* Find the parent of a possibly nested package */

    Parent = Op->Common.Parent;
    while ((Parent->Common.AmlOpcode == AML_PACKAGE_OP) ||
           (Parent->Common.AmlOpcode == AML_VARIABLE_PACKAGE_OP))
    {
        Parent = Parent->Common.Parent;
    }

    /*
     * If we are evaluating a Named package object of the form:
     *      Name (xxxx, Package)
     * the package object already exists, otherwise it must be created.
     */
    ObjDesc = *ObjDescPtr;
    if (!ObjDesc)
    {
        ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_PACKAGE);
        *ObjDescPtr = ObjDesc;
        if (!ObjDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        ObjDesc->Package.Node = Parent->Common.Node;
    }

    if (ObjDesc->Package.Flags & AOPOBJ_DATA_VALID) /* Just in case */
    {
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * Allocate the element array (array of pointers to the individual
     * objects) if necessary. the count is based on the NumElements
     * parameter. Add an extra pointer slot so that the list is always
     * null terminated.
     */
    if (!ObjDesc->Package.Elements)
    {
        ObjDesc->Package.Elements = ACPI_ALLOCATE_ZEROED (
            ((ACPI_SIZE) ElementCount + 1) * sizeof (void *));

        if (!ObjDesc->Package.Elements)
        {
            AcpiUtDeleteObjectDesc (ObjDesc);
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        ObjDesc->Package.Count = ElementCount;
    }

    /* First arg is element count. Second arg begins the initializer list */

    Arg = Op->Common.Value.Arg;
    Arg = Arg->Common.Next;

    /*
     * If we are executing module-level code, we will defer the
     * full resolution of the package elements in order to support
     * forward references from the elements. This provides
     * compatibility with other ACPI implementations.
     */
    if (ModuleLevelCode)
    {
        ObjDesc->Package.AmlStart = WalkState->Aml;
        ObjDesc->Package.AmlLength = 0;

        ACPI_DEBUG_PRINT_RAW ((ACPI_DB_PARSE,
            "%s: Deferring resolution of Package elements\n",
            ACPI_GET_FUNCTION_NAME));
    }

    /*
     * Initialize the elements of the package, up to the NumElements count.
     * Package is automatically padded with uninitialized (NULL) elements
     * if NumElements is greater than the package list length. Likewise,
     * Package is truncated if NumElements is less than the list length.
     */
    for (i = 0; Arg && (i < ElementCount); i++)
    {
        if (Arg->Common.AmlOpcode == AML_INT_RETURN_VALUE_OP)
        {
            if (!Arg->Common.Node)
            {
                /*
                 * This is the case where an expression has returned a value.
                 * The use of expressions (TermArgs) within individual
                 * package elements is not supported by the AML interpreter,
                 * even though the ASL grammar supports it. Example:
                 *
                 *      Name (INT1, 0x1234)
                 *
                 *      Name (PKG3, Package () {
                 *          Add (INT1, 0xAAAA0000)
                 *      })
                 *
                 *  1) No known AML interpreter supports this type of construct
                 *  2) This fixes a fault if the construct is encountered
                 */
                ACPI_EXCEPTION ((AE_INFO, AE_SUPPORT,
                    "Expressions within package elements are not supported"));

                /* Cleanup the return object, it is not needed */

                AcpiUtRemoveReference (WalkState->Results->Results.ObjDesc[0]);
                return_ACPI_STATUS (AE_SUPPORT);
            }

            if (Arg->Common.Node->Type == ACPI_TYPE_METHOD)
            {
                /*
                 * A method reference "looks" to the parser to be a method
                 * invocation, so we special case it here
                 */
                Arg->Common.AmlOpcode = AML_INT_NAMEPATH_OP;
                Status = AcpiDsBuildInternalObject (
                    WalkState, Arg, &ObjDesc->Package.Elements[i]);
            }
            else
            {
                /* This package element is already built, just get it */

                ObjDesc->Package.Elements[i] =
                    ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, Arg->Common.Node);
            }
        }
        else
        {
            Status = AcpiDsBuildInternalObject (
                WalkState, Arg, &ObjDesc->Package.Elements[i]);
            if (Status == AE_NOT_FOUND)
            {
                ACPI_ERROR ((AE_INFO, "%-48s", "****DS namepath not found"));
            }

            if (!ModuleLevelCode)
            {
                /*
                 * Initialize this package element. This function handles the
                 * resolution of named references within the package.
                 * Forward references from module-level code are deferred
                 * until all ACPI tables are loaded.
                 */
                AcpiDsInitPackageElement (0, ObjDesc->Package.Elements[i],
                    NULL, &ObjDesc->Package.Elements[i]);
            }
        }

        if (*ObjDescPtr)
        {
            /* Existing package, get existing reference count */

            ReferenceCount = (*ObjDescPtr)->Common.ReferenceCount;
            if (ReferenceCount > 1)
            {
                /* Make new element ref count match original ref count */
                /* TBD: Probably need an AcpiUtAddReferences function */

                for (Index = 0; Index < ((UINT32) ReferenceCount - 1); Index++)
                {
                    AcpiUtAddReference ((ObjDesc->Package.Elements[i]));
                }
            }
        }

        Arg = Arg->Common.Next;
    }

    /* Check for match between NumElements and actual length of PackageList */

    if (Arg)
    {
        /*
         * NumElements was exhausted, but there are remaining elements in
         * the PackageList. Truncate the package to NumElements.
         *
         * Note: technically, this is an error, from ACPI spec: "It is an
         * error for NumElements to be less than the number of elements in
         * the PackageList". However, we just print a message and no
         * exception is returned. This provides compatibility with other
         * ACPI implementations. Some firmware implementations will alter
         * the NumElements on the fly, possibly creating this type of
         * ill-formed package object.
         */
        while (Arg)
        {
            /*
             * We must delete any package elements that were created earlier
             * and are not going to be used because of the package truncation.
             */
            if (Arg->Common.Node)
            {
                AcpiUtRemoveReference (
                    ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, Arg->Common.Node));
                Arg->Common.Node = NULL;
            }

            /* Find out how many elements there really are */

            i++;
            Arg = Arg->Common.Next;
        }

        ACPI_INFO ((
            "Actual Package length (%u) is larger than "
            "NumElements field (%u), truncated",
            i, ElementCount));
    }
    else if (i < ElementCount)
    {
        /*
         * Arg list (elements) was exhausted, but we did not reach
         * NumElements count.
         *
         * Note: this is not an error, the package is padded out
         * with NULLs as per the ACPI specification.
         */
        ACPI_DEBUG_PRINT_RAW ((ACPI_DB_INFO,
            "%s: Package List length (%u) smaller than NumElements "
            "count (%u), padded with null elements\n",
            ACPI_GET_FUNCTION_NAME, i, ElementCount));
    }

    /* Module-level packages will be resolved later */

    if (!ModuleLevelCode)
    {
        ObjDesc->Package.Flags |= AOPOBJ_DATA_VALID;
    }

    Op->Common.Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsInitPackageElement
 *
 * PARAMETERS:  ACPI_PKG_CALLBACK
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Resolve a named reference element within a package object
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsInitPackageElement (
    UINT8                   ObjectType,
    ACPI_OPERAND_OBJECT     *SourceObject,
    ACPI_GENERIC_STATE      *State,
    void                    *Context)
{
    ACPI_OPERAND_OBJECT     **ElementPtr;


    ACPI_FUNCTION_TRACE (DsInitPackageElement);


    if (!SourceObject)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * The following code is a bit of a hack to workaround a (current)
     * limitation of the ACPI_PKG_CALLBACK interface. We need a pointer
     * to the location within the element array because a new object
     * may be created and stored there.
     */
    if (Context)
    {
        /* A direct call was made to this function */

        ElementPtr = (ACPI_OPERAND_OBJECT **) Context;
    }
    else
    {
        /* Call came from AcpiUtWalkPackageTree */

        ElementPtr = State->Pkg.ThisTargetObj;
    }

    /* We are only interested in reference objects/elements */

    if (SourceObject->Common.Type == ACPI_TYPE_LOCAL_REFERENCE)
    {
        /* Attempt to resolve the (named) reference to a namespace node */

        AcpiDsResolvePackageElement (ElementPtr);
    }
    else if (SourceObject->Common.Type == ACPI_TYPE_PACKAGE)
    {
        SourceObject->Package.Flags |= AOPOBJ_DATA_VALID;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsResolvePackageElement
 *
 * PARAMETERS:  ElementPtr          - Pointer to a reference object
 *
 * RETURN:      Possible new element is stored to the indirect ElementPtr
 *
 * DESCRIPTION: Resolve a package element that is a reference to a named
 *              object.
 *
 ******************************************************************************/

static void
AcpiDsResolvePackageElement (
    ACPI_OPERAND_OBJECT     **ElementPtr)
{
    ACPI_STATUS             Status;
    ACPI_STATUS             Status2;
    ACPI_GENERIC_STATE      ScopeInfo;
    ACPI_OPERAND_OBJECT     *Element = *ElementPtr;
    ACPI_NAMESPACE_NODE     *ResolvedNode;
    ACPI_NAMESPACE_NODE     *OriginalNode;
    char                    *ExternalPath = "";
    ACPI_OBJECT_TYPE        Type;


    ACPI_FUNCTION_TRACE (DsResolvePackageElement);


    /* Check if reference element is already resolved */

    if (Element->Reference.Resolved)
    {
        ACPI_DEBUG_PRINT_RAW ((ACPI_DB_PARSE,
            "%s: Package element is already resolved\n",
            ACPI_GET_FUNCTION_NAME));

        return_VOID;
    }

    /* Element must be a reference object of correct type */

    ScopeInfo.Scope.Node = Element->Reference.Node; /* Prefix node */

    Status = AcpiNsLookup (&ScopeInfo, (char *) Element->Reference.Aml,
        ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
        ACPI_NS_SEARCH_PARENT | ACPI_NS_DONT_OPEN_SCOPE,
        NULL, &ResolvedNode);
    if (ACPI_FAILURE (Status))
    {
        if ((Status == AE_NOT_FOUND) && AcpiGbl_IgnorePackageResolutionErrors)
        {
            /*
             * Optionally be silent about the NOT_FOUND case for the referenced
             * name. Although this is potentially a serious problem,
             * it can generate a lot of noise/errors on platforms whose
             * firmware carries around a bunch of unused Package objects.
             * To disable these errors, set this global to TRUE:
             *     AcpiGbl_IgnorePackageResolutionErrors
             *
             * If the AML actually tries to use such a package, the unresolved
             * element(s) will be replaced with NULL elements.
             */

            /* Referenced name not found, set the element to NULL */

            AcpiUtRemoveReference (*ElementPtr);
            *ElementPtr = NULL;
            return_VOID;
        }

        Status2 = AcpiNsExternalizeName (ACPI_UINT32_MAX,
            (char *) Element->Reference.Aml, NULL, &ExternalPath);

        ACPI_EXCEPTION ((AE_INFO, Status,
            "While resolving a named reference package element - %s",
            ExternalPath));
        if (ACPI_SUCCESS (Status2))
        {
            ACPI_FREE (ExternalPath);
        }

        /* Could not resolve name, set the element to NULL */

        AcpiUtRemoveReference (*ElementPtr);
        *ElementPtr = NULL;
        return_VOID;
    }
    else if (ResolvedNode->Type == ACPI_TYPE_ANY)
    {
        /* Named reference not resolved, return a NULL package element */

        ACPI_ERROR ((AE_INFO,
            "Could not resolve named package element [%4.4s] in [%4.4s]",
            ResolvedNode->Name.Ascii, ScopeInfo.Scope.Node->Name.Ascii));
        *ElementPtr = NULL;
        return_VOID;
    }

    /*
     * Special handling for Alias objects. We need ResolvedNode to point
     * to the Alias target. This effectively "resolves" the alias.
     */
    if (ResolvedNode->Type == ACPI_TYPE_LOCAL_ALIAS)
    {
        ResolvedNode = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE,
            ResolvedNode->Object);
    }

    /* Update the reference object */

    Element->Reference.Resolved = TRUE;
    Element->Reference.Node = ResolvedNode;
    Type = Element->Reference.Node->Type;

    /*
     * Attempt to resolve the node to a value before we insert it into
     * the package. If this is a reference to a common data type,
     * resolve it immediately. According to the ACPI spec, package
     * elements can only be "data objects" or method references.
     * Attempt to resolve to an Integer, Buffer, String or Package.
     * If cannot, return the named reference (for things like Devices,
     * Methods, etc.) Buffer Fields and Fields will resolve to simple
     * objects (int/buf/str/pkg).
     *
     * NOTE: References to things like Devices, Methods, Mutexes, etc.
     * will remain as named references. This behavior is not described
     * in the ACPI spec, but it appears to be an oversight.
     */
    OriginalNode = ResolvedNode;
    Status = AcpiExResolveNodeToValue (&ResolvedNode, NULL);
    if (ACPI_FAILURE (Status))
    {
        return_VOID;
    }

    switch (Type)
    {
    /*
     * These object types are a result of named references, so we will
     * leave them as reference objects. In other words, these types
     * have no intrinsic "value".
     */
    case ACPI_TYPE_DEVICE:
    case ACPI_TYPE_THERMAL:
    case ACPI_TYPE_METHOD:
        break;

    case ACPI_TYPE_MUTEX:
    case ACPI_TYPE_POWER:
    case ACPI_TYPE_PROCESSOR:
    case ACPI_TYPE_EVENT:
    case ACPI_TYPE_REGION:

        /* AcpiExResolveNodeToValue gave these an extra reference */

        AcpiUtRemoveReference (OriginalNode->Object);
        break;

    default:
        /*
         * For all other types - the node was resolved to an actual
         * operand object with a value, return the object. Remove
         * a reference on the existing object.
         */
        AcpiUtRemoveReference (Element);
        *ElementPtr = (ACPI_OPERAND_OBJECT *) ResolvedNode;
        break;
    }

    return_VOID;
}
