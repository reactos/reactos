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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/bootsup.c
 * PURPOSE:         Bootloader support functions
 * PROGRAMMER:      Eric Kohl
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include "usetup.h"
#include "inicache.h"
#include "bootsup.h"


/* FUNCTIONS ****************************************************************/


static VOID
CreateCommonFreeLoaderSections(PINICACHE IniCache)
{
  PINICACHESECTION IniSection;

  /* Create "FREELOADER" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"FREELOADER");

#if 0
MessageLine=Welcome to FreeLoader!
MessageLine=Copyright (c) 2002 by Brian Palmer <brianp@sginet.com>
MessageLine=
MessageBox=Edit your FREELDR.INI file to change your boot settings.
#endif

  /* DefaultOS=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"DefaultOS",
		    L"ReactOS");

  /* Timeout=10 */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TimeOut",
		    L"10");


  /* Create "Display" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"Display");

  /* DisplayMode=NORMAL_VGA */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"DisplayMode",
		    L"NORMAL_VGA");

  /* TitleText=Boot Menu */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TitleText",
		    L"Boot Menu");

  /* StatusBarColor=Cyan */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"StatusBarColor",
		    L"Cyan");

  /* StatusBarTextColor=Black */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"StatusBarTextColor",
		    L"Black");

  /* BackdropTextColor=White */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BackdropTextColor",
		    L"White");

  /* BackdropColor=Blue */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BackdropColor",
		    L"Blue");

  /* BackdropFillStyle=Medium */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BackdropFillStyle",
		    L"Medium");

  /* TitleBoxTextColor=White */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TitleBoxTextColor",
		    L"White");

  /* TitleBoxColor=Red */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TitleBoxColor",
		    L"Red");

  /* MessageBoxTextColor=White */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"MessageBoxTextColor",
		    L"White");

  /* MessageBoxColor=Blue */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"MessageBoxColor",
		    L"Blue");

  /* MenuTextColor=White */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"MenuTextColor",
		    L"White");

  /* MenuColor=Blue */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"MenuColor",
		    L"Blue");

  /* TextColor=Yellow */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"TextColor",
		    L"Yellow");

  /* SelectedTextColor=Black */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SelectedTextColor",
		    L"Black");

  /* SelectedColor=Gray */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SelectedColor",
		    L"Gray");
}


NTSTATUS
CreateFreeLoaderIniForDos(PWCHAR IniPath,
			  PWCHAR SystemPath)
{
  PINICACHE IniCache;
  PINICACHESECTION IniSection;

  IniCache = IniCacheCreate();

  CreateCommonFreeLoaderSections(IniCache);

  /* Create "OperatingSystems" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"OperatingSystems");

  /* REACTOS=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"REACTOS",
		    L"ReactOS");

  /* DOS=Dos/Windows */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"DOS",
		    L"DOS/Windows");

  /* Create "REACTOS" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"REACTOS");

  /* BootType=ReactOS */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootType",
		    L"ReactOS");

  /* SystemPath=multi(0)disk(0)rdisk(0)partition(1)\reactos */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"SystemPath",
		    SystemPath);

  /* Options=/DEBUGPORT=SCREEN */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"Options",
		    L"/DEBUGPORT=SCREEN");


  /* Kernel=\REACTOS\SYSTEM32\NTOSKRNL.EXE */
#if 0
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"Options",
		    L"/DEBUGPORT=SCREEN");
#endif

  /* Hal=\REACTOS\SYSTEM32\HAL.DLL */
#if 0
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"Hal",
		    L"/DEBUGPORT=SCREEN");
#endif

  /* Create "DOS" section */
  IniSection = IniCacheAppendSection(IniCache,
				     L"DOS");

  /* BootType=BootSector */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootType",
		    L"BootSector");

  /* BootSector=BOOTDOS.BIN */
  IniCacheInsertKey(IniSection,
		    NULL,
		    INSERT_LAST,
		    L"BootSector",
		    L"BOOTDOS.BIN");

  IniCacheSave(IniCache, IniPath);
  IniCacheDestroy(IniCache);
}

/* EOF */