/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ppi.c
 * PURPOSE:         Programmable Peripheral Interface emulation -
 *                  i8255A-5 compatible
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTES: - Most of its functionality as keyboard controller is replaced
 *          by the PS/2 controller.
 *        - This controller is here only for having ports 61h and 62h working.
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "ppi.h"

#include "hardware/pit.h"
#include "hardware/sound/speaker.h"

#include "io.h"

/* PRIVATE VARIABLES **********************************************************/

/*static*/ BYTE Port61hState = 0x00; // Used in emulator.c
static     BYTE Port62hState = 0x00;

/* PRIVATE FUNCTIONS **********************************************************/

static BYTE WINAPI PpiReadPort(USHORT Port)
{
    if (Port == PPI_PORT_61H)
        return Port61hState;
    else if (Port == PPI_PORT_62H)
        return Port62hState;

    return 0x00;
}

static VOID WINAPI Port61hWrite(USHORT Port, BYTE Data)
{
    // BOOLEAN SpeakerStateChange = FALSE;
    BYTE OldPort61hState = Port61hState;

    /* Only the four lowest bytes can be written */
    Port61hState = (Port61hState & 0xF0) | (Data & 0x0F);

    if ((OldPort61hState ^ Port61hState) & 0x01)
    {
        DPRINT("PIT 2 Gate %s\n", Port61hState & 0x01 ? "on" : "off");
        PitSetGate(2, !!(Port61hState & 0x01));
        // SpeakerStateChange = TRUE;
    }

    if ((OldPort61hState ^ Port61hState) & 0x02)
    {
        /* There were some change for the speaker... */
        DPRINT("Speaker %s\n", Port61hState & 0x02 ? "on" : "off");
        // SpeakerStateChange = TRUE;
    }
    // if (SpeakerStateChange) SpeakerChange(Port61hState);
    SpeakerChange(Port61hState);
}

static VOID WINAPI Port62hWrite(USHORT Port, BYTE Data)
{
    Port62hState = Data;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID PpiInitialize(VOID)
{
    /* Register the I/O Ports */
    // Port 0x60 is now used by the PS/2 controller
    RegisterIoPort(PPI_PORT_61H, PpiReadPort, Port61hWrite);
    RegisterIoPort(PPI_PORT_62H, PpiReadPort, Port62hWrite);
    // Port 0x63 is unused
}

/* EOF */
