/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/window.h
 * PURPOSE:     Window management definitions
 */

#ifndef __LIB_USER32_INCLUDE_MENU_H
#define __LIB_USER32_INCLUDE_MENU_H

UINT
MenuDrawMenuBar(HDC hDC, LPRECT Rect, HWND hWnd, BOOL Draw);
BOOL
MenuInit(VOID);
VOID
MenuCleanup(VOID);
VOID
MenuTrackMouseMenuBar(HWND hWnd, ULONG Ht, POINT Pt);
VOID
MenuTrackKbdMenuBar(HWND hWnd, ULONG wParam, ULONG Key);

#endif /* __LIB_USER32_INCLUDE_MENU_H */
