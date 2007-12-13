/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "TextMenu.h"

void TextMenuDraw(void);
void TextMenuBack(void);

TEXTMENUITEM *firstVisibleMenuItem=0l;
TEXTMENUITEM *selectedMenuItem=0l;
TEXTMENU *firstMenu=0l;
TEXTMENU *currentMenu=0l;
		
unsigned char *textmenusavepage;

void TextMenuAddItem(TEXTMENU *menu, TEXTMENUITEM *newMenuItem) {
	TEXTMENUITEM *menuItem = menu->firstMenuItem;
	TEXTMENUITEM *currentMenuItem=0l;
	
	while (menuItem != 0l) {
		currentMenuItem = menuItem;
		menuItem = (TEXTMENUITEM*)menuItem->nextMenuItem;
	}
	
	if (currentMenuItem==0l) { 
		//This is the first icon in the chain
		menu->firstMenuItem = newMenuItem;
	}
	//Append to the end of the chain
	else currentMenuItem->nextMenuItem = (struct TEXTMENUITEM*)newMenuItem;
	newMenuItem->nextMenuItem = 0l;
	newMenuItem->previousMenuItem = (struct TEXTMENUITEM*)currentMenuItem; 
}

void TextMenuBack(void) {
	currentMenu = (TEXTMENU*)currentMenu->parentMenu;
	selectedMenuItem = currentMenu->firstMenuItem;
	firstVisibleMenuItem = currentMenu->firstMenuItem;
	memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
	TextMenuDraw();
}

void TextMenuDraw(void) {
	TEXTMENUITEM *item=0l;
	int menucount;
	
	VIDEO_CURSOR_POSX=75;
	VIDEO_CURSOR_POSY=125;
	
	if (currentMenu==0l) currentMenu = firstMenu;
	if (selectedMenuItem==0l) selectedMenuItem = currentMenu->firstMenuItem;
	if (firstVisibleMenuItem==0l) firstVisibleMenuItem = currentMenu->firstMenuItem;
	
	//Draw the menu title.
	VIDEO_ATTR=0x000000;
	printk("\2%s",currentMenu->szCaption);
	VIDEO_CURSOR_POSY+=30;
	
	//Draw the menu items
	VIDEO_CURSOR_POSX=150;
	item=firstVisibleMenuItem;
	for (menucount=0; menucount<8; menucount++) {
		if (item==0l) {
			//No more menu items to draw
			return;
		}
		//Selected item in red
		if (item == selectedMenuItem) VIDEO_ATTR=0xff0000;
		else VIDEO_ATTR=0xffffff;
		//Font size 2=big.
		printk("\n\2\t%s\n",item->szCaption);
		item=(TEXTMENUITEM *)item->nextMenuItem;
	}
}

void TextMenu(void) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	
	//Back up the current framebuffer contents
	textmenusavepage = malloc(FB_SIZE);
	memcpy(textmenusavepage,(void*)FB_START,FB_SIZE);

	TextMenuDraw();
	
	//Main menu event loop.
	while(1)
	{
		int changed=0;
		USBGetEvents();
		
		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_UP) == 1)
		{
			if (selectedMenuItem->previousMenuItem!=0l) {
				if (selectedMenuItem == firstVisibleMenuItem) {
					firstVisibleMenuItem = (TEXTMENUITEM *)selectedMenuItem->previousMenuItem;
					memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
				}
				
				selectedMenuItem=(TEXTMENUITEM*)selectedMenuItem->previousMenuItem;
				changed=1;
			}
		} 
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_DOWN) == 1) {
			if (selectedMenuItem->nextMenuItem!=0l) {
				TEXTMENUITEM *lastVisibleMenuItem = firstVisibleMenuItem;
				int i=0;
				//8 menu items per page.
				for (i=0; i<7; i++) {
					if (lastVisibleMenuItem->nextMenuItem==0l) break;
					lastVisibleMenuItem = (TEXTMENUITEM *)lastVisibleMenuItem->nextMenuItem;
				}
				if (selectedMenuItem == lastVisibleMenuItem) {
					firstVisibleMenuItem = (TEXTMENUITEM *)firstVisibleMenuItem->nextMenuItem;
					memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
				}
				selectedMenuItem=(TEXTMENUITEM*)selectedMenuItem->nextMenuItem;
				changed=1;
			}
		}
			
		else if ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1)) {
			//Redraw the page as it was before the menu was displayed.
			memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
			free(textmenusavepage);
			//Menu item selected - invoke function pointer.
			if (selectedMenuItem->functionPtr!=0l) selectedMenuItem->functionPtr(selectedMenuItem->functionDataPtr);
			//Re-malloc these if the function pointer did return us to the menu.	
			textmenusavepage = malloc(FB_SIZE);
			memcpy(textmenusavepage,(void*)FB_START,FB_SIZE);
			//Display the childmenu, if this menu item has one.	
			if (selectedMenuItem->childMenu!=0l) {
				currentMenu = (TEXTMENU*)selectedMenuItem->childMenu;
				selectedMenuItem = currentMenu->firstMenuItem;
				firstVisibleMenuItem = currentMenu->firstMenuItem;
			}
			changed=1;
		}
		else if ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_B) == 1)) {
			//B button takes us back up a menu
			if (currentMenu->parentMenu==0l) {
				//If this is the top level menu, save and quit the text menu.
				memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
				free(textmenusavepage);
				return;
			}
			TextMenuBack();
		}
		
		if (changed) {
			TextMenuDraw();
			changed=0;
		}
	}
}

