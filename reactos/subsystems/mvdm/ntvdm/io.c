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

#include "ntvdm.h"
#include "emulator.h"
#include "io.h"

/* PRIVATE VARIABLES **********************************************************/

typedef struct _EMULATOR_IO_HANDLERS
{
    EMULATOR_INB_PROC   InB;
    EMULATOR_INW_PROC   InW;
    EMULATOR_IND_PROC   InD;

    EMULATOR_INSB_PROC  InsB;
    EMULATOR_INSW_PROC  InsW;
    EMULATOR_INSD_PROC  InsD;

    EMULATOR_OUTB_PROC  OutB;
    EMULATOR_OUTW_PROC  OutW;
    EMULATOR_OUTD_PROC  OutD;

    EMULATOR_OUTSB_PROC OutsB;
    EMULATOR_OUTSW_PROC OutsW;
    EMULATOR_OUTSD_PROC OutsD;
} EMULATOR_IO_HANDLERS, *PEMULATOR_IO_HANDLERS;

typedef struct _EMULATOR_IOPORT_HANDLERS
{
    HANDLE hVdd; // == NULL if unused,
                 //    INVALID_HANDLE_VALUE if handled internally,
                 //    a valid VDD handle   if handled externally.
    union
    {
        /* For Windows compatibility only, not used internally... */
        VDD_IO_HANDLERS VddIoHandlers;

        /* ... we use these members internally */
        EMULATOR_IO_HANDLERS IoHandlers;
    };
} EMULATOR_IOPORT_HANDLERS, *PEMULATOR_IOPORT_HANDLERS;

/*
 * This is the list of registered I/O Port handlers.
 */
EMULATOR_IOPORT_HANDLERS IoPortProc[EMULATOR_MAX_IOPORTS_NUM] = {{NULL}};

/* PUBLIC FUNCTIONS ***********************************************************/

UCHAR
IOReadB(USHORT Port)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InB)
    {
        return IoPortProc[Port].IoHandlers.InB(Port);
    }
    else if (IoPortProc[Port].hVdd != NULL && IoPortProc[Port].hVdd != INVALID_HANDLE_VALUE &&
             IoPortProc[Port].VddIoHandlers.inb_handler)
    {
        UCHAR Data;
        ASSERT(Port <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.inb_handler(Port, &Data);
        return Data;
    }
    else
    {
        /* Return an empty port byte value */
        DPRINT("Read from unknown port: 0x%X\n", Port);
        return 0xFF;
    }
}

VOID
IOReadStrB(USHORT Port,
           PUCHAR Buffer,
           ULONG  Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InsB)
    {
        IoPortProc[Port].IoHandlers.InsB(Port, Buffer, Count);
    }
    else if (IoPortProc[Port].hVdd != NULL && IoPortProc[Port].hVdd != INVALID_HANDLE_VALUE &&
             IoPortProc[Port].VddIoHandlers.insb_handler)
    {
        ASSERT(Port  <= MAXWORD);
        ASSERT(Count <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.insb_handler(Port, Buffer, (WORD)Count);
    }
    else
    {
        while (Count--) *Buffer++ = IOReadB(Port);
    }
}

VOID
IOWriteB(USHORT Port,
         UCHAR  Buffer)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutB)
    {
        IoPortProc[Port].IoHandlers.OutB(Port, Buffer);
    }
    else if (IoPortProc[Port].hVdd != NULL && IoPortProc[Port].hVdd != INVALID_HANDLE_VALUE &&
             IoPortProc[Port].VddIoHandlers.outb_handler)
    {
        ASSERT(Port <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.outb_handler(Port, Buffer);
    }
    else
    {
        /* Do nothing */
        DPRINT("Write to unknown port: 0x%X\n", Port);
    }
}

VOID
IOWriteStrB(USHORT Port,
            PUCHAR Buffer,
            ULONG  Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutsB)
    {
        IoPortProc[Port].IoHandlers.OutsB(Port, Buffer, Count);
    }
    else if (IoPortProc[Port].hVdd != NULL && IoPortProc[Port].hVdd != INVALID_HANDLE_VALUE &&
             IoPortProc[Port].VddIoHandlers.outsb_handler)
    {
        ASSERT(Port  <= MAXWORD);
        ASSERT(Count <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.outsb_handler(Port, Buffer, (WORD)Count);
    }
    else
    {
        while (Count--) IOWriteB(Port, *Buffer++);
    }
}

USHORT
IOReadW(USHORT Port)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InW)
    {
        return IoPortProc[Port].IoHandlers.InW(Port);
    }
    else if (IoPortProc[Port].hVdd != NULL && IoPortProc[Port].hVdd != INVALID_HANDLE_VALUE &&
             IoPortProc[Port].VddIoHandlers.inw_handler)
    {
        USHORT Data;
        ASSERT(Port <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.inw_handler(Port, &Data);
        return Data;
    }
    else
    {
        UCHAR Low, High;

        // FIXME: Is it ok on Little endian and Big endian ??
        Low  = IOReadB(Port);
        High = IOReadB(Port + sizeof(UCHAR));
        return MAKEWORD(Low, High);
    }
}

VOID
IOReadStrW(USHORT  Port,
           PUSHORT Buffer,
           ULONG   Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InsW)
    {
        IoPortProc[Port].IoHandlers.InsW(Port, Buffer, Count);
    }
    else if (IoPortProc[Port].hVdd != NULL && IoPortProc[Port].hVdd != INVALID_HANDLE_VALUE &&
             IoPortProc[Port].VddIoHandlers.insw_handler)
    {
        ASSERT(Port  <= MAXWORD);
        ASSERT(Count <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.insw_handler(Port, Buffer, (WORD)Count);
    }
    else
    {
        while (Count--) *Buffer++ = IOReadW(Port);
    }
}

VOID
IOWriteW(USHORT Port,
         USHORT Buffer)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutW)
    {
        IoPortProc[Port].IoHandlers.OutW(Port, Buffer);
    }
    else if (IoPortProc[Port].hVdd != NULL && IoPortProc[Port].hVdd != INVALID_HANDLE_VALUE &&
             IoPortProc[Port].VddIoHandlers.outw_handler)
    {
        ASSERT(Port <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.outw_handler(Port, Buffer);
    }
    else
    {
        // FIXME: Is it ok on Little endian and Big endian ??
        IOWriteB(Port, LOBYTE(Buffer));
        IOWriteB(Port + sizeof(UCHAR), HIBYTE(Buffer));
    }
}

VOID
IOWriteStrW(USHORT  Port,
            PUSHORT Buffer,
            ULONG   Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutsW)
    {
        IoPortProc[Port].IoHandlers.OutsW(Port, Buffer, Count);
    }
    else if (IoPortProc[Port].hVdd != NULL && IoPortProc[Port].hVdd != INVALID_HANDLE_VALUE &&
             IoPortProc[Port].VddIoHandlers.outsw_handler)
    {
        ASSERT(Port  <= MAXWORD);
        ASSERT(Count <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.outsw_handler(Port, Buffer, (WORD)Count);
    }
    else
    {
        while (Count--) IOWriteW(Port, *Buffer++);
    }
}

ULONG
IOReadD(USHORT Port)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InD)
    {
        return IoPortProc[Port].IoHandlers.InD(Port);
    }
    else
    {
        USHORT Low, High;

        // FIXME: Is it ok on Little endian and Big endian ??
        Low  = IOReadW(Port);
        High = IOReadW(Port + sizeof(USHORT));
        return MAKELONG(Low, High);
    }
}

VOID
IOReadStrD(USHORT Port,
           PULONG Buffer,
           ULONG  Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InsD)
    {
        IoPortProc[Port].IoHandlers.InsD(Port, Buffer, Count);
    }
    else
    {
        while (Count--) *Buffer++ = IOReadD(Port);
    }
}

VOID
IOWriteD(USHORT Port,
         ULONG  Buffer)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutD)
    {
        IoPortProc[Port].IoHandlers.OutD(Port, Buffer);
    }
    else
    {
        // FIXME: Is it ok on Little endian and Big endian ??
        IOWriteW(Port, LOWORD(Buffer));
        IOWriteW(Port + sizeof(USHORT), HIWORD(Buffer));
    }
}

VOID
IOWriteStrD(USHORT Port,
            PULONG Buffer,
            ULONG  Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutsD)
    {
        IoPortProc[Port].IoHandlers.OutsD(Port, Buffer, Count);
    }
    else
    {
        while (Count--) IOWriteD(Port, *Buffer++);
    }
}


VOID RegisterIoPort(USHORT Port,
                    EMULATOR_INB_PROC  InHandler,
                    EMULATOR_OUTB_PROC OutHandler)
{
    if (IoPortProc[Port].IoHandlers.InB == NULL)
        IoPortProc[Port].IoHandlers.InB = InHandler;
    else
        DPRINT1("IoPortProc[0x%X].IoHandlers.InB already registered\n", Port);

    if (IoPortProc[Port].IoHandlers.OutB == NULL)
        IoPortProc[Port].IoHandlers.OutB = OutHandler;
    else
        DPRINT1("IoPortProc[0x%X].IoHandlers.OutB already registered\n", Port);

    /* We hold the I/O port internally */
    IoPortProc[Port].hVdd = INVALID_HANDLE_VALUE;
}

VOID UnregisterIoPort(USHORT Port)
{
    /*
     * Put automagically all the fields to zero:
     * the hVdd gets unregistered as well as all the handlers.
     */
    // IoPortProc[Port] = {NULL};
    RtlZeroMemory(&IoPortProc[Port], sizeof(IoPortProc[Port]));
}

VOID FASTCALL
EmulatorReadIo(PFAST486_STATE State,
               USHORT Port,
               PVOID Buffer,
               ULONG DataCount,
               UCHAR DataSize)
{
    UNREFERENCED_PARAMETER(State);

    if (DataSize == 0 || DataCount == 0) return;

    if (DataSize == sizeof(UCHAR))
    {
        if (DataCount == 1)
            *(PUCHAR)Buffer = IOReadB(Port);
        else
            IOReadStrB(Port, Buffer, DataCount);
    }
    else if (DataSize == sizeof(USHORT))
    {
        if (DataCount == 1)
            *(PUSHORT)Buffer = IOReadW(Port);
        else
            IOReadStrW(Port, Buffer, DataCount);
    }
    else if (DataSize == sizeof(ULONG))
    {
        if (DataCount == 1)
            *(PULONG)Buffer = IOReadD(Port);
        else
            IOReadStrD(Port, Buffer, DataCount);
    }
    else
    {
        PUCHAR Address = (PUCHAR)Buffer;

        while (DataCount--)
        {
            ULONG CurrentPort = Port;
            ULONG Count;
            UCHAR NewDataSize = DataSize;

            /* Read dword */
            Count       = NewDataSize >> 2; // NewDataSize / sizeof(ULONG);
            NewDataSize = NewDataSize  & 3; // NewDataSize % sizeof(ULONG);
            while (Count--)
            {
                *(PULONG)Address = IOReadD(CurrentPort);
                CurrentPort += sizeof(ULONG);
                Address     += sizeof(ULONG);
            }

            /* Read word */
            Count       = NewDataSize >> 1; // NewDataSize / sizeof(USHORT);
            NewDataSize = NewDataSize  & 1; // NewDataSize % sizeof(USHORT);
            while (Count--)
            {
                *(PUSHORT)Address = IOReadW(CurrentPort);
                CurrentPort += sizeof(USHORT);
                Address     += sizeof(USHORT);
            }

            /* Read byte */
            Count       = NewDataSize; // NewDataSize / sizeof(UCHAR);
            // NewDataSize = NewDataSize % sizeof(UCHAR);
            while (Count--)
            {
                *(PUCHAR)Address = IOReadB(CurrentPort);
                CurrentPort += sizeof(UCHAR);
                Address     += sizeof(UCHAR);
            }
        }
    }
}

VOID FASTCALL
EmulatorWriteIo(PFAST486_STATE State,
                USHORT Port,
                PVOID Buffer,
                ULONG DataCount,
                UCHAR DataSize)
{
    UNREFERENCED_PARAMETER(State);

    if (DataSize == 0 || DataCount == 0) return;

    if (DataSize == sizeof(UCHAR))
    {
        if (DataCount == 1)
            IOWriteB(Port, *(PUCHAR)Buffer);
        else
            IOWriteStrB(Port, Buffer, DataCount);
    }
    else if (DataSize == sizeof(USHORT))
    {
        if (DataCount == 1)
            IOWriteW(Port, *(PUSHORT)Buffer);
        else
            IOWriteStrW(Port, Buffer, DataCount);
    }
    else if (DataSize == sizeof(ULONG))
    {
        if (DataCount == 1)
            IOWriteD(Port, *(PULONG)Buffer);
        else
            IOWriteStrD(Port, Buffer, DataCount);
    }
    else
    {
        PUCHAR Address = (PUCHAR)Buffer;

        while (DataCount--)
        {
            ULONG CurrentPort = Port;
            ULONG Count;
            UCHAR NewDataSize = DataSize;

            /* Write dword */
            Count       = NewDataSize >> 2; // NewDataSize / sizeof(ULONG);
            NewDataSize = NewDataSize  & 3; // NewDataSize % sizeof(ULONG);
            while (Count--)
            {
                IOWriteD(CurrentPort, *(PULONG)Address);
                CurrentPort += sizeof(ULONG);
                Address     += sizeof(ULONG);
            }

            /* Write word */
            Count       = NewDataSize >> 1; // NewDataSize / sizeof(USHORT);
            NewDataSize = NewDataSize  & 1; // NewDataSize % sizeof(USHORT);
            while (Count--)
            {
                IOWriteW(CurrentPort, *(PUSHORT)Address);
                CurrentPort += sizeof(USHORT);
                Address     += sizeof(USHORT);
            }

            /* Write byte */
            Count       = NewDataSize; // NewDataSize / sizeof(UCHAR);
            // NewDataSize = NewDataSize % sizeof(UCHAR);
            while (Count--)
            {
                IOWriteB(CurrentPort, *(PUCHAR)Address);
                CurrentPort += sizeof(UCHAR);
                Address     += sizeof(UCHAR);
            }
        }
    }
}



BOOL
WINAPI
VDDInstallIOHook(IN HANDLE            hVdd,
                 IN WORD              cPortRange,
                 IN PVDD_IO_PORTRANGE pPortRange,
                 IN PVDD_IO_HANDLERS  IoHandlers)
{
    WORD i;

    /* Check validity of the VDD handle */
    if (hVdd == NULL || hVdd == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Loop for each range of I/O ports */
    while (cPortRange--)
    {
        /* Register the range of I/O ports */
        for (i = pPortRange->First; i <= pPortRange->Last; ++i)
        {
            /*
             * Don't do anything if the I/O port is already
             * handled internally or externally.
             */
            if (IoPortProc[i].hVdd != NULL)
            {
                DPRINT1("IoPortProc[0x%X] already registered\n", i);
                continue;
            }

            /* Register wrt. the VDD */
            IoPortProc[i].hVdd = hVdd;

            /* Disable the internal handlers */
            IoPortProc[i].IoHandlers.InB = NULL;
            IoPortProc[i].IoHandlers.InW = NULL;
            IoPortProc[i].IoHandlers.InD = NULL;

            IoPortProc[i].IoHandlers.InsB = NULL;
            IoPortProc[i].IoHandlers.InsW = NULL;
            IoPortProc[i].IoHandlers.InsD = NULL;

            IoPortProc[i].IoHandlers.OutB = NULL;
            IoPortProc[i].IoHandlers.OutW = NULL;
            IoPortProc[i].IoHandlers.OutD = NULL;

            IoPortProc[i].IoHandlers.OutsB = NULL;
            IoPortProc[i].IoHandlers.OutsW = NULL;
            IoPortProc[i].IoHandlers.OutsD = NULL;

            /* Save our handlers */
            IoPortProc[i].VddIoHandlers = *IoHandlers;
        }

        /* Go to the next range */
        ++pPortRange;
    }

    return TRUE;
}

VOID
WINAPI
VDDDeInstallIOHook(IN HANDLE            hVdd,
                   IN WORD              cPortRange,
                   IN PVDD_IO_PORTRANGE pPortRange)
{
    WORD i;

    /* Check validity of the VDD handle */
    if (hVdd == NULL || hVdd == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    /* Loop for each range of I/O ports */
    while (cPortRange--)
    {
        /* Unregister the range of I/O ports */
        for (i = pPortRange->First; i <= pPortRange->Last; ++i)
        {
            /*
             * Don't do anything if we don't own the I/O port.
             */
            if (IoPortProc[i].hVdd != hVdd)
            {
                DPRINT1("IoPortProc[0x%X] owned by somebody else\n", i);
                continue;
            }

            /*
             * Put automagically all the fields to zero:
             * the hVdd gets unregistered as well as all the handlers.
             */
            // IoPortProc[i] = {NULL};
            RtlZeroMemory(&IoPortProc[i], sizeof(IoPortProc[i]));
        }

        /* Go to the next range */
        ++pPortRange;
    }
}

/* EOF */
