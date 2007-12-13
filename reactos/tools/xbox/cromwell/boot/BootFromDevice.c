/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"
#include "video.h"
#include "BootFATX.h"
#include "BootFromDevice.h"
#include "IconMenu.h"
#include "LoadLinux.h"
#include "LoadReactOS.h"

static void SelectOS(void (*LinuxBoot)(void *), void (*ReactOSBoot)(void *), void *Context)
{
	PICONMENU Menu;
	PICON iconPtr=0l;
	
	Menu = CreateIconMenu();
	if (NULL == Menu) {
		return;
	}

	iconPtr = (PICON)malloc(sizeof(ICON));
	if (NULL == iconPtr) {
		DestroyIconMenu(Menu);
		return;
	}
	iconPtr->iconSlot = ICON_SOURCE_SLOT5;
	iconPtr->szCaption = "Linux";
	iconPtr->functionPtr = LinuxBoot;
	iconPtr->functionDataPtr = Context;
	AddIcon(Menu, iconPtr, false);

	iconPtr = (PICON)malloc(sizeof(ICON));
	if (NULL == iconPtr) {
		DestroyIconMenu(Menu);
		return;
	}
	iconPtr->iconSlot = ICON_SOURCE_SLOT6;
	iconPtr->szCaption = "ReactOS";
	iconPtr->functionPtr = ReactOSBoot;
	iconPtr->functionDataPtr = Context;
	AddIcon(Menu, iconPtr, false);

	DisplayIconMenu(Menu, true);

	DestroyIconMenu(Menu);
}

static void CallBootLinuxFromCD(void *Context)
{
	BootLinuxFromCD(*((int *) Context));
}

static void CallBootReactOSFromCD(void *Context)
{
	BootReactOSFromCD(*((int *) Context));
}

void BootFromCD(void *data) {
	DWORD dwY=VIDEO_CURSOR_POSY;
	DWORD dwX=VIDEO_CURSOR_POSX;
	int n;
	bool quit;
	bool linuxPresent, reactosPresent;
	int drive = *((int *) data);

	//See if we already have a CDROM in the drive
	//Try for 4 seconds.
	for (n=0;n<16;++n) {
		linuxPresent = LinuxPresentOnCD(drive);
		reactosPresent = ReactOSPresentOnCD(drive);
		if (linuxPresent || reactosPresent) {
			break;
		}
		wait_ms(250);
        }

	if (! linuxPresent && ! reactosPresent) {
	        //Needs to be changed for non-xbox drives, which don't have an eject line
		//Need to send ATA eject command.
		I2CTransmitWord(0x10, 0x0c00); // eject DVD tray
		wait_ms(2000); // Wait for DVD to become responsive to inject command

		VIDEO_ATTR=0xffeeeeff;
		VIDEO_CURSOR_POSX=dwX;
		VIDEO_CURSOR_POSY=dwY;
		printk("Please insert CD and press Button A\n");

		while(1) {
			if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) {
				quit = false;
				break;
			}
			if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_B) == 1) {
				quit = true;
				break;
			}
			USBGetEvents();
			wait_ms(10);
		}
		BootVideoClearScreen(&jpegBackdrop, dwY, VIDEO_CURSOR_POSY + 1);
		I2CTransmitWord(0x10, 0x0c01); // close DVD tray
		wait_ms(500);
		VIDEO_ATTR = 0xffffffff;
		VIDEO_CURSOR_POSY = dwY;
		if (quit) {
			return;
		}

		/* Wait until the media is readable */
		while (1) {
			linuxPresent = LinuxPresentOnCD(drive);
			reactosPresent = ReactOSPresentOnCD(drive);
			if (linuxPresent || reactosPresent) {
				break;
			}
			if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_B) == 1) {
				break;
			}
			USBGetEvents();
			wait_ms(200);
		}
	}
	if (linuxPresent && reactosPresent) {
		SelectOS(CallBootLinuxFromCD, CallBootReactOSFromCD, data);
	} else if (linuxPresent) {
		BootLinuxFromCD(drive);
	} else if (reactosPresent) {
		BootReactOSFromCD(drive);
	}
}

static void CallBootLinuxFromFATX(void *Context)
{
	BootLinuxFromFATX();
}

static void CallBootReactOSFromFATX(void *Context)
{
	BootReactOSFromFATX();
}

void BootFromFATX(void *Context) {
	bool linuxPresent;
	bool reactosPresent;

	linuxPresent = LinuxPresentOnFATX(NULL);
	reactosPresent = ReactOSPresentOnFATX(NULL);
	if (linuxPresent && reactosPresent) {
		SelectOS(CallBootLinuxFromFATX, CallBootReactOSFromFATX, NULL);
	} else if (linuxPresent) {
		BootLinuxFromFATX();
	} else if (reactosPresent) {
		BootReactOSFromFATX();
	}
}

static void CallBootLinuxFromNative(void *Context)
{
	BootLinuxFromNative(*((int *) Context));
}

static void CallBootReactOSFromNative(void *Context)
{
	BootReactOSFromNative(*((int *) Context));
}

void BootFromNative(void *Context) {
	int partitionId = *((int *) Context);
	bool linuxPresent;
	bool reactosPresent;

	linuxPresent = LinuxPresentOnNative(partitionId);
	reactosPresent = ReactOSPresentOnNative(partitionId);
	if (linuxPresent && reactosPresent) {
		SelectOS(CallBootLinuxFromNative, CallBootReactOSFromNative, Context);
	} else if (linuxPresent) {
		BootLinuxFromNative(partitionId);
	} else if (reactosPresent) {
		BootReactOSFromNative(partitionId);
	}
}

#ifdef ETHERBOOT
static void CallBootLinuxFromEtherboot(void *Context)
{
	BootLinuxFromEtherboot();
}

static void CallBootReactOSFromEtherboot(void *Context)
{
	BootReactOSFromEtherboot();
}

void BootFromEtherboot(void *Context) {
	bool linuxPresent;
	bool reactosPresent;

	linuxPresent = LinuxPresentOnEtherboot();
	reactosPresent = ReactOSPresentOnEtherboot();
	if (linuxPresent && reactosPresent) {
		SelectOS(CallBootLinuxFromEtherboot, CallBootReactOSFromEtherboot, NULL);
	} else if (linuxPresent) {
		BootLinuxFromEtherboot();
	} else if (reactosPresent) {
		BootReactOSFromEtherboot();
	}
}
#endif
