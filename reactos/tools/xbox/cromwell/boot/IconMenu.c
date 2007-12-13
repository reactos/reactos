/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
 * Redesigned icon menu, allowing icons to be added/removed at runtime.
 * 02/10/04 - David Pye dmp@davidmpye.dyndns.org
 * You should not need to edit this file in normal circumstances - icons are
 * added/removed via boot/IconMenuInit.c
 */

#include "boot.h"
#include "video.h"
#include "memory_layout.h"
#include <shared.h>
#include <filesys.h>
#include "rc4.h"
#include "sha1.h"
#include "BootFATX.h"
#include "xbox.h"
#include "BootFlash.h"
#include "cpu.h"
#include "BootIde.h"
#include "MenuActions.h"
#include "config.h"
#include "IconMenu.h"

#define TRANSPARENTNESS 0x30
#define SELECTED 0xff

typedef struct tagICONMENU {
	PICON firstIcon;
	PICON selectedIcon;
	PICON firstVisibleIcon;
} ICONMENU;

PICONMENU CreateIconMenu(void) {
	PICONMENU Menu;

	Menu = (PICONMENU)malloc(sizeof(ICONMENU));
	if (NULL != Menu) {
		Menu->firstIcon = 0l;
		Menu->selectedIcon = 0l;
		Menu->firstVisibleIcon = 0l;
	}

	return Menu;
}

void AddIcon(PICONMENU Menu, PICON newIcon, bool Default) {
	PICON iconPtr = Menu->firstIcon;
	PICON currentIcon = 0l;
	while (iconPtr != 0l) {
		currentIcon = iconPtr;
		iconPtr = iconPtr->nextIcon;
	}
	
	if (currentIcon==0l) { 
		//This is the first icon in the chain
		Menu->firstIcon = newIcon;
	}
	//Append to the end of the chain
	else currentIcon->nextIcon = newIcon;
	iconPtr = newIcon;
	iconPtr->nextIcon = 0l;
	iconPtr->previousIcon = currentIcon; 
	if (Default) {
		Menu->selectedIcon = newIcon;
	}
}

static void IconMenuDraw(PICONMENU Menu, int nXOffset, int nYOffset) {
	PICON iconPtr;
	int iconcount;
	
	if (Menu->firstVisibleIcon==0l) Menu->firstVisibleIcon = Menu->firstIcon;
	if (Menu->selectedIcon==0l) Menu->selectedIcon = Menu->firstIcon;
	iconPtr = Menu->firstVisibleIcon;
	//There are max four 'bays' for displaying icons in - we only draw the four.
	for (iconcount=0; iconcount<4; iconcount++) {
		BYTE opaqueness;
		if (iconPtr==0l) {
			//No more icons to draw
			return;
		}
		if (iconPtr==Menu->selectedIcon) {
			//Selected icon has less transparency
			//and has a caption drawn underneath it
			opaqueness = SELECTED;
			VIDEO_CURSOR_POSX=nXOffset+112*(iconcount+1)*4;
			VIDEO_CURSOR_POSY=nYOffset+20;
			VIDEO_ATTR=0xffffff;
			printk("%s\n",iconPtr->szCaption);
		}
		else opaqueness = TRANSPARENTNESS;
		
		BootVideoJpegBlitBlend(
			(BYTE *)(FB_START+((vmode.width * (nYOffset-74))+nXOffset+(112*(iconcount+1))) * 4),
			vmode.width, // dest bytes per line
			&jpegBackdrop, // source jpeg object
			(BYTE *)(jpegBackdrop.pData+(iconPtr->iconSlot * jpegBackdrop.bpp)),
			0xff00ff|(((DWORD)opaqueness)<<24),
			(BYTE *)(jpegBackdrop.pBackdrop + ((jpegBackdrop.width * (nYOffset-74)) + nXOffset+(112*(iconcount+1))) * jpegBackdrop.bpp),
			ICON_WIDTH, ICON_HEIGHT
		);
		iconPtr = iconPtr->nextIcon;
	}
}

void DisplayIconMenu(PICONMENU Menu, bool AllowExit) {
        unsigned char *videosavepage;
        
        DWORD COUNT_start;
        DWORD temp=1;
	PICON iconPtr=0l;

	extern int nTempCursorMbrX, nTempCursorMbrY; 
	int nTempCursorResumeX, nTempCursorResumeY ;
	int nTempCursorX, nTempCursorY;
	int nModeDependentOffset=(vmode.width-640)/2;  

	
	nTempCursorResumeX=nTempCursorMbrX;
	nTempCursorResumeY=nTempCursorMbrY;

	nTempCursorX=VIDEO_CURSOR_POSX;
	nTempCursorY=vmode.height-80;
	
	// We save the complete framebuffer to memory (we restore at exit)
	videosavepage = malloc(FB_SIZE);
	memcpy(videosavepage,(void*)FB_START,FB_SIZE);
	
	VIDEO_CURSOR_POSX=((252+nModeDependentOffset)<<2);
	VIDEO_CURSOR_POSY=nTempCursorY-100;
	
	VIDEO_ATTR=0xffc8c8c8;
	printk("Select from Menu\n");
	VIDEO_ATTR=0xffffffff;
	
	IconMenuDraw(Menu, nModeDependentOffset, nTempCursorY);
	COUNT_start = IoInputDword(0x8008);
	//Main menu event loop.
	while(1)
	{
		int changed=0;
		USBGetEvents();
		
		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_RIGHT) == 1)
		{
			if (Menu->selectedIcon->nextIcon!=0l) {
				//A bit ugly, but need to find the last visible icon, and see if 
				//we are moving further right from it.
				PICON lastVisibleIcon=Menu->firstVisibleIcon;
				int i=0;
				for (i=0; i<3; i++) {
					if (lastVisibleIcon->nextIcon==0l) break;
					lastVisibleIcon = lastVisibleIcon->nextIcon;
				}
				if (Menu->selectedIcon == lastVisibleIcon) { 
					//We are moving further right, so slide all the icons along. 
					Menu->firstVisibleIcon = Menu->firstVisibleIcon->nextIcon;	
					//As all the icons have moved, we need to refresh the entire page.
					memcpy((void*)FB_START,videosavepage,FB_SIZE);
				}
				Menu->selectedIcon = Menu->selectedIcon->nextIcon;
				changed=1;
			}
			temp=0;
		}
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_LEFT) == 1)
		{
			if (Menu->selectedIcon->previousIcon!=0l) {
				if (Menu->selectedIcon == Menu->firstVisibleIcon) {
					//We are moving further left, so slide all the icons along. 
					Menu->firstVisibleIcon = Menu->selectedIcon->previousIcon;
					//As all the icons have moved, we need to refresh the entire page.
					memcpy((void*)FB_START,videosavepage,FB_SIZE);
				}
				Menu->selectedIcon = Menu->selectedIcon->previousIcon;
				changed=1;
			}
			temp=0;
		}
		//If anybody has toggled the xpad left/right, disable the timeout.
		if (temp!=0) {
			temp = IoInputDword(0x8008) - COUNT_start;
		}
		
		if ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) || (DWORD)(temp>(0x369E99*BOOT_TIMEWAIT))) {
			memcpy((void*)FB_START,videosavepage,FB_SIZE);
			free(videosavepage);
			
			VIDEO_CURSOR_POSX=nTempCursorResumeX;
			VIDEO_CURSOR_POSY=nTempCursorResumeY;
			//Icon selected - invoke function pointer.
			if (Menu->selectedIcon->functionPtr!=0l)
				Menu->selectedIcon->functionPtr(Menu->selectedIcon->functionDataPtr);
			//Shouldn't usually come back but at least if we do, the menu can
			//continue to work.
			//Setting changed means the icon menu will redraw itself.
			changed=1;
			videosavepage = malloc(FB_SIZE);
			memcpy(videosavepage,(void*)FB_START,FB_SIZE);
			temp = 0;
	
		}
		
		if (AllowExit && (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_B) == 1)) {
			memcpy((void*)FB_START,videosavepage,FB_SIZE);
			free(videosavepage);
			
			VIDEO_CURSOR_POSX=nTempCursorResumeX;
			VIDEO_CURSOR_POSY=nTempCursorResumeY;

			return;
		}
		if (changed) {
			BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);
			IconMenuDraw(Menu, nModeDependentOffset, nTempCursorY);
			changed=0;
		}
	}
}

void DestroyIconMenu(PICONMENU Menu) {
	PICON current;
	PICON next;

	current = Menu->firstIcon;
	while (NULL != current) {
		next = current->nextIcon;
		free(current);
		current = next;
	}
	free(Menu);
}
