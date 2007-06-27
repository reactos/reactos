#ifndef CONSOLE_H__
#define CONSOLE_H__

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <stdio.h>
#include <limits.h>
#include "resource.h"

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct TAGConsoleInfo
{
  HWND hConsoleWindow;
  WCHAR szProcessName[MAX_PATH];
  BOOLEAN AppliedConfig;
  DWORD UseRasterFonts;
  DWORD FontSize;
  DWORD FontWeight;
  FONTSIGNATURE FontSignature;
  DWORD CursorSize;
  DWORD NumberOfHistoryBuffers;
  DWORD HistoryBufferSize;
  DWORD HistoryNoDup;
  DWORD FullScreen;
  DWORD QuickEdit;
  DWORD InsertMode;
  DWORD ScreenBuffer;
  DWORD WindowSize;
  DWORD WindowPosition;
  DWORD ActiveStaticControl;
  COLORREF ScreenText;
  COLORREF ScreenBackground;
  COLORREF PopupText;
  COLORREF PopupBackground;
  COLORREF Colors[16];
} ConsoleInfo, *PConsoleInfo;

void ApplyConsoleInfo(HWND hwndDlg, PConsoleInfo pConInfo);
void PaintConsole(LPDRAWITEMSTRUCT drawItem, PConsoleInfo pConInfo);
void PaintText(LPDRAWITEMSTRUCT drawItem, PConsoleInfo pConInfo);

#define PM_APPLY_CONSOLE_INFO (WM_APP + 100)


//globals
extern HINSTANCE hApplet;

#endif /* CONSOLE_H__ */
