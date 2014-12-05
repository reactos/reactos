/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            vidbios32.c
 * PURPOSE:         VDM Video 32-bit BIOS
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE:            All of the real code is in bios/vidbios.c
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "int32.h"

#include "vidbios32.h"
#include <bios/vidbios.h>
#include "bios32p.h"

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN VidBios32Initialize(VOID)
{
    /* Initialize the common Video BIOS Support Library */
    if (!VidBiosInitialize()) return FALSE;

    /* Register the BIOS 32-bit Interrupts */
    RegisterBiosInt32(BIOS_VIDEO_INTERRUPT, VidBiosVideoService);

    return TRUE;
}

VOID VidBios32Cleanup(VOID)
{
    /* Cleanup the common Video BIOS Support Library */
    VidBiosCleanup();
}

/* EOF */
