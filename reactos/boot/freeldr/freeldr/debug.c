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
//#define DEBUG_INIFILE
//#define DEBUG_REACTOS
//#define DEBUG_CUSTOM
#define DEBUG_NONE

#if defined (DEBUG_ALL)
ULONG   DebugPrintMask = DPRINT_WARNING | DPRINT_MEMORY | DPRINT_FILESYSTEM |
		                 DPRINT_UI | DPRINT_DISK | DPRINT_CACHE | DPRINT_REACTOS |
		                 DPRINT_LINUX | DPRINT_HWDETECT | DPRINT_PELOADER | DPRINT_WINDOWS;
#elif defined (DEBUG_INIFILE)
ULONG   DebugPrintMask = DPRINT_INIFILE;
#elif defined (DEBUG_REACTOS)
ULONG   DebugPrintMask = DPRINT_REACTOS | DPRINT_REGISTRY;
#elif defined (DEBUG_CUSTOM)
ULONG   DebugPrintMask = DPRINT_WARNING | DPRINT_WINDOWS;
#else //#elif defined (DEBUG_NONE)
ULONG   DebugPrintMask = 0;
#endif

#define	SCREEN				1
#define	RS232				2
#define BOCHS				4

#define	COM1				1
#define	COM2				2
#define	COM3				3
#define	COM4				4

#define BOCHS_OUTPUT_PORT	0xe9

ULONG		DebugPort = RS232;
//ULONG		DebugPort = SCREEN;
//ULONG		DebugPort = BOCHS;
//ULONG		DebugPort = SCREEN|BOCHS;
ULONG		ComPort = COM1;
//ULONG		BaudRate = 19200;
ULONG		BaudRate = 115200;

BOOLEAN	DebugStartOfLine = TRUE;

VOID DebugInit(VOID)
{
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
	if (Level >= WARN_LEVEL && !(Mask & DebugPrintMask))
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
	PUCHAR	BufPtr = (PUCHAR)Buffer;
	ULONG		Idx;
	ULONG		Idx2;

	// Mask out unwanted debug messages
	if (!(Mask & DebugPrintMask))
	{
		return;
	}

	DebugStartOfLine = FALSE; // We don't want line headers
	DbgPrint("Dumping buffer at 0x%x with length of %d bytes:\n", Buffer, Length);

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
