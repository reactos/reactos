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
#include <ui.h>
#include "tui.h"
#include <rtl.h>
#include <mm.h>
#include <machine.h>
#include <debug.h>
#include <inifile.h>
#include <version.h>
#include <video.h>

ULONG	UiScreenWidth = 80;							// Screen Width
ULONG	UiScreenHeight = 25;							// Screen Height

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
UCHAR	UiEditBoxTextColor			= COLOR_WHITE;			// Edit box text color
UCHAR	UiEditBoxBgColor			= COLOR_BLACK;			// Edit box text background color

UCHAR	UiTitleBoxTitleText[260]	= "Boot Menu";			// Title box's title text

BOOL	UserInterfaceUp				= FALSE;				// Tells us if the user interface is displayed

VIDEODISPLAYMODE	UiDisplayMode		= VideoTextMode;		// Tells us if we are in text or graphics mode

BOOL	UiUseSpecialEffects			= FALSE;				// Tells us if we should use fade effects

UCHAR	UiMonthNames[12][15] = { "January ", "February ", "March ", "April ", "May ", "June ", "July ", "August ", "September ", "October ", "November ", "December " };


BOOL UiInitialize(BOOLEAN ShowGui)
{
	ULONG	SectionId;
	UCHAR	DisplayModeText[260];
	UCHAR	SettingText[260];
	ULONG	Depth;

	if (!ShowGui) {
		if (!TuiInitialize())
		{
			MachVideoSetDisplayMode(NULL, FALSE);
			return FALSE;
		}
		UserInterfaceUp = FALSE;
		return TRUE;
	}
	
	DbgPrint((DPRINT_UI, "Initializing User Interface.\n"));

	DbgPrint((DPRINT_UI, "Reading in UI settings from [Display] section.\n"));

	DisplayModeText[0] = '\0';
	if (IniOpenSection("Display", &SectionId))
	{
		if (! IniReadSettingByName(SectionId, "DisplayMode", DisplayModeText, 260))
		{
			DisplayModeText[0] = '\0';
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
		if (IniReadSettingByName(SectionId, "EditBoxTextColor", SettingText, 260))
		{
			UiEditBoxTextColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "EditBoxColor", SettingText, 260))
		{
			UiEditBoxBgColor = UiTextToColor(SettingText);
		}
		if (IniReadSettingByName(SectionId, "SpecialEffects", SettingText, 260))
		{
			if (stricmp(SettingText, "Yes") == 0 && strlen(SettingText) == 3)
			{
				UiUseSpecialEffects = TRUE;
			}
			else
			{
				UiUseSpecialEffects = FALSE;
			}
		}
	}

	UiDisplayMode = MachVideoSetDisplayMode(DisplayModeText, TRUE);
	MachVideoGetDisplaySize(&UiScreenWidth, &UiScreenHeight, &Depth);


	if (VideoTextMode == UiDisplayMode)
	{
		if (!TuiInitialize())
		{
			MachVideoSetDisplayMode(NULL, FALSE);
			return FALSE;
		}
	}
	else
	{
		UNIMPLEMENTED();
		//if (!GuiInitialize())
		//{
		//	MachSetDisplayMode(NULL, FALSE);
		//	return FALSE;
		//}
	}

	// Draw the backdrop and fade it in if special effects are enabled
	UiFadeInBackdrop();
	
	UserInterfaceUp = TRUE;

	DbgPrint((DPRINT_UI, "UiInitialize() returning TRUE.\n"));

	return TRUE;
}

VOID UiUnInitialize(PUCHAR BootText)
{
	UiDrawBackdrop();
	UiDrawStatusText("Booting...");
	UiInfoBox(BootText);

	if (VideoTextMode == UiDisplayMode)
	{
		TuiUnInitialize();
	}
	else
	{
		UNIMPLEMENTED();
		//GuiUnInitialize();
	}
}

VOID UiDrawBackdrop(VOID)
{
	if (!UserInterfaceUp) return;
	
	if (VideoTextMode == UiDisplayMode)
	{
		TuiDrawBackdrop();
	}
	else
	{
		UNIMPLEMENTED();
		//GuiDrawBackdrop();
	}
}

VOID UiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR FillChar, UCHAR Attr /* Color Attributes */)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiFillArea(Left, Top, Right, Bottom, FillChar, Attr);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiFillArea(Left, Top, Right, Bottom, FillChar, Attr);
	}
}

VOID UiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiDrawShadow(Left, Top, Right, Bottom);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiDrawShadow(Left, Top, Right, Bottom);
	}
}

VOID UiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOL Fill, BOOL Shadow, UCHAR Attr)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiDrawBox(Left, Top, Right, Bottom, VertStyle, HorzStyle, Fill, Shadow, Attr);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiDrawBox(Left, Top, Right, Bottom, VertStyle, HorzStyle, Fill, Shadow, Attr);
	}
}

VOID UiDrawText(ULONG X, ULONG Y, PUCHAR Text, UCHAR Attr)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiDrawText(X, Y, Text, Attr);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiDrawText(X, Y, Text, Attr);
	}
}

VOID UiDrawCenteredText(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PUCHAR TextString, UCHAR Attr)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiDrawCenteredText(Left, Top, Right, Bottom, TextString, Attr);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiDrawCenteredText(Left, Top, Right, Bottom, TextString, Attr);
	}
}

VOID UiDrawStatusText(PUCHAR StatusText)
{
	if (!UserInterfaceUp) return;
	
	if (VideoTextMode == UiDisplayMode)
	{
		TuiDrawStatusText(StatusText);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiDrawStatusText(StatusText);
	}
}

VOID UiUpdateDateTime(VOID)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiUpdateDateTime();
	}
	else
	{
		UNIMPLEMENTED();
		//GuiUpdateDateTime();
	}
}

VOID UiInfoBox(PUCHAR MessageText)
{
	ULONG		TextLength;
	ULONG		BoxWidth;
	ULONG		BoxHeight;
	ULONG		LineBreakCount;
	ULONG		Index;
	ULONG		LastIndex;
	ULONG		Left;
	ULONG		Top;
	ULONG		Right;
	ULONG		Bottom;

	TextLength = strlen(MessageText);

	// Count the new lines and the box width
	LineBreakCount = 0;
	BoxWidth = 0;
	LastIndex = 0;
	for (Index=0; Index<TextLength; Index++)
	{
		if (MessageText[Index] == '\n')
		{
			LastIndex = Index;
			LineBreakCount++;
		}
		else
		{
			if ((Index - LastIndex) > BoxWidth)
			{
				BoxWidth = (Index - LastIndex);
			}
		}
	}

	// Calc the box width & height
	BoxWidth += 6;
	BoxHeight = LineBreakCount + 4;

	// Calc the box coordinates
	Left = (UiScreenWidth / 2) - (BoxWidth / 2);
	Top =(UiScreenHeight / 2) - (BoxHeight / 2);
	Right = (UiScreenWidth / 2) + (BoxWidth / 2);
	Bottom = (UiScreenHeight / 2) + (BoxHeight / 2);

	// Draw the box
	UiDrawBox(Left,
			  Top,
			  Right,
			  Bottom,
			  VERT,
			  HORZ,
			  TRUE,
			  TRUE,
			  ATTR(UiMenuFgColor, UiMenuBgColor)
			  );

	// Draw the text
	UiDrawCenteredText(Left, Top, Right, Bottom, MessageText, ATTR(UiTextColor, UiMenuBgColor));
}

VOID UiMessageBox(PUCHAR MessageText)
{
	// We have not yet displayed the user interface
	// We are probably still reading the .ini file
	// and have encountered an error. Just use printf()
	// and return.
	if (!UserInterfaceUp)
	{
		printf("%s\n", MessageText);
		printf("Press any key\n");
		MachConsGetCh();
		return;
	}

	if (VideoTextMode == UiDisplayMode)
	{
		TuiMessageBox(MessageText);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiMessageBox(MessageText);
	}
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
		MachConsGetCh();
		return;
	}

	if (VideoTextMode == UiDisplayMode)
	{
		TuiMessageBoxCritical(MessageText);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiMessageBoxCritical(MessageText);
	}
}

UCHAR UiTextToColor(PUCHAR ColorText)
{
	if (VideoTextMode == UiDisplayMode)
	{
		return TuiTextToColor(ColorText);
	}
	else
	{
		UNIMPLEMENTED();
		return 0;
		//return GuiTextToColor(ColorText);
	}
}

UCHAR UiTextToFillStyle(PUCHAR FillStyleText)
{
	if (VideoTextMode == UiDisplayMode)
	{
		return TuiTextToFillStyle(FillStyleText);
	}
	else
	{
		UNIMPLEMENTED();
		return 0;
		//return GuiTextToFillStyle(FillStyleText);
	}
}

VOID UiDrawProgressBarCenter(ULONG Position, ULONG Range, PUCHAR ProgressText)
{
	if (!UserInterfaceUp) return;
	
	if (VideoTextMode == UiDisplayMode)
	{
		TuiDrawProgressBarCenter(Position, Range, ProgressText);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiDrawProgressBarCenter(Position, Range, ProgressText);
	}
}

VOID UiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range, PUCHAR ProgressText)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiDrawProgressBar(Left, Top, Right, Bottom, Position, Range, ProgressText);
	}
	else
	{
		UNIMPLEMENTED();
		//GuiDrawProgressBar(Left, Top, Right, Bottom, Position, Range, ProgressText);
	}
}

VOID UiShowMessageBoxesInSection(PUCHAR SectionName)
{
	ULONG		Idx;
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	PUCHAR	MessageBoxText;
	ULONG		MessageBoxTextSize;
	ULONG		SectionId;

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
		IniReadSettingByNumber(SectionId, Idx, SettingName, 79, SettingValue, 79);
		
		if (stricmp(SettingName, "MessageBox") == 0)
		{
			// Get the real length of the MessageBox text
			MessageBoxTextSize = IniGetSectionSettingValueSize(SectionId, Idx);

			//if (MessageBoxTextSize > 0)
			{
				// Allocate enough memory to hold the text
				MessageBoxText = (PUCHAR)MmAllocateMemory(MessageBoxTextSize);

				if (MessageBoxText)
				{
					// Get the MessageBox text
					IniReadSettingByNumber(SectionId, Idx, SettingName, 80, MessageBoxText, MessageBoxTextSize);

					// Fix it up
					UiEscapeString(MessageBoxText);

					// Display it
					UiMessageBox(MessageBoxText);

					// Free the memory
					MmFreeMemory(MessageBoxText);
				}
			}
		}
	}
}

VOID UiEscapeString(PUCHAR String)
{
	ULONG		Idx;

	for (Idx=0; Idx<strlen(String); Idx++)
	{
		// Escape the new line characters
		if (String[Idx] == '\\' && String[Idx+1] == 'n')
		{
			// Escape the character
			String[Idx] = '\n';

			// Move the rest of the string up
			strcpy(&String[Idx+1], &String[Idx+2]);
		}
	}
}

VOID UiTruncateStringEllipsis(PUCHAR StringText, ULONG MaxChars)
{
	if (strlen(StringText) > MaxChars)
	{
		strcpy(&StringText[MaxChars - 3], "...");
	}
}

BOOL UiDisplayMenu(PUCHAR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, ULONG* SelectedMenuItem, BOOL CanEscape, UiMenuKeyPressFilterCallback KeyPressFilter)
{
	if (VideoTextMode == UiDisplayMode)
	{
		return TuiDisplayMenu(MenuItemList, MenuItemCount, DefaultMenuItem, MenuTimeOut, SelectedMenuItem, CanEscape, KeyPressFilter);
	}
	else
	{
		UNIMPLEMENTED();
		return FALSE;
		//return GuiDisplayMenu(MenuItemList, MenuItemCount, DefaultMenuItem, MenuTimeOut, SelectedMenuItem, CanEscape, KeyPressFilter);
	}
}

VOID UiFadeInBackdrop(VOID)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiFadeInBackdrop();
	}
	else
	{
		UNIMPLEMENTED();
		//GuiFadeInBackdrop();
	}
}

VOID UiFadeOut(VOID)
{
	if (VideoTextMode == UiDisplayMode)
	{
		TuiFadeOut();
	}
	else
	{
		UNIMPLEMENTED();
		//GuiFadeInOut();
	}
}

BOOL UiEditBox(PUCHAR MessageText, PUCHAR EditTextBuffer, ULONG Length)
{
	if (VideoTextMode == UiDisplayMode)
	{
		return TuiEditBox(MessageText, EditTextBuffer, Length);
	}
	else
	{
		UNIMPLEMENTED();
		return FALSE;
		//return GuiEditBox(MessageText, EditTextBuffer, Length);
	}
}
