/* $Id: lpctypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
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
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/lpctypes.h
 * PURPOSE:         Definitions for Local Procedure Call Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _LPCTYPES_H
#define _LPCTYPES_H

/* Local Procedure Call (LPC) */

#define LPC_MESSAGE_BASE_SIZE	4
#define MAX_MESSAGE_DATA	(0x130)

typedef struct _LPC_MESSAGE {
	USHORT  DataSize;
	USHORT  MessageSize;
	USHORT  MessageType;
	USHORT  VirtualRangesOffset;
	CLIENT_ID  ClientId;
	ULONG  MessageId;
	ULONG  SectionSize;
	UCHAR  Data[ANYSIZE_ARRAY];
} LPC_MESSAGE, *PLPC_MESSAGE;

typedef enum _LPC_TYPE {
	LPC_NEW_MESSAGE,
	LPC_REQUEST,
	LPC_REPLY,
	LPC_DATAGRAM,
	LPC_LOST_REPLY,
	LPC_PORT_CLOSED,
	LPC_CLIENT_DIED,
	LPC_EXCEPTION,
	LPC_DEBUG_EVENT,
	LPC_ERROR_EVENT,
	LPC_CONNECTION_REQUEST,
	LPC_CONNECTION_REFUSED,
	LPC_MAXIMUM
} LPC_TYPE;

typedef struct _LPC_SECTION_WRITE {
	ULONG  Length;
	HANDLE  SectionHandle;
	ULONG  SectionOffset;
	ULONG  ViewSize;
	PVOID  ViewBase;
	PVOID  TargetViewBase;
} LPC_SECTION_WRITE, *PLPC_SECTION_WRITE;

typedef struct _LPC_SECTION_READ {
	ULONG  Length;
	ULONG  ViewSize;
	PVOID  ViewBase;
} LPC_SECTION_READ, *PLPC_SECTION_READ; 

typedef struct _LPC_MAX_MESSAGE {
	LPC_MESSAGE Header;
	BYTE Data[MAX_MESSAGE_DATA];
} LPC_MAX_MESSAGE, *PLPC_MAX_MESSAGE;

#endif
