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
#include <arch.h>


void putchar(int ch)
{
	REGS	Regs;

	/* If we are displaying a CR '\n' then do a LF also */
	if (ch == '\n')
	{
		/* Display the LF */
		putchar('\r');
	}

	/* If we are displaying a TAB '\t' then display 8 spaces ' ' */
	if (ch == '\t')
	{
		/* Display the 8 spaces ' ' */
		putchar(' ');
		putchar(' ');
		putchar(' ');
		putchar(' ');
		putchar(' ');
		putchar(' ');
		putchar(' ');
		putchar(' ');
		return;
	}

	// Int 10h AH=0Eh
	// VIDEO - TELETYPE OUTPUT
	//
	// AH = 0Eh
	// AL = character to write
	// BH = page number
	// BL = foreground color (graphics modes only)
	Regs.b.ah = 0x0E;
	Regs.b.al = ch;
	Regs.w.bx = 1;
	Int386(0x10, &Regs, &Regs);
}

int kbhit(void)
{
	REGS	Regs;

	// Int 16h AH=01h
	// KEYBOARD - CHECK FOR KEYSTROKE
	//
	// AH = 01h
	// Return:
	// ZF set if no keystroke available
	// ZF clear if keystroke available
	// AH = BIOS scan code
	// AL = ASCII character
	Regs.b.ah = 0x01;
	Int386(0x16, &Regs, &Regs);

	if (Regs.x.eflags & I386FLAG_ZF)
	{
		return 0;
	}

	return 1;
}

int getch(void)
{
	REGS	Regs;
	static BOOL		ExtendedKey = FALSE;
	static char		ExtendedScanCode = 0;

	// If the last time we were called an
	// extended key was pressed then return
	// that keys scan code.
	if (ExtendedKey)
	{
		ExtendedKey = FALSE;
		return ExtendedScanCode;
	}

	// Int 16h AH=00h
	// KEYBOARD - GET KEYSTROKE
	//
	// AH = 00h
	// Return:
	// AH = BIOS scan code
	// AL = ASCII character
	Regs.b.ah = 0x00;
	Int386(0x16, &Regs, &Regs);

	// Check for an extended keystroke
	if (Regs.b.al == 0)
	{
		ExtendedKey = TRUE;
		ExtendedScanCode = Regs.b.ah;
	}

	// Return keystroke
	return Regs.b.al;
}

int getyear(void)
{
	REGS	Regs;
	U16		Digit1;
	U16		Digit2;
	U16		Cent1;
	U16		Cent2;
	U16		Year;

	// Some BIOSes, such es the 1998/07/25 system ROM
	// in the Compaq Deskpro EP/SB, leave CF unchanged
	// if successful, so CF should be cleared before
	// calling this function.
	__asm__ ("clc");

	// Int 1Ah AH=04h
	// TIME - GET REAL-TIME CLOCK DATE (AT,XT286,PS)
	//
	// AH = 04h
	// CF clear to avoid bug
	// Return:
	// CF clear if successful
	// CH = century (BCD)
	// CL = year (BCD)
	// DH = month (BCD)
	// DL = day (BCD)
	// CF set on error
	Regs.b.ah = 0x04;
	Int386(0x1A, &Regs, &Regs);

	/* Convert from BCD to normal */
	Digit1 = Regs.b.cl & 0x0F;
	Digit2 = ((Regs.b.cl >> 4) & 0x0F) * 10;
	Cent1 = Regs.b.ch & 0x0F;
	Cent2 = ((Regs.b.ch >> 4) & 0x0F) * 10;

	Year = Cent1 + Cent2;
	Year *= 100;
	Year += Digit1 + Digit2;

	return Year;
}

int getday(void)
{
	REGS	Regs;
	U16		Digit1;
	U16		Digit2;

	// Some BIOSes, such es the 1998/07/25 system ROM
	// in the Compaq Deskpro EP/SB, leave CF unchanged
	// if successful, so CF should be cleared before
	// calling this function.
	__asm__ ("clc");

	// Int 1Ah AH=04h
	// TIME - GET REAL-TIME CLOCK DATE (AT,XT286,PS)
	//
	// AH = 04h
	// CF clear to avoid bug
	// Return:
	// CF clear if successful
	// CH = century (BCD)
	// CL = year (BCD)
	// DH = month (BCD)
	// DL = day (BCD)
	// CF set on error
	Regs.b.ah = 0x04;
	Int386(0x1A, &Regs, &Regs);

	/* Convert from BCD to normal */
	Digit1 = Regs.b.dl & 0x0F;
	Digit2 = ((Regs.b.dl >> 4) & 0x0F) * 10;

	return (Digit1 + Digit2);
}

int getmonth(void)
{
	REGS	Regs;
	U16		Digit1;
	U16		Digit2;

	// Some BIOSes, such es the 1998/07/25 system ROM
	// in the Compaq Deskpro EP/SB, leave CF unchanged
	// if successful, so CF should be cleared before
	// calling this function.
	__asm__ ("clc");

	// Int 1Ah AH=04h
	// TIME - GET REAL-TIME CLOCK DATE (AT,XT286,PS)
	//
	// AH = 04h
	// CF clear to avoid bug
	// Return:
	// CF clear if successful
	// CH = century (BCD)
	// CL = year (BCD)
	// DH = month (BCD)
	// DL = day (BCD)
	// CF set on error
	Regs.b.ah = 0x04;
	Int386(0x1A, &Regs, &Regs);

	/* Convert from BCD to normal */
	Digit1 = Regs.b.dh & 0x0F;
	Digit2 = ((Regs.b.dh >> 4) & 0x0F) * 10;

	return (Digit1 + Digit2);
}

int gethour(void)
{
	REGS	Regs;
	U16		Digit1;
	U16		Digit2;

	// Some BIOSes leave CF unchanged if successful,
	// so CF should be cleared before calling this function.
	__asm__ ("clc");

	// Int 1Ah AH=02h
	// TIME - GET REAL-TIME CLOCK TIME (AT,XT286,PS)
	//
	// AH = 02h
	// CF clear to avoid bug
	// Return:
	// CF clear if successful
	// CH = hour (BCD)
	// CL = minutes (BCD)
	// DH = seconds (BCD)
	// DL = daylight savings flag (00h standard time, 01h daylight time)
	// CF set on error (i.e. clock not running or in middle of update)
	Regs.b.ah = 0x02;
	Int386(0x1A, &Regs, &Regs);

	/* Convert from BCD to normal */
	Digit1 = Regs.b.ch & 0x0F;
	Digit2 = ((Regs.b.ch >> 4) & 0x0F) * 10;

	return (Digit1 + Digit2);
}

int getminute(void)
{
	REGS	Regs;
	U16		Digit1;
	U16		Digit2;

	// Some BIOSes leave CF unchanged if successful,
	// so CF should be cleared before calling this function.
	__asm__ ("clc");

	// Int 1Ah AH=02h
	// TIME - GET REAL-TIME CLOCK TIME (AT,XT286,PS)
	//
	// AH = 02h
	// CF clear to avoid bug
	// Return:
	// CF clear if successful
	// CH = hour (BCD)
	// CL = minutes (BCD)
	// DH = seconds (BCD)
	// DL = daylight savings flag (00h standard time, 01h daylight time)
	// CF set on error (i.e. clock not running or in middle of update)
	Regs.b.ah = 0x02;
	Int386(0x1A, &Regs, &Regs);

	/* Convert from BCD to normal */
	Digit1 = Regs.b.cl & 0x0F;
	Digit2 = ((Regs.b.cl >> 4) & 0x0F) * 10;

	return (Digit1 + Digit2);
}

int getsecond(void)
{
	REGS	Regs;
	U16		Digit1;
	U16		Digit2;

	// Some BIOSes leave CF unchanged if successful,
	// so CF should be cleared before calling this function.
	__asm__ ("clc");

	// Int 1Ah AH=02h
	// TIME - GET REAL-TIME CLOCK TIME (AT,XT286,PS)
	//
	// AH = 02h
	// CF clear to avoid bug
	// Return:
	// CF clear if successful
	// CH = hour (BCD)
	// CL = minutes (BCD)
	// DH = seconds (BCD)
	// DL = daylight savings flag (00h standard time, 01h daylight time)
	// CF set on error (i.e. clock not running or in middle of update)
	Regs.b.ah = 0x02;
	Int386(0x1A, &Regs, &Regs);

	/* Convert from BCD to normal */
	Digit1 = Regs.b.dh & 0x0F;
	Digit2 = ((Regs.b.dh >> 4) & 0x0F) * 10;

	return (Digit1 + Digit2);
}
