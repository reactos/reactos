#ifndef _ICONMENU_H_
#define _ICONMENU_H_

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define ICON_SOURCE_SLOT0 0
#define ICON_SOURCE_SLOT1 ICON_WIDTH
#define ICON_SOURCE_SLOT2 ICON_WIDTH*2
#define ICON_SOURCE_SLOT3 ICON_WIDTH*3
#define ICON_SOURCE_SLOT4 ICON_WIDTH*4
#define ICON_SOURCE_SLOT5 ICON_WIDTH*5
#define ICON_SOURCE_SLOT6 ICON_WIDTH*6
#define ICON_SOURCE_SLOT7 ICON_WIDTH*7
#define ICON_SOURCE_SLOT8 ICON_WIDTH*8

typedef struct tagICON *PICON;
typedef struct tagICON {
	int iconSlot;
	char *szCaption;
	void (*functionPtr) (void *);
	void *functionDataPtr;
	PICON previousIcon;
	PICON nextIcon;
} ICON;

typedef struct tagICONMENU *PICONMENU;

/* Create a new icon menu */
PICONMENU CreateIconMenu(void);

//Adds a new icon into the menu - they are displayed in the order added.
void AddIcon(PICONMENU Menu, PICON Icon, bool Default);

//This draws and handles input for the main menu
void DisplayIconMenu(PICONMENU Menu, bool AllowExit);

/* Destroy an icon menu */
void DestroyIconMenu(PICONMENU Menu);

#endif
