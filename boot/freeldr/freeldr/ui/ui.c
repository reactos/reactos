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

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

UCHAR UiStatusBarFgColor;       // Status bar foreground color
UCHAR UiStatusBarBgColor;       // Status bar background color
UCHAR UiBackdropFgColor;        // Backdrop foreground color
UCHAR UiBackdropBgColor;        // Backdrop background color
UCHAR UiBackdropFillStyle;      // Backdrop fill style
UCHAR UiTitleBoxFgColor;        // Title box foreground color
UCHAR UiTitleBoxBgColor;        // Title box background color
UCHAR UiMessageBoxFgColor;      // Message box foreground color
UCHAR UiMessageBoxBgColor;      // Message box background color
UCHAR UiMenuFgColor;            // Menu foreground color
UCHAR UiMenuBgColor;            // Menu background color
UCHAR UiTextColor;              // Normal text color
UCHAR UiSelectedTextColor;      // Selected text color
UCHAR UiSelectedTextBgColor;    // Selected text background color
UCHAR UiEditBoxTextColor;       // Edit box text color
UCHAR UiEditBoxBgColor;         // Edit box text background color

BOOLEAN UiShowTime;             // Whether to draw the time
BOOLEAN UiMenuBox;              // Whether to draw a box around the menu
BOOLEAN UiCenterMenu;           // Whether to use a centered or left-aligned menu
BOOLEAN UiUseSpecialEffects;    // Whether to use fade effects

CHAR UiTitleBoxTitleText[260] = "Boot Menu";    // Title box's title text
CHAR UiTimeText[260] = "[Time Remaining: %d]";

const PCSTR UiMonthNames[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

#define TAG_UI_TEXT 'xTiU'

ULONG UiScreenWidth;    // Screen Width
ULONG UiScreenHeight;   // Screen Height

/*
 * Loading progress bar, based on the NTOS Inbv one.
 * Supports progress within sub-ranges, used when loading
 * with an unknown number of steps.
 */
UI_PROGRESS_BAR UiProgressBar = {{0}};

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
    NoUiSetProgressBarText,
    NoUiTickProgressBar,
    NoUiEditBox,
    NoUiTextToColor,
    NoUiTextToFillStyle,
    NoUiFadeInBackdrop,
    NoUiFadeOut,
    NoUiDisplayMenu,
    NoUiDrawMenu,
};

BOOLEAN UiInitialize(BOOLEAN ShowUi)
{
    VIDEODISPLAYMODE UiDisplayMode; // Tells us if we are in text or graphics mode
    BOOLEAN UiMinimal = FALSE;      // Tells us if we are using a minimal console-like UI
    ULONG_PTR SectionId;
    ULONG Depth;
    CHAR  SettingText[260];

    if (!ShowUi)
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
        UiMinimal = (_stricmp(SettingText, "Yes") == 0);
    }

    if (UiDisplayMode == VideoGraphicsMode)
#if 0 // We don't support a GUI mode yet.
        UiVtbl = GuiVtbl;
#else
    {
        // Switch back to text mode.
        MachVideoSetDisplayMode(NULL, TRUE);
        UiDisplayMode = VideoTextMode;
    }
#endif
    else // if (UiDisplayMode == VideoTextMode)
        UiVtbl = (UiMinimal ? MiniTuiVtbl : TuiVtbl);

    /* Load the UI and initialize its default settings */
    if (!UiVtbl.Initialize())
    {
        MachVideoSetDisplayMode(NULL, FALSE);
        return FALSE;
    }

    /* Load the user UI settings */
    if (SectionId != 0)
    {
        static const struct
        {
            PCSTR SettingName;
            PVOID SettingVar;
            SIZE_T SettingSize OPTIONAL; // Must be non-zero only for text buffers.
            UCHAR SettingType; // 0: Text, 1: Yes/No, 2: Color, 3: Fill style
        } Settings[] =
        {
            {"TitleText", &UiTitleBoxTitleText, sizeof(UiTitleBoxTitleText), 0},
            {"TimeText" , &UiTimeText, sizeof(UiTimeText), 0},

            {"ShowTime"      , &UiShowTime         , 0, 1},
            {"MenuBox"       , &UiMenuBox          , 0, 1},
            {"CenterMenu"    , &UiCenterMenu       , 0, 1},
            {"SpecialEffects", &UiUseSpecialEffects, 0, 1},

            {"BackdropColor"      , &UiBackdropBgColor    , 0, 2},
            {"BackdropTextColor"  , &UiBackdropFgColor    , 0, 2},
            {"StatusBarColor"     , &UiStatusBarBgColor   , 0, 2},
            {"StatusBarTextColor" , &UiStatusBarFgColor   , 0, 2},
            {"TitleBoxColor"      , &UiTitleBoxBgColor    , 0, 2},
            {"TitleBoxTextColor"  , &UiTitleBoxFgColor    , 0, 2},
            {"MessageBoxColor"    , &UiMessageBoxBgColor  , 0, 2},
            {"MessageBoxTextColor", &UiMessageBoxFgColor  , 0, 2},
            {"MenuColor"          , &UiMenuBgColor        , 0, 2},
            {"MenuTextColor"      , &UiMenuFgColor        , 0, 2},
            {"TextColor"          , &UiTextColor          , 0, 2},
            {"SelectedColor"      , &UiSelectedTextBgColor, 0, 2},
            {"SelectedTextColor"  , &UiSelectedTextColor  , 0, 2},
            {"EditBoxColor"       , &UiEditBoxBgColor     , 0, 2},
            {"EditBoxTextColor"   , &UiEditBoxTextColor   , 0, 2},

            {"BackdropFillStyle", &UiBackdropFillStyle, 0, 3},
        };
        ULONG i;

        for (i = 0; i < RTL_NUMBER_OF(Settings); ++i)
        {
            if (!IniReadSettingByName(SectionId, Settings[i].SettingName, SettingText, sizeof(SettingText)))
                continue;

            switch (Settings[i].SettingType)
            {
            case 0: // Text
                RtlStringCbCopyA((PCHAR)Settings[i].SettingVar,
                                 Settings[i].SettingSize, SettingText);
                break;
            case 1: // Yes/No
                *(PBOOLEAN)Settings[i].SettingVar = (_stricmp(SettingText, "Yes") == 0);
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

VOID
UiDrawText(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ PCSTR Text,
    _In_ UCHAR Attr)
{
    UiVtbl.DrawText(X, Y, Text, Attr);
}

VOID
UiDrawText2(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_opt_ ULONG MaxNumChars,
    _In_reads_or_z_(MaxNumChars) PCSTR Text,
    _In_ UCHAR Attr)
{
    UiVtbl.DrawText2(X, Y, MaxNumChars, Text, Attr);
}

VOID
UiDrawCenteredText(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR TextString,
    _In_ UCHAR Attr)
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

VOID
UiInfoBox(
    _In_ PCSTR MessageText)
{
    SIZE_T TextLength;
    ULONG  BoxWidth;
    ULONG  BoxHeight;
    ULONG  LineBreakCount;
    SIZE_T Index;
    SIZE_T LastIndex;
    ULONG  Left;
    ULONG  Top;
    ULONG  Right;
    ULONG  Bottom;

    TextLength = strlen(MessageText);

    /* Count the new lines and the box width */
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

    /* Calc the box width & height */
    BoxWidth += 6;
    BoxHeight = LineBreakCount + 4;

    /* Calc the box coordinates */
    Left = (UiScreenWidth / 2) - (BoxWidth / 2);
    Top  = (UiScreenHeight / 2) - (BoxHeight / 2);
    Right  = (UiScreenWidth / 2) + (BoxWidth / 2);
    Bottom = (UiScreenHeight / 2) + (BoxHeight / 2);

    /* Draw the box */
    UiDrawBox(Left,
              Top,
              Right,
              Bottom,
              VERT,
              HORZ,
              TRUE,
              TRUE,
              ATTR(UiMenuFgColor, UiMenuBgColor));

    /* Draw the text */
    UiDrawCenteredText(Left, Top, Right, Bottom, MessageText, ATTR(UiTextColor, UiMenuBgColor));
}

VOID
UiMessageBox(
    _In_ PCSTR Format, ...)
{
    va_list ap;
    CHAR Buffer[1024];

    va_start(ap, Format);
    _vsnprintf(Buffer, sizeof(Buffer) - sizeof(CHAR), Format, ap);
    UiVtbl.MessageBox(Buffer);
    va_end(ap);
}

VOID
UiMessageBoxCritical(
    _In_ PCSTR MessageText)
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

VOID
UiInitProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR ProgressText)
{
    /* Progress bar area */
    UiProgressBar.Left = Left;
    UiProgressBar.Top  = Top;
    UiProgressBar.Right  = Right;
    UiProgressBar.Bottom = Bottom;
    // UiProgressBar.Width = Right - Left + 1;

    /* Set the progress bar ranges */
    UiSetProgressBarSubset(0, 100);
    UiProgressBar.Indicator.Count = 0;
    UiProgressBar.Indicator.Expected = 25;
    UiProgressBar.Indicator.Percentage = 0;

    /* Enable the progress bar */
    UiProgressBar.Show = TRUE;

    /* Initial drawing: set the "Loading..." text and the original position */
    UiVtbl.SetProgressBarText(ProgressText);
    UiVtbl.TickProgressBar(0);
}

VOID
UiIndicateProgress(VOID)
{
    ULONG Percentage;

    /* Increase progress */
    UiProgressBar.Indicator.Count++;

    /* Compute the new percentage - Don't go over 100% */
    Percentage = 100 * UiProgressBar.Indicator.Count /
                       UiProgressBar.Indicator.Expected;
    Percentage = min(Percentage, 99);

    if (Percentage != UiProgressBar.Indicator.Percentage)
    {
        /* Percentage has changed, update the progress bar */
        UiProgressBar.Indicator.Percentage = Percentage;
        UiUpdateProgressBar(Percentage, NULL);
    }
}

VOID
UiSetProgressBarSubset(
    _In_ ULONG Floor,
    _In_ ULONG Ceiling)
{
    /* Sanity checks */
    ASSERT(Floor < Ceiling);
    ASSERT(Ceiling <= 100);

    /* Update the progress bar state */
    UiProgressBar.State.Floor = Floor * 100;
    // UiProgressBar.State.Ceiling = Ceiling * 100;
    UiProgressBar.State.Bias = Ceiling - Floor;
}

VOID
UiUpdateProgressBar(
    _In_ ULONG Percentage,
    _In_opt_ PCSTR ProgressText)
{
    ULONG TotalProgress;

    /* Make sure the progress bar is enabled */
    if (!UiProgressBar.Show)
        return;

    /* Set the progress text if specified */
    if (ProgressText)
        UiSetProgressBarText(ProgressText);

    /* Compute the total progress and tick the progress bar */
    TotalProgress = UiProgressBar.State.Floor + (Percentage * UiProgressBar.State.Bias);
    // TotalProgress /= (100 * 100);

    UiVtbl.TickProgressBar(TotalProgress);
}

VOID
UiSetProgressBarText(
    _In_ PCSTR ProgressText)
{
    /* Make sure the progress bar is enabled */
    if (!UiProgressBar.Show)
        return;

    UiVtbl.SetProgressBarText(ProgressText);
}

VOID
UiDrawProgressBarCenter(
    _In_ PCSTR ProgressText)
{
    UiVtbl.DrawProgressBarCenter(ProgressText);
}

VOID
UiDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR ProgressText)
{
    UiVtbl.DrawProgressBar(Left, Top, Right, Bottom, ProgressText);
}

static VOID
UiEscapeString(PCHAR String)
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
    IN PVOID Context OPTIONAL)
{
    return UiVtbl.DisplayMenu(MenuHeader, MenuFooter,
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

VOID
UiResetForSOS(VOID)
{
#ifdef _M_ARM
    /* Re-initialize the UI */
    UiInitialize(TRUE);
#else
    /* Reset the UI and switch to MiniTui */
    UiVtbl.UnInitialize();
    UiVtbl = MiniTuiVtbl;
    UiVtbl.Initialize();
#endif
    /* Disable the progress bar */
    UiProgressBar.Show = FALSE;
}

ULONG
UiGetScreenHeight(VOID)
{
    return UiScreenHeight;
}

UCHAR
UiGetMenuBgColor(VOID)
{
    return UiMenuBgColor;
}

/* EOF */
