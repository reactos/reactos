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

typedef struct _SMALL_RECT
{
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT, *PSMALL_RECT;

PVOID TextVideoBuffer = NULL;

/* GENERIC TUI UTILS *********************************************************/

/*
 * TuiPrintf()
 * Prints formatted text to the screen.
 */
INT
TuiPrintf(
    _In_ PCSTR Format, ...)
{
    INT i;
    INT Length;
    va_list ap;
    CHAR Buffer[512];

    va_start(ap, Format);
    Length = _vsnprintf(Buffer, sizeof(Buffer), Format, ap);
    va_end(ap);

    if (Length == -1)
        Length = (INT)sizeof(Buffer);

    for (i = 0; i < Length; i++)
    {
        MachConsPutChar(Buffer[i]);
    }

    return Length;
}

VOID
TuiTruncateStringEllipsis(
    _Inout_z_ PSTR StringText,
    _In_ ULONG MaxChars)
{
    /* If it's too large, just add some ellipsis past the maximum */
    if (strlen(StringText) > MaxChars)
        strcpy(&StringText[MaxChars - 3], "...");
}

/*
 * DrawText()
 * Displays a string on a single screen line.
 * This function assumes coordinates are zero-based.
 */
VOID
TuiDrawText(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ PCSTR Text,
    _In_ UCHAR Attr)
{
    TuiDrawText2(X, Y, 0 /*(ULONG)strlen(Text)*/, Text, Attr);
}

/*
 * DrawText2()
 * Displays a string on a single screen line.
 * This function assumes coordinates are zero-based.
 * MaxNumChars is the maximum number of characters to display.
 * If MaxNumChars == 0, then display the whole string.
 */
VOID
TuiDrawText2(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_opt_ ULONG MaxNumChars,
    _In_reads_or_z_(MaxNumChars) PCSTR Text,
    _In_ UCHAR Attr)
{
    PUCHAR ScreenMemory = (PUCHAR)TextVideoBuffer;
    ULONG i, j;

    /* Don't display anything if we are out of the screen */
    if ((X >= UiScreenWidth) || (Y >= UiScreenHeight))
        return;

    /* Draw the text, not exceeding the width */
    for (i = X, j = 0; Text[j] && i < UiScreenWidth && (MaxNumChars > 0 ? j < MaxNumChars : TRUE); i++, j++)
    {
        ScreenMemory[((Y*2)*UiScreenWidth)+(i*2)]   = (UCHAR)Text[j];
        ScreenMemory[((Y*2)*UiScreenWidth)+(i*2)+1] = Attr;
    }
}

VOID
TuiDrawCenteredText(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR TextString,
    _In_ UCHAR Attr)
{
    SIZE_T TextLength;
    SIZE_T Index, LastIndex;
    ULONG  LineBreakCount;
    ULONG  BoxWidth, BoxHeight;
    ULONG  RealLeft, RealTop;
    ULONG  X, Y;
    CHAR   Temp[2];

    /* Query text length */
    TextLength = strlen(TextString);

    /* Count the new lines and the box width */
    LineBreakCount = 0;
    BoxWidth = 0;
    LastIndex = 0;
    for (Index = 0; Index < TextLength; Index++)
    {
        /* Scan for new lines */
        if (TextString[Index] == '\n')
        {
            /* Remember the new line */
            LastIndex = Index;
            LineBreakCount++;
        }
        else
        {
            /* Check for new larger box width */
            if ((Index - LastIndex) > BoxWidth)
            {
                /* Update it */
                BoxWidth = (ULONG)(Index - LastIndex);
            }
        }
    }

    /* Base the box height on the number of lines */
    BoxHeight = LineBreakCount + 1;

    /*
     * Create the centered coordinates.
     * Here, the Left/Top/Right/Bottom rectangle is a hint, around
     * which we center the "real" text rectangle RealLeft/RealTop.
     */
    RealLeft = (Left + Right - BoxWidth + 1) / 2;
    RealTop  = (Top + Bottom - BoxHeight + 1) / 2;

    /* Now go for a second scan */
    LastIndex = 0;
    for (Index = 0; Index < TextLength; Index++)
    {
        /* Look for new lines again */
        if (TextString[Index] == '\n')
        {
            /* Update where the text should start */
            RealTop++;
            LastIndex = 0;
        }
        else
        {
            /* We've got a line of text to print, do it */
            X = (ULONG)(RealLeft + LastIndex);
            Y = RealTop;
            LastIndex++;
            Temp[0] = TextString[Index];
            Temp[1] = 0;
            TuiDrawText(X, Y, Temp, Attr);
        }
    }
}

/* FULL TUI THEME ************************************************************/

#define TAG_TUI_SCREENBUFFER 'SiuT'
#define TAG_TUI_PALETTE      'PiuT'

extern UCHAR MachDefaultTextColor;

BOOLEAN TuiInitialize(VOID)
{
    MachVideoHideShowTextCursor(FALSE);
    MachVideoSetTextCursorPosition(0, 0);
    MachVideoClearScreen(ATTR(COLOR_GRAY, COLOR_BLACK));

    TextVideoBuffer = VideoAllocateOffScreenBuffer();
    if (TextVideoBuffer == NULL)
    {
        return FALSE;
    }

    /* Load default settings with "Full" TUI Theme */

    UiStatusBarFgColor    = COLOR_BLACK;
    UiStatusBarBgColor    = COLOR_CYAN;
    UiBackdropFgColor     = COLOR_WHITE;
    UiBackdropBgColor     = COLOR_BLUE;
    UiBackdropFillStyle   = MEDIUM_FILL;
    UiTitleBoxFgColor     = COLOR_WHITE;
    UiTitleBoxBgColor     = COLOR_RED;
    UiMessageBoxFgColor   = COLOR_WHITE;
    UiMessageBoxBgColor   = COLOR_BLUE;
    UiMenuFgColor         = COLOR_WHITE;
    UiMenuBgColor         = COLOR_BLUE;
    UiTextColor           = COLOR_YELLOW;
    UiSelectedTextColor   = COLOR_BLACK;
    UiSelectedTextBgColor = COLOR_GRAY;
    UiEditBoxTextColor    = COLOR_WHITE;
    UiEditBoxBgColor      = COLOR_BLACK;

    UiShowTime          = TRUE;
    UiMenuBox           = TRUE;
    UiCenterMenu        = TRUE;
    UiUseSpecialEffects = FALSE;

    // TODO: Have a boolean to show/hide title box?
    RtlStringCbCopyA(UiTitleBoxTitleText, sizeof(UiTitleBoxTitleText),
                     "Boot Menu");

    RtlStringCbCopyA(UiTimeText, sizeof(UiTimeText),
                     "[Time Remaining: %d]");

    return TRUE;
}

VOID TuiUnInitialize(VOID)
{
    /* Do nothing if already uninitialized */
    if (!TextVideoBuffer)
        return;

    if (UiUseSpecialEffects)
    {
        TuiFadeOut();
    }
    else
    {
        MachVideoSetDisplayMode(NULL, FALSE);
    }

    VideoFreeOffScreenBuffer();
    TextVideoBuffer = NULL;

    MachVideoClearScreen(ATTR(COLOR_GRAY, COLOR_BLACK));
    MachVideoSetTextCursorPosition(0, 0);
    MachVideoHideShowTextCursor(TRUE);
}

VOID TuiDrawBackdrop(VOID)
{
    /* Fill in the background (excluding title box & status bar) */
    TuiFillArea(0,
                TUI_TITLE_BOX_CHAR_HEIGHT,
                UiScreenWidth - 1,
                UiScreenHeight - 2,
                UiBackdropFillStyle,
                ATTR(UiBackdropFgColor, UiBackdropBgColor));

    /* Draw the title box */
    TuiDrawBox(0,
               0,
               UiScreenWidth - 1,
               TUI_TITLE_BOX_CHAR_HEIGHT - 1,
               D_VERT,
               D_HORZ,
               TRUE,
               FALSE,
               ATTR(UiTitleBoxFgColor, UiTitleBoxBgColor));

    /* Draw version text */
    TuiDrawText(2,
                1,
                VERSION,
                ATTR(UiTitleBoxFgColor, UiTitleBoxBgColor));

    /* Draw copyright */
    TuiDrawText(2,
                2,
                BY_AUTHOR,
                ATTR(UiTitleBoxFgColor, UiTitleBoxBgColor));
    TuiDrawText(2,
                3,
                AUTHOR_EMAIL,
                ATTR(UiTitleBoxFgColor, UiTitleBoxBgColor));

    /* Draw help text */
    TuiDrawText(UiScreenWidth - 16, 3,
                /*"F1 for Help"*/ "F8 for Options",
                ATTR(UiTitleBoxFgColor, UiTitleBoxBgColor));

    /* Draw title text */
    TuiDrawText((UiScreenWidth - (ULONG)strlen(UiTitleBoxTitleText)) / 2,
                2,
                UiTitleBoxTitleText,
                ATTR(UiTitleBoxFgColor, UiTitleBoxBgColor));

    /* Update the date & time */
    TuiUpdateDateTime();
    VideoCopyOffScreenBufferToVRAM();
}

/*
 * FillArea()
 * This function assumes coordinates are zero-based
 */
VOID TuiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr /* Color Attributes */)
{
    PUCHAR ScreenMemory = (PUCHAR)TextVideoBuffer;
    ULONG  i, j;

    /* Clip the area to the screen */
    if ((Left >= UiScreenWidth) || (Top >= UiScreenHeight))
    {
        return;
    }
    if (Right >= UiScreenWidth)
        Right = UiScreenWidth - 1;
    if (Bottom >= UiScreenHeight)
        Bottom = UiScreenHeight - 1;

    /* Loop through each line and column and fill it in */
    for (i = Top; i <= Bottom; ++i)
    {
        for (j = Left; j <= Right; ++j)
        {
            ScreenMemory[((i*2)*UiScreenWidth)+(j*2)] = (UCHAR)FillChar;
            ScreenMemory[((i*2)*UiScreenWidth)+(j*2)+1] = Attr;
        }
    }
}

/*
 * DrawShadow()
 * This function assumes coordinates are zero-based
 */
VOID TuiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom)
{
    PUCHAR ScreenMemory = (PUCHAR)TextVideoBuffer;
    ULONG  i;
    BOOLEAN RightShadow = (Right < (UiScreenWidth - 1));
    BOOLEAN DoubleRightShadow = ((Right + 1) < (UiScreenWidth - 1));
    BOOLEAN BottomShadow = (Bottom < (UiScreenHeight - 1));
    BOOLEAN DoubleWidth = (UiScreenHeight < 34);

    /* Cap the right and bottom borders */
    Right = min(Right, UiScreenWidth - 1);
    Bottom = min(Bottom, UiScreenHeight - 1);

    /* Shade the bottom of the area */
    if (BottomShadow)
    {
        i = Left + (DoubleWidth ? 2 : 1);
        for (; i <= Right; ++i)
        {
            ScreenMemory[(((Bottom+1)*2)*UiScreenWidth)+(i*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
        }
    }

    /* Shade the right of the area */
    if (RightShadow)
    {
        for (i = Top + 1; i <= Bottom; ++i)
        {
            ScreenMemory[((i*2)*UiScreenWidth)+((Right+1)*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
        }
    }
    if (DoubleWidth && DoubleRightShadow)
    {
        for (i = Top + 1; i <= Bottom; ++i)
        {
            ScreenMemory[((i*2)*UiScreenWidth)+((Right+2)*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
        }
    }

    /* Shade the bottom right corner */
    if (RightShadow && BottomShadow)
    {
        ScreenMemory[(((Bottom+1)*2)*UiScreenWidth)+((Right+1)*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
    }
    if (DoubleWidth && DoubleRightShadow && BottomShadow)
    {
        ScreenMemory[(((Bottom+1)*2)*UiScreenWidth)+((Right+2)*2)+1] = ATTR(COLOR_GRAY, COLOR_BLACK);
    }
}

/*
 * DrawBox()
 * This function assumes coordinates are zero-based
 */
VOID
TuiDrawBoxTopLine(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ UCHAR VertStyle,
    _In_ UCHAR HorzStyle,
    _In_ UCHAR Attr)
{
    UCHAR ULCorner, URCorner;

    /* Calculate the corner values */
    if (HorzStyle == HORZ)
    {
        if (VertStyle == VERT)
        {
            ULCorner = UL;
            URCorner = UR;
        }
        else // VertStyle == D_VERT
        {
            ULCorner = VD_UL;
            URCorner = VD_UR;
        }
    }
    else // HorzStyle == D_HORZ
    {
        if (VertStyle == VERT)
        {
            ULCorner = HD_UL;
            URCorner = HD_UR;
        }
        else // VertStyle == D_VERT
        {
            ULCorner = D_UL;
            URCorner = D_UR;
        }
    }

    TuiFillArea(Left, Top, Left, Top, ULCorner, Attr);
    TuiFillArea(Left+1, Top, Right-1, Top, HorzStyle, Attr);
    TuiFillArea(Right, Top, Right, Top, URCorner, Attr);
}

VOID
TuiDrawBoxBottomLine(
    _In_ ULONG Left,
    _In_ ULONG Bottom,
    _In_ ULONG Right,
    _In_ UCHAR VertStyle,
    _In_ UCHAR HorzStyle,
    _In_ UCHAR Attr)
{
    UCHAR LLCorner, LRCorner;

    /* Calculate the corner values */
    if (HorzStyle == HORZ)
    {
        if (VertStyle == VERT)
        {
            LLCorner = LL;
            LRCorner = LR;
        }
        else // VertStyle == D_VERT
        {
            LLCorner = VD_LL;
            LRCorner = VD_LR;
        }
    }
    else // HorzStyle == D_HORZ
    {
        if (VertStyle == VERT)
        {
            LLCorner = HD_LL;
            LRCorner = HD_LR;
        }
        else // VertStyle == D_VERT
        {
            LLCorner = D_LL;
            LRCorner = D_LR;
        }
    }

    TuiFillArea(Left, Bottom, Left, Bottom, LLCorner, Attr);
    TuiFillArea(Left+1, Bottom, Right-1, Bottom, HorzStyle, Attr);
    TuiFillArea(Right, Bottom, Right, Bottom, LRCorner, Attr);
}

VOID
TuiDrawBox(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ UCHAR VertStyle,
    _In_ UCHAR HorzStyle,
    _In_ BOOLEAN Fill,
    _In_ BOOLEAN Shadow,
    _In_ UCHAR Attr)
{
    /* Fill in the box background */
    if (Fill)
        TuiFillArea(Left, Top, Right, Bottom, ' ', Attr);

    /* Fill in the top horizontal line */
    TuiDrawBoxTopLine(Left, Top, Right, VertStyle, HorzStyle, Attr);

    /* Fill in the vertical left and right lines */
    TuiFillArea(Left, Top+1, Left, Bottom-1, VertStyle, Attr);
    TuiFillArea(Right, Top+1, Right, Bottom-1, VertStyle, Attr);

    /* Fill in the bottom horizontal line */
    TuiDrawBoxBottomLine(Left, Bottom, Right, VertStyle, HorzStyle, Attr);

    /* Draw the shadow */
    if (Shadow)
        TuiDrawShadow(Left, Top, Right, Bottom);
}

VOID TuiDrawStatusText(PCSTR StatusText)
{
    SIZE_T    i;

    TuiDrawText(0, UiScreenHeight-1, " ", ATTR(UiStatusBarFgColor, UiStatusBarBgColor));
    TuiDrawText(1, UiScreenHeight-1, StatusText, ATTR(UiStatusBarFgColor, UiStatusBarBgColor));

    for (i=strlen(StatusText)+1; i<UiScreenWidth; i++)
    {
        TuiDrawText((ULONG)i, UiScreenHeight-1, " ", ATTR(UiStatusBarFgColor, UiStatusBarBgColor));
    }

    VideoCopyOffScreenBufferToVRAM();
}

VOID TuiUpdateDateTime(VOID)
{
    TIMEINFO* TimeInfo;
    PCSTR   DayPostfix;
    BOOLEAN PMHour = FALSE;
    CHAR Buffer[40];

    /* Don't draw the time if this has been disabled */
    if (!UiShowTime) return;

    TimeInfo = ArcGetTime();
    if (TimeInfo->Year < 1 || 9999 < TimeInfo->Year ||
        TimeInfo->Month < 1 || 12 < TimeInfo->Month ||
        TimeInfo->Day < 1 || 31 < TimeInfo->Day ||
        23 < TimeInfo->Hour ||
        59 < TimeInfo->Minute ||
        59 < TimeInfo->Second)
    {
        /* This happens on QEmu sometimes. We just skip updating. */
        return;
    }

    /* Get the day postfix */
    if (1 == TimeInfo->Day || 21 == TimeInfo->Day || 31 == TimeInfo->Day)
        DayPostfix = "st";
    else if (2 == TimeInfo->Day || 22 == TimeInfo->Day)
        DayPostfix = "nd";
    else if (3 == TimeInfo->Day || 23 == TimeInfo->Day)
        DayPostfix = "rd";
    else
        DayPostfix = "th";

    /* Build the date string in format: "MMMM dx yyyy" */
    RtlStringCbPrintfA(Buffer, sizeof(Buffer),
                       "%s %d%s %d",
                       UiMonthNames[TimeInfo->Month - 1],
                       TimeInfo->Day,
                       DayPostfix,
                       TimeInfo->Year);

    /* Draw the date */
    TuiDrawText(UiScreenWidth - (ULONG)strlen(Buffer) - 2, 1,
                Buffer, ATTR(UiTitleBoxFgColor, UiTitleBoxBgColor));

    /* Get the hour and change from 24-hour mode to 12-hour */
    if (TimeInfo->Hour > 12)
    {
        TimeInfo->Hour -= 12;
        PMHour = TRUE;
    }
    if (TimeInfo->Hour == 0)
    {
        TimeInfo->Hour = 12;
    }

    /* Build the time string in format: "h:mm:ss tt" */
    RtlStringCbPrintfA(Buffer, sizeof(Buffer),
                       "  %d:%02d:%02d %s",
                       TimeInfo->Hour,
                       TimeInfo->Minute,
                       TimeInfo->Second,
                       PMHour ? "PM" : "AM");

    /* Draw the time */
    TuiDrawText(UiScreenWidth - (ULONG)strlen(Buffer) - 2, 2,
                Buffer, ATTR(UiTitleBoxFgColor, UiTitleBoxBgColor));
}

_Ret_maybenull_
__drv_allocatesMem(Mem)
PUCHAR
TuiSaveScreen(VOID)
{
    PUCHAR Buffer;
    PUCHAR ScreenMemory = (PUCHAR)TextVideoBuffer;
    ULONG i;

    /* Allocate the buffer */
    Buffer = FrLdrTempAlloc(UiScreenWidth * UiScreenHeight * 2,
                            TAG_TUI_SCREENBUFFER);
    if (!Buffer)
        return NULL;

    /* Loop through each cell and copy it */
    for (i=0; i < (UiScreenWidth * UiScreenHeight * 2); i++)
    {
        Buffer[i] = ScreenMemory[i];
    }

    return Buffer;
}

VOID
TuiRestoreScreen(
    _In_opt_ __drv_freesMem(Mem) PUCHAR Buffer)
{
    PUCHAR ScreenMemory = (PUCHAR)TextVideoBuffer;
    ULONG i;

    if (!Buffer)
        return;

    /* Loop through each cell and copy it */
    for (i=0; i < (UiScreenWidth * UiScreenHeight * 2); i++)
    {
        ScreenMemory[i] = Buffer[i];
    }

    /* Free the buffer */
    FrLdrTempFree(Buffer, TAG_TUI_SCREENBUFFER);

    VideoCopyOffScreenBufferToVRAM();
}

static VOID
TuiDrawMsgBoxCommon(
    _In_ PCSTR MessageText,
    _Out_ PSMALL_RECT MsgBoxRect)
{
    INT width = 8;
    INT height = 1;
    INT curline = 0;
    INT k;
    size_t i, j;
    INT x1, x2, y1, y2;
    CHAR temp[260];

    /* Find the height */
    for (i = 0; i < strlen(MessageText); i++)
    {
        if (MessageText[i] == '\n')
            height++;
    }

    /* Find the width */
    for (i = j = k = 0; i < height; i++)
    {
        while ((MessageText[j] != '\n') && (MessageText[j] != ANSI_NULL))
        {
            j++;
            k++;
        }

        if (k > width)
            width = k;

        k = 0;
        j++;
    }

    /* Account for the message box margins & bottom button/edit box */
    width  += 4; // Border & space on left and right.
    height += 5; // Border on top and bottom, plus 3 lines for button/edit box.

    /* Calculate the centered box area, also ensuring that the top-left
     * corner is always visible if the borders are partly off-screen */
    x1 = (UiScreenWidth - min(width, UiScreenWidth)) / 2;
    if (UiCenterMenu && (height <= UiScreenHeight - TUI_TITLE_BOX_CHAR_HEIGHT - 1))
    {
        /* Exclude the header and the status bar */
        // y1 = (UiScreenHeight - TUI_TITLE_BOX_CHAR_HEIGHT - 1 - height) / 2
        //      + TUI_TITLE_BOX_CHAR_HEIGHT;
        y1 = (UiScreenHeight + TUI_TITLE_BOX_CHAR_HEIGHT - 1 - height) / 2;
    }
    else
    {
        y1 = (UiScreenHeight - min(height, UiScreenHeight)) / 2;
    }
    x2 = x1 + width - 1;
    y2 = y1 + height - 1;

    MsgBoxRect->Left = x1; MsgBoxRect->Right  = x2;
    MsgBoxRect->Top  = y1; MsgBoxRect->Bottom = y2;


    /* Draw the box */
    TuiDrawBox(x1, y1, x2, y2, D_VERT, D_HORZ, TRUE, TRUE,
               ATTR(UiMessageBoxFgColor, UiMessageBoxBgColor));

    /* Draw the text */
    for (i = j = 0; i < strlen(MessageText) + 1; i++)
    {
        if ((MessageText[i] == '\n') || (MessageText[i] == ANSI_NULL))
        {
            temp[j] = 0;
            j = 0;
            UiDrawText(x1 + 2, y1 + 1 + curline, temp,
                       ATTR(UiMessageBoxFgColor, UiMessageBoxBgColor));
            curline++;
        }
        else
        {
            temp[j++] = MessageText[i];
        }
    }
}

VOID
TuiMessageBox(
    _In_ PCSTR MessageText)
{
    PVOID ScreenBuffer;

    /* Save the screen contents */
    ScreenBuffer = TuiSaveScreen();

    /* Display the message box */
    TuiMessageBoxCritical(MessageText);

    /* Restore the screen contents */
    TuiRestoreScreen(ScreenBuffer);
}

VOID
TuiMessageBoxCritical(
    _In_ PCSTR MessageText)
{
    SMALL_RECT BoxRect;
    CHAR key;

    /* Draw the common parts of the message box */
    TuiDrawMsgBoxCommon(MessageText, &BoxRect);

    /* Draw centered OK button */
    UiDrawText((BoxRect.Left + BoxRect.Right) / 2 - 3,
               BoxRect.Bottom - 2,
               "   OK   ",
               ATTR(COLOR_BLACK, COLOR_GRAY));

    /* Draw status text */
    UiDrawStatusText("Press ENTER to continue");

    VideoCopyOffScreenBufferToVRAM();

    for (;;)
    {
        if (MachConsKbHit())
        {
            key = MachConsGetCh();
            if (key == KEY_EXTENDED)
                key = MachConsGetCh();

            if ((key == KEY_ENTER) || (key == KEY_SPACE) || (key == KEY_ESC))
                break;
        }

        TuiUpdateDateTime();

        VideoCopyOffScreenBufferToVRAM();

        MachHwIdle();
    }
}

static VOID
TuiSetProgressBarText(
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
                UiProgressBar.Right, UiProgressBar.Bottom - 1,
                ' ', ATTR(UiTextColor, UiMenuBgColor));

    /* Draw the "Loading..." text */
    TuiDrawCenteredText(UiProgressBar.Left, UiProgressBar.Top,
                        UiProgressBar.Right, UiProgressBar.Bottom - 1,
                        ProgressString, ATTR(UiTextColor, UiMenuBgColor));
}

static VOID
TuiTickProgressBar(
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
    /* Fill the remaining with shadow blanks */
    TuiFillArea(UiProgressBar.Left + FillCount, UiProgressBar.Bottom,
                UiProgressBar.Right, UiProgressBar.Bottom,
                '\xB2', ATTR(UiTextColor, UiMenuBgColor));

    TuiUpdateDateTime();
    VideoCopyOffScreenBufferToVRAM();
}

static VOID
TuiDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR ProgressText);

static VOID
TuiDrawProgressBarCenter(
    _In_ PCSTR ProgressText)
{
    ULONG Left, Top, Right, Bottom, Width, Height;

    /* Build the coordinates and sizes */
    Height = 2;
    Width  = 50; // Allow for 50 "bars"
    Left = (UiScreenWidth - Width) / 2;
    Top  = (UiScreenHeight - Height + 4) / 2;
    Right  = Left + Width - 1;
    Bottom = Top + Height - 1;

    /* Inflate to include the box margins */
    Left -= 2;
    Right += 2;
    Top -= 1;
    Bottom += 1;

    /* Draw the progress bar */
    TuiDrawProgressBar(Left, Top, Right, Bottom, ProgressText);
}

static VOID
TuiDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR ProgressText)
{
    /* Draw the box */
    TuiDrawBox(Left, Top, Right, Bottom,
               VERT, HORZ, TRUE, TRUE,
               ATTR(UiMenuFgColor, UiMenuBgColor));

    /* Exclude the box margins */
    Left += 2;
    Right -= 2;
    Top += 1;
    Bottom -= 1;

    UiInitProgressBar(Left, Top, Right, Bottom, ProgressText);
}

UCHAR TuiTextToColor(PCSTR ColorText)
{
    static const struct
    {
        PCSTR ColorName;
        UCHAR ColorValue;
    } Colors[] =
    {
        {"Black"  , COLOR_BLACK  },
        {"Blue"   , COLOR_BLUE   },
        {"Green"  , COLOR_GREEN  },
        {"Cyan"   , COLOR_CYAN   },
        {"Red"    , COLOR_RED    },
        {"Magenta", COLOR_MAGENTA},
        {"Brown"  , COLOR_BROWN  },
        {"Gray"   , COLOR_GRAY   },
        {"DarkGray"    , COLOR_DARKGRAY    },
        {"LightBlue"   , COLOR_LIGHTBLUE   },
        {"LightGreen"  , COLOR_LIGHTGREEN  },
        {"LightCyan"   , COLOR_LIGHTCYAN   },
        {"LightRed"    , COLOR_LIGHTRED    },
        {"LightMagenta", COLOR_LIGHTMAGENTA},
        {"Yellow"      , COLOR_YELLOW      },
        {"White"       , COLOR_WHITE       },
    };
    ULONG i;

    if (_stricmp(ColorText, "Default") == 0)
        return MachDefaultTextColor;

    for (i = 0; i < RTL_NUMBER_OF(Colors); ++i)
    {
        if (_stricmp(ColorText, Colors[i].ColorName) == 0)
            return Colors[i].ColorValue;
    }

    return COLOR_BLACK;
}

UCHAR TuiTextToFillStyle(PCSTR FillStyleText)
{
    static const struct
    {
        PCSTR FillStyleName;
        UCHAR FillStyleValue;
    } FillStyles[] =
    {
        {"None"  , ' '},
        {"Light" , LIGHT_FILL },
        {"Medium", MEDIUM_FILL},
        {"Dark"  , DARK_FILL  },
    };
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(FillStyles); ++i)
    {
        if (_stricmp(FillStyleText, FillStyles[i].FillStyleName) == 0)
            return FillStyles[i].FillStyleValue;
    }

    return LIGHT_FILL;
}

VOID TuiFadeInBackdrop(VOID)
{
    PPALETTE_ENTRY TuiFadePalette = NULL;

    if (UiUseSpecialEffects && ! MachVideoIsPaletteFixed())
    {
        TuiFadePalette = (PPALETTE_ENTRY)FrLdrTempAlloc(sizeof(PALETTE_ENTRY) * 64,
                                                        TAG_TUI_PALETTE);

        if (TuiFadePalette != NULL)
        {
            VideoSavePaletteState(TuiFadePalette, 64);
            VideoSetAllColorsToBlack(64);
        }
    }

    // Draw the backdrop and title box
    TuiDrawBackdrop();

    if (UiUseSpecialEffects && ! MachVideoIsPaletteFixed() && TuiFadePalette != NULL)
    {
        VideoFadeIn(TuiFadePalette, 64);
        FrLdrTempFree(TuiFadePalette, TAG_TUI_PALETTE);
    }
}

VOID TuiFadeOut(VOID)
{
    PPALETTE_ENTRY TuiFadePalette = NULL;

    if (UiUseSpecialEffects && ! MachVideoIsPaletteFixed())
    {
        TuiFadePalette = (PPALETTE_ENTRY)FrLdrTempAlloc(sizeof(PALETTE_ENTRY) * 64,
                                                        TAG_TUI_PALETTE);

        if (TuiFadePalette != NULL)
        {
            VideoSavePaletteState(TuiFadePalette, 64);
        }
    }

    if (UiUseSpecialEffects && ! MachVideoIsPaletteFixed() && TuiFadePalette != NULL)
    {
        VideoFadeOut(64);
    }

    MachVideoSetDisplayMode(NULL, FALSE);

    if (UiUseSpecialEffects && ! MachVideoIsPaletteFixed() && TuiFadePalette != NULL)
    {
        VideoRestorePaletteState(TuiFadePalette, 64);
        FrLdrTempFree(TuiFadePalette, TAG_TUI_PALETTE);
    }

}

BOOLEAN TuiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length)
{
    CHAR    key;
    BOOLEAN Extended;
    INT     EditBoxLine;
    ULONG   EditBoxStartX, EditBoxEndX;
    INT     EditBoxCursorX;
    ULONG   EditBoxTextLength, EditBoxTextPosition;
    INT     EditBoxTextDisplayIndex;
    BOOLEAN ReturnCode;
    SMALL_RECT BoxRect;
    PVOID ScreenBuffer;

    /* Save the screen contents */
    ScreenBuffer = TuiSaveScreen();

    /* Draw the common parts of the message box */
    TuiDrawMsgBoxCommon(MessageText, &BoxRect);

    EditBoxTextLength = (ULONG)strlen(EditTextBuffer);
    EditBoxTextLength = min(EditBoxTextLength, Length - 1);
    EditBoxTextPosition = 0;
    EditBoxLine = BoxRect.Bottom - 2;
    EditBoxStartX = BoxRect.Left + 3;
    EditBoxEndX = BoxRect.Right - 3;

    // Draw the edit box background and the text
    UiFillArea(EditBoxStartX, EditBoxLine, EditBoxEndX, EditBoxLine, ' ', ATTR(UiEditBoxTextColor, UiEditBoxBgColor));
    UiDrawText2(EditBoxStartX, EditBoxLine, EditBoxEndX - EditBoxStartX + 1, EditTextBuffer, ATTR(UiEditBoxTextColor, UiEditBoxBgColor));

    // Show the cursor
    EditBoxCursorX = EditBoxStartX;
    MachVideoSetTextCursorPosition(EditBoxCursorX, EditBoxLine);
    MachVideoHideShowTextCursor(TRUE);

    // Draw status text
    UiDrawStatusText("Press ENTER to continue, or ESC to cancel");

    VideoCopyOffScreenBufferToVRAM();

    //
    // Enter the text. Please keep in mind that the default input mode
    // of the edit boxes is in insertion mode, that is, you can insert
    // text without erasing the existing one.
    //
    for (;;)
    {
        if (MachConsKbHit())
        {
            Extended = FALSE;
            key = MachConsGetCh();
            if (key == KEY_EXTENDED)
            {
                Extended = TRUE;
                key = MachConsGetCh();
            }

            if (key == KEY_ENTER)
            {
                ReturnCode = TRUE;
                break;
            }
            else if (key == KEY_ESC)
            {
                ReturnCode = FALSE;
                break;
            }
            else if (key == KEY_BACKSPACE) // Remove a character
            {
                if ( (EditBoxTextLength > 0) && (EditBoxTextPosition > 0) &&
                     (EditBoxTextPosition <= EditBoxTextLength) )
                {
                    EditBoxTextPosition--;
                    memmove(EditTextBuffer + EditBoxTextPosition,
                            EditTextBuffer + EditBoxTextPosition + 1,
                            EditBoxTextLength - EditBoxTextPosition);
                    EditBoxTextLength--;
                    EditTextBuffer[EditBoxTextLength] = 0;
                }
                else
                {
                    MachBeep();
                }
            }
            else if (Extended && key == KEY_DELETE) // Remove a character
            {
                if ( (EditBoxTextLength > 0) &&
                     (EditBoxTextPosition < EditBoxTextLength) )
                {
                    memmove(EditTextBuffer + EditBoxTextPosition,
                            EditTextBuffer + EditBoxTextPosition + 1,
                            EditBoxTextLength - EditBoxTextPosition);
                    EditBoxTextLength--;
                    EditTextBuffer[EditBoxTextLength] = 0;
                }
                else
                {
                    MachBeep();
                }
            }
            else if (Extended && key == KEY_HOME) // Go to the start of the buffer
            {
                EditBoxTextPosition = 0;
            }
            else if (Extended && key == KEY_END) // Go to the end of the buffer
            {
                EditBoxTextPosition = EditBoxTextLength;
            }
            else if (Extended && key == KEY_RIGHT) // Go right
            {
                if (EditBoxTextPosition < EditBoxTextLength)
                    EditBoxTextPosition++;
                else
                    MachBeep();
            }
            else if (Extended && key == KEY_LEFT) // Go left
            {
                if (EditBoxTextPosition > 0)
                    EditBoxTextPosition--;
                else
                    MachBeep();
            }
            else if (!Extended) // Add this key to the buffer
            {
                if ( (EditBoxTextLength   < Length - 1) &&
                     (EditBoxTextPosition < Length - 1) )
                {
                    memmove(EditTextBuffer + EditBoxTextPosition + 1,
                            EditTextBuffer + EditBoxTextPosition,
                            EditBoxTextLength - EditBoxTextPosition);
                    EditTextBuffer[EditBoxTextPosition] = key;
                    EditBoxTextPosition++;
                    EditBoxTextLength++;
                    EditTextBuffer[EditBoxTextLength] = 0;
                }
                else
                {
                    MachBeep();
                }
            }
            else
            {
                MachBeep();
            }
        }

        // Draw the edit box background
        UiFillArea(EditBoxStartX, EditBoxLine, EditBoxEndX, EditBoxLine, ' ', ATTR(UiEditBoxTextColor, UiEditBoxBgColor));

        // Fill the text in
        if (EditBoxTextPosition > (EditBoxEndX - EditBoxStartX))
        {
            EditBoxTextDisplayIndex = EditBoxTextPosition - (EditBoxEndX - EditBoxStartX);
            EditBoxCursorX = EditBoxEndX;
        }
        else
        {
            EditBoxTextDisplayIndex = 0;
            EditBoxCursorX = EditBoxStartX + EditBoxTextPosition;
        }
        UiDrawText2(EditBoxStartX, EditBoxLine, EditBoxEndX - EditBoxStartX + 1, &EditTextBuffer[EditBoxTextDisplayIndex], ATTR(UiEditBoxTextColor, UiEditBoxBgColor));

        // Move the cursor
        MachVideoSetTextCursorPosition(EditBoxCursorX, EditBoxLine);

        TuiUpdateDateTime();

        VideoCopyOffScreenBufferToVRAM();

        MachHwIdle();
    }

    // Hide the cursor again
    MachVideoHideShowTextCursor(FALSE);

    /* Restore the screen contents */
    TuiRestoreScreen(ScreenBuffer);

    return ReturnCode;
}

const UIVTBL TuiVtbl =
{
    TuiInitialize,
    TuiUnInitialize,
    TuiDrawBackdrop,
    TuiFillArea,
    TuiDrawShadow,
    TuiDrawBox,
    TuiDrawText,
    TuiDrawText2,
    TuiDrawCenteredText,
    TuiDrawStatusText,
    TuiUpdateDateTime,
    TuiMessageBox,
    TuiMessageBoxCritical,
    TuiDrawProgressBarCenter,
    TuiDrawProgressBar,
    TuiSetProgressBarText,
    TuiTickProgressBar,
    TuiEditBox,
    TuiTextToColor,
    TuiTextToFillStyle,
    TuiFadeInBackdrop,
    TuiFadeOut,
    TuiDisplayMenu,
    TuiDrawMenu,
};
