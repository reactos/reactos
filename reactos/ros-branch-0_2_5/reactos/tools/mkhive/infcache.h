/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: infcache.h,v 1.1 2003/04/14 17:18:48 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/infcache.h
 * PURPOSE:         INF file parser that caches contents of INF file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#ifndef __INFCACHE_H__
#define __INFCACHE_H__


#define STATUS_BAD_SECTION_NAME_LINE   (0xC0700001)
#define STATUS_SECTION_NAME_TOO_LONG   (0xC0700002)
#define STATUS_WRONG_INF_STYLE         (0xC0700003)
#define STATUS_NOT_ENOUGH_MEMORY       (0xC0700004)

#define MAX_INF_STRING_LENGTH  512

typedef PVOID HINF, *PHINF;

typedef struct _INFCONTEXT
{
  PVOID Inf;
//  PVOID CurrentInf;
  PVOID Section;
  PVOID Line;
} INFCONTEXT, *PINFCONTEXT;


/* FUNCTIONS ****************************************************************/

BOOL
InfOpenFile (PHINF InfHandle,
	     PCHAR FileName,
	     PULONG ErrorLine);

VOID
InfCloseFile (HINF InfHandle);


BOOL
InfFindFirstLine (HINF InfHandle,
		  PCHAR Section,
		  PCHAR Key,
		  PINFCONTEXT Context);

BOOL
InfFindNextLine (PINFCONTEXT ContextIn,
		 PINFCONTEXT ContextOut);

BOOL
InfFindFirstMatchLine (PINFCONTEXT ContextIn,
		       PCHAR Key,
		       PINFCONTEXT ContextOut);

BOOL
InfFindNextMatchLine (PINFCONTEXT ContextIn,
		      PCHAR Key,
		      PINFCONTEXT ContextOut);


LONG
InfGetLineCount (HINF InfHandle,
		 PCHAR Section);

LONG
InfGetFieldCount (PINFCONTEXT Context);


BOOL
InfGetBinaryField (PINFCONTEXT Context,
		   ULONG FieldIndex,
		   PUCHAR ReturnBuffer,
		   ULONG ReturnBufferSize,
		   PULONG RequiredSize);

BOOL
InfGetIntField (PINFCONTEXT Context,
		ULONG FieldIndex,
		PLONG IntegerValue);

BOOL
InfGetMultiSzField (PINFCONTEXT Context,
		    ULONG FieldIndex,
		    PCHAR ReturnBuffer,
		    ULONG ReturnBufferSize,
		    PULONG RequiredSize);

BOOL
InfGetStringField (PINFCONTEXT Context,
		   ULONG FieldIndex,
		   PCHAR ReturnBuffer,
		   ULONG ReturnBufferSize,
		   PULONG RequiredSize);



BOOL
InfGetData (PINFCONTEXT Context,
	    PCHAR *Key,
	    PCHAR *Data);

BOOL
InfGetDataField (PINFCONTEXT Context,
		 ULONG FieldIndex,
		 PCHAR *Data);

#endif /* __INFCACHE_H__ */

/* EOF */
