/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/ex/init.c
 * PURPOSE:         executive initalization
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * PORTABILITY:     Checked.
 * UPDATE HISTORY:
 *                  Created 11/09/99
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* DATA **********************************************************************/

/* FUNCTIONS ****************************************************************/

VOID 
ExInit (VOID)
{
  ExInitTimeZoneInfo();
  ExInitializeWorkerThreads();
  ExpInitLookasideLists();
  ExpWin32kInit();
}


BOOLEAN STDCALL
ExIsProcessorFeaturePresent (IN	ULONG	ProcessorFeature)
{
  if (ProcessorFeature >= 32)
    return FALSE;
  
  return FALSE;
}


VOID STDCALL
ExPostSystemEvent (ULONG	Unknown1,
		   ULONG	Unknown2,
		   ULONG	Unknown3)
{
  /* doesn't do anything */
}

/* EOF */
