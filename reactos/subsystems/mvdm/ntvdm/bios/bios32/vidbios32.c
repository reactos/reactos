/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/bios32/vidbios32.c
 * PURPOSE:         VDM 32-bit Video BIOS
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE:            All of the real code is in bios/vidbios.c
 */

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_VIDEO_INT   0x10

/* PUBLIC FUNCTIONS ***********************************************************/

extern VOID WINAPI VidBiosVideoService(LPWORD Stack);
static VOID WINAPI VidBiosINT(LPWORD Stack)
{
    /*
     * Set up a false stack to hardwire the BOP function (that can directly
     * manipulate CPU registers) to the 32-bit interrupt function (which uses
     * the stack to be able to modify the original CS:IP and FLAGS).
     *
     * See int32.h stack codes.
     */
    WORD EmuStack[4];
    DWORD Flags = getEFLAGS();

    DPRINT1("Calling BOP VidBiosINT\n");

    EmuStack[STACK_FLAGS]   = LOWORD(Flags);
    EmuStack[STACK_CS]      = getCS();
    EmuStack[STACK_IP]      = getIP();
    EmuStack[STACK_INT_NUM] = BOP_VIDEO_INT;

    VidBiosVideoService(EmuStack);

    setIP(EmuStack[STACK_IP]);
    setCS(EmuStack[STACK_CS]);
    setEFLAGS(MAKELONG(EmuStack[STACK_FLAGS], HIWORD(Flags)));
}

BOOLEAN VidBios32Initialize(VOID)
{
    /* Register the BIOS support BOPs */
    RegisterBop(BOP_VIDEO_INT, VidBiosINT);
    return TRUE;
}

VOID VidBios32Cleanup(VOID)
{
    /* Unregister the BIOS support BOPs */
    RegisterBop(BOP_VIDEO_INT, NULL);
}

/* EOF */
