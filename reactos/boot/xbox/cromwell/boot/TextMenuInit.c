/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "include/boot.h"
#include "TextMenu.h"

void TextMenuInit(void) {
	
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	
	//Create the root menu - MANDATORY
	firstMenu = malloc(sizeof(TEXTMENU));
	firstMenu->szCaption="Main Menu\n";
	firstMenu->parentMenu=0l;
	firstMenu->firstMenuItem=0l;
	
	//Add the first Item
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="HDD Tools";	
	TextMenuAddItem(firstMenu, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Bios Flashing";	
	TextMenuAddItem(firstMenu, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="LED Colors";	
	TextMenuAddItem(firstMenu, itemPtr);

	//Child menu
	menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
	memset(menuPtr,0x00,sizeof(TEXTMENU));
	menuPtr->szCaption="LED colors menu";
	menuPtr->parentMenu=(struct TEXTMENU*)firstMenu;
	//itemptr here points to the "LED Colors" menuitem of the parentmenu, so this child menu is
	//attached to that.
	itemPtr->childMenu = (struct TEXTMENU*)menuPtr;

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Red LED";	
	itemPtr->functionPtr=SetLEDColor;
        itemPtr->functionDataPtr = malloc(sizeof(BYTE));
	*(BYTE*)(itemPtr->functionDataPtr) = (I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );
	
	TextMenuAddItem(menuPtr, itemPtr);
	
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Orange LED";	
	itemPtr->functionPtr=SetLEDColor;
        itemPtr->functionDataPtr = malloc(sizeof(BYTE));
	*(BYTE*)(itemPtr->functionDataPtr) = (
			I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
			I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
	);
	TextMenuAddItem(menuPtr, itemPtr);
	
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Green LED";
	itemPtr->functionPtr=SetLEDColor;
        itemPtr->functionDataPtr = malloc(sizeof(BYTE));
	*(BYTE*)(itemPtr->functionDataPtr) = (I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 );
	TextMenuAddItem(menuPtr, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Flashing LED";
	itemPtr->functionPtr=SetLEDColor;
        itemPtr->functionDataPtr = malloc(sizeof(BYTE));
	*(BYTE*)(itemPtr->functionDataPtr) = (I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_RED1 | I2C_LED_RED2);
	TextMenuAddItem(menuPtr, itemPtr);
}
