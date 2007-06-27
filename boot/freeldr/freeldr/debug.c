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
ULONG		DebugPrintMask = DPRINT_WARNING | DPRINT_MEMORY |
		                 DPRINT_REACTOS | DPRINT_WINDOWS | DPRINT_HWDETECT;
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

VOID DebugPrintHeader(ULONG Mask)
{
  /* No header */
  if (Mask == 0)
    return;

	switch (Mask)
	{
	case DPRINT_WARNING:
		DebugPrintChar('W');
		DebugPrintChar('A');
		DebugPrintChar('R');
		DebugPrintChar('N');
		DebugPrintChar('I');
		DebugPrintChar('N');
		DebugPrintChar('G');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_MEMORY:
		DebugPrintChar('M');
		DebugPrintChar('E');
		DebugPrintChar('M');
		DebugPrintChar('O');
		DebugPrintChar('R');
		DebugPrintChar('Y');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_FILESYSTEM:
		DebugPrintChar('F');
		DebugPrintChar('I');
		DebugPrintChar('L');
		DebugPrintChar('E');
		DebugPrintChar('S');
		DebugPrintChar('Y');
		DebugPrintChar('S');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_INIFILE:
		DebugPrintChar('I');
		DebugPrintChar('N');
		DebugPrintChar('I');
		DebugPrintChar('F');
		DebugPrintChar('I');
		DebugPrintChar('L');
		DebugPrintChar('E');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_UI:
		DebugPrintChar('U');
		DebugPrintChar('I');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_DISK:
		DebugPrintChar('D');
		DebugPrintChar('I');
		DebugPrintChar('S');
		DebugPrintChar('K');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_CACHE:
		DebugPrintChar('C');
		DebugPrintChar('A');
		DebugPrintChar('C');
		DebugPrintChar('H');
		DebugPrintChar('E');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_REGISTRY:
		DebugPrintChar('R');
		DebugPrintChar('E');
		DebugPrintChar('G');
		DebugPrintChar('I');
		DebugPrintChar('S');
		DebugPrintChar('T');
		DebugPrintChar('R');
		DebugPrintChar('Y');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_REACTOS:
		DebugPrintChar('R');
		DebugPrintChar('E');
		DebugPrintChar('A');
		DebugPrintChar('C');
		DebugPrintChar('T');
		DebugPrintChar('O');
		DebugPrintChar('S');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_LINUX:
		DebugPrintChar('L');
		DebugPrintChar('I');
		DebugPrintChar('N');
		DebugPrintChar('U');
		DebugPrintChar('X');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_WINDOWS:
		DebugPrintChar('W');
		DebugPrintChar('I');
		DebugPrintChar('N');
		DebugPrintChar('L');
		DebugPrintChar('D');
		DebugPrintChar('R');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	case DPRINT_HWDETECT:
		DebugPrintChar('H');
		DebugPrintChar('W');
		DebugPrintChar('D');
		DebugPrintChar('E');
		DebugPrintChar('T');
		DebugPrintChar('E');
		DebugPrintChar('C');
		DebugPrintChar('T');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	default:
		DebugPrintChar('U');
		DebugPrintChar('N');
		DebugPrintChar('K');
		DebugPrintChar('N');
		DebugPrintChar('O');
		DebugPrintChar('W');
		DebugPrintChar('N');
		DebugPrintChar(':');
		DebugPrintChar(' ');
		break;
	}
}

VOID DebugPrint(ULONG Mask, char *format, ...)
{
	va_list ap;
	char Buffer[4096];
	char *ptr = Buffer;

	// Mask out unwanted debug messages
	if (!(Mask & DebugPrintMask))
	{
		return;
	}

	// Print the header if we have started a new line
	if (DebugStartOfLine)
	{
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

VOID DebugPrint1(char *format, ...)
{
	va_list ap;
	char Buffer[4096];
	char *ptr = Buffer;

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
	DebugPrint(Mask, "Dumping buffer at 0x%x with length of %d bytes:\n", Buffer, Length);

	for (Idx=0; Idx<Length; )
	{
		DebugStartOfLine = FALSE; // We don't want line headers

		if (Idx < 0x0010)
		{
			DebugPrint(Mask, "000%x:\t", Idx);
		}
		else if (Idx < 0x0100)
		{
			DebugPrint(Mask, "00%x:\t", Idx);
		}
		else if (Idx < 0x1000)
		{
			DebugPrint(Mask, "0%x:\t", Idx);
		}
		else
		{
			DebugPrint(Mask, "%x:\t", Idx);
		}

		for (Idx2=0; Idx2<16; Idx2++,Idx++)
		{
			if (BufPtr[Idx] < 0x10)
			{
				DebugPrint(Mask, "0");
			}
			DebugPrint(Mask, "%x", BufPtr[Idx]);

			if (Idx2 == 7)
			{
				DebugPrint(Mask, "-");
			}
			else
			{
				DebugPrint(Mask, " ");
			}
		}

		Idx -= 16;
		DebugPrint(Mask, " ");

		for (Idx2=0; Idx2<16; Idx2++,Idx++)
		{
			if ((BufPtr[Idx] > 20) && (BufPtr[Idx] < 0x80))
			{
				DebugPrint(Mask, "%c", BufPtr[Idx]);
			}
			else
			{
				DebugPrint(Mask, ".");
			}
		}

		DebugPrint(Mask, "\n");
	}
}

#else

VOID DebugPrint(ULONG Mask, char *format, ...)
{
}

#endif // defined DBG
