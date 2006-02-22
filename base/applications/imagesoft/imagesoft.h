#ifndef __IMAGESOFT_H
#define __IMAGESOFT_H

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

#define MAX_KEY_LENGTH 256
#define NUM_BUTTONS 13

BOOL CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID FileInitialize (HWND hwnd);
VOID DoOpenFile(HWND hwnd);
VOID DoSaveFile(HWND hwnd);

#endif /* __IMAGESOFT_H */
