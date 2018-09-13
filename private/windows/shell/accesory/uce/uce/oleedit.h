//**********************************************************************
// File name: oleedit.h
//
// Header file for extensions required to enable drag and drop with edit
// controls
//
//
// Copyright (c) 1997-1999 Microsoft Corporation.
//**********************************************************************

#ifndef OLEEDIT_H
#define OLEEDIT_H

BOOL SetEditProc(HWND hWndEdit);
LRESULT CALLBACK OleEnabledEditControlProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL PointInSel(HWND hWnd, POINT ptDragStart);
int EditCtrlDragAndDrop(HWND hWndDlg, HWND hWndEdit, LPDROPSOURCE pDropSource);
int XToCP(HWND hWnd, LPTSTR lpszText, POINT ptDragStart);

#endif // OLEEDIT_H
