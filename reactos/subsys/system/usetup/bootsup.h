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
/* $Id: bootsup.h,v 1.3 2003/01/30 14:41:45 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/bootsup.h
 * PURPOSE:         Bootloader support functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __BOOTSUP_H__
#define __BOOTSUP_H__

NTSTATUS
CreateFreeLoaderIniForDos(PWCHAR IniPath,
			  PWCHAR ArcPath);

NTSTATUS
CreateFreeLoaderIniForReactos(PWCHAR IniPath,
			      PWCHAR ArcPath);

NTSTATUS
UpdateFreeLoaderIni(PWCHAR IniPath,
		    PWCHAR ArcPath);

NTSTATUS
SaveCurrentBootSector(PWSTR RootPath,
		      PWSTR DstPath);

NTSTATUS
InstallFat16BootCodeToFile(PWSTR SrcPath,
			   PWSTR DstPath,
			   PWSTR RootPath);

NTSTATUS
InstallFat32BootCodeToFile(PWSTR SrcPath,
			   PWSTR DstPath,
			   PWSTR RootPath);

NTSTATUS
InstallFat16BootCodeToDisk(PWSTR SrcPath,
			   PWSTR RootPath);

NTSTATUS
InstallFat32BootCodeToDisk(PWSTR SrcPath,
			   PWSTR RootPath);


NTSTATUS
UpdateBootIni(PWSTR BootIniPath,
	      PWSTR EntryName,
	      PWSTR EntryValue);

#endif /* __BOOTSUP_H__ */

/* EOF */
