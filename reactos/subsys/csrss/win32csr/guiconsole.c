/* $Id: guiconsole.c,v 1.3 2003/12/07 23:02:57 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/dllmain.c
 * PURPOSE:         Initialization 
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include "conio.h"
#include "guiconsole.h"
#include "win32csr.h"

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
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;

#ifndef WM_APP
#define WM_APP 0x8000
#endif
#define PM_CREATE_CONSOLE  (WM_APP + 1)
#define PM_DESTROY_CONSOLE (WM_APP + 2)

#define PM_COPY_REGION     (WM_APP + 100)

#define CURSOR_BLINK_TIME 500

static HWND NotifyWnd;

/* FUNCTIONS *****************************************************************/

static VOID FASTCALL
GuiConsoleGetDataPointers(HWND hWnd, PCSRSS_CONSOLE *Console, PGUI_CONSOLE_DATA *GuiData)
{
  *Console = (PCSRSS_CONSOLE) GetWindowLongW(hWnd, GWL_USERDATA);
  *GuiData = (NULL == *Console ? NULL : (*Console)->GuiConsoleData);
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

  GuiData = HeapAlloc(Win32CsrApiHeap, 0,
                      sizeof(GUI_CONSOLE_DATA) +
                      (Console->Size.X + 1) * sizeof(WCHAR));
  if (NULL == GuiData)
    {
      DbgPrint("GuiConsoleNcCreate: HeapAlloc failed\n");
      return FALSE;
    }
  GuiData->LineBuffer = (PWCHAR)(GuiData + 1);

  GuiData->Font = CreateFontW(12, 0, 0, TA_BASELINE, FW_NORMAL,
                              FALSE, FALSE, FALSE, ANSI_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE,
                              L"Bitstream Vera Sans Mono");
  if (NULL == GuiData->Font)
    {
      DbgPrint("GuiConsoleNcCreate: CreateFont failed\n");
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  Dc = GetDC(hWnd);
  if (NULL == Dc)
    {
      DbgPrint("GuiConsoleNcCreate: GetDC failed\n");
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  OldFont = SelectObject(Dc, GuiData->Font);
  if (NULL == OldFont)
    {
      DbgPrint("GuiConsoleNcCreate: SelectObject failed\n");
      ReleaseDC(hWnd, Dc);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  if (! GetTextMetricsW(Dc, &Metrics))
    {
      DbgPrint("GuiConsoleNcCreate: GetTextMetrics failed\n");
      SelectObject(Dc, OldFont);
      ReleaseDC(hWnd, Dc);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  GuiData->CharWidth = Metrics.tmMaxCharWidth;
  GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;
  SelectObject(Dc, OldFont);
  ReleaseDC(hWnd, Dc);
  GuiData->CursorBlinkOn = TRUE;
  GuiData->ForceCursorOff = FALSE;

  Console->GuiConsoleData = GuiData;
  SetWindowLongW(hWnd, GWL_USERDATA, (LONG) Console);

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
GuiConsoleHandlePaint(HWND hWnd)
{
  PAINTSTRUCT Ps;
  HDC Dc;
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  PCSRSS_SCREEN_BUFFER Buff;
  unsigned TopLine, BottomLine, LeftChar, RightChar;
  unsigned Line, Char, Start;
  HFONT OldFont;
  PCHAR From;
  PWCHAR To;
  BYTE LastAttribute, Attribute;
  ULONG CursorX, CursorY, CursorHeight;
  HBRUSH CursorBrush, OldBrush;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (NULL != Console && NULL != GuiData && NULL != Console->ActiveBuffer)
    {
      Buff = Console->ActiveBuffer;
      EnterCriticalSection(&(Buff->Lock));

      Dc = BeginPaint(hWnd, &Ps);
      if (Ps.rcPaint.right <= Ps.rcPaint.left || Ps.rcPaint.bottom <= Ps.rcPaint.top)
        {
          EndPaint(hWnd, &Ps);
          LeaveCriticalSection(&(Buff->Lock));
          return;
        }
      OldFont = SelectObject(Dc, GuiData->Font);

      TopLine = Ps.rcPaint.top / GuiData->CharHeight;
      BottomLine = (Ps.rcPaint.bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1;
      LeftChar = Ps.rcPaint.left / GuiData->CharWidth;
      RightChar = (Ps.rcPaint.right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1;
      LastAttribute = Buff->Buffer[(TopLine * Buff->MaxX + LeftChar) * 2 + 1];
      GuiConsoleSetTextColors(Dc, LastAttribute);
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
          Attribute = *(From + 1);
          Start = LeftChar;
          To = GuiData->LineBuffer;
          for (Char = LeftChar; Char <= RightChar; Char++)
            {
              if (*(From + 1) != Attribute)
                {
                  TextOutW(Dc, Start * GuiData->CharWidth, Line * GuiData->CharHeight,
                           GuiData->LineBuffer, Char - Start);
                  Start = Char;
                  Attribute = *(From + 1);
                  if (Attribute != LastAttribute)
                    {
                      GuiConsoleSetTextColors(Dc, LastAttribute);
                    }
                }  
              *((PBYTE) To) = *From;
              *(((PBYTE) To) + 1) = '\0';
              To++;
              From += 2;
            }
          TextOutW(Dc, Start * GuiData->CharWidth, Line * GuiData->CharHeight,
                   GuiData->LineBuffer, RightChar - Start + 1);
        }

      SelectObject(Dc, OldFont);

      if (Buff->CursorInfo.bVisible && GuiData->CursorBlinkOn
          &&! GuiData->ForceCursorOff)
        {
          GuiConsoleGetLogicalCursorPos(Buff, &CursorX, &CursorY);
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
              OldBrush = SelectObject(Dc, CursorBrush);
              PatBlt(Dc, CursorX * GuiData->CharWidth,
                     CursorY * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                     GuiData->CharWidth, CursorHeight, PATCOPY);
              SelectObject(Dc, OldBrush);
              DeleteObject(CursorBrush);
            }
        }
      EndPaint(hWnd, &Ps);

      LeaveCriticalSection(&(Buff->Lock));
    }
  else
    {
      Dc = BeginPaint(hWnd, &Ps);
      EndPaint(hWnd, &Ps);
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

  CsrProcessKey(&Message, Console);
}

static VOID FASTCALL
GuiConsoleHandleCopyRegion(HWND hWnd, PRECT Source, PRECT Dest)
{
  HDC Dc;
  int XDest, YDest, Width, Height, XSrc, YSrc;
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  RECT CursorRect, UpdateRect;
  DWORD CursorX, CursorY;
  PCSRSS_SCREEN_BUFFER Buff;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  Buff = Console->ActiveBuffer;

  /* Check if the cursor is in the source rectangle and if it is,
   * make sure it's invisible */ 
  GuiConsoleGetLogicalCursorPos(Buff, &CursorX, &CursorY);
  if (Source->left <= CursorX && CursorX <= Source->right &&
      Source->top <= CursorY && CursorY <= Source->bottom)
    {
      GuiData->ForceCursorOff = TRUE;

      CursorRect.left = CursorX * GuiData->CharWidth;
      CursorRect.top = CursorY * GuiData->CharHeight;
      CursorRect.right = (CursorX + 1) * GuiData->CharWidth;
      CursorRect.bottom = (CursorY + 1) * GuiData->CharHeight;

      InvalidateRect(hWnd, &CursorRect, FALSE);
    }


  /* This is a bit tricky. We want to copy a source rectangle to
   * a destination rectangle, but there is no guarantee that the
   * contents of the source rectangle is valid. First let's try
   * to make it as valid as possible by painting all outstanding
   * updates. To do that we have to release the lock, otherwise
   * the paint code can't acquire it */

  UpdateWindow(hWnd);

  Dc = GetDC(hWnd);
  if (NULL == Dc)
    {
      return;
    }

  XSrc = Source->left * GuiData->CharWidth;
  YSrc = Source->top * GuiData->CharHeight;
  XDest = Dest->left * GuiData->CharWidth;
  YDest = Dest->top * GuiData->CharHeight;
  Width = (Dest->right - Dest->left + 1) * GuiData->CharWidth;
  Height = (Dest->bottom - Dest->top + 1) * GuiData->CharHeight;

  BitBlt(Dc, XDest, YDest, Width, Height, Dc, XSrc, YSrc, SRCCOPY);

  ReleaseDC(hWnd, Dc);

  /* Although we tried to make sure that the source rectangle was
   * up-to-date, this is not guaranteed. For example, the user could
   * have moved a window between the UpdateWindow() and the BitBlt()
   * above, invalidating part of the window. So, we're going to
   * check if the current update rect of the window overlaps with the
   * source rectangle. If it does, we invalidate the corresponding
   * part of the destination rectangle so it will eventually be
   * repainted. Note that this is probably doesn't happen all that
   * often and GetUpdateRect() below will usually return FALSE,
   * indicating that the whole window is valid */

  if (GetUpdateRect(hWnd, &UpdateRect, FALSE))
    {
      UpdateRect.left = max(UpdateRect.left, XSrc);
      UpdateRect.top = max(UpdateRect.top, YSrc);
      UpdateRect.right = min(UpdateRect.right, XSrc + Width);
      UpdateRect.bottom = min(UpdateRect.bottom, YSrc + Height);
      if (UpdateRect.left < UpdateRect.right && UpdateRect.top < UpdateRect.bottom)
        {
          UpdateRect.left += XDest - XSrc;
          UpdateRect.top += YDest - YSrc;
          UpdateRect.right += XDest - XSrc;
          UpdateRect.bottom += YDest - YSrc;

          InvalidateRect(Console->hWindow, &UpdateRect, FALSE);
        }
    }

  /* Show cursor again if we made it invisible */
  if (GuiData->ForceCursorOff)
    {
      GuiData->ForceCursorOff = FALSE;

      InvalidateRect(hWnd, &CursorRect, FALSE);
    }
}

static VOID FASTCALL
GuiConsoleHandleTimer(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  SMALL_RECT CursorRect;
  ULONG CursorX, CursorY;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  GuiData->CursorBlinkOn = ! GuiData->CursorBlinkOn;

  GuiConsoleGetLogicalCursorPos(Console->ActiveBuffer, &CursorX, &CursorY);
  CursorRect.Left = CursorX;
  CursorRect.Top = CursorY;
  CursorRect.Right = CursorX;
  CursorRect.Bottom = CursorY;
  GuiConsoleDrawRegion(Console, CursorRect);
}

static VOID FASTCALL
GuiConsoleHandleClose(HWND hWnd)
{
  /* FIXME for now, just ignore close requests */
}

static VOID FASTCALL
GuiConsoleHandleNcDestroy(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  KillTimer(hWnd, 1);
  Console->GuiConsoleData = NULL;
  HeapFree(Win32CsrApiHeap, 0, GuiData);
}

static LRESULT CALLBACK
GuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT Result;

  switch(msg)
    {
      case WM_NCCREATE:
        Result = (LRESULT) GuiConsoleHandleNcCreate(hWnd, (CREATESTRUCTW *) lParam);
        break;
      case WM_PAINT:
        GuiConsoleHandlePaint(hWnd);
        Result = 0;
        break;
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_CHAR:
        GuiConsoleHandleKey(hWnd, msg, wParam, lParam);
        Result = 0;
        break;
      case WM_TIMER:
        GuiConsoleHandleTimer(hWnd);
        Result = 0;
        break;
      case PM_COPY_REGION:
        GuiConsoleHandleCopyRegion(hWnd, (PRECT) wParam, (PRECT) lParam);
        break;
      case WM_CLOSE:
        GuiConsoleHandleClose(hWnd);
        Result = 0;
        break;
      case WM_NCDESTROY:
        GuiConsoleHandleNcDestroy(hWnd);
        Result = 0;
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
  PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) lParam;

  switch(msg)
    {
      case WM_CREATE:
        SetWindowLongW(hWnd, GWL_USERDATA, 0);
        return 0;
      case PM_CREATE_CONSOLE:
        NewWindow = CreateWindowW(L"Win32CsrConsole",
                                  Console->Title.Buffer,
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  NULL,
                                  NULL,
                                  (HINSTANCE) GetModuleHandleW(NULL),
                                  (PVOID) Console);
        if (NULL != NewWindow)
          {
            SetWindowLongW(hWnd, GWL_USERDATA, GetWindowLongW(hWnd, GWL_USERDATA) + 1);
            ShowWindow(NewWindow, SW_SHOW);
          }
        return (LRESULT) NewWindow;
      case PM_DESTROY_CONSOLE:
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
          }
        return 0;
      default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static DWORD STDCALL
GuiConsoleGuiThread(PVOID Data)
{
  WNDCLASSW wc;
  MSG msg;
  PHANDLE GraphicsStartupEvent = (PHANDLE) Data;

  PrivateCsrssManualGuiCheck(+1);

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
  if (RegisterClassW(&wc) == 0)
    {
      PrivateCsrssManualGuiCheck(-1);
      NtSetEvent(*GraphicsStartupEvent, 0);
      return 1;
    }

  wc.lpszClassName = L"Win32CsrConsole";
  wc.lpfnWndProc = GuiConsoleWndProc;
  wc.style = 0;
  wc.hInstance = (HINSTANCE) GetModuleHandleW(NULL);
  wc.hIcon = LoadIconW(NULL, (LPCWSTR) IDI_APPLICATION);
  wc.hCursor = LoadCursorW(NULL, (LPCWSTR) IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClassW(&wc) == 0)
    {
      PrivateCsrssManualGuiCheck(-1);
      NtSetEvent(*GraphicsStartupEvent, 0);
      return 1;
    }

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
      NtSetEvent(*GraphicsStartupEvent, 0);
      return 1;
    }

  NtSetEvent(*GraphicsStartupEvent, 0);

  while(GetMessageW(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }

  return 1;
}

VOID FASTCALL
GuiConsoleInitConsoleSupport(VOID)
{
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE GraphicsStartupEvent;
  HDESK Desktop;
  HANDLE ThreadHandle;

  Desktop = OpenDesktopW(L"Default", 0, FALSE, GENERIC_ALL);
  if (NULL == Desktop)
    {
      DbgPrint("Win32Csr: failed to open desktop\n");
      return;
    }
  Status = NtSetInformationProcess(NtCurrentProcess(),
                                   ProcessDesktop,
                                   &Desktop,
                                   sizeof(Desktop));
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Win32Csr: cannot set default desktop.\n");
      return;
    }
  if (! SetThreadDesktop(Desktop))
    {
      DbgPrint("Win32Csr: failed to set thread desktop\n");
      return;
    }

  InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_INHERIT, NULL, NULL);

  Status = NtCreateEvent(&GraphicsStartupEvent, STANDARD_RIGHTS_ALL, &ObjectAttributes, FALSE, FALSE);
  if( !NT_SUCCESS(Status))
    {
      return;
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
      DbgPrint("Win32Csr: Failed to create graphics console thread. Expect problems\n");
      return;
    }
  CloseHandle(ThreadHandle);

  NtWaitForSingleObject(GraphicsStartupEvent, FALSE, NULL);
  NtClose(GraphicsStartupEvent);

  if (NULL == NotifyWnd)
    {
      DbgPrint("Win32Csr: Failed to create notification window.\n");
      return;
    }
}

BOOL STDCALL
GuiConsoleInitConsole(PCSRSS_CONSOLE Console)
{
  Console->Size.X = 80;
  Console->Size.Y = 25;
  Console->hWindow = (HWND) SendMessageW(NotifyWnd, PM_CREATE_CONSOLE, 0, (LPARAM) Console);

  return NULL != Console->hWindow;
}

VOID STDCALL
GuiConsoleDrawRegion(PCSRSS_CONSOLE Console, SMALL_RECT Region)
{
  PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA) Console->GuiConsoleData;
  RECT RegionRect;

  if (NULL == Console->hWindow || NULL == GuiData)
    {
      return;
    }

  RegionRect.left = Region.Left * GuiData->CharWidth;
  RegionRect.top = Region.Top * GuiData->CharHeight;
  RegionRect.right = (Region.Right + 1) * GuiData->CharWidth;
  RegionRect.bottom = (Region.Bottom + 1) * GuiData->CharHeight;

  InvalidateRect(Console->hWindow, &RegionRect, FALSE);
}

VOID STDCALL
GuiConsoleCopyRegion(PCSRSS_CONSOLE Console,
                     RECT *Source,
                     RECT *Dest)
{
  LeaveCriticalSection(&(Console->ActiveBuffer->Lock));
  SendMessageW(Console->hWindow, PM_COPY_REGION, (WPARAM) Source, (LPARAM) Dest);
  EnterCriticalSection(&(Console->ActiveBuffer->Lock));
}

VOID STDCALL
GuiConsoleChangeTitle(PCSRSS_CONSOLE Console)
{
  SendMessageW(Console->hWindow, WM_SETTEXT, 0, (LPARAM) Console->Title.Buffer);
}

VOID STDCALL
GuiConsoleDeleteConsole(PCSRSS_CONSOLE Console)
{
  SendMessageW(NotifyWnd, PM_DESTROY_CONSOLE, 0, (LPARAM) Console);
}

/* EOF */
