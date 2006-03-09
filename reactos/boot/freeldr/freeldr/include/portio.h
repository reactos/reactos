/*
 *  FreeLoader
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2001  Eric Kohl
 *  Copyright (C) 2001  Emanuele Aliberti
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

#ifndef __PORTIO_H
#define __PORTIO_H

/*
 * Port I/O functions
 */

VOID
STDCALL
READ_PORT_BUFFER_UCHAR (PUCHAR Port, PUCHAR Value, ULONG Count);

VOID
STDCALL
READ_PORT_BUFFER_ULONG (ULONG* Port, ULONG* Value, ULONG Count);

VOID
STDCALL
READ_PORT_BUFFER_USHORT (USHORT* Port, USHORT* Value, ULONG Count);

UCHAR
STDCALL
READ_PORT_UCHAR (PUCHAR Port);

ULONG
STDCALL
READ_PORT_ULONG (ULONG* Port);

USHORT
STDCALL
READ_PORT_USHORT (USHORT* Port);

VOID
STDCALL
WRITE_PORT_BUFFER_UCHAR (PUCHAR Port, PUCHAR Value, ULONG Count);

VOID
STDCALL
WRITE_PORT_BUFFER_ULONG (ULONG* Port, ULONG* Value, ULONG Count);

VOID
STDCALL
WRITE_PORT_BUFFER_USHORT (USHORT* Port, USHORT* Value, ULONG Count);

VOID
STDCALL
WRITE_PORT_UCHAR (PUCHAR Port, UCHAR Value);

VOID
STDCALL
WRITE_PORT_ULONG (ULONG* Port, ULONG Value);

VOID
STDCALL
WRITE_PORT_USHORT (USHORT* Port, USHORT Value);


#endif // defined __PORTIO_H
