/*
 *  FreeLoader
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
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

#include "freeldr.h"
#include "debug.h"
#include "stdlib.h"
#include "rs232.h"
#include "parseini.h"
#include "portio.h"

#ifdef DEBUG

ULONG	DebugPrintMask = DPRINT_WARNING | DPRINT_MEMORY | DPRINT_FILESYSTEM | DPRINT_UI;

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
ULONG	BaudRate = 19200;

VOID DebugInit(VOID)
{
	if (DebugPort == RS232)
	{
		Rs232PortInitialize(ComPort, BaudRate);
	}
}

VOID DebugPrintChar(UCHAR Character)
{
	if (DebugPort == RS232)
	{
		Rs232PortPutByte(Character);
		if (Character == '\n')
		{
			Rs232PortPutByte('\r');
		}
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

VOID DebugPrint(ULONG Mask, char *format, ...)
{
	int *dataptr = (int *) &format;
	char c, *ptr, str[16];
	
	// Mask out unwanted debug messages
	if (!(Mask & DebugPrintMask))
	{
		return;
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
