/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    resize.c

Abstract:

        This file implements window resizing.

Author:

    Therese Stowell (thereses) 6-Oct-1991

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop


VOID
CalculateNewSize(
    IN PBOOLEAN MaximizedX,
    IN PBOOLEAN MaximizedY,
    IN OUT PSHORT DeltaX,
    IN OUT PSHORT DeltaY,
    IN SHORT WindowSizeX,
    IN SHORT WindowSizeY,
    IN COORD ScreenBufferSize,
    IN COORD FontSize
    );

VOID
ProcessResizeWindow(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PCONSOLE_INFORMATION Console,
    IN LPWINDOWPOS WindowPos
    )
{
    SHORT DeltaX,DeltaY;
    SHORT PixelDeltaX,PixelDeltaY;
    DWORD Flags=0;
    COORD FontSize;

#ifdef THERESES_DEBUG
DbgPrint("WM_WINDOWPOSCHANGING message ");
DbgPrint("  WindowSize is %d %d\n",CONSOLE_WINDOW_SIZE_X(ScreenInfo),CONSOLE_WINDOW_SIZE_Y(ScreenInfo));
DbgPrint("  WindowRect is %d %d %d %d\n",Console->WindowRect.left,
                                         Console->WindowRect.top,
                                         Console->WindowRect.right,
                                         Console->WindowRect.bottom);
DbgPrint("  window pos is %d %d %d %d\n",WindowPos->x,
                                         WindowPos->y,
                                         WindowPos->cx,
                                         WindowPos->cy);
#endif

    //
    // If the window is not being resized, don't do anything
    //

    if (WindowPos->flags & SWP_NOSIZE) {
        return;
    }

    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        FontSize = SCR_FONTSIZE(ScreenInfo);
    } else {
        FontSize.X = 1;
        FontSize.Y = 1;
    }

    //
    // If the frame changed, update the system metrics
    //

    if (WindowPos->flags & SWP_FRAMECHANGED) {

        InitializeSystemMetrics();
        if (Console->VerticalClientToWindow != VerticalClientToWindow ||
            Console->HorizontalClientToWindow != HorizontalClientToWindow) {

            Console->VerticalClientToWindow = VerticalClientToWindow;
            Console->HorizontalClientToWindow = HorizontalClientToWindow;
            Console->WindowRect.left = WindowPos->x;
            Console->WindowRect.top = WindowPos->y;
            Console->WindowRect.right = WindowPos->x + WindowPos->cx;
            Console->WindowRect.bottom = WindowPos->y + WindowPos->cy;
            return;
        }
    }

    PixelDeltaX = (SHORT)(WindowPos->cx - (Console->WindowRect.right - Console->WindowRect.left));
    PixelDeltaY = (SHORT)(WindowPos->cy - (Console->WindowRect.bottom - Console->WindowRect.top));

    if (WindowPos->cx >= (ScreenInfo->ScreenBufferSize.X * FontSize.X + VerticalClientToWindow) &&
        WindowPos->cy >= (ScreenInfo->ScreenBufferSize.Y * FontSize.Y + HorizontalClientToWindow)) {

        //
        // handle maximized case
        //

        ScreenInfo->WindowMaximizedX = TRUE;
        ScreenInfo->WindowMaximizedY = TRUE;
        DeltaX = (SHORT)(ScreenInfo->ScreenBufferSize.X - CONSOLE_WINDOW_SIZE_X(ScreenInfo));
        DeltaY = (SHORT)(ScreenInfo->ScreenBufferSize.Y - CONSOLE_WINDOW_SIZE_Y(ScreenInfo));
    } else {

        DeltaX = PixelDeltaX / FontSize.X;
        DeltaY = PixelDeltaY / FontSize.Y;

        //
        // the only time we will get a WM_WINDOWPOSCHANGING message to grow the
        // window larger than the maximum window size is when another app calls
        // SetWindowPos for our window.  the program manager does that when
        // the user requests Tiling.
        //

        CalculateNewSize(&ScreenInfo->WindowMaximizedX,
                         &ScreenInfo->WindowMaximizedY,
                         &DeltaX,
                         &DeltaY,
                         (SHORT)(CONSOLE_WINDOW_SIZE_X(ScreenInfo)),
                         (SHORT)(CONSOLE_WINDOW_SIZE_Y(ScreenInfo)),
                         ScreenInfo->ScreenBufferSize,
                         FontSize
                        );
#ifdef THERESES_DEBUG
DbgPrint("Delta X Y is now %d %d\n",DeltaX,DeltaY);
DbgPrint("Maximized X Y is now %d %d\n",ScreenInfo->WindowMaximizedX,ScreenInfo->WindowMaximizedY);
#endif
    }

    //
    // don't move window when resizing less than a column or row.
    //

    if (!DeltaX && !DeltaY && (PixelDeltaX || PixelDeltaY)) {
        COORD OriginDifference;

        //
        // handle tiling case.  tiling can move the window without resizing, but using
        // a size message. we detect this by checking for the window origin changed by
        // more than one character.
        //

        OriginDifference.X = (SHORT)(WindowPos->x - Console->WindowRect.left);
        OriginDifference.Y = (SHORT)(WindowPos->y - Console->WindowRect.top);
        if (OriginDifference.X < FontSize.X && OriginDifference.X > -FontSize.X &&
            OriginDifference.Y < FontSize.Y && OriginDifference.Y > -FontSize.Y) {
            WindowPos->x = Console->WindowRect.left;
            WindowPos->y = Console->WindowRect.top;
            WindowPos->cx = Console->WindowRect.right - Console->WindowRect.left;
            WindowPos->cy = Console->WindowRect.bottom - Console->WindowRect.top;
            return;
        }
    }

    Flags |= RESIZE_SCROLL_BARS;
    WindowPos->cx = (DeltaX + CONSOLE_WINDOW_SIZE_X(ScreenInfo)) * FontSize.X + (!ScreenInfo->WindowMaximizedY * VerticalScrollSize) + VerticalClientToWindow;
    WindowPos->cy = (DeltaY + CONSOLE_WINDOW_SIZE_Y(ScreenInfo)) * FontSize.Y + (!ScreenInfo->WindowMaximizedX * HorizontalScrollSize) + HorizontalClientToWindow;

    //
    // reflect the new window size in the
    // console window structure
    //

    {
    SHORT ScrollRange,ScrollPos;

    //
    // PercentFromTop = ScrollPos / ScrollRange;
    // PercentFromBottom = (ScrollRange - ScrollPos) / ScrollRange;
    //
    // if drag top border up
    //     Window.Top -= NumLines * PercentFromBottom;
    //     Window.Bottom +=  NumLines - (NumLines * PercentFromBottom);
    //
    // if drag top border down
    //     Window.Top += NumLines * PercentFromBottom;
    //     Window.Bottom -=  NumLines - (NumLines * PercentFromBottom);
    //
    // if drag bottom border up
    //     Window.Top -= NumLines * PercentFromTop;
    //     Window.Bottom +=  NumLines - (NumLines * PercentFromTop);
    //
    // if drag bottom border down
    //     Window.Top += NumLines * PercentFromTop;
    //     Window.Bottom -=  NumLines - (NumLines * PercentFromTop);
    //

    ScrollRange = (SHORT)(ScreenInfo->ScreenBufferSize.X - CONSOLE_WINDOW_SIZE_X(ScreenInfo));
    ScrollPos = ScreenInfo->Window.Left;

    if (WindowPos->x != Console->WindowRect.left) {
        SHORT NumLinesFromRight;
        if (ScrollRange) {
            NumLinesFromRight = DeltaX * (ScrollRange - ScrollPos) / ScrollRange;
        } else {
            NumLinesFromRight = DeltaX; // have scroll pos at left edge
        }
        ScreenInfo->Window.Left -= DeltaX - NumLinesFromRight;
        ScreenInfo->Window.Right += NumLinesFromRight;
    } else {
        SHORT NumLinesFromLeft;
        if (ScrollRange) {
            NumLinesFromLeft = DeltaX * ScrollPos / ScrollRange;
        } else {
            NumLinesFromLeft = 0;   // have scroll pos at left edge
        }
        ScreenInfo->Window.Left -= NumLinesFromLeft;
        ScreenInfo->Window.Right += DeltaX - NumLinesFromLeft;
    }

    ScrollRange = (SHORT)(ScreenInfo->ScreenBufferSize.Y - CONSOLE_WINDOW_SIZE_Y(ScreenInfo));
    ScrollPos = ScreenInfo->Window.Top;
    if (WindowPos->y != Console->WindowRect.top) {
        SHORT NumLinesFromBottom;
        if (ScrollRange) {
            NumLinesFromBottom = DeltaY * (ScrollRange - ScrollPos) / ScrollRange;
        } else {
            NumLinesFromBottom = DeltaY; // have scroll pos at top edge
        }
        ScreenInfo->Window.Top -= DeltaY - NumLinesFromBottom;
        ScreenInfo->Window.Bottom += NumLinesFromBottom;
    } else {
        SHORT NumLinesFromTop;
        if (ScrollRange) {
            NumLinesFromTop = DeltaY * ScrollPos / ScrollRange;
        } else {
            NumLinesFromTop = 0;   // have scroll pos at top edge
        }
        ScreenInfo->Window.Top -= NumLinesFromTop;
        ScreenInfo->Window.Bottom += DeltaY - NumLinesFromTop;
    }
    }

    if (ScreenInfo->WindowMaximizedX)
        ASSERT (CONSOLE_WINDOW_SIZE_X(ScreenInfo) == ScreenInfo->ScreenBufferSize.X);
    if (ScreenInfo->WindowMaximizedY)
        ASSERT (CONSOLE_WINDOW_SIZE_Y(ScreenInfo) == ScreenInfo->ScreenBufferSize.Y);
#ifdef THERESES_DEBUG
DbgPrint("  WindowSize is now %d %d\n",CONSOLE_WINDOW_SIZE_X(ScreenInfo),CONSOLE_WINDOW_SIZE_Y(ScreenInfo));
DbgPrint("  window pos is now %d %d %d %d\n",WindowPos->x,
                                         WindowPos->y,
                                         WindowPos->cx,
                                         WindowPos->cy);
#endif
    Console->ResizeFlags = Flags | (Console->ResizeFlags & SCREEN_BUFFER_CHANGE);
}



VOID
CalculateNewSize(
    IN PBOOLEAN MaximizedX,
    IN PBOOLEAN MaximizedY,
    IN OUT PSHORT DeltaX,
    IN OUT PSHORT DeltaY,
    IN SHORT WindowSizeX,
    IN SHORT WindowSizeY,
    IN COORD ScreenBufferSize,
    IN COORD FontSize
    )
{
    SHORT MaxDeltaX = ScreenBufferSize.X - WindowSizeX;
    SHORT MaxDeltaY = ScreenBufferSize.Y - WindowSizeY;
    SHORT MinDeltaX = 1 - WindowSizeX;
    SHORT MinDeltaY = 1 - WindowSizeY;

    while (TRUE) {

        /*
         * Do we need to remove a horizontal scroll bar?
         */
        if (!*MaximizedX && *DeltaX >= MaxDeltaX) {
            *MaximizedX = TRUE;
            *DeltaY += (VerticalScrollSize+FontSize.Y-1) / FontSize.Y;
        }

        /*
         * Do we need to remove a vertical scroll bar?
         */
        else if (!*MaximizedY && *DeltaY >= MaxDeltaY) {
            *MaximizedY = TRUE;
            *DeltaX += (HorizontalScrollSize+FontSize.X-1) / FontSize.X;
        }

        /*
         * Do we need to add a horizontal scroll bar?
         */
        else if (*MaximizedX && *DeltaX < MaxDeltaX) {
            *MaximizedX = FALSE;
            *DeltaY -= (VerticalScrollSize+FontSize.Y-1) / FontSize.Y;
        }

        /*
         * Do we need to add a vertical scroll bar?
         */
        else if (*MaximizedY && *DeltaY < MaxDeltaY) {
            *MaximizedY = FALSE;
            *DeltaX -= (HorizontalScrollSize+FontSize.X-1) / FontSize.X;
        }

        /*
         * Everything is done, so get out.
         */
        else {
            if (*DeltaX > MaxDeltaX)
                *DeltaX = MaxDeltaX;
            else if (*DeltaX < MinDeltaX)
                *DeltaX = MinDeltaX;
            if (*DeltaY > MaxDeltaY)
                *DeltaY = MaxDeltaY;
            else if (*DeltaY < MinDeltaY)
                *DeltaY = MinDeltaY;
            return;
        }
    }
}
