/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/window.h
 * PURPOSE:     Window management definitions
 */

#pragma once

UINT
MenuDrawMenuBar(HDC hDC, LPRECT Rect, HWND hWnd, BOOL Draw);
BOOL
MenuInit(VOID);
VOID
MenuCleanup(VOID);
VOID
MenuTrackMouseMenuBar(HWND hWnd, ULONG Ht, POINT Pt);
VOID
MenuTrackKbdMenuBar(HWND hWnd, UINT wParam, WCHAR wChar);

LRESULT WINAPI PopupMenuWndProcA(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI PopupMenuWndProcW(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
