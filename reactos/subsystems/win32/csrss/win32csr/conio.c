/*
 * reactos/subsys/csrss/win32csr/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include "w32csr.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define ConioInitRect(Rect, Top, Left, Bottom, Right) \
  ((Rect)->top) = Top; \
  ((Rect)->left) = Left; \
  ((Rect)->bottom) = Bottom; \
  ((Rect)->right) = Right

#define ConioIsRectEmpty(Rect) \
  (((Rect)->left > (Rect)->right) || ((Rect)->top > (Rect)->bottom))

#define ConsoleInputUnicodeCharToAnsiChar(Console, dChar, sWChar) \
  WideCharToMultiByte((Console)->CodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleUnicodeCharToAnsiChar(Console, dChar, sWChar) \
  WideCharToMultiByte((Console)->OutputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleAnsiCharToUnicodeChar(Console, sWChar, dChar) \
  MultiByteToWideChar((Console)->OutputCodePage, 0, (dChar), 1, (sWChar), 1)


/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
ConioConsoleFromProcessData(PCSRSS_PROCESS_DATA ProcessData, PCSRSS_CONSOLE *Console)
{
  PCSRSS_CONSOLE ProcessConsole = ProcessData->Console;

  if (!ProcessConsole)
    {
      *Console = NULL;
      return STATUS_INVALID_HANDLE;
    }

  InterlockedIncrement(&ProcessConsole->Header.ReferenceCount);
  EnterCriticalSection(&(ProcessConsole->Header.Lock));
  *Console = ProcessConsole;

  return STATUS_SUCCESS;
}

VOID FASTCALL
ConioConsoleCtrlEventTimeout(DWORD Event, PCSRSS_PROCESS_DATA ProcessData, DWORD Timeout)
{
  HANDLE Thread;

  DPRINT("ConioConsoleCtrlEvent Parent ProcessId = %x\n", ProcessData->ProcessId);

  if (ProcessData->CtrlDispatcher)
    {

      Thread = CreateRemoteThread(ProcessData->Process, NULL, 0,
                                  (LPTHREAD_START_ROUTINE) ProcessData->CtrlDispatcher,
                                  (PVOID) Event, 0, NULL);
      if (NULL == Thread)
        {
          DPRINT1("Failed thread creation (Error: 0x%x)\n", GetLastError());
          return;
        }
      WaitForSingleObject(Thread, Timeout);
      CloseHandle(Thread);
    }
}

VOID FASTCALL
ConioConsoleCtrlEvent(DWORD Event, PCSRSS_PROCESS_DATA ProcessData)
{
  ConioConsoleCtrlEventTimeout(Event, ProcessData, 0);
}

PBYTE FASTCALL
ConioCoordToPointer(PCSRSS_SCREEN_BUFFER Buff, ULONG X, ULONG Y)
{
  return &Buff->Buffer[2 * (((Y + Buff->VirtualY) % Buff->MaxY) * Buff->MaxX + X)];
}

static VOID FASTCALL
ClearLineBuffer(PCSRSS_SCREEN_BUFFER Buff)
{
  PBYTE Ptr = ConioCoordToPointer(Buff, 0, Buff->CurrentY);
  UINT Pos;

  for (Pos = 0; Pos < Buff->MaxX; Pos++)
    {
      /* Fill the cell */
      *Ptr++ = ' ';
      *Ptr++ = Buff->DefaultAttrib;
    }
}

static NTSTATUS FASTCALL
CsrInitConsoleScreenBuffer(PCSRSS_CONSOLE Console,
                           PCSRSS_SCREEN_BUFFER Buffer)
{
  DPRINT("CsrInitConsoleScreenBuffer Size X %d Size Y %d\n", Buffer->MaxX, Buffer->MaxY);

  Buffer->Header.Type = CONIO_SCREEN_BUFFER_MAGIC;
  Buffer->Header.ReferenceCount = 0;
  Buffer->ShowX = 0;
  Buffer->ShowY = 0;
  Buffer->VirtualY = 0;
  Buffer->Buffer = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, Buffer->MaxX * Buffer->MaxY * 2);
  if (NULL == Buffer->Buffer)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  InitializeCriticalSection(&Buffer->Header.Lock);
  ConioInitScreenBuffer(Console, Buffer);
  /* initialize buffer to be empty with default attributes */
  for (Buffer->CurrentY = 0 ; Buffer->CurrentY < Buffer->MaxY; Buffer->CurrentY++)
    {
      ClearLineBuffer(Buffer);
    }
  Buffer->Mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
  Buffer->CurrentX = 0;
  Buffer->CurrentY = 0;

  return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
CsrInitConsole(PCSRSS_CONSOLE Console)
{
  NTSTATUS Status;
  SECURITY_ATTRIBUTES SecurityAttributes;
  PCSRSS_SCREEN_BUFFER NewBuffer;
  BOOL GuiMode;

  Console->Title.MaximumLength = Console->Title.Length = 0;
  Console->Title.Buffer = NULL;

  //FIXME
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
  Console->CodePage = GetOEMCP();
  Console->OutputCodePage = GetOEMCP();

  SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  SecurityAttributes.lpSecurityDescriptor = NULL;
  SecurityAttributes.bInheritHandle = TRUE;

  Console->ActiveEvent = CreateEventW(&SecurityAttributes, TRUE, FALSE, NULL);
  if (NULL == Console->ActiveEvent)
    {
      RtlFreeUnicodeString(&Console->Title);
      return STATUS_UNSUCCESSFUL;
    }
  Console->PrivateData = NULL;
  InitializeCriticalSection(&Console->Header.Lock);

  GuiMode = DtbgIsDesktopVisible();

  /* allocate console screen buffer */
  NewBuffer = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, sizeof(CSRSS_SCREEN_BUFFER));
  if (NULL == NewBuffer)
    {
      RtlFreeUnicodeString(&Console->Title);
      DeleteCriticalSection(&Console->Header.Lock);
      CloseHandle(Console->ActiveEvent);
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  /* init screen buffer with defaults */
  NewBuffer->CursorInfo.bVisible = TRUE;
  NewBuffer->CursorInfo.dwSize = 5;
  /* make console active, and insert into console list */
  Console->ActiveBuffer = (PCSRSS_SCREEN_BUFFER) NewBuffer;

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
          HeapFree(Win32CsrApiHeap,0, NewBuffer);
          RtlFreeUnicodeString(&Console->Title);
          DeleteCriticalSection(&Console->Header.Lock);
          CloseHandle(Console->ActiveEvent);
          DPRINT1("GuiInitConsole: failed\n");
          return Status;
        }
    }

  Status = CsrInitConsoleScreenBuffer(Console, NewBuffer);
  if (! NT_SUCCESS(Status))
    {
      ConioCleanupConsole(Console);
      RtlFreeUnicodeString(&Console->Title);
      DeleteCriticalSection(&Console->Header.Lock);
      CloseHandle(Console->ActiveEvent);
      HeapFree(Win32CsrApiHeap, 0, NewBuffer);
      DPRINT1("CsrInitConsoleScreenBuffer: failed\n");
      return Status;
    }

  /* add a reference count because the buffer is tied to the console */
  InterlockedIncrement(&Console->ActiveBuffer->Header.ReferenceCount);

  /* copy buffer contents to screen */
  ConioDrawConsole(Console);

  return STATUS_SUCCESS;
}


CSR_API(CsrAllocConsole)
{
    PCSRSS_CONSOLE Console;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN NewConsole = FALSE;

    DPRINT("CsrAllocConsole\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    if (ProcessData->Console)
    {
        DPRINT1("Process already has a console\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* If we don't need a console, then get out of here */
    if (!Request->Data.AllocConsoleRequest.ConsoleNeeded)
    {
        DPRINT("No console needed\n");
        return STATUS_SUCCESS;
    }

    /* If we already have one, then don't create a new one... */
    if (!Request->Data.AllocConsoleRequest.Console ||
        Request->Data.AllocConsoleRequest.Console != ProcessData->ParentConsole)
    {
        /* Allocate a console structure */
        NewConsole = TRUE;
        Console = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, sizeof(CSRSS_CONSOLE));
        if (NULL == Console)
        {
            DPRINT1("Not enough memory for console\n");
            return STATUS_NO_MEMORY;
        }
        /* initialize list head */
        InitializeListHead(&Console->ProcessList);
        /* insert process data required for GUI initialization */
        InsertHeadList(&Console->ProcessList, &ProcessData->ProcessEntry);
        /* Initialize the Console */
        Status = CsrInitConsole(Console);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Console init failed\n");
            HeapFree(Win32CsrApiHeap, 0, Console);
            return Status;
        }
    }
    else
    {
        /* Reuse our current console */
        Console = Request->Data.AllocConsoleRequest.Console;
    }

    /* Set the Process Console */
    ProcessData->Console = Console;

    /* Return it to the caller */
    Request->Data.AllocConsoleRequest.Console = Console;

    /* Add a reference count because the process is tied to the console */
    Console->Header.ReferenceCount++;

    if (NewConsole || !ProcessData->bInheritHandles)
    {
        /* Insert the Objects */
        Status = Win32CsrInsertObject(ProcessData,
                                      &Request->Data.AllocConsoleRequest.InputHandle,
                                      &Console->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert object\n");
            ConioDeleteConsole((Object_t *) Console);
            ProcessData->Console = 0;
            return Status;
        }

        Status = Win32CsrInsertObject(ProcessData,
                                      &Request->Data.AllocConsoleRequest.OutputHandle,
                                      &Console->ActiveBuffer->Header,
                                      GENERIC_READ | GENERIC_WRITE,
                                      TRUE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert object\n");
            ConioDeleteConsole((Object_t *) Console);
            Win32CsrReleaseObject(ProcessData,
                                  Request->Data.AllocConsoleRequest.InputHandle);
            ProcessData->Console = 0;
            return Status;
        }
    }

    /* Duplicate the Event */
    if (!DuplicateHandle(GetCurrentProcess(),
                         ProcessData->Console->ActiveEvent,
                         ProcessData->Process,
                         &ProcessData->ConsoleEvent,
                         EVENT_ALL_ACCESS,
                         FALSE,
                         0))
    {
        DPRINT1("DuplicateHandle() failed: %d\n", GetLastError);
        ConioDeleteConsole((Object_t *) Console);
        if (NewConsole || !ProcessData->bInheritHandles)
        {
            Win32CsrReleaseObject(ProcessData,
                                  Request->Data.AllocConsoleRequest.OutputHandle);
            Win32CsrReleaseObject(ProcessData,
                                  Request->Data.AllocConsoleRequest.InputHandle);
        }
        ProcessData->Console = 0;
        return Status;
    }

    /* Set the Ctrl Dispatcher */
    ProcessData->CtrlDispatcher = Request->Data.AllocConsoleRequest.CtrlDispatcher;
    DPRINT("CSRSS:CtrlDispatcher address: %x\n", ProcessData->CtrlDispatcher);

    if (!NewConsole)
    {
        /* Insert into the list if it has not been added */
        InsertHeadList(&ProcessData->Console->ProcessList, &ProcessData->ProcessEntry);
    }

    return STATUS_SUCCESS;
}

CSR_API(CsrFreeConsole)
{
  PCSRSS_CONSOLE Console;


  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  if (ProcessData->Console == NULL)
    {
      return STATUS_INVALID_PARAMETER;
    }

  Console = ProcessData->Console;
  ProcessData->Console = NULL;
  RemoveEntryList(&ProcessData->ProcessEntry);
  if (0 == InterlockedDecrement(&Console->Header.ReferenceCount))
    {
      ConioDeleteConsole((Object_t *) Console);
    }
  return STATUS_SUCCESS;
}

static VOID FASTCALL
ConioNextLine(PCSRSS_SCREEN_BUFFER Buff, RECT *UpdateRect, UINT *ScrolledLines)
{
  /* If we hit bottom, slide the viewable screen */
  if (++Buff->CurrentY == Buff->MaxY)
    {
      Buff->CurrentY--;
      if (++Buff->VirtualY == Buff->MaxY)
        {
          Buff->VirtualY = 0;
        }
      (*ScrolledLines)++;
      ClearLineBuffer(Buff);
      if (UpdateRect->top != 0)
        {
          UpdateRect->top--;
        }
    }
  UpdateRect->left = 0;
  UpdateRect->right = Buff->MaxX - 1;
  UpdateRect->bottom = Buff->CurrentY;
}

static NTSTATUS FASTCALL
ConioWriteConsole(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff,
                  CHAR *Buffer, DWORD Length, BOOL Attrib)
{
  UINT i;
  PBYTE Ptr;
  RECT UpdateRect;
  LONG CursorStartX, CursorStartY;
  UINT ScrolledLines;

  CursorStartX = Buff->CurrentX;
  CursorStartY = Buff->CurrentY;
  UpdateRect.left = Buff->MaxX;
  UpdateRect.top = Buff->CurrentY;
  UpdateRect.right = -1;
  UpdateRect.bottom = Buff->CurrentY;
  ScrolledLines = 0;

  for (i = 0; i < Length; i++)
    {
      if (Buff->Mode & ENABLE_PROCESSED_OUTPUT)
        {
          /* --- LF --- */
          if (Buffer[i] == '\n')
            {
              Buff->CurrentX = 0;
              ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
              continue;
            }
          /* --- BS --- */
          else if (Buffer[i] == '\b')
            {
              /* Only handle BS if we're not on the first pos of the first line */
              if (0 != Buff->CurrentX || 0 != Buff->CurrentY)
                {
                  if (0 == Buff->CurrentX)
                    {
                      /* slide virtual position up */
                      Buff->CurrentX = Buff->MaxX - 1;
                      Buff->CurrentY--;
                      UpdateRect.top = min(UpdateRect.top, (LONG)Buff->CurrentY);
                    }
                  else
                    {
                      Buff->CurrentX--;
                    }
                  Ptr = ConioCoordToPointer(Buff, Buff->CurrentX, Buff->CurrentY);
                  Ptr[0] = ' ';
                  Ptr[1] = Buff->DefaultAttrib;
                  UpdateRect.left = min(UpdateRect.left, (LONG) Buff->CurrentX);
                  UpdateRect.right = max(UpdateRect.right, (LONG) Buff->CurrentX);
                }
                continue;
            }
          /* --- CR --- */
          else if (Buffer[i] == '\r')
            {
              Buff->CurrentX = 0;
              UpdateRect.left = min(UpdateRect.left, (LONG) Buff->CurrentX);
              UpdateRect.right = max(UpdateRect.right, (LONG) Buff->CurrentX);
              continue;
            }
          /* --- TAB --- */
          else if (Buffer[i] == '\t')
            {
              UINT EndX;

              UpdateRect.left = min(UpdateRect.left, (LONG)Buff->CurrentX);
              EndX = (Buff->CurrentX + 8) & ~7;
              if (EndX > Buff->MaxX)
                {
                  EndX = Buff->MaxX;
                }
              Ptr = ConioCoordToPointer(Buff, Buff->CurrentX, Buff->CurrentY);
              while (Buff->CurrentX < EndX)
                {
                  *Ptr++ = ' ';
                  *Ptr++ = Buff->DefaultAttrib;
                  Buff->CurrentX++;
                }
              UpdateRect.right = max(UpdateRect.right, (LONG) Buff->CurrentX - 1);
              if (Buff->CurrentX == Buff->MaxX)
                {
                  if (Buff->Mode & ENABLE_WRAP_AT_EOL_OUTPUT)
                    {
                      Buff->CurrentX = 0;
                      ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
                    }
                  else
                    {
                      Buff->CurrentX--;
                    }
                }
              continue;
            }
        }
      UpdateRect.left = min(UpdateRect.left, (LONG)Buff->CurrentX);
      UpdateRect.right = max(UpdateRect.right, (LONG) Buff->CurrentX);
      Ptr = ConioCoordToPointer(Buff, Buff->CurrentX, Buff->CurrentY);
      Ptr[0] = Buffer[i];
      if (Attrib)
        {
          Ptr[1] = Buff->DefaultAttrib;
        }
      Buff->CurrentX++;
      if (Buff->CurrentX == Buff->MaxX)
        {
          if (Buff->Mode & ENABLE_WRAP_AT_EOL_OUTPUT)
            {
              Buff->CurrentX = 0;
              ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
            }
          else
            {
              Buff->CurrentX = CursorStartX;
            }
        }
    }

  if (! ConioIsRectEmpty(&UpdateRect) && Buff == Console->ActiveBuffer)
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
  PUCHAR Buffer;
  PWCHAR UnicodeBuffer;
  ULONG i;
  ULONG nNumberOfCharsToRead, CharSize;
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrReadConsole\n");

  CharSize = (Request->Data.ReadConsoleRequest.Unicode ? sizeof(WCHAR) : sizeof(CHAR));

  /* truncate length to CSRSS_MAX_READ_CONSOLE_REQUEST */
  nNumberOfCharsToRead = min(Request->Data.ReadConsoleRequest.NrCharactersToRead, CSRSS_MAX_READ_CONSOLE / CharSize);
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Buffer = Request->Data.ReadConsoleRequest.Buffer;
  UnicodeBuffer = (PWCHAR)Buffer;
  Status = ConioLockConsole(ProcessData, Request->Data.ReadConsoleRequest.ConsoleHandle,
                               &Console, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  Request->Data.ReadConsoleRequest.EventHandle = ProcessData->ConsoleEvent;
  for (i = 0; i < nNumberOfCharsToRead && Console->InputEvents.Flink != &Console->InputEvents; i++)
    {
      /* remove input event from queue */
      CurrentEntry = RemoveHeadList(&Console->InputEvents);
      if (IsListEmpty(&Console->InputEvents))
      {
         ResetEvent(Console->ActiveEvent);
      }
      Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

      /* only pay attention to valid ascii chars, on key down */
      if (KEY_EVENT == Input->InputEvent.EventType
          && Input->InputEvent.Event.KeyEvent.bKeyDown
          && Input->InputEvent.Event.KeyEvent.uChar.AsciiChar != '\0')
        {
          /*
           * backspace handling - if we are in charge of echoing it then we handle it here
           * otherwise we treat it like a normal char.
           */
          if ('\b' == Input->InputEvent.Event.KeyEvent.uChar.AsciiChar && 0
              != (Console->Mode & ENABLE_ECHO_INPUT))
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
                  Console->WaitingChars--;
                  ConioUnlockConsole(Console);
                  HeapFree(Win32CsrApiHeap, 0, Input);
                  Request->Data.ReadConsoleRequest.NrCharactersRead = 0;
                  return STATUS_NOTIFY_CLEANUP;

                }
              Request->Data.ReadConsoleRequest.nCharsCanBeDeleted--;
              Input->Echoed = TRUE;   /* mark as echoed so we don't echo it below */
            }
          /* do not copy backspace to buffer */
          else
            {
              if(Request->Data.ReadConsoleRequest.Unicode)
                UnicodeBuffer[i] = Input->InputEvent.Event.KeyEvent.uChar.AsciiChar; /* FIXME */
              else
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
  Request->Data.ReadConsoleRequest.NrCharactersRead = i;
  if (0 == i)
    {
      Status = STATUS_PENDING;    /* we didn't read anything */
    }
  else if (0 != (Console->Mode & ENABLE_LINE_INPUT))
    {
      if (0 == Console->WaitingLines ||
          (Request->Data.ReadConsoleRequest.Unicode ? (L'\n' != UnicodeBuffer[i - 1]) : ('\n' != Buffer[i - 1])))
        {
          Status = STATUS_PENDING; /* line buffered, didn't get a complete line */
        }
      else
        {
          Console->WaitingLines--;
          Status = STATUS_SUCCESS; /* line buffered, did get a complete line */
        }
    }
  else
    {
      Status = STATUS_SUCCESS;  /* not line buffered, did read something */
    }

  if (Status == STATUS_PENDING)
    {
      Console->EchoCount = nNumberOfCharsToRead - i;
    }
  else
    {
      Console->EchoCount = 0;             /* if the client is no longer waiting on input, do not echo */
    }

  ConioUnlockConsole(Console);

  if (CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE) + i * CharSize > sizeof(CSR_API_MESSAGE))
    {
      Request->Header.u1.s1.TotalLength = CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE) + i * CharSize;
      Request->Header.u1.s1.DataLength = Request->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);
    }

  return Status;
}

BOOLEAN __inline ConioGetIntersection(
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
               max(Rect1->top, Rect2->top),
               max(Rect1->left, Rect2->left),
               min(Rect1->bottom, Rect2->bottom),
               min(Rect1->right, Rect2->right));

  return TRUE;
}

BOOLEAN __inline ConioGetUnion(
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
                   min(Rect1->top, Rect2->top),
                   min(Rect1->left, Rect2->left),
                   max(Rect1->bottom, Rect2->bottom),
                   max(Rect1->right, Rect2->right));
    }

  return TRUE;
}

/* Move from one rectangle to another. We must be careful about the order that
 * this is done, to avoid overwriting parts of the source before they are moved. */
static VOID FASTCALL
ConioMoveRegion(PCSRSS_SCREEN_BUFFER ScreenBuffer,
                RECT *SrcRegion,
                RECT *DstRegion,
                RECT *ClipRegion,
                WORD Fill)
{
  int Width = ConioRectWidth(SrcRegion);
  int Height = ConioRectHeight(SrcRegion);
  int SX, SY;
  int DX, DY;
  int XDelta, YDelta;
  int i, j;

  SY = SrcRegion->top;
  DY = DstRegion->top;
  YDelta = 1;
  if (SY < DY)
    {
      /* Moving down: work from bottom up */
      SY = SrcRegion->bottom;
      DY = DstRegion->bottom;
      YDelta = -1;
    }
  for (i = 0; i < Height; i++)
    {
      PWORD SRow = (PWORD)ConioCoordToPointer(ScreenBuffer, 0, SY);
      PWORD DRow = (PWORD)ConioCoordToPointer(ScreenBuffer, 0, DY);

      SX = SrcRegion->left;
      DX = DstRegion->left;
      XDelta = 1;
      if (SX < DX)
        {
          /* Moving right: work from right to left */
          SX = SrcRegion->right;
          DX = DstRegion->right;
          XDelta = -1;
        }
      for (j = 0; j < Width; j++)
        {
          WORD Cell = SRow[SX];
          if (SX >= ClipRegion->left && SX <= ClipRegion->right
              && SY >= ClipRegion->top && SY <= ClipRegion->bottom)
            {
              SRow[SX] = Fill;
            }
          if (DX >= ClipRegion->left && DX <= ClipRegion->right
              && DY >= ClipRegion->top && DY <= ClipRegion->bottom)
            {
              DRow[DX] = Cell;
            }
          SX += XDelta;
          DX += XDelta;
        }
      SY += YDelta;
      DY += YDelta;
    }
}

static VOID FASTCALL
ConioInputEventToAnsi(PCSRSS_CONSOLE Console, PINPUT_RECORD InputEvent)
{
  if (InputEvent->EventType == KEY_EVENT)
    {
      WCHAR UnicodeChar = InputEvent->Event.KeyEvent.uChar.UnicodeChar;
      InputEvent->Event.KeyEvent.uChar.UnicodeChar = 0;
      ConsoleInputUnicodeCharToAnsiChar(Console,
                                        &InputEvent->Event.KeyEvent.uChar.AsciiChar,
                                        &UnicodeChar);
    }
}

CSR_API(CsrWriteConsole)
{
  NTSTATUS Status;
  PCHAR Buffer;
  PCSRSS_SCREEN_BUFFER Buff;
  PCSRSS_CONSOLE Console;
  DWORD Written = 0;
  ULONG Length;
  ULONG CharSize = (Request->Data.WriteConsoleRequest.Unicode ? sizeof(WCHAR) : sizeof(CHAR));

  DPRINT("CsrWriteConsole\n");

  if (Request->Header.u1.s1.TotalLength
      < CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE)
        + (Request->Data.WriteConsoleRequest.NrCharactersToWrite * CharSize))
    {
      DPRINT1("Invalid request size\n");
      Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
      Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
      return STATUS_INVALID_PARAMETER;
    }
  Status = ConioConsoleFromProcessData(ProcessData, &Console);

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  if(Request->Data.WriteConsoleRequest.Unicode)
    {
      Length = WideCharToMultiByte(Console->OutputCodePage, 0,
                                   (PWCHAR)Request->Data.WriteConsoleRequest.Buffer,
                                   Request->Data.WriteConsoleRequest.NrCharactersToWrite,
                                   NULL, 0, NULL, NULL);
      Buffer = RtlAllocateHeap(GetProcessHeap(), 0, Length);
      if (Buffer)
        {
          WideCharToMultiByte(Console->OutputCodePage, 0,
                              (PWCHAR)Request->Data.WriteConsoleRequest.Buffer,
                              Request->Data.WriteConsoleRequest.NrCharactersToWrite,
                              Buffer, Length, NULL, NULL);
        }
      else
        {
          Status = STATUS_NO_MEMORY;
        }
    }
  else
    {
      Buffer = (PCHAR)Request->Data.WriteConsoleRequest.Buffer;
    }

  if (Buffer)
    {
      Status = ConioLockScreenBuffer(ProcessData, Request->Data.WriteConsoleRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
      if (NT_SUCCESS(Status))
        {
          Status = ConioWriteConsole(Console, Buff, Buffer,
                                     Request->Data.WriteConsoleRequest.NrCharactersToWrite, TRUE);
          if (NT_SUCCESS(Status))
            {
              Written = Request->Data.WriteConsoleRequest.NrCharactersToWrite;
            }
          ConioUnlockScreenBuffer(Buff);
        }
      if (Request->Data.WriteConsoleRequest.Unicode)
        {
          RtlFreeHeap(GetProcessHeap(), 0, Buffer);
        }
    }
  ConioUnlockConsole(Console);

  Request->Data.WriteConsoleRequest.NrCharactersWritten = Written;

  return Status;
}

VOID STDCALL
ConioDeleteScreenBuffer(Object_t *Object)
{
  PCSRSS_SCREEN_BUFFER Buffer = (PCSRSS_SCREEN_BUFFER) Object;
  DeleteCriticalSection(&Buffer->Header.Lock);
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

  ConioCleanupConsole(Console);
  if (0 == InterlockedDecrement(&Console->ActiveBuffer->Header.ReferenceCount))
    {
      ConioDeleteScreenBuffer((Object_t *) Console->ActiveBuffer);
    }

  Console->ActiveBuffer = NULL;

  CloseHandle(Console->ActiveEvent);
  DeleteCriticalSection(&Console->Header.Lock);
  RtlFreeUnicodeString(&Console->Title);
  IntDeleteAllAliases(Console->Aliases);
  HeapFree(Win32CsrApiHeap, 0, Console);
}

VOID STDCALL
CsrInitConsoleSupport(VOID)
{
  DPRINT("CSR: CsrInitConsoleSupport()\n");

  /* Should call LoadKeyboardLayout */
}

static VOID FASTCALL
ConioProcessChar(PCSRSS_CONSOLE Console,
                 ConsoleInput *KeyEventRecord)
{
  BOOL updown;
  ConsoleInput *TempInput;

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
          && KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown))
    {
      if ('\n' == KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar)
        {
          Console->WaitingLines++;
        }
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
          return;
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
  SetEvent(Console->ActiveEvent);
}

static DWORD FASTCALL
ConioGetShiftState(PBYTE KeyState)
{
  DWORD ssOut = 0;

  if (KeyState[VK_CAPITAL] & 1)
      ssOut |= CAPSLOCK_ON;

  if (KeyState[VK_NUMLOCK] & 1)
      ssOut |= NUMLOCK_ON;

  if (KeyState[VK_SCROLL] & 1)
      ssOut |= SCROLLLOCK_ON;

  if (KeyState[VK_SHIFT] & 0x80)
      ssOut |= SHIFT_PRESSED;

  if (KeyState[VK_LCONTROL] & 0x80)
      ssOut |= LEFT_CTRL_PRESSED;
  if (KeyState[VK_RCONTROL] & 0x80)
      ssOut |= RIGHT_CTRL_PRESSED;

  if (KeyState[VK_LMENU] & 0x80)
      ssOut |= LEFT_ALT_PRESSED;
  if (KeyState[VK_RMENU] & 0x80)
      ssOut |= RIGHT_ALT_PRESSED;

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

  if (0 == ResultSize)
    {
      AsciiChar = 0;
    }

  er.EventType = KEY_EVENT;
  er.Event.KeyEvent.bKeyDown = Down;
  er.Event.KeyEvent.wRepeatCount = RepeatCount;
  er.Event.KeyEvent.uChar.UnicodeChar = UnicodeChar;
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
      DPRINT1("No Active Console!\n");
      return;
    }

  ConInRec = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));

  if (NULL == ConInRec)
    {
      return;
    }

  ConInRec->InputEvent = er;
  ConInRec->Fake = UnicodeChar &&
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

  if (ConInRec->Fake && ConInRec->NotChar)
    {
      HeapFree(Win32CsrApiHeap, 0, ConInRec);
      return;
    }

  /* process Ctrl-C and Ctrl-Break */
  if (Console->Mode & ENABLE_PROCESSED_INPUT &&
      er.Event.KeyEvent.bKeyDown &&
      ((er.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE) ||
       (er.Event.KeyEvent.wVirtualKeyCode == 'C')) &&
      (er.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
    {
      PCSRSS_PROCESS_DATA current;
      PLIST_ENTRY current_entry;
      DPRINT1("Console_Api Ctrl-C\n");
      current_entry = Console->ProcessList.Flink;
      while (current_entry != &Console->ProcessList)
      {
        current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
        current_entry = current_entry->Flink;
        ConioConsoleCtrlEvent((DWORD)CTRL_C_EVENT, current);
      }
      HeapFree(Win32CsrApiHeap, 0, ConInRec);
      return;
    }

  if (0 != (er.Event.KeyEvent.dwControlKeyState
            & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
      && (VK_UP == er.Event.KeyEvent.wVirtualKeyCode
          || VK_DOWN == er.Event.KeyEvent.wVirtualKeyCode))
    {
      if (er.Event.KeyEvent.bKeyDown)
        {
          /* scroll up or down */
          if (VK_UP == er.Event.KeyEvent.wVirtualKeyCode)
            {
              /* only scroll up if there is room to scroll up into */
              if (Console->ActiveBuffer->CurrentY != Console->ActiveBuffer->MaxY - 1)
                {
                  Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY +
                                                     Console->ActiveBuffer->MaxY - 1) %
                                                    Console->ActiveBuffer->MaxY;
                  Console->ActiveBuffer->CurrentY++;
                }
            }
          else
            {
              /* only scroll down if there is room to scroll down into */
              if (Console->ActiveBuffer->CurrentY != 0)
                {
                  Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY + 1) %
                                                    Console->ActiveBuffer->MaxY;
                  Console->ActiveBuffer->CurrentY--;
                }
            }
          ConioDrawConsole(Console);
        }
      HeapFree(Win32CsrApiHeap, 0, ConInRec);
      return;
    }
  /* FIXME - convert to ascii */
  ConioProcessChar(Console, ConInRec);
}

CSR_API(CsrGetScreenBufferInfo)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  PCONSOLE_SCREEN_BUFFER_INFO pInfo;

  DPRINT("CsrGetScreenBufferInfo\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ScreenBufferInfoRequest.ConsoleHandle, &Buff, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }
  pInfo = &Request->Data.ScreenBufferInfoRequest.Info;
  pInfo->dwSize.X = Buff->MaxX;
  pInfo->dwSize.Y = Buff->MaxY;
  pInfo->dwCursorPosition.X = Buff->CurrentX;
  pInfo->dwCursorPosition.Y = Buff->CurrentY;
  pInfo->wAttributes = Buff->DefaultAttrib;
  pInfo->srWindow.Left = Buff->ShowX;
  pInfo->srWindow.Right = Buff->ShowX + Console->Size.X - 1;
  pInfo->srWindow.Top = Buff->ShowY;
  pInfo->srWindow.Bottom = Buff->ShowY + Console->Size.Y - 1;
  pInfo->dwMaximumWindowSize.X = Buff->MaxX;
  pInfo->dwMaximumWindowSize.Y = Buff->MaxY;
  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  return STATUS_SUCCESS;
}

CSR_API(CsrSetCursor)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  LONG OldCursorX, OldCursorY;
  LONG NewCursorX, NewCursorY;

  DPRINT("CsrSetCursor\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }

  NewCursorX = Request->Data.SetCursorRequest.Position.X;
  NewCursorY = Request->Data.SetCursorRequest.Position.Y;
  if (NewCursorX < 0 || NewCursorX >= Buff->MaxX ||
      NewCursorY < 0 || NewCursorY >= Buff->MaxY)
    {
      ConioUnlockScreenBuffer(Buff);
      ConioUnlockConsole(Console);
      return STATUS_INVALID_PARAMETER;
    }
  OldCursorX = Buff->CurrentX;
  OldCursorY = Buff->CurrentY;
  Buff->CurrentX = NewCursorX;
  Buff->CurrentY = NewCursorY;
  if (Buff == Console->ActiveBuffer)
    {
      if (! ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY))
        {
          ConioUnlockScreenBuffer(Buff);
          ConioUnlockConsole(Console);
          return STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  return STATUS_SUCCESS;
}

static VOID FASTCALL
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
  PCHAR String, tmpString = NULL;
  PBYTE Buffer;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD X, Y, Length, CharSize, Written = 0;
  RECT UpdateRect;

  DPRINT("CsrWriteConsoleOutputChar\n");

  CharSize = (Request->Data.WriteConsoleOutputCharRequest.Unicode ? sizeof(WCHAR) : sizeof(CHAR));

  if (Request->Header.u1.s1.TotalLength
      < CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_CHAR)
        + (Request->Data.WriteConsoleOutputCharRequest.Length * CharSize))
    {
      DPRINT1("Invalid request size\n");
      Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
      Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
      return STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if (NT_SUCCESS(Status))
    {
      if(Request->Data.WriteConsoleOutputCharRequest.Unicode)
        {
          Length = WideCharToMultiByte(Console->OutputCodePage, 0,
                                      (PWCHAR)Request->Data.WriteConsoleOutputCharRequest.String,
                                       Request->Data.WriteConsoleOutputCharRequest.Length,
                                       NULL, 0, NULL, NULL);
          tmpString = String = RtlAllocateHeap(GetProcessHeap(), 0, Length);
          if (String)
            {
              WideCharToMultiByte(Console->OutputCodePage, 0,
                                  (PWCHAR)Request->Data.WriteConsoleOutputCharRequest.String,
                                  Request->Data.WriteConsoleOutputCharRequest.Length,
                                  String, Length, NULL, NULL);
            }
          else
            {
              Status = STATUS_NO_MEMORY;
            }
        }
      else
        {
          String = (PCHAR)Request->Data.WriteConsoleOutputCharRequest.String;
        }

      if (String)
        {
          Status = ConioLockScreenBuffer(ProcessData,
                                         Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle,
                                         &Buff,
                                         GENERIC_WRITE);
          if (NT_SUCCESS(Status))
            {
              X = Request->Data.WriteConsoleOutputCharRequest.Coord.X;
              Y = (Request->Data.WriteConsoleOutputCharRequest.Coord.Y + Buff->VirtualY) % Buff->MaxY;
              Length = Request->Data.WriteConsoleOutputCharRequest.Length;
              Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X)];
              while (Length--)
                {
                  *Buffer = *String++;
                  Written++;
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
              if (Buff == Console->ActiveBuffer)
                {
                  ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.WriteConsoleOutputCharRequest.Coord,
                                         Request->Data.WriteConsoleOutputCharRequest.Length);
                  ConioDrawRegion(Console, &UpdateRect);
                }

                Request->Data.WriteConsoleOutputCharRequest.EndCoord.X = X;
                Request->Data.WriteConsoleOutputCharRequest.EndCoord.Y = (Y + Buff->MaxY - Buff->VirtualY) % Buff->MaxY;

                ConioUnlockScreenBuffer(Buff);
            }
          if (Request->Data.WriteConsoleRequest.Unicode)
            {
              RtlFreeHeap(GetProcessHeap(), 0, tmpString);
            }
        }
      ConioUnlockConsole(Console);
    }
  Request->Data.WriteConsoleOutputCharRequest.NrCharactersWritten = Written;
  return Status;
}

CSR_API(CsrFillOutputChar)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD X, Y, Length, Written = 0;
  CHAR Char;
  PBYTE Buffer;
  RECT UpdateRect;

  DPRINT("CsrFillOutputChar\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.FillOutputRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }

  X = Request->Data.FillOutputRequest.Position.X;
  Y = (Request->Data.FillOutputRequest.Position.Y + Buff->VirtualY) % Buff->MaxY;
  Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X)];
  if(Request->Data.FillOutputRequest.Unicode)
    ConsoleUnicodeCharToAnsiChar(Console, &Char, &Request->Data.FillOutputRequest.Char.UnicodeChar);
  else
    Char = Request->Data.FillOutputRequest.Char.AsciiChar;
  Length = Request->Data.FillOutputRequest.Length;
  while (Length--)
    {
      *Buffer = Char;
      Buffer += 2;
      Written++;
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

  if (Buff == Console->ActiveBuffer)
    {
      ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.FillOutputRequest.Position,
                             Request->Data.FillOutputRequest.Length);
      ConioDrawRegion(Console, &UpdateRect);
    }

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);
  Length = Request->Data.FillOutputRequest.Length;
  Request->Data.FillOutputRequest.NrCharactersWritten = Length;
  return STATUS_SUCCESS;
}

CSR_API(CsrReadInputEvent)
{
  PLIST_ENTRY CurrentEntry;
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;
  BOOLEAN Done = FALSE;
  ConsoleInput *Input;

  DPRINT("CsrReadInputEvent\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Request->Data.ReadInputRequest.Event = ProcessData->ConsoleEvent;

  Status = ConioLockConsole(ProcessData, Request->Data.ReadInputRequest.ConsoleHandle, &Console, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  /* only get input if there is any */
  CurrentEntry = Console->InputEvents.Flink;
  while (CurrentEntry != &Console->InputEvents)
    {
      Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
      CurrentEntry = CurrentEntry->Flink;

      if (Done && !Input->Fake)
        {
          Request->Data.ReadInputRequest.MoreEvents = TRUE;
          break;
        }

      RemoveEntryList(&Input->ListEntry);

      if (!Done && !Input->Fake)
        {
          Request->Data.ReadInputRequest.Input = Input->InputEvent;
          if (Request->Data.ReadInputRequest.Unicode == FALSE)
            {
              ConioInputEventToAnsi(Console, &Request->Data.ReadInputRequest.Input);
            }
          Done = TRUE;
        }

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
    }

  if (Done)
    {
      Status = STATUS_SUCCESS;
      Console->EarlyReturn = FALSE;
    }
  else
    {
      Status = STATUS_PENDING;
      Console->EarlyReturn = TRUE;  /* mark for early return */
    }

  if (IsListEmpty(&Console->InputEvents))
    {
      ResetEvent(Console->ActiveEvent);
    }

  ConioUnlockConsole(Console);

  return Status;
}

CSR_API(CsrWriteConsoleOutputAttrib)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  PUCHAR Buffer;
  PWORD Attribute;
  int X, Y, Length;
  NTSTATUS Status;
  RECT UpdateRect;

  DPRINT("CsrWriteConsoleOutputAttrib\n");

  if (Request->Header.u1.s1.TotalLength
      < CSR_API_MESSAGE_HEADER_SIZE(CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB)
        + Request->Data.WriteConsoleOutputAttribRequest.Length * sizeof(WORD))
    {
      DPRINT1("Invalid request size\n");
      Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
      Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
      return STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Status = ConioLockScreenBuffer(ProcessData,
                                 Request->Data.WriteConsoleOutputAttribRequest.ConsoleHandle,
                                 &Buff,
                                 GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }

  X = Request->Data.WriteConsoleOutputAttribRequest.Coord.X;
  Y = (Request->Data.WriteConsoleOutputAttribRequest.Coord.Y + Buff->VirtualY) % Buff->MaxY;
  Length = Request->Data.WriteConsoleOutputAttribRequest.Length;
  Buffer = &Buff->Buffer[2 * (Y * Buff->MaxX + X) + 1];
  Attribute = Request->Data.WriteConsoleOutputAttribRequest.Attribute;
  while (Length--)
    {
      *Buffer = (UCHAR)(*Attribute++);
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

  if (Buff == Console->ActiveBuffer)
    {
      ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.WriteConsoleOutputAttribRequest.Coord,
                             Request->Data.WriteConsoleOutputAttribRequest.Length);
      ConioDrawRegion(Console, &UpdateRect);
    }

  ConioUnlockConsole(Console);

  Request->Data.WriteConsoleOutputAttribRequest.EndCoord.X = X;
  Request->Data.WriteConsoleOutputAttribRequest.EndCoord.Y = (Y + Buff->MaxY - Buff->VirtualY) % Buff->MaxY;

  ConioUnlockScreenBuffer(Buff);

  return STATUS_SUCCESS;
}

CSR_API(CsrFillOutputAttrib)
{
  PCSRSS_SCREEN_BUFFER Buff;
  PUCHAR Buffer;
  NTSTATUS Status;
  int X, Y, Length;
  UCHAR Attr;
  RECT UpdateRect;
  PCSRSS_CONSOLE Console;

  DPRINT("CsrFillOutputAttrib\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData, Request->Data.FillOutputAttribRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }

  X = Request->Data.FillOutputAttribRequest.Coord.X;
  Y = (Request->Data.FillOutputAttribRequest.Coord.Y + Buff->VirtualY) % Buff->MaxY;
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

  if (Buff == Console->ActiveBuffer)
    {
      ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.FillOutputAttribRequest.Coord,
                             Request->Data.FillOutputAttribRequest.Length);
      ConioDrawRegion(Console, &UpdateRect);
    }

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  return STATUS_SUCCESS;
}


CSR_API(CsrGetCursorInfo)
{
  PCSRSS_SCREEN_BUFFER Buff;
  NTSTATUS Status;

  DPRINT("CsrGetCursorInfo\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.GetCursorInfoRequest.ConsoleHandle, &Buff, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  Request->Data.GetCursorInfoRequest.Info.bVisible = Buff->CursorInfo.bVisible;
  Request->Data.GetCursorInfoRequest.Info.dwSize = Buff->CursorInfo.dwSize;
  ConioUnlockScreenBuffer(Buff);

  return STATUS_SUCCESS;
}

CSR_API(CsrSetCursorInfo)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD Size;
  BOOL Visible;
  NTSTATUS Status;

  DPRINT("CsrSetCursorInfo\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorInfoRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
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

      if (! ConioSetCursorInfo(Console, Buff))
        {
          ConioUnlockScreenBuffer(Buff);
          ConioUnlockConsole(Console);
          return STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  return STATUS_SUCCESS;
}

CSR_API(CsrSetTextAttrib)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;

  DPRINT("CsrSetTextAttrib\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }

  Buff->DefaultAttrib = Request->Data.SetAttribRequest.Attrib;
  if (Buff == Console->ActiveBuffer)
    {
      if (! ConioUpdateScreenInfo(Console, Buff))
        {
          ConioUnlockScreenBuffer(Buff);
          ConioUnlockConsole(Console);
          return STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  return STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleMode)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;

  DPRINT("CsrSetConsoleMode\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = Win32CsrGetObject(ProcessData,
                             Request->Data.SetConsoleModeRequest.ConsoleHandle,
                             (Object_t **) &Console, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      return Status;
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
      Status = STATUS_INVALID_HANDLE;
    }

  Win32CsrReleaseObjectByPointer((Object_t *)Console);

  return Status;
}

CSR_API(CsrGetConsoleMode)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;   /* gee, I really wish I could use an anonymous union here */

  DPRINT("CsrGetConsoleMode\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = Win32CsrGetObject(ProcessData, Request->Data.GetConsoleModeRequest.ConsoleHandle,
                             (Object_t **) &Console, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }
  Status = STATUS_SUCCESS;
  Buff = (PCSRSS_SCREEN_BUFFER) Console;
  if (CONIO_CONSOLE_MAGIC == Console->Header.Type)
    {
      Request->Data.GetConsoleModeRequest.ConsoleMode = Console->Mode;
    }
  else if (CONIO_SCREEN_BUFFER_MAGIC == Buff->Header.Type)
    {
      Request->Data.GetConsoleModeRequest.ConsoleMode = Buff->Mode;
    }
  else
    {
      Status = STATUS_INVALID_HANDLE;
    }

  Win32CsrReleaseObjectByPointer((Object_t *)Console);
  return Status;
}

CSR_API(CsrCreateScreenBuffer)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  NTSTATUS Status;

  DPRINT("CsrCreateScreenBuffer\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Buff = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, sizeof(CSRSS_SCREEN_BUFFER));

  if (Buff != NULL)
    {
      if (Console->ActiveBuffer)
        {
          Buff->MaxX = Console->ActiveBuffer->MaxX;
          Buff->MaxY = Console->ActiveBuffer->MaxY;
          Buff->CursorInfo.bVisible = Console->ActiveBuffer->CursorInfo.bVisible;
          Buff->CursorInfo.dwSize = Console->ActiveBuffer->CursorInfo.dwSize;
        }
      else
        {
          Buff->CursorInfo.bVisible = TRUE;
          Buff->CursorInfo.dwSize = 5;
        }

      if (Buff->MaxX == 0)
        {
          Buff->MaxX = 80;
        }

      if (Buff->MaxY == 0)
        {
          Buff->MaxY = 25;
        }

      Status = CsrInitConsoleScreenBuffer(Console, Buff);
      if(NT_SUCCESS(Status))
        {
          Status = Win32CsrInsertObject(ProcessData,
            &Request->Data.CreateScreenBufferRequest.OutputHandle,
            &Buff->Header,
            Request->Data.CreateScreenBufferRequest.Access,
            Request->Data.CreateScreenBufferRequest.Inheritable);
        }
    }
  else
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }

  ConioUnlockConsole(Console);
  return Status;
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
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetScreenBufferRequest.OutputHandle, &Buff, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
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

  return STATUS_SUCCESS;
}

CSR_API(CsrSetTitle)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PWCHAR Buffer;

  DPRINT("CsrSetTitle\n");

  if (Request->Header.u1.s1.TotalLength
      < CSR_API_MESSAGE_HEADER_SIZE(CSRSS_SET_TITLE)
        + Request->Data.SetTitleRequest.Length)
    {
      DPRINT1("Invalid request size\n");
      Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
      Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
      return STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if(NT_SUCCESS(Status))
    {
      Buffer =  RtlAllocateHeap(RtlGetProcessHeap(), 0, Request->Data.SetTitleRequest.Length);
      if (Buffer)
        {
          /* copy title to console */
          RtlFreeUnicodeString(&Console->Title);
          Console->Title.Buffer = Buffer;
          Console->Title.Length = Console->Title.MaximumLength = Request->Data.SetTitleRequest.Length;
          memcpy(Console->Title.Buffer, Request->Data.SetTitleRequest.Title, Console->Title.Length);
          if (! ConioChangeTitle(Console))
            {
              Status = STATUS_UNSUCCESSFUL;
            }
          else
            {
              Status = STATUS_SUCCESS;
            }
        }
      else
        {
          Status = STATUS_NO_MEMORY;
        }
      ConioUnlockConsole(Console);
    }

  return Status;
}

CSR_API(CsrGetTitle)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  DWORD Length;

  DPRINT("CsrGetTitle\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Can't get console\n");
      return Status;
    }

  /* Copy title of the console to the user title buffer */
  RtlZeroMemory(&Request->Data.GetTitleRequest, sizeof(CSRSS_GET_TITLE));
  Request->Data.GetTitleRequest.Length = Console->Title.Length;
  memcpy (Request->Data.GetTitleRequest.Title, Console->Title.Buffer,
          Console->Title.Length);
  Length = CSR_API_MESSAGE_HEADER_SIZE(CSRSS_SET_TITLE) + Console->Title.Length;

  ConioUnlockConsole(Console);

  if (Length > sizeof(CSR_API_MESSAGE))
    {
      Request->Header.u1.s1.TotalLength = Length;
      Request->Header.u1.s1.DataLength = Length - sizeof(PORT_MESSAGE);
    }
  return STATUS_SUCCESS;
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
  PBYTE Ptr;
  DWORD PSize;

  DPRINT("CsrWriteConsoleOutput\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData,
                                 Request->Data.WriteConsoleOutputRequest.ConsoleHandle,
                                 &Buff,
                                 GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }

  BufferSize = Request->Data.WriteConsoleOutputRequest.BufferSize;
  PSize = BufferSize.X * BufferSize.Y * sizeof(CHAR_INFO);
  BufferCoord = Request->Data.WriteConsoleOutputRequest.BufferCoord;
  CharInfo = Request->Data.WriteConsoleOutputRequest.CharInfo;
  if (((PVOID)CharInfo < ProcessData->CsrSectionViewBase) ||
      (((ULONG_PTR)CharInfo + PSize) >
       ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockScreenBuffer(Buff);
      ConioUnlockConsole(Console);
      return STATUS_ACCESS_VIOLATION;
    }
  WriteRegion.left = Request->Data.WriteConsoleOutputRequest.WriteRegion.Left;
  WriteRegion.top = Request->Data.WriteConsoleOutputRequest.WriteRegion.Top;
  WriteRegion.right = Request->Data.WriteConsoleOutputRequest.WriteRegion.Right;
  WriteRegion.bottom = Request->Data.WriteConsoleOutputRequest.WriteRegion.Bottom;

  SizeY = min(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&WriteRegion));
  SizeX = min(BufferSize.X - BufferCoord.X, ConioRectWidth(&WriteRegion));
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
      return STATUS_SUCCESS;
    }

  for (i = 0, Y = WriteRegion.top; Y <= WriteRegion.bottom; i++, Y++)
    {
      CurCharInfo = CharInfo + (i + BufferCoord.Y) * BufferSize.X + BufferCoord.X;
      Ptr = ConioCoordToPointer(Buff, WriteRegion.left, Y);
      for (X = WriteRegion.left; X <= WriteRegion.right; X++)
        {
          CHAR AsciiChar;
          if (Request->Data.WriteConsoleOutputRequest.Unicode)
            {
              ConsoleUnicodeCharToAnsiChar(Console, &AsciiChar, &CurCharInfo->Char.UnicodeChar);
            }
          else
            {
              AsciiChar = CurCharInfo->Char.AsciiChar;
            }
          *Ptr++ = AsciiChar;
          *Ptr++ = CurCharInfo->Attributes;
          CurCharInfo++;
        }
    }

  ConioDrawRegion(Console, &WriteRegion);

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  Request->Data.WriteConsoleOutputRequest.WriteRegion.Right = WriteRegion.left + SizeX - 1;
  Request->Data.WriteConsoleOutputRequest.WriteRegion.Bottom = WriteRegion.top + SizeY - 1;
  Request->Data.WriteConsoleOutputRequest.WriteRegion.Left = WriteRegion.left;
  Request->Data.WriteConsoleOutputRequest.WriteRegion.Top = WriteRegion.top;

  return STATUS_SUCCESS;
}

CSR_API(CsrFlushInputBuffer)
{
  PLIST_ENTRY CurrentEntry;
  PCSRSS_CONSOLE Console;
  ConsoleInput* Input;
  NTSTATUS Status;

  DPRINT("CsrFlushInputBuffer\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioLockConsole(ProcessData,
                            Request->Data.FlushInputBufferRequest.ConsoleInput,
                            &Console,
                            GENERIC_WRITE);
  if(! NT_SUCCESS(Status))
    {
      return Status;
    }

  /* Discard all entries in the input event queue */
  while (!IsListEmpty(&Console->InputEvents))
    {
      CurrentEntry = RemoveHeadList(&Console->InputEvents);
      Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
      /* Destroy the event */
      HeapFree(Win32CsrApiHeap, 0, Input);
    }
  ResetEvent(Console->ActiveEvent);
  Console->WaitingChars=0;

  ConioUnlockConsole(Console);

  return STATUS_SUCCESS;
}

CSR_API(CsrScrollConsoleScreenBuffer)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  RECT ScreenBuffer;
  RECT SrcRegion;
  RECT DstRegion;
  RECT UpdateRegion;
  RECT ScrollRectangle;
  RECT ClipRectangle;
  NTSTATUS Status;
  HANDLE ConsoleHandle;
  BOOLEAN UseClipRectangle;
  COORD DestinationOrigin;
  CHAR_INFO Fill;
  CHAR FillChar;

  DPRINT("CsrScrollConsoleScreenBuffer\n");

  ConsoleHandle = Request->Data.ScrollConsoleScreenBufferRequest.ConsoleHandle;
  UseClipRectangle = Request->Data.ScrollConsoleScreenBufferRequest.UseClipRectangle;
  DestinationOrigin = Request->Data.ScrollConsoleScreenBufferRequest.DestinationOrigin;
  Fill = Request->Data.ScrollConsoleScreenBufferRequest.Fill;

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData, ConsoleHandle, &Buff, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }

  ScrollRectangle.left = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle.Left;
  ScrollRectangle.top = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle.Top;
  ScrollRectangle.right = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle.Right;
  ScrollRectangle.bottom = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle.Bottom;

  /* Make sure source rectangle is inside the screen buffer */
  ConioInitRect(&ScreenBuffer, 0, 0, Buff->MaxY - 1, Buff->MaxX - 1);
  if (! ConioGetIntersection(&SrcRegion, &ScreenBuffer, &ScrollRectangle))
    {
      ConioUnlockScreenBuffer(Buff);
      ConioUnlockConsole(Console);
      return STATUS_SUCCESS;
    }

  /* If the source was clipped on the left or top, adjust the destination accordingly */
  if (ScrollRectangle.left < 0)
    {
      DestinationOrigin.X -= ScrollRectangle.left;
    }
  if (ScrollRectangle.top < 0)
    {
      DestinationOrigin.Y -= ScrollRectangle.top;
    }

  if (UseClipRectangle)
    {
      ClipRectangle.left = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle.Left;
      ClipRectangle.top = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle.Top;
      ClipRectangle.right = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle.Right;
      ClipRectangle.bottom = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle.Bottom;
      if (!ConioGetIntersection(&ClipRectangle, &ClipRectangle, &ScreenBuffer))
        {
          ConioUnlockConsole(Console);
          ConioUnlockScreenBuffer(Buff);
          return STATUS_SUCCESS;
      }
    }
  else
    {
      ClipRectangle = ScreenBuffer;
    }

  ConioInitRect(&DstRegion,
               DestinationOrigin.Y,
               DestinationOrigin.X,
               DestinationOrigin.Y + ConioRectHeight(&SrcRegion) - 1,
               DestinationOrigin.X + ConioRectWidth(&SrcRegion) - 1);

  if (Request->Data.ScrollConsoleScreenBufferRequest.Unicode)
    ConsoleUnicodeCharToAnsiChar(Console, &FillChar, &Fill.Char.UnicodeChar);
  else
    FillChar = Fill.Char.AsciiChar;

  ConioMoveRegion(Buff, &SrcRegion, &DstRegion, &ClipRectangle, Fill.Attributes << 8 | (BYTE)FillChar);

  if (Buff == Console->ActiveBuffer)
    {
      ConioGetUnion(&UpdateRegion, &SrcRegion, &DstRegion);
      if (ConioGetIntersection(&UpdateRegion, &UpdateRegion, &ClipRectangle))
        {
          /* Draw update region */
          ConioDrawRegion(Console, &UpdateRegion);
        }
    }

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  return STATUS_SUCCESS;
}

CSR_API(CsrReadConsoleOutputChar)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD Xpos, Ypos;
  PCHAR ReadBuffer;
  DWORD i;
  ULONG CharSize;
  CHAR Char;

  DPRINT("CsrReadConsoleOutputChar\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = Request->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);
  ReadBuffer = Request->Data.ReadConsoleOutputCharRequest.String;

  CharSize = (Request->Data.ReadConsoleOutputCharRequest.Unicode ? sizeof(WCHAR) : sizeof(CHAR));

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputCharRequest.ConsoleHandle, &Buff, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Status;
    }

  Xpos = Request->Data.ReadConsoleOutputCharRequest.ReadCoord.X;
  Ypos = (Request->Data.ReadConsoleOutputCharRequest.ReadCoord.Y + Buff->VirtualY) % Buff->MaxY;

  for (i = 0; i < Request->Data.ReadConsoleOutputCharRequest.NumCharsToRead; ++i)
    {
      Char = Buff->Buffer[(Xpos * 2) + (Ypos * 2 * Buff->MaxX)];

      if(Request->Data.ReadConsoleOutputCharRequest.Unicode)
      {
        ConsoleAnsiCharToUnicodeChar(Console, (WCHAR*)ReadBuffer, &Char);
        ReadBuffer += sizeof(WCHAR);
      }
      else
        *(ReadBuffer++) = Char;

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
  Request->Data.ReadConsoleOutputCharRequest.EndCoord.X = Xpos;
  Request->Data.ReadConsoleOutputCharRequest.EndCoord.Y = (Ypos - Buff->VirtualY + Buff->MaxY) % Buff->MaxY;

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  Request->Data.ReadConsoleOutputCharRequest.CharsRead = (DWORD)((ULONG_PTR)ReadBuffer - (ULONG_PTR)Request->Data.ReadConsoleOutputCharRequest.String) / CharSize;
  if (Request->Data.ReadConsoleOutputCharRequest.CharsRead * CharSize + CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR) > sizeof(CSR_API_MESSAGE))
    {
      Request->Header.u1.s1.TotalLength = Request->Data.ReadConsoleOutputCharRequest.CharsRead * CharSize + CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR);
      Request->Header.u1.s1.DataLength = Request->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);
    }

  return STATUS_SUCCESS;
}


CSR_API(CsrReadConsoleOutputAttrib)
{
  NTSTATUS Status;
  PCSRSS_SCREEN_BUFFER Buff;
  DWORD Xpos, Ypos;
  PWORD ReadBuffer;
  DWORD i;
  DWORD CurrentLength;

  DPRINT("CsrReadConsoleOutputAttrib\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = Request->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);
  ReadBuffer = Request->Data.ReadConsoleOutputAttribRequest.Attribute;

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputAttribRequest.ConsoleHandle, &Buff, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Xpos = Request->Data.ReadConsoleOutputAttribRequest.ReadCoord.X;
  Ypos = (Request->Data.ReadConsoleOutputAttribRequest.ReadCoord.Y + Buff->VirtualY) % Buff->MaxY;

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

  Request->Data.ReadConsoleOutputAttribRequest.EndCoord.X = Xpos;
  Request->Data.ReadConsoleOutputAttribRequest.EndCoord.Y = (Ypos - Buff->VirtualY + Buff->MaxY) % Buff->MaxY;

  ConioUnlockScreenBuffer(Buff);

  CurrentLength = CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_ATTRIB)
                     + Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead * sizeof(WORD);
  if (CurrentLength > sizeof(CSR_API_MESSAGE))
    {
      Request->Header.u1.s1.TotalLength = CurrentLength;
      Request->Header.u1.s1.DataLength = CurrentLength - sizeof(PORT_MESSAGE);
    }

  return STATUS_SUCCESS;
}


CSR_API(CsrGetNumberOfConsoleInputEvents)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  PLIST_ENTRY CurrentItem;
  DWORD NumEvents;
  ConsoleInput *Input;

  DPRINT("CsrGetNumberOfConsoleInputEvents\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = Request->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);

  Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  CurrentItem = Console->InputEvents.Flink;
  NumEvents = 0;

  /* If there are any events ... */
  while (CurrentItem != &Console->InputEvents)
    {
      Input = CONTAINING_RECORD(CurrentItem, ConsoleInput, ListEntry);
      CurrentItem = CurrentItem->Flink;
      if (!Input->Fake)
        {
          NumEvents++;
        }
    }

  ConioUnlockConsole(Console);

  Request->Data.GetNumInputEventsRequest.NumInputEvents = NumEvents;

  return STATUS_SUCCESS;
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

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console, GENERIC_READ);
  if(! NT_SUCCESS(Status))
    {
      return Status;
    }

  InputRecord = Request->Data.PeekConsoleInputRequest.InputRecord;
  Length = Request->Data.PeekConsoleInputRequest.Length;
  Size = Length * sizeof(INPUT_RECORD);

  if (((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
      || (((ULONG_PTR)InputRecord + Size) > ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockConsole(Console);
      return STATUS_ACCESS_VIOLATION;
    }

  NumItems = 0;

  if (! IsListEmpty(&Console->InputEvents))
    {
      CurrentItem = Console->InputEvents.Flink;

      while (CurrentItem != &Console->InputEvents && NumItems < Length)
        {
          Item = CONTAINING_RECORD(CurrentItem, ConsoleInput, ListEntry);

          if (Item->Fake)
            {
              CurrentItem = CurrentItem->Flink;
              continue;
            }

          ++NumItems;
          *InputRecord = Item->InputEvent;

          if (Request->Data.ReadInputRequest.Unicode == FALSE)
            {
              ConioInputEventToAnsi(Console, InputRecord);
            }

          InputRecord++;
          CurrentItem = CurrentItem->Flink;
        }
    }

  ConioUnlockConsole(Console);

  Request->Data.PeekConsoleInputRequest.Length = NumItems;

  return STATUS_SUCCESS;
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
  DWORD i;
  PBYTE Ptr;
  LONG X, Y;
  UINT CodePage;

  DPRINT("CsrReadConsoleOutput\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputRequest.ConsoleHandle, &Buff, GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      return Status;
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

  /* FIXME: Is this correct? */
  CodePage = ProcessData->Console->OutputCodePage;

  if (((PVOID)CharInfo < ProcessData->CsrSectionViewBase)
      || (((ULONG_PTR)CharInfo + Size) > ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockScreenBuffer(Buff);
      return STATUS_ACCESS_VIOLATION;
    }

  SizeY = min(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&ReadRegion));
  SizeX = min(BufferSize.X - BufferCoord.X, ConioRectWidth(&ReadRegion));
  ReadRegion.bottom = ReadRegion.top + SizeY;
  ReadRegion.right = ReadRegion.left + SizeX;

  ConioInitRect(&ScreenRect, 0, 0, Buff->MaxY, Buff->MaxX);
  if (! ConioGetIntersection(&ReadRegion, &ScreenRect, &ReadRegion))
    {
      ConioUnlockScreenBuffer(Buff);
      return STATUS_SUCCESS;
    }

  for (i = 0, Y = ReadRegion.top; Y < ReadRegion.bottom; ++i, ++Y)
    {
      CurCharInfo = CharInfo + (i * BufferSize.X);

      Ptr = ConioCoordToPointer(Buff, ReadRegion.left, Y);
      for (X = ReadRegion.left; X < ReadRegion.right; ++X)
        {
          if (Request->Data.ReadConsoleOutputRequest.Unicode)
            {
              MultiByteToWideChar(CodePage, 0,
                                  (PCHAR)Ptr++, 1,
                                  &CurCharInfo->Char.UnicodeChar, 1);
            }
          else
            {
              CurCharInfo->Char.AsciiChar = *Ptr++;
            }
          CurCharInfo->Attributes = *Ptr++;
          ++CurCharInfo;
        }
    }

  ConioUnlockScreenBuffer(Buff);

  Request->Data.ReadConsoleOutputRequest.ReadRegion.Right = ReadRegion.left + SizeX - 1;
  Request->Data.ReadConsoleOutputRequest.ReadRegion.Bottom = ReadRegion.top + SizeY - 1;
  Request->Data.ReadConsoleOutputRequest.ReadRegion.Left = ReadRegion.left;
  Request->Data.ReadConsoleOutputRequest.ReadRegion.Top = ReadRegion.top;

  return STATUS_SUCCESS;
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

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockConsole(ProcessData, Request->Data.WriteConsoleInputRequest.ConsoleHandle, &Console, GENERIC_WRITE);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  InputRecord = Request->Data.WriteConsoleInputRequest.InputRecord;
  Length = Request->Data.WriteConsoleInputRequest.Length;
  Size = Length * sizeof(INPUT_RECORD);

  if (((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
      || (((ULONG_PTR)InputRecord + Size) > ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockConsole(Console);
      return STATUS_ACCESS_VIOLATION;
    }

  for (i = 0; i < Length; i++)
    {
      Record = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));
      if (NULL == Record)
        {
          ConioUnlockConsole(Console);
          return STATUS_INSUFFICIENT_RESOURCES;
        }

      Record->Echoed = FALSE;
      Record->Fake = FALSE;
      //Record->InputEvent = *InputRecord++;
      memcpy(&Record->InputEvent, &InputRecord[i], sizeof(INPUT_RECORD));
      if (KEY_EVENT == Record->InputEvent.EventType)
        {
          /* FIXME - convert from unicode to ascii!! */
          ConioProcessChar(Console, Record);
        }
    }

  ConioUnlockConsole(Console);

  Request->Data.WriteConsoleInputRequest.Length = i;

  return STATUS_SUCCESS;
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
 *		object. We use the same object to Request.
 *	NOTE
 *		ConsoleHwState has the correct size to be compatible
 *		with NT's, but values are not.
 */
static NTSTATUS FASTCALL
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

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockConsole(ProcessData,
                            Request->Data.ConsoleHardwareStateRequest.ConsoleHandle,
                            &Console,
                            GENERIC_READ);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed to get console handle in SetConsoleHardwareState\n");
      return Status;
    }

  switch (Request->Data.ConsoleHardwareStateRequest.SetGet)
    {
      case CONSOLE_HARDWARE_STATE_GET:
        Request->Data.ConsoleHardwareStateRequest.State = Console->HardwareState;
        break;

      case CONSOLE_HARDWARE_STATE_SET:
        DPRINT("Setting console hardware state.\n");
        Status = SetConsoleHardwareState(Console, Request->Data.ConsoleHardwareStateRequest.State);
        break;

      default:
        Status = STATUS_INVALID_PARAMETER_2; /* Client: (handle, [set_get], mode) */
        break;
    }

  ConioUnlockConsole(Console);

  return Status;
}

CSR_API(CsrGetConsoleWindow)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrGetConsoleWindow\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Data.GetConsoleWindowRequest.WindowHandle = Console->hWindow;
  ConioUnlockConsole(Console);

  return STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleIcon)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrSetConsoleIcon\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Status = (ConioChangeIcon(Console, Request->Data.SetConsoleIconRequest.WindowIcon)
            ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
  ConioUnlockConsole(Console);

  return Status;
}

CSR_API(CsrGetConsoleCodePage)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrGetConsoleCodePage\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Request->Data.GetConsoleCodePage.CodePage = Console->CodePage;
  ConioUnlockConsole(Console);
  return STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleCodePage)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrSetConsoleCodePage\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if (IsValidCodePage(Request->Data.SetConsoleCodePage.CodePage))
    {
      Console->CodePage = Request->Data.SetConsoleCodePage.CodePage;
      ConioUnlockConsole(Console);
      return STATUS_SUCCESS;
    }
  ConioUnlockConsole(Console);
  return STATUS_UNSUCCESSFUL;
}

CSR_API(CsrGetConsoleOutputCodePage)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrGetConsoleOutputCodePage\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Request->Data.GetConsoleOutputCodePage.CodePage = Console->OutputCodePage;
  ConioUnlockConsole(Console);
  return STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleOutputCodePage)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrSetConsoleOutputCodePage\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if (IsValidCodePage(Request->Data.SetConsoleOutputCodePage.CodePage))
    {
      Console->OutputCodePage = Request->Data.SetConsoleOutputCodePage.CodePage;
      ConioUnlockConsole(Console);
      return STATUS_SUCCESS;
    }
  ConioUnlockConsole(Console);
  return STATUS_UNSUCCESSFUL;
}

CSR_API(CsrGetProcessList)
{
  PHANDLE Buffer;
  PCSRSS_CONSOLE Console;
  PCSRSS_PROCESS_DATA current;
  PLIST_ENTRY current_entry;
  ULONG nItems, nCopied, Length;
  NTSTATUS Status;

  DPRINT("CsrGetProcessList\n");

  Buffer = Request->Data.GetProcessListRequest.ProcessId;
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  nItems = nCopied = 0;
  Request->Data.GetProcessListRequest.nProcessIdsCopied = 0;
  Request->Data.GetProcessListRequest.nProcessIdsTotal = 0;

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
  {
    return Status;
  }

  DPRINT1("Console_Api Ctrl-C\n");

  for(current_entry = Console->ProcessList.Flink;
      current_entry != &Console->ProcessList;
      current_entry = current_entry->Flink)
  {
    current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
    if(++nItems < Request->Data.GetProcessListRequest.nMaxIds)
    {
      *(Buffer++) = current->ProcessId;
      nCopied++;
    }
  }

  ConioUnlockConsole(Console);

  Request->Data.GetProcessListRequest.nProcessIdsCopied = nCopied;
  Request->Data.GetProcessListRequest.nProcessIdsTotal = nItems;

  Length = CSR_API_MESSAGE_HEADER_SIZE(CSRSS_GET_PROCESS_LIST) + nCopied * sizeof(HANDLE);
  if (Length > sizeof(CSR_API_MESSAGE))
  {
     Request->Header.u1.s1.TotalLength = Length;
     Request->Header.u1.s1.DataLength = Length - sizeof(PORT_MESSAGE);
  }
  return STATUS_SUCCESS;
}

CSR_API(CsrGenerateCtrlEvent)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_PROCESS_DATA current;
  PLIST_ENTRY current_entry;
  DWORD Group;
  NTSTATUS Status;

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
  {
    return Status;
  }

  Group = Request->Data.GenerateCtrlEvent.ProcessGroup;
  Status = STATUS_INVALID_PARAMETER;
  for (current_entry = Console->ProcessList.Flink;
       current_entry != &Console->ProcessList;
       current_entry = current_entry->Flink)
  {
    current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
    if (Group == 0 || current->ProcessGroup == Group)
    {
      ConioConsoleCtrlEvent(Request->Data.GenerateCtrlEvent.Event, current);
      Status = STATUS_SUCCESS;
    }
  }

  ConioUnlockConsole(Console);

  return Status;
}

/* EOF */
