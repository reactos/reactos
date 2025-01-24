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

#pragma once

extern ULONG UiScreenWidth;             // Screen Width
extern ULONG UiScreenHeight;            // Screen Height

extern UCHAR UiStatusBarFgColor;        // Status bar foreground color
extern UCHAR UiStatusBarBgColor;        // Status bar background color
extern UCHAR UiBackdropFgColor;         // Backdrop foreground color
extern UCHAR UiBackdropBgColor;         // Backdrop background color
extern UCHAR UiBackdropFillStyle;       // Backdrop fill style
extern UCHAR UiTitleBoxFgColor;         // Title box foreground color
extern UCHAR UiTitleBoxBgColor;         // Title box background color
extern UCHAR UiMessageBoxFgColor;       // Message box foreground color
extern UCHAR UiMessageBoxBgColor;       // Message box background color
extern UCHAR UiMenuFgColor;             // Menu foreground color
extern UCHAR UiMenuBgColor;             // Menu background color
extern UCHAR UiTextColor;               // Normal text color
extern UCHAR UiSelectedTextColor;       // Selected text color
extern UCHAR UiSelectedTextBgColor;     // Selected text background color
extern UCHAR UiEditBoxTextColor;        // Edit box text color
extern UCHAR UiEditBoxBgColor;          // Edit box text background color

extern BOOLEAN UiShowTime;              // Whether to draw the time
extern BOOLEAN UiMenuBox;               // Whether to draw a box around the menu
extern BOOLEAN UiCenterMenu;            // Whether to use a centered or left-aligned menu
extern BOOLEAN UiUseSpecialEffects;     // Whether to use fade effects

extern CHAR UiTitleBoxTitleText[260];   // Title box's title text
extern CHAR UiTimeText[260];

extern const PCSTR UiMonthNames[12];

/* User Interface Functions **************************************************/

BOOLEAN    UiInitialize(BOOLEAN ShowUi);                                // Initialize User-Interface
VOID    UiUnInitialize(PCSTR BootText);                        // Un-initialize User-Interface
VOID    UiDrawBackdrop(VOID);                                    // Fills the entire screen with a backdrop
VOID    UiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr /* Color Attributes */);    // Fills the area specified with FillChar and Attr
VOID    UiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);    // Draws a shadow on the bottom and right sides of the area specified
VOID    UiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr);    // Draws a box around the area specified

/* Draws text at coordinates specified */
VOID
UiDrawText(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ PCSTR Text,
    _In_ UCHAR Attr);

/* Draws text at coordinates specified */
VOID
UiDrawText2(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_opt_ ULONG MaxNumChars,
    _In_reads_or_z_(MaxNumChars) PCSTR Text,
    _In_ UCHAR Attr);

/* Draws centered text at the coordinates specified and clips the edges */
VOID
UiDrawCenteredText(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR TextString,
    _In_ UCHAR Attr);

VOID    UiDrawStatusText(PCSTR StatusText);                    // Draws text at the very bottom line on the screen
VOID    UiUpdateDateTime(VOID);                                    // Updates the date and time

/* Displays an info box on the screen */
VOID
UiInfoBox(
    _In_ PCSTR MessageText);

/* Displays a message box on the screen with an ok button */
VOID
UiMessageBox(
    _In_ PCSTR Format, ...);

/* Displays a message box on the screen with an ok button using no system resources */
VOID
UiMessageBoxCritical(
    _In_ PCSTR MessageText);

ULONG
UiGetScreenHeight(VOID);

UCHAR
UiGetMenuBgColor(VOID);

/* Loading Progress-Bar Functions ********************************************/

/*
 * Loading progress bar, based on the one from NTOS Inbv.
 * Supports progress within sub-ranges, used when loading
 * with an unknown number of steps.
 */
typedef struct _UI_PROGRESS_BAR
{
    // UI_PROGRESS_STATE
    struct
    {
        ULONG Floor;
        // ULONG Ceiling;
        ULONG Bias;
    } State;

    // BT_PROGRESS_INDICATOR
    struct
    {
        ULONG Count;
        ULONG Expected;
        ULONG Percentage;
    } Indicator;

    ULONG Left;
    ULONG Top;
    ULONG Right;
    ULONG Bottom;
    // ULONG Width; // == Right - Left + 1;
    BOOLEAN Show;
} UI_PROGRESS_BAR, *PUI_PROGRESS_BAR;

extern UI_PROGRESS_BAR UiProgressBar;

VOID
UiInitProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR ProgressText);

/* Indicate loading progress without any specific number of steps */
VOID
UiIndicateProgress(VOID);

/* Set a progress loading percentage range */
VOID
UiSetProgressBarSubset(
    _In_ ULONG Floor,
    _In_ ULONG Ceiling);

/* Update the loading progress percentage within a selected range */
VOID
UiUpdateProgressBar(
    _In_ ULONG Percentage,
    _In_opt_ PCSTR ProgressText);

VOID
UiSetProgressBarText(
    _In_ PCSTR ProgressText);

/* Draws the progress bar showing nPos percent filled */
VOID
UiDrawProgressBarCenter(
    _In_ PCSTR ProgressText);

/* Draws the progress bar showing nPos percent filled */
VOID
UiDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR ProgressText);


// Displays all the message boxes in a given section.
VOID
UiShowMessageBoxesInSection(
    IN ULONG_PTR SectionId);

VOID
UiShowMessageBoxesInArgv(
    IN ULONG Argc,
    IN PCHAR Argv[]);

BOOLEAN    UiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length);

UCHAR    UiTextToColor(PCSTR ColorText);                        // Converts the text color into it's equivalent color value
UCHAR    UiTextToFillStyle(PCSTR FillStyleText);                // Converts the text fill into it's equivalent fill value

VOID    UiFadeInBackdrop(VOID);                                    // Draws the backdrop and fades the screen in
VOID    UiFadeOut(VOID);                                        // Fades the screen out

/* Menu Functions ************************************************************/

typedef struct tagUI_MENU_INFO
{
    PCSTR   MenuHeader;
    PCSTR   MenuFooter;

    PCSTR*  MenuItemList;
    ULONG   MenuItemCount;
    LONG    MenuTimeRemaining;
    ULONG   SelectedMenuItem;
    PVOID   Context;

    ULONG   Left;
    ULONG   Top;
    ULONG   Right;
    ULONG   Bottom;
} UI_MENU_INFO, *PUI_MENU_INFO;

typedef
BOOLEAN
(*UiMenuKeyPressFilterCallback)(
    IN ULONG KeyPress,
    IN ULONG SelectedMenuItem,
    IN PVOID Context OPTIONAL);

BOOLEAN
UiDisplayMenu(
    IN PCSTR MenuHeader,
    IN PCSTR MenuFooter OPTIONAL,
    IN PCSTR MenuItemList[],
    IN ULONG MenuItemCount,
    IN ULONG DefaultMenuItem,
    IN LONG MenuTimeOut,
    OUT PULONG SelectedMenuItem,
    IN BOOLEAN CanEscape,
    IN UiMenuKeyPressFilterCallback KeyPressFilter OPTIONAL,
    IN PVOID Context OPTIONAL);

///////////////////////////////////////////////////////////////////////////////////////
//
// UI virtual table
//
///////////////////////////////////////////////////////////////////////////////////////
typedef struct tagUIVTBL
{
    BOOLEAN (*Initialize)(VOID);
    VOID (*UnInitialize)(VOID);

    VOID (*DrawBackdrop)(VOID);
    VOID (*FillArea)(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr);
    VOID (*DrawShadow)(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);
    VOID (*DrawBox)(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr);
    VOID (*DrawText)(ULONG X, ULONG Y, PCSTR Text, UCHAR Attr);
    VOID (*DrawText2)(ULONG X, ULONG Y, ULONG MaxNumChars, PCSTR Text, UCHAR Attr);
    VOID (*DrawCenteredText)(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PCSTR TextString, UCHAR Attr);
    VOID (*DrawStatusText)(PCSTR StatusText);
    VOID (*UpdateDateTime)(VOID);
    VOID (*MessageBox)(PCSTR MessageText);
    VOID (*MessageBoxCritical)(PCSTR MessageText);

    VOID (*DrawProgressBarCenter)(
        _In_ PCSTR ProgressText);

    VOID (*DrawProgressBar)(
        _In_ ULONG Left,
        _In_ ULONG Top,
        _In_ ULONG Right,
        _In_ ULONG Bottom,
        _In_ PCSTR ProgressText);

    VOID (*SetProgressBarText)(
        _In_ PCSTR ProgressText);

    VOID (*TickProgressBar)(
        _In_ ULONG SubPercentTimes100);

    BOOLEAN (*EditBox)(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length);
    UCHAR (*TextToColor)(PCSTR ColorText);
    UCHAR (*TextToFillStyle)(PCSTR FillStyleText);
    VOID (*FadeInBackdrop)(VOID);
    VOID (*FadeOut)(VOID);

    BOOLEAN (*DisplayMenu)(
        IN PCSTR MenuHeader,
        IN PCSTR MenuFooter OPTIONAL,
        IN PCSTR MenuItemList[],
        IN ULONG MenuItemCount,
        IN ULONG DefaultMenuItem,
        IN LONG MenuTimeOut,
        OUT PULONG SelectedMenuItem,
        IN BOOLEAN CanEscape,
        IN UiMenuKeyPressFilterCallback KeyPressFilter OPTIONAL,
        IN PVOID Context OPTIONAL);

    VOID (*DrawMenu)(PUI_MENU_INFO MenuInfo);
} UIVTBL, *PUIVTBL;

VOID UiInit(const char *CmdLine);

VOID
UiResetForSOS(VOID);

extern UIVTBL UiVtbl;

/*
 * Fill styles for DrawBackdrop()
 */
#define LIGHT_FILL      0xB0
#define MEDIUM_FILL     0xB1
#define DARK_FILL       0xB2

/*
 * Combines the foreground and background colors into a single attribute byte
 */
#define ATTR(cFore, cBack)  ((cBack << 4) | cFore)

/*
 * Screen colors
 */
#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_GRAY          7

#define COLOR_DARKGRAY      8
#define COLOR_LIGHTBLUE     9
#define COLOR_LIGHTGREEN    10
#define COLOR_LIGHTCYAN     11
#define COLOR_LIGHTRED      12
#define COLOR_LIGHTMAGENTA  13
#define COLOR_YELLOW        14
#define COLOR_WHITE         15

/* Add COLOR_BLINK to a background to cause blinking */
// #define COLOR_BLINK         8

/*
 * Defines for IBM box drawing characters
 */
#define HORZ    0xC4    // Single horizontal line
#define D_HORZ  0xCD    // Double horizontal line
#define VERT    0xB3    // Single vertical line
#define D_VERT  0xBA    // Double vertical line


/* THEME HEADERS *************************************************************/

// #include <ui/gui.h>
#include <ui/minitui.h>
#include <ui/noui.h>
#include <ui/tui.h>
