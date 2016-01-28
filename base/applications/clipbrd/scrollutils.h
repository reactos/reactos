/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/scrollutils.h
 * PURPOSE:         Scrolling releated helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

typedef struct _SCROLLSTATE
{
    int CurrentX;
    int CurrentY;
    int MaxX;
    int MaxY;
} SCROLLSTATE, *LPSCROLLSTATE;

void HandleKeyboardScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void HandleMouseScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state);
void HandleHorizontalScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state);
void HandleVerticalScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPSCROLLSTATE state);
void UpdateWindowScrollState(HWND hWnd, HBITMAP hBmp, LPSCROLLSTATE lpState);
BOOL ScrollBlt(PAINTSTRUCT ps, HBITMAP hBmp, SCROLLSTATE state);
