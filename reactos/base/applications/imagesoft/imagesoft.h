#ifndef __PAINT_H
#define __PAINT_H

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

#define MAX_KEY_LENGTH 256
#define NUM_BUTTONS 14

BOOL CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ToolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

BOOL ShowHideToolbar(HWND hwnd);

VOID FileInitialize(HWND hwnd);
VOID DoOpenFile(HWND hwnd);
VOID DoSaveFile(HWND hwnd);

#endif /* __SERVMAN_H */
