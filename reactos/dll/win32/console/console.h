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
  DWORD CursorSize;
  DWORD NumberOfHistoryBuffers;
  DWORD HistoryBufferSize;
  DWORD HistoryNoDup;
  DWORD FullScreen;
  DWORD QuickEdit;
  DWORD InsertMode;

} ConsoleInfo, *PConsoleInfo;

extern ConsoleInfo g_ConsoleInfo;

#endif /* CONSOLE_H__ */
