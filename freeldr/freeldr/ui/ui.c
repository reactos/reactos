/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
#include <ui.h>
#include "tui.h"
#include <rtl.h>
#include <mm.h>
#include <debug.h>
#include <inifile.h>
#include <version.h>
#include <video.h>


#define DISPLAYMODE_TEXT		0
#define DISPLAYMODE_GRAPHICS	1

ULONG	UiScreenWidth = 80;									// Screen Width
ULONG	UiScreenHeight = 25;								// Screen Height

UCHAR	UiStatusBarFgColor			= COLOR_BLACK;			// Status bar foreground color
UCHAR	UiStatusBarBgColor			= COLOR_CYAN;			// Status bar background color
UCHAR	UiBackdropFgColor			= COLOR_WHITE;			// Backdrop foreground color
UCHAR	UiBackdropBgColor			= COLOR_BLUE;			// Backdrop background color
UCHAR	UiBackdropFillStyle			= MEDIUM_FILL;			// Backdrop fill style
UCHAR	UiTitleBoxFgColor			= COLOR_WHITE;			// Title box foreground color
UCHAR	UiTitleBoxBgColor			= COLOR_RED;			// Title box background color
UCHAR	UiMessageBoxFgColor			= COLOR_WHITE;			// Message box foreground color
UCHAR	UiMessageBoxBgColor			= COLOR_BLUE;			// Message box background color
UCHAR	UiMenuFgColor				= COLOR_WHITE;			// Menu foreground color
UCHAR	UiMenuBgColor				= COLOR_BLUE;			// Menu background color
UCHAR	UiTextColor					= COLOR_YELLOW;			// Normal text color
UCHAR	UiSelectedTextColor			= COLOR_BLACK;			// Selected text color
UCHAR	UiSelectedTextBgColor		= COLOR_GRAY;			// Selected text background color
UCHAR	UiTitleBoxTitleText[260]	= "Boot Menu";			// Title box's title text

PUCHAR	UiMessageBoxLineText		= NULL;
#define UIMESSAGEBOXLINETEXTSIZE	4096

BOOL	UserInterfaceUp				= FALSE;				// Tells us if the user interface is displayed

BOOL	UiDisplayMode				= DISPLAYMODE_TEXT;		// Tells us if we are in text or graphics mode

UCHAR	UiMonthNames[12][15] = { "January ", "February ", "March ", "April ", "May ", "June ", "July ", "August ", "September ", "October ", "November ", "December " };


BOOL UiInitialize(VOID)
{
	ULONG	SectionId;
	UCHAR	SettingText[260];
	ULONG	VideoMode = VIDEOMODE_NORMAL_TEXT;

	DbgPrint((DPRINT_UI, "Initializing User Interface.\n"));
	
	UiMessageBoxLineText = MmAllocateMemory(UIMESSAGEBOXLINETEXTSIZE);
	
	if (UiMessageBoxLineText == NULL)
	{
		return FALSE;
	}

	RtlZeroMemory(UiMessageBoxLineText, UIMESSAGEBOXLINETEXTSIZE);

	DbgPrint((DPRINT_UI, "Reading in UI settings from [Display] section.\n"));

	if (IniOpenSection("Display", &SectionId))
	{
		if (IniReadSettingByName(SectionId, "DisplayMode", SettingText, 260))
		{
			if (BiosDetectVideoCard() == VIDEOCARD_CGA_OR_OTHER)
			{
				DbgPrint((DPRINT_UI, "CGA or other display adapter detected.\n"));
			}
			else if (BiosDetectVideoCard() == VIDEOCARD_EGA)
			{
				DbgPrint((DPRINT_UI, "EGA display adapter detected.\n"));
			}
			else if (BiosDetectVideoCard() == VIDEOCARD_VGA)
			{
				DbgPrint((DPRINT_UI, "VGA display adapter detected.\n"));
			}

			if (stricmp(SettingText, "NORMAL_VGA") == 0)
			{
				VideoMode = VIDEOMODE_NORMAL_TEXT;
			}
			else if (stricmp(SettingText, "EXTENDED_VGA") == 0)
			{
				VideoMode = VIDEOMODE_EXTENDED_TEXT;
			}
			else
			{
				VideoMode = atoi(SettingText);
			}

			if (!VideoSetMode(VideoMode))
			{
				printf("Error: unable to set video display mode 0x%x\n", VideoMode);
				printf("Press any key to continue.\n");
				getch();
			}

			UiScreenWidth = VideoGetCurrentModeResolutionX();
			UiScreenHeight = VideoGetCurrentModeResolutionY();
		}
		if (IniReadSettingByName(SectionId, "TitleText", SettingText, 260))
		{
			strcpy(UiTitleBoxTitleText, SettingText);
		}
		if (IniReadSettingByName(SectionId, "StatusBarColor", SettingText, 260))
		{
			UiStatusBarBgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "StatusBarTextColor", SettingText, 260))
		{
			UiStatusBarFgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "BackdropTextColor", SettingText, 260))
		{
			UiBackdropFgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "BackdropColor", SettingText, 260))
		{
			UiBackdropBgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "BackdropFillStyle", SettingText, 260))
		{
			UiBackdropFillStyle = UiTextToFillStyle(SettingText);
		}
		if (IniReadSettingByName(SectionId, "TitleBoxTextColor", SettingText, 260))
		{
			UiTitleBoxFgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "TitleBoxColor", SettingText, 260))
		{
			UiTitleBoxBgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "MessageBoxTextColor", SettingText, 260))
		{
			UiMessageBoxFgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "MessageBoxColor", SettingText, 260))
		{
			UiMessageBoxBgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "MenuTextColor", SettingText, 260))
		{
			UiMenuFgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "MenuColor", SettingText, 260))
		{
			UiMenuBgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "TextColor", SettingText, 260))
		{
			UiTextColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "SelectedTextColor", SettingText, 260))
		{
			UiSelectedTextColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "SelectedColor", SettingText, 260))
		{
			UiSelectedTextBgColor = UiTextToColor(SettingText);
		}
	}

	VideoClearScreen();
	VideoHideTextCursor();

	// Draw the backdrop and title box
	UiDrawBackdrop();
	
	UserInterfaceUp = TRUE;

	DbgPrint((DPRINT_UI, "UiInitialize() returning TRUE.\n"));

	return TRUE;
}

VOID UiDrawBackdrop(VOID)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiDrawBackdrop();
	}
	else
	{
		//GuiDrawBackdrop();
	}
}

VOID UiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR FillChar, UCHAR Attr /* Color Attributes */)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiFillArea(Left, Top, Right, Bottom, FillChar, Attr);
	}
	else
	{
		//GuiFillArea(Left, Top, Right, Bottom, FillChar, Attr);
	}
}

VOID UiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiDrawShadow(Left, Top, Right, Bottom);
	}
	else
	{
		//GuiDrawShadow(Left, Top, Right, Bottom);
	}
}

VOID UiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOL Fill, BOOL Shadow, UCHAR Attr)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiDrawBox(Left, Top, Right, Bottom, VertStyle, HorzStyle, Fill, Shadow, Attr);
	}
	else
	{
		//GuiDrawBox(Left, Top, Right, Bottom, VertStyle, HorzStyle, Fill, Shadow, Attr);
	}
}

VOID UiDrawText(ULONG X, ULONG Y, PUCHAR Text, UCHAR Attr)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiDrawText(X, Y, Text, Attr);
	}
	else
	{
		//GuiDrawText(X, Y, Text, Attr);
	}
}

VOID UiDrawCenteredText(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PUCHAR TextString, UCHAR Attr)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiDrawCenteredText(Left, Top, Right, Bottom, TextString, Attr);
	}
	else
	{
		//GuiDrawCenteredText(Left, Top, Right, Bottom, TextString, Attr);
	}
}

VOID UiDrawStatusText(PUCHAR StatusText)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiDrawStatusText(StatusText);
	}
	else
	{
		//GuiDrawStatusText(StatusText);
	}
}

VOID UiUpdateDateTime(VOID)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiUpdateDateTime();
	}
	else
	{
		//TuiUpdateDateTime();
	}
}

VOID UiMessageBox(PUCHAR MessageText)
{
	
	strcat(UiMessageBoxLineText, MessageText);

	// We have not yet displayed the user interface
	// We are probably still reading the .ini file
	// and have encountered an error. Just use printf()
	// and return.
	if (!UserInterfaceUp)
	{
		printf("%s\n", UiMessageBoxLineText);
		printf("Press any key\n");
		getch();
		return;
	}

	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiMessageBox(UiMessageBoxLineText);
	}
	else
	{
		//GuiMessageBox(UiMessageBoxLineText);
	}

	RtlZeroMemory(UiMessageBoxLineText, UIMESSAGEBOXLINETEXTSIZE);
}

VOID UiMessageBoxCritical(PUCHAR MessageText)
{
	// We have not yet displayed the user interface
	// We are probably still reading the .ini file
	// and have encountered an error. Just use printf()
	// and return.
	if (!UserInterfaceUp)
	{
		printf("%s\n", MessageText);
		printf("Press any key\n");
		getch();
		return;
	}

	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiMessageBoxCritical(MessageText);
	}
	else
	{
		//GuiMessageBoxCritical(MessageText);
	}
}

VOID UiMessageLine(PUCHAR MessageText)
{
	strcat(UiMessageBoxLineText, MessageText);
	strcat(UiMessageBoxLineText, "\n");
}

UCHAR UiTextToColor(PUCHAR ColorText)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		return TuiTextToColor(ColorText);
	}
	else
	{
		//return GuiTextToColor(ColorText);
	}
}

UCHAR UiTextToFillStyle(PUCHAR FillStyleText)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		return TuiTextToFillStyle(FillStyleText);
	}
	else
	{
		//return GuiTextToFillStyle(FillStyleText);
	}
}

VOID UiDrawProgressBarCenter(ULONG Position, ULONG Range)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiDrawProgressBarCenter(Position, Range);
	}
	else
	{
		//GuiDrawProgressBarCenter(Position, Range);
	}
}

VOID UiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		TuiDrawProgressBar(Left, Top, Right, Bottom, Position, Range);
	}
	else
	{
		//GuiDrawProgressBar(Left, Top, Right, Bottom, Position, Range);
	}
}

VOID UiShowMessageBoxesInSection(PUCHAR SectionName)
{
	ULONG	Idx;
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	ULONG	SectionId;

	//
	// Zero out message line text
	//
	strcpy(UiMessageBoxLineText, "");

	if (!IniOpenSection(SectionName, &SectionId))
	{
		sprintf(SettingName, "Section %s not found in freeldr.ini.\n", SectionName);
		UiMessageBox(SettingName);
		return;
	}

	//
	// Find all the message box settings and run them
	//
	for (Idx=0; Idx<IniGetNumSectionItems(SectionId); Idx++)
	{
		IniReadSettingByNumber(SectionId, Idx, SettingName, 80, SettingValue, 80);
		
		if (stricmp(SettingName, "MessageBox") == 0)
		{
			UiMessageBox(SettingValue);
		}
		else if (stricmp(SettingName, "MessageLine") == 0)
		{
			UiMessageLine(SettingValue);
		}
	}

	//
	// Zero out message line text
	//
	strcpy(UiMessageBoxLineText, "");
}

VOID UiTruncateStringEllipsis(PUCHAR StringText, ULONG MaxChars)
{
}

BOOL UiDisplayMenu(PUCHAR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, PULONG SelectedMenuItem)
{
	if (UiDisplayMode == DISPLAYMODE_TEXT)
	{
		return TuiDisplayMenu(MenuItemList, MenuItemCount, DefaultMenuItem, MenuTimeOut, SelectedMenuItem);
	}
	else
	{
		//return GuiDisplayMenu(MenuItemList, MenuItemCount, DefaultMenuItem, MenuTimeOut, SelectedMenuItem);
	}
}
