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

typedef UCHAR  (WINAPI *EMULATOR_INB_PROC)(ULONG Port);
typedef USHORT (WINAPI *EMULATOR_INW_PROC)(ULONG Port);
typedef ULONG  (WINAPI *EMULATOR_IND_PROC)(ULONG Port);

typedef VOID (WINAPI *EMULATOR_INSB_PROC)(ULONG Port, PUCHAR  Buffer, ULONG Count);
typedef VOID (WINAPI *EMULATOR_INSW_PROC)(ULONG Port, PUSHORT Buffer, ULONG Count);
typedef VOID (WINAPI *EMULATOR_INSD_PROC)(ULONG Port, PULONG  Buffer, ULONG Count);

typedef VOID (WINAPI *EMULATOR_OUTB_PROC)(ULONG Port, UCHAR  Data);
typedef VOID (WINAPI *EMULATOR_OUTW_PROC)(ULONG Port, USHORT Data);
typedef VOID (WINAPI *EMULATOR_OUTD_PROC)(ULONG Port, ULONG  Data);

typedef VOID (WINAPI *EMULATOR_OUTSB_PROC)(ULONG Port, PUCHAR  Buffer, ULONG Count);
typedef VOID (WINAPI *EMULATOR_OUTSW_PROC)(ULONG Port, PUSHORT Buffer, ULONG Count);
typedef VOID (WINAPI *EMULATOR_OUTSD_PROC)(ULONG Port, PULONG  Buffer, ULONG Count);


UCHAR
IOReadB(ULONG Port);
VOID
IOReadStrB(ULONG  Port,
           PUCHAR Buffer,
           ULONG  Count);

VOID
IOWriteB(ULONG Port,
         UCHAR Buffer);
VOID
IOWriteStrB(ULONG  Port,
            PUCHAR Buffer,
            ULONG  Count);

USHORT
IOReadW(ULONG Port);
VOID
IOReadStrW(ULONG   Port,
           PUSHORT Buffer,
           ULONG   Count);

VOID
IOWriteW(ULONG  Port,
         USHORT Buffer);
VOID
IOWriteStrW(ULONG   Port,
            PUSHORT Buffer,
            ULONG   Count);

ULONG
IOReadD(ULONG Port);
VOID
IOReadStrD(ULONG  Port,
           PULONG Buffer,
           ULONG  Count);

VOID
IOWriteD(ULONG Port,
         ULONG Buffer);
VOID
IOWriteStrD(ULONG  Port,
            PULONG Buffer,
            ULONG  Count);


VOID RegisterIoPort(ULONG Port,
                    EMULATOR_INB_PROC  InHandler,
                    EMULATOR_OUTB_PROC OutHandler);

VOID UnregisterIoPort(ULONG Port);

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
