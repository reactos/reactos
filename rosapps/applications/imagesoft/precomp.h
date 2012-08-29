#ifndef __IMAGESOFT_PRECOMP_H
#define __IMAGESOFT_PRECOMP_H

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

#include "tooldock.h"
#include "imgedwnd.h"
#include "mainwnd.h"
#include "imageprop.h"
#include "misc.h"

#define MAX_KEY_LENGTH 256
#define NUM_MAINTB_IMAGES 10
#define TB_BMP_WIDTH 16
#define TB_BMP_HEIGHT 16

#define TOOLS   0
#define COLORS  1
#define HISTORY 2

extern HINSTANCE hInstance;
extern HANDLE ProcessHeap;

/* about.c */
INT_PTR CALLBACK AboutDialogProc(HWND hDlg,
                                 UINT message,
                                 WPARAM wParam,
                                 LPARAM lParam);

/* opensave.c */
VOID FileInitialize(HWND hwnd);
BOOL DoOpenFile(HWND hwnd,
                LPTSTR lpFileName,
                LPTSTR lpName);
BOOL DoSaveFile(HWND hwnd);

/* floattoolbar.c */
typedef struct _FLT_WND
{
    HWND hSelf;
    LPTSTR lpName;
    INT x;
    INT y;
    INT Width;
    INT Height;
    INT Transparancy;
    BOOL bOpaque;
} FLT_WND, *PFLT_WND;

BOOL FloatToolbarCreateToolsGui(PMAIN_WND_INFO Info);
BOOL FloatToolbarCreateColorsGui(PMAIN_WND_INFO Info);
BOOL FloatToolbarCreateHistoryGui(PMAIN_WND_INFO Info);
BOOL InitFloatWndClass(VOID);
VOID UninitFloatWndImpl(VOID);
BOOL ShowHideWindow(HWND hwnd);

/* font.c */
VOID FillFontStyleComboList(HWND hwndCombo);
VOID FillFontSizeComboList(HWND hwndCombo);

/* custcombo.c */
VOID MakeFlatCombo(HWND hwndCombo);

#endif /* __IMAGESOFT_PRECOMP_H */
