/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Security access state functions support
 * COPYRIGHT:   Copyright Alex Ionescu <alex@relsoft.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * An extended function that creates an access state.
 *
 * @param[in] Thread
 * Valid thread object where subject context is to be captured.
 *
 * @param[in] Process
 * Valid process object where subject context is to be captured.
 *
 * @param[out] AccessState
 * An initialized returned parameter to an access state.
 *
 * @param[out] AuxData
 * Auxiliary security data for access state.
 *
 * @param[in] Access
 * Type of access mask to assign.
 *
 * @param[in] GenericMapping
 * Generic mapping for the access state to assign.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
NTSTATUS
NTAPI
SeCreateAccessStateEx(
    _In_ PETHREAD Thread,
    _In_ PEPROCESS Process,
    _Out_ PACCESS_STATE AccessState,
    _Out_ __drv_aliasesMem PAUX_ACCESS_DATA AuxData,
    _In_ ACCESS_MASK Access,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    ACCESS_MASK AccessMask = Access;
    PTOKEN Token;
    PAGED_CODE();

    /* Map the Generic Acess to Specific Access if we have a Mapping */
    if ((Access & GENERIC_ACCESS) && (GenericMapping))
    {
        RtlMapGenericMask(&AccessMask, GenericMapping);
    }

    /* Initialize the Access State */
    RtlZeroMemory(AccessState, sizeof(ACCESS_STATE));
    ASSERT(AccessState->SecurityDescriptor == NULL);
    ASSERT(AccessState->PrivilegesAllocated == FALSE);

    /* Initialize and save aux data */
    RtlZeroMemory(AuxData, sizeof(AUX_ACCESS_DATA));
    AccessState->AuxData = AuxData;

    /* Capture the Subject Context */
    SeCaptureSubjectContextEx(Thread,
                              Process,
                              &AccessState->SubjectSecurityContext);

    /* Set Access State Data */
    AccessState->RemainingDesiredAccess = AccessMask;
    AccessState->OriginalDesiredAccess = AccessMask;
    ExAllocateLocallyUniqueId(&AccessState->OperationID);

    /* Get the Token to use */
    Token = SeQuerySubjectContextToken(&AccessState->SubjectSecurityContext);

    /* Check for Travers Privilege */
    if (Token->TokenFlags & TOKEN_HAS_TRAVERSE_PRIVILEGE)
    {
        /* Preserve the Traverse Privilege */
        AccessState->Flags = TOKEN_HAS_TRAVERSE_PRIVILEGE;
    }

    /* Set the Auxiliary Data */
    AuxData->PrivilegesUsed = (PPRIVILEGE_SET)((ULONG_PTR)AccessState +
                                             FIELD_OFFSET(ACCESS_STATE,
                                                          Privileges));
    if (GenericMapping) AuxData->GenericMapping = *GenericMapping;

    /* Return Sucess */
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Creates an access state.
 *
 * @param[out] AccessState
 * An initialized returned parameter to an access state.
 *
 * @param[out] AuxData
 * Auxiliary security data for access state.
 *
 * @param[in] Access
 * Type of access mask to assign.
 *
 * @param[in] GenericMapping
 * Generic mapping for the access state to assign.
 *
 * @return
 * See SeCreateAccessStateEx.
 */
NTSTATUS
NTAPI
SeCreateAccessState(
    _Out_ PACCESS_STATE AccessState,
    _Out_ __drv_aliasesMem PAUX_ACCESS_DATA AuxData,
    _In_ ACCESS_MASK Access,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    /* Call the extended API */
    return SeCreateAccessStateEx(PsGetCurrentThread(),
                                 PsGetCurrentProcess(),
                                 AccessState,
                                 AuxData,
                                 Access,
                                 GenericMapping);
}

/**
 * @brief
 * Deletes an allocated access state from the memory.
 *
 * @param[in] AccessState
 * A valid access state.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeDeleteAccessState(
    _In_ PACCESS_STATE AccessState)
{
    PAUX_ACCESS_DATA AuxData;
    PAGED_CODE();

    /* Get the Auxiliary Data */
    AuxData = AccessState->AuxData;

    /* Deallocate Privileges */
    if (AccessState->PrivilegesAllocated)
        ExFreePoolWithTag(AuxData->PrivilegesUsed, TAG_PRIVILEGE_SET);

    /* Deallocate Name and Type Name */
    if (AccessState->ObjectName.Buffer)
    {
        ExFreePool(AccessState->ObjectName.Buffer);
    }

    if (AccessState->ObjectTypeName.Buffer)
    {
        ExFreePool(AccessState->ObjectTypeName.Buffer);
    }

    /* Release the Subject Context */
    SeReleaseSubjectContext(&AccessState->SubjectSecurityContext);
}

/**
 * @brief
 * Sets a new generic mapping for an allocated access state.
 *
 * @param[in] AccessState
 * A valid access state.
 *
 * @param[in] GenericMapping
 * New generic mapping to assign.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeSetAccessStateGenericMapping(
    _In_ PACCESS_STATE AccessState,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    /* Set the Generic Mapping */
    ((PAUX_ACCESS_DATA)AccessState->AuxData)->GenericMapping = *GenericMapping;
}

/* EOF */
