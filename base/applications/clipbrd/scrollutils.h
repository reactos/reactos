/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Scrolling related helper functions.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 */

#pragma once

typedef struct _SCROLLSTATE
{
    UINT uLinesToScroll;    /* Number of lines to scroll on one wheel rotation movement (== one "click" == WHEEL_DELTA ticks) */
    INT iWheelCarryoverX;   /* Unused wheel ticks (< WHEEL_DELTA) */
    INT iWheelCarryoverY;
    INT nPageX;             /* Number of lines per page */
    INT nPageY;
    INT CurrentX;           /* Current scrollbar position */
    INT CurrentY;
    INT MaxX;               /* Maximum scrollbar position */
    INT MaxY;
    INT nMaxWidth;          /* Maximum span of displayed data */
    INT nMaxHeight;
} SCROLLSTATE, *LPSCROLLSTATE;

void OnKeyScroll(HWND hWnd, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state);
void OnMouseScroll(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state);
void OnScroll(HWND hWnd, INT nBar, WPARAM wParam, INT iDelta, LPSCROLLSTATE state);

void UpdateLinesToScroll(LPSCROLLSTATE state);
void UpdateWindowScrollState(HWND hWnd, INT nMaxWidth, INT nMaxHeight, LPSCROLLSTATE lpState);
