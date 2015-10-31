/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/systempage.h
 * PURPOSE:     System page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *                        2011      Gregor Schneider <Gregor.Schneider@reactos.org>
 *              Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef _SYSTEMPAGE_H_
#define _SYSTEMPAGE_H_

extern LPCWSTR lpszSystemIni;
extern LPCWSTR lpszWinIni;

DWORD GetSystemIniActivation(VOID);
DWORD GetWinIniActivation(VOID);

INT_PTR CALLBACK SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WinPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif
