/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
#include <rtl.h>
#include <comm.h>

#ifdef DEBUG

//#define DEBUG_ULTRA
//#define DEBUG_INIFILE
//#define DEBUG_REACTOS
//#define DEBUG_CUSTOM
#define DEBUG_NONE

#if defined (DEBUG_ULTRA)
U32		DebugPrintMask = DPRINT_WARNING | DPRINT_MEMORY | DPRINT_FILESYSTEM |
						DPRINT_UI | DPRINT_DISK | DPRINT_CACHE | DPRINT_REACTOS |
						DPRINT_LINUX;
#elif defined (DEBUG_INIFILE)
U32		DebugPrintMask = DPRINT_INIFILE;
#elif defined (DEBUG_REACTOS)
U32		DebugPrintMask = DPRINT_REACTOS | DPRINT_REGISTRY;
#elif defined (DEBUG_CUSTOM)
U32		DebugPrintMask = DPRINT_WARNING|DPRINT_FILESYSTEM;
#else //#elif defined (DEBUG_NONE)
U32		DebugPrintMask = 0;
#endif

#define	SCREEN				0
#define	RS232				1
#define BOCHS				2

#define	COM1				1
#define	COM2				2
#define	COM3				3
#define	COM4				4

#define BOCHS_OUTPUT_PORT	0xe9

//U32		DebugPort = RS232;
U32		DebugPort = SCREEN;
//U32		DebugPort = BOCHS;
U32		ComPort = COM1;
//U32		BaudRate = 19200;
U32		BaudRate = 115200;

BOOL	DebugStartOfLine = TRUE;

VOID DebugInit(VOID)
{
	if (DebugPort == RS232)
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

	if (DebugPort == RS232)
	{
		if (Character == '\n')
		{
			Rs232PortPutByte('\r');
		}
		Rs232PortPutByte(Character);
	}
	else if (DebugPort == BOCHS)
	{
		WRITE_PORT_UCHAR((PUCHAR)BOCHS_OUTPUT_PORT, Character);
	}
	else
	{
		putchar(Character);
	}
}

VOID DebugPrintHeader(U32 Mask)
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

VOID DebugPrint(U32 Mask, char *format, ...)
{
	int *dataptr = (int *) &format;
	char c, *ptr, str[16];
	
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

	dataptr++;

	while ((c = *(format++)))
	{
		if (c != '%')
		{
			DebugPrintChar(c);
		}
		else
		{
			switch (c = *(format++))
			{
			case 'd': case 'u': case 'x':
				
				*convert_to_ascii(str, c, *((unsigned long *) dataptr++)) = 0;

				ptr = str;

				while (*ptr)
				{
					DebugPrintChar(*(ptr++));
				}
				break;

			case 'c':

				DebugPrintChar((*(dataptr++))&0xff);
				break;

			case 's':

				ptr = (char *)(*(dataptr++));

				while ((c = *(ptr++)))
				{
					DebugPrintChar(c);
				}
				break;
			case '%':
				DebugPrintChar(c);
				break;
			default:
				DebugPrint(Mask, "\nDebugPrint() invalid format specifier - %%%c\n", c);
				break;
			}
		}
	}


	if (DebugPort == SCREEN)
	{
		//getch();
	}

}

VOID DebugDumpBuffer(U32 Mask, PVOID Buffer, U32 Length)
{
	PUCHAR	BufPtr = (PUCHAR)Buffer;
	U32		Idx;
	U32		Idx2;
	
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

#endif // defined DEBUG
