/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/kbdbios.c
 * PURPOSE:         VDM 32-bit PS/2 Keyboard BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "cpu/bop.h"
#include "int32.h"

#include "bios.h"
// #include "kbdbios.h"

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_KBD_IRQ     0x09
#define BOP_KBD_INT     0x16

/* PUBLIC FUNCTIONS ***********************************************************/

extern VOID WINAPI BiosKeyboardIrq(LPWORD Stack);
static VOID WINAPI KbdBiosIRQ(LPWORD Stack)
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

    DPRINT1("Calling BOP KbdBiosIRQ\n");

    EmuStack[STACK_FLAGS]   = LOWORD(Flags);
    EmuStack[STACK_CS]      = getCS();
    EmuStack[STACK_IP]      = getIP();
    EmuStack[STACK_INT_NUM] = BOP_KBD_IRQ;

    BiosKeyboardIrq(EmuStack);

    setIP(EmuStack[STACK_IP]);
    setCS(EmuStack[STACK_CS]);
    setEFLAGS(MAKELONG(EmuStack[STACK_FLAGS], HIWORD(Flags)));
}

extern VOID WINAPI BiosKeyboardService(LPWORD Stack);
static VOID WINAPI KbdBiosINT(LPWORD Stack)
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

    DPRINT1("Calling BOP KbdBiosINT\n");

    EmuStack[STACK_FLAGS]   = LOWORD(Flags);
    EmuStack[STACK_CS]      = getCS();
    EmuStack[STACK_IP]      = getIP();
    EmuStack[STACK_INT_NUM] = BOP_KBD_IRQ;

    BiosKeyboardService(EmuStack);

    setIP(EmuStack[STACK_IP]);
    setCS(EmuStack[STACK_CS]);
    setEFLAGS(MAKELONG(EmuStack[STACK_FLAGS], HIWORD(Flags)));
}

BOOLEAN KbdBiosInitialize(VOID)
{
    /* Register the BIOS support BOPs */
    RegisterBop(BOP_KBD_IRQ, KbdBiosIRQ);   // BiosKeyboardIrq in kbdbios32.c
    RegisterBop(BOP_KBD_INT, KbdBiosINT);   // BiosKeyboardService in kbdbios32.c
    return TRUE;
}

VOID KbdBiosCleanup(VOID)
{
    /* Unregister the BIOS support BOPs */
    RegisterBop(BOP_KBD_IRQ, NULL);
    RegisterBop(BOP_KBD_INT, NULL);
}

/* EOF */
