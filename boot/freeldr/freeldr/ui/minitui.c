/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            boot/freeldr/freeldr/ui/minitui.c
 * PURPOSE:         Mini Text UI interface
 * PROGRAMMERS:     Brian Palmer <brianp@sginet.com>
 *                  Hervé Poussineau
 */

#include <freeldr.h>

/* NTLDR or Vista+ BOOTMGR progress-bar style */
// #define NTLDR_PROGRESSBAR
// #define BTMGR_PROGRESSBAR /* Default style */

BOOLEAN MiniTuiInitialize(VOID)
{
    /* Initialize main TUI */
    if (!TuiInitialize())
        return FALSE;

    /* Override default settings with "Mini" TUI Theme */

    UiTextColor = TuiTextToColor("Default");

    UiStatusBarFgColor    = UiTextColor;
    UiStatusBarBgColor    = COLOR_BLACK;
    UiBackdropFgColor     = UiTextColor;
    UiBackdropBgColor     = COLOR_BLACK;
    UiBackdropFillStyle   = ' '; // TuiTextToFillStyle("None");
    UiTitleBoxFgColor     = COLOR_WHITE;
    UiTitleBoxBgColor     = COLOR_BLACK;
    // UiMessageBoxFgColor   = COLOR_WHITE;
    // UiMessageBoxBgColor   = COLOR_BLUE;
    UiMenuFgColor         = UiTextColor;
    UiMenuBgColor         = COLOR_BLACK;
    UiSelectedTextColor   = COLOR_BLACK;
    UiSelectedTextBgColor = UiTextColor;
    // UiEditBoxTextColor    = COLOR_WHITE;
    // UiEditBoxBgColor      = COLOR_BLACK;

    UiShowTime          = FALSE;
    UiMenuBox           = FALSE;
    UiCenterMenu        = FALSE;
    UiUseSpecialEffects = FALSE;

    // TODO: Have a boolean to show/hide title box?
    UiTitleBoxTitleText[0] = ANSI_NULL;

    RtlStringCbCopyA(UiTimeText, sizeof(UiTimeText),
                     "Seconds until highlighted choice will be started automatically:");

    return TRUE;
}

VOID MiniTuiDrawBackdrop(VOID)
{
    /* Fill in a black background */
    TuiFillArea(0, 0, UiScreenWidth - 1, UiScreenHeight - 3,
                UiBackdropFillStyle,
                ATTR(UiBackdropFgColor, UiBackdropBgColor));

    /* Update the screen buffer */
    VideoCopyOffScreenBufferToVRAM();
}

VOID MiniTuiDrawStatusText(PCSTR StatusText)
{
    /* Minimal UI doesn't have a status bar */
}

/*static*/ VOID
MiniTuiSetProgressBarText(
    _In_ PCSTR ProgressText)
{
    ULONG ProgressBarWidth;
    CHAR ProgressString[256];

    /* Make sure the progress bar is enabled */
    ASSERT(UiProgressBar.Show);

    /* Calculate the width of the bar proper */
    ProgressBarWidth = UiProgressBar.Right - UiProgressBar.Left + 1;

    /* First make sure the progress bar text fits */
    RtlStringCbCopyA(ProgressString, sizeof(ProgressString), ProgressText);
    TuiTruncateStringEllipsis(ProgressString, ProgressBarWidth);

    /* Clear the text area */
    TuiFillArea(UiProgressBar.Left, UiProgressBar.Top,
                UiProgressBar.Right,
#ifdef NTLDR_PROGRESSBAR
                UiProgressBar.Bottom - 1,
#else // BTMGR_PROGRESSBAR
                UiProgressBar.Bottom - 2, // One empty line between text and bar.
#endif
                ' ', ATTR(UiTextColor, UiMenuBgColor));

    /* Draw the "Loading..." text */
    TuiDrawCenteredText(UiProgressBar.Left, UiProgressBar.Top,
                        UiProgressBar.Right,
#ifdef NTLDR_PROGRESSBAR
                        UiProgressBar.Bottom - 1,
#else // BTMGR_PROGRESSBAR
                        UiProgressBar.Bottom - 2, // One empty line between text and bar.
#endif
                        ProgressString, ATTR(UiTextColor, UiMenuBgColor));
}

/*static*/ VOID
MiniTuiTickProgressBar(
    _In_ ULONG SubPercentTimes100)
{
    ULONG ProgressBarWidth;
    ULONG FillCount;

    /* Make sure the progress bar is enabled */
    ASSERT(UiProgressBar.Show);

    ASSERT(SubPercentTimes100 <= (100 * 100));

    /* Calculate the width of the bar proper */
    ProgressBarWidth = UiProgressBar.Right - UiProgressBar.Left + 1;

    /* Compute fill count */
    // FillCount = (ProgressBarWidth * Position) / Range;
    FillCount = ProgressBarWidth * SubPercentTimes100 / (100 * 100);

    /* Fill the progress bar */
    /* Draw the percent complete -- Use the fill character */
    if (FillCount > 0)
    {
        TuiFillArea(UiProgressBar.Left, UiProgressBar.Bottom,
                    UiProgressBar.Left + FillCount - 1, UiProgressBar.Bottom,
                    '\xDB', ATTR(UiTextColor, UiMenuBgColor));
    }
    /* Fill the remaining with blanks */
    TuiFillArea(UiProgressBar.Left + FillCount, UiProgressBar.Bottom,
                UiProgressBar.Right, UiProgressBar.Bottom,
                ' ', ATTR(UiTextColor, UiMenuBgColor));

    TuiUpdateDateTime();
    VideoCopyOffScreenBufferToVRAM();
}

VOID
MiniTuiDrawProgressBarCenter(
    _In_ PCSTR ProgressText)
{
    ULONG Left, Top, Right, Bottom, Width, Height;

    /* Build the coordinates and sizes */
#ifdef NTLDR_PROGRESSBAR
    Height = 2;
    Width  = UiScreenWidth;
    Left = 0;
    Top  = UiScreenHeight - Height - 2;
#else // BTMGR_PROGRESSBAR
    Height = 3;
    Width  = UiScreenWidth - 4;
    Left = 2;
    Top  = UiScreenHeight - Height - 3;
#endif
    Right  = Left + Width - 1;
    Bottom = Top + Height - 1;

    /* Draw the progress bar */
    MiniTuiDrawProgressBar(Left, Top, Right, Bottom, ProgressText);
}

VOID
MiniTuiDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR ProgressText)
{
    UiInitProgressBar(Left, Top, Right, Bottom, ProgressText);
}

VOID
MiniTuiDrawMenu(
    _In_ PUI_MENU_INFO MenuInfo)
{
    ULONG i;

    /* Draw the backdrop */
    UiDrawBackdrop();

    /* No GUI status bar text, just minimal text. Show the menu header. */
    if (MenuInfo->MenuHeader)
    {
        UiVtbl.DrawText(0,
                        MenuInfo->Top - 2,
                        MenuInfo->MenuHeader,
                        ATTR(UiMenuFgColor, UiMenuBgColor));
    }

    /* Draw the menu box */
    TuiDrawMenuBox(MenuInfo);

    /* Draw each line of the menu */
    for (i = 0; i < MenuInfo->MenuItemCount; ++i)
    {
        TuiDrawMenuItem(MenuInfo, i);
    }

    /* Now tell the user how to choose */
    UiVtbl.DrawText(0,
                    MenuInfo->Bottom + 1,
                    "Use \x18 and \x19 to move the highlight to your choice.",
                    ATTR(UiMenuFgColor, UiMenuBgColor));
    UiVtbl.DrawText(0,
                    MenuInfo->Bottom + 2,
                    "Press ENTER to choose.",
                    ATTR(UiMenuFgColor, UiMenuBgColor));

    /* And show the menu footer */
    if (MenuInfo->MenuFooter)
    {
        UiVtbl.DrawText(0,
                        UiScreenHeight - 4,
                        MenuInfo->MenuFooter,
                        ATTR(UiMenuFgColor, UiMenuBgColor));
    }

    VideoCopyOffScreenBufferToVRAM();
}

const UIVTBL MiniTuiVtbl =
{
    MiniTuiInitialize,
    TuiUnInitialize,
    MiniTuiDrawBackdrop,
    TuiFillArea,
    TuiDrawShadow,
    TuiDrawBox,
    TuiDrawText,
    TuiDrawText2,
    TuiDrawCenteredText,
    MiniTuiDrawStatusText,
    TuiUpdateDateTime,
    TuiMessageBox,
    TuiMessageBoxCritical,
    MiniTuiDrawProgressBarCenter,
    MiniTuiDrawProgressBar,
    MiniTuiSetProgressBarText,
    MiniTuiTickProgressBar,
    TuiEditBox,
    TuiTextToColor,
    TuiTextToFillStyle,
    MiniTuiDrawBackdrop, /* no FadeIn */
    TuiFadeOut,
    TuiDisplayMenu,
    MiniTuiDrawMenu,
};

