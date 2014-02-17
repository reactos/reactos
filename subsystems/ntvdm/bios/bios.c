/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "bios.h"

/* PRIVATE VARIABLES **********************************************************/

static BOOLEAN Bios32Loaded = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN BiosInitialize(IN LPCWSTR BiosFileName,
                       IN HANDLE  ConsoleInput,
                       IN HANDLE  ConsoleOutput)
{
    Bios32Loaded = Bios32Initialize(ConsoleInput, ConsoleOutput);
    return Bios32Loaded;
}

VOID BiosCleanup(VOID)
{
    if (Bios32Loaded) Bios32Cleanup();
}

/* EOF */
