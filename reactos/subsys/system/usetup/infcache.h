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
/* $Id: infcache.h,v 1.2 2003/03/13 17:58:52 ekohl Exp $
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


typedef PVOID HINF, *PHINF;

typedef struct _INFCONTEXT
{
  PVOID Inf;
//  PVOID CurrentInf;
  PVOID Section;
  PVOID Line;
} INFCONTEXT, *PINFCONTEXT;


/* FUNCTIONS ****************************************************************/

NTSTATUS
InfOpenFile(PHINF InfHandle,
	    PUNICODE_STRING FileName,
	    PULONG ErrorLine);

VOID
InfCloseFile(HINF InfHandle);


BOOLEAN
InfFindFirstLine (HINF InfHandle,
		  PCWSTR Section,
		  PCWSTR Key,
		  PINFCONTEXT Context);

BOOLEAN
InfFindNextLine (PINFCONTEXT ContextIn,
		 PINFCONTEXT ContextOut);


LONG
InfGetLineCount(HINF InfHandle,
		PCWSTR Section);


BOOLEAN
InfGetData (PINFCONTEXT Context,
	    PWCHAR *Key,
	    PWCHAR *Data);

#endif /* __INFCACHE_H__ */

/* EOF */
