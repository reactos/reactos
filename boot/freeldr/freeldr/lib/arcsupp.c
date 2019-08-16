/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Generic ARC Support Functions
 * COPYRIGHT:   Copyright 2019 Hermes Belusca-Maito
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

/* FUNCTIONS *****************************************************************/

PCHAR
GetNextArgumentValue(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN OUT PULONG LastIndex OPTIONAL,
    IN PCHAR ArgumentName)
{
    ULONG i;
    SIZE_T ArgNameLen = strlen(ArgumentName);

    for (i = (LastIndex ? *LastIndex : 0); i < Argc; ++i)
    {
        if (strlen(Argv[i]) >= ArgNameLen + 1 /* Count the '=' sign */ &&
            _strnicmp(Argv[i], ArgumentName, ArgNameLen) == 0 &&
            Argv[i][ArgNameLen] == '=')
        {
            /* Found it, return the value */
            if (LastIndex) *LastIndex = i;
            return &Argv[i][ArgNameLen + 1];
        }
    }

    if (LastIndex) *LastIndex = (ULONG)-1;
    return NULL;
}

PCHAR
GetArgumentValue(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR ArgumentName)
{
    return GetNextArgumentValue(Argc, Argv, NULL, ArgumentName);
}
