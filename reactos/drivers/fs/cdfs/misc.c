/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: misc.c,v 1.1 2002/05/01 13:15:42 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/misc.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Eric Kohl
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* FUNCTIONS ****************************************************************/


BOOLEAN
wstrcmpjoki(PWSTR s1,
	    PWSTR s2)
/*
 * FUNCTION: Compare two wide character strings, s2 with jokers (* or ?)
 * return TRUE if s1 like s2
 */
{
  while ((*s2=='*')||(*s2=='?')||(towlower(*s1)==towlower(*s2)))
    {
      if ((*s1)==0 && (*s2)==0)
        return(TRUE);

      if(*s2=='*')
	{
	  s2++;
	  while (*s1)
	    if (wstrcmpjoki(s1,s2))
	      return(TRUE);
	    else
	      s1++;
	}
      else
	{
	  s1++;
	  s2++;
	}
    }

  if ((*s2)=='.')
    {
      for (;((*s2)=='.')||((*s2)=='*')||((*s2)=='?');s2++)
	;
    }

  if ((*s1)==0 && (*s2)==0)
    return(TRUE);

  return(FALSE);
}


VOID
CdfsSwapString(PWCHAR Out,
	       PUCHAR In,
	       ULONG Count)
{
  PUCHAR t = (PUCHAR)Out;
  ULONG i;

  for (i = 0; i < Count; i += 2)
    {
      t[i] = In[i+1];
      t[i+1] = In[i];
    }
  t[i] = 0;
  t[i+1] = 0;
}


VOID
CdfsDateTimeToFileTime(PFCB Fcb,
		       TIME *FileTime)
{
  TIME_FIELDS TimeFields;

  TimeFields.Milliseconds = 0;
  TimeFields.Second = Fcb->Entry.Second;
  TimeFields.Minute = Fcb->Entry.Minute;
  TimeFields.Hour = Fcb->Entry.Hour;

  TimeFields.Day = Fcb->Entry.Day;
  TimeFields.Month = Fcb->Entry.Month;
  TimeFields.Year = Fcb->Entry.Year + 1900;

  RtlTimeFieldsToTime(&TimeFields,
		      (PLARGE_INTEGER)FileTime);
}


VOID
CdfsFileFlagsToAttributes(PFCB Fcb,
			  PULONG FileAttributes)
{
  /* FIXME: Fix attributes */

  *FileAttributes = // FILE_ATTRIBUTE_READONLY |
		    (Fcb->Entry.FileFlags & 0x01) ? FILE_ATTRIBUTE_HIDDEN : 0 |
		    (Fcb->Entry.FileFlags & 0x02) ? FILE_ATTRIBUTE_DIRECTORY : 0 |
		    (Fcb->Entry.FileFlags & 0x04) ? FILE_ATTRIBUTE_SYSTEM : 0 |
		    (Fcb->Entry.FileFlags & 0x10) ? FILE_ATTRIBUTE_READONLY : 0;
}

/* EOF */