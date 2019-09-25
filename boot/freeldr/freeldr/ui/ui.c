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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _M_ARM

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

#define TAG_UI_TEXT 'xTiU'

ULONG UiScreenWidth;    // Screen Width
ULONG UiScreenHeight;   // Screen Height

UCHAR UiStatusBarFgColor    = COLOR_BLACK;  // Status bar foreground color
UCHAR UiStatusBarBgColor    = COLOR_CYAN;   // Status bar background color
UCHAR UiBackdropFgColor     = COLOR_WHITE;  // Backdrop foreground color
UCHAR UiBackdropBgColor     = COLOR_BLUE;   // Backdrop background color
UCHAR UiBackdropFillStyle   = MEDIUM_FILL;  // Backdrop fill style
UCHAR UiTitleBoxFgColor     = COLOR_WHITE;  // Title box foreground color
UCHAR UiTitleBoxBgColor     = COLOR_RED;    // Title box background color
UCHAR UiMessageBoxFgColor   = COLOR_WHITE;  // Message box foreground color
UCHAR UiMessageBoxBgColor   = COLOR_BLUE;   // Message box background color
UCHAR UiMenuFgColor         = COLOR_WHITE;  // Menu foreground color
UCHAR UiMenuBgColor         = COLOR_BLUE;   // Menu background color
UCHAR UiTextColor           = COLOR_YELLOW; // Normal text color
UCHAR UiSelectedTextColor   = COLOR_BLACK;  // Selected text color
UCHAR UiSelectedTextBgColor = COLOR_GRAY;   // Selected text background color
UCHAR UiEditBoxTextColor    = COLOR_WHITE;  // Edit box text color
UCHAR UiEditBoxBgColor      = COLOR_BLACK;  // Edit box text background color

CHAR UiTitleBoxTitleText[260] = "Boot Menu";    // Title box's title text

BOOLEAN UiUseSpecialEffects = FALSE;    // Tells us if we should use fade effects
BOOLEAN UiDrawTime          = TRUE;     // Tells us if we should draw the time
BOOLEAN UiCenterMenu        = TRUE;     // Tells us if we should use a centered or left-aligned menu
BOOLEAN UiMenuBox           = TRUE;     // Tells us if we should draw a box around the menu
CHAR    UiTimeText[260] = "[Time Remaining: ] ";

const CHAR UiMonthNames[12][15] = { "January ", "February ", "March ", "April ", "May ", "June ", "July ", "August ", "September ", "October ", "November ", "December " };

UIVTBL UiVtbl =
{
    NoUiInitialize,
    NoUiUnInitialize,
    NoUiDrawBackdrop,
    NoUiFillArea,
    NoUiDrawShadow,
    NoUiDrawBox,
    NoUiDrawText,
    NoUiDrawText2,
    NoUiDrawCenteredText,
    NoUiDrawStatusText,
    NoUiUpdateDateTime,
    NoUiMessageBox,
    NoUiMessageBoxCritical,
    NoUiDrawProgressBarCenter,
    NoUiDrawProgressBar,
    NoUiEditBox,
    NoUiTextToColor,
    NoUiTextToFillStyle,
    NoUiFadeInBackdrop,
    NoUiFadeOut,
    NoUiDisplayMenu,
    NoUiDrawMenu,
};

BOOLEAN UiInitialize(BOOLEAN ShowGui)
{
    VIDEODISPLAYMODE UiDisplayMode; // Tells us if we are in text or graphics mode
    BOOLEAN UiMinimal = FALSE;      // Tells us if we are using a minimal console-like UI
    ULONG_PTR SectionId;
    ULONG Depth;
    CHAR  SettingText[260];

    if (!ShowGui)
    {
        if (!UiVtbl.Initialize())
        {
            MachVideoSetDisplayMode(NULL, FALSE);
            return FALSE;
        }
        return TRUE;
    }

    TRACE("Initializing User Interface.\n");
    TRACE("Reading UI settings from [Display] section.\n");

    /* Open the [Display] section */
    if (!IniOpenSection("Display", &SectionId))
        SectionId = 0;

    /* Select the video mode */
    SettingText[0] = '\0';
    if ((SectionId != 0) && !IniReadSettingByName(SectionId, "DisplayMode", SettingText, sizeof(SettingText)))
    {
        SettingText[0] = '\0';
    }
    UiDisplayMode = MachVideoSetDisplayMode(SettingText, TRUE);
    MachVideoGetDisplaySize(&UiScreenWidth, &UiScreenHeight, &Depth);

    /* Select the UI */
    if ((SectionId != 0) && IniReadSettingByName(SectionId, "MinimalUI", SettingText, sizeof(SettingText)))
    {
        UiMinimal = (_stricmp(SettingText, "Yes") == 0 && strlen(SettingText) == 3);
    }

    if (UiDisplayMode == VideoTextMode)
        UiVtbl = (UiMinimal ? MiniTuiVtbl : TuiVtbl);
    else
        UiVtbl = GuiVtbl;

    if (!UiVtbl.Initialize())
    {
        MachVideoSetDisplayMode(NULL, FALSE);
        return FALSE;
    }

    /* Load the settings */
    if (SectionId != 0)
    {
        static const struct
        {
            PCSTR SettingName;
            PVOID SettingVar;
            UCHAR SettingType; // 0: Text, 1: Yes/No, 2: Color, 3: Fill style
        } Settings[] =
        {
            {"TitleText", &UiTitleBoxTitleText, 0},
            {"TimeText" , &UiTimeText         , 0},

            {"SpecialEffects", &UiUseSpecialEffects, 1},
            {"ShowTime"      , &UiDrawTime         , 1},
            {"MenuBox"       , &UiMenuBox          , 1},
            {"CenterMenu"    , &UiCenterMenu       , 1},

            {"BackdropColor"      , &UiBackdropBgColor    , 2},
            {"BackdropTextColor"  , &UiBackdropFgColor    , 2},
            {"StatusBarColor"     , &UiStatusBarBgColor   , 2},
            {"StatusBarTextColor" , &UiStatusBarFgColor   , 2},
            {"TitleBoxColor"      , &UiTitleBoxBgColor    , 2},
            {"TitleBoxTextColor"  , &UiTitleBoxFgColor    , 2},
            {"MessageBoxColor"    , &UiMessageBoxBgColor  , 2},
            {"MessageBoxTextColor", &UiMessageBoxFgColor  , 2},
            {"MenuColor"          , &UiMenuBgColor        , 2},
            {"MenuTextColor"      , &UiMenuFgColor        , 2},
            {"TextColor"          , &UiTextColor          , 2},
            {"SelectedColor"      , &UiSelectedTextBgColor, 2},
            {"SelectedTextColor"  , &UiSelectedTextColor  , 2},
            {"EditBoxColor"       , &UiEditBoxBgColor     , 2},
            {"EditBoxTextColor"   , &UiEditBoxTextColor   , 2},

            {"BackdropFillStyle", &UiBackdropFillStyle, 3},
        };
        ULONG i;

        for (i = 0; i < sizeof(Settings)/sizeof(Settings[0]); ++i)
        {
            if (!IniReadSettingByName(SectionId, Settings[i].SettingName, SettingText, sizeof(SettingText)))
                continue;

            switch (Settings[i].SettingType)
            {
            case 0: // Text
                strcpy((PCHAR)Settings[i].SettingVar, SettingText);
                break;
            case 1: // Yes/No
                *(PBOOLEAN)Settings[i].SettingVar = (_stricmp(SettingText, "Yes") == 0 && strlen(SettingText) == 3);
                break;
            case 2: // Color
                *(PUCHAR)Settings[i].SettingVar = UiTextToColor(SettingText);
                break;
            case 3: // Fill style
                *(PUCHAR)Settings[i].SettingVar = UiTextToFillStyle(SettingText);
                break;
            default:
                break;
            }
        }
    }

    /* Draw the backdrop and fade it in if special effects are enabled */
    UiFadeInBackdrop();

    TRACE("UiInitialize() returning TRUE.\n");
    return TRUE;
}

VOID UiUnInitialize(PCSTR BootText)
{
    UiDrawBackdrop();
    UiDrawStatusText(BootText);
    UiInfoBox(BootText);

    UiVtbl.UnInitialize();
}

VOID UiDrawBackdrop(VOID)
{
    UiVtbl.DrawBackdrop();
}

VOID UiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr /* Color Attributes */)
{
    UiVtbl.FillArea(Left, Top, Right, Bottom, FillChar, Attr);
}

VOID UiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom)
{
    UiVtbl.DrawShadow(Left, Top, Right, Bottom);
}

VOID UiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr)
{
    UiVtbl.DrawBox(Left, Top, Right, Bottom, VertStyle, HorzStyle, Fill, Shadow, Attr);
}

VOID UiDrawText(ULONG X, ULONG Y, PCSTR Text, UCHAR Attr)
{
    UiVtbl.DrawText(X, Y, Text, Attr);
}

VOID UiDrawText2(ULONG X, ULONG Y, ULONG MaxNumChars, PCSTR Text, UCHAR Attr)
{
    UiVtbl.DrawText2(X, Y, MaxNumChars, Text, Attr);
}

VOID UiDrawCenteredText(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PCSTR TextString, UCHAR Attr)
{
    UiVtbl.DrawCenteredText(Left, Top, Right, Bottom, TextString, Attr);
}

VOID UiDrawStatusText(PCSTR StatusText)
{
    UiVtbl.DrawStatusText(StatusText);
}

VOID UiUpdateDateTime(VOID)
{
    UiVtbl.UpdateDateTime();
}

VOID UiInfoBox(PCSTR MessageText)
{
    SIZE_T        TextLength;
    ULONG        BoxWidth;
    ULONG        BoxHeight;
    ULONG        LineBreakCount;
    SIZE_T        Index;
    SIZE_T        LastIndex;
    ULONG        Left;
    ULONG        Top;
    ULONG        Right;
    ULONG        Bottom;

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
                BoxWidth = (ULONG)(Index - LastIndex);
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

VOID UiMessageBox(PCSTR Format, ...)
{
    CHAR Buffer[256];
    va_list ap;

    va_start(ap, Format);
    vsnprintf(Buffer, sizeof(Buffer) - sizeof(CHAR), Format, ap);
    UiVtbl.MessageBox(Buffer);
    va_end(ap);
}

VOID UiMessageBoxCritical(PCSTR MessageText)
{
    UiVtbl.MessageBoxCritical(MessageText);
}

UCHAR UiTextToColor(PCSTR ColorText)
{
    return UiVtbl.TextToColor(ColorText);
}

UCHAR UiTextToFillStyle(PCSTR FillStyleText)
{
    return UiVtbl.TextToFillStyle(FillStyleText);
}

VOID UiDrawProgressBarCenter(ULONG Position, ULONG Range, PCHAR ProgressText)
{
    UiVtbl.DrawProgressBarCenter(Position, Range, ProgressText);
}

VOID UiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range, PCHAR ProgressText)
{
    UiVtbl.DrawProgressBar(Left, Top, Right, Bottom, Position, Range, ProgressText);
}

VOID
UiShowMessageBoxesInSection(
    IN ULONG_PTR SectionId)
{
    ULONG Idx;
    CHAR  SettingName[80];
    CHAR  SettingValue[80];
    PCHAR MessageBoxText;
    ULONG MessageBoxTextSize;

    if (SectionId == 0)
        return;

    /* Find all the message box settings and run them */
    for (Idx = 0; Idx < IniGetNumSectionItems(SectionId); Idx++)
    {
        IniReadSettingByNumber(SectionId, Idx, SettingName, sizeof(SettingName), SettingValue, sizeof(SettingValue));
        if (_stricmp(SettingName, "MessageBox") != 0)
            continue;

        /* Get the real length of the MessageBox text */
        MessageBoxTextSize = IniGetSectionSettingValueSize(SectionId, Idx);
        // if (MessageBoxTextSize <= 0)
            // continue;

        /* Allocate enough memory to hold the text */
        MessageBoxText = FrLdrTempAlloc(MessageBoxTextSize, TAG_UI_TEXT);
        if (!MessageBoxText)
            continue;

        /* Get the MessageBox text */
        IniReadSettingByNumber(SectionId, Idx, SettingName, sizeof(SettingName), MessageBoxText, MessageBoxTextSize);

        /* Fix it up */
        UiEscapeString(MessageBoxText);

        /* Display it */
        UiMessageBox(MessageBoxText);

        /* Free the memory */
        FrLdrTempFree(MessageBoxText, TAG_UI_TEXT);
    }
}

VOID
UiShowMessageBoxesInArgv(
    IN ULONG Argc,
    IN PCHAR Argv[])
{
    ULONG LastIndex;
    PCSTR ArgValue;
    PCHAR MessageBoxText;
    SIZE_T MessageBoxTextSize;

    /* Find all the message box settings and run them */
    for (LastIndex = 0;
         (ArgValue = GetNextArgumentValue(Argc, Argv, &LastIndex, "MessageBox")) != NULL;
         ++LastIndex)
    {
        /* Get the real length of the MessageBox text */
        MessageBoxTextSize = (strlen(ArgValue) + 1) * sizeof(CHAR);

        /* Allocate enough memory to hold the text */
        MessageBoxText = FrLdrTempAlloc(MessageBoxTextSize, TAG_UI_TEXT);
        if (!MessageBoxText)
            continue;

        /* Get the MessageBox text */
        strcpy(MessageBoxText, ArgValue);

        /* Fix it up */
        UiEscapeString(MessageBoxText);

        /* Display it */
        UiMessageBox(MessageBoxText);

        /* Free the memory */
        FrLdrTempFree(MessageBoxText, TAG_UI_TEXT);
    }
}

VOID UiEscapeString(PCHAR String)
{
    ULONG    Idx;

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

VOID UiTruncateStringEllipsis(PCHAR StringText, ULONG MaxChars)
{
    /* If it's too large, just add some ellipsis past the maximum */
    if (strlen(StringText) > MaxChars)
        strcpy(&StringText[MaxChars - 3], "...");
}

BOOLEAN
UiDisplayMenu(
    IN PCSTR MenuHeader,
    IN PCSTR MenuFooter OPTIONAL,
    IN BOOLEAN ShowBootOptions,
    IN PCSTR MenuItemList[],
    IN ULONG MenuItemCount,
    IN ULONG DefaultMenuItem,
    IN LONG MenuTimeOut,
    OUT PULONG SelectedMenuItem,
    IN BOOLEAN CanEscape,
    IN UiMenuKeyPressFilterCallback KeyPressFilter OPTIONAL,
    IN PVOID Context OPTIONAL)
{
    return UiVtbl.DisplayMenu(MenuHeader, MenuFooter, ShowBootOptions,
                              MenuItemList, MenuItemCount, DefaultMenuItem,
                              MenuTimeOut, SelectedMenuItem, CanEscape,
                              KeyPressFilter, Context);
}

VOID UiFadeInBackdrop(VOID)
{
    UiVtbl.FadeInBackdrop();
}

VOID UiFadeOut(VOID)
{
    UiVtbl.FadeOut();
}

BOOLEAN UiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length)
{
    return UiVtbl.EditBox(MessageText, EditTextBuffer, Length);
}

#else
BOOLEAN UiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length)
{
    return FALSE;
}
#endif
