/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"
#include "BootFATX.h"
#include "BootIde.h"
#include "BootFromDevice.h"
#include "IconMenu.h"
#include "LoadLinux.h"
#include "LoadReactOS.h"
#include "MenuActions.h"

#ifdef FLASH
void FlashBios(void *data) {
        BootLoadFlashCD();
}
#endif

void MoveToTextMenu(void *nothing) {
	TextMenuInit();
	TextMenu();
}

void SetLEDColor(void *color) {
	I2cSetFrontpanelLed(*(BYTE*)color);
}

static void InitCDIcons(PICONMENU Menu) {
	int i=0;
	PICON iconPtr=0l;
	for (i=0; i<2; ++i) {
		//Add the cdrom icon - if you have two cdroms, you'll get two icons!
		if (tsaHarddiskInfo[i].m_fAtapi) {
			char *driveName=malloc(sizeof(char)*14);
			sprintf(driveName,"CD-ROM (hd%c)",i ? 'b':'a');
			iconPtr = (PICON)malloc(sizeof(ICON));
			iconPtr->iconSlot = ICON_SOURCE_SLOT2;
			iconPtr->szCaption = driveName;
			iconPtr->functionPtr = BootFromCD;
			iconPtr->functionDataPtr = malloc(sizeof(int));
			*(int*)iconPtr->functionDataPtr = i;
			AddIcon(Menu, iconPtr, false);
		}
	}
}

static void InitFatXIcons(PICONMENU Menu) {
	PICON iconPtr=0l;
	BYTE ba[512];
	memset(ba,0x00,512);
	BootIdeReadSector(0, &ba[0], 3, 0, 512);
	if (!strncmp("BRFR",&ba[0],4)) {
		//Got a FATX formatted HDD
		FATXPartition *partition = NULL;

		partition = OpenFATXPartition(0,SECTOR_STORE,STORE_SIZE);

		if (NULL != partition) {
			if (LinuxPresentOnFATX(partition) ||
			    ReactOSPresentOnFATX(partition)) {
				//We can load the config, so it's bootable.
				iconPtr = (PICON)malloc(sizeof(ICON));
		   		iconPtr->iconSlot = ICON_SOURCE_SLOT4;
		   		iconPtr->szCaption = "FatX (E:)";
		   		iconPtr->functionPtr = BootFromFATX;
				iconPtr->functionDataPtr = NULL;
				//If we have fatx, mark it as default.
				//If there are natives, they'll take over shortly
		   		AddIcon(Menu, iconPtr, true);
			}
			CloseFATXPartition(partition);
		}
	}
}

static void InitNativeIcons(PICONMENU Menu) {
	PICON iconPtr=0l;
	BYTE ba[512];
	memset(ba,0x00,512);

	//This needs enhancing to check multiple HDDs, and support multiple
	//boot entries.
	BootIdeReadSector(0, &ba[0], 0, 0, 512);
	        
	//Is there an MBR here?
	if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) {
		volatile BYTE * pb;
		int n=0, nPos=0;
		(volatile BYTE *)pb=&ba[0x1be];
		//Check the first four partitions (this isn't good enough!)
		for (n=0; n<4; n++,pb+=16) {
			//Is this partition bootable?
			if(pb[0]&0x80) {
				if (LinuxPresentOnNative(n) ||
				    ReactOSPresentOnNative(n)) {
					//Got a partition with an OS on - lets add an icon for it
					iconPtr = (PICON)malloc(sizeof(ICON));
					iconPtr->iconSlot = ICON_SOURCE_SLOT1;
					iconPtr->szCaption = "HDD";
					iconPtr->functionPtr = BootFromNative;
					iconPtr->functionDataPtr=malloc(sizeof(int));
					*(int*)(iconPtr->functionDataPtr)=n;
					//At the moment, the *LAST* native partition on disk will
					//be the one selected as bootable by default.
					AddIcon(Menu, iconPtr, true);
				}
			}
		}
	}
}

#ifdef ETHERBOOT
static void InitEtherbootIcons(PICONMENU Menu) {
	PICON iconPtr=0l;
	
	if (LinuxPresentOnEtherboot() ||
	    ReactOSPresentOnEtherboot()) {
		iconPtr = (PICON)malloc(sizeof(ICON));
		iconPtr->iconSlot = ICON_SOURCE_SLOT3;
		iconPtr->szCaption = "Etherboot";
		iconPtr->functionPtr = BootFromEtherboot;
		AddIcon(Menu, iconPtr, false);
	}
}
#endif	

#ifdef FLASH
static void InitFlashIcons(PICONMENU Menu) {
	PICON iconPtr=0l;

	iconPtr = (PICON)malloc(sizeof(ICON));
	iconPtr->iconSlot = ICON_SOURCE_SLOT0;
	iconPtr->szCaption = "Flash Bios";
	iconPtr->functionPtr = FlashBios;
	AddIcon(Menu, iconPtr, false);
}
#endif

/* Uncomment this one to test the new text menu system.
static void InitTextMenuIcons(PICONMENU Menu) {
	PICON iconPtr=0l;

	iconPtr = (PICON)malloc(sizeof(ICON));
	iconPtr->iconSlot = ICON_SOURCE_SLOT0;
	iconPtr->szCaption = "Advanced";
	iconPtr->functionPtr = MoveToTextMenu;
	AddIcon(Menu, iconPtr, false);
}
*/

static void MainMenuInit(PICONMENU Menu) {
	int i=0;
	PICON iconPtr=0l;

	InitCDIcons(Menu);
	InitFatXIcons(Menu);
	InitNativeIcons(Menu);
#ifdef ETHERBOOT
	InitEtherbootIcons(Menu);
#endif	
#ifdef FLASH
	InitFlashIcons(Menu);
#endif
	/* Uncomment this one to test the new text menu system.
	InitTextMenuIcons(Menu);
	*/
}

void MainMenu(void) {
	PICONMENU Menu;

	Menu = CreateIconMenu();
	MainMenuInit(Menu);
	DisplayIconMenu(Menu, false);
	DestroyIconMenu(Menu);
}
