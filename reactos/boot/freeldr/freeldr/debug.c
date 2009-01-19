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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#include <debug.h>

#ifdef DBG

//#define DEBUG_ALL
//#define DEBUG_INIFILE
//#define DEBUG_REACTOS
//#define DEBUG_CUSTOM
#define DEBUG_NONE

#if defined (DEBUG_ALL)
ULONG		DebugPrintMask = DPRINT_WARNING | DPRINT_MEMORY | DPRINT_FILESYSTEM |
		                 DPRINT_UI | DPRINT_DISK | DPRINT_CACHE | DPRINT_REACTOS |
		                 DPRINT_LINUX | DPRINT_HWDETECT;
#elif defined (DEBUG_INIFILE)
ULONG		DebugPrintMask = DPRINT_INIFILE;
#elif defined (DEBUG_REACTOS)
ULONG		DebugPrintMask = DPRINT_REACTOS | DPRINT_REGISTRY;
#elif defined (DEBUG_CUSTOM)
ULONG		DebugPrintMask = DPRINT_WARNING |
		                 DPRINT_UI | DPRINT_CACHE | DPRINT_REACTOS |
		                 DPRINT_LINUX;
#else //#elif defined (DEBUG_NONE)
ULONG		DebugPrintMask = 0;
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
	va_list ap;
	CHAR Buffer[512];
	ULONG Length;
	char *ptr = Buffer;

	va_start(ap, Format);

	/* Construct a string */
	Length = _vsnprintf(Buffer, 512, Format, ap);

	/* Check if we went past the buffer */
	if (Length == -1)
	{
		/* Terminate it if we went over-board */
		Buffer[sizeof(Buffer) - 1] = '\n';

		/* Put maximum */
		Length = sizeof(Buffer);
	}

	while (*ptr)
	{
		DebugPrintChar(*ptr++);
	}

	va_end(ap);
	return 0;
}

VOID DebugPrintHeader(ULONG Mask)
{
  /* No header */
  if (Mask == 0)
    return;

	switch (Mask)
	{
	case DPRINT_WARNING:
	    DbgPrint("WARNING: ");
		break;
	case DPRINT_MEMORY:
	    DbgPrint("MEMORY: ");
		break;
	case DPRINT_FILESYSTEM:
	    DbgPrint("FILESYS: ");
		break;
	case DPRINT_INIFILE:
	    DbgPrint("INIFILE: ");
		break;
	case DPRINT_UI:
	    DbgPrint("UI: ");
		break;
	case DPRINT_DISK:
	    DbgPrint("DISK: ");
		break;
	case DPRINT_CACHE:
	    DbgPrint("CACHE: ");
		break;
	case DPRINT_REGISTRY:
	    DbgPrint("REGISTER: ");
		break;
	case DPRINT_REACTOS:
	    DbgPrint("REACTOS: ");
		break;
	case DPRINT_LINUX:
	    DbgPrint("LINUX: ");
		break;
	case DPRINT_WINDOWS:
	    DbgPrint("WINLDR: ");
		break;
	case DPRINT_HWDETECT:
	    DbgPrint("HWDETECT: ");
		break;
	default:
	    DbgPrint("UNKNOWN: ");
		break;
	}
}

char* g_file;
int g_line;

VOID DbgPrintMask(ULONG Mask, char *format, ...)
{
	va_list ap;
	char Buffer[2096];
	char *ptr = Buffer;

	// Mask out unwanted debug messages
	if (!(Mask & DebugPrintMask))
	{
		return;
	}

	// Print the header if we have started a new line
	if (DebugStartOfLine)
	{
        DbgPrint("(%s:%d) ", g_file, g_line);
		DebugPrintHeader(Mask);
		DebugStartOfLine = FALSE;
	}

	va_start(ap, format);
	vsprintf(Buffer, format, ap);
	va_end(ap);
	while (*ptr)
	{
		DebugPrintChar(*ptr++);
	}
}

VOID DebugDumpBuffer(ULONG Mask, PVOID Buffer, ULONG Length)
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
	DbgPrintMask(Mask, "Dumping buffer at 0x%x with length of %d bytes:\n", Buffer, Length);

	for (Idx=0; Idx<Length; )
	{
		DebugStartOfLine = FALSE; // We don't want line headers

		if (Idx < 0x0010)
		{
			DbgPrintMask(Mask, "000%x:\t", Idx);
		}
		else if (Idx < 0x0100)
		{
			DbgPrintMask(Mask, "00%x:\t", Idx);
		}
		else if (Idx < 0x1000)
		{
			DbgPrintMask(Mask, "0%x:\t", Idx);
		}
		else
		{
			DbgPrintMask(Mask, "%x:\t", Idx);
		}

		for (Idx2=0; Idx2<16; Idx2++,Idx++)
		{
			if (BufPtr[Idx] < 0x10)
			{
				DbgPrintMask(Mask, "0");
			}
			DbgPrintMask(Mask, "%x", BufPtr[Idx]);

			if (Idx2 == 7)
			{
				DbgPrintMask(Mask, "-");
			}
			else
			{
				DbgPrintMask(Mask, " ");
			}
		}

		Idx -= 16;
		DbgPrintMask(Mask, " ");

		for (Idx2=0; Idx2<16; Idx2++,Idx++)
		{
			if ((BufPtr[Idx] > 20) && (BufPtr[Idx] < 0x80))
			{
				DbgPrintMask(Mask, "%c", BufPtr[Idx]);
			}
			else
			{
				DbgPrintMask(Mask, ".");
			}
		}

		DbgPrintMask(Mask, "\n");
	}
}

#else

VOID DbgPrintMask(ULONG Mask, char *format, ...)
{
}

#endif // defined DBG

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
	if (Length == -1U)
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

