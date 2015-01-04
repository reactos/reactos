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

#if DBG && !defined(_M_ARM)

//#define DEBUG_ALL
//#define DEBUG_WARN
//#define DEBUG_ERR
//#define DEBUG_INIFILE
//#define DEBUG_REACTOS
//#define DEBUG_CUSTOM
#define DEBUG_NONE

#define DBG_DEFAULT_LEVELS (ERR_LEVEL|FIXME_LEVEL)

#define SCREEN  1
#define RS232   2
#define BOCHS   4

#define COM1    1
#define COM2    2
#define COM3    3
#define COM4    4

#define BOCHS_OUTPUT_PORT    0xe9


static UCHAR DbgChannels[DBG_CHANNELS_COUNT];

ULONG        DebugPort = RS232;
//ULONG        DebugPort = SCREEN;
//ULONG        DebugPort = BOCHS;
//ULONG        DebugPort = SCREEN|BOCHS;
#ifdef _WINKD_
/* COM1 is the WinDbg port */
ULONG        ComPort = COM2;
#else
ULONG        ComPort = COM1;
#endif
//ULONG        BaudRate = 19200;
ULONG        BaudRate = 115200;

BOOLEAN    DebugStartOfLine = TRUE;

VOID DebugInit(VOID)
{
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

    if (DebugPort & RS232)
    {
        Rs232PortInitialize(ComPort, BaudRate);
    }
}

VOID DebugPrintChar(UCHAR Character)
{
    if (Character == '\n')
    {
        DebugStartOfLine = TRUE;
    }

    if (DebugPort & RS232)
    {
        if (Character == '\n')
        {
            Rs232PortPutByte('\r');
        }
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
    int i;
    int Length;
    va_list ap;
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

    for (i = 0; i < Length; i++)
    {
        DebugPrintChar(Buffer[i]);
    }

    return 0;
}

VOID
DbgPrint2(ULONG Mask, ULONG Level, const char *File, ULONG Line, char *Format, ...)
{
    va_list ap;
    char Buffer[2096];
    char *ptr = Buffer;

    // Mask out unwanted debug messages
    if (!(DbgChannels[Mask] & Level) && !(Level & DBG_DEFAULT_LEVELS ))
    {
        return;
    }

    // Print the header if we have started a new line
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
    PUCHAR    BufPtr = (PUCHAR)Buffer;
    ULONG        Idx;
    ULONG        Idx2;

    // Mask out unwanted debug messages
    if (!(DbgChannels[Mask] & TRACE_LEVEL))
    {
        return;
    }

    DebugStartOfLine = FALSE; // We don't want line headers
    DbgPrint("Dumping buffer at %p with length of %lu bytes:\n", Buffer, Length);

    for (Idx=0; Idx<Length; )
    {
        DebugStartOfLine = FALSE; // We don't want line headers

        if (Idx < 0x0010)
        {
            DbgPrint("000%x:\t", Idx);
        }
        else if (Idx < 0x0100)
        {
            DbgPrint("00%x:\t", Idx);
        }
        else if (Idx < 0x1000)
        {
            DbgPrint("0%x:\t", Idx);
        }
        else
        {
            DbgPrint("%x:\t", Idx);
        }

        for (Idx2=0; Idx2<16; Idx2++,Idx++)
        {
            if (BufPtr[Idx] < 0x10)
            {
                DbgPrint("0");
            }
            DbgPrint("%x", BufPtr[Idx]);

            if (Idx2 == 7)
            {
                DbgPrint("-");
            }
            else
            {
                DbgPrint(" ");
            }
        }

        Idx -= 16;
        DbgPrint(" ");

        for (Idx2=0; Idx2<16; Idx2++,Idx++)
        {
            if ((BufPtr[Idx] > 20) && (BufPtr[Idx] < 0x80))
            {
                DbgPrint("%c", BufPtr[Idx]);
            }
            else
            {
                DbgPrint(".");
            }
        }

        DbgPrint("\n");
    }
}

static BOOLEAN
DbgAddDebugChannel( CHAR* channel, CHAR* level, CHAR op)
{
    int iLevel, iChannel;

    if(channel == NULL || *channel == L'\0' ||strlen(channel) == 0 )
        return FALSE;

    if(level == NULL || *level == L'\0' ||strlen(level) == 0 )
        iLevel = MAX_LEVEL;
    else if(strcmp(level, "err") == 0)
        iLevel = ERR_LEVEL;
    else if(strcmp(level, "fixme") == 0)
        iLevel = FIXME_LEVEL;
    else if(strcmp(level, "warn") == 0)
        iLevel = WARN_LEVEL;
    else if (strcmp(level, "trace") == 0)
        iLevel = TRACE_LEVEL;
    else
        return FALSE;

    if(strcmp(channel, "memory") == 0) iChannel = DPRINT_MEMORY;
    else if(strcmp(channel, "filesystem") == 0) iChannel = DPRINT_FILESYSTEM;
    else if(strcmp(channel, "inifile") == 0) iChannel = DPRINT_INIFILE;
    else if(strcmp(channel, "ui") == 0) iChannel = DPRINT_UI;
    else if(strcmp(channel, "disk") == 0) iChannel = DPRINT_DISK;
    else if(strcmp(channel, "cache") == 0) iChannel = DPRINT_CACHE;
    else if(strcmp(channel, "registry") == 0) iChannel = DPRINT_REGISTRY;
    else if(strcmp(channel, "linux") == 0) iChannel = DPRINT_LINUX;
    else if(strcmp(channel, "hwdetect") == 0) iChannel = DPRINT_HWDETECT;
    else if(strcmp(channel, "windows") == 0) iChannel = DPRINT_WINDOWS;
    else if(strcmp(channel, "peloader") == 0) iChannel = DPRINT_PELOADER;
    else if(strcmp(channel, "scsiport") == 0) iChannel = DPRINT_SCSIPORT;
    else if(strcmp(channel, "heap") == 0) iChannel = DPRINT_HEAP;
    else if(strcmp(channel, "all") == 0)
    {
        int i;

        for(i= 0 ; i < DBG_CHANNELS_COUNT; i++)
        {
            if(op==L'+')
                DbgChannels[i] |= iLevel;
            else
                DbgChannels[i] &= ~iLevel;
        }

        return TRUE;
    }
    else return FALSE;

    if(op==L'+')
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
        separator = strchr(str, L',');
        if(separator != NULL)
            *separator = L'\0';

        c = strchr(str, L'+');
        if(c == NULL)
            c = strchr(str, L'-');

        if(c != NULL)
        {
            op = *c;
            *c = L'\0';
            c++;

            DbgAddDebugChannel(c, str, op);
        }

        str = separator + 1;
    } while(separator != NULL);
}

#else

ULONG
DbgPrint(PCCH Format, ...)
{
    return 0;
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

//DECLSPEC_NORETURN
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
    sprintf(Buffer, "*** STOP: 0x%08lX (0x%08lX, 0x%08lX, 0x%08lX, 0x%08lX)",
        BugCheckCode, BugCheckParameter1, BugCheckParameter2,
        BugCheckParameter3, BugCheckParameter4);
    UiMessageBoxCritical(Buffer);
    assert(FALSE);
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
      DbgPrint("Assertion \'%s\' failed at %s line %u: %s\n",
               (PCHAR)FailedAssertion,
               (PCHAR)FileName,
               LineNumber,
               Message);
   }
   else
   {
      DbgPrint("Assertion \'%s\' failed at %s line %u\n",
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
};

ULONG_PTR BugCheckInfo[5];

