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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/fslist.c
 * PURPOSE:         Filesystem list functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

PFILE_SYSTEM_LIST
CreateFileSystemList (SHORT Left,
		      SHORT Top,
		      BOOLEAN ForceFormat,
		      FILE_SYSTEM ForceFileSystem)
{
  PFILE_SYSTEM_LIST List;

  List = (PFILE_SYSTEM_LIST)RtlAllocateHeap (ProcessHeap, 0, sizeof(FILE_SYSTEM_LIST));
  if (List == NULL)
    return NULL;

  List->Left = Left;
  List->Top = Top;

  List->ForceFormat = ForceFormat;
  List->FileSystemCount = 1;
  if (ForceFormat)
    {
      List->CurrentFileSystem = ForceFileSystem;
    }
  else
    {
      List->FileSystemCount++;
      List->CurrentFileSystem = FsKeep;
    }

  return List;
}


VOID
DestroyFileSystemList (PFILE_SYSTEM_LIST List)
{
  RtlFreeHeap (ProcessHeap, 0, List);
}


VOID
DrawFileSystemList (PFILE_SYSTEM_LIST List)
{
  COORD coPos;
  ULONG Written;
  ULONG Index;

  Index = 0;

  coPos.X = List->Left;
  coPos.Y = List->Top + Index;
  FillConsoleOutputAttribute (0x17,
			      50,
			      coPos,
			      &Written);
  FillConsoleOutputCharacter (' ',
			      50,
			      coPos,
			      &Written);

  if (List->CurrentFileSystem == FsFat)
    {
      SetInvertedTextXY (List->Left,
			 List->Top + Index,
			 " Format partition as FAT file system ");
    }
  else
    {
      SetTextXY (List->Left,
		 List->Top + Index,
		 " Format partition as FAT file system ");
    }
  Index++;

  if (List->ForceFormat == FALSE)
    {
      coPos.X = List->Left;
      coPos.Y = List->Top + Index;
      FillConsoleOutputAttribute (0x17,
				  50,
				  coPos,
				  &Written);
      FillConsoleOutputCharacter (' ',
				  50,
				  coPos,
				  &Written);

      if (List->CurrentFileSystem == FsKeep)
	{
	  SetInvertedTextXY (List->Left,
			     List->Top + Index,
			     " Keep current file system (no changes) ");
	}
      else
	{
	  SetTextXY (List->Left,
		     List->Top + Index,
		     " Keep current file system (no changes) ");
	}
    }
}


VOID
ScrollDownFileSystemList (PFILE_SYSTEM_LIST List)
{
  if ((ULONG) List->CurrentFileSystem < List->FileSystemCount - 1)
    {
      (ULONG) List->CurrentFileSystem++;
      DrawFileSystemList (List);
    }
}


VOID
ScrollUpFileSystemList (PFILE_SYSTEM_LIST List)
{
  if ((ULONG) List->CurrentFileSystem > 0)
    {
      (ULONG) List->CurrentFileSystem--;
      DrawFileSystemList (List);
    }
}

/* EOF */
