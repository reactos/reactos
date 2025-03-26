/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Security object type list support routines
 * COPYRIGHT:   Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Validates a list of object types passed from user mode,
 * ensuring the following conditions are met for a valid
 * list:
 *
 * - The list must not be too big and it can be read
 * - Each object must have a valid level
 * - The level hierarchy between objects has to be consistent
 *   (e.g. a root cannot have a level 2 subordinate object)
 * - The list must have only one root and it must be in the
 *   first position
 * - Each object type GUID can be read and captured
 *
 * @param[in] ObjectTypeList
 * A pointer to an object type list of which the elements
 * are being validated.
 *
 * @param[in] ObjectTypeListLength
 * The length of the list, representing the number of object
 * elements in that list.
 *
 * @return
 * Returns STATUS_SUCCESS if the list has been validated and it
 * contains valid objects. STATUS_INVALID_PARAMETER is returned
 * if the list is not valid. Otherwise a NTSTATUS code is returned.
 */
static
NTSTATUS
SepValidateObjectTypeList(
    _In_reads_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength)
{
    PGUID ObjectTypeGuid;
    ULONG ObjectTypeIndex;
    USHORT Level, PrevLevel;
    SIZE_T Size;

    /* Ensure we do not hit an integer overflow */
    Size = ObjectTypeListLength * sizeof(OBJECT_TYPE_LIST);
    if (Size == 0)
    {
        DPRINT1("The object type list is too big, integer overflow alert!\n");
        return STATUS_INVALID_PARAMETER;
    }

    _SEH2_TRY
    {
        /* Ensure we can actually read from that list */
        ProbeForRead(ObjectTypeList, Size, sizeof(ULONG));

        /* Begin looping for each object from the list */
        for (ObjectTypeIndex = 0;
             ObjectTypeIndex < ObjectTypeListLength;
             ObjectTypeIndex++)
        {
            /* Get the level of this object and check for validity */
            Level = ObjectTypeList[ObjectTypeIndex].Level;
            if (Level > ACCESS_MAX_LEVEL)
            {
                DPRINT1("Invalid object level found (level %u, at index %lu)\n", Level, ObjectTypeIndex);
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
            }

            /* Are we past the first position in the list? */
            if (ObjectTypeIndex != 0)
            {
                /* Ensure that we do not have two object roots */
                if (Level == ACCESS_OBJECT_GUID)
                {
                    DPRINT1("This list has two roots (at index %lu)\n", ObjectTypeIndex);
                    _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
                }

                /*
                 * Ensure the current level is consistent with the prior level.
                 * That means, if the previous object is the root (denoted by the
                 * level as ACCESS_OBJECT_GUID aka 0) the current object must
                 * be a child of the parent which is the root (also called a
                 * property set). Whereas a property is a sibling of the
                 * child, the property set.
                 */
                if (Level > PrevLevel + 1)
                {
                    DPRINT1("The object levels are not consistent (current level %u, previous level %u, at index %lu)\n",
                            Level, PrevLevel, ObjectTypeIndex);
                    _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
                }
            }
            else
            {
                /* This is the first position so the object must be the root */
                if (Level != ACCESS_OBJECT_GUID)
                {
                    DPRINT1("The object is not the root at first index!\n");
                    _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
                }
            }

            /* Get the object type and check that we can read from it */
            ObjectTypeGuid = ObjectTypeList[ObjectTypeIndex].ObjectType;
            ProbeForRead(ObjectTypeGuid, sizeof(GUID), sizeof(ULONG));

            /*
             * Cache the level, we need it to ensure the levels between
             * the previous and the next object are consistent with each other.
             */
            PrevLevel = Level;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Compares two object type GUIDs for equality.
 *
 * @param[in] Guid1
 * A pointer to the first object type GUID.
 *
 * @param[in] Guid2
 * A pointer to the second object type GUID.
 *
 * @return
 * Returns TRUE if both GUIDs are equal, FALSE otherwise.
 */
static
BOOLEAN
SepIsEqualObjectTypeGuid(
    _In_ CONST GUID *Guid1,
    _In_ CONST GUID *Guid2)
{
    return RtlCompareMemory(Guid1, Guid2, sizeof(GUID)) == sizeof(GUID);
}

/* PUBLIC FUNCTIONS ************************************************************/

/**
 * @brief
 * Captures an object type GUID from an object
 * access control entry (ACE).
 *
 * @param[in] Ace
 * A pointer to an access control entry, of which
 * the object type GUID is to be captured from.
 *
 * @param[in] IsAceDenied
 * If set to TRUE, the function will capture the
 * GUID from a denied object ACE, otherwise from
 * the allowed object ACE.
 *
 * @return
 * Returns a pointer to an object type GUID, otherwise
 * NULL is returned if the target ACE does not have
 * an object type GUID.
 */
PGUID
SepGetObjectTypeGuidFromAce(
    _In_ PACE Ace,
    _In_ BOOLEAN IsAceDenied)
{
    PGUID ObjectTypeGuid = NULL;

    PAGED_CODE();

    /* This Ace must not be NULL */
    ASSERT(Ace);

    /* Grab the GUID based on the object type ACE header */
    ObjectTypeGuid = IsAceDenied ? (PGUID)&((PACCESS_DENIED_OBJECT_ACE)Ace)->ObjectType :
        (PGUID)&((PACCESS_ALLOWED_OBJECT_ACE)Ace)->ObjectType;
    return ObjectTypeGuid;
}

/**
 * @brief
 * Searches for an object type GUID if it exists
 * on an object type list.
 *
 * @param[in] ObjectTypeList
 * A pointer to an object type list.
 *
 * @param[in] ObjectTypeListLength
 * The length of the list, representing the number
 * of object elements in that list.
 *
 * @param[in] ObjectTypeGuid
 * A pointer to an object type GUID to search in the
 * list of interest.
 *
 * @param[out] ObjectIndex
 * If the function found the target GUID, the function
 * returns a pointer to the object index location to
 * this parameter.
 *
 * @return
 * Returns TRUE if the object type GUID of interest exists
 * in the target list, FALSE otherwise.
 */
BOOLEAN
SepObjectTypeGuidInList(
    _In_reads_(ObjectTypeListLength) POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGUID ObjectTypeGuid,
    _Out_ PULONG ObjectIndex)
{
    ULONG ObjectTypeIndex;
    GUID ObjectTypeGuidToSearch;

    PAGED_CODE();

    /* Loop over the object elements */
    for (ObjectTypeIndex = 0;
         ObjectTypeIndex < ObjectTypeListLength;
         ObjectTypeIndex++)
    {
        /* Is this the object we are searching for? */
        ObjectTypeGuidToSearch = ObjectTypeList[ObjectTypeIndex].ObjectTypeGuid;
        if (SepIsEqualObjectTypeGuid(ObjectTypeGuid, &ObjectTypeGuidToSearch))
        {
            /* Return the indext of that object to caller */
            *ObjectIndex = ObjectTypeIndex;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * @brief
 * Captures a list of object types and converts it to
 * an internal form for use by the kernel. The list
 * is validated before its data is copied.
 *
 * @param[in] ObjectTypeList
 * A pointer to a list of object types passed from
 * UM to be captured.
 *
 * @param[in] ObjectTypeListLength
 * The length size of the list. This length represents
 * the number of object elements in that list.
 *
 * @param[in] PreviousMode
 * Processor access level mode. This has to be set to
 * UserMode as object type access check is not supported
 * in the kernel.
 *
 * @param[out] CapturedObjectTypeList
 * A pointer to a returned captured list of object types.
 *
 * @return
 * Returns STATUS_SUCCESS if the list of object types has been captured
 * successfully. STATUS_INVALID_PARAMETER is returned if the caller hasn't
 * supplied a buffer list of object types or the list is invalid.
 * STATUS_INSUFFICIENT_RESOURCES is returned if pool memory allocation
 * for the captured list has failed.
 */
NTSTATUS
SeCaptureObjectTypeList(
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ POBJECT_TYPE_LIST_INTERNAL *CapturedObjectTypeList)
{
    NTSTATUS Status;
    ULONG ObjectTypeIndex;
    SIZE_T Size;
    PGUID ObjectTypeGuid;
    POBJECT_TYPE_LIST_INTERNAL InternalTypeList;

    PAGED_CODE();

    /* We do not support that in the kernel */
    if (PreviousMode == KernelMode)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    /* No count elements of objects means no captured list for you */
    if (ObjectTypeListLength == 0)
    {
        *CapturedObjectTypeList = NULL;
        return STATUS_SUCCESS;
    }

    /* Check if the caller supplied a list since we have the count of elements */
    if (ObjectTypeList == NULL)
    {
        DPRINT1("The caller did not provide a list of object types!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate that list before we copy contents from it */
    Status = SepValidateObjectTypeList(ObjectTypeList, ObjectTypeListLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SepValidateObjectTypeList failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Allocate a new list */
    Size = ObjectTypeListLength * sizeof(OBJECT_TYPE_LIST_INTERNAL);
    InternalTypeList = ExAllocatePoolWithTag(PagedPool, Size, TAG_SEPA);
    if (InternalTypeList == NULL)
    {
        DPRINT1("Failed to allocate pool memory for the object type list!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY
    {
        /* Loop for every object element, data was already probed */
        for (ObjectTypeIndex = 0;
             ObjectTypeIndex < ObjectTypeListLength;
             ObjectTypeIndex++)
        {
            /* Copy the object type GUID */
            ObjectTypeGuid = ObjectTypeList[ObjectTypeIndex].ObjectType;
            InternalTypeList[ObjectTypeIndex].ObjectTypeGuid = *ObjectTypeGuid;

            /* Copy the object hierarchy level */
            InternalTypeList[ObjectTypeIndex].Level = ObjectTypeList[ObjectTypeIndex].Level;

            /* Initialize the access check rights */
            InternalTypeList[ObjectTypeIndex].ObjectAccessRights.RemainingAccessRights = 0;
            InternalTypeList[ObjectTypeIndex].ObjectAccessRights.GrantedAccessRights = 0;
            InternalTypeList[ObjectTypeIndex].ObjectAccessRights.DeniedAccessRights = 0;
        }

        /* Give the captured list to caller */
        *CapturedObjectTypeList = InternalTypeList;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(InternalTypeList, TAG_SEPA);
        InternalTypeList = NULL;
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Releases a buffer list of object types.
 *
 * @param[in] CapturedObjectTypeList
 * A list of object types to free.
 *
 * @param[in] PreviousMode
 * Processor access level mode.
 */
VOID
SeReleaseObjectTypeList(
    _In_  _Post_invalid_ POBJECT_TYPE_LIST_INTERNAL CapturedObjectTypeList,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PAGED_CODE();

    if ((PreviousMode != KernelMode) && (CapturedObjectTypeList != NULL))
    {
        ExFreePoolWithTag(CapturedObjectTypeList, TAG_SEPA);
    }
}

/* EOF */
