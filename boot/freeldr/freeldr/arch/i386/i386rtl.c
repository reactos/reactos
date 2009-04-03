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

void PcBeep(void)
{
	sound(700);
	delay(200);
	sound(0);
}

void delay(unsigned msec)
{
	REGS		Regs;
	unsigned	usec;
	unsigned	msec_this;

	// Int 15h AH=86h
	// BIOS - WAIT (AT,PS)
	//
	// AH = 86h
	// CX:DX = interval in microseconds
	// Return:
	// CF clear if successful (wait interval elapsed)
	// CF set on error or AH=83h wait already in progress
	// AH = status (see #00496)

	// Note: The resolution of the wait period is 977 microseconds on
	// many systems because many BIOSes use the 1/1024 second fast
	// interrupt from the AT real-time clock chip which is available on INT 70;
	// because newer BIOSes may have much more precise timers available, it is
	// not possible to use this function accurately for very short delays unless
	// the precise behavior of the BIOS is known (or found through testing)

	while (msec)
	{
		msec_this = msec;

		if (msec_this > 4000)
		{
			msec_this = 4000;
		}

		usec = msec_this * 1000;

		Regs.b.ah = 0x86;
		Regs.w.cx = usec >> 16;
		Regs.w.dx = usec & 0xffff;
		Int386(0x15, &Regs, &Regs);

		msec -= msec_this;
	}
}

void sound(int freq)
{
	int scale;

	if (freq == 0)
	{
		WRITE_PORT_UCHAR((PUCHAR)0x61, READ_PORT_UCHAR((PUCHAR)0x61) & ~3);
		return;
	}

	scale = 1193046 / freq;
	WRITE_PORT_UCHAR((PUCHAR)0x43, 0xb6);
	WRITE_PORT_UCHAR((PUCHAR)0x42, scale & 0xff);
	WRITE_PORT_UCHAR((PUCHAR)0x42, scale >> 8);
	WRITE_PORT_UCHAR((PUCHAR)0x61, READ_PORT_UCHAR((PUCHAR)0x61) | 3);
}
