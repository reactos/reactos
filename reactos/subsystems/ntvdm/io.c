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
    HANDLE hVdd; // == 0 if unused,
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
IOReadB(ULONG Port)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InB)
    {
        return IoPortProc[Port].IoHandlers.InB(Port);
    }
    else if (IoPortProc[Port].hVdd > 0 &&
             IoPortProc[Port].VddIoHandlers.inb_handler)
    {
        UCHAR Data;
        ASSERT(Port <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.inb_handler((WORD)Port, &Data);
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
IOReadStrB(ULONG  Port,
           PUCHAR Buffer,
           ULONG  Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InsB)
    {
        IoPortProc[Port].IoHandlers.InsB(Port, Buffer, Count);
    }
    else if (IoPortProc[Port].hVdd > 0 &&
             IoPortProc[Port].VddIoHandlers.insb_handler)
    {
        ASSERT(Port  <= MAXWORD);
        ASSERT(Count <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.insb_handler((WORD)Port, Buffer, (WORD)Count);
    }
    else
    {
        while (Count--)
            *Buffer++ = IOReadB(Port);
    }
}

VOID
IOWriteB(ULONG Port,
         UCHAR Buffer)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutB)
    {
        IoPortProc[Port].IoHandlers.OutB(Port, Buffer);
    }
    else if (IoPortProc[Port].hVdd > 0 &&
             IoPortProc[Port].VddIoHandlers.outb_handler)
    {
        ASSERT(Port <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.outb_handler((WORD)Port, Buffer);
    }
    else
    {
        /* Do nothing */
        DPRINT("Write to unknown port: 0x%X\n", Port);
    }
}

VOID
IOWriteStrB(ULONG  Port,
            PUCHAR Buffer,
            ULONG  Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutsB)
    {
        IoPortProc[Port].IoHandlers.OutsB(Port, Buffer, Count);
    }
    else if (IoPortProc[Port].hVdd > 0 &&
             IoPortProc[Port].VddIoHandlers.outsb_handler)
    {
        ASSERT(Port  <= MAXWORD);
        ASSERT(Count <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.outsb_handler((WORD)Port, Buffer, (WORD)Count);
    }
    else
    {
        while (Count--) IOWriteB(Port, *Buffer++);
    }
}

USHORT
IOReadW(ULONG Port)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InW)
    {
        return IoPortProc[Port].IoHandlers.InW(Port);
    }
    else if (IoPortProc[Port].hVdd > 0 &&
             IoPortProc[Port].VddIoHandlers.inw_handler)
    {
        USHORT Data;
        ASSERT(Port <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.inw_handler((WORD)Port, &Data);
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
IOReadStrW(ULONG   Port,
           PUSHORT Buffer,
           ULONG   Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.InsW)
    {
        IoPortProc[Port].IoHandlers.InsW(Port, Buffer, Count);
    }
    else if (IoPortProc[Port].hVdd > 0 &&
             IoPortProc[Port].VddIoHandlers.insw_handler)
    {
        ASSERT(Port  <= MAXWORD);
        ASSERT(Count <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.insw_handler((WORD)Port, Buffer, (WORD)Count);
    }
    else
    {
        while (Count--)
            *Buffer++ = IOReadW(Port);
    }
}

VOID
IOWriteW(ULONG  Port,
         USHORT Buffer)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutW)
    {
        IoPortProc[Port].IoHandlers.OutW(Port, Buffer);
    }
    else if (IoPortProc[Port].hVdd > 0 &&
             IoPortProc[Port].VddIoHandlers.outw_handler)
    {
        ASSERT(Port <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.outw_handler((WORD)Port, Buffer);
    }
    else
    {
        // FIXME: Is it ok on Little endian and Big endian ??
        IOWriteB(Port, LOBYTE(Buffer));
        IOWriteB(Port + sizeof(UCHAR), HIBYTE(Buffer));
    }
}

VOID
IOWriteStrW(ULONG   Port,
            PUSHORT Buffer,
            ULONG   Count)
{
    if (IoPortProc[Port].hVdd == INVALID_HANDLE_VALUE &&
        IoPortProc[Port].IoHandlers.OutsW)
    {
        IoPortProc[Port].IoHandlers.OutsW(Port, Buffer, Count);
    }
    else if (IoPortProc[Port].hVdd > 0 &&
             IoPortProc[Port].VddIoHandlers.outsw_handler)
    {
        ASSERT(Port  <= MAXWORD);
        ASSERT(Count <= MAXWORD);
        IoPortProc[Port].VddIoHandlers.outsw_handler((WORD)Port, Buffer, (WORD)Count);
    }
    else
    {
        while (Count--) IOWriteW(Port, *Buffer++);
    }
}

ULONG
IOReadD(ULONG Port)
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
IOReadStrD(ULONG  Port,
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
        while (Count--)
            *Buffer++ = IOReadD(Port);
    }
}

VOID
IOWriteD(ULONG Port,
         ULONG Buffer)
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
IOWriteStrD(ULONG  Port,
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


VOID RegisterIoPort(ULONG Port,
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

VOID UnregisterIoPort(ULONG Port)
{
    /*
     * Put automagically all the fields to zero:
     * the hVdd gets unregistered as well as all the handlers.
     */
    // IoPortProc[Port] = {NULL};
    RtlZeroMemory(&IoPortProc[Port], sizeof(IoPortProc[Port]));
}

VOID WINAPI
EmulatorReadIo(PFAST486_STATE State,
               ULONG Port,
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
        PBYTE Address = (PBYTE)Buffer;

        while (DataCount--)
        {
            ULONG CurrentPort = Port;
            ULONG Count;
            UCHAR NewDataSize = DataSize;

            /* Read dword */
            Count       = NewDataSize / sizeof(ULONG);
            NewDataSize = NewDataSize % sizeof(ULONG);
            while (Count--)
            {
                *(PULONG)Address = IOReadD(CurrentPort);
                CurrentPort += sizeof(ULONG);
                Address     += sizeof(ULONG);
            }

            /* Read word */
            Count       = NewDataSize / sizeof(USHORT);
            NewDataSize = NewDataSize % sizeof(USHORT);
            while (Count--)
            {
                *(PUSHORT)Address = IOReadW(CurrentPort);
                CurrentPort += sizeof(USHORT);
                Address     += sizeof(USHORT);
            }

            /* Read byte */
            Count       = NewDataSize / sizeof(UCHAR);
            NewDataSize = NewDataSize % sizeof(UCHAR);
            while (Count--)
            {
                *(PUCHAR)Address = IOReadB(CurrentPort);
                CurrentPort += sizeof(UCHAR);
                Address     += sizeof(UCHAR);
            }

            ASSERT(Count == 0);
            ASSERT(NewDataSize == 0);
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
        PBYTE Address = (PBYTE)Buffer;

        while (DataCount--)
        {
            ULONG CurrentPort = Port;
            ULONG Count;
            UCHAR NewDataSize = DataSize;

            /* Write dword */
            Count       = NewDataSize / sizeof(ULONG);
            NewDataSize = NewDataSize % sizeof(ULONG);
            while (Count--)
            {
                IOWriteD(CurrentPort, *(PULONG)Address);
                CurrentPort += sizeof(ULONG);
                Address     += sizeof(ULONG);
            }

            /* Write word */
            Count       = NewDataSize / sizeof(USHORT);
            NewDataSize = NewDataSize % sizeof(USHORT);
            while (Count--)
            {
                IOWriteW(CurrentPort, *(PUSHORT)Address);
                CurrentPort += sizeof(USHORT);
                Address     += sizeof(USHORT);
            }

            /* Write byte */
            Count       = NewDataSize / sizeof(UCHAR);
            NewDataSize = NewDataSize % sizeof(UCHAR);
            while (Count--)
            {
                IOWriteB(CurrentPort, *(PUCHAR)Address);
                CurrentPort += sizeof(UCHAR);
                Address     += sizeof(UCHAR);
            }

            ASSERT(Count == 0);
            ASSERT(NewDataSize == 0);
        }
    }
}



BOOL
WINAPI
VDDInstallIOHook(HANDLE            hVdd,
                 WORD              cPortRange,
                 PVDD_IO_PORTRANGE pPortRange,
                 PVDD_IO_HANDLERS  IOhandler)
{
    /* Check possible validity of the VDD handle */
    if (hVdd == 0 || hVdd == INVALID_HANDLE_VALUE) return FALSE;

    /* Loop for each range of I/O ports */
    while (cPortRange--)
    {
        WORD i;

        /* Register the range of I/O ports */
        for (i = pPortRange->First; i <= pPortRange->Last; ++i)
        {
            /*
             * Don't do anything if the I/O port is already
             * handled internally or externally.
             */
            if (IoPortProc[i].hVdd != 0)
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
            IoPortProc[i].VddIoHandlers = *IOhandler;
        }

        /* Go to the next range */
        ++pPortRange;
        ++IOhandler;
    }

    return TRUE;
}

VOID
WINAPI
VDDDeInstallIOHook(HANDLE            hVdd,
                   WORD              cPortRange,
                   PVDD_IO_PORTRANGE pPortRange)
{
    /* Check possible validity of the VDD handle */
    if (hVdd == 0 || hVdd == INVALID_HANDLE_VALUE) return;

    /* Loop for each range of I/O ports */
    while (cPortRange--)
    {
        WORD i;

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
