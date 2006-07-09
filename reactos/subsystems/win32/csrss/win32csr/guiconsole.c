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
  WCHAR FontName[128];
  DWORD FontSize;
  DWORD FontWeight;
  DWORD CursorSize;
  DWORD HistoryNoDup;
  DWORD FullScreen;
  DWORD QuickEdit;
  DWORD InsertMode;
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;

#ifndef WM_APP
#define WM_APP 0x8000
#endif
#define PM_CREATE_CONSOLE  (WM_APP + 1)
#define PM_DESTROY_CONSOLE (WM_APP + 2)

#define CURSOR_BLINK_TIME 500

static BOOL ConsInitialized = FALSE;
static HWND NotifyWnd;

/* FUNCTIONS *****************************************************************/

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
GuiConsoleOpenUserSettings(HWND hWnd, DWORD ProcessId, PHKEY hSubKey, REGSAM samDesired)
{
  WCHAR szProcessName[MAX_PATH];
  WCHAR szBuffer[MAX_PATH];
  UINT fLength, wLength;
  DWORD dwBitmask, dwLength;
  WCHAR CurDrive[] = { 'A',':', 0 };
  HANDLE hProcess;
  HKEY hKey;
  WCHAR * ptr, *res;
  static const WCHAR szSystemRoot[] = { '%','S','y','s','t','e','m','R','o','o','t','%', 0 };
  

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
  
  if (!GuiConsoleOpenUserRegistryPathPerProcessId(ProcessId, &hProcess, &hKey, samDesired))
    return FALSE;

  fLength = GetProcessImageFileNameW(hProcess, szProcessName, MAX_PATH);
  CloseHandle(hProcess);

  if (!fLength)
    {
	  DPRINT1("GetProcessImageFileNameW failed(0x%x)ProcessId %d\n", GetLastError(),hProcess);
	  return FALSE;
    }

    
  ptr = wcsrchr(szProcessName, L'\\');
  swprintf(szBuffer, L"Console%s",ptr);

  if (RegOpenKeyExW(hKey, szBuffer, 0, samDesired, hSubKey) == ERROR_SUCCESS)
    {
      RegCloseKey(hKey);
      return TRUE;
    }

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
                  wcscpy(szBuffer, CurDrive);
                  wcscat(&szBuffer[(sizeof(CurDrive)/sizeof(WCHAR))-1], &szProcessName[dwLength-2]);
                  break;
                }
            }
        }
      dwBitmask = (dwBitmask >> 1);
      CurDrive[0]++;
  }
  
  wLength = GetWindowsDirectoryW(szProcessName, MAX_PATH);

  if (!wcsncmp(szProcessName, szBuffer, wLength))
    {
      wcscpy(szProcessName, szSystemRoot);
      wcscpy(&szProcessName[(sizeof(szSystemRoot) / sizeof(WCHAR))-1], &szBuffer[wLength]);
      ptr = res = szProcessName;
    }
  else
    {
      ptr = res = szBuffer;
    }

  while((ptr = wcschr(szProcessName, L'\\')))
    ptr[0] = L'_';

  if (RegOpenKeyExW(hKey, res, 0, samDesired, hSubKey) == ERROR_SUCCESS)
    {
      RegCloseKey(hKey);
      return TRUE;
    }
  RegCloseKey(hKey);
  return FALSE;
}
static void FASTCALL
GuiConsoleReadUserSettings(HKEY hKey, PGUI_CONSOLE_DATA GuiData)
{
  DWORD dwNumSubKeys = 0;
  DWORD dwIndex;
  DWORD dwValueName;
  DWORD dwValue;
  DWORD dwType;
  WCHAR szValueName[MAX_PATH];
  WCHAR szValue[MAX_PATH];
  DWORD Value;

  RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwNumSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL );

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
            GuiData->CursorSize = Value;
          else if (Value == 0x64)
              GuiData->CursorSize = Value;
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
GuiConsoleUseDefaults(PGUI_CONSOLE_DATA GuiData)
{
  /*
   * init guidata with default properties
   */

  wcscpy(GuiData->FontName, L"Bitstream Vera Sans Mono");
  GuiData->FontSize = 0x0008000C; // font is 8x12
  GuiData->FontWeight = FW_NORMAL;
  GuiData->CursorSize = 0;
  GuiData->HistoryNoDup = FALSE;
  GuiData->FullScreen = FALSE;
  GuiData->QuickEdit = FALSE;
  GuiData->InsertMode = TRUE;
}



static BOOL FASTCALL
GuiConsoleHandleNcCreate(HWND hWnd, CREATESTRUCTW *Create)
{
  RECT Rect;
  PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) Create->lpCreateParams;
  PGUI_CONSOLE_DATA GuiData;
  HDC Dc;
  HFONT OldFont;
  TEXTMETRICW Metrics;
  PCSRSS_PROCESS_DATA ProcessData;
  HKEY hKey;

  GuiData = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY,
                      sizeof(GUI_CONSOLE_DATA) +
                      (Console->Size.X + 1) * sizeof(WCHAR));
  if (NULL == GuiData)
    {
      DPRINT1("GuiConsoleNcCreate: HeapAlloc failed\n");
      return FALSE;
    }

  GuiConsoleUseDefaults(GuiData);
  if (Console->ProcessList.Flink != &Console->ProcessList)
    {
      ProcessData = CONTAINING_RECORD(Console->ProcessList.Flink, CSRSS_PROCESS_DATA, ProcessEntry);
      if (GuiConsoleOpenUserSettings(hWnd, PtrToUlong(ProcessData->ProcessId), &hKey, KEY_READ))
        {
          GuiConsoleReadUserSettings(hKey, GuiData);
          RegCloseKey(hKey);
        }
    }

  InitializeCriticalSection(&GuiData->Lock);

  GuiData->LineBuffer = (PWCHAR)(GuiData + 1);

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
GuiConsoleSetTextColors(HDC Dc, BYTE Attribute)
{
  SetTextColor(Dc, GuiConsoleRGBFromAttribute(Attribute & 0x0f));
  SetBkColor(Dc, GuiConsoleRGBFromAttribute((Attribute & 0xf0) >> 4));
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
    HBRUSH CursorBrush, OldBrush;
    HFONT OldFont;

    Buff = Console->ActiveBuffer;

    TopLine = rc->top / GuiData->CharHeight;
    BottomLine = (rc->bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1;
    LeftChar = rc->left / GuiData->CharWidth;
    RightChar = (rc->right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1;
    LastAttribute = Buff->Buffer[(TopLine * Buff->MaxX + LeftChar) * 2 + 1];

    GuiConsoleSetTextColors(hDC,
                            LastAttribute);

    EnterCriticalSection(&Buff->Header.Lock);

    OldFont = SelectObject(hDC,
                           GuiData->Font);

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
                                            Attribute);
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
            CursorBrush = CreateSolidBrush(GuiConsoleRGBFromAttribute(*From));
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
GuiConsoleShowConsoleProperties(HWND hWnd, BOOL Defaults)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  APPLET_PROC CPLFunc;
  TCHAR szBuffer[MAX_PATH];

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);

  if (GuiData == NULL)
  {
    DPRINT1("GuiConsoleGetDataPointers failed\n");
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

  CPLFunc(hWnd, CPL_DBLCLK, 0, Defaults);

  // TODO
  //
  // read back the changes from console.dll
  //
  // if the changes are system-wide then 
  // console.dll should have written it to
  // registry
  //
  // if the changes only apply to this session
  // then exchange this info with console.dll in
  // some private way
}
static LRESULT FASTCALL
GuiConsoleHandleSysMenuCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  DPRINT1("GuiConsoleHandleSysMenuCommand entered %d\n", wParam);

  switch(wParam)
    {
      case IDS_MARK:
      case IDS_COPY:
      case IDS_PASTE:
      case IDS_SELECTALL:
      case IDS_SCROLL:
      case IDS_FIND:
        break;

      case IDS_DEFAULTS:
        GuiConsoleShowConsoleProperties(hWnd, TRUE);
        break;
      case IDS_PROPERTIES:
        GuiConsoleShowConsoleProperties(hWnd, FALSE);
        break;
      default:
        break;
   }

  return 0;
}
static BOOLEAN FASTCALL
InsertItem(HMENU hMenu, INT fType, INT fMask, INT fState, HMENU hSubMenu, INT ResourceId)
{
  MENUITEMINFO MenuItemInfo;
  TCHAR szBuffer[MAX_PATH];

  memset(&MenuItemInfo, 0x0, sizeof(MENUITEMINFO));
  MenuItemInfo.cbSize = sizeof (MENUITEMINFO);
  MenuItemInfo.fMask = fMask;
  MenuItemInfo.fType = fType;
  MenuItemInfo.fState = fState;
  MenuItemInfo.hSubMenu = hSubMenu;
  MenuItemInfo.wID = ResourceId;

  if (fType != MFT_SEPARATOR)
    {
      MenuItemInfo.cch = LoadString(Win32CsrDllHandle, ResourceId, szBuffer, MAX_PATH);
      if (!MenuItemInfo.cch)
        {
          DPRINT("LoadString failed ResourceId %d Error %x\n", ResourceId, GetLastError());
          return FALSE;
        }
        MenuItemInfo.dwTypeData = szBuffer;
    }

  if (InsertMenuItem(hMenu, ResourceId, FALSE, &MenuItemInfo))
    return TRUE;

  DPRINT("InsertMenuItem failed Last error %x\n", GetLastError());
  return FALSE;
}



static VOID FASTCALL
GuiConsoleCreateSysMenu(HWND hWnd)
{
  HMENU hMenu;
  HMENU hSubMenu;


  hMenu = GetSystemMenu(hWnd, FALSE);
  if (hMenu == NULL)
    {
      DPRINT("GetSysMenu failed\n");
      return;
    }
  /* insert seperator */
  InsertItem(hMenu, MFT_SEPARATOR, MIIM_FTYPE, 0, NULL, -1);

    /* create submenu */
  hSubMenu = CreatePopupMenu();
  InsertItem(hSubMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_MARK);
  InsertItem(hSubMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING | MIIM_STATE, MFS_GRAYED, NULL, IDS_COPY);
  InsertItem(hSubMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_PASTE);
  InsertItem(hSubMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SELECTALL);
  InsertItem(hSubMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_SCROLL);
  InsertItem(hSubMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_FIND);
  InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING | MIIM_SUBMENU, 0, hSubMenu, IDS_EDIT);

  /* create default/properties item */
  InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_DEFAULTS);
  InsertItem(hMenu, MIIM_STRING, MIIM_ID | MIIM_FTYPE | MIIM_STRING, 0, NULL, IDS_PROPERTIES);
  DrawMenuBar(hWnd);
}

static LRESULT CALLBACK
GuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result = 0;

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
          return GuiConsoleHandleSysMenuCommand(hWnd, wParam, lParam);		
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
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // | WS_HSCROLL | WS_VSCROLL,
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
        Console->hWindow = NewWindow;
        if (NULL != NewWindow)
          {
            GuiConsoleCreateSysMenu(NewWindow);
            //ShowScrollBar(NewWindow, SB_VERT, FALSE);
            //ShowScrollBar(NewWindow, SB_HORZ, FALSE);
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
  wc.hIcon = LoadIconW(Win32CsrDllHandle, MAKEINTRESOURCEW(1));
  wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIconSm = LoadImageW(Win32CsrDllHandle, MAKEINTRESOURCEW(1), IMAGE_ICON,
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
  Buffer->DefaultAttrib = 0x0f;
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
  GuiChangeTitle,
  GuiCleanupConsole,
  GuiChangeIcon
};

NTSTATUS FASTCALL
GuiInitConsole(PCSRSS_CONSOLE Console)
{
  HANDLE GraphicsStartupEvent;
  HANDLE ThreadHandle;

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
  Console->Size.X = 80;
  Console->Size.Y = 25;
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

  PostMessageW(NotifyWnd, PM_CREATE_CONSOLE, 0, (LPARAM) Console);

  return STATUS_SUCCESS;
}

/* EOF */
