/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Generic ARC Support Functions
 * COPYRIGHT:   Copyright 2019 Hermes Belusca-Maito
 */

#pragma once

typedef
ARC_STATUS
(__cdecl *ARC_ENTRY_POINT)(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[]);

PCHAR
GetNextArgumentValue(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN OUT PULONG LastIndex OPTIONAL,
    IN PCHAR ArgumentName);

PCHAR
GetArgumentValue(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR ArgumentName);
