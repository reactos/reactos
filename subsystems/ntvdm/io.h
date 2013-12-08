/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            io.c
 * PURPOSE:         I/O Port Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _IO_H_
#define _IO_H_

/* DEFINES ********************************************************************/

#define EMULATOR_MAX_IOPORTS_NUM    0x10000

/* FUNCTIONS ******************************************************************/

typedef BYTE (WINAPI *EMULATOR_IN_PROC)(ULONG Port);
typedef VOID (WINAPI *EMULATOR_OUT_PROC)(ULONG Port, BYTE Data);

VOID WINAPI RegisterIoPort(ULONG Port,
                           EMULATOR_IN_PROC  InHandler,
                           EMULATOR_OUT_PROC OutHandler);

VOID WINAPI UnregisterIoPort(ULONG Port);

VOID WINAPI EmulatorReadIo
(
    PFAST486_STATE State,
    ULONG Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize
);

VOID WINAPI EmulatorWriteIo
(
    PFAST486_STATE State,
    ULONG Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize
);

#endif // _IO_H_

/* EOF */
