/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Framebuffer-backed graphics UI
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include <freeldr.h>
#include "../arch/vidfb.h"

#define FBGUI_PROGRESS_FILL_ATTR      ATTR(COLOR_WHITE, COLOR_WHITE)
#define FBGUI_PROGRESS_REMAINING_ATTR ATTR(COLOR_GRAY, COLOR_GRAY)

static
BOOLEAN
FbGuiInitialize(VOID)
{
    if (!MiniTuiInitialize())
        return FALSE;

    return TRUE;
}

static
VOID
FbGuiSetProgressBarText(
    _In_ PCSTR ProgressText)
{
    ULONG ProgressBarWidth;
    CHAR ProgressString[256];

    ASSERT(UiProgressBar.Show);

    ProgressBarWidth = UiProgressBar.Right - UiProgressBar.Left + 1;

    RtlStringCbCopyA(ProgressString, sizeof(ProgressString), ProgressText);
    TuiTruncateStringEllipsis(ProgressString, ProgressBarWidth);

    /* Update the text region above the progress bar. */
    TuiFillArea(UiProgressBar.Left, UiProgressBar.Top,
                UiProgressBar.Right, UiProgressBar.Bottom - 2,
                ' ', ATTR(COLOR_GRAY, COLOR_BLACK));

    /* Draw the loading text centered above the progress bar. */
    TuiDrawCenteredText(UiProgressBar.Left, UiProgressBar.Top,
                        UiProgressBar.Right, UiProgressBar.Bottom - 2,
                        ProgressString, ATTR(COLOR_GRAY, COLOR_BLACK));
}

static
VOID
FbGuiTickProgressBar(
    _In_ ULONG SubPercentTimes100)
{
    ASSERT(UiProgressBar.Show);
    ASSERT(SubPercentTimes100 <= (100 * 100));

    /* Reserve the bottom row for the pixel progress bar. */
    TuiFillArea(UiProgressBar.Left, UiProgressBar.Bottom,
                UiProgressBar.Right, UiProgressBar.Bottom,
                ' ', ATTR(COLOR_BLACK, COLOR_BLACK));

    /* Copy the text layer first, then overlay the progress bar. */
    VideoCopyOffScreenBufferToVRAM();

    FbConsDrawProgressBar(UiProgressBar.Left,
                          UiProgressBar.Right,
                          UiProgressBar.Bottom,
                          FBGUI_PROGRESS_FILL_ATTR,
                          FBGUI_PROGRESS_REMAINING_ATTR,
                          SubPercentTimes100);
}

const UIVTBL FbGuiVtbl =
{
    FbGuiInitialize,
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
    FbGuiSetProgressBarText,
    FbGuiTickProgressBar,
    TuiEditBox,
    TuiTextToColor,
    TuiTextToFillStyle,
    MiniTuiFadeInBackdrop,
    TuiFadeOut,
    TuiDisplayMenu,
    MiniTuiDrawMenu,
};
