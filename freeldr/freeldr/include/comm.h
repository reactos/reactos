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

#ifndef __RS232_H
#define __RS232_H

BOOL Rs232PortInitialize(U32 ComPort, U32 BaudRate);
BOOL Rs232PortGetByte(PUCHAR ByteRecieved);
BOOL Rs232PortPollByte(PUCHAR ByteRecieved);
VOID Rs232PortPutByte(UCHAR ByteToSend);



/*
 * Port I/O functions
 */

VOID
/*STDCALL*/
READ_PORT_BUFFER_UCHAR (PUCHAR Port, PUCHAR Value, U32 Count);

VOID
/*STDCALL*/
READ_PORT_BUFFER_ULONG (U32* Port, U32* Value, U32 Count);

VOID
/*STDCALL*/
READ_PORT_BUFFER_USHORT (U16* Port, U16* Value, U32 Count);

UCHAR
/*STDCALL*/
READ_PORT_UCHAR (PUCHAR Port);

U32
/*STDCALL*/
READ_PORT_ULONG (U32* Port);

U16
/*STDCALL*/
READ_PORT_USHORT (U16* Port);

VOID
/*STDCALL*/
WRITE_PORT_BUFFER_UCHAR (PUCHAR Port, PUCHAR Value, U32 Count);

VOID
/*STDCALL*/
WRITE_PORT_BUFFER_ULONG (U32* Port, U32* Value, U32 Count);

VOID
/*STDCALL*/
WRITE_PORT_BUFFER_USHORT (U16* Port, U16* Value, U32 Count);

VOID
/*STDCALL*/
WRITE_PORT_UCHAR (PUCHAR Port, UCHAR Value);

VOID
/*STDCALL*/
WRITE_PORT_ULONG (U32* Port, U32 Value);

VOID
/*STDCALL*/
WRITE_PORT_USHORT (U16* Port, U16 Value);


#endif // defined __RS232_H
