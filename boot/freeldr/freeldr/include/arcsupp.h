/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Generic ARC Support Functions
 * COPYRIGHT:   Copyright 2019-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

typedef
ARC_STATUS
(__cdecl *ARC_ENTRY_POINT)(
    _In_ ULONG Argc,
    _In_ PCHAR Argv[],
    _In_ PCHAR Envp[]);

PSTR
GetNextArgumentValue(
    _In_ ULONG Argc,
    _In_ PCHAR Argv[],
    _Inout_opt_ PULONG LastIndex,
    _In_ PCSTR ArgumentName);

PSTR
GetArgumentValue(
    _In_ ULONG Argc,
    _In_ PCHAR Argv[],
    _In_ PCSTR ArgumentName);

/* EOF */
