/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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
	
#include <freeldr.h>
#include <rtl.h>
#include <ui.h>
#include <options.h>
#include <miscboot.h>
#include <debug.h>
#include <disk.h>
#include <arch.h>


PUCHAR	OptionsMenuList[] =
{
	"Safe Mode",
	"Safe Mode with Networking",
	"Safe Mode with Command Prompt",

	"SEPARATOR",

	"Enable Boot Logging",
	"Enable VGA Mode",
	"Last Known Good Configuration",
	"Directory Services Restore Mode",
	"Debugging Mode",

	"SEPARATOR",

	"Custom Boot",
	"Reboot",
};

enum OptionMenuItems
{
	SAFE_MODE = 0,
	SAFE_MODE_WITH_NETWORKING = 1,
	SAFE_MODE_WITH_COMMAND_PROMPT = 2,

	SEPARATOR1 = 3,

	ENABLE_BOOT_LOGGING = 4,
	ENABLE_VGA_MODE = 5,
	LAST_KNOWN_GOOD_CONFIGURATION = 6,
	DIRECTORY_SERVICES_RESTORE_MODE = 7,
	DEBUGGING_MODE = 8,

	SEPARATOR2 = 9,

	CUSTOM_BOOT = 10,
	REBOOT = 11,
};

U32		OptionsMenuItemCount = sizeof(OptionsMenuList) / sizeof(OptionsMenuList[0]);

VOID DoOptionsMenu(VOID)
{
	U32		SelectedMenuItem;

	if (!UiDisplayMenu(OptionsMenuList, OptionsMenuItemCount, 0, -1, &SelectedMenuItem, TRUE, NULL))
	{
		// The user pressed ESC
		return;
	}

	// Clear the backdrop
	UiDrawBackdrop();

	switch (SelectedMenuItem)
	{
	case SAFE_MODE:
		break;
	case SAFE_MODE_WITH_NETWORKING:
		break;
	case SAFE_MODE_WITH_COMMAND_PROMPT:
		break;
	//case SEPARATOR1:
	//	break;
	case ENABLE_BOOT_LOGGING:
		break;
	case ENABLE_VGA_MODE:
		break;
	case LAST_KNOWN_GOOD_CONFIGURATION:
		break;
	case DIRECTORY_SERVICES_RESTORE_MODE:
		break;
	case DEBUGGING_MODE:
		break;
	//case SEPARATOR2:
	//	break;
	case CUSTOM_BOOT:
		OptionMenuCustomBoot();
		break;
	case REBOOT:
		OptionMenuReboot();
		break;
	}
}

VOID OptionMenuReboot(VOID)
{
	UiMessageBox("The system will now reboot.");

#ifdef __i386__
	DiskStopFloppyMotor();
	SoftReboot();
#else
	UNIMPLEMENTED();
#endif
}
