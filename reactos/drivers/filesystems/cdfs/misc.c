/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2004 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/misc.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Eric Kohl
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

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
      if (t[i+1] == 0 && t[i] == ';')
	break;
    }
  if ((i>2)&&(t[i-2] == '.'))
  {
    t[i-2] = 0;
    t[i-1] = 0;
  }
  t[i] = 0;
  t[i+1] = 0;
}


VOID
CdfsDateTimeToSystemTime(PFCB Fcb,
			 PLARGE_INTEGER SystemTime)
{
  TIME_FIELDS TimeFields;
  LARGE_INTEGER LocalTime;

  TimeFields.Milliseconds = 0;
  TimeFields.Second = Fcb->Entry.Second;
  TimeFields.Minute = Fcb->Entry.Minute;
  TimeFields.Hour = Fcb->Entry.Hour;

  TimeFields.Day = Fcb->Entry.Day;
  TimeFields.Month = Fcb->Entry.Month;
  TimeFields.Year = Fcb->Entry.Year + 1900;

  RtlTimeFieldsToTime(&TimeFields,
		      &LocalTime);
  ExLocalTimeToSystemTime(&LocalTime, SystemTime);
}


VOID
CdfsFileFlagsToAttributes(PFCB Fcb,
			  PULONG FileAttributes)
{
  /* FIXME: Fix attributes */

  *FileAttributes = // FILE_ATTRIBUTE_READONLY |
		    ((Fcb->Entry.FileFlags & FILE_FLAG_HIDDEN) ? FILE_ATTRIBUTE_HIDDEN : 0) |
		    ((Fcb->Entry.FileFlags & FILE_FLAG_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY : 0) |
		    ((Fcb->Entry.FileFlags & FILE_FLAG_SYSTEM) ? FILE_ATTRIBUTE_SYSTEM : 0) |
		    ((Fcb->Entry.FileFlags & FILE_FLAG_READONLY) ? FILE_ATTRIBUTE_READONLY : 0);
}

/* EOF */
