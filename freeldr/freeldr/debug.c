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

//ULONG	DebugPrintMask = DPRINT_WARNING | DPRINT_MEMORY | DPRINT_FILESYSTEM |
//						 DPRINT_UI | DPRINT_DISK | DPRINT_CACHE;
ULONG	DebugPrintMask = DPRINT_WARNING | DPRINT_MEMORY | DPRINT_FILESYSTEM |
						 DPRINT_UI | DPRINT_DISK;
//ULONG	DebugPrintMask = DPRINT_INIFILE;

#define	SCREEN				0
#define	RS232				1
#define BOCHS				2

#define	COM1				1
#define	COM2				2
#define	COM3				3
#define	COM4				4

#define BOCHS_OUTPUT_PORT	0xe9

//ULONG	DebugPort = RS232;
//ULONG	DebugPort = SCREEN;
ULONG	DebugPort = BOCHS;
ULONG	ComPort = COM1;
//ULONG	BaudRate = 19200;
ULONG	BaudRate = 115200;

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

VOID DebugPrintHeader(ULONG Mask)
{
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
			}
		}
	}


	if (DebugPort == SCREEN)
	{
		//getch();
	}

}

#endif // defined DEBUG
