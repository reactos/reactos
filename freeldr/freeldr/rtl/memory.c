/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
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

int RtlCompareMemory(const PVOID Source1, const PVOID Source2, ULONG Length)
{
	ULONG		i;
	const PCHAR buffer1 = Source1;
	const PCHAR buffer2 = Source2;

	for (i=0; i<Length; i++)
	{
		if(buffer1[i] == buffer2[i])
			continue;
		else
			return (buffer1[i] - buffer2[i]);
	}

	return 0;
}

VOID RtlCopyMemory(PVOID Destination, const PVOID Source, ULONG Length)
{
	ULONG		i;
	PCHAR		buf1 = Destination;
	const PCHAR	buf2 = Source;

	for (i=0; i<Length; i++)
	{
		buf1[i] = buf2[i];
	}

}

VOID RtlFillMemory(PVOID Destination, ULONG Length, UCHAR Fill)
{
	ULONG		i;
	PUCHAR		buf1 = Destination;

	for (i=0; i<Length; i++)
	{
		buf1[i] = Fill;
	}

}

VOID RtlZeroMemory(PVOID Destination, ULONG Length)
{
	RtlFillMemory(Destination, Length, 0);
}
