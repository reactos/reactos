/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/tuiconsole.c
 * PURPOSE:         Implementation of text-mode consoles
 */

#define NDEBUG
#include "w32csr.h"
#include <debug.h>

CRITICAL_SECTION ActiveConsoleLock;
static COORD PhysicalConsoleSize;
static HANDLE ConsoleDeviceHandle;
static PCSRSS_CONSOLE ActiveConsole;

static BOOL ConsInitialized = FALSE;

static LRESULT CALLBACK
TuiConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_ACTIVATE)
    {
      if (LOWORD(wParam) != WA_INACTIVE)
        {
          SetFocus(hWnd);
          ConioDrawConsole(ActiveConsole);
        }
    }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static BOOL FASTCALL
TuiInit(VOID)
{
  CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
  DWORD BytesReturned;
  WNDCLASSEXW wc;

  ConsoleDeviceHandle = CreateFileW(L"\\\\.\\BlueScreen", FILE_ALL_ACCESS, 0, NULL,
                                    OPEN_EXISTING, 0, NULL);
  if (INVALID_HANDLE_VALUE == ConsoleDeviceHandle)
    {
      DPRINT1("Failed to open BlueScreen.\n");
      return FALSE;
    }

  ActiveConsole = NULL;
  InitializeCriticalSection(&ActiveConsoleLock);
  if (! DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO,
                        NULL, 0, &ScrInfo, sizeof(ScrInfo), &BytesReturned, NULL))
    {
      DPRINT1("Failed to get console info\n");
      return FALSE;
    }
  PhysicalConsoleSize = ScrInfo.dwSize;

  RtlZeroMemory(&wc, sizeof(WNDCLASSEXW));
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpszClassName = L"TuiConsoleWindowClass";
  wc.lpfnWndProc = TuiConsoleWndProc;
  wc.hInstance = (HINSTANCE) GetModuleHandleW(NULL);
  if (RegisterClassExW(&wc) == 0)
    {
      DPRINT1("Failed to register console wndproc\n");
      return FALSE;
    }

  return TRUE;
}

static VOID WINAPI
TuiInitScreenBuffer(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buffer)
{
  Buffer->DefaultAttrib = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_BLUE;
}

static void FASTCALL
TuiCopyRect(char *Dest, PCSRSS_SCREEN_BUFFER Buff, RECT *Region)
{
  UINT SrcDelta, DestDelta;
  LONG i;
  PBYTE Src, SrcEnd;

  Src = ConioCoordToPointer(Buff, Region->left, Region->top);
  SrcDelta = Buff->MaxX * 2;
  SrcEnd = Buff->Buffer + Buff->MaxY * Buff->MaxX * 2;
  DestDelta = ConioRectWidth(Region) * 2;
  for (i = Region->top; i <= Region->bottom; i++)
    {
      memcpy(Dest, Src, DestDelta);
      Src += SrcDelta;
      if (SrcEnd <= Src)
        {
          Src -= Buff->MaxY * Buff->MaxX * 2;
        }
      Dest += DestDelta;
    }
}

static VOID WINAPI
TuiDrawRegion(PCSRSS_CONSOLE Console, RECT *Region)
{
  DWORD BytesReturned;
  PCSRSS_SCREEN_BUFFER Buff = Console->ActiveBuffer;
  PCONSOLE_DRAW ConsoleDraw;
  UINT ConsoleDrawSize;

  if (ActiveConsole != Console)
    {
      return;
    }

  ConsoleDrawSize = sizeof(CONSOLE_DRAW) +
                    (ConioRectWidth(Region) * ConioRectHeight(Region)) * 2;
  ConsoleDraw = HeapAlloc(Win32CsrApiHeap, 0, ConsoleDrawSize);
  if (NULL == ConsoleDraw)
    {
      DPRINT1("HeapAlloc failed\n");
      return;
    }
  ConsoleDraw->X = Region->left;
  ConsoleDraw->Y = Region->top;
  ConsoleDraw->SizeX = ConioRectWidth(Region);
  ConsoleDraw->SizeY = ConioRectHeight(Region);
  ConsoleDraw->CursorX = Buff->CurrentX;
  ConsoleDraw->CursorY = Buff->CurrentY;

  TuiCopyRect((char *) (ConsoleDraw + 1), Buff, Region);

  if (! DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_DRAW,
                        NULL, 0, ConsoleDraw, ConsoleDrawSize, &BytesReturned, NULL))
    {
      DPRINT1("Failed to draw console\n");
      HeapFree(Win32CsrApiHeap, 0, ConsoleDraw);
      return;
    }

  HeapFree(Win32CsrApiHeap, 0, ConsoleDraw);
}

static VOID WINAPI
TuiWriteStream(PCSRSS_CONSOLE Console, RECT *Region, LONG CursorStartX, LONG CursorStartY,
               UINT ScrolledLines, CHAR *Buffer, UINT Length)
{
  DWORD BytesWritten;
  PCSRSS_SCREEN_BUFFER Buff = Console->ActiveBuffer;

  if (ActiveConsole->ActiveBuffer != Buff)
    {
      return;
    }

  if (! WriteFile(ConsoleDeviceHandle, Buffer, Length, &BytesWritten, NULL))
    {
      DPRINT1("Error writing to BlueScreen\n");
    }
}

static BOOL WINAPI
TuiSetCursorInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff)
{
  DWORD BytesReturned;

  if (ActiveConsole->ActiveBuffer != Buff)
    {
      return TRUE;
    }

  if (! DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_SET_CURSOR_INFO,
                        &Buff->CursorInfo, sizeof(Buff->CursorInfo), NULL, 0,
                        &BytesReturned, NULL))
    {
      DPRINT1( "Failed to set cursor info\n" );
      return FALSE;
    }

  return TRUE;
}

static BOOL WINAPI
TuiSetScreenInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff, UINT OldCursorX, UINT OldCursorY)
{
  CONSOLE_SCREEN_BUFFER_INFO Info;
  DWORD BytesReturned;

  if (ActiveConsole->ActiveBuffer != Buff)
    {
      return TRUE;
    }

  Info.dwCursorPosition.X = Buff->CurrentX;
  Info.dwCursorPosition.Y = Buff->CurrentY;
  Info.wAttributes = Buff->DefaultAttrib;

  if (! DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO,
                        &Info, sizeof(CONSOLE_SCREEN_BUFFER_INFO), NULL, 0,
                        &BytesReturned, NULL))
    {
      DPRINT1( "Failed to set cursor position\n" );
      return FALSE;
    }

  return TRUE;
}

static BOOL WINAPI
TuiUpdateScreenInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff)
{
    return TRUE;
}

static BOOL WINAPI
TuiChangeTitle(PCSRSS_CONSOLE Console)
{
  return TRUE;
}

static VOID WINAPI
TuiCleanupConsole(PCSRSS_CONSOLE Console)
{
  DestroyWindow(Console->hWindow);

  EnterCriticalSection(&ActiveConsoleLock);

  /* Switch to next console */
  if (ActiveConsole == Console)
    {
      ActiveConsole = Console->Next != Console ? Console->Next : NULL;
    }

  if (Console->Next != Console)
    {
      Console->Prev->Next = Console->Next;
      Console->Next->Prev = Console->Prev;
    }
  LeaveCriticalSection(&ActiveConsoleLock);

  if (NULL != ActiveConsole)
    {
      ConioDrawConsole(ActiveConsole);
    }
}

DWORD WINAPI
TuiConsoleThread (PVOID Data)
{
  PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) Data;
  HWND NewWindow;
  MSG msg;

  NewWindow = CreateWindowW(L"TuiConsoleWindowClass",
                            Console->Title.Buffer,
                            0,
                            -32000, -32000, 0, 0,
                            NULL, NULL,
                            (HINSTANCE) GetModuleHandleW(NULL),
                            (PVOID) Console);
  Console->hWindow = NewWindow;
  if (NULL == NewWindow)
    {
      DPRINT1("CSR: Unable to create console window\n");
      return 1;
    }

  SetForegroundWindow(Console->hWindow);

  while (TRUE)
    {
      GetMessageW(&msg, 0, 0, 0);
      DispatchMessage(&msg);
      TranslateMessage(&msg);

      if (msg.message == WM_CHAR || msg.message == WM_SYSCHAR ||
          msg.message == WM_KEYDOWN || msg.message == WM_KEYUP ||
          msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP)
        {
          ConioProcessKey(&msg, Console, TRUE);
        }
    }

  return 0;
}

static CSRSS_CONSOLE_VTBL TuiVtbl =
{
  TuiInitScreenBuffer,
  TuiWriteStream,
  TuiDrawRegion,
  TuiSetCursorInfo,
  TuiSetScreenInfo,
  TuiUpdateScreenInfo,
  TuiChangeTitle,
  TuiCleanupConsole,
  NULL  // ChangeIcon
};

NTSTATUS FASTCALL
TuiInitConsole(PCSRSS_CONSOLE Console)
{
  HANDLE ThreadHandle;

  if (! ConsInitialized)
    {
      ConsInitialized = TRUE;
      if (! TuiInit())
        {
          ConsInitialized = FALSE;
          return STATUS_UNSUCCESSFUL;
        }
    }

  Console->Vtbl = &TuiVtbl;
  Console->hWindow = NULL;
  Console->Size = PhysicalConsoleSize;
  Console->ActiveBuffer->MaxX = PhysicalConsoleSize.X;
  Console->ActiveBuffer->MaxY = PhysicalConsoleSize.Y;

  ThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) TuiConsoleThread,
                              Console, 0, NULL);
  if (NULL == ThreadHandle)
    {
      DPRINT1("CSR: Unable to create console thread\n");
      return STATUS_UNSUCCESSFUL;
    }
  CloseHandle(ThreadHandle);

  EnterCriticalSection(&ActiveConsoleLock);
  if (NULL != ActiveConsole)
    {
      Console->Prev = ActiveConsole;
      Console->Next = ActiveConsole->Next;
      ActiveConsole->Next->Prev = Console;
      ActiveConsole->Next = Console;
    }
  else
    {
      Console->Prev = Console;
      Console->Next = Console;
    }
  ActiveConsole = Console;
  LeaveCriticalSection(&ActiveConsoleLock);

  return STATUS_SUCCESS;
}

PCSRSS_CONSOLE FASTCALL
TuiGetFocusConsole(VOID)
{
  return ActiveConsole;
}

BOOL FASTCALL
TuiSwapConsole(int Next)
{
  static PCSRSS_CONSOLE SwapConsole = NULL; /* console we are thinking about swapping with */
  DWORD BytesReturned;
  ANSI_STRING Title;
  void * Buffer;
  COORD *pos;

  if (0 != Next)
    {
      /* alt-tab, swap consoles */
      /* move SwapConsole to next console, and print its title */
      EnterCriticalSection(&ActiveConsoleLock);
      if (! SwapConsole)
        {
          SwapConsole = ActiveConsole;
        }

      SwapConsole = (0 < Next ? SwapConsole->Next : SwapConsole->Prev);
      Title.MaximumLength = RtlUnicodeStringToAnsiSize(&SwapConsole->Title);
      Title.Length = 0;
      Buffer = HeapAlloc(Win32CsrApiHeap,
                               0,
                               sizeof(COORD) + Title.MaximumLength);
      pos = (COORD *)Buffer;
      Title.Buffer = (PVOID)((ULONG_PTR)Buffer + sizeof( COORD ));

      RtlUnicodeStringToAnsiString(&Title, &SwapConsole->Title, FALSE);
      pos->Y = PhysicalConsoleSize.Y / 2;
      pos->X = (PhysicalConsoleSize.X - Title.Length) / 2;
      /* redraw the console to clear off old title */
      ConioDrawConsole(ActiveConsole);
      if (! DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
                            NULL, 0, Buffer, sizeof(COORD) + Title.Length,
                            &BytesReturned, NULL))
        {
          DPRINT1( "Error writing to console\n" );
        }
      HeapFree(Win32CsrApiHeap, 0, Buffer);
      LeaveCriticalSection(&ActiveConsoleLock);

      return TRUE;
    }
  else if (NULL != SwapConsole)
    {
      EnterCriticalSection(&ActiveConsoleLock);
      if (SwapConsole != ActiveConsole)
        {
          /* first remove swapconsole from the list */
          SwapConsole->Prev->Next = SwapConsole->Next;
          SwapConsole->Next->Prev = SwapConsole->Prev;
          /* now insert before activeconsole */
          SwapConsole->Next = ActiveConsole;
          SwapConsole->Prev = ActiveConsole->Prev;
          ActiveConsole->Prev->Next = SwapConsole;
          ActiveConsole->Prev = SwapConsole;
        }
      ActiveConsole = SwapConsole;
      SwapConsole = NULL;
      ConioDrawConsole(ActiveConsole);
      LeaveCriticalSection(&ActiveConsoleLock);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

/* EOF */
