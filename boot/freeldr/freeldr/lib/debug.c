/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include <debug.h>

#if DBG

// #define DEBUG_ALL
// #define DEBUG_WARN
// #define DEBUG_ERR
// #define DEBUG_INIFILE
// #define DEBUG_REACTOS
// #define DEBUG_CUSTOM
#define DEBUG_NONE

#define DBG_DEFAULT_LEVELS (ERR_LEVEL|FIXME_LEVEL)

static UCHAR DbgChannels[DBG_CHANNELS_COUNT];

#define SCREEN  1
#define RS232   2
#define BOCHS   4

#define BOCHS_OUTPUT_PORT   0xE9

ULONG DebugPort = RS232;

/* Serial debug connection */
#if defined(SARCH_PC98)
ULONG BaudRate = 9600;
#else
ULONG BaudRate = 115200;
#endif

ULONG ComPort  = 0; // The COM port initializer chooses the first available port starting from COM4 down to COM1.
ULONG PortIrq  = 0; // Not used at the moment.

BOOLEAN DebugStartOfLine = TRUE;

#ifdef UEFIBOOT
VOID
ARMWriteToUART(UCHAR Data);
#endif

VOID
DebugInit(
    _In_ PCSTR DebugString)
{
    static BOOLEAN Initialized = FALSE;
    PSTR CommandLine, PortString, BaudString, IrqString;
    ULONG Value;
    CHAR DbgStringBuffer[256];

    /* Always reset the debugging channels */

#if defined (DEBUG_ALL)
    memset(DbgChannels, MAX_LEVEL, DBG_CHANNELS_COUNT);
#elif defined (DEBUG_WARN)
    memset(DbgChannels, WARN_LEVEL|FIXME_LEVEL|ERR_LEVEL, DBG_CHANNELS_COUNT);
#elif defined (DEBUG_ERR)
    memset(DbgChannels, ERR_LEVEL, DBG_CHANNELS_COUNT);
#else
    memset(DbgChannels, 0, DBG_CHANNELS_COUNT);
#endif

#if defined (DEBUG_INIFILE)
    DbgChannels[DPRINT_INIFILE] = MAX_LEVEL;
#elif defined (DEBUG_REACTOS)
    DbgChannels[DPRINT_REACTOS] = MAX_LEVEL;
    DbgChannels[DPRINT_REGISTRY] = MAX_LEVEL;
#elif defined (DEBUG_CUSTOM)
    DbgChannels[DPRINT_WARNING] = MAX_LEVEL;
    DbgChannels[DPRINT_WINDOWS] = MAX_LEVEL;
#endif

    CommandLine = NULL;
    if (!DebugString || !*DebugString)
    {
        /* No command-line is provided: during pre-initialization,
         * initialize the debug port with default settings;
         * otherwise just return during main initialization */
        if (!Initialized)
            goto Done;
        return;
    }

    /* Get a copy of the command-line */
    strcpy(DbgStringBuffer, DebugString);
    CommandLine = DbgStringBuffer;

    /* Upcase it */
    _strupr(CommandLine);

    /* Get the port and baud rate */
    PortString = strstr(CommandLine, "DEBUGPORT");
    BaudString = strstr(CommandLine, "BAUDRATE");
    IrqString  = strstr(CommandLine, "IRQ");

    /*
     * Check if we got /DEBUGPORT parameters.
     * NOTE: Inspired by reactos/ntoskrnl/kd/kdinit.c, KdInitSystem(...)
     */
    while (PortString)
    {
        /* Move past the actual string, to reach the port*/
        PortString += strlen("DEBUGPORT");

        /* Now get past any spaces and skip the equal sign */
        while (*PortString == ' ') PortString++;
        PortString++;

        /* Check for possible ports and set the port to use */
        if (strncmp(PortString, "SCREEN", 6) == 0)
        {
            PortString += 6;
            DebugPort |= SCREEN;
        }
        else if (strncmp(PortString, "BOCHS", 5) == 0)
        {
            PortString += 5;
            DebugPort |= BOCHS;
        }
        else if (strncmp(PortString, "COM", 3) == 0)
        {
            PortString += 3;
            DebugPort |= RS232;

            /* Set the port to use */
            Value = atol(PortString);
            if (Value) ComPort = Value;
        }

        PortString = strstr(PortString, "DEBUGPORT");
   }

    /* Check if we got a baud rate */
    if (BaudString)
    {
        /* Move past the actual string, to reach the rate */
        BaudString += strlen("BAUDRATE");

        /* Now get past any spaces */
        while (*BaudString == ' ') BaudString++;

        /* And make sure we have a rate */
        if (*BaudString)
        {
            /* Read and set it */
            Value = atol(BaudString + 1);
            if (Value) BaudRate = Value;
        }
    }

    /* Check Serial Port Settings [IRQ] */
    if (IrqString)
    {
        /* Move past the actual string, to reach the rate */
        IrqString += strlen("IRQ");

        /* Now get past any spaces */
        while (*IrqString == ' ') IrqString++;

        /* And make sure we have an IRQ */
        if (*IrqString)
        {
            /* Read and set it */
            Value = atol(IrqString + 1);
            if (Value) PortIrq = Value;
        }
    }

Done:
    Initialized = TRUE;

    /* Try to initialize the port; if it fails, remove the corresponding flag */
    if (DebugPort & RS232)
    {
        if (!Rs232PortInitialize(ComPort, BaudRate))
            DebugPort &= ~RS232;
    }
}

VOID DebugPrintChar(UCHAR Character)
{
    if (Character == '\n')
        DebugStartOfLine = TRUE;

    if (DebugPort & RS232)
    {
        if (Character == '\n')
            Rs232PortPutByte('\r');

        Rs232PortPutByte(Character);
    }
    if (DebugPort & BOCHS)
    {
        WRITE_PORT_UCHAR((PUCHAR)BOCHS_OUTPUT_PORT, Character);
    }
    if (DebugPort & SCREEN)
    {
        MachConsPutChar(Character);
    }
}

ULONG
DbgPrint(const char *Format, ...)
{
    va_list ap;
    int Length;
    char* ptr;
    CHAR Buffer[512];

    va_start(ap, Format);
    Length = _vsnprintf(Buffer, sizeof(Buffer), Format, ap);
    va_end(ap);

    /* Check if we went past the buffer */
    if (Length == -1)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 1] = '\n';

        /* Put maximum */
        Length = sizeof(Buffer);
    }

    ptr = Buffer;
    while (Length--)
        DebugPrintChar(*ptr++);

    return 0;
}

VOID
DbgPrint2(ULONG Mask, ULONG Level, const char *File, ULONG Line, char *Format, ...)
{
    va_list ap;
    char Buffer[2096];
    char *ptr = Buffer;

    /* Mask out unwanted debug messages */
    if (!(DbgChannels[Mask] & Level) && !(Level & DBG_DEFAULT_LEVELS))
    {
        return;
    }

    /* Print the header if we have started a new line */
    if (DebugStartOfLine)
    {
        DbgPrint("(%s:%lu) ", File, Line);

        switch (Level)
        {
            case ERR_LEVEL:
                DbgPrint("err: ");
                break;
            case FIXME_LEVEL:
                DbgPrint("fixme: ");
                break;
            case WARN_LEVEL:
                DbgPrint("warn: ");
                break;
            case TRACE_LEVEL:
                DbgPrint("trace: ");
                break;
        }

        DebugStartOfLine = FALSE;
    }

    va_start(ap, Format);
    vsprintf(Buffer, Format, ap);
    va_end(ap);

    while (*ptr)
    {
        DebugPrintChar(*ptr++);
    }
}

VOID
DebugDumpBuffer(ULONG Mask, PVOID Buffer, ULONG Length)
{
    PUCHAR BufPtr = (PUCHAR)Buffer;
    ULONG Offset, Count, i;

    /* Mask out unwanted debug messages */
    if (!(DbgChannels[Mask] & TRACE_LEVEL))
        return;

    DebugStartOfLine = FALSE; // We don't want line headers
    DbgPrint("Dumping buffer at %p with length of %lu bytes:\n", Buffer, Length);

    Offset = 0;
    while (Offset < Length)
    {
        /* We don't want line headers */
        DebugStartOfLine = FALSE;

        /* Print the offset */
        DbgPrint("%04x:\t", Offset);

        /* Print either 16 or the remaining number of bytes */
        Count = min(Length - Offset, 16);
        for (i = 0; i < Count; i++, Offset++)
        {
            DbgPrint("%02x%c", BufPtr[Offset], (i == 7) ? '-' : ' ');
        }

        DbgPrint("\n");
    }
}

VOID
DebugDisableScreenPort(VOID)
{
    DebugPort &= ~SCREEN;
}

static BOOLEAN
DbgAddDebugChannel(CHAR* channel, CHAR* level, CHAR op)
{
    int iLevel, iChannel;

    if (channel == NULL || *channel == '\0' || strlen(channel) == 0)
        return FALSE;

    if (level == NULL || *level == '\0' || strlen(level) == 0)
        iLevel = MAX_LEVEL;
    else if (strcmp(level, "err") == 0)
        iLevel = ERR_LEVEL;
    else if (strcmp(level, "fixme") == 0)
        iLevel = FIXME_LEVEL;
    else if (strcmp(level, "warn") == 0)
        iLevel = WARN_LEVEL;
    else if (strcmp(level, "trace") == 0)
        iLevel = TRACE_LEVEL;
    else
        return FALSE;

         if (strcmp(channel, "memory"    ) == 0) iChannel = DPRINT_MEMORY;
    else if (strcmp(channel, "filesystem") == 0) iChannel = DPRINT_FILESYSTEM;
    else if (strcmp(channel, "inifile"   ) == 0) iChannel = DPRINT_INIFILE;
    else if (strcmp(channel, "ui"        ) == 0) iChannel = DPRINT_UI;
    else if (strcmp(channel, "disk"      ) == 0) iChannel = DPRINT_DISK;
    else if (strcmp(channel, "cache"     ) == 0) iChannel = DPRINT_CACHE;
    else if (strcmp(channel, "registry"  ) == 0) iChannel = DPRINT_REGISTRY;
    else if (strcmp(channel, "linux"     ) == 0) iChannel = DPRINT_LINUX;
    else if (strcmp(channel, "hwdetect"  ) == 0) iChannel = DPRINT_HWDETECT;
    else if (strcmp(channel, "windows"   ) == 0) iChannel = DPRINT_WINDOWS;
    else if (strcmp(channel, "peloader"  ) == 0) iChannel = DPRINT_PELOADER;
    else if (strcmp(channel, "scsiport"  ) == 0) iChannel = DPRINT_SCSIPORT;
    else if (strcmp(channel, "heap"      ) == 0) iChannel = DPRINT_HEAP;
    else if (strcmp(channel, "all"       ) == 0)
    {
        int i;

        for (i = 0; i < DBG_CHANNELS_COUNT; i++)
        {
            if (op == '+')
                DbgChannels[i] |= iLevel;
            else
                DbgChannels[i] &= ~iLevel;
        }

        return TRUE;
    }
    else return FALSE;

    if (op == '+')
        DbgChannels[iChannel] |= iLevel;
    else
        DbgChannels[iChannel] &= ~iLevel;

    return TRUE;
}

VOID
DbgParseDebugChannels(PCHAR Value)
{
    CHAR *str, *separator, *c, op;

    str = Value;

    do
    {
        separator = strchr(str, ',');
        if (separator != NULL)
            *separator = '\0';

        c = strchr(str, '+');
        if (c == NULL)
            c = strchr(str, '-');

        if (c != NULL)
        {
            op = *c;
            *c = '\0';
            c++;

            DbgAddDebugChannel(c, str, op);
        }

        str = separator + 1;
    } while (separator != NULL);
}

#else

#undef DebugInit
VOID
DebugInit(
    _In_ PCSTR DebugString)
{
    UNREFERENCED_PARAMETER(DebugString);
}

ULONG
DbgPrint(PCCH Format, ...)
{
    UNREFERENCED_PARAMETER(Format);
    return 0;
}

VOID
DbgPrint2(ULONG Mask, ULONG Level, const char *File, ULONG Line, char *Format, ...)
{
    UNREFERENCED_PARAMETER(Mask);
    UNREFERENCED_PARAMETER(Level);
    UNREFERENCED_PARAMETER(File);
    UNREFERENCED_PARAMETER(Line);
    UNREFERENCED_PARAMETER(Format);
}

VOID
DebugDumpBuffer(ULONG Mask, PVOID Buffer, ULONG Length)
{
    UNREFERENCED_PARAMETER(Mask);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);
}

#undef DbgParseDebugChannels
VOID
DbgParseDebugChannels(PCHAR Value)
{
    UNREFERENCED_PARAMETER(Value);
}

#endif // DBG

ULONG
MsgBoxPrint(const char *Format, ...)
{
    va_list ap;
    CHAR Buffer[512];
    ULONG Length;

    va_start(ap, Format);

    /* Construct a string */
    Length = _vsnprintf(Buffer, 512, Format, ap);

    /* Check if we went past the buffer */
    if (Length == MAXULONG)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 1] = '\n';

        /* Put maximum */
        Length = sizeof(Buffer);
    }

    /* Show it as a message box */
    UiMessageBox(Buffer);

    /* Cleanup and exit */
    va_end(ap);
    return 0;
}

DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckEx(
    IN ULONG  BugCheckCode,
    IN ULONG_PTR  BugCheckParameter1,
    IN ULONG_PTR  BugCheckParameter2,
    IN ULONG_PTR  BugCheckParameter3,
    IN ULONG_PTR  BugCheckParameter4)
{
    char Buffer[70];

    sprintf(Buffer,
            "*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)",
            BugCheckCode,
            (PVOID)BugCheckParameter1,
            (PVOID)BugCheckParameter2,
            (PVOID)BugCheckParameter3,
            (PVOID)BugCheckParameter4);

    UiMessageBoxCritical(Buffer);
    ASSERT(FALSE);
    for (;;);
}

VOID
NTAPI
RtlAssert(IN PVOID FailedAssertion,
          IN PVOID FileName,
          IN ULONG LineNumber,
          IN PCHAR Message OPTIONAL)
{
    if (Message)
    {
        DbgPrint("Assertion \'%s\' failed at %s line %lu: %s\n",
                 (PCHAR)FailedAssertion,
                 (PCHAR)FileName,
                 LineNumber,
                 Message);
    }
    else
    {
        DbgPrint("Assertion \'%s\' failed at %s line %lu\n",
                 (PCHAR)FailedAssertion,
                 (PCHAR)FileName,
                 LineNumber);
    }

    DbgBreakPoint();
}

char *BugCodeStrings[] =
{
    "TEST_BUGCHECK",
    "MISSING_HARDWARE_REQUIREMENTS",
    "FREELDR_IMAGE_CORRUPTION",
    "MEMORY_INIT_FAILURE",
#ifdef UEFIBOOT
    "EXIT_BOOTSERVICES_FAILURE",
#endif
};

ULONG_PTR BugCheckInfo[5];
