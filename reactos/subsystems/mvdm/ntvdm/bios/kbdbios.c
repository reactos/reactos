/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            kbdbios.c
 * PURPOSE:         VDM Keyboard BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/bop.h"

#include "bios.h"
// #include "kbdbios.h"

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_KBD_IRQ     0x09
#define BOP_KBD_INT     0x16

/* PUBLIC FUNCTIONS ***********************************************************/

static VOID WINAPI KbdBiosIRQ(LPWORD Stack)
{
    DPRINT1("KbdBiosIRQ is UNIMPLEMENTED\n");
}

static VOID WINAPI KbdBiosINT(LPWORD Stack)
{
    DPRINT1("KbdBiosINT is UNIMPLEMENTED\n");
}

BOOLEAN KbdBiosInitialize(VOID)
{
    /* Register the BIOS support BOPs */
    RegisterBop(BOP_KBD_IRQ, KbdBiosIRQ);
    RegisterBop(BOP_KBD_INT, KbdBiosINT);

    return TRUE;
}

VOID KbdBiosCleanup(VOID)
{
}

/* EOF */
