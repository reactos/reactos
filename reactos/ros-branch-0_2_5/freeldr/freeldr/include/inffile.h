/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003 ReactOS Team
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
/* $Id: inffile.h,v 1.1 2003/05/25 21:17:30 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/infcache.h
 * PURPOSE:         INF file parser that caches contents of INF file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 */

#ifndef __INFCACHE_H__
#define __INFCACHE_H__


#define STATUS_BAD_SECTION_NAME_LINE   (0xC0700001)
#define STATUS_SECTION_NAME_TOO_LONG   (0xC0700002)
#define STATUS_WRONG_INF_STYLE         (0xC0700003)
#define STATUS_NOT_ENOUGH_MEMORY       (0xC0700004)

#define MAX_INF_STRING_LENGTH  512

typedef PU32 HINF, *PHINF;

typedef struct _INFCONTEXT
{
  PVOID Inf;
//  PVOID CurrentInf;
  PVOID Section;
  PVOID Line;
} INFCONTEXT, *PINFCONTEXT;


/* FUNCTIONS ****************************************************************/

BOOLEAN
InfOpenFile (PHINF InfHandle,
	     PCHAR FileName,
	     PU32 ErrorLine);

VOID
InfCloseFile (HINF InfHandle);


BOOLEAN
InfFindFirstLine (HINF InfHandle,
		  PCHAR Section,
		  PCHAR Key,
		  PINFCONTEXT Context);

BOOLEAN
InfFindNextLine (PINFCONTEXT ContextIn,
		 PINFCONTEXT ContextOut);

BOOLEAN
InfFindFirstMatchLine (PINFCONTEXT ContextIn,
		       PCHAR Key,
		       PINFCONTEXT ContextOut);

BOOLEAN
InfFindNextMatchLine (PINFCONTEXT ContextIn,
		      PCHAR Key,
		      PINFCONTEXT ContextOut);


S32
InfGetLineCount (HINF InfHandle,
		 PCHAR Section);

S32
InfGetFieldCount (PINFCONTEXT Context);


BOOLEAN
InfGetBinaryField (PINFCONTEXT Context,
		   U32 FieldIndex,
		   PU8 ReturnBuffer,
		   U32 ReturnBufferSize,
		   PU32 RequiredSize);

BOOLEAN
InfGetIntField (PINFCONTEXT Context,
		U32 FieldIndex,
		S32 *IntegerValue);

BOOLEAN
InfGetMultiSzField (PINFCONTEXT Context,
		    U32 FieldIndex,
		    PCHAR ReturnBuffer,
		    U32 ReturnBufferSize,
		    PU32 RequiredSize);

BOOLEAN
InfGetStringField (PINFCONTEXT Context,
		   U32 FieldIndex,
		   PCHAR ReturnBuffer,
		   U32 ReturnBufferSize,
		   PU32 RequiredSize);



BOOLEAN
InfGetData (PINFCONTEXT Context,
	    PCHAR *Key,
	    PCHAR *Data);

BOOLEAN
InfGetDataField (PINFCONTEXT Context,
		 U32 FieldIndex,
		 PCHAR *Data);

#endif /* __INFCACHE_H__ */

/* EOF */
