//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//   File:      wselect.h
//
//  Contents:   Unicode support for CSelectElement on win95.
//
//------------------------------------------------------------------------

#ifndef I_WSELECT_H_
#define I_WSELECT_H_
#pragma INCMSG("--- Beg 'wselect.h'")

LRESULT CALLBACK WListboxHookProc(WNDPROC pfnWndProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WComboboxHookProc(WNDPROC pfnWndProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#pragma INCMSG("--- End 'wselect.h'")
#else
#pragma INCMSG("*** Dup 'wselect.h'")
#endif
