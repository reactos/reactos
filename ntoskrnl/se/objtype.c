/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security object type list support routines
 * COPYRIGHT:       Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS ***********************************************************/

/**
 * @brief
 * Captures a list of object types.
 *
 * @param[in] ObjectTypeList
 * An existing list of object types.
 *
 * @param[in] ObjectTypeListLength
 * The length size of the list.
 *
 * @param[in] PreviousMode
 * Processor access level mode.
 *
 * @param[out] CapturedObjectTypeList
 * The captured list of object types.
 *
 * @return
 * Returns STATUS_SUCCESS if the list of object types has been captured
 * successfully. STATUS_INVALID_PARAMETER is returned if the caller hasn't
 * supplied a buffer list of object types. STATUS_INSUFFICIENT_RESOURCES
 * is returned if pool memory allocation for the captured list has failed.
 */
NTSTATUS
SeCaptureObjectTypeList(
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ POBJECT_TYPE_LIST *CapturedObjectTypeList)
{
    SIZE_T Size;

    if (PreviousMode == KernelMode)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    if (ObjectTypeListLength == 0)
    {
        *CapturedObjectTypeList = NULL;
        return STATUS_SUCCESS;
    }

    if (ObjectTypeList == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Calculate the list size and check for integer overflow */
    Size = ObjectTypeListLength * sizeof(OBJECT_TYPE_LIST);
    if (Size == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate a new list */
    *CapturedObjectTypeList = ExAllocatePoolWithTag(PagedPool, Size, TAG_SEPA);
    if (*CapturedObjectTypeList == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY
    {
        ProbeForRead(ObjectTypeList, Size, sizeof(ULONG));
        RtlCopyMemory(*CapturedObjectTypeList, ObjectTypeList, Size);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(*CapturedObjectTypeList, TAG_SEPA);
        *CapturedObjectTypeList = NULL;
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
 *
 * @return
 * Nothing.
 */
VOID
SeReleaseObjectTypeList(
    _In_  _Post_invalid_ POBJECT_TYPE_LIST CapturedObjectTypeList,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    if ((PreviousMode != KernelMode) && (CapturedObjectTypeList != NULL))
        ExFreePoolWithTag(CapturedObjectTypeList, TAG_SEPA);
}

/* EOF */
