/* $Id: tuiconsole.c,v 1.1 2004/01/11 17:31:16 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/tuiconsole.c
 * PURPOSE:         Implementation of text-mode consoles
 */

#include <windows.h>
#include <ddk/ntddblue.h>
#include <string.h>
#include "api.h"
#include "conio.h"
#include "tuiconsole.h"
#include "win32csr.h"

#define NDEBUG
#include <debug.h>

CRITICAL_SECTION ActiveConsoleLock;
static COORD PhysicalConsoleSize;
static HANDLE ConsoleDeviceHandle;
static PCSRSS_CONSOLE ActiveConsole;

static BOOL Initialized = FALSE;

static BOOL FASTCALL
TuiInit(VOID)
{
  CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
  DWORD BytesReturned;
 
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

  return TRUE;
}

static VOID STDCALL
TuiInitScreenBuffer(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buffer)
{
  Buffer->DefaultAttrib = 0x17;
}

static void FASTCALL
TuiCopyRect(char *Dest, PCSRSS_SCREEN_BUFFER Buff, RECT *Region)
{
  UINT SrcDelta, DestDelta, i;
  char *Src, *SrcEnd;

  Src = Buff->Buffer + (((Region->top + Buff->ShowY) % Buff->MaxY) * Buff->MaxX
                        + Region->left + Buff->ShowX) * 2;
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

static VOID STDCALL
TuiDrawRegion(PCSRSS_CONSOLE Console, RECT *Region)
{
  DWORD BytesReturned;
  PCSRSS_SCREEN_BUFFER Buff = Console->ActiveBuffer;
  LONG CursorX, CursorY;
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
  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY, &CursorX, &CursorY);
  ConsoleDraw->X = Region->left;
  ConsoleDraw->Y = Region->top;
  ConsoleDraw->SizeX = ConioRectWidth(Region);
  ConsoleDraw->SizeY = ConioRectHeight(Region);
  ConsoleDraw->CursorX = CursorX;
  ConsoleDraw->CursorY = CursorY;
  
  TuiCopyRect((char *) (ConsoleDraw + 1), Buff, Region);
  
  if (! DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_DRAW,
                        ConsoleDraw, ConsoleDrawSize, NULL, 0, &BytesReturned, NULL))
    {
      DPRINT1("Failed to draw console\n");
      HeapFree(Win32CsrApiHeap, 0, ConsoleDraw);
      return;
    }

  HeapFree(Win32CsrApiHeap, 0, ConsoleDraw);
}

static VOID STDCALL
TuiWriteStream(PCSRSS_CONSOLE Console, RECT *Region, UINT CursorStartX, UINT CursorStartY,
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

static BOOL STDCALL
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

static BOOL STDCALL
TuiSetScreenInfo(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff, UINT OldCursorX, UINT OldCursorY)
{
  CONSOLE_SCREEN_BUFFER_INFO Info;
  LONG CursorX, CursorY;
  DWORD BytesReturned;

  if (ActiveConsole->ActiveBuffer != Buff)
    {
      return TRUE;
    }

  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY, &CursorX, &CursorY);
  Info.dwCursorPosition.X = CursorX;
  Info.dwCursorPosition.Y = CursorY;
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

STATIC BOOL STDCALL
TuiChangeTitle(PCSRSS_CONSOLE Console)
{
  return TRUE;
}

STATIC VOID STDCALL
TuiCleanupConsole(PCSRSS_CONSOLE Console)
{
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

static CSRSS_CONSOLE_VTBL TuiVtbl =
{
  TuiInitScreenBuffer,
  TuiWriteStream,
  TuiDrawRegion,
  TuiSetCursorInfo,
  TuiSetScreenInfo,
  TuiChangeTitle,
  TuiCleanupConsole
};

NTSTATUS FASTCALL
TuiInitConsole(PCSRSS_CONSOLE Console)
{
  if (! Initialized)
    {
      Initialized = TRUE;
      if (! TuiInit())
        {
          Initialized = FALSE;
          return STATUS_UNSUCCESSFUL;
        }
    }

  Console->Vtbl = &TuiVtbl;
  Console->hWindow = (HWND) NULL;
  Console->Size = PhysicalConsoleSize;

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
      Title.Buffer = Buffer + sizeof( COORD );

      RtlUnicodeStringToAnsiString(&Title, &SwapConsole->Title, FALSE);
      pos->Y = PhysicalConsoleSize.Y / 2;
      pos->X = (PhysicalConsoleSize.X - Title.Length) / 2;
      /* redraw the console to clear off old title */
      ConioDrawConsole(ActiveConsole);
      if (! DeviceIoControl(ConsoleDeviceHandle, IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
                            Buffer, sizeof(COORD) + Title.Length, NULL, 0,
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
