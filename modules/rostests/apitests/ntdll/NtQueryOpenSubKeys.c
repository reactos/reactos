/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for the NtQueryOpenSubKeys API
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

START_TEST(NtQueryOpenSubKeys)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Subkeys;
    UNICODE_STRING RegistryKey = RTL_CONSTANT_STRING(L"\\Registry");
    UNICODE_STRING SystemKey = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM");
    UNICODE_STRING SoftwareKey = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SOFTWARE");
    UNICODE_STRING DefaultUserKey = RTL_CONSTANT_STRING(L"\\Registry\\User\\.DEFAULT");

    /* We give no object attributes and no return variable */
    Status = NtQueryOpenSubKeys(NULL, NULL);
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* Build a key path to the main registry tree */
    InitializeObjectAttributes(&ObjectAttributes,
                               &RegistryKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* We give object attributes but no return variable */
    Status = NtQueryOpenSubKeys(&ObjectAttributes, NULL);
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /*
     * We give no object attributes but return variable.
     *
     * NOTE: Windows 10 and Server 2003 return different kinds of status
     * codes. In Server 2003 it returns STATUS_ACCESS_VIOLATION because
     * this function implements more probe checks against the object
     * attributes parameter (namely the path name of the object) so it
     * fails earlier. In Windows 10 instead the function only probes the
     * memory address of the object attributes so that it resides in the boundary
     * of the UM memory range so the function lets this NULL parameter
     * slide through until ObOpenObjectByName hits this parameter as being
     * NULL and returns STATUS_INVALID_PARAMETER. Currently ReactOS follows
     * the behavior of Windows 10.
     */
    Status = NtQueryOpenSubKeys(NULL, &Subkeys);
    ok(Status == STATUS_ACCESS_VIOLATION || Status == STATUS_INVALID_PARAMETER,
       "STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER expected, got 0x%lx\n", Status);

    /* Garbage return variable, this function doesn't check for alignment */
    Status = NtQueryOpenSubKeys(&ObjectAttributes, (PVOID)1);
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* Return the open subkeys of this key now */
    Status = NtQueryOpenSubKeys(&ObjectAttributes, &Subkeys);
    ok_ntstatus(Status, STATUS_SUCCESS);

    trace("\\Registry has %lu opened subkeys\n", Subkeys);

    /* Build a key path to the SYSTEM key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Return the open subkeys of this key now */
    Status = NtQueryOpenSubKeys(&ObjectAttributes, &Subkeys);
    ok_ntstatus(Status, STATUS_SUCCESS);

    trace("\\Registry\\Machine\\SYSTEM has %lu opened subkeys\n", Subkeys);

    /* Build a key path to the SYSTEM key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SoftwareKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Return the open subkeys of this key now */
    Status = NtQueryOpenSubKeys(&ObjectAttributes, &Subkeys);
    ok_ntstatus(Status, STATUS_SUCCESS);

    trace("\\Registry\\Machine\\SOFTWARE has %lu opened subkeys\n", Subkeys);

    /* Build a key path to the default user key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DefaultUserKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Return the open subkeys of this key now */
    Status = NtQueryOpenSubKeys(&ObjectAttributes, &Subkeys);
    ok_ntstatus(Status, STATUS_SUCCESS);

    trace("\\Registry\\User\\.DEFAULT has %lu opened subkeys\n", Subkeys);
}
