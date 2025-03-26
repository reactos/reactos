/*
 * PROJECT:     ReactOS SM Helper Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Utility functions built around the client SM API
 * COPYRIGHT:   Copyright 2005 Emanuele Aliberti <ea@reactos.com>
 *              Copyright 2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifndef _SM_HELPER_H_
#define _SM_HELPER_H_

#include "smrosdbg.h"

NTSTATUS
NTAPI
SmExecuteProgram(
    _In_ HANDLE SmApiPort,
    _In_ PUNICODE_STRING Pgm /*,
    _Out_opt_ PRTL_USER_PROCESS_INFORMATION ProcessInformation*/);

NTSTATUS
NTAPI
SmLookupSubsystem(
    _In_ PWSTR Name,
    _Out_ PWSTR Data,
    _Inout_ PULONG DataLength,
    _Out_opt_ PULONG DataType,
    _In_opt_ PVOID Environment);

NTSTATUS
NTAPI
SmQueryInformation(
    _In_ HANDLE SmApiPort,
    _In_ SM_INFORMATION_CLASS SmInformationClass,
    _Inout_ PVOID Data,
    _In_ ULONG DataLength,
    _Inout_opt_ PULONG ReturnedDataLength);

#endif // _SM_HELPER_H_
