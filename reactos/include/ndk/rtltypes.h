/* $Id: rtltypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
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
 * FILE:            include/ndk/rtltypes.h
 * PURPOSE:         Defintions for Runtime Library Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _RTLTYPES_H
#define _RTLTYPES_H

#define MAXIMUM_LEADBYTES 12

typedef struct {
	ACE_HEADER Header;
	ACCESS_MASK AccessMask;
} ACE, *PACE;

typedef struct _RTL_HEAP_DEFINITION {
	ULONG Length;
	ULONG Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

typedef struct _RTL_MESSAGE_RESOURCE_ENTRY {
	USHORT Length;
	USHORT Flags;
	UCHAR Text[1];
} RTL_MESSAGE_RESOURCE_ENTRY, *PRTL_MESSAGE_RESOURCE_ENTRY;

typedef struct _RTL_MESSAGE_RESOURCE_BLOCK {
	ULONG LowId;
	ULONG HighId;
	ULONG OffsetToEntries;
} RTL_MESSAGE_RESOURCE_BLOCK, *PRTL_MESSAGE_RESOURCE_BLOCK;

typedef struct _RTL_MESSAGE_RESOURCE_DATA {
	ULONG NumberOfBlocks;
	RTL_MESSAGE_RESOURCE_BLOCK Blocks[1];
} RTL_MESSAGE_RESOURCE_DATA, *PRTL_MESSAGE_RESOURCE_DATA;

typedef struct _NLS_FILE_HEADER {
	USHORT  HeaderSize;
	USHORT  CodePage;
	USHORT  MaximumCharacterSize;  /* SBCS = 1, DBCS = 2 */
	USHORT  DefaultChar;
	USHORT  UniDefaultChar;
	USHORT  TransDefaultChar;
	USHORT  TransUniDefaultChar;
	USHORT  DBCSCodePage;
	UCHAR   LeadByte[MAXIMUM_LEADBYTES];
} NLS_FILE_HEADER, *PNLS_FILE_HEADER;

/* Let Kernel Drivers use this */
#ifndef _WINDOWS_H
typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _TIME_ZONE_INFORMATION {
	LONG Bias;
	WCHAR StandardName[32];
	SYSTEMTIME StandardDate;
	LONG StandardBias;
	WCHAR DaylightName[32];
	SYSTEMTIME DaylightDate;
	LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;
#endif

#define	EH_NONCONTINUABLE	0x01
#define	EH_UNWINDING		0x02
#define	EH_EXIT_UNWIND		0x04
#define	EH_STACK_INVALID	0x08
#define	EH_NESTED_CALL		0x10

typedef enum {
	ExceptionContinueExecution,
	ExceptionContinueSearch,
	ExceptionNestedException,
	ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;

typedef EXCEPTION_DISPOSITION (*PEXCEPTION_HANDLER)
		(struct _EXCEPTION_RECORD*, void*, struct _CONTEXT*, void*);
		
typedef struct _EXCEPTION_REGISTRATION {
	struct _EXCEPTION_REGISTRATION*	prev;
	PEXCEPTION_HANDLER		handler;
} EXCEPTION_REGISTRATION, *PEXCEPTION_REGISTRATION;

typedef EXCEPTION_REGISTRATION EXCEPTION_REGISTRATION_RECORD;
typedef PEXCEPTION_REGISTRATION PEXCEPTION_REGISTRATION_RECORD;

#endif
