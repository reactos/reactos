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
/* $Id: progress.h,v 1.1 2002/12/06 21:39:04 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/partlist.h
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __PROGRESS_H__
#define __PROGRESS_H__


typedef struct _PROGRESS
{
  SHORT Left;
  SHORT Top;
  SHORT Right;
  SHORT Bottom;

  SHORT Width;

  ULONG Percent;
  ULONG Pos;

  ULONG StepCount;
  ULONG CurrentStep;
} PROGRESS, *PPROGRESS;

/* FUNCTIONS ****************************************************************/

PPROGRESS
CreateProgressBar(SHORT Left,
		  SHORT Top,
		  SHORT Right,
		  SHORT Bottom);

VOID
DestroyProgressBar(PPROGRESS Bar);

VOID
ProgressSetStepCount(PPROGRESS Bar,
		     ULONG StepCount);

VOID
ProgressNextStep(PPROGRESS Bar);

#endif /* __PROGRESS_H__ */

/* EOF */