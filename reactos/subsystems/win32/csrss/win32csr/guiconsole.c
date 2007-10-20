/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/guiconsole.c
 * PURPOSE:         Implementation of gui-mode consoles
 */

/* INCLUDES ******************************************************************/

#include "w32csr.h"

#define NDEBUG
#include <debug.h>

/* Not defined in any header file */
extern VOID STDCALL PrivateCsrssManualGuiCheck(LONG Check);

/* GLOBALS *******************************************************************/

typedef struct GUI_CONSOLE_DATA_TAG
{
  HFONT Font;
  unsigned CharWidth;
  unsigned CharHeight;
  PWCHAR LineBuffer;
  BOOL CursorBlinkOn;
  BOOL ForceCursorOff;
  CRITICAL_SECTION Lock;
  RECT Selection;
  POINT SelectionStart;
  BOOL MouseDown;
  HMODULE ConsoleLibrary;
  HANDLE hGuiInitEvent;
  HWND hVScrollBar;
  HWND hHScrollBar;
  WCHAR FontName[LF_FACESIZE];
  DWORD FontSize;
  DWORD FontWeight;
  DWORD HistoryNoDup;
  DWORD FullScreen;
  DWORD QuickEdit;
  DWORD InsertMode;
  DWORD NumberOfHistoryBuffers;
  DWORD HistoryBufferSize;
  DWORD WindowPosition;
  DWORD ScreenBufferSize;
  DWORD UseRasterFonts;
  COLORREF ScreenText;
  COLORREF ScreenBackground;
  COLORREF PopupBackground;
  COLORREF PopupText;
  COLORREF Colors[16];
  WCHAR szProcessName[MAX_PATH];
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;

#ifndef WM_APP
#define WM_APP 0x8000
#endif
#define PM_CREATE_CONSOLE  (WM_APP + 1)
#define PM_DESTROY_CONSOLE (WM_APP + 2)

#define CURSOR_BLINK_TIME 500
#define DEFAULT_ATTRIB (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)

static BOOL ConsInitialized = FALSE;
static HWND NotifyWnd;

typedef struct _GUICONSOLE_MENUITEM
{
    UINT uID;
    const struct _GUICONSOLE_MENUITEM *SubMenu;
    WORD wCmdID;
} GUICONSOLE_MENUITEM, *PGUICONSOLE_MENUITEM;

static const GUICONSOLE_MENUITEM GuiConsoleEditMenuItems[] =
{
    { IDS_MARK, NULL, ID_SYSTEM_EDIT_MARK },
    { IDS_COPY, NULL, ID_SYSTEM_EDIT_COPY },
    { IDS_PASTE, NULL, ID_SYSTEM_EDIT_PASTE },
    { IDS_SELECTALL, NULL, ID_SYSTEM_EDIT_SELECTALL },
    { IDS_SCROLL, NULL, ID_SYSTEM_EDIT_SCROLL },
    { IDS_FIND, NULL, ID_SYSTEM_EDIT_FIND },

    { 0, NULL, 0 } /* End of list */
};

static const GUICONSOLE_MENUITEM GuiConsoleMainMenuItems[] =
{
    { (UINT)-1, NULL, 0 }, /* Separator */
    { IDS_EDIT, GuiConsoleEditMenuItems, 0 },
    { IDS_DEFAULTS, NULL, ID_SYSTEM_DEFAULTS },
    { IDS_PROPERTIES, NULL, ID_SYSTEM_PROPERTIES },

    { 0, NULL, 0 } /* End of list */
};

static const COLORREF s_Colors[] =
{
    RGB(0, 0, 0),
    RGB(0, 0, 128),
    RGB(0, 128, 0),
    RGB(0, 128, 128),
    RGB(128, 0, 0),
    RGB(128, 0, 128),
    RGB(128, 128, 0),
    RGB(192, 192, 192),
    RGB(128, 128, 128),
    RGB(0, 0, 255),
    RGB(0, 255, 0),
    RGB(0, 255, 255),
    RGB(255, 0, 0),
    RGB(255, 0, 255),
    RGB(255, 255, 0),
    RGB(255, 255, 255)
};

/* FUNCTIONS *****************************************************************/

static VOID FASTCALL
GuiConsoleAppendMenuItems(HMENU hMenu,
                          const GUICONSOLE_MENUITEM *Items)
{
    UINT i = 0;
    WCHAR szMenuString[255];
    HMENU hSubMenu;
    HINSTANCE hInst = GetModuleHandleW(L"win32csr");

    do
    {
        if (Items[i].uID != (UINT)-1)
        {
            if (LoadStringW(hInst,
                            Items[i].uID,
                            szMenuString,
                            sizeof(szMenuString) / sizeof(szMenuString[0])) > 0)
            {
                if (Items[i].SubMenu != NULL)
                {
                    hSubMenu = CreatePopupMenu();
                    if (hSubMenu != NULL)
                    {
                        GuiConsoleAppendMenuItems(hSubMenu,
                                                  Items[i].SubMenu);

                        if (!AppendMenuW(hMenu,
                                         MF_STRING | MF_POPUP,
                                         (UINT_PTR)hSubMenu,
                                         szMenuString))
                        {
                            DestroyMenu(hSubMenu);
                        }
                    }
                }
                else
                {
                    AppendMenuW(hMenu,
                                MF_STRING,
                                Items[i].wCmdID,
                                szMenuString);
                }
            }
        }
        else
        {
            AppendMenuW(hMenu,
                        MF_SEPARATOR,
                        0,
                        NULL);
        }
    i++;
    }while(!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].wCmdID == 0));
}

static VOID FASTCALL
GuiConsoleCreateSysMenu(PCSRSS_CONSOLE Console)
{
    HMENU hMenu;
    hMenu = GetSystemMenu(Console->hWindow,
                          FALSE);
    if (hMenu != NULL)
    {
        GuiConsoleAppendMenuItems(hMenu,
                                  GuiConsoleMainMenuItems);
        DrawMenuBar(Console->hWindow);
    }
}

static VOID FASTCALL
GuiConsoleGetDataPointers(HWND hWnd, PCSRSS_CONSOLE *Console, PGUI_CONSOLE_DATA *GuiData)
{
  *Console = (PCSRSS_CONSOLE) GetWindowLongPtrW(hWnd, GWL_USERDATA);
  *GuiData = (NULL == *Console ? NULL : (*Console)->PrivateData);
}

static BOOL FASTCALL
GuiConsoleOpenUserRegistryPathPerProcessId(DWORD ProcessId, PHANDLE hProcHandle, PHKEY hResult, REGSAM samDesired)
{
  HANDLE hProcessToken = NULL;
  HANDLE hProcess;

  BYTE Buffer[256];
  DWORD Length = 0;
  UNICODE_STRING SidName;
  LONG res;
  PTOKEN_USER TokUser;

  hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | READ_CONTROL, FALSE, ProcessId);
  if (!hProcess)
    {
      DPRINT("Error: OpenProcess failed(0x%x)\n", GetLastError());
      return FALSE;
    }

  if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hProcessToken))
    {
      DPRINT("Error: OpenProcessToken failed(0x%x)\n", GetLastError());
      CloseHandle(hProcess);
      return FALSE;
    }

  if (!GetTokenInformation(hProcessToken, TokenUser, (PVOID)Buffer, sizeof(Buffer), &Length))
    {
      DPRINT("Error: GetTokenInformation failed(0x%x)\n",GetLastError());
      CloseHandle(hProcess);
      CloseHandle(hProcessToken);
      return FALSE;
    }

  TokUser = ((PTOKEN_USER)Buffer)->User.Sid;
  if (!NT_SUCCESS(RtlConvertSidToUnicodeString(&SidName, TokUser, TRUE)))
    {
      DPRINT("Error: RtlConvertSidToUnicodeString failed(0x%x)\n", GetLastError());
      return FALSE;
    }

  res = RegOpenKeyExW(HKEY_USERS, SidName.Buffer, 0, samDesired, hResult);
  RtlFreeUnicodeString(&SidName);

  CloseHandle(hProcessToken);
  if (hProcHandle)
    *hProcHandle = hProcess;
  else
    CloseHandle(hProcess);

  if (res != ERROR_SUCCESS)
    return FALSE;
  else
    return TRUE;
}

static BOOL FASTCALL
GuiConsoleOpenUserSettings(PGUI_CONSOLE_DATA GuiData, DWORD ProcessId, PHKEY hSubKey, REGSAM samDesired, BOOL bCreate)
{
  WCHAR szProcessName[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  UINT fLength, wLength;
  DWORD dwBitmask, dwLength;
  WCHAR CurDrive[] = { 'A',':', 0 };
  HANDLE hProcess;
  HKEY hKey;
  WCHAR * ptr;

  /*
   * console properties are stored under
   * HKCU\Console\*
   *
   * There are 3 ways to store console properties
   *
   *  1. use console title as subkey name
   *    i.e. cmd.exe
   *
   *  2. use application name as subkey name
   *
   *  3. use unexpanded path to console application.
   *     i.e. %SystemRoot%_system32_cmd.exe
   */

  DPRINT("GuiConsoleOpenUserSettings entered\n");

  if (!GuiConsoleOpenUserRegistryPathPerProcessId(ProcessId, &hProcess, &hKey, samDesired))
    {
      DPRINT("GuiConsoleOpenUserRegistryPathPerProcessId failed\n");
      return FALSE;
    }

  /* FIXME we do not getting the process name so no menu will be loading, why ?*/
  fLength = GetProcessImageFileNameW(hProcess, szProcessName, sizeof(GuiData->szProcessName) / sizeof(WCHAR));
  CloseHandle(hProcess);

  //DPRINT1("szProcessName3 : %S\n",szProcessName);

  if (!fLength)
    {
      DPRINT("GetProcessImageFileNameW failed(0x%x)ProcessId %d\n", GetLastError(),hProcess);
      return FALSE;
    }
  /*
   * try the process name as path
   */

  ptr = wcsrchr(szProcessName, L'\\');
  wcscpy(GuiData->szProcessName, ptr);

  swprintf(szBuffer, L"Console%s",ptr);
  DPRINT("#1 Path : %S\n", szBuffer);

  if (bCreate)
    {
      if (RegCreateKeyW(hKey, szBuffer, hSubKey) == ERROR_SUCCESS)
        {
          RegCloseKey(hKey);
          return TRUE;
        }
      RegCloseKey(hKey);
      return FALSE;
  }

  if (RegOpenKeyExW(hKey, szBuffer, 0, samDesired, hSubKey) == ERROR_SUCCESS)
    {
      RegCloseKey(hKey);
      return TRUE;
    }

  /*
   * try the "Shortcut to processname" as path
   * FIXME: detect wheter the process was started as a shortcut
   */

  swprintf(szBuffer, L"Console\\Shortcut to %S", ptr);
  DPRINT("#2 Path : %S\n", szBuffer);
  if (RegOpenKeyExW(hKey, szBuffer, 0, samDesired, hSubKey) == ERROR_SUCCESS)
    {
      swprintf(GuiData->szProcessName, L"Shortcut to %S", ptr);
      RegCloseKey(hKey);
      return TRUE;
    }

  /*
   * if the path contains \\Device\\HarddiskVolume1\... remove it
   */

  if (szProcessName[0] == L'\\')
    {
      dwBitmask = GetLogicalDrives();
      while(dwBitmask)
        {
          if (dwBitmask & 0x1)
            {
              dwLength = QueryDosDeviceW(CurDrive, szBuffer, MAX_PATH);
              if (dwLength)
                {
                  if (!memcmp(szBuffer, szProcessName, (dwLength-2)*sizeof(WCHAR)))
                    {
                      wcscpy(szProcessName, CurDrive);
                      RtlMoveMemory(&szProcessName[2], &szProcessName[dwLength-1], fLength - dwLength -1);
                      break;
                    }
                }
            }
          dwBitmask = (dwBitmask >> 1);
          CurDrive[0]++;
        }
    }

  /*
   * last attempt: check whether the file is under %SystemRoot%
   * and use path like Console\%SystemRoot%_dir_dir2_file.exe
   */

  wLength = GetWindowsDirectoryW(szBuffer, MAX_PATH);
  if (wLength)
    {
      if (!wcsncmp(szProcessName, szBuffer, wLength))
        {
          /* replace slashes by underscores */
          while((ptr = wcschr(szProcessName, L'\\')))
            ptr[0] = L'_';

          swprintf(szBuffer, L"Console\\\%SystemRoot\%%S", &szProcessName[wLength]);
          DPRINT("#3 Path : %S\n", szBuffer);
          if (RegOpenKeyExW(hKey, szBuffer, 0, samDesired, hSubKey) == ERROR_SUCCESS)
            {
              swprintf(GuiData->szProcessName, L"\%SystemRoot\%%S", &szProcessName[wLength]);
              RegCloseKey(hKey);
              return TRUE;
            }
        }
    }
  RegCloseKey(hKey);
  return FALSE;
}

static VOID
GuiConsoleWriteUserSettings(PCSRSS_CONSOLE Console, PGUI_CONSOLE_DATA GuiData)
{
    HKEY hKey;
    PCSRSS_PROCESS_DATA ProcessData;

    if (Console->ProcessList.Flink == &Console->ProcessList)
      {
        DPRINT("GuiConsoleWriteUserSettings: No Process!!!\n");
        return;
      }
    ProcessData = CONTAINING_RECORD(Console->ProcessList.Flink, CSRSS_PROCESS_DATA, ProcessEntry);
    if (!GuiConsoleOpenUserSettings(GuiData, PtrToUlong(ProcessData->ProcessId), &hKey, KEY_READ | KEY_WRITE, TRUE))
      {
        return;
      }

  if (Console->ActiveBuffer->CursorInfo.dwSize <= 1)
    {
      RegDeleteKeyW(hKey, L"CursorSize");
    }
  else
    {
      RegSetValueExW(hKey, L"CursorSize", 0, REG_DWORD, (const BYTE *)&Console->ActiveBuffer->CursorInfo.dwSize, sizeof(DWORD));
    }

  if (GuiData->NumberOfHistoryBuffers == 5)
    {
      RegDeleteKeyW(hKey, L"NumberOfHistoryBuffers");
    }
  else
    {
      RegSetValueExW(hKey, L"NumberOfHistoryBuffers", 0, REG_DWORD, (const BYTE *)&GuiData->NumberOfHistoryBuffers, sizeof(DWORD));
    }

  if (GuiData->HistoryBufferSize == 50)
    {
      RegDeleteKeyW(hKey, L"HistoryBufferSize");
    }
  else
    {
      RegSetValueExW(hKey, L"HistoryBufferSize", 0, REG_DWORD, (const BYTE *)&GuiData->HistoryBufferSize, sizeof(DWORD));
    }

  if (GuiData->FullScreen == FALSE)
    {
      RegDeleteKeyW(hKey, L"FullScreen");
    }
  else
    {
      RegSetValueExW(hKey, L"FullScreen", 0, REG_DWORD, (const BYTE *)&GuiData->FullScreen, sizeof(DWORD));
    }

  if ( GuiData->QuickEdit == FALSE)
    {
      RegDeleteKeyW(hKey, L"QuickEdit");
    }
    else
    {
      RegSetValueExW(hKey, L"QuickEdit", 0, REG_DWORD, (const BYTE *)&GuiData->QuickEdit, sizeof(DWORD));
    }

  if (GuiData->InsertMode == TRUE)
    {
      RegDeleteKeyW(hKey, L"InsertMode");
    }
  else
    {
      RegSetValueExW(hKey, L"InsertMode", 0, REG_DWORD, (const BYTE *)&GuiData->InsertMode, sizeof(DWORD));
    }

  if (GuiData->HistoryNoDup == FALSE)
    {
      RegDeleteKeyW(hKey, L"HistoryNoDup");
    }
  else
    {
      RegSetValueExW(hKey, L"HistoryNoDup", 0, REG_DWORD, (const BYTE *)&GuiData->HistoryNoDup, sizeof(DWORD));
    }

  if (GuiData->ScreenText == RGB(192, 192, 192))
    {
      /*
       * MS uses console attributes instead of real color
       */
       RegDeleteKeyW(hKey, L"ScreenText");
    }
  else
    {
       RegSetValueExW(hKey, L"ScreenText", 0, REG_DWORD, (const BYTE *)&GuiData->ScreenText, sizeof(COLORREF));
    }

  if (GuiData->ScreenBackground == RGB(0, 0, 0))
    {
       RegDeleteKeyW(hKey, L"ScreenBackground");
    }
  else
    {
       RegSetValueExW(hKey, L"ScreenBackground", 0, REG_DWORD, (const BYTE *)&GuiData->ScreenBackground, sizeof(COLORREF));
    }

  RegCloseKey(hKey);
}

static void FASTCALL
GuiConsoleReadUserSettings(HKEY hKey, PCSRSS_CONSOLE Console, PGUI_CONSOLE_DATA GuiData, PCSRSS_SCREEN_BUFFER Buffer)
{
  DWORD dwNumSubKeys = 0;
  DWORD dwIndex;
  DWORD dwValueName;
  DWORD dwValue;
  DWORD dwType;
  WCHAR szValueName[MAX_PATH];
  WCHAR szValue[MAX_PATH];
  DWORD Value;

  if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwNumSubKeys, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
       DPRINT("GuiConsoleReadUserSettings: RegQueryInfoKey failed\n");
       return;
    }

  DPRINT("GuiConsoleReadUserSettings entered dwNumSubKeys %d\n", dwNumSubKeys);

  for (dwIndex = 0; dwIndex < dwNumSubKeys; dwIndex++)
    {
      dwValue = sizeof(Value);
      dwValueName = MAX_PATH;

      if (RegEnumValueW(hKey, dwIndex, szValueName, &dwValueName, NULL, &dwType, (BYTE*)&Value, &dwValue) != ERROR_SUCCESS)
        {
          if (dwType == REG_SZ)
            {
              /*
               * retry in case of string value
               */
              dwValue = sizeof(szValue);
              dwValueName = MAX_PATH;
              if (RegEnumValueW(hKey, dwIndex, szValueName, &dwValueName, NULL, NULL, (BYTE*)szValue, &dwValue) != ERROR_SUCCESS)
                break;
            }
          else
            break;
        }
      if (!wcscmp(szValueName, L"CursorSize"))
        {
          if (Value == 0x32)
            {
              Buffer->CursorInfo.dwSize = Value;
            }
          else if (Value == 0x64)
            {
              Buffer->CursorInfo.dwSize = Value;
            }
        }
      else if (!wcscmp(szValueName, L"ScreenText"))
        {
          GuiData->ScreenText = Value;
        }
      else if (!wcscmp(szValueName, L"ScreenBackground"))
        {
          GuiData->ScreenBackground = Value;
        }
      else if (!wcscmp(szValueName, L"FaceName"))
        {
          wcscpy(GuiData->FontName, szValue);
        }
      else if (!wcscmp(szValueName, L"FontSize"))
        {
          GuiData->FontSize = Value;
        }
      else if (!wcscmp(szValueName, L"FontWeight"))
        {
          GuiData->FontWeight = Value;
        }
      else if (!wcscmp(szValueName, L"HistoryNoDup"))
        {
          GuiData->HistoryNoDup = Value;
        }
      else if (!wcscmp(szValueName, L"WindowSize"))
        {
          Console->Size.X = LOWORD(Value);
		  Console->Size.Y = HIWORD(Value);
        }
      else if (!wcscmp(szValueName, L"ScreenBufferSize"))
        {
            if(Buffer)
              {
                Buffer->MaxX = LOWORD(Value);
                Buffer->MaxY = HIWORD(Value);
              }
        }
      else if (!wcscmp(szValueName, L"FullScreen"))
        {
          GuiData->FullScreen = Value;
        }
      else if (!wcscmp(szValueName, L"QuickEdit"))
        {
          GuiData->QuickEdit = Value;
        }
      else if (!wcscmp(szValueName, L"InsertMode"))
        {
          GuiData->InsertMode = Value;
        }
   }
}
static VOID FASTCALL
GuiConsoleUseDefaults(PCSRSS_CONSOLE Console, PGUI_CONSOLE_DATA GuiData, PCSRSS_SCREEN_BUFFER Buffer)
{
  /*
   * init guidata with default properties
   */

  wcscpy(GuiData->FontName, L"DejaVu Sans Mono");
  GuiData->FontSize = 0x0008000C; // font is 8x12
  GuiData->FontWeight = FW_NORMAL;
  GuiData->HistoryNoDup = FALSE;
  GuiData->FullScreen = FALSE;
  GuiData->QuickEdit = FALSE;
  GuiData->InsertMode = TRUE;
  GuiData->HistoryBufferSize = 50;
  GuiData->NumberOfHistoryBuffers = 5;
  GuiData->ScreenText = RGB(192, 192, 192);
  GuiData->ScreenBackground = RGB(0, 0, 0);
  GuiData->PopupText = RGB(128, 0, 128);
  GuiData->PopupBackground = RGB(255, 255, 255);
  GuiData->WindowPosition = UINT_MAX;
  GuiData->ScreenBufferSize = MAKELONG(80, 300); //FIXME
  GuiData->UseRasterFonts = TRUE;
  memcpy(GuiData->Colors, s_Colors, sizeof(s_Colors));

  Console->Size.X = 80;
  Console->Size.Y = 25;

  if (Buffer)
    {
      Buffer->MaxX = 80;
      Buffer->MaxY = 25;
      Buffer->CursorInfo.bVisible = TRUE;
      Buffer->CursorInfo.dwSize = 5;
    }
}

VOID
FASTCALL
GuiConsoleInitScrollbar(PCSRSS_CONSOLE Console, HWND hwnd)
{
  SCROLLINFO sInfo;

  /* set scrollbar sizes */
  sInfo.cbSize = sizeof(SCROLLINFO);
  sInfo.fMask = SIF_RANGE | SIF_POS;
  sInfo.nMin = 0;
  sInfo.nMax = Console->ActiveBuffer->MaxY;
  sInfo.nPos = 0;
  SetScrollInfo(hwnd, SB_HORZ, &sInfo, TRUE);
  ShowScrollBar(hwnd, SB_VERT, TRUE);

  if (Console->ActiveBuffer->MaxX > Console->Size.X)
  {
      sInfo.cbSize = sizeof(SCROLLINFO);
      sInfo.fMask = SIF_RANGE | SIF_POS;
      sInfo.nMin = 0;
      sInfo.nPos = 0;
      sInfo.nMax = Console->ActiveBuffer->MaxX;
      SetScrollInfo(hwnd, SB_HORZ, &sInfo, TRUE);
  }
  else
  {
    ShowScrollBar(hwnd, SB_HORZ, FALSE);
  }
}

static BOOL FASTCALL
GuiConsoleHandleNcCreate(HWND hWnd, CREATESTRUCTW *Create)
{
  RECT Rect;
  PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) Create->lpCreateParams;
  PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA)Console->PrivateData;
  HDC Dc;
  HFONT OldFont;
  TEXTMETRICW Metrics;
  PCSRSS_PROCESS_DATA ProcessData;
  HKEY hKey;

  Console->hWindow = hWnd;

  if (NULL == GuiData)
    {
      DPRINT1("GuiConsoleNcCreate: HeapAlloc failed\n");
      return FALSE;
    }

  GuiConsoleUseDefaults(Console, GuiData, Console->ActiveBuffer);
  if (Console->ProcessList.Flink != &Console->ProcessList)
    {
      ProcessData = CONTAINING_RECORD(Console->ProcessList.Flink, CSRSS_PROCESS_DATA, ProcessEntry);
      if (GuiConsoleOpenUserSettings(GuiData, PtrToUlong(ProcessData->ProcessId), &hKey, KEY_READ, FALSE))
        {
          GuiConsoleReadUserSettings(hKey, Console, GuiData, Console->ActiveBuffer);
          RegCloseKey(hKey);
        }
    }

  InitializeCriticalSection(&GuiData->Lock);

  GuiData->LineBuffer = (PWCHAR)HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY,
                                          Console->Size.X * sizeof(WCHAR));

  GuiData->Font = CreateFontW(LOWORD(GuiData->FontSize),
                              0, //HIWORD(GuiData->FontSize),
                              0,
                              TA_BASELINE,
                              GuiData->FontWeight,
                              FALSE,
                              FALSE,
                              FALSE,
                              OEM_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              NONANTIALIASED_QUALITY, FIXED_PITCH | FF_DONTCARE,
                              GuiData->FontName);
  if (NULL == GuiData->Font)
    {
      DPRINT1("GuiConsoleNcCreate: CreateFont failed\n");
      DeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  Dc = GetDC(hWnd);
  if (NULL == Dc)
    {
      DPRINT1("GuiConsoleNcCreate: GetDC failed\n");
      DeleteObject(GuiData->Font);
      DeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  OldFont = SelectObject(Dc, GuiData->Font);
  if (NULL == OldFont)
    {
      DPRINT1("GuiConsoleNcCreate: SelectObject failed\n");
      ReleaseDC(hWnd, Dc);
      DeleteObject(GuiData->Font);
      DeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  if (! GetTextMetricsW(Dc, &Metrics))
    {
      DPRINT1("GuiConsoleNcCreate: GetTextMetrics failed\n");
      SelectObject(Dc, OldFont);
      ReleaseDC(hWnd, Dc);
      DeleteObject(GuiData->Font);
      DeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  GuiData->CharWidth = Metrics.tmMaxCharWidth;
  GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;
  SelectObject(Dc, OldFont);

  ReleaseDC(hWnd, Dc);
  GuiData->CursorBlinkOn = TRUE;
  GuiData->ForceCursorOff = FALSE;

  GuiData->Selection.left = -1;
  DPRINT("Console %p GuiData %p\n", Console, GuiData);
  Console->PrivateData = GuiData;
  SetWindowLongPtrW(hWnd, GWL_USERDATA, (DWORD_PTR) Console);

  GetWindowRect(hWnd, &Rect);
  Rect.right = Rect.left + Console->Size.X * GuiData->CharWidth +
               2 * GetSystemMetrics(SM_CXFIXEDFRAME);
  Rect.bottom = Rect.top + Console->Size.Y * GuiData->CharHeight +
               2 * GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);
  MoveWindow(hWnd, Rect.left, Rect.top, Rect.right - Rect.left,
             Rect.bottom - Rect.top, FALSE);

  SetTimer(hWnd, 1, CURSOR_BLINK_TIME, NULL);
  GuiConsoleCreateSysMenu(Console);
  GuiConsoleInitScrollbar(Console, hWnd);
  SetEvent(GuiData->hGuiInitEvent);

  return (BOOL) DefWindowProcW(hWnd, WM_NCCREATE, 0, (LPARAM) Create);
}

static COLORREF FASTCALL
GuiConsoleRGBFromAttribute(BYTE Attribute)
{
  int Red = (Attribute & 0x04 ? (Attribute & 0x08 ? 0xff : 0x80) : 0x00);
  int Green = (Attribute & 0x02 ? (Attribute & 0x08 ? 0xff : 0x80) : 0x00);
  int Blue = (Attribute & 0x01 ? (Attribute & 0x08 ? 0xff : 0x80) : 0x00);

  return RGB(Red, Green, Blue);
}

static VOID FASTCALL
GuiConsoleSetTextColors(HDC Dc, BYTE Attribute, PCSRSS_SCREEN_BUFFER Buff, COLORREF TextColor, COLORREF BkColor)
{
  if (Attribute != Buff->DefaultAttrib)
    {
      SetTextColor(Dc, GuiConsoleRGBFromAttribute(Attribute & 0x0f));
      SetBkColor(Dc, GuiConsoleRGBFromAttribute((Attribute & 0xf0) >> 4));
    }
  else
    {
      SetTextColor(Dc, TextColor);
      SetBkColor(Dc, BkColor);
    }
}

static VOID FASTCALL
GuiConsoleGetLogicalCursorPos(PCSRSS_SCREEN_BUFFER Buff, ULONG *CursorX, ULONG *CursorY)
{
  *CursorX = Buff->CurrentX;
  if (Buff->CurrentY < Buff->ShowY)
    {
      *CursorY = Buff->MaxY - Buff->ShowY + Buff->CurrentY;
    }
  else
    {
      *CursorY = Buff->CurrentY - Buff->ShowY;
    }
}


static VOID FASTCALL
GuiConsoleUpdateSelection(HWND hWnd, PRECT rc, PGUI_CONSOLE_DATA GuiData)
{
  RECT oldRect = GuiData->Selection;

  if(rc != NULL)
  {
    RECT changeRect = *rc;

    GuiData->Selection = *rc;

    changeRect.left *= GuiData->CharWidth;
    changeRect.top *= GuiData->CharHeight;
    changeRect.right *= GuiData->CharWidth;
    changeRect.bottom *= GuiData->CharHeight;

    if(rc->left != oldRect.left ||
       rc->top != oldRect.top ||
       rc->right != oldRect.right ||
       rc->bottom != oldRect.bottom)
    {
      if(oldRect.left != -1)
      {
        HRGN rgn1, rgn2;

        oldRect.left *= GuiData->CharWidth;
        oldRect.top *= GuiData->CharHeight;
        oldRect.right *= GuiData->CharWidth;
        oldRect.bottom *= GuiData->CharHeight;

        /* calculate the region that needs to be updated */
        if((rgn1 = CreateRectRgnIndirect(&oldRect)))
        {
          if((rgn2 = CreateRectRgnIndirect(&changeRect)))
          {
            if(CombineRgn(rgn1, rgn2, rgn1, RGN_XOR) != ERROR)
            {
              InvalidateRgn(hWnd, rgn1, FALSE);
            }

            DeleteObject(rgn2);
          }
          DeleteObject(rgn1);
        }
      }
      else
      {
        InvalidateRect(hWnd, &changeRect, FALSE);
      }
    }
  }
  else if(oldRect.left != -1)
  {
    /* clear the selection */
    GuiData->Selection.left = -1;
    oldRect.left *= GuiData->CharWidth;
    oldRect.top *= GuiData->CharHeight;
    oldRect.right *= GuiData->CharWidth;
    oldRect.bottom *= GuiData->CharHeight;
    InvalidateRect(hWnd, &oldRect, FALSE);
  }
}


static VOID FASTCALL
GuiConsolePaint(PCSRSS_CONSOLE Console,
                PGUI_CONSOLE_DATA GuiData,
                HDC hDC,
                PRECT rc)
{
    PCSRSS_SCREEN_BUFFER Buff;
    ULONG TopLine, BottomLine, LeftChar, RightChar;
    ULONG Line, Char, Start;
    PBYTE From;
    PWCHAR To;
    BYTE LastAttribute, Attribute;
    ULONG CursorX, CursorY, CursorHeight;
    HBRUSH CursorBrush, OldBrush, BackgroundBrush;
    HFONT OldFont;

    Buff = Console->ActiveBuffer;

    TopLine = rc->top / GuiData->CharHeight;
    BottomLine = (rc->bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1;
    LeftChar = rc->left / GuiData->CharWidth;
    RightChar = (rc->right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1;
    LastAttribute = Buff->Buffer[(TopLine * Buff->MaxX + LeftChar) * 2 + 1];

    GuiConsoleSetTextColors(hDC,
                            LastAttribute,
                            Buff,
                            GuiData->ScreenText,
                            GuiData->ScreenBackground);

    EnterCriticalSection(&Buff->Header.Lock);

    OldFont = SelectObject(hDC,
                           GuiData->Font);

	BackgroundBrush = CreateSolidBrush(GuiData->ScreenBackground);
	FillRect(hDC, rc, BackgroundBrush);
	DeleteObject(BackgroundBrush);

    for (Line = TopLine; Line <= BottomLine; Line++)
    {
        if (Line + Buff->ShowY < Buff->MaxY)
        {
            From = Buff->Buffer + ((Line + Buff->ShowY) * Buff->MaxX + LeftChar) * 2;
        }
        else
        {
            From = Buff->Buffer +
                   ((Line - (Buff->MaxY - Buff->ShowY)) * Buff->MaxX + LeftChar) * 2;
        }
        Start = LeftChar;
        To = GuiData->LineBuffer;

        for (Char = LeftChar; Char <= RightChar; Char++)
        {
            if (*(From + 1) != LastAttribute)
            {
                TextOutW(hDC,
                         Start * GuiData->CharWidth,
                         Line * GuiData->CharHeight,
                         GuiData->LineBuffer,
                         Char - Start);
                Start = Char;
                To = GuiData->LineBuffer;
                Attribute = *(From + 1);
                if (Attribute != LastAttribute)
                {
                    GuiConsoleSetTextColors(hDC,
                                            Attribute,
                                            Buff,
                                            GuiData->ScreenText,
                                            GuiData->ScreenBackground);
                    LastAttribute = Attribute;
                }
            }

            MultiByteToWideChar(Console->OutputCodePage,
                                0,
                                (PCHAR)From,
                                1,
                                To,
                                1);
            To++;
            From += 2;
        }

        TextOutW(hDC,
                 Start * GuiData->CharWidth,
                 Line * GuiData->CharHeight,
                 GuiData->LineBuffer,
                 RightChar - Start + 1);
    }

    if (Buff->CursorInfo.bVisible && GuiData->CursorBlinkOn &&
        !GuiData->ForceCursorOff)
    {
        GuiConsoleGetLogicalCursorPos(Buff,
                                      &CursorX,
                                      &CursorY);
        if (LeftChar <= CursorX && CursorX <= RightChar &&
            TopLine <= CursorY && CursorY <= BottomLine)
        {
            CursorHeight = (GuiData->CharHeight * Buff->CursorInfo.dwSize) / 100;
            if (CursorHeight < 1)
            {
                CursorHeight = 1;
            }
            From = Buff->Buffer + (Buff->CurrentY * Buff->MaxX + Buff->CurrentX) * 2 + 1;

            if (*From != DEFAULT_ATTRIB)
            {
                CursorBrush = CreateSolidBrush(GuiConsoleRGBFromAttribute(*From));
            }
            else
            {
                CursorBrush = CreateSolidBrush(GuiData->ScreenText);
            }

            OldBrush = SelectObject(hDC,
                                    CursorBrush);
            PatBlt(hDC,
                   CursorX * GuiData->CharWidth,
                   CursorY * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                   GuiData->CharWidth,
                   CursorHeight,
                   PATCOPY);
            SelectObject(hDC,
                         OldBrush);
            DeleteObject(CursorBrush);
        }
    }

    LeaveCriticalSection(&Buff->Header.Lock);

    SelectObject(hDC,
                 OldFont);
}

static VOID FASTCALL
GuiConsoleHandlePaint(HWND hWnd, HDC hDCPaint)
{
    HDC hDC;
    PAINTSTRUCT ps;
    PCSRSS_CONSOLE Console;
    PGUI_CONSOLE_DATA GuiData;

    hDC = BeginPaint(hWnd, &ps);
    if (hDC != NULL &&
        ps.rcPaint.left < ps.rcPaint.right &&
        ps.rcPaint.top < ps.rcPaint.bottom)
    {
        GuiConsoleGetDataPointers(hWnd,
                                  &Console,
                                  &GuiData);
        if (Console != NULL && GuiData != NULL &&
            Console->ActiveBuffer != NULL)
        {
            if (Console->ActiveBuffer->Buffer != NULL)
            {
                EnterCriticalSection(&GuiData->Lock);

                GuiConsolePaint(Console,
                                GuiData,
                                hDC,
                                &ps.rcPaint);

                if (GuiData->Selection.left != -1)
                {
                    RECT rc = GuiData->Selection;

                    rc.left *= GuiData->CharWidth;
                    rc.top *= GuiData->CharHeight;
                    rc.right *= GuiData->CharWidth;
                    rc.bottom *= GuiData->CharHeight;

                    /* invert the selection */
                    if (IntersectRect(&rc,
                                      &ps.rcPaint,
                                      &rc))
                    {
                        PatBlt(hDC,
                               rc.left,
                               rc.top,
                               rc.right - rc.left,
                               rc.bottom - rc.top,
                               DSTINVERT);
                    }
                }

                LeaveCriticalSection(&GuiData->Lock);
            }
        }

        EndPaint(hWnd, &ps);
    }
}

static VOID FASTCALL
GuiConsoleHandleKey(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  MSG Message;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  Message.hwnd = hWnd;
  Message.message = msg;
  Message.wParam = wParam;
  Message.lParam = lParam;

  if(msg == WM_CHAR || msg == WM_SYSKEYDOWN)
  {
    /* clear the selection */
    GuiConsoleUpdateSelection(hWnd, NULL, GuiData);
  }

  ConioProcessKey(&Message, Console, FALSE);
}

static VOID FASTCALL
GuiIntDrawRegion(PGUI_CONSOLE_DATA GuiData, HWND Wnd, RECT *Region)
{
  RECT RegionRect;

  RegionRect.left = Region->left * GuiData->CharWidth;
  RegionRect.top = Region->top * GuiData->CharHeight;
  RegionRect.right = (Region->right + 1) * GuiData->CharWidth;
  RegionRect.bottom = (Region->bottom + 1) * GuiData->CharHeight;

  InvalidateRect(Wnd, &RegionRect, FALSE);
}

static VOID STDCALL
GuiDrawRegion(PCSRSS_CONSOLE Console, RECT *Region)
{
  PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA) Console->PrivateData;

  if (NULL != Console->hWindow && NULL != GuiData)
    {
      GuiIntDrawRegion(GuiData, Console->hWindow, Region);
    }
}

static VOID FASTCALL
GuiInvalidateCell(PGUI_CONSOLE_DATA GuiData, HWND Wnd, UINT x, UINT y)
{
  RECT CellRect;

  CellRect.left = x;
  CellRect.top = y;
  CellRect.right = x;
  CellRect.bottom = y;

  GuiIntDrawRegion(GuiData, Wnd, &CellRect);
}

static VOID STDCALL
GuiWriteStream(PCSRSS_CONSOLE Console, RECT *Region, LONG CursorStartX, LONG CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
  PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA) Console->PrivateData;
  PCSRSS_SCREEN_BUFFER Buff = Console->ActiveBuffer;
  LONG CursorEndX, CursorEndY;
  RECT ScrollRect;

  if (NULL == Console->hWindow || NULL == GuiData)
    {
      return;
    }

  if (0 != ScrolledLines)
    {
      ScrollRect.left = 0;
      ScrollRect.top = 0;
      ScrollRect.right = Console->Size.X * GuiData->CharWidth;
      ScrollRect.bottom = Region->top * GuiData->CharHeight;

      if (GuiData->Selection.left != -1)
      {
          /* scroll the selection */
          if (GuiData->Selection.top > ScrolledLines)
          {
              GuiData->Selection.top -= ScrolledLines;
              GuiData->Selection.bottom -= ScrolledLines;
          }
          else if (GuiData->Selection.bottom < ScrolledLines)
          {
              GuiData->Selection.left = -1;
          }
          else
          {
              GuiData->Selection.top = 0;
              GuiData->Selection.bottom -= ScrolledLines;
          }
      }

      ScrollWindowEx(Console->hWindow,
                     0,
                     -(ScrolledLines * GuiData->CharHeight),
                     &ScrollRect,
                     NULL,
                     NULL,
                     NULL,
                     SW_INVALIDATE);
    }

  GuiIntDrawRegion(GuiData, Console->hWindow, Region);

  if (CursorStartX < Region->left || Region->right < CursorStartX
      || CursorStartY < Region->top || Region->bottom < CursorStartY)
    {
      GuiInvalidateCell(GuiData, Console->hWindow, CursorStartX, CursorStartY);
    }

  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY,
                         &CursorEndX, &CursorEndY);
  if ((CursorEndX < Region->left || Region->right < CursorEndX
       || CursorEndY < Region->top || Region->bottom < CursorEndY)
      && (CursorEndX != CursorStartX || CursorEndY != CursorStartY))
    {
      GuiInvalidateCell(GuiData, Console->hWindow, CursorEndX, CursorEndY);
    }
}

static BOOL STDCALL
GuiSetCursorInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff)
{
  RECT UpdateRect;

  if (Console->ActiveBuffer == Buff)
    {
      ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY,
                             &UpdateRect.left, &UpdateRect.top);
      UpdateRect.right = UpdateRect.left;
      UpdateRect.bottom = UpdateRect.top;
      ConioDrawRegion(Console, &UpdateRect);
    }

  return TRUE;
}

static BOOL STDCALL
GuiSetScreenInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff, UINT OldCursorX, UINT OldCursorY)
{
  RECT UpdateRect;

  if (Console->ActiveBuffer == Buff)
    {
      /* Redraw char at old position (removes cursor) */
      UpdateRect.left = OldCursorX;
      UpdateRect.top = OldCursorY;
      UpdateRect.right = OldCursorX;
      UpdateRect.bottom = OldCursorY;
      ConioDrawRegion(Console, &UpdateRect);
      /* Redraw char at new position (shows cursor) */
      ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY,
                             &(UpdateRect.left), &(UpdateRect.top));
      UpdateRect.right = UpdateRect.left;
      UpdateRect.bottom = UpdateRect.top;
      ConioDrawRegion(Console, &UpdateRect);
    }

  return TRUE;
}

static BOOL STDCALL
GuiUpdateScreenInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff)
{
    PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA) Console->PrivateData;

    if (Console->ActiveBuffer == Buff)
    {
        GuiData->ScreenText = GuiConsoleRGBFromAttribute(Buff->DefaultAttrib & 0x0f);
        GuiData->ScreenBackground = GuiConsoleRGBFromAttribute((Buff->DefaultAttrib & 0xf0) >> 4);
    }

    return TRUE;
}

static VOID FASTCALL
GuiConsoleHandleTimer(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  RECT CursorRect;
  ULONG CursorX, CursorY;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  GuiData->CursorBlinkOn = ! GuiData->CursorBlinkOn;

  GuiConsoleGetLogicalCursorPos(Console->ActiveBuffer, &CursorX, &CursorY);
  CursorRect.left = CursorX;
  CursorRect.top = CursorY;
  CursorRect.right = CursorX;
  CursorRect.bottom = CursorY;
  GuiDrawRegion(Console, &CursorRect);
}

static VOID FASTCALL
GuiConsoleHandleClose(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  PLIST_ENTRY current_entry;
  PCSRSS_PROCESS_DATA current;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);

  EnterCriticalSection(&Console->Header.Lock);

  current_entry = Console->ProcessList.Flink;
  while (current_entry != &Console->ProcessList)
    {
      current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
      current_entry = current_entry->Flink;

      ConioConsoleCtrlEvent(CTRL_CLOSE_EVENT, current);
    }

  LeaveCriticalSection(&Console->Header.Lock);
}

static VOID FASTCALL
GuiConsoleHandleNcDestroy(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;


  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  KillTimer(hWnd, 1);
  Console->PrivateData = NULL;
  DeleteCriticalSection(&GuiData->Lock);
  GetSystemMenu(hWnd, TRUE);
  if (GuiData->ConsoleLibrary)
    FreeLibrary(GuiData->ConsoleLibrary);

  HeapFree(Win32CsrApiHeap, 0, GuiData);
}

static VOID FASTCALL
GuiConsoleLeftMouseDown(HWND hWnd, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  POINTS pt;
  RECT rc;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (Console == NULL || GuiData == NULL) return;

  pt = MAKEPOINTS(lParam);

  rc.left = pt.x / GuiData->CharWidth;
  rc.top = pt.y / GuiData->CharHeight;
  rc.right = rc.left + 1;
  rc.bottom = rc.top + 1;

  GuiData->SelectionStart.x = rc.left;
  GuiData->SelectionStart.y = rc.top;

  SetCapture(hWnd);

  GuiData->MouseDown = TRUE;

  GuiConsoleUpdateSelection(hWnd, &rc, GuiData);
}

static VOID FASTCALL
GuiConsoleLeftMouseUp(HWND hWnd, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  RECT rc;
  POINTS pt;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (Console == NULL || GuiData == NULL) return;
  if (GuiData->Selection.left == -1 || !GuiData->MouseDown) return;

  pt = MAKEPOINTS(lParam);

  rc.left = GuiData->SelectionStart.x;
  rc.top = GuiData->SelectionStart.y;
  rc.right = (pt.x >= 0 ? (pt.x / GuiData->CharWidth) + 1 : 0);
  rc.bottom = (pt.y >= 0 ? (pt.y / GuiData->CharHeight) + 1 : 0);

  /* exchange left/top with right/bottom if required */
  if(rc.left >= rc.right)
  {
    LONG tmp;
    tmp = rc.left;
    rc.left = max(rc.right - 1, 0);
    rc.right = tmp + 1;
  }
  if(rc.top >= rc.bottom)
  {
    LONG tmp;
    tmp = rc.top;
    rc.top = max(rc.bottom - 1, 0);
    rc.bottom = tmp + 1;
  }

  GuiData->MouseDown = FALSE;

  GuiConsoleUpdateSelection(hWnd, &rc, GuiData);

  ReleaseCapture();
}

static VOID FASTCALL
GuiConsoleMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  RECT rc;
  POINTS pt;

  if (!(wParam & MK_LBUTTON)) return;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (Console == NULL || GuiData == NULL || !GuiData->MouseDown) return;

  pt = MAKEPOINTS(lParam);

  rc.left = GuiData->SelectionStart.x;
  rc.top = GuiData->SelectionStart.y;
  rc.right = (pt.x >= 0 ? (pt.x / GuiData->CharWidth) + 1 : 0);
  if (Console->Size.X < rc.right)
  {
    rc.right = Console->Size.X;
  }
  rc.bottom = (pt.y >= 0 ? (pt.y / GuiData->CharHeight) + 1 : 0);
  if (Console->Size.Y < rc.bottom)
  {
    rc.bottom = Console->Size.Y;
  }

  /* exchange left/top with right/bottom if required */
  if(rc.left >= rc.right)
  {
    LONG tmp;
    tmp = rc.left;
    rc.left = max(rc.right - 1, 0);
    rc.right = tmp + 1;
  }
  if(rc.top >= rc.bottom)
  {
    LONG tmp;
    tmp = rc.top;
    rc.top = max(rc.bottom - 1, 0);
    rc.bottom = tmp + 1;
  }

  GuiConsoleUpdateSelection(hWnd, &rc, GuiData);
}

static VOID FASTCALL
GuiConsoleRightMouseDown(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (Console == NULL || GuiData == NULL) return;

  if (GuiData->Selection.left == -1)
  {
    /* FIXME - paste text from clipboard */
  }
  else
  {
    /* FIXME - copy selection to clipboard */

    GuiConsoleUpdateSelection(hWnd, NULL, GuiData);
  }

}


static VOID
GuiConsoleShowConsoleProperties(HWND hWnd, BOOL Defaults, PGUI_CONSOLE_DATA GuiData)
{
  PCSRSS_CONSOLE Console;
  APPLET_PROC CPLFunc;
  TCHAR szBuffer[MAX_PATH];
  ConsoleInfo SharedInfo;

  DPRINT("GuiConsoleShowConsoleProperties entered\n");

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);

  if (GuiData == NULL)
    {
      DPRINT("GuiConsoleGetDataPointers failed\n");
      return;
    }

  if (GuiData->ConsoleLibrary == NULL)
    {
      GetWindowsDirectory(szBuffer,MAX_PATH);
      _tcscat(szBuffer, _T("\\system32\\console.dll"));
      GuiData->ConsoleLibrary = LoadLibrary(szBuffer);

      if (GuiData->ConsoleLibrary == NULL)
        {
          DPRINT1("failed to load console.dll");
          return;
        }
    }

  CPLFunc = (APPLET_PROC) GetProcAddress(GuiData->ConsoleLibrary, _T("CPlApplet"));
  if (!CPLFunc)
    {
      DPRINT("Error: Console.dll misses CPlApplet export\n");
      return;
    }

  /* setup struct */
  SharedInfo.InsertMode = GuiData->InsertMode;
  SharedInfo.HistoryBufferSize = GuiData->HistoryBufferSize;
  SharedInfo.NumberOfHistoryBuffers = GuiData->NumberOfHistoryBuffers;
  SharedInfo.ScreenText = GuiData->ScreenText;
  SharedInfo.ScreenBackground = GuiData->ScreenBackground;
  SharedInfo.PopupText = GuiData->PopupText;
  SharedInfo.PopupBackground = GuiData->PopupBackground;
  SharedInfo.WindowSize = (DWORD)MAKELONG(Console->Size.X, Console->Size.Y);
  SharedInfo.WindowPosition = GuiData->WindowPosition;
  SharedInfo.ScreenBuffer = GuiData->ScreenBufferSize;
  SharedInfo.UseRasterFonts = GuiData->UseRasterFonts;
  SharedInfo.FontSize = (DWORD)GuiData->FontSize;
  SharedInfo.FontWeight = GuiData->FontWeight;
  SharedInfo.CursorSize = Console->ActiveBuffer->CursorInfo.dwSize;
  SharedInfo.HistoryNoDup = GuiData->HistoryNoDup;
  SharedInfo.FullScreen = GuiData->FullScreen;
  SharedInfo.QuickEdit = GuiData->QuickEdit;
  memcpy(&SharedInfo.Colors[0], GuiData->Colors, sizeof(s_Colors));

  if (!CPLFunc(hWnd, CPL_INIT, 0, 0))
    {
      DPRINT("Error: failed to initialize console.dll\n");
      return;
    }

  if (CPLFunc(hWnd, CPL_GETCOUNT, 0, 0) != 1)
    {
      DPRINT("Error: console.dll returned unexpected CPL count\n");
      return;
    }

  CPLFunc(hWnd, CPL_DBLCLK, (LPARAM)&SharedInfo, Defaults);
}
static LRESULT FASTCALL
GuiConsoleHandleSysMenuCommand(HWND hWnd, WPARAM wParam, LPARAM lParam, PGUI_CONSOLE_DATA GuiData)
{
    LRESULT Ret = TRUE;

    switch(wParam)
    {
        case ID_SYSTEM_EDIT_MARK:
        case ID_SYSTEM_EDIT_COPY:
        case ID_SYSTEM_EDIT_PASTE:
        case ID_SYSTEM_EDIT_SELECTALL:
        case ID_SYSTEM_EDIT_SCROLL:
        case ID_SYSTEM_EDIT_FIND:
            break;

        case ID_SYSTEM_DEFAULTS:
            GuiConsoleShowConsoleProperties(hWnd, TRUE, GuiData);
            break;

        case ID_SYSTEM_PROPERTIES:
            GuiConsoleShowConsoleProperties(hWnd, FALSE, GuiData);
            break;

        default:
            Ret = DefWindowProcW(hWnd, WM_SYSCOMMAND, wParam, lParam);
            break;
    }
    return Ret;
}

static VOID FASTCALL
GuiConsoleResize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED)
  {
      DPRINT1("GuiConsoleResize X %d Y %d\n", LOWORD(lParam), HIWORD(lParam));
  }
}
VOID
FASTCALL
GuiConsoleHandleScrollbarMenu()
{
  HMENU hMenu;

  hMenu = CreatePopupMenu();
  if (hMenu == NULL)
    {
      DPRINT("CreatePopupMenu failed\n");
      return;
    }
  //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLHERE);
  //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
  //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLTOP);
  //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLBOTTOM);
  //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
  //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLPAGE_UP);
  //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLPAGE_DOWN);
  //InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);
  //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLUP);
  //InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLLDOWN);

}

static VOID FASTCALL
GuiApplyUserSettings(PCSRSS_CONSOLE Console, PGUI_CONSOLE_DATA GuiData, PConsoleInfo pConInfo)
{
  DWORD windx, windy;
  RECT rect;
  PCSRSS_SCREEN_BUFFER ActiveBuffer;
  PCSRSS_PROCESS_DATA ProcessData = NULL;

  if (Console->ProcessList.Flink != &Console->ProcessList)
    {
      ProcessData = CONTAINING_RECORD(Console->ProcessList.Flink, CSRSS_PROCESS_DATA, ProcessEntry);
      ConioLockScreenBuffer(ProcessData, Console->hActiveBuffer, (Object_t **)&ActiveBuffer);
    }

  /* apply text / background color */
  GuiData->ScreenText = pConInfo->ScreenText;
  GuiData->ScreenBackground = pConInfo->ScreenBackground;

  /* apply cursor size */
  Console->ActiveBuffer->CursorInfo.dwSize = max(min(pConInfo->CursorSize, 1), 100);

  windx = LOWORD(pConInfo->ScreenBuffer);
  windy = HIWORD(pConInfo->ScreenBuffer);

  if (windx != ActiveBuffer->MaxX || windy != ActiveBuffer->MaxY)
  {
     BYTE * Buffer = HeapAlloc(Win32CsrApiHeap, 0, windx * windy * 2);

     if (Buffer)
     {
        DWORD Offset = 0;
        DWORD BufferOffset = 0;
        USHORT CurrentY;
        BYTE * OldBuffer;
        USHORT value;
        DWORD diff;
        DWORD i;

        value = MAKEWORD(' ', ActiveBuffer->DefaultAttrib);

        DPRINT("MaxX %d MaxY %d windx %d windy %d value %04x DefaultAttrib %d\n",ActiveBuffer->MaxX, ActiveBuffer->MaxY, windx, windy, value, ActiveBuffer->DefaultAttrib);
        OldBuffer = ActiveBuffer->Buffer;

        for (CurrentY = 0; CurrentY < min(ActiveBuffer->MaxY, windy); CurrentY++)
        {
            if (windx <= ActiveBuffer->MaxX)
            {
                /* reduce size */
                RtlCopyMemory(&Buffer[Offset], &OldBuffer[BufferOffset], windx * 2);
                Offset += (windx * 2);
                BufferOffset += (ActiveBuffer->MaxX * 2);
            }
            else
            {
                /* enlarge size */
                RtlCopyMemory(&Buffer[Offset], &OldBuffer[BufferOffset], ActiveBuffer->MaxX * 2);
                Offset += (ActiveBuffer->MaxX * 2);

                diff = windx - ActiveBuffer->MaxX;
                /* zero new part of it */
#if HAVE_WMEMSET
                wmemset((WCHAR*)&Buffer[Offset], value, diff);
#else
                for (i = 0; i < diff * 2; i++)
                {
                    Buffer[Offset * 2] = ' ';
                    Buffer[Offset * 2 + 1] = ActiveBuffer->DefaultAttrib;
                }
#endif
                Offset += (diff * 2);
                BufferOffset += (Console->ActiveBuffer->MaxX * 2);
            }
        }

        if (windy > Console->ActiveBuffer->MaxY)
        {
            diff = windy - Console->ActiveBuffer->MaxX;
#if HAVE_WMEMSET
                wmemset((WCHAR*)&Buffer[Offset], value, diff * windx);
#else
                for (i = 0; i < diff * 2; i++)
                {
                    Buffer[Offset * 2] = ' ';
                    Buffer[Offset * 2 + 1] = ActiveBuffer->DefaultAttrib;
                }
#endif
        }

        (void)InterlockedExchangePointer((PVOID volatile  *)&Console->ActiveBuffer->Buffer, Buffer);
        HeapFree(Win32CsrApiHeap, 0, OldBuffer);
        Console->ActiveBuffer->MaxX = windx;
        Console->ActiveBuffer->MaxY = windy;
        InvalidateRect(pConInfo->hConsoleWindow, NULL, TRUE);
     }
     else
     {
        if (ProcessData)
        {
            ConioUnlockScreenBuffer(ActiveBuffer);
        }
        return;
     }
  }

  windx = LOWORD(pConInfo->WindowSize);
  windy = HIWORD(pConInfo->WindowSize);

  if (windx > Console->Size.X)
  {
      PWCHAR LineBuffer = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, windx * sizeof(WCHAR));
      if (LineBuffer)
      {
          HeapFree(Win32CsrApiHeap, 0, GuiData->LineBuffer);
          GuiData->LineBuffer = LineBuffer;
      }
      else
      {
          if (ProcessData)
          {
              ConioUnlockScreenBuffer(ActiveBuffer);
          }
          return;
      }
  }


  if (windx != Console->Size.X || windy != Console->Size.Y)
  {
      /* resize window */
      Console->Size.X = windx;
      Console->Size.Y = windy;

      GetWindowRect(pConInfo->hConsoleWindow, &rect);

      rect.right = rect.left + Console->Size.X * GuiData->CharWidth + 2 * GetSystemMetrics(SM_CXFIXEDFRAME);
      rect.bottom = rect.top + Console->Size.Y * GuiData->CharHeight + 2 * GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION);

      MoveWindow(pConInfo->hConsoleWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);

      if (Console->Size.X < Console->ActiveBuffer->MaxX)
      {
          /* show scrollbar when window becomes smaller than active screen buffer */
          ShowScrollBar(pConInfo->hConsoleWindow, SB_CTL, TRUE);
      }
      else
      {
          /* hide scrollbar */
          ShowScrollBar(pConInfo->hConsoleWindow, SB_CTL, FALSE);
      }
  }
  if (ProcessData)
  {
      ConioUnlockScreenBuffer(ActiveBuffer);
  }
  InvalidateRect(pConInfo->hConsoleWindow, NULL, TRUE);
}

static
LRESULT
GuiConsoleHandleScroll(HWND hwnd, UINT uMsg, WPARAM wParam, PGUI_CONSOLE_DATA GuiData)
{
  SCROLLINFO sInfo;
  int old_pos;

  /* set scrollbar sizes */
  sInfo.cbSize = sizeof(SCROLLINFO);
  sInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_TRACKPOS;

  if (!GetScrollInfo(hwnd,
                    (uMsg == WM_HSCROLL ? SB_HORZ : SB_VERT),
                    &sInfo))
  {
    return FALSE;
  }

  old_pos = sInfo.nPos;

  switch(LOWORD(wParam))
  {
  case SB_LINELEFT:
      sInfo.nPos -= 1;
      break;

  case SB_LINERIGHT:
      sInfo.nPos += 1;
      break;

  case SB_PAGELEFT:
      sInfo.nPos -= sInfo.nPage;
      break;

  case SB_PAGERIGHT:
      sInfo.nPos += sInfo.nPage;
      break;

  case SB_THUMBTRACK:
      sInfo.nPage = sInfo.nTrackPos;
      break;

  case SB_TOP:
      sInfo.nPos = sInfo.nMin;
      break;

  case SB_BOTTOM:
      sInfo.nPos = sInfo.nMax;
      break;

  default:
     break;
  }

  sInfo.fMask = SIF_POS;
  sInfo.cbSize = sizeof(SCROLLINFO);

  SetScrollInfo(hwnd,
                (uMsg == WM_HSCROLL ? SB_HORZ : SB_VERT),
                &sInfo,
                TRUE);

  sInfo.cbSize = sizeof(SCROLLINFO);
  sInfo.fMask = SIF_POS;

  if (!GetScrollInfo(hwnd,
                (uMsg == WM_HSCROLL ? SB_HORZ : SB_VERT),
                &sInfo))
  {
    return 0;
  }

  if (old_pos != sInfo.nPos)
  {
     ///
     /// fixme scroll window
     ///

      ScrollWindowEx(hwnd,
                     0,
                     GuiData->CharHeight * (old_pos - sInfo.nPos),
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     SW_INVALIDATE);

      UpdateWindow(hwnd);
  }
  return 0;
}

static LRESULT CALLBACK
GuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result = 0;
  PGUI_CONSOLE_DATA GuiData = NULL;
  PCSRSS_CONSOLE Console = NULL;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);

  switch(msg)
    {
      case WM_NCCREATE:
        Result = (LRESULT) GuiConsoleHandleNcCreate(hWnd, (CREATESTRUCTW *) lParam);
        break;
      case WM_PAINT:
        GuiConsoleHandlePaint(hWnd, (HDC)wParam);
        break;
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_CHAR:
        GuiConsoleHandleKey(hWnd, msg, wParam, lParam);
        break;
      case WM_TIMER:
        GuiConsoleHandleTimer(hWnd);
        break;
      case WM_CLOSE:
        GuiConsoleHandleClose(hWnd);
        break;
      case WM_NCDESTROY:
        GuiConsoleHandleNcDestroy(hWnd);
        break;
      case WM_LBUTTONDOWN:
          GuiConsoleLeftMouseDown(hWnd, lParam);
        break;
      case WM_LBUTTONUP:
          GuiConsoleLeftMouseUp(hWnd, lParam);
        break;
      case WM_RBUTTONDOWN:
          GuiConsoleRightMouseDown(hWnd);
        break;
      case WM_MOUSEMOVE:
          GuiConsoleMouseMove(hWnd, wParam, lParam);
        break;
      case WM_SYSCOMMAND:
          Result = GuiConsoleHandleSysMenuCommand(hWnd, wParam, lParam, GuiData);
          break;
      case WM_HSCROLL:
      case WM_VSCROLL:
          Result = GuiConsoleHandleScroll(hWnd, msg, wParam, GuiData);
          break;
      case WM_SIZE:
          GuiConsoleResize(hWnd, wParam, lParam);
          break;
      case PM_APPLY_CONSOLE_INFO:
          GuiApplyUserSettings(Console, GuiData, (PConsoleInfo)wParam);
          if (lParam)
            {
              GuiConsoleWriteUserSettings(Console, GuiData);
            }
          break;
      default:
        Result = DefWindowProcW(hWnd, msg, wParam, lParam);
        break;
    }

  return Result;
}

static LRESULT CALLBACK
GuiConsoleNotifyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  HWND NewWindow;
  LONG WindowCount;
  MSG Msg;
  PWCHAR Buffer, Title;
  PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) lParam;



  switch(msg)
    {
      case WM_CREATE:
        SetWindowLongW(hWnd, GWL_USERDATA, 0);
        return 0;
      case PM_CREATE_CONSOLE:
        Buffer = HeapAlloc(Win32CsrApiHeap, 0,
                           Console->Title.Length + sizeof(WCHAR));
        if (NULL != Buffer)
          {
            memcpy(Buffer, Console->Title.Buffer, Console->Title.Length);
            Buffer[Console->Title.Length / sizeof(WCHAR)] = L'\0';
            Title = Buffer;
          }
        else
          {
            Title = L"";
          }
        NewWindow = CreateWindowW(L"ConsoleWindowClass",
                                  Title,
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_HSCROLL | WS_VSCROLL, //WS_OVERLAPPEDWINDOW
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  NULL,
                                  NULL,
                                  (HINSTANCE) GetModuleHandleW(NULL),
                                  (PVOID) Console);
        if (NULL != Buffer)
          {
            HeapFree(Win32CsrApiHeap, 0, Buffer);
          }
        if (NULL != NewWindow)
          {
            SetWindowLongW(hWnd, GWL_USERDATA, GetWindowLongW(hWnd, GWL_USERDATA) + 1);
            ShowWindow(NewWindow, SW_SHOW);
          }
        return (LRESULT) NewWindow;
      case PM_DESTROY_CONSOLE:
        /* Window creation is done using a PostMessage(), so it's possible that the
         * window that we want to destroy doesn't exist yet. So first empty the message
         * queue */
        while(PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
          {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
          }
        DestroyWindow(Console->hWindow);
        Console->hWindow = NULL;
        WindowCount = GetWindowLongW(hWnd, GWL_USERDATA);
        WindowCount--;
        SetWindowLongW(hWnd, GWL_USERDATA, WindowCount);
        if (0 == WindowCount)
          {
            NotifyWnd = NULL;
            DestroyWindow(hWnd);
            PrivateCsrssManualGuiCheck(-1);
            PostQuitMessage(0);
          }
        return 0;
      default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static DWORD STDCALL
GuiConsoleGuiThread(PVOID Data)
{
  MSG msg;
  PHANDLE GraphicsStartupEvent = (PHANDLE) Data;

  NotifyWnd = CreateWindowW(L"Win32CsrCreateNotify",
                            L"",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            NULL,
                            NULL,
                            (HINSTANCE) GetModuleHandleW(NULL),
                            NULL);
  if (NULL == NotifyWnd)
    {
      PrivateCsrssManualGuiCheck(-1);
      SetEvent(*GraphicsStartupEvent);
      return 1;
    }

  SetEvent(*GraphicsStartupEvent);

  while(GetMessageW(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }

  return 1;
}

static BOOL FASTCALL
GuiInit(VOID)
{
  WNDCLASSEXW wc;

  if (NULL == NotifyWnd)
    {
      PrivateCsrssManualGuiCheck(+1);
    }

  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpszClassName = L"Win32CsrCreateNotify";
  wc.lpfnWndProc = GuiConsoleNotifyWndProc;
  wc.style = 0;
  wc.hInstance = (HINSTANCE) GetModuleHandleW(NULL);
  wc.hIcon = NULL;
  wc.hCursor = NULL;
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIconSm = NULL;
  if (RegisterClassExW(&wc) == 0)
    {
      DPRINT1("Failed to register notify wndproc\n");
      return FALSE;
    }

  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpszClassName = L"ConsoleWindowClass";
  wc.lpfnWndProc = GuiConsoleWndProc;
  wc.style = 0;
  wc.hInstance = (HINSTANCE) GetModuleHandleW(NULL);
  wc.hIcon = LoadIconW(GetModuleHandleW(L"win32csr"), MAKEINTRESOURCEW(1));
  wc.hCursor = LoadCursorW(NULL, (LPCWSTR) IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIconSm = LoadImageW(GetModuleHandleW(L"win32csr"), MAKEINTRESOURCEW(1), IMAGE_ICON,
                          GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                          LR_SHARED);
  if (RegisterClassExW(&wc) == 0)
    {
      DPRINT1("Failed to register console wndproc\n");
      return FALSE;
    }

  return TRUE;
}

static VOID STDCALL
GuiInitScreenBuffer(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buffer)
{
  Buffer->DefaultAttrib = DEFAULT_ATTRIB;
}

static BOOL STDCALL
GuiChangeTitle(PCSRSS_CONSOLE Console)
{
  PWCHAR Buffer, Title;

  Buffer = HeapAlloc(Win32CsrApiHeap, 0,
                     Console->Title.Length + sizeof(WCHAR));
  if (NULL != Buffer)
    {
      memcpy(Buffer, Console->Title.Buffer, Console->Title.Length);
      Buffer[Console->Title.Length / sizeof(WCHAR)] = L'\0';
      Title = Buffer;
    }
  else
    {
      Title = L"";
    }

  SendMessageW(Console->hWindow, WM_SETTEXT, 0, (LPARAM) Title);

  if (NULL != Buffer)
    {
      HeapFree(Win32CsrApiHeap, 0, Buffer);
    }

  return TRUE;
}

static BOOL STDCALL
GuiChangeIcon(PCSRSS_CONSOLE Console)
{
  SendMessageW(Console->hWindow, WM_SETICON, ICON_BIG, (LPARAM)Console->hWindowIcon);
  SendMessageW(Console->hWindow, WM_SETICON, ICON_SMALL, (LPARAM)Console->hWindowIcon);

  return TRUE;
}

static VOID STDCALL
GuiCleanupConsole(PCSRSS_CONSOLE Console)
{
  SendMessageW(NotifyWnd, PM_DESTROY_CONSOLE, 0, (LPARAM) Console);
}

static CSRSS_CONSOLE_VTBL GuiVtbl =
{
  GuiInitScreenBuffer,
  GuiWriteStream,
  GuiDrawRegion,
  GuiSetCursorInfo,
  GuiSetScreenInfo,
  GuiUpdateScreenInfo,
  GuiChangeTitle,
  GuiCleanupConsole,
  GuiChangeIcon
};

NTSTATUS FASTCALL
GuiInitConsole(PCSRSS_CONSOLE Console)
{
  HANDLE GraphicsStartupEvent;
  HANDLE ThreadHandle;
  PGUI_CONSOLE_DATA GuiData;

  if (! ConsInitialized)
    {
      ConsInitialized = TRUE;
      if (! GuiInit())
        {
          ConsInitialized = FALSE;
          return STATUS_UNSUCCESSFUL;
        }
    }

  Console->Vtbl = &GuiVtbl;
  if (NULL == NotifyWnd)
    {
      GraphicsStartupEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
      if (NULL == GraphicsStartupEvent)
        {
          return STATUS_UNSUCCESSFUL;
        }

      ThreadHandle = CreateThread(NULL,
                                  0,
                                  GuiConsoleGuiThread,
                                  (PVOID) &GraphicsStartupEvent,
                                  0,
                                  NULL);
      if (NULL == ThreadHandle)
        {
          NtClose(GraphicsStartupEvent);
          DPRINT1("Win32Csr: Failed to create graphics console thread. Expect problems\n");
          return STATUS_UNSUCCESSFUL;
        }
      SetThreadPriority(ThreadHandle, THREAD_PRIORITY_HIGHEST);
      CloseHandle(ThreadHandle);

      WaitForSingleObject(GraphicsStartupEvent, INFINITE);
      CloseHandle(GraphicsStartupEvent);

      if (NULL == NotifyWnd)
        {
          DPRINT1("Win32Csr: Failed to create notification window.\n");
          return STATUS_UNSUCCESSFUL;
        }
    }
    GuiData = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY,
                        sizeof(GUI_CONSOLE_DATA));
    if (!GuiData)
      {
        DPRINT1("Win32Csr: Failed to create GUI_CONSOLE_DATA\n");
        return STATUS_UNSUCCESSFUL;
      }

    Console->PrivateData = (PVOID) GuiData;
    /*
     * we need to wait untill the GUI has been fully initialized
     * to retrieve custom settings i.e. WindowSize etc..
     * Ideally we could use SendNotifyMessage for this but its not
     * yet implemented.
     *
     */
    GuiData->hGuiInitEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    /* create console */
    PostMessageW(NotifyWnd, PM_CREATE_CONSOLE, 0, (LPARAM) Console);

    /* wait untill initialization has finished */
    WaitForSingleObject(GuiData->hGuiInitEvent, INFINITE);
    DPRINT1("received event Console %p GuiData %p X %d Y %d\n", Console, Console->PrivateData, Console->Size.X, Console->Size.Y);
    CloseHandle(GuiData->hGuiInitEvent);
    GuiData->hGuiInitEvent = NULL;

  return STATUS_SUCCESS;
}

/* EOF */
