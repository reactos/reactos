/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            io.c
 * PURPOSE:         I/O Port Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "io.h"

/* PRIVATE VARIABLES **********************************************************/

typedef struct _EMULATOR_IOPORT_HANDLER
{
    EMULATOR_IN_PROC  In;
    EMULATOR_OUT_PROC Out;
} EMULATOR_IOPORT_HANDLER, *PEMULATOR_IOPORT_HANDLER;

/*
 * This is the list of registered I/O Port handlers.
 */
EMULATOR_IOPORT_HANDLER IoPortProc[EMULATOR_MAX_IOPORTS_NUM];

/* PUBLIC FUNCTIONS ***********************************************************/

VOID WINAPI RegisterIoPort(ULONG Port,
                           EMULATOR_IN_PROC  InHandler,
                           EMULATOR_OUT_PROC OutHandler)
{
    if (IoPortProc[Port].In == NULL)
        IoPortProc[Port].In = InHandler;
    else
        DPRINT1("IoPortProc[%d].In already registered\n", Port);

    if (IoPortProc[Port].Out == NULL)
        IoPortProc[Port].Out = OutHandler;
    else
        DPRINT1("IoPortProc[%d].Out already registered\n", Port);
}

VOID WINAPI
EmulatorReadIo(PFAST486_STATE State,
               ULONG Port,
               PVOID Buffer,
               ULONG DataCount,
               UCHAR DataSize)
{
    INT i, j;
    LPBYTE Address = (LPBYTE)Buffer;

    UNREFERENCED_PARAMETER(State);

    for (i = 0; i < DataCount; i++) for (j = 0; j < DataSize; j++)
    {
        ULONG CurrentPort = Port + j;

        /* Call the IN Port handler */
        if (IoPortProc[CurrentPort].In != NULL)
        {
            *(Address++) = IoPortProc[CurrentPort].In(CurrentPort);
        }
        else
        {
            DPRINT1("Read from unknown port: 0x%X\n", CurrentPort);
            *(Address++) = 0xFF;    // Empty port value
        }
    }
}

VOID WINAPI
EmulatorWriteIo(PFAST486_STATE State,
                ULONG Port,
                PVOID Buffer,
                ULONG DataCount,
                UCHAR DataSize)
{
    INT i, j;
    LPBYTE Address = (LPBYTE)Buffer;

    UNREFERENCED_PARAMETER(State);

    for (i = 0; i < DataCount; i++) for (j = 0; j < DataSize; j++)
    {
        ULONG CurrentPort = Port + j;

        /* Call the OUT Port handler */
        if (IoPortProc[CurrentPort].Out != NULL)
            IoPortProc[CurrentPort].Out(CurrentPort, *(Address++));
        else
            DPRINT1("Write to unknown port: 0x%X\n", CurrentPort);
    }
}

/* EOF */
