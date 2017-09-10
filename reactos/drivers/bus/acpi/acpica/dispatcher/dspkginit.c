/******************************************************************************
 *
 * Module Name: dspkginit - Completion of deferred package initialization
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
#include "amlcode.h"
#include "acdispat.h"
#include "acinterp.h"


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
    UINT16                  ReferenceCount;
    UINT32                  Index;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (DsBuildInternalPackageObj);


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
     * objects) based on the NumElements parameter. Add an extra pointer slot
     * so that the list is always null terminated.
     */
    ObjDesc->Package.Elements = ACPI_ALLOCATE_ZEROED (
        ((ACPI_SIZE) ElementCount + 1) * sizeof (void *));

    if (!ObjDesc->Package.Elements)
    {
        AcpiUtDeleteObjectDesc (ObjDesc);
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    ObjDesc->Package.Count = ElementCount;
    Arg = Op->Common.Value.Arg;
    Arg = Arg->Common.Next;

    if (Arg)
    {
        ObjDesc->Package.Flags |= AOPOBJ_DATA_VALID;
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

            /*
             * Initialize this package element. This function handles the
             * resolution of named references within the package.
             */
            AcpiDsInitPackageElement (0, ObjDesc->Package.Elements[i],
                NULL, &ObjDesc->Package.Elements[i]);
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
         * with NULLs.
         */
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Package List length (%u) smaller than NumElements "
            "count (%u), padded with null elements\n",
            i, ElementCount));
    }

    ObjDesc->Package.Flags |= AOPOBJ_DATA_VALID;
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


    if (!SourceObject)
    {
        return (AE_OK);
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

    return (AE_OK);
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
    ACPI_GENERIC_STATE      ScopeInfo;
    ACPI_OPERAND_OBJECT     *Element = *ElementPtr;
    ACPI_NAMESPACE_NODE     *ResolvedNode;
    char                    *ExternalPath = NULL;
    ACPI_OBJECT_TYPE        Type;


    ACPI_FUNCTION_TRACE (DsResolvePackageElement);


    /* Check if reference element is already resolved */

    if (Element->Reference.Resolved)
    {
        return_VOID;
    }

    /* Element must be a reference object of correct type */

    ScopeInfo.Scope.Node = Element->Reference.Node; /* Prefix node */

    Status = AcpiNsLookup (&ScopeInfo,
        (char *) Element->Reference.Aml,            /* Pointer to AML path */
        ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
        ACPI_NS_SEARCH_PARENT | ACPI_NS_DONT_OPEN_SCOPE,
        NULL, &ResolvedNode);
    if (ACPI_FAILURE (Status))
    {
        Status = AcpiNsExternalizeName (ACPI_UINT32_MAX,
            (char *) Element->Reference.Aml,
            NULL, &ExternalPath);

        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not find/resolve named package element: %s", ExternalPath));

        ACPI_FREE (ExternalPath);
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
#if 0
    else if (ResolvedNode->Flags & ANOBJ_TEMPORARY)
    {
        /*
         * A temporary node found here indicates that the reference is
         * to a node that was created within this method. We are not
         * going to allow it (especially if the package is returned
         * from the method) -- the temporary node will be deleted out
         * from under the method. (05/2017).
         */
        ACPI_ERROR ((AE_INFO,
            "Package element refers to a temporary name [%4.4s], "
            "inserting a NULL element",
            ResolvedNode->Name.Ascii));
        *ElementPtr = NULL;
        return_VOID;
    }
#endif

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
    Status = AcpiExResolveNodeToValue (&ResolvedNode, NULL);
    if (ACPI_FAILURE (Status))
    {
        return_VOID;
    }

#if 0
/* TBD - alias support */
    /*
     * Special handling for Alias objects. We need to setup the type
     * and the Op->Common.Node to point to the Alias target. Note,
     * Alias has at most one level of indirection internally.
     */
    Type = Op->Common.Node->Type;
    if (Type == ACPI_TYPE_LOCAL_ALIAS)
    {
        Type = ObjDesc->Common.Type;
        Op->Common.Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE,
            Op->Common.Node->Object);
    }
#endif

    switch (Type)
    {
    /*
     * These object types are a result of named references, so we will
     * leave them as reference objects. In other words, these types
     * have no intrinsic "value".
     */
    case ACPI_TYPE_DEVICE:
    case ACPI_TYPE_THERMAL:

        /* TBD: This may not be necesssary */

        AcpiUtAddReference (ResolvedNode->Object);
        break;

    case ACPI_TYPE_MUTEX:
    case ACPI_TYPE_METHOD:
    case ACPI_TYPE_POWER:
    case ACPI_TYPE_PROCESSOR:
    case ACPI_TYPE_EVENT:
    case ACPI_TYPE_REGION:

        break;

    default:
        /*
         * For all other types - the node was resolved to an actual
         * operand object with a value, return the object
         */
        *ElementPtr = (ACPI_OPERAND_OBJECT *) ResolvedNode;
        break;
    }

    return_VOID;
}
