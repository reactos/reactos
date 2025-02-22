/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/freeldrpage.h
 * PURPOSE:     Freeloader configuration page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *                        2011      Gregor Schneider <Gregor.Schneider@reactos.org>
 *              Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef _FREELDRPAGE_H_
#define _FREELDRPAGE_H_

extern LPCWSTR lpszFreeLdrIni;
extern LPCWSTR lpszBootIni;

INT_PTR CALLBACK FreeLdrPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif /* _FREELDRPAGE_H_ */
