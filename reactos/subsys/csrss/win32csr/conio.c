/* $Id: conio.c,v 1.5 2004/02/25 18:12:52 hbirr Exp $
 *
 * reactos/subsys/csrss/win32csr/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <string.h>
#include <windows.h>

#include <csrss/csrss.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <ddk/ntddblue.h>
#include <rosrtl/string.h>
#include <rosrtl/minmax.h>
#include "api.h"
#include "conio.h"
#include "desktopbg.h"
#include "guiconsole.h"
#include "tuiconsole.h"
#include "win32csr.h"

#define NDEBUG
#include <debug.h>

/* FIXME: Is there a way to create real aliasses with gcc? [CSH] */
#define ALIAS(Name, Target) typeof(Target) Name = Target

/* Private user32 routines for CSRSS, not defined in any header file */
extern VOID STDCALL PrivateCsrssRegisterPrimitive(VOID);
extern VOID STDCALL PrivateCsrssAcquireOrReleaseInputOwnership(BOOL Release);

/* GLOBALS *******************************************************************/

#define ConioInitRect(Rect, Top, Left, Bottom, Right) \
  ((Rect)->top) = Top; \
  ((Rect)->left) = Left; \
  ((Rect)->bottom) = Bottom; \
  ((Rect)->right) = Right

#define ConioIsRectEmpty(Rect) \
  (((Rect)->left > (Rect)->right) || ((Rect)->top > (Rect)->bottom))

/* FUNCTIONS *****************************************************************/

STATIC NTSTATUS FASTCALL
ConioConsoleFromProcessData(PCSRSS_PROCESS_DATA ProcessData, PCSRSS_CONSOLE *Console)
{
  if (NULL == ProcessData->Console)
    {
      *Console = NULL;
      return STATUS_SUCCESS;
    }

  EnterCriticalSection(&(ProcessData->Console->Header.Lock));
  *Console = ProcessData->Console;

  return STATUS_SUCCESS;
}

STATIC VOID FASTCALL
CsrConsoleCtrlEvent(DWORD Event, PCSRSS_PROCESS_DATA ProcessData)
{
  HANDLE Process, Thread;
	
  DPRINT("CsrConsoleCtrlEvent Parent ProcessId = %x\n",	ClientId.UniqueProcess);

  if (ProcessData->CtrlDispatcher)
    {
      Process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, ProcessData->ProcessId);
      if (NULL == Process)
        {
          DPRINT1("Failed for handle duplication\n");
          return;
        }

      DPRINT("CsrConsoleCtrlEvent Process Handle = %x\n", Process);

      Thread = CreateRemoteThread(Process, NULL, 0,
                                  (LPTHREAD_START_ROUTINE) ProcessData->CtrlDispatcher,
                                  (PVOID) Event, 0, NULL);
      if (NULL == Thread)
        {
          DPRINT1("Failed thread creation\n");
          CloseHandle(Process);
          return;
        }
      CloseHandle(Thread);
      CloseHandle(Process);
    }
}

#define GET_CELL_BUFFER(b,o)\
(b)->Buffer[(o)++];

#define SET_CELL_BUFFER(b,o,c,a)\
(b)->Buffer[(o)++]=(c);\
(b)->Buffer[(o)++]=(a);

static VOID FASTCALL
ClearLineBuffer(PCSRSS_SCREEN_BUFFER Buff)
{
  DWORD Offset = 2 * (Buff->CurrentY * Buff->MaxX);
  UINT Pos;
	
  for (Pos = 0; Pos < Buff->MaxX; Pos++)
    {
      /* Fill the cell: Offset is incremented by the macro */
      SET_CELL_BUFFER(Buff, Offset, ' ', Buff->DefaultAttrib)
    }
}

STATIC NTSTATUS FASTCALL
CsrInitConsoleScreenBuffer(PCSRSS_CONSOLE Console,
                           PCSRSS_SCREEN_BUFFER Buffer)
{
  Buffer->Header.Type = CONIO_SCREEN_BUFFER_MAGIC;
  Buffer->Header.ReferenceCount = 0;
  Buffer->MaxX = Console->Size.X;
  Buffer->MaxY = Console->Size.Y;
  Buffer->ShowX = 0;
  Buffer->ShowY = 0;
  Buffer->Buffer = HeapAlloc(Win32CsrApiHeap, 0, Buffer->MaxX * Buffer->MaxY * 2);
  if (NULL == Buffer->Buffer)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  ConioInitScreenBuffer(Console, Buffer);
  /* initialize buffer to be empty with default attributes */
  for (Buffer->CurrentY = 0 ; Buffer->CurrentY < Buffer->MaxY; Buffer->CurrentY++)
    {
      ClearLineBuffer(Buffer);
    }
  Buffer->CursorInfo.bVisible = TRUE;
  Buffer->CursorInfo.dwSize = 5;
  Buffer->Mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
  Buffer->CurrentX = 0;
  Buffer->CurrentY = 0;

  return STATUS_SUCCESS;
}

STATIC NTSTATUS STDCALL
CsrInitConsole(PCSRSS_CONSOLE Console)
{
  NTSTATUS Status;
  SECURITY_ATTRIBUTES SecurityAttributes;
  PCSRSS_SCREEN_BUFFER NewBuffer;
  BOOL GuiMode;

  Console->Title.MaximumLength = Console->Title.Length = 0;
  Console->Title.Buffer = NULL;
  
  RtlCreateUnicodeString(&Console->Title, L"Command Prompt");
  
  Console->Header.ReferenceCount = 0;
  Console->WaitingChars = 0;
  Console->WaitingLines = 0;
  Console->EchoCount = 0;
  Console->Header.Type = CONIO_CONSOLE_MAGIC;
  Console->Mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
  Console->EarlyReturn = FALSE;
  Console->ActiveBuffer = NULL;
  InitializeListHead(&Console->InputEvents);
  InitializeListHead(&Console->ProcessList);

  SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  SecurityAttributes.lpSecurityDescriptor = NULL;
  SecurityAttributes.bInheritHandle = TRUE;

  Console->ActiveEvent = CreateEventW(&SecurityAttributes, FALSE, FALSE, NULL);
  if (NULL == Console->ActiveEvent)
    {
      return STATUS_UNSUCCESSFUL;
    }
  Console->PrivateData = NULL;
  GuiMode = DtbgIsDesktopVisible();
  if (! GuiMode)
    {
      Status = TuiInitConsole(Console);
      if (! NT_SUCCESS(Status))
        {
          DPRINT1("Failed to open text-mode console, switching to gui-mode\n");
          GuiMode = TRUE;
        }
    }
  if (GuiMode)
    {
      Status = GuiInitConsole(Console);
      if (! NT_SUCCESS(Status))
        {
          CloseHandle(Console->ActiveEvent);
          return Status;
        }
    }

  NewBuffer = HeapAlloc(Win32CsrApiHeap, 0, sizeof(CSRSS_SCREEN_BUFFER));
  if (NULL == NewBuffer)
    {
      CloseHandle(Console->ActiveEvent);
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  Status = CsrInitConsoleScreenBuffer(Console, NewBuffer);
  if (! NT_SUCCESS(Status))
    {
      CloseHandle(Console->ActiveEvent);
      HeapFree(Win32CsrApiHeap, 0, NewBuffer);
      return Status;
    }
  Console->ActiveBuffer = NewBuffer;
  /* add a reference count because the buffer is tied to the console */
  Console->ActiveBuffer->Header.ReferenceCount++;
  /* make console active, and insert into console list */
  /* copy buffer contents to screen */
  ConioDrawConsole(Console);

  return STATUS_SUCCESS;
}


CSR_API(CsrAllocConsole)
{
  PCSRSS_CONSOLE Console;
  HANDLE Process;
  NTSTATUS Status;

  DPRINT("CsrAllocConsole\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  if (ProcessData == NULL)
    {
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }

  if (ProcessData->Console)
    {
      Reply->Status = STATUS_INVALID_PARAMETER;
      return STATUS_INVALID_PARAMETER;
    }

  Reply->Status = STATUS_SUCCESS;
  Console = HeapAlloc(Win32CsrApiHeap, 0, sizeof(CSRSS_CONSOLE));
  if (NULL == Console)
    {
      Reply->Status = STATUS_NO_MEMORY;
      return STATUS_NO_MEMORY;
    }
  Reply->Status = CsrInitConsole(Console);
  if (! NT_SUCCESS(Reply->Status))
    {
      HeapFree(Win32CsrApiHeap, 0, Console);
      return Reply->Status;
    }
  ProcessData->Console = Console;
  Reply->Data.AllocConsoleReply.Console = Console;

  /* add a reference count because the process is tied to the console */
  Console->Header.ReferenceCount++;
  Status = Win32CsrInsertObject(ProcessData, &Reply->Data.AllocConsoleReply.InputHandle, &Console->Header);
  if (! NT_SUCCESS(Status))
    {
      ConioDeleteConsole((Object_t *) Console);
      ProcessData->Console = 0;
      return Reply->Status = Status;
    }
  Status = Win32CsrInsertObject(ProcessData, &Reply->Data.AllocConsoleReply.OutputHandle, &Console->ActiveBuffer->Header);
  if (!NT_SUCCESS(Status))
    {
      Console->Header.ReferenceCount--;
      Win32CsrReleaseObject(ProcessData, Reply->Data.AllocConsoleReply.InputHandle);
      ProcessData->Console = 0;
      return Reply->Status = Status;
    }

  Process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, ProcessData->ProcessId);
  if (NULL == Process)
    {
      DPRINT1("OpenProcess() failed for handle duplication\n");
      Console->Header.ReferenceCount--;
      ProcessData->Console = 0;
      Win32CsrReleaseObject(ProcessData, Reply->Data.AllocConsoleReply.OutputHandle);
      Win32CsrReleaseObject(ProcessData, Reply->Data.AllocConsoleReply.InputHandle);
      Reply->Status = Status;
      return Status;
    }
  if (! DuplicateHandle(GetCurrentProcess(), ProcessData->Console->ActiveEvent,
                        Process, &ProcessData->ConsoleEvent, EVENT_ALL_ACCESS, FALSE, 0))
    {
      DPRINT1("DuplicateHandle() failed: %d\n", GetLastError);
      CloseHandle(Process);
      Console->Header.ReferenceCount--;
      Win32CsrReleaseObject(ProcessData, Reply->Data.AllocConsoleReply.OutputHandle);
      Win32CsrReleaseObject(ProcessData, Reply->Data.AllocConsoleReply.InputHandle);
      ProcessData->Console = 0;
      Reply->Status = Status;
      return Status;
    }
  CloseHandle(Process);
  ProcessData->CtrlDispatcher = Request->Data.AllocConsoleRequest.CtrlDispatcher;
  DPRINT("CSRSS:CtrlDispatcher address: %x\n", ProcessData->CtrlDispatcher);      
  InsertHeadList(&ProcessData->Console->ProcessList, &ProcessData->ProcessEntry);

  return STATUS_SUCCESS;
}

CSR_API(CsrFreeConsole)
{
  PCSRSS_CONSOLE Console;

  DPRINT("CsrFreeConsole\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  if (ProcessData == NULL || ProcessData->Console == NULL)
    {
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }

  Console = ProcessData->Console;
  Console->Header.ReferenceCount--;
  ProcessData->Console = NULL;
  if (0 == Console->Header.ReferenceCount)
    {
      ConioDeleteConsole((Object_t *) Console);
    }
   
  return STATUS_SUCCESS;
}

STATIC VOID FASTCALL
ConioNextLine(PCSRSS_SCREEN_BUFFER Buff, RECT *UpdateRect, UINT *ScrolledLines)
{
  /* slide the viewable screen */
  if (((Buff->CurrentY - Buff->ShowY + Buff->MaxY) % Buff->MaxY) == Buff->MaxY - 1)
    {
      if (++Buff->ShowY == Buff->MaxY)
        {
          Buff->ShowY = 0;
        }
      (*ScrolledLines)++;
    }
  if (++Buff->CurrentY == Buff->MaxY)
    {
      Buff->CurrentY = 0;
    }
  ClearLineBuffer(Buff);
  UpdateRect->left = 0;
  UpdateRect->right = Buff->MaxX - 1;
  if (UpdateRect->top == Buff->CurrentY)
    {
      if (++UpdateRect->top == Buff->MaxY)
        {
          UpdateRect->top = 0;
        }
    }
  UpdateRect->bottom = Buff->CurrentY;
}

STATIC NTSTATUS FASTCALL
ConioWriteConsole(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff,
                  CHAR *Buffer, DWORD Length, BOOL Attrib)
{
  int i;
  DWORD Offset;
  RECT UpdateRect;
  LONG CursorStartX, CursorStartY;
  UINT ScrolledLines;

  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY, &CursorStartX, &CursorStartY);
  UpdateRect.left = Buff->MaxX;
  UpdateRect.top = Buff->CurrentY;
  UpdateRect.right = -1;
  UpdateRect.bottom = Buff->CurrentY;
  ScrolledLines = 0;

  for (i = 0; i < Length; i++)
    {
      switch(Buffer[i])
        {
          /* --- LF --- */
          case '\n':
            Buff->CurrentX = 0;
            ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
            break;

          /* --- BS --- */
          case '\b':
            /* Only handle BS if we're not on the first pos of the first line */
            if (0 != Buff->CurrentX || Buff->ShowY != Buff->CurrentY)
              {
                if (0 == Buff->CurrentX)
                  {
                    /* slide virtual position up */
                    Buff->CurrentX = Buff->MaxX - 1;
                    if (0 == Buff->CurrentY)
                      {
                        Buff->CurrentY = Buff->MaxY;
                      }
                    else
                      {
                        Buff->CurrentY--;
                      }
                    if ((0 == UpdateRect.top && UpdateRect.bottom < Buff->CurrentY)
                        || (0 != UpdateRect.top && Buff->CurrentY < UpdateRect.top))
                      {
                        UpdateRect.top = Buff->CurrentY;
                      }
                  }
                else
                  {
                     Buff->CurrentX--;
                  }
                Offset = 2 * ((Buff->CurrentY * Buff->MaxX) + Buff->CurrentX);
                SET_CELL_BUFFER(Buff, Offset, ' ', Buff->DefaultAttrib);
                UpdateRect.left = RtlRosMin(UpdateRect.left, Buff->CurrentX);
                UpdateRect.right = RtlRosMax(UpdateRect.right, (LONG) Buff->CurrentX);
              }
            break;

          /* --- CR --- */
          case '\r':
            Buff->CurrentX = 0;
            UpdateRect.left = RtlRosMin(UpdateRect.left, Buff->CurrentX);
            UpdateRect.right = RtlRosMax(UpdateRect.right, (LONG) Buff->CurrentX);
            break;

          /* --- TAB --- */
          case '\t':
            {
              UINT EndX;

              UpdateRect.left = RtlRosMin(UpdateRect.left, Buff->CurrentX);
              EndX = 8 * ((Buff->CurrentX + 8) / 8);
              if (Buff->MaxX < EndX)
                {
                  EndX = Buff->MaxX;
                }
              Offset = 2 * (((Buff->CurrentY * Buff->MaxX)) + Buff->CurrentX);
              while (Buff->CurrentX < EndX)
                {
                  Buff->Buffer[Offset] = ' ';
                  Offset += 2;
                  Buff->CurrentX++;
                }
              UpdateRect.right = RtlRosMax(UpdateRect.right, (LONG) Buff->CurrentX - 1);
              if (Buff->CurrentX == Buff->MaxX)
                {
                  Buff->CurrentX = 0;
                  ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
                }
            }
            break;

          /* --- */
          default:
            UpdateRect.left = RtlRosMin(UpdateRect.left, Buff->CurrentX);
            UpdateRect.right = RtlRosMax(UpdateRect.right, (LONG) Buff->CurrentX);
            Offset = 2 * (((Buff->CurrentY * Buff->MaxX)) + Buff->CurrentX);
            Buff->Buffer[Offset++] = Buffer[i];
            if (Attrib)
              {
                Buff->Buffer[Offset] = Buff->DefaultAttrib;
              }
            Buff->CurrentX++;
            if (Buff->CurrentX == Buff->MaxX)
              {
                Buff->CurrentX = 0;
                ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
              }
            break;
        }
    }

  ConioPhysicalToLogical(Buff, UpdateRect.left, UpdateRect.top, &(UpdateRect.left),
                         &(UpdateRect.top));
  ConioPhysicalToLogical(Buff, UpdateRect.right, UpdateRect.bottom, &(UpdateRect.right),
                         &(UpdateRect.bottom));
  if (! ConioIsRectEmpty(&UpdateRect) && NULL != Console && Buff == Console->ActiveBuffer)
    {
      ConioWriteStream(Console, &UpdateRect, CursorStartX, CursorStartY, ScrolledLines,
                       Buffer, Length);
    }

  return STATUS_SUCCESS;
}

CSR_API(CsrReadConsole)
{
  PLIST_ENTRY CurrentEntry;
  ConsoleInput *Input;
  PCHAR Buffer;
  int i;
  ULONG nNumberOfCharsToRead;
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;
   
  DPRINT("CsrReadConsole\n");

  /* truncate length to CSRSS_MAX_READ_CONSOLE_REQUEST */
  nNumberOfCharsToRead = Request->Data.ReadConsoleRequest.NrCharactersToRead > CSRSS_MAX_READ_CONSOLE_REQUEST ? CSRSS_MAX_READ_CONSOLE_REQUEST : Request->Data.ReadConsoleRequest.NrCharactersToRead;
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = Reply->Header.MessageSize - sizeof(LPC_MESSAGE);

  Buffer = Reply->Data.ReadConsoleReply.Buffer;
  Status = ConioLockConsole(ProcessData, Request->Data.ReadConsoleRequest.ConsoleHandle,
                               &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
  Reply->Data.ReadConsoleReply.EventHandle = ProcessData->ConsoleEvent;
  for (i = 0; i < nNumberOfCharsToRead && Console->InputEvents.Flink != &Console->InputEvents; i++)
    {
      /* remove input event from queue */
      CurrentEntry = RemoveHeadList(&Console->InputEvents);
      Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

      /* only pay attention to valid ascii chars, on key down */
      if (KEY_EVENT == Input->InputEvent.EventType
          && Input->InputEvent.Event.KeyEvent.bKeyDown
          && Input->InputEvent.Event.KeyEvent.uChar.AsciiChar)
        {
          /* backspace handling */
          if ('\b' == Input->InputEvent.Event.KeyEvent.uChar.AsciiChar)
            {
              /* echo if it has not already been done, and either we or the client has chars to be deleted */
              if (! Input->Echoed
                  && (0 !=  i || Request->Data.ReadConsoleRequest.nCharsCanBeDeleted))
                {
                  ConioWriteConsole(Console, Console->ActiveBuffer,
                                   &Input->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE);
                }
              if (0 != i)
                {
                  i -= 2;        /* if we already have something to return, just back it up by 2 */
                }
              else
                {            /* otherwise, return STATUS_NOTIFY_CLEANUP to tell client to back up its buffer */
                  Reply->Data.ReadConsoleReply.NrCharactersRead = 0;
                  Reply->Status = STATUS_NOTIFY_CLEANUP;
                  Console->WaitingChars--;
                  HeapFree(Win32CsrApiHeap, 0, Input);
                  ConioUnlockConsole(Console);
                  return STATUS_NOTIFY_CLEANUP;
                }
              Request->Data.ReadConsoleRequest.nCharsCanBeDeleted--;
              Input->Echoed = TRUE;   /* mark as echoed so we don't echo it below */
            }
          /* do not copy backspace to buffer */
          else
            {
              Buffer[i] = Input->InputEvent.Event.KeyEvent.uChar.AsciiChar;
            }
          /* echo to screen if enabled and we did not already echo the char */
          if (0 != (Console->Mode & ENABLE_ECHO_INPUT)
              && ! Input->Echoed
              && '\r' != Input->InputEvent.Event.KeyEvent.uChar.AsciiChar)
            {
              ConioWriteConsole(Console, Console->ActiveBuffer,
                               &Input->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE);
            }
        }
      else
        {
          i--;
        }
      Console->WaitingChars--;
      HeapFree(Win32CsrApiHeap, 0, Input);
    }
  Reply->Data.ReadConsoleReply.NrCharactersRead = i;
  if (0 == i)
    {
      Reply->Status = STATUS_PENDING;    /* we didn't read anything */
    }
  else if (0 != (Console->Mode & ENABLE_LINE_INPUT))
    {
      if (0 == Console->WaitingLines || '\n' != Buffer[i - 1])
        {
          Reply->Status = STATUS_PENDING; /* line buffered, didn't get a complete line */
        }
      else
        {
          Console->WaitingLines--;
          Reply->Status = STATUS_SUCCESS; /* line buffered, did get a complete line */
        }
    }
  else
    {
      Reply->Status = STATUS_SUCCESS;  /* not line buffered, did read something */
    }

  if (Reply->Status == STATUS_PENDING)
    {
      Console->EchoCount = nNumberOfCharsToRead - i;
    }
  else
    {
      Console->EchoCount = 0;             /* if the client is no longer waiting on input, do not echo */
    }
  Reply->Header.MessageSize += i;

  ConioUnlockConsole(Console);
  return Reply->Status;
}

VOID FASTCALL
ConioPhysicalToLogical(PCSRSS_SCREEN_BUFFER Buff,
                       ULONG PhysicalX,
                       ULONG PhysicalY,
                       LONG *LogicalX,
                       LONG *LogicalY)
{
   *LogicalX = PhysicalX;
   if (PhysicalY < Buff->ShowY)
     {
       *LogicalY = Buff->MaxY - Buff->ShowY + PhysicalY;
     }
   else
     {
       *LogicalY = PhysicalY - Buff->ShowY;
     }
}

inline BOOLEAN ConioIsEqualRect(
  RECT *Rect1,
  RECT *Rect2)
{
  return ((Rect1->left == Rect2->left) && (Rect1->right == Rect2->right) &&
	  (Rect1->top == Rect2->top) && (Rect1->bottom == Rect2->bottom));
}

inline BOOLEAN ConioGetIntersection(
  RECT *Intersection,
  RECT *Rect1,
  RECT *Rect2)
{
  if (ConioIsRectEmpty(Rect1) ||
    (ConioIsRectEmpty(Rect2)) ||
    (Rect1->top > Rect2->bottom) ||
    (Rect1->left > Rect2->right) ||
    (Rect1->bottom < Rect2->top) ||
    (Rect1->right < Rect2->left))
  {
    /* The rectangles do not intersect */
    ConioInitRect(Intersection, 0, -1, 0, -1);
    return FALSE;
  }

  ConioInitRect(Intersection,
               RtlRosMax(Rect1->top, Rect2->top),
               RtlRosMax(Rect1->left, Rect2->left),
               RtlRosMin(Rect1->bottom, Rect2->bottom),
               RtlRosMin(Rect1->right, Rect2->right));

  return TRUE;
}

inline BOOLEAN ConioGetUnion(
  RECT *Union,
  RECT *Rect1,
  RECT *Rect2)
{
  if (ConioIsRectEmpty(Rect1))
    {
      if (ConioIsRectEmpty(Rect2))
        {
          ConioInitRect(Union, 0, -1, 0, -1);
          return FALSE;
        }
      else
        {
          *Union = *Rect2;
        }
    }
  else if (ConioIsRectEmpty(Rect2))
    {
      *Union = *Rect1;
    }
  else
    {
      ConioInitRect(Union,
                   RtlRosMin(Rect1->top, Rect2->top),
                   RtlRosMin(Rect1->left, Rect2->left),
                   RtlRosMax(Rect1->bottom, Rect2->bottom),
                   RtlRosMax(Rect1->right, Rect2->right));
    }

  return TRUE;
}

inline BOOLEAN ConioSubtractRect(
  RECT *Subtraction,
  RECT *Rect1,
  RECT *Rect2)
{
  RECT tmp;

  if (ConioIsRectEmpty(Rect1))
    {
      ConioInitRect(Subtraction, 0, -1, 0, -1);
      return FALSE;
    }
  *Subtraction = *Rect1;
  if (ConioGetIntersection(&tmp, Rect1, Rect2))
    {
      if (ConioIsEqualRect(&tmp, Subtraction))
        {
          ConioInitRect(Subtraction, 0, -1, 0, -1);
          return FALSE;
        }
      if ((tmp.top == Subtraction->top) && (tmp.bottom == Subtraction->bottom))
        {
          if (tmp.left == Subtraction->left)
            {
              Subtraction->left = tmp.right;
            }
          else if (tmp.right == Subtraction->right)
            {
              Subtraction->right = tmp.left;
            }
        }
      else if ((tmp.left == Subtraction->left) && (tmp.right == Subtraction->right))
        {
          if (tmp.top == Subtraction->top)
            {
              Subtraction->top = tmp.bottom;
            }
          else if (tmp.bottom == Subtraction->bottom)
            {
              Subtraction->bottom = tmp.top;
            }
        }
    }

  return TRUE;
}

STATIC VOID FASTCALL
ConioCopyRegion(PCSRSS_SCREEN_BUFFER ScreenBuffer,
                RECT *SrcRegion,
                RECT *DstRegion)
{
  SHORT SrcY, DstY;
  DWORD SrcOffset;
  DWORD DstOffset;
  DWORD BytesPerLine;
  ULONG i;

  DstY = DstRegion->top;
  BytesPerLine = ConioRectWidth(DstRegion) * 2;

  SrcY = (SrcRegion->top + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;
  DstY = (DstRegion->top + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;
  SrcOffset = (SrcY * ScreenBuffer->MaxX + SrcRegion->left + ScreenBuffer->ShowX) * 2;
  DstOffset = (DstY * ScreenBuffer->MaxX + DstRegion->left + ScreenBuffer->ShowX) * 2;
  
  for (i = SrcRegion->top; i <= SrcRegion->bottom; i++)
    {
      RtlCopyMemory(
        &ScreenBuffer->Buffer[DstOffset],
        &ScreenBuffer->Buffer[SrcOffset],
        BytesPerLine);

      if (++DstY == ScreenBuffer->MaxY)
        {
          DstY = 0;
          DstOffset = (DstRegion->left + ScreenBuffer->ShowX) * 2;
        }
      else
        {
          DstOffset += ScreenBuffer->MaxX * 2;
        }

      if (++SrcY == ScreenBuffer->MaxY)
        {
          SrcY = 0;
          SrcOffset = (SrcRegion->left + ScreenBuffer->ShowX) * 2;
        }
      else
        {
          SrcOffset += ScreenBuffer->MaxX * 2;
        }
    }
}

STATIC VOID FASTCALL
ConioFillRegion(PCSRSS_SCREEN_BUFFER ScreenBuffer,
                RECT *Region,
                CHAR_INFO CharInfo)
{
  SHORT X, Y;
  DWORD Offset;
  DWORD Delta;
  ULONG i;

  Y = (Region->top + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;
  Offset = (Y * ScreenBuffer->MaxX + Region->left + ScreenBuffer->ShowX) * 2;
  Delta = (ScreenBuffer->MaxX - ConioRectWidth(Region)) * 2;

  for (i = Region->top; i <= Region->bottom; i++)
    {
      for (X = Region->left; X <= Region->right; X++)
        {
          SET_CELL_BUFFER(ScreenBuffer, Offset, CharInfo.Char.AsciiChar, CharInfo.Attributes);
        }
      if (++Y == ScreenBuffer->MaxY)
        {
          Y = 0;
          Offset = (Region->left + ScreenBuffer->ShowX) * 2;
        }
      else
        {
          Offset += Delta;
        }
    }
}

CSR_API(CsrWriteConsole)
{
  NTSTATUS Status;
  BYTE *Buffer = Request->Data.WriteConsoleRequest.Buffer;
  PCSRSS_SCREEN_BUFFER Buff;
  PCSRSS_CONSOLE Console;

  DPRINT("CsrWriteConsole\n");
   
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
                           sizeof(LPC_MESSAGE);

  if (Request->Header.DataSize
      < sizeof(CSRSS_WRITE_CONSOLE_REQUEST) - 1
        + Request->Data.WriteConsoleRequest.NrCharactersToWrite)
    {
      DPRINT1("Invalid request size\n");
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }
  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.WriteConsoleRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  ConioWriteConsole(Console, Buff, Buffer,
                    Request->Data.WriteConsoleRequest.NrCharactersToWrite, TRUE);
  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Reply->Status = STATUS_SUCCESS;
}

VOID STDCALL
ConioDeleteScreenBuffer(Object_t *Object)
{
  PCSRSS_SCREEN_BUFFER Buffer = (PCSRSS_SCREEN_BUFFER) Object;
  HeapFree(Win32CsrApiHeap, 0, Buffer->Buffer);
  HeapFree(Win32CsrApiHeap, 0, Buffer);
}

VOID FASTCALL
ConioDrawConsole(PCSRSS_CONSOLE Console)
{
   RECT Region;

   ConioInitRect(&Region, 0, 0, Console->Size.Y - 1, Console->Size.X - 1);

   ConioDrawRegion(Console, &Region);
}


VOID STDCALL
ConioDeleteConsole(Object_t *Object)
{
  PCSRSS_CONSOLE Console = (PCSRSS_CONSOLE) Object;
  ConsoleInput *Event;

  DPRINT("ConioDeleteConsole\n");

  /* Drain input event queue */
  while (Console->InputEvents.Flink != &Console->InputEvents)
    {
      Event = (ConsoleInput *) Console->InputEvents.Flink;
      Console->InputEvents.Flink = Console->InputEvents.Flink->Flink;
      Console->InputEvents.Flink->Flink->Blink = &Console->InputEvents;
      HeapFree(Win32CsrApiHeap, 0, Event);
    }

  if (0 == --Console->ActiveBuffer->Header.ReferenceCount)
    {
      ConioDeleteScreenBuffer((Object_t *) Console->ActiveBuffer);
    }

  Console->ActiveBuffer = NULL;
  ConioCleanupConsole(Console);

  CloseHandle(Console->ActiveEvent);
  RtlFreeUnicodeString(&Console->Title);
  HeapFree(Win32CsrApiHeap, 0, Console);
}

VOID STDCALL
CsrInitConsoleSupport(VOID)
{
  DPRINT("CSR: CsrInitConsoleSupport()\n");

  /* Should call LoadKeyboardLayout */
}

STATIC VOID FASTCALL
ConioProcessChar(PCSRSS_CONSOLE Console,
                            ConsoleInput *KeyEventRecord)
{
  BOOL updown;
  BOOL bClientWake = FALSE;
  ConsoleInput *TempInput;

  /* process Ctrl-C and Ctrl-Break */
  if (Console->Mode & ENABLE_PROCESSED_INPUT &&
      KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown &&
      ((KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE) || 
       (KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == 'C')) &&
      (KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
    {
      PCSRSS_PROCESS_DATA current;
      PLIST_ENTRY current_entry;
      DPRINT1("Console_Api Ctrl-C\n");
      current_entry = Console->ProcessList.Flink;
      while (current_entry != &Console->ProcessList)
	{
	  current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
	  current_entry = current_entry->Flink;
	  CsrConsoleCtrlEvent((DWORD)CTRL_C_EVENT, current);
	}
      HeapFree(Win32CsrApiHeap, 0, KeyEventRecord);
      return;
    }

  if (0 != (KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState
            & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
      && (VK_UP == KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode
          || VK_DOWN == KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode))
    {
      if (KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown)
	{
	  /* scroll up or down */
	  if (NULL == Console)
	    {
	      DPRINT1("No Active Console!\n");
	      HeapFree(Win32CsrApiHeap, 0, KeyEventRecord);
	      return;
	    }
	  if (VK_UP == KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode)
	    {
	      /* only scroll up if there is room to scroll up into */
	      if (Console->ActiveBuffer->ShowY != ((Console->ActiveBuffer->CurrentY + 1) %
                                                   Console->ActiveBuffer->MaxY))
                {
                  Console->ActiveBuffer->ShowY = (Console->ActiveBuffer->ShowY +
                                                  Console->ActiveBuffer->MaxY - 1) %
                                                 Console->ActiveBuffer->MaxY;
                }
            }
	  else if (Console->ActiveBuffer->ShowY != Console->ActiveBuffer->CurrentY)
	    /* only scroll down if there is room to scroll down into */
            {
	      if (Console->ActiveBuffer->ShowY % Console->ActiveBuffer->MaxY != 
		  Console->ActiveBuffer->CurrentY)
                {
                  if (((Console->ActiveBuffer->CurrentY + 1) % Console->ActiveBuffer->MaxY) != 
                      (Console->ActiveBuffer->ShowY + Console->ActiveBuffer->MaxY) %
                      Console->ActiveBuffer->MaxY)
                    {
                      Console->ActiveBuffer->ShowY = (Console->ActiveBuffer->ShowY + 1) %
                                                     Console->ActiveBuffer->MaxY;
                    }
                }
            }
          ConioDrawConsole(Console);
        }
      HeapFree(Win32CsrApiHeap, 0, KeyEventRecord);
      return;
    }
  if (NULL == Console)
    {
      DPRINT1("No Active Console!\n");
      HeapFree(Win32CsrApiHeap, 0, KeyEventRecord);
      return;
    }

  if (0 != (Console->Mode & (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT)))
    {
      switch(KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar)
        {
          case '\r':
            /* first add the \r */
            KeyEventRecord->InputEvent.EventType = KEY_EVENT;
            updown = KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown;
            KeyEventRecord->Echoed = FALSE;
            KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
            KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar = '\r';
            InsertTailList(&Console->InputEvents, &KeyEventRecord->ListEntry);
            Console->WaitingChars++;
            KeyEventRecord = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));
            if (NULL == KeyEventRecord)
              {
                DPRINT1("Failed to allocate KeyEventRecord\n");
                return;
              }
            KeyEventRecord->InputEvent.EventType = KEY_EVENT;
            KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown = updown;
            KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode = 0;
            KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualScanCode = 0;
            KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar = '\n';
            KeyEventRecord->Fake = TRUE;
            break;
        }
    }
  /* add event to the queue */
  InsertTailList(&Console->InputEvents, &KeyEventRecord->ListEntry);
  Console->WaitingChars++;
  /* if line input mode is enabled, only wake the client on enter key down */
  if (0 == (Console->Mode & ENABLE_LINE_INPUT)
      || Console->EarlyReturn
      || ('\n' == KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar
          && ! KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown))
    {
      if ('\n' == KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar)
        {
          Console->WaitingLines++;
        }
      bClientWake = TRUE;
      SetEvent(Console->ActiveEvent);
    }
  KeyEventRecord->Echoed = FALSE;
  if (0 != (Console->Mode & (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT))
      && '\b' == KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar
      && KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown)
    {
      /* walk the input queue looking for a char to backspace */
      for (TempInput = (ConsoleInput *) Console->InputEvents.Blink;
	   TempInput != (ConsoleInput *) &Console->InputEvents
           && (KEY_EVENT == TempInput->InputEvent.EventType
               || ! TempInput->InputEvent.Event.KeyEvent.bKeyDown
               || '\b' == TempInput->InputEvent.Event.KeyEvent.uChar.AsciiChar);
	   TempInput = (ConsoleInput *) TempInput->ListEntry.Blink)
        {
          /* NOP */;
        }
      /* if we found one, delete it, otherwise, wake the client */
      if (TempInput != (ConsoleInput *) &Console->InputEvents)
	{
	  /* delete previous key in queue, maybe echo backspace to screen, and do not place backspace on queue */
	  RemoveEntryList(&TempInput->ListEntry);
	  if (TempInput->Echoed)
            {
              ConioWriteConsole(Console, Console->ActiveBuffer,
                               &KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar,
                               1, TRUE);	
            }
          HeapFree(Win32CsrApiHeap, 0, TempInput);
	  RemoveEntryList(&KeyEventRecord->ListEntry);
	  HeapFree(Win32CsrApiHeap, 0, KeyEventRecord);
	  Console->WaitingChars -= 2;
	}
      else
        {
          SetEvent(Console->ActiveEvent);
        }
    }
  else
    {
      /* echo chars if we are supposed to and client is waiting for some */
      if (0 != (Console->Mode & ENABLE_ECHO_INPUT) && Console->EchoCount
          && KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar
          && KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown
          && '\r' != KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar)
        {
          /* mark the char as already echoed */
          ConioWriteConsole(Console, Console->ActiveBuffer,
                           &KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar,
                           1, TRUE);
          Console->EchoCount--;
          KeyEventRecord->Echoed = TRUE;
        }
    }

  /* Console->WaitingChars++; */
  if (bClientWake || 0 == (Console->Mode & ENABLE_LINE_INPUT))
    {
      SetEvent(Console->ActiveEvent);
    }
}

STATIC DWORD FASTCALL
ConioGetShiftState(PBYTE KeyState)
{
  int i;
  DWORD ssOut = 0;

  for (i = 0; i < 0x100; i++)
    {
      if (0 != (KeyState[i] & 0x80))
        {
          UINT vk = MapVirtualKeyExW(i, 3, 0) & 0xff;
          switch(vk)
            {
              case VK_CAPITAL:
                ssOut |= CAPSLOCK_ON;
                break;

              case VK_NUMLOCK:
                ssOut |= NUMLOCK_ON;
                break;

              case VK_SCROLL:
                ssOut |= SCROLLLOCK_ON;
                break;
            }
        }
    }

  if (KeyState[VK_LSHIFT] & 0x80 || KeyState[VK_RSHIFT] & 0x80)
      ssOut |= SHIFT_PRESSED;

  if (KeyState[VK_LCONTROL] & 0x80)
      ssOut |= RIGHT_CTRL_PRESSED;
  else if (KeyState[VK_RCONTROL] & 0x80)
      ssOut |= LEFT_CTRL_PRESSED;

  if (KeyState[VK_LMENU] & 0x80)
      ssOut |= RIGHT_ALT_PRESSED;
  else if (KeyState[VK_RMENU] & 0x80)
      ssOut |= LEFT_ALT_PRESSED;

  return ssOut;
}

VOID STDCALL
ConioProcessKey(MSG *msg, PCSRSS_CONSOLE Console, BOOL TextMode)
{
  static BYTE KeyState[256] = { 0 };
  /* MSDN mentions that you should use the last virtual key code received
   * when putting a virtual key identity to a WM_CHAR message since multiple
   * or translated keys may be involved. */
  static UINT LastVirtualKey = 0;
  DWORD ShiftState;
  ConsoleInput *ConInRec;
  UINT RepeatCount;
  CHAR AsciiChar;
  WCHAR UnicodeChar;
  UINT VirtualKeyCode;
  UINT VirtualScanCode;
  BOOL Down = FALSE;
  INPUT_RECORD er;
  ULONG ResultSize = 0;

  RepeatCount = 1;
  VirtualScanCode = (msg->lParam >> 16) & 0xff;
  Down = msg->message == WM_KEYDOWN || msg->message == WM_CHAR || 
    msg->message == WM_SYSKEYDOWN || msg->message == WM_SYSCHAR;

  GetKeyboardState(KeyState);
  ShiftState = ConioGetShiftState(KeyState);

  if (msg->message == WM_CHAR || msg->message == WM_SYSCHAR)
    {
      VirtualKeyCode = LastVirtualKey;
      UnicodeChar = msg->wParam;
    } 
  else
    { 
      WCHAR Chars[2];
      INT RetChars = 0;

      VirtualKeyCode = msg->wParam;
      RetChars = ToUnicodeEx(VirtualKeyCode,
                             VirtualScanCode,
                             KeyState,
                             Chars,
                             2,
                             0,
                             0);
      UnicodeChar = (1 == RetChars ? Chars[0] : 0);
    }

  if (UnicodeChar)
    {
      RtlUnicodeToOemN(&AsciiChar,
                       1,
                       &ResultSize,
                       &UnicodeChar,
                       sizeof(WCHAR));
    }
  if (0 == ResultSize)
    {
      AsciiChar = 0;
    }
  
  er.EventType = KEY_EVENT;
  er.Event.KeyEvent.bKeyDown = Down;
  er.Event.KeyEvent.wRepeatCount = RepeatCount;
  er.Event.KeyEvent.uChar.UnicodeChar = AsciiChar & 0xff;
  er.Event.KeyEvent.dwControlKeyState = ShiftState;
  er.Event.KeyEvent.wVirtualKeyCode = VirtualKeyCode;
  er.Event.KeyEvent.wVirtualScanCode = VirtualScanCode;

  if (TextMode)
    {    
      if (0 != (ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
          && VK_TAB == VirtualKeyCode)
        {
          if (Down)
            {
              TuiSwapConsole(ShiftState & SHIFT_PRESSED ? -1 : 1);
            }

          return;
        }
      else if (VK_MENU == VirtualKeyCode && ! Down)
        {
          if (TuiSwapConsole(0))
            {
              return;
            }
        }
    }

  if (NULL == Console) 
    {
      return;
    }

  ConInRec = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));

  if (NULL == ConInRec)
    {
      return;
    }
    
  ConInRec->InputEvent = er;
  ConInRec->Fake = AsciiChar && 
    (msg->message != WM_CHAR && msg->message != WM_SYSCHAR &&
     msg->message != WM_KEYUP && msg->message != WM_SYSKEYUP);
  ConInRec->NotChar = (msg->message != WM_CHAR && msg->message != WM_SYSCHAR);
  ConInRec->Echoed = FALSE;
  if (ConInRec->NotChar)
    LastVirtualKey = msg->wParam;

  DPRINT  ("csrss: %s %s %s %s %02x %02x '%c' %04x\n",
	   Down ? "down" : "up  ",
	   (msg->message == WM_CHAR || msg->message == WM_SYSCHAR) ?
	   "char" : "key ",
	   ConInRec->Fake ? "fake" : "real",
	   ConInRec->NotChar ? "notc" : "char",
	   VirtualScanCode,
	   VirtualKeyCode,
	   (AsciiChar >= ' ') ? AsciiChar : '.',
	   ShiftState);
    
  if (! ConInRec->Fake || ! ConInRec->NotChar)
    {
      ConioProcessChar(Console, ConInRec);
    }
  else
    {
      HeapFree(Win32CsrApiHeap, 0, ConInRec);
    }
}

VOID
Console_Api(DWORD RefreshEvent)
{
  /* keep reading events from the keyboard and stuffing them into the current
     console's input queue */
  MSG msg;

  /* This call establishes our message queue */
  PeekMessageW(&msg, 0, 0, 0, PM_NOREMOVE);
  /* This call registers our message queue */
  PrivateCsrssRegisterPrimitive();
  /* This call turns on the input system in win32k */
  PrivateCsrssAcquireOrReleaseInputOwnership(FALSE);
  
  while (TRUE)
    {
      GetMessageW(&msg, 0, 0, 0);
      TranslateMessage(&msg);

      if (msg.message == WM_CHAR || msg.message == WM_SYSCHAR ||
          msg.message == WM_KEYDOWN || msg.message == WM_KEYUP ||
          msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP)
        {
          ConioProcessKey(&msg, TuiGetFocusConsole(), TRUE);
        }
    }

  PrivateCsrssAcquireOrReleaseInputOwnership(TRUE);
}

CSR_API(CsrGetScreenBufferInfo)
{
  NTSTATUS Status;
  PCSRSS_SCREEN_BUFFER Buff;
  PCONSOLE_SCREEN_BUFFER_INFO pInfo;
   
  DPRINT("CsrGetScreenBufferInfo\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ScreenBufferInfoRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
  pInfo = &Reply->Data.ScreenBufferInfoReply.Info;
  pInfo->dwSize.X = Buff->MaxX;
  pInfo->dwSize.Y = Buff->MaxY;
  pInfo->dwCursorPosition.X = Buff->CurrentX - Buff->ShowX;
  pInfo->dwCursorPosition.Y = (Buff->CurrentY + Buff->MaxY - Buff->ShowY) % Buff->MaxY;
  pInfo->wAttributes = Buff->DefaultAttrib;
  pInfo->srWindow.Left = 0;
  pInfo->srWindow.Right = Buff->MaxX - 1;
  pInfo->srWindow.Top = 0;
  pInfo->srWindow.Bottom = Buff->MaxY - 1;
  pInfo->dwMaximumWindowSize.X = Buff->MaxX;
  pInfo->dwMaximumWindowSize.Y = Buff->MaxY;
  ConioUnlockScreenBuffer(Buff);

  Reply->Status = STATUS_SUCCESS;

  return Reply->Status;
}

CSR_API(CsrSetCursor)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  LONG OldCursorX, OldCursorY;
   
  DPRINT("CsrSetCursor\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY, &OldCursorX, &OldCursorY);
  Buff->CurrentX = Request->Data.SetCursorRequest.Position.X + Buff->ShowX;
  Buff->CurrentY = (Request->Data.SetCursorRequest.Position.Y + Buff->ShowY) % Buff->MaxY;
  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      if (! ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY))
        {
          ConioUnlockScreenBuffer(Buff);
          return Reply->Status = STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Reply->Status = STATUS_SUCCESS;
}

STATIC FASTCALL VOID
ConioComputeUpdateRect(PCSRSS_SCREEN_BUFFER Buff, RECT *UpdateRect, COORD *Start, UINT Length)
{
  if (Buff->MaxX <= Start->X + Length)
    {
      UpdateRect->left = 0;
    }
  else
    {
      UpdateRect->left = Start->X;
    }
  if (Buff->MaxX <= Start->X + Length)
    {
      UpdateRect->right = Buff->MaxX - 1;
    }
  else
    {
      UpdateRect->right = Start->X + Length - 1;
    }
  UpdateRect->top = Start->Y;
  UpdateRect->bottom = Start->Y+ (Start->X + Length - 1) / Buff->MaxX;
  if (Buff->MaxY <= UpdateRect->bottom)
    {
      UpdateRect->bottom = Buff->MaxY - 1;
    }
}

CSR_API(CsrWriteConsoleOutputChar)
{
  NTSTATUS Status;
  PBYTE String = Request->Data.WriteConsoleOutputCharRequest.String;
  PBYTE Buffer;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD X, Y, Length;
  RECT UpdateRect;

  DPRINT("CsrWriteConsoleOutputChar\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
                           sizeof(LPC_MESSAGE);

  if (Request->Header.DataSize
      < sizeof(CSRSS_WRITE_CONSOLE_OUTPUT_CHAR_REQUEST) - 1
        + Request->Data.WriteConsoleOutputCharRequest.Length)
    {
      DPRINT1("Invalid request size\n");
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData,
                                 Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle,
                                 &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  X = Request->Data.WriteConsoleOutputCharRequest.Coord.X + Buff->ShowX;
  Y = (Request->Data.WriteConsoleOutputCharRequest.Coord.Y + Buff->ShowY) % Buff->MaxY;
  Length = Request->Data.WriteConsoleOutputCharRequest.Length;
  Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X)];
  while (Length--)
    {
      *Buffer = *String++;
      Buffer += 2;
      if (++X == Buff->MaxX)
        {
          if (++Y == Buff->MaxY)
            {
              Y = 0;
              Buffer = Buff->Buffer;
            }
          X = 0;
        }
    }

  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.WriteConsoleOutputCharRequest.Coord,
                             Request->Data.WriteConsoleOutputCharRequest.Length);
      ConioDrawRegion(Console, &UpdateRect);
    }

  Reply->Data.WriteConsoleOutputCharReply.EndCoord.X = X - Buff->ShowX;
  Reply->Data.WriteConsoleOutputCharReply.EndCoord.Y = (Y + Buff->MaxY - Buff->ShowY) % Buff->MaxY;

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrFillOutputChar)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD X, Y, Length;
  BYTE Char;
  PBYTE Buffer;
  RECT UpdateRect;

  DPRINT("CsrFillOutputChar\n");
   
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.FillOutputRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  X = Request->Data.FillOutputRequest.Position.X + Buff->ShowX;
  Y = (Request->Data.FillOutputRequest.Position.Y + Buff->ShowY) % Buff->MaxY;
  Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X)];
  Char = Request->Data.FillOutputRequest.Char;
  Length = Request->Data.FillOutputRequest.Length;
  while (Length--)
    {
      *Buffer = Char;
      Buffer += 2;
      if (++X == Buff->MaxX)
        {
          if (++Y == Buff->MaxY)
            {
              Y = 0;
              Buffer = Buff->Buffer;
            }
          X = 0;
        }
    }

  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.FillOutputRequest.Position,
                             Request->Data.FillOutputRequest.Length);
      ConioDrawRegion(Console, &UpdateRect);
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Reply->Status;
}

CSR_API(CsrReadInputEvent)
{
  PLIST_ENTRY CurrentEntry;
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;
  BOOLEAN Done = FALSE;
  ConsoleInput *Input;
   
  DPRINT("CsrReadInputEvent\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  Reply->Data.ReadInputReply.Event = ProcessData->ConsoleEvent;
   
  Status = ConioLockConsole(ProcessData, Request->Data.ReadInputRequest.ConsoleHandle, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  /* only get input if there is any */
  while (Console->InputEvents.Flink != &Console->InputEvents && ! Done)
    {
      CurrentEntry = RemoveHeadList(&Console->InputEvents);
      Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
      Done = !Input->Fake;
      Reply->Data.ReadInputReply.Input = Input->InputEvent;

      if (Input->InputEvent.EventType == KEY_EVENT)
        {
          if (0 != (Console->Mode & ENABLE_LINE_INPUT)
	      && Input->InputEvent.Event.KeyEvent.bKeyDown
	      && '\r' == Input->InputEvent.Event.KeyEvent.uChar.AsciiChar)
            {
              Console->WaitingLines--;
            }
          Console->WaitingChars--;
        }
      HeapFree(Win32CsrApiHeap, 0, Input);

      Reply->Data.ReadInputReply.MoreEvents = (Console->InputEvents.Flink != &Console->InputEvents);
      Status = STATUS_SUCCESS;
      Console->EarlyReturn = FALSE; /* clear early return */
    }
   
  if (! Done)
    {
      Status = STATUS_PENDING;
      Console->EarlyReturn = TRUE;  /* mark for early return */
    }

  ConioUnlockConsole(Console);

  return Reply->Status = Status;
}

CSR_API(CsrWriteConsoleOutputAttrib)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  PUCHAR Buffer, Attribute;
  int X, Y, Length;
  NTSTATUS Status;
  RECT UpdateRect;

  DPRINT("CsrWriteConsoleOutputAttrib\n");
   
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
                           sizeof(LPC_MESSAGE);

  if (Request->Header.DataSize
      < sizeof(CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB_REQUEST) - 1
        + Request->Data.WriteConsoleOutputAttribRequest.Length)
    {
      DPRINT1("Invalid request size\n");
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData,
                                 Request->Data.WriteConsoleOutputAttribRequest.ConsoleHandle,
                                 &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  X = Request->Data.WriteConsoleOutputAttribRequest.Coord.X + Buff->ShowX;
  Y = (Request->Data.WriteConsoleOutputAttribRequest.Coord.Y + Buff->ShowY) % Buff->MaxY;
  Length = Request->Data.WriteConsoleOutputAttribRequest.Length;
  Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X) + 1];
  Attribute = Request->Data.WriteConsoleOutputAttribRequest.String;
  while (Length--)
    {
      *Buffer = *Attribute++;
      Buffer += 2;
      if (++X == Buff->MaxX)
        {
          if (++Y == Buff->MaxY)
            {
              Y = 0;
              Buffer = Buff->Buffer + 1;
            }
          X = 0;
        }
    }

  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.WriteConsoleOutputAttribRequest.Coord,
                             Request->Data.WriteConsoleOutputAttribRequest.Length);
      ConioDrawRegion(Console, &UpdateRect);
    }

  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  Reply->Data.WriteConsoleOutputAttribReply.EndCoord.X = Buff->CurrentX - Buff->ShowX;
  Reply->Data.WriteConsoleOutputAttribReply.EndCoord.Y = (Buff->CurrentY + Buff->MaxY - Buff->ShowY) % Buff->MaxY;

  ConioUnlockScreenBuffer(Buff);

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrFillOutputAttrib)
{
  PCSRSS_SCREEN_BUFFER Buff;
  PCHAR Buffer;
  NTSTATUS Status;
  int X, Y, Length;
  UCHAR Attr;
  RECT UpdateRect;
  PCSRSS_CONSOLE Console;

  DPRINT("CsrFillOutputAttrib\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
   
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData, Request->Data.FillOutputAttribRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  X = Request->Data.FillOutputAttribRequest.Coord.X + Buff->ShowX;
  Y = (Request->Data.FillOutputAttribRequest.Coord.Y + Buff->ShowY) % Buff->MaxY;
  Length = Request->Data.FillOutputAttribRequest.Length;
  Attr = Request->Data.FillOutputAttribRequest.Attribute;
  Buffer = &Buff->Buffer[(Y * Buff->MaxX * 2) + (X * 2) + 1];
  while (Length--)
    {
      *Buffer = Attr;
      Buffer += 2;
      if (++X == Buff->MaxX)
        {
          if (++Y == Buff->MaxY)
            {
              Y = 0;
              Buffer = Buff->Buffer + 1;
            }
          X = 0;
        }
    }

  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.FillOutputAttribRequest.Coord,
                             Request->Data.FillOutputAttribRequest.Length);
      ConioDrawRegion(Console, &UpdateRect);
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Reply->Status = STATUS_SUCCESS;
}


CSR_API(CsrGetCursorInfo)
{
  PCSRSS_SCREEN_BUFFER Buff;
  NTSTATUS Status;
   
  DPRINT("CsrGetCursorInfo\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.GetCursorInfoRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
  Reply->Data.GetCursorInfoReply.Info = Buff->CursorInfo;
  ConioUnlockScreenBuffer(Buff);

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetCursorInfo)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD Size;
  BOOL Visible;
  NTSTATUS Status;
   
  DPRINT("CsrSetCursorInfo\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorInfoRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  Size = Request->Data.SetCursorInfoRequest.Info.dwSize;
  Visible = Request->Data.SetCursorInfoRequest.Info.bVisible;
  if (Size < 1)
    {
      Size = 1;
    }
  if (100 < Size)
    {
      Size = 100;
    }

  if (Size != Buff->CursorInfo.dwSize
      || (Visible && ! Buff->CursorInfo.bVisible) || (! Visible && Buff->CursorInfo.bVisible))
    {
      Buff->CursorInfo.dwSize = Size;
      Buff->CursorInfo.bVisible = Visible;

      if (NULL != Console && ! ConioSetCursorInfo(Console, Buff))
        {
          ConioUnlockScreenBuffer(Buff);
          ConioUnlockConsole(Console);
          return Reply->Status = STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetTextAttrib)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  LONG OldCursorX, OldCursorY;   

  DPRINT("CsrSetTextAttrib\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY, &OldCursorX, &OldCursorY);

  Buff->DefaultAttrib = Request->Data.SetAttribRequest.Attrib;
  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      if (! ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY))
        {
          ConioUnlockScreenBuffer(Buff);
          ConioUnlockConsole(Console);
          return Reply->Status = STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleMode)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;

  DPRINT("CsrSetConsoleMode\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  Status = Win32CsrGetObject(ProcessData,
                             Request->Data.SetConsoleModeRequest.ConsoleHandle,
                             (Object_t **) &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Buff = (PCSRSS_SCREEN_BUFFER)Console;
  if (CONIO_CONSOLE_MAGIC == Console->Header.Type)
    {
      Console->Mode = Request->Data.SetConsoleModeRequest.Mode & CONSOLE_INPUT_MODE_VALID;
    }
  else if (CONIO_SCREEN_BUFFER_MAGIC == Console->Header.Type)
    {
      Buff->Mode = Request->Data.SetConsoleModeRequest.Mode & CONSOLE_OUTPUT_MODE_VALID;
    }
  else
    {
      return Reply->Status = STATUS_INVALID_HANDLE;
    }

  Reply->Status = STATUS_SUCCESS;

  return Reply->Status;
}

CSR_API(CsrGetConsoleMode)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;   /* gee, I really wish I could use an anonymous union here */

  DPRINT("CsrGetConsoleMode\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  Status = Win32CsrGetObject(ProcessData, Request->Data.GetConsoleModeRequest.ConsoleHandle,
                             (Object_t **) &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
  Reply->Status = STATUS_SUCCESS;
  Buff = (PCSRSS_SCREEN_BUFFER) Console;
  if (CONIO_CONSOLE_MAGIC == Console->Header.Type)
    {
      Reply->Data.GetConsoleModeReply.ConsoleMode = Console->Mode;
    }
  else if (CONIO_SCREEN_BUFFER_MAGIC == Buff->Header.Type)
    {
      Reply->Data.GetConsoleModeReply.ConsoleMode = Buff->Mode;
    }
  else
    {
      Reply->Status = STATUS_INVALID_HANDLE;
    }

  return Reply->Status;
}

CSR_API(CsrCreateScreenBuffer)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  NTSTATUS Status;
   
  DPRINT("CsrCreateScreenBuffer\n");

  if (ProcessData == NULL)
    {
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
  if (NULL == Console)
    {
      return Reply->Status = STATUS_INVALID_HANDLE;
    }

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  Buff = HeapAlloc(Win32CsrApiHeap, 0, sizeof(CSRSS_SCREEN_BUFFER));
  if (NULL == Buff)
    {
      Reply->Status = STATUS_INSUFFICIENT_RESOURCES;
    }

  Status = CsrInitConsoleScreenBuffer(Console, Buff);
  if(! NT_SUCCESS(Status))
    {
      Reply->Status = Status;
    }
  else
    {
      Reply->Status = Win32CsrInsertObject(ProcessData, &Reply->Data.CreateScreenBufferReply.OutputHandle, &Buff->Header);
    }

  ConioUnlockConsole(Console);

  return Reply->Status;
}

CSR_API(CsrSetScreenBuffer)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;

  DPRINT("CsrSetScreenBuffer\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
  if (NULL == Console)
    {
      DPRINT1("Trying to set screen buffer for app without console\n");
      return Reply->Status = STATUS_INVALID_HANDLE;
    }

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetScreenBufferRequest.OutputHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Reply->Status;
    }

  if (Buff == Console->ActiveBuffer)
    {
      ConioUnlockScreenBuffer(Buff);
      ConioUnlockConsole(Console);
      return STATUS_SUCCESS;
    }

  /* drop reference to old buffer, maybe delete */
  if (! InterlockedDecrement(&Console->ActiveBuffer->Header.ReferenceCount))
    {
      ConioDeleteScreenBuffer((Object_t *) Console->ActiveBuffer);
    }
  /* tie console to new buffer */
  Console->ActiveBuffer = Buff;
  /* inc ref count on new buffer */
  InterlockedIncrement(&Buff->Header.ReferenceCount);
  /* Redraw the console */
  ConioDrawConsole(Console);

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetTitle)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;

  DPRINT("CsrSetTitle\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);

  if (Request->Header.DataSize
      < sizeof(CSRSS_SET_TITLE_REQUEST) - 1
        + Request->Data.SetTitleRequest.Length)
    {
      DPRINT1("Invalid request size\n");
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }

  Status = ConioLockConsole(ProcessData, Request->Data.SetTitleRequest.Console, &Console);
  if(! NT_SUCCESS(Status))
    {
      Reply->Status = Status;  
    }
  else
    {
      /* copy title to console */
      RtlFreeUnicodeString(&Console->Title);
      RtlCreateUnicodeString(&Console->Title, Request->Data.SetTitleRequest.Title);
      if (! ConioChangeTitle(Console))
        {
          Reply->Status = STATUS_UNSUCCESSFUL;
        }
      else
        {
          Reply->Status = STATUS_SUCCESS;
        }
    }
  ConioUnlockConsole(Console);

  return Reply->Status;
}

CSR_API(CsrGetTitle)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  
  DPRINT("CsrGetTitle\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  Status = ConioLockConsole(ProcessData,
                            Request->Data.GetTitleRequest.ConsoleHandle,
                            &Console);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Can't get console\n");
      return Reply->Status = Status;
    }
		
  /* Copy title of the console to the user title buffer */
  RtlZeroMemory(&Reply->Data.GetTitleReply, sizeof(CSRSS_GET_TITLE_REPLY));
  Reply->Data.GetTitleReply.ConsoleHandle = Request->Data.GetTitleRequest.ConsoleHandle;
  Reply->Data.GetTitleReply.Length = Console->Title.Length;
  wcscpy (Reply->Data.GetTitleReply.Title, Console->Title.Buffer);
  Reply->Header.MessageSize += Console->Title.Length;
  Reply->Header.DataSize += Console->Title.Length;
  Reply->Status = STATUS_SUCCESS;

  ConioUnlockConsole(Console);

  return Reply->Status;
}

CSR_API(CsrWriteConsoleOutput)
{
  SHORT i, X, Y, SizeX, SizeY;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  RECT ScreenBuffer;
  CHAR_INFO* CurCharInfo;
  RECT WriteRegion;
  CHAR_INFO* CharInfo;
  COORD BufferCoord;
  COORD BufferSize;
  NTSTATUS Status;
  DWORD Offset;
  DWORD PSize;

  DPRINT("CsrWriteConsoleOutput\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData,
                                 Request->Data.WriteConsoleOutputRequest.ConsoleHandle,
                                 &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  BufferSize = Request->Data.WriteConsoleOutputRequest.BufferSize;
  PSize = BufferSize.X * BufferSize.Y * sizeof(CHAR_INFO);
  BufferCoord = Request->Data.WriteConsoleOutputRequest.BufferCoord;
  CharInfo = Request->Data.WriteConsoleOutputRequest.CharInfo;
  if (((PVOID)CharInfo < ProcessData->CsrSectionViewBase) ||
      (((PVOID)CharInfo + PSize) > 
       (ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockScreenBuffer(Buff);
      ConioUnlockConsole(Console);
      return Reply->Status = STATUS_ACCESS_VIOLATION;
    }
  WriteRegion.left = Request->Data.WriteConsoleOutputRequest.WriteRegion.Left;
  WriteRegion.top = Request->Data.WriteConsoleOutputRequest.WriteRegion.Top;
  WriteRegion.right = Request->Data.WriteConsoleOutputRequest.WriteRegion.Right;
  WriteRegion.bottom = Request->Data.WriteConsoleOutputRequest.WriteRegion.Bottom;

  SizeY = RtlRosMin(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&WriteRegion));
  SizeX = RtlRosMin(BufferSize.X - BufferCoord.X, ConioRectWidth(&WriteRegion));
  WriteRegion.bottom = WriteRegion.top + SizeY - 1;
  WriteRegion.right = WriteRegion.left + SizeX - 1;

  /* Make sure WriteRegion is inside the screen buffer */
  ConioInitRect(&ScreenBuffer, 0, 0, Buff->MaxY - 1, Buff->MaxX - 1);
  if (! ConioGetIntersection(&WriteRegion, &ScreenBuffer, &WriteRegion))
    {
      ConioUnlockScreenBuffer(Buff);
      ConioUnlockConsole(Console);

      /* It is okay to have a WriteRegion completely outside the screen buffer.
         No data is written then. */
      return Reply->Status = STATUS_SUCCESS;
    }

  for (i = 0, Y = WriteRegion.top; Y <= WriteRegion.bottom; i++, Y++)
    {
      CurCharInfo = CharInfo + (i + BufferCoord.Y) * BufferSize.X + BufferCoord.X;
      Offset = (((Y + Buff->ShowY) % Buff->MaxY) * Buff->MaxX + WriteRegion.left) * 2;
      for (X = WriteRegion.left; X <= WriteRegion.right; X++)
        {
          SET_CELL_BUFFER(Buff, Offset, CurCharInfo->Char.AsciiChar, CurCharInfo->Attributes);
          CurCharInfo++;
        }
    }

  if (NULL != Console)
    {
      ConioDrawRegion(Console, &WriteRegion);
    }

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  Reply->Data.WriteConsoleOutputReply.WriteRegion.Right = WriteRegion.left + SizeX - 1;
  Reply->Data.WriteConsoleOutputReply.WriteRegion.Bottom = WriteRegion.top + SizeY - 1;
  Reply->Data.WriteConsoleOutputReply.WriteRegion.Left = WriteRegion.left;
  Reply->Data.WriteConsoleOutputReply.WriteRegion.Top = WriteRegion.top;

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrFlushInputBuffer)
{
  PLIST_ENTRY CurrentEntry;
  PCSRSS_CONSOLE Console;
  ConsoleInput* Input;
  NTSTATUS Status;

  DPRINT("CsrFlushInputBuffer\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  Status = ConioLockConsole(ProcessData,
                            Request->Data.FlushInputBufferRequest.ConsoleInput,
                            &Console);
  if(! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  /* Discard all entries in the input event queue */
  while (!IsListEmpty(&Console->InputEvents))
    {
      CurrentEntry = RemoveHeadList(&Console->InputEvents);
      Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
      /* Destroy the event */
      HeapFree(Win32CsrApiHeap, 0, Input);
    }
  Console->WaitingChars=0;

  ConioUnlockConsole(Console);

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrScrollConsoleScreenBuffer)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  RECT ScreenBuffer;
  RECT SrcRegion;
  RECT DstRegion;
  RECT FillRegion;
  RECT ScrollRectangle;
  RECT ClipRectangle;
  NTSTATUS Status;
  BOOLEAN DoFill;

  DPRINT("CsrScrollConsoleScreenBuffer\n");

  ALIAS(ConsoleHandle,Request->Data.ScrollConsoleScreenBufferRequest.ConsoleHandle);
  ALIAS(UseClipRectangle,Request->Data.ScrollConsoleScreenBufferRequest.UseClipRectangle);
  ALIAS(DestinationOrigin,Request->Data.ScrollConsoleScreenBufferRequest.DestinationOrigin);
  ALIAS(Fill,Request->Data.ScrollConsoleScreenBufferRequest.Fill);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData, ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Reply->Status = Status;
    }

  ScrollRectangle.left = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle.Left;
  ScrollRectangle.top = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle.Top;
  ScrollRectangle.right = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle.Right;
  ScrollRectangle.bottom = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle.Bottom;
  ClipRectangle.left = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle.Left;
  ClipRectangle.top = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle.Top;
  ClipRectangle.right = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle.Right;
  ClipRectangle.bottom = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle.Bottom;

  /* Make sure source rectangle is inside the screen buffer */
  ConioInitRect(&ScreenBuffer, 0, 0, Buff->MaxY - 1, Buff->MaxX - 1);
  if (! ConioGetIntersection(&SrcRegion, &ScreenBuffer, &ScrollRectangle))
    {
      ConioUnlockScreenBuffer(Buff);
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }

  if (UseClipRectangle && ! ConioGetIntersection(&SrcRegion, &SrcRegion, &ClipRectangle))
    {
      ConioUnlockScreenBuffer(Buff);
      return Reply->Status = STATUS_SUCCESS;
    }


  ConioInitRect(&DstRegion,
               DestinationOrigin.Y,
               DestinationOrigin.X,
               DestinationOrigin.Y + ConioRectHeight(&ScrollRectangle) - 1,
               DestinationOrigin.X + ConioRectWidth(&ScrollRectangle) - 1);

  /* Make sure destination rectangle is inside the screen buffer */
  if (! ConioGetIntersection(&DstRegion, &DstRegion, &ScreenBuffer))
    {
      ConioUnlockScreenBuffer(Buff);
      return Reply->Status = STATUS_INVALID_PARAMETER;
    }

  ConioCopyRegion(Buff, &SrcRegion, &DstRegion);

  /* Get the region that should be filled with the specified character and attributes */

  DoFill = FALSE;

  ConioGetUnion(&FillRegion, &SrcRegion, &DstRegion);

  if (ConioSubtractRect(&FillRegion, &FillRegion, &DstRegion))
    {
      /* FIXME: The subtracted rectangle is off by one line */
      FillRegion.top += 1;

      ConioFillRegion(Buff, &FillRegion, Fill);
      DoFill = TRUE;
    }

  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      /* Draw destination region */
      ConioDrawRegion(Console, &DstRegion);

      if (DoFill)
        {
          /* Draw filled region */
          ConioDrawRegion(Console, &FillRegion);
        }
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrReadConsoleOutputChar)
{
  NTSTATUS Status;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD Xpos, Ypos;
  BYTE* ReadBuffer;
  DWORD i;

  DPRINT("CsrReadConsoleOutputChar\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = Reply->Header.MessageSize - sizeof(LPC_MESSAGE);
  ReadBuffer = Reply->Data.ReadConsoleOutputCharReply.String;

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputCharRequest.ConsoleHandle, Buff);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Xpos = Request->Data.ReadConsoleOutputCharRequest.ReadCoord.X + Buff->ShowX;
  Ypos = (Request->Data.ReadConsoleOutputCharRequest.ReadCoord.Y + Buff->ShowY) % Buff->MaxY;

  for (i = 0; i < Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead; ++i)
    {
      *ReadBuffer = Buff->Buffer[(Xpos * 2) + (Ypos * 2 * Buff->MaxX)];

      ReadBuffer++;
      Xpos++;

      if (Xpos == Buff->MaxX)
	{
	  Xpos = 0;
	  Ypos++;

	  if (Ypos == Buff->MaxY)
            {
              Ypos = 0;
            }
	}
    }

  *ReadBuffer = 0;
  Reply->Status = STATUS_SUCCESS;
  Reply->Data.ReadConsoleOutputCharReply.EndCoord.X = Xpos - Buff->ShowX;
  Reply->Data.ReadConsoleOutputCharReply.EndCoord.Y = (Ypos - Buff->ShowY + Buff->MaxY) % Buff->MaxY;
  Reply->Header.MessageSize += Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead;
  Reply->Header.DataSize += Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead;

  ConioUnlockScreenBuffer(Buff);

  return Reply->Status;
}


CSR_API(CsrReadConsoleOutputAttrib)
{
  NTSTATUS Status;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD Xpos, Ypos;
  CHAR* ReadBuffer;
  DWORD i;

  DPRINT("CsrReadConsoleOutputAttrib\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = Reply->Header.MessageSize - sizeof(LPC_MESSAGE);
  ReadBuffer = Reply->Data.ReadConsoleOutputAttribReply.String;

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputAttribRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Xpos = Request->Data.ReadConsoleOutputAttribRequest.ReadCoord.X + Buff->ShowX;
  Ypos = (Request->Data.ReadConsoleOutputAttribRequest.ReadCoord.Y + Buff->ShowY) % Buff->MaxY;

  for (i = 0; i < Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead; ++i)
    {
      *ReadBuffer = Buff->Buffer[(Xpos * 2) + (Ypos * 2 * Buff->MaxX) + 1];

      ReadBuffer++;
      Xpos++;

      if (Xpos == Buff->MaxX)
        {
          Xpos = 0;
          Ypos++;

          if (Ypos == Buff->MaxY)
            {
              Ypos = 0;
            }
        }
    }

  *ReadBuffer = 0;

  Reply->Status = STATUS_SUCCESS;
  Reply->Data.ReadConsoleOutputAttribReply.EndCoord.X = Xpos - Buff->ShowX;
  Reply->Data.ReadConsoleOutputAttribReply.EndCoord.Y = (Ypos - Buff->ShowY + Buff->MaxY) % Buff->MaxY;
  Reply->Header.MessageSize += Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead;
  Reply->Header.DataSize += Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead;

  ConioUnlockScreenBuffer(Buff);

  return Reply->Status;
}


CSR_API(CsrGetNumberOfConsoleInputEvents)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PLIST_ENTRY CurrentItem;
  DWORD NumEvents;
  
  DPRINT("CsrGetNumberOfConsoleInputEvents\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = Reply->Header.MessageSize - sizeof(LPC_MESSAGE);

  Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
  
  CurrentItem = &Console->InputEvents;
  NumEvents = 0;
  
  /* If there are any events ... */
  if (CurrentItem->Flink != CurrentItem)
    {
      do
        {
          CurrentItem = CurrentItem->Flink;
          ++NumEvents;
        }
      while (CurrentItem != &Console->InputEvents);
    }

  ConioUnlockConsole(Console);
  
  Reply->Status = STATUS_SUCCESS;
  Reply->Data.GetNumInputEventsReply.NumInputEvents = NumEvents;
   
  return Reply->Status;
}


CSR_API(CsrPeekConsoleInput)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  DWORD Size;
  DWORD Length;
  PLIST_ENTRY CurrentItem;
  PINPUT_RECORD InputRecord;
  ConsoleInput* Item;
  UINT NumItems;
   
  DPRINT("CsrPeekConsoleInput\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   
  Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console);
  if(! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
   
  InputRecord = Request->Data.PeekConsoleInputRequest.InputRecord;
  Length = Request->Data.PeekConsoleInputRequest.Length;
  Size = Length * sizeof(INPUT_RECORD);
   
  if (((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
      || (((PVOID)InputRecord + Size) > (ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockConsole(Console);
      Reply->Status = STATUS_ACCESS_VIOLATION;
      return Reply->Status ;
    }
   
  NumItems = 0;
   
  if (! IsListEmpty(&Console->InputEvents))
    {
      CurrentItem = &Console->InputEvents;
   
      while (NumItems < Length)
        {
          ++NumItems;
          Item = CONTAINING_RECORD(CurrentItem, ConsoleInput, ListEntry);
          *InputRecord++ = Item->InputEvent;
         
          if (CurrentItem->Flink == &Console->InputEvents)
            {
              break;
            }
          else
            {
              CurrentItem = CurrentItem->Flink;
            }
        }
    }

  ConioUnlockConsole(Console);

  Reply->Status = STATUS_SUCCESS;
  Reply->Data.PeekConsoleInputReply.Length = NumItems;

  return Reply->Status;
}


CSR_API(CsrReadConsoleOutput)
{
  PCHAR_INFO CharInfo;
  PCHAR_INFO CurCharInfo;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD Size;
  DWORD Length;
  DWORD SizeX, SizeY;
  NTSTATUS Status;
  COORD BufferSize;
  COORD BufferCoord;
  RECT ReadRegion;
  RECT ScreenRect;
  DWORD i, Y, X, Offset;
      
  DPRINT("CsrReadConsoleOutput\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
  
  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
   
  CharInfo = Request->Data.ReadConsoleOutputRequest.CharInfo;
  ReadRegion.left = Request->Data.ReadConsoleOutputRequest.ReadRegion.Left;
  ReadRegion.top = Request->Data.ReadConsoleOutputRequest.ReadRegion.Top;
  ReadRegion.right = Request->Data.ReadConsoleOutputRequest.ReadRegion.Right;
  ReadRegion.bottom = Request->Data.ReadConsoleOutputRequest.ReadRegion.Bottom;
  BufferSize = Request->Data.ReadConsoleOutputRequest.BufferSize;
  BufferCoord = Request->Data.ReadConsoleOutputRequest.BufferCoord;
  Length = BufferSize.X * BufferSize.Y;
  Size = Length * sizeof(CHAR_INFO);
   
  if (((PVOID)CharInfo < ProcessData->CsrSectionViewBase)
      || (((PVOID)CharInfo + Size) > (ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockScreenBuffer(Buff);
      Reply->Status = STATUS_ACCESS_VIOLATION;
      return Reply->Status ;
    }
   
  SizeY = RtlRosMin(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&ReadRegion));
  SizeX = RtlRosMin(BufferSize.X - BufferCoord.X, ConioRectWidth(&ReadRegion));
  ReadRegion.bottom = ReadRegion.top + SizeY;
  ReadRegion.right = ReadRegion.left + SizeX;

  ConioInitRect(&ScreenRect, 0, 0, Buff->MaxY, Buff->MaxX);
  if (! ConioGetIntersection(&ReadRegion, &ScreenRect, &ReadRegion))
    {
      ConioUnlockScreenBuffer(Buff);
      Reply->Status = STATUS_SUCCESS;
      return Reply->Status;
    }

  for (i = 0, Y = ReadRegion.top; Y < ReadRegion.bottom; ++i, ++Y)
    {
      CurCharInfo = CharInfo + (i * BufferSize.X);
     
      Offset = (((Y + Buff->ShowY) % Buff->MaxY) * Buff->MaxX + ReadRegion.left) * 2;
      for (X = ReadRegion.left; X < ReadRegion.right; ++X)
        {
          CurCharInfo->Char.AsciiChar = GET_CELL_BUFFER(Buff, Offset);
          CurCharInfo->Attributes = GET_CELL_BUFFER(Buff, Offset);
          ++CurCharInfo;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  
  Reply->Status = STATUS_SUCCESS;
  Reply->Data.ReadConsoleOutputReply.ReadRegion.Right = ReadRegion.left + SizeX - 1;
  Reply->Data.ReadConsoleOutputReply.ReadRegion.Bottom = ReadRegion.top + SizeY - 1;
  Reply->Data.ReadConsoleOutputReply.ReadRegion.Left = ReadRegion.left;
  Reply->Data.ReadConsoleOutputReply.ReadRegion.Top = ReadRegion.top;
   
  return Reply->Status;
}


CSR_API(CsrWriteConsoleInput)
{
  PINPUT_RECORD InputRecord;
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;
  DWORD Length;
  DWORD Size;
  DWORD i;
  ConsoleInput* Record;
   
  DPRINT("CsrWriteConsoleInput\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   
  Status = ConioLockConsole(ProcessData, Request->Data.WriteConsoleInputRequest.ConsoleHandle, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }
   
  InputRecord = Request->Data.WriteConsoleInputRequest.InputRecord;
  Length = Request->Data.WriteConsoleInputRequest.Length;
  Size = Length * sizeof(INPUT_RECORD);
   
  if (((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
      || (((PVOID)InputRecord + Size) > (ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockConsole(Console);
      Reply->Status = STATUS_ACCESS_VIOLATION;
      return Reply->Status ;
    }
   
  for (i = 0; i < Length; i++)
    {
      Record = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));
      if (NULL == Record)
        {
          ConioUnlockConsole(Console);
          Reply->Status = STATUS_INSUFFICIENT_RESOURCES;
          return Reply->Status;
        }

      Record->Echoed = FALSE;
      Record->Fake = FALSE;
      Record->InputEvent = *InputRecord++;
      if (KEY_EVENT == Record->InputEvent.EventType)
        {
          ConioProcessChar(Console, Record);
        }
    }

  ConioUnlockConsole(Console);
   
  Reply->Status = STATUS_SUCCESS;
  Reply->Data.WriteConsoleInputReply.Length = i;

  return Reply->Status;
}

/**********************************************************************
 *	HardwareStateProperty
 *
 *	DESCRIPTION
 *		Set/Get the value of the HardwareState and switch
 *		between direct video buffer ouput and GDI windowed
 *		output.
 *	ARGUMENTS
 *		Client hands us a CSRSS_CONSOLE_HARDWARE_STATE
 *		object. We use the same object to reply.
 *	NOTE
 *		ConsoleHwState has the correct size to be compatible
 *		with NT's, but values are not.
 */
STATIC NTSTATUS FASTCALL
SetConsoleHardwareState (PCSRSS_CONSOLE Console, DWORD ConsoleHwState)
{
  DPRINT1("Console Hardware State: %d\n", ConsoleHwState);

  if ((CONSOLE_HARDWARE_STATE_GDI_MANAGED == ConsoleHwState)
      ||(CONSOLE_HARDWARE_STATE_DIRECT == ConsoleHwState))
    {
      if (Console->HardwareState != ConsoleHwState)
        {
          /* TODO: implement switching from full screen to windowed mode */
          /* TODO: or back; now simply store the hardware state */
          Console->HardwareState = ConsoleHwState;
        }

      return STATUS_SUCCESS;	
    }

  return STATUS_INVALID_PARAMETER_3; /* Client: (handle, set_get, [mode]) */
}

CSR_API(CsrHardwareStateProperty)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;
 
  DPRINT("CsrHardwareStateProperty\n");

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   
  Status = ConioLockConsole(ProcessData,
                            Request->Data.ConsoleHardwareStateRequest.ConsoleHandle,
                            &Console);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed to get console handle in SetConsoleHardwareState\n");
      return Reply->Status = Status;
    }

  switch (Request->Data.ConsoleHardwareStateRequest.SetGet)
    {
      case CONSOLE_HARDWARE_STATE_GET:
        Reply->Data.ConsoleHardwareStateReply.State = Console->HardwareState;
        break;
      
      case CONSOLE_HARDWARE_STATE_SET:
        DPRINT("Setting console hardware state.\n");
        Reply->Status = SetConsoleHardwareState(Console, Request->Data.ConsoleHardwareStateRequest.State);
        break;

      default:
        Reply->Status = STATUS_INVALID_PARAMETER_2; /* Client: (handle, [set_get], mode) */
        break;
    }

  ConioUnlockConsole(Console);

  return Reply->Status;
}

CSR_API(CsrGetConsoleWindow)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrGetConsoleWindow\n");
 
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE);
   
  Status = ConioLockConsole(ProcessData,
                            Request->Data.ConsoleWindowRequest.ConsoleHandle,
                            &Console);
  if (! NT_SUCCESS(Status))
    {
      return Reply->Status = Status;
    }

  Reply->Data.ConsoleWindowReply.WindowHandle = Console->hWindow;
  ConioUnlockConsole(Console);

  return Reply->Status = STATUS_SUCCESS;
}

/* EOF */
