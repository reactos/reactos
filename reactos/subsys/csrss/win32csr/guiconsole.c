/* $Id: guiconsole.c,v 1.11 2004/03/07 21:00:11 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/guiconsole.c
 * PURPOSE:         Implementation of gui-mode consoles
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include "conio.h"
#include "guiconsole.h"
#include "win32csr.h"

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
  HDC MemoryDC;
  HBITMAP MemoryBitmap;
} GUI_CONSOLE_DATA, *PGUI_CONSOLE_DATA;

#ifndef WM_APP
#define WM_APP 0x8000
#endif
#define PM_CREATE_CONSOLE  (WM_APP + 1)
#define PM_DESTROY_CONSOLE (WM_APP + 2)

#define CURSOR_BLINK_TIME 500

static BOOL Initialized = FALSE;
static HWND NotifyWnd;

/* FUNCTIONS *****************************************************************/

static VOID FASTCALL
GuiConsoleGetDataPointers(HWND hWnd, PCSRSS_CONSOLE *Console, PGUI_CONSOLE_DATA *GuiData)
{
  *Console = (PCSRSS_CONSOLE) GetWindowLongW(hWnd, GWL_USERDATA);
  *GuiData = (NULL == *Console ? NULL : (*Console)->PrivateData);
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
  NTSTATUS Status;

  GuiData = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY,
                      sizeof(GUI_CONSOLE_DATA) +
                      (Console->Size.X + 1) * sizeof(WCHAR));
  if (NULL == GuiData)
    {
      DPRINT1("GuiConsoleNcCreate: HeapAlloc failed\n");
      return FALSE;
    }

  Status = RtlInitializeCriticalSection(&GuiData->Lock);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlInitializeCriticalSection failed, Status=%x\n", Status);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
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
      DPRINT1("GuiConsoleNcCreate: CreateFont failed\n");
      RtlDeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  Dc = GetDC(hWnd);
  if (NULL == Dc)
    {
      DPRINT1("GuiConsoleNcCreate: GetDC failed\n");
      DeleteObject(GuiData->Font);
      RtlDeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  OldFont = SelectObject(Dc, GuiData->Font);
  if (NULL == OldFont)
    {
      DPRINT1("GuiConsoleNcCreate: SelectObject failed\n");
      ReleaseDC(hWnd, Dc);
      DeleteObject(GuiData->Font);
      RtlDeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  if (! GetTextMetricsW(Dc, &Metrics))
    {
      DPRINT1("GuiConsoleNcCreate: GetTextMetrics failed\n");
      SelectObject(Dc, OldFont);
      ReleaseDC(hWnd, Dc);
      DeleteObject(GuiData->Font);
      RtlDeleteCriticalSection(&GuiData->Lock);
      HeapFree(Win32CsrApiHeap, 0, GuiData);
      return FALSE;
    }
  GuiData->CharWidth = Metrics.tmMaxCharWidth;
  GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;
  SelectObject(Dc, OldFont);

  GuiData->MemoryDC = CreateCompatibleDC(Dc);
  GuiData->MemoryBitmap = CreateCompatibleBitmap(Dc, 
                                                 Console->Size.X * GuiData->CharWidth, 
						 Console->Size.Y * GuiData->CharHeight);
  DeleteObject(SelectObject(GuiData->MemoryDC, GuiData->MemoryBitmap));
  DeleteObject(SelectObject(GuiData->MemoryDC, GuiData->Font));


  ReleaseDC(hWnd, Dc);
  GuiData->CursorBlinkOn = TRUE;
  GuiData->ForceCursorOff = FALSE;

  Console->PrivateData = GuiData;
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

VOID FASTCALL
GuiConsoleUpdateBitmap(HWND hWnd, RECT rc)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;
  PCSRSS_SCREEN_BUFFER Buff;
  HDC Dc;
  ULONG TopLine, BottomLine, LeftChar, RightChar;
  ULONG Line, Char, Start;
  PBYTE From;
  PWCHAR To;
  BYTE LastAttribute, Attribute;
  ULONG CursorX, CursorY, CursorHeight;
  HBRUSH CursorBrush, OldBrush;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (NULL != Console && NULL != GuiData && NULL != Console->ActiveBuffer)
    {
      Buff = Console->ActiveBuffer;
      EnterCriticalSection(&Buff->Header.Lock);
      Dc = GetDC(hWnd);
      if (rc.right <= rc.left || rc.bottom <= rc.top)
        {
          ReleaseDC(hWnd, Dc);
          LeaveCriticalSection(&Buff->Header.Lock);
          return;
        }

      EnterCriticalSection(&GuiData->Lock);

      TopLine = rc.top / GuiData->CharHeight;
      BottomLine = (rc.bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1;
      LeftChar = rc.left / GuiData->CharWidth;
      RightChar = (rc.right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1;
      LastAttribute = Buff->Buffer[(TopLine * Buff->MaxX + LeftChar) * 2 + 1];
      GuiConsoleSetTextColors(GuiData->MemoryDC, LastAttribute);

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
                  TextOutW(GuiData->MemoryDC, Start * GuiData->CharWidth, Line * GuiData->CharHeight,
                           GuiData->LineBuffer, Char - Start);
                  Start = Char;
                  To = GuiData->LineBuffer;
                  Attribute = *(From + 1);
                  if (Attribute != LastAttribute)
                    {
                      GuiConsoleSetTextColors(GuiData->MemoryDC, Attribute);
                      LastAttribute = Attribute;
                    }
                }  
              *((PBYTE) To) = *From;
              *(((PBYTE) To) + 1) = '\0';
              To++;
              From += 2;
            }
          TextOutW(GuiData->MemoryDC, Start * GuiData->CharWidth, Line * GuiData->CharHeight,
                   GuiData->LineBuffer, RightChar - Start + 1);
        }

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
              OldBrush = SelectObject(GuiData->MemoryDC, CursorBrush);
              PatBlt(GuiData->MemoryDC, CursorX * GuiData->CharWidth,
                     CursorY * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                     GuiData->CharWidth, CursorHeight, PATCOPY);
              SelectObject(GuiData->MemoryDC, OldBrush);
              DeleteObject(CursorBrush);
            }
        }

      LeaveCriticalSection(&GuiData->Lock);
      ReleaseDC(hWnd, Dc);
      LeaveCriticalSection(&Buff->Header.Lock);
      InvalidateRect(hWnd, &rc, FALSE);
   }

}

VOID FASTCALL
GuiConsoleHandlePaint(HWND hWnd)
{
  PAINTSTRUCT Ps;
  HDC Dc;
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  if (NULL != Console && NULL != GuiData)
    {
      EnterCriticalSection(&GuiData->Lock);
      Dc = BeginPaint (hWnd, &Ps);
      BitBlt(Dc, Ps.rcPaint.left, Ps.rcPaint.top,
             Ps.rcPaint.right - Ps.rcPaint.left + 1,
             Ps.rcPaint.bottom - Ps.rcPaint.top + 1, GuiData->MemoryDC,
             Ps.rcPaint.left, Ps.rcPaint.top, SRCCOPY);
      EndPaint (hWnd, &Ps);
      LeaveCriticalSection(&GuiData->Lock);
    }
  else
    {
      Dc = BeginPaint (hWnd, &Ps);
      EndPaint (hWnd, &Ps);
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

  GuiConsoleUpdateBitmap(Wnd, RegionRect);
}

static VOID STDCALL
GuiDrawRegion(PCSRSS_CONSOLE Console, RECT *Region)
{
  PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA) Console->PrivateData;

  if (NULL == Console->hWindow || NULL == GuiData)
    {
      return;
    }

  GuiIntDrawRegion(GuiData, Console->hWindow, Region);
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
GuiWriteStream(PCSRSS_CONSOLE Console, RECT *Region, UINT CursorStartX, UINT CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
  PGUI_CONSOLE_DATA GuiData = (PGUI_CONSOLE_DATA) Console->PrivateData;
  PCSRSS_SCREEN_BUFFER Buff = Console->ActiveBuffer;
  LONG CursorEndX, CursorEndY;
  RECT Source, Dest;

  if (NULL == Console->hWindow || NULL == GuiData)
    {
      return;
    }

  if (0 != ScrolledLines)
    {
      Source.left = 0;
      Source.top = ScrolledLines;
      Source.right = Console->Size.X - 1;
      Source.bottom = ScrolledLines + Region->top - 1;
      Dest.left = 0;
      Dest.top = 0;
      Dest.right = Console->Size.X - 1;
      Dest.bottom = Region->top - 1;

      GuiConsoleCopyRegion(Console->hWindow, &Source, &Dest);
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
  /* FIXME for now, just ignore close requests */
}

static VOID FASTCALL
GuiConsoleHandleNcDestroy(HWND hWnd)
{
  PCSRSS_CONSOLE Console;
  PGUI_CONSOLE_DATA GuiData;

  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);
  KillTimer(hWnd, 1);
  Console->PrivateData = NULL;
  DeleteDC(GuiData->MemoryDC);
  RtlDeleteCriticalSection(&GuiData->Lock);
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
        Console->hWindow = NewWindow;
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
  HDESK Desktop;
  NTSTATUS Status;
  WNDCLASSEXW wc;

  Desktop = OpenDesktopW(L"Default", 0, FALSE, GENERIC_ALL);
  if (NULL == Desktop)
    {
      DPRINT1("Failed to open desktop\n");
      return FALSE;
    }
  Status = NtSetInformationProcess(NtCurrentProcess(),
                                   ProcessDesktop,
                                   &Desktop,
                                   sizeof(Desktop));
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Cannot set default desktop.\n");
      return FALSE;
    }
  if (! SetThreadDesktop(Desktop))
    {
      DPRINT1("Failed to set thread desktop\n");
      return FALSE;
    }

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
  wc.lpszClassName = L"Win32CsrConsole";
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

STATIC BOOL STDCALL
GuiChangeTitle(PCSRSS_CONSOLE Console)
{
  SendMessageW(Console->hWindow, WM_SETTEXT, 0, (LPARAM) Console->Title.Buffer);

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
  GuiCleanupConsole
};

NTSTATUS FASTCALL
GuiInitConsole(PCSRSS_CONSOLE Console)
{
  HANDLE GraphicsStartupEvent;
  HANDLE ThreadHandle;

  if (! Initialized)
    {
      Initialized = TRUE;
      if (! GuiInit())
        {
          Initialized = FALSE;
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

VOID STDCALL
GuiConsoleCopyRegion(HWND hWnd,
                     RECT *Source,
                     RECT *Dest)
{
  RECT ScrollRect;
  PGUI_CONSOLE_DATA GuiData;
  PCSRSS_CONSOLE Console;


  GuiConsoleGetDataPointers(hWnd, &Console, &GuiData);

  ScrollRect.left = Dest->left * GuiData->CharWidth;
  ScrollRect.right = (Dest->right + 1) * GuiData->CharWidth;
  ScrollRect.top = Dest->top * GuiData->CharHeight;
  ScrollRect.bottom = (Dest->bottom + 1) * GuiData->CharHeight;
  EnterCriticalSection(&GuiData->Lock);
  BitBlt(GuiData->MemoryDC, ScrollRect.left, ScrollRect.top, 
         ScrollRect.right - ScrollRect.left, ScrollRect.bottom - ScrollRect.top,
	 GuiData->MemoryDC, Source->left * GuiData->CharWidth, Source->top * GuiData->CharHeight, SRCCOPY);

  LeaveCriticalSection(&GuiData->Lock);

  InvalidateRect(hWnd, &ScrollRect, FALSE);
}

/* EOF */
