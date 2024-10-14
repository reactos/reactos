/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Generic ARC Support Functions
 * COPYRIGHT:   Copyright 2019-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

/* FUNCTIONS *****************************************************************/

PSTR
GetNextArgumentValue(
    _In_ ULONG Argc,
    _In_ PCHAR Argv[],
    _Inout_opt_ PULONG LastIndex,
    _In_ PCSTR ArgumentName)
{
    ULONG i;
    SIZE_T ArgNameLen = strlen(ArgumentName);

    for (i = (LastIndex ? *LastIndex : 0); i < Argc; ++i)
    {
        if (Argv[i] /* NULL pointer is a valid entry in Argv: skip it */ &&
            (strlen(Argv[i]) >= ArgNameLen + 1 /* Count the '=' sign */) &&
            (_strnicmp(Argv[i], ArgumentName, ArgNameLen) == 0) &&
            (Argv[i][ArgNameLen] == '='))
        {
            /* Found it, return the value */
            if (LastIndex) *LastIndex = i;
            return &Argv[i][ArgNameLen + 1];
        }
    }

    if (LastIndex) *LastIndex = (ULONG)-1;
    return NULL;
}

PSTR
GetArgumentValue(
    _In_ ULONG Argc,
    _In_ PCHAR Argv[],
    _In_ PCSTR ArgumentName)
{
    return GetNextArgumentValue(Argc, Argv, NULL, ArgumentName);
}

/* EOF */
