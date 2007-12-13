#ifndef _TEXTMENU_H_
#define _TEXTMENU_H_

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
#include "memory_layout.h"
#include <shared.h>
#include "BootFATX.h"
#include "xbox.h"
#include "BootFlash.h"
#include "cpu.h"
#include "BootIde.h"
#include "MenuActions.h"
#include "config.h"

void TextMenuDraw(void);
void TextMenuBack(void);

struct TEXTMENUITEM;
struct TEXTMENU;

typedef struct {
	//Menu item text
	char *szCaption;
	//Pointer to function to run when menu item selected.
	void (*functionPtr) (void *);
	//Pointer data, 0l if none.
	void *functionDataPtr;
	//Child menu, if any, attached to this menu item
	struct TEXTMENU *childMenu;
	//Next / previous menu items within this menu
	struct TEXTMENUITEM *previousMenuItem;
	struct TEXTMENUITEM *nextMenuItem;
} TEXTMENUITEM;

typedef struct {
	//Menu title e.g. "Main Menu"
	char *szCaption;
	//A pointer to the first item of the linked list of menuitems that
	//make up this menu.
	TEXTMENUITEM* firstMenuItem;
	//If 0l, we're a top level menu, otherwise a "BACK" menu item will be created,
	//which takes us back to the parent menu..
	struct TEXTMENU* parentMenu;
} TEXTMENU;

extern TEXTMENU *firstMenu;
extern TEXTMENU *currentMenu;
#endif
