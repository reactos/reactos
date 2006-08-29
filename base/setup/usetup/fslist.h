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
/* $Id$
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/fslist.h
 * PURPOSE:         Filesystem list functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#ifndef __FSLIST_H__
#define __FSLIST_H__

typedef enum
{
  FsFat = 0,
  FsKeep = 1
} FILE_SYSTEM;

typedef struct _FILE_SYSTEM_LIST
{
  SHORT Left;
  SHORT Top;
  BOOLEAN ForceFormat;
  FILE_SYSTEM CurrentFileSystem;
  ULONG FileSystemCount;
} FILE_SYSTEM_LIST, *PFILE_SYSTEM_LIST;


PFILE_SYSTEM_LIST
CreateFileSystemList (SHORT Left,
		      SHORT Top,
		      BOOLEAN ForceFormat,
		      FILE_SYSTEM ForceFileSystem);

VOID
DestroyFileSystemList (PFILE_SYSTEM_LIST List);

VOID
DrawFileSystemList (PFILE_SYSTEM_LIST List);

VOID
ScrollDownFileSystemList (PFILE_SYSTEM_LIST List);

VOID
ScrollUpFileSystemList (PFILE_SYSTEM_LIST List);

#endif /* __FSLIST_H__ */

/* EOF */
