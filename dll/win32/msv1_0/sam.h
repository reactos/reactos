/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Security Account Manager (SAM) related functions - header
 * COPYRIGHT:   Copyright 2013 Eric Kohl <eric.kohl@reactos.org>
 */

#pragma once

typedef struct _LSA_SAM_PWD_DATA
{
    /* TRUE: PlainPwd is filled,
       FALSE: LmPwd and NtPwd is filled */
    BOOL IsNetwork;
    PUNICODE_STRING PlainPwd;

} LSA_SAM_PWD_DATA, *PLSA_SAM_PWD_DATA;

/**
 * @brief Validates a normal user by checking if it exists in the sam database.
 *        Further some other checks are done.
 */
NTSTATUS
SamValidateNormalUser(
    _In_ PUNICODE_STRING UserName,
    _In_ PLSA_SAM_PWD_DATA PwdData,
    _In_ PUNICODE_STRING ComputerName,
    _Out_ PRPC_SID* AccountDomainSidPtr,
    _Out_ SAMPR_HANDLE* UserHandlePtr,
    _Out_ PSAMPR_USER_INFO_BUFFER* UserInfoPtr,
    _Out_ PNTSTATUS SubStatus);
