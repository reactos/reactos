/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/user32.h
 * PURPOSE:     Global user32 definitions
 */

#pragma once

LRESULT WINAPI ScrollBarWndProcW( HWND hwnd, UINT uMsg, WPARAM wParam,LPARAM lParam );
LRESULT WINAPI ScrollBarWndProcA( HWND hwnd, UINT uMsg, WPARAM wParam,LPARAM lParam );
VOID FASTCALL ScrollTrackScrollBar(HWND Wnd, INT SBType, POINT Pt);
