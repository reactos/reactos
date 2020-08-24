/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
SmpCheckForCrashDump(IN PUNICODE_STRING FileName)
{
    return FALSE;
}
