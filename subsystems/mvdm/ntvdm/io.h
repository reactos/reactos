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

typedef UCHAR  (WINAPI *EMULATOR_INB_PROC)(USHORT Port);
typedef USHORT (WINAPI *EMULATOR_INW_PROC)(USHORT Port);
typedef ULONG  (WINAPI *EMULATOR_IND_PROC)(USHORT Port);

typedef VOID (WINAPI *EMULATOR_INSB_PROC)(USHORT Port, PUCHAR  Buffer, ULONG Count);
typedef VOID (WINAPI *EMULATOR_INSW_PROC)(USHORT Port, PUSHORT Buffer, ULONG Count);
typedef VOID (WINAPI *EMULATOR_INSD_PROC)(USHORT Port, PULONG  Buffer, ULONG Count);

typedef VOID (WINAPI *EMULATOR_OUTB_PROC)(USHORT Port, UCHAR  Data);
typedef VOID (WINAPI *EMULATOR_OUTW_PROC)(USHORT Port, USHORT Data);
typedef VOID (WINAPI *EMULATOR_OUTD_PROC)(USHORT Port, ULONG  Data);

typedef VOID (WINAPI *EMULATOR_OUTSB_PROC)(USHORT Port, PUCHAR  Buffer, ULONG Count);
typedef VOID (WINAPI *EMULATOR_OUTSW_PROC)(USHORT Port, PUSHORT Buffer, ULONG Count);
typedef VOID (WINAPI *EMULATOR_OUTSD_PROC)(USHORT Port, PULONG  Buffer, ULONG Count);


UCHAR
IOReadB(USHORT Port);
VOID
IOReadStrB(USHORT Port,
           PUCHAR Buffer,
           ULONG  Count);

VOID
IOWriteB(USHORT Port,
         UCHAR  Buffer);
VOID
IOWriteStrB(USHORT Port,
            PUCHAR Buffer,
            ULONG  Count);

USHORT
IOReadW(USHORT Port);
VOID
IOReadStrW(USHORT  Port,
           PUSHORT Buffer,
           ULONG   Count);

VOID
IOWriteW(USHORT Port,
         USHORT Buffer);
VOID
IOWriteStrW(USHORT  Port,
            PUSHORT Buffer,
            ULONG   Count);

ULONG
IOReadD(USHORT Port);
VOID
IOReadStrD(USHORT Port,
           PULONG Buffer,
           ULONG  Count);

VOID
IOWriteD(USHORT Port,
         ULONG  Buffer);
VOID
IOWriteStrD(USHORT Port,
            PULONG Buffer,
            ULONG  Count);


VOID RegisterIoPort(USHORT Port,
                    EMULATOR_INB_PROC  InHandler,
                    EMULATOR_OUTB_PROC OutHandler);

VOID UnregisterIoPort(USHORT Port);

VOID WINAPI EmulatorReadIo
(
    PFAST486_STATE State,
    USHORT Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize
);

VOID WINAPI EmulatorWriteIo
(
    PFAST486_STATE State,
    USHORT Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize
);

#endif // _IO_H_

/* EOF */
