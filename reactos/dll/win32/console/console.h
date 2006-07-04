#ifndef CONSOLE_H__
#define CONSOLE_H__

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <stdio.h>
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
  TCHAR szProcessName[MAX_PATH];
  DWORD CursorSize;
  DWORD NumberOfHistoryBuffers;
  DWORD HistoryBufferSize;
  DWORD HistoryNoDup;
  DWORD FullScreen;
  DWORD QuickEdit;
  DWORD InsertMode;

} ConsoleInfo, *PConsoleInfo;

BOOL WriteConsoleOptions(PConsoleInfo pConInfo);
BOOL InitConsoleInfo(HWND hwnd);

#endif /* CONSOLE_H__ */
