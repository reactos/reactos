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

extern NTSTATUS FASTCALL
Win32CsrInsertObject2(PCSRSS_PROCESS_DATA, PHANDLE, Object_t *);

/* GLOBALS *******************************************************************/

#define ConioInitRect(Rect, Top, Left, Bottom, Right) \
  ((Rect)->top) = Top; \
  ((Rect)->left) = Left; \
  ((Rect)->bottom) = Bottom; \
  ((Rect)->right) = Right

#define ConioIsRectEmpty(Rect) \
  (((Rect)->left > (Rect)->right) || ((Rect)->top > (Rect)->bottom))

#define ConsoleUnicodeCharToAnsiChar(Console, dChar, sWChar) \
  WideCharToMultiByte((Console)->CodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleAnsiCharToUnicodeChar(Console, sWChar, dChar) \
  MultiByteToWideChar((Console)->CodePage, 0, (dChar), 1, (sWChar), 1)


/* FUNCTIONS *****************************************************************/

static NTSTATUS FASTCALL
ConioConsoleFromProcessData(PCSRSS_PROCESS_DATA ProcessData, PCSRSS_CONSOLE *Console)
{
  PCSRSS_CONSOLE ProcessConsole = ProcessData->Console;

  if (!ProcessConsole)
    {
      *Console = NULL;
      return STATUS_SUCCESS;
    }

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
  ConioConsoleCtrlEventTimeout(Event, ProcessData, INFINITE);
}

#define GET_CELL_BUFFER(b,o)\
(b)->Buffer[(o)++]

#define SET_CELL_BUFFER(b,o,c,a)\
(b)->Buffer[(o)++]=(c),\
(b)->Buffer[(o)++]=(a)

static VOID FASTCALL
ClearLineBuffer(PCSRSS_SCREEN_BUFFER Buff)
{
  DWORD Offset = 2 * (Buff->CurrentY * Buff->MaxX);
  UINT Pos;

  for (Pos = 0; Pos < Buff->MaxX; Pos++)
    {
      /* Fill the cell: Offset is incremented by the macro */
      SET_CELL_BUFFER(Buff, Offset, ' ', Buff->DefaultAttrib);
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
  /* init screen buffer with defaults */
  NewBuffer->CursorInfo.bVisible = TRUE;
  NewBuffer->CursorInfo.dwSize = 5;
  /* make console active, and insert into console list */
  Console->ActiveBuffer = (PCSRSS_SCREEN_BUFFER) NewBuffer;
  /* add a reference count because the buffer is tied to the console */
  InterlockedIncrement(&Console->ActiveBuffer->Header.ReferenceCount);
  if (NULL == NewBuffer)
    {
      RtlFreeUnicodeString(&Console->Title);
      DeleteCriticalSection(&Console->Header.Lock);
      CloseHandle(Console->ActiveEvent);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

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

    if (ProcessData == NULL)
    {
        DPRINT1("No process data\n");
        return Request->Status = STATUS_INVALID_PARAMETER;
    }

    if (ProcessData->Console)
    {
        DPRINT1("Process already has a console\n");
        Request->Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    /* Assume success */
    Request->Status = STATUS_SUCCESS;

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
            Request->Status = STATUS_NO_MEMORY;
            return STATUS_NO_MEMORY;
        }
        /* initialize list head */
        InitializeListHead(&Console->ProcessList);
        /* insert process data required for GUI initialization */
        InsertHeadList(&Console->ProcessList, &ProcessData->ProcessEntry);
        /* Initialize the Console */
        Request->Status = CsrInitConsole(Console);
        if (!NT_SUCCESS(Request->Status))
        {
            DPRINT1("Console init failed\n");
            HeapFree(Win32CsrApiHeap, 0, Console);
            return Request->Status;
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
        Status = Win32CsrInsertObject2(ProcessData,
                                       &Request->Data.AllocConsoleRequest.InputHandle,
                                       &Console->Header);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert object\n");
            ConioDeleteConsole((Object_t *) Console);
            ProcessData->Console = 0;
            return Request->Status = Status;
        }

        Status = Win32CsrInsertObject2(ProcessData,
                                       &Request->Data.AllocConsoleRequest.OutputHandle,
                                       &Console->ActiveBuffer->Header);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to insert object\n");
            ConioDeleteConsole((Object_t *) Console);
            Win32CsrReleaseObject(ProcessData,
                                  Request->Data.AllocConsoleRequest.InputHandle);
            ProcessData->Console = 0;
            return Request->Status = Status;
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
        return Request->Status = Status;
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

  if (ProcessData == NULL || ProcessData->Console == NULL)
    {
      return Request->Status = STATUS_INVALID_PARAMETER;
    }

  Console = ProcessData->Console;
  ProcessData->Console = NULL;
  if (0 == InterlockedDecrement(&Console->Header.ReferenceCount))
    {
      ConioDeleteConsole((Object_t *) Console);
    }
  return STATUS_SUCCESS;
}

static VOID FASTCALL
ConioNextLine(PCSRSS_SCREEN_BUFFER Buff, RECT *UpdateRect, UINT *ScrolledLines)
{
  /* slide the viewable screen */
  if (((Buff->CurrentY - Buff->ShowY + Buff->MaxY) % Buff->MaxY) == (ULONG)Buff->MaxY - 1)
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
  if (UpdateRect->top == (LONG)Buff->CurrentY)
    {
      if (++UpdateRect->top == Buff->MaxY)
        {
          UpdateRect->top = 0;
        }
    }
  UpdateRect->bottom = Buff->CurrentY;
}

static NTSTATUS FASTCALL
ConioWriteConsole(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff,
                  CHAR *Buffer, DWORD Length, BOOL Attrib)
{
  UINT i;
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
                      if ((0 == UpdateRect.top && UpdateRect.bottom < (LONG)Buff->CurrentY)
                          || (0 != UpdateRect.top && (LONG)Buff->CurrentY < UpdateRect.top))
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
              Offset = 2 * (((Buff->CurrentY * Buff->MaxX)) + Buff->CurrentX);
              while (Buff->CurrentX < EndX)
                {
                  Buff->Buffer[Offset] = ' ';
                  Offset += 2;
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
      Offset = 2 * (((Buff->CurrentY * Buff->MaxX)) + Buff->CurrentX);
      Buff->Buffer[Offset++] = Buffer[i];
      if (Attrib)
        {
          Buff->Buffer[Offset] = Buff->DefaultAttrib;
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
                               &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }
  Request->Data.ReadConsoleRequest.EventHandle = ProcessData->ConsoleEvent;
  for (i = 0; i < nNumberOfCharsToRead && Console->InputEvents.Flink != &Console->InputEvents; i++)
    {
      /* remove input event from queue */
      CurrentEntry = RemoveHeadList(&Console->InputEvents);
      if (IsListEmpty(&Console->InputEvents))
      {
         CHECKPOINT;
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
                  Request->Status = STATUS_NOTIFY_CLEANUP;
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
      Request->Status = STATUS_PENDING;    /* we didn't read anything */
    }
  else if (0 != (Console->Mode & ENABLE_LINE_INPUT))
    {
      if (0 == Console->WaitingLines ||
          (Request->Data.ReadConsoleRequest.Unicode ? (L'\n' != UnicodeBuffer[i - 1]) : ('\n' != Buffer[i - 1])))
        {
          Request->Status = STATUS_PENDING; /* line buffered, didn't get a complete line */
        }
      else
        {
          Console->WaitingLines--;
          Request->Status = STATUS_SUCCESS; /* line buffered, did get a complete line */
        }
    }
  else
    {
      Request->Status = STATUS_SUCCESS;  /* not line buffered, did read something */
    }

  if (Request->Status == STATUS_PENDING)
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

  return Request->Status;
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

BOOLEAN __inline ConioIsEqualRect(
  RECT *Rect1,
  RECT *Rect2)
{
  return ((Rect1->left == Rect2->left) && (Rect1->right == Rect2->right) &&
    (Rect1->top == Rect2->top) && (Rect1->bottom == Rect2->bottom));
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

BOOLEAN __inline ConioSubtractRect(
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

static VOID FASTCALL
ConioCopyRegion(PCSRSS_SCREEN_BUFFER ScreenBuffer,
                RECT *SrcRegion,
                RECT *DstRegion)
{
  SHORT SrcY, DstY;
  DWORD SrcOffset;
  DWORD DstOffset;
  DWORD BytesPerLine;
  LONG i;

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

static VOID FASTCALL
ConioFillRegion(PCSRSS_CONSOLE Console,
                PCSRSS_SCREEN_BUFFER ScreenBuffer,
                RECT *Region,
                CHAR_INFO *CharInfo,
                BOOL bUnicode)
{
  SHORT X, Y;
  DWORD Offset;
  DWORD Delta;
  LONG i;
  CHAR Char;

  if(bUnicode)
    ConsoleUnicodeCharToAnsiChar(Console, &Char, &CharInfo->Char.UnicodeChar);
  else
    Char = CharInfo->Char.AsciiChar;

  Y = (Region->top + ScreenBuffer->ShowY) % ScreenBuffer->MaxY;
  Offset = (Y * ScreenBuffer->MaxX + Region->left + ScreenBuffer->ShowX) * 2;
  Delta = (ScreenBuffer->MaxX - ConioRectWidth(Region)) * 2;

  for (i = Region->top; i <= Region->bottom; i++)
    {
      for (X = Region->left; X <= Region->right; X++)
        {
          SET_CELL_BUFFER(ScreenBuffer, Offset, Char, CharInfo->Attributes);
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

static VOID FASTCALL
ConioInputEventToAnsi(PCSRSS_CONSOLE Console, PINPUT_RECORD InputEvent)
{
  if (InputEvent->EventType == KEY_EVENT)
    {
      WCHAR UnicodeChar = InputEvent->Event.KeyEvent.uChar.UnicodeChar;
      InputEvent->Event.KeyEvent.uChar.UnicodeChar = 0;
      ConsoleUnicodeCharToAnsiChar(Console,
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
      return Request->Status = STATUS_INVALID_PARAMETER;
    }
  Status = ConioConsoleFromProcessData(ProcessData, &Console);

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }

  if(Request->Data.WriteConsoleRequest.Unicode)
    {
      Length = WideCharToMultiByte(Console->CodePage, 0,
                                   (PWCHAR)Request->Data.WriteConsoleRequest.Buffer,
                                   Request->Data.WriteConsoleRequest.NrCharactersToWrite,
                                   NULL, 0, NULL, NULL);
      Buffer = RtlAllocateHeap(GetProcessHeap(), 0, Length);
      if (Buffer)
        {
          WideCharToMultiByte(Console->CodePage, 0,
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
      Status = ConioLockScreenBuffer(ProcessData, Request->Data.WriteConsoleRequest.ConsoleHandle, &Buff);
      if (NT_SUCCESS(Status))
        {
          Request->Status = ConioWriteConsole(Console, Buff, Buffer,
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
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  Request->Data.WriteConsoleRequest.NrCharactersWritten = Written;

  return Request->Status = Status;
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

#if 0 // FIXME
  if (0 == InterlockedDecrement(&Console->ActiveBuffer->Header.ReferenceCount))
    {
      ConioDeleteScreenBuffer((Object_t *) Console->ActiveBuffer);
    }
#endif

  Console->ActiveBuffer = NULL;
  ConioCleanupConsole(Console);

  CloseHandle(Console->ActiveEvent);
  DeleteCriticalSection(&Console->Header.Lock);
  RtlFreeUnicodeString(&Console->Title);
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
        ConioConsoleCtrlEvent((DWORD)CTRL_C_EVENT, current);
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
          && KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown))
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

  if (! ConInRec->Fake || ! ConInRec->NotChar)
    {
      /* FIXME - convert to ascii */
      ConioProcessChar(Console, ConInRec);
    }
  else
    {
      HeapFree(Win32CsrApiHeap, 0, ConInRec);
    }
}

CSR_API(CsrGetScreenBufferInfo)
{
  NTSTATUS Status;
  PCSRSS_SCREEN_BUFFER Buff;
  PCONSOLE_SCREEN_BUFFER_INFO pInfo;

  DPRINT("CsrGetScreenBufferInfo\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ScreenBufferInfoRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }
  pInfo = &Request->Data.ScreenBufferInfoRequest.Info;
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

  Request->Status = STATUS_SUCCESS;

  return Request->Status;
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
      return Request->Status = Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = Status;
    }

  NewCursorX = Request->Data.SetCursorRequest.Position.X;
  NewCursorY = Request->Data.SetCursorRequest.Position.Y;
  if (NewCursorX < 0 || NewCursorX >= Buff->MaxX ||
      NewCursorY < 0 || NewCursorY >= Buff->MaxY)
    {
      ConioUnlockScreenBuffer(Buff);
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = STATUS_INVALID_PARAMETER;
    }
  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY, &OldCursorX, &OldCursorY);
  Buff->CurrentX = NewCursorX + Buff->ShowX;
  Buff->CurrentY = (NewCursorY + Buff->ShowY) % Buff->MaxY;
  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      if (! ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY))
        {
          ConioUnlockScreenBuffer(Buff);
          if (NULL != Console)
            {
              ConioUnlockConsole(Console);
            }
          return Request->Status = STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Request->Status = STATUS_SUCCESS;
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
      return Request->Status = STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if (NT_SUCCESS(Status))
    {
      if(Request->Data.WriteConsoleOutputCharRequest.Unicode)
        {
          Length = WideCharToMultiByte(Console->CodePage, 0,
                                      (PWCHAR)Request->Data.WriteConsoleOutputCharRequest.String,
                                       Request->Data.WriteConsoleOutputCharRequest.Length,
                                       NULL, 0, NULL, NULL);
          tmpString = String = RtlAllocateHeap(GetProcessHeap(), 0, Length);
          if (String)
            {
              WideCharToMultiByte(Console->CodePage, 0,
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
                                         &Buff);
          if (NT_SUCCESS(Status))
            {
              X = Request->Data.WriteConsoleOutputCharRequest.Coord.X + Buff->ShowX;
              Y = (Request->Data.WriteConsoleOutputCharRequest.Coord.Y + Buff->ShowY) % Buff->MaxY;
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
              if (NULL != Console && Buff == Console->ActiveBuffer)
                {
                  ConioComputeUpdateRect(Buff, &UpdateRect, &Request->Data.WriteConsoleOutputCharRequest.Coord,
                                         Request->Data.WriteConsoleOutputCharRequest.Length);
                  ConioDrawRegion(Console, &UpdateRect);
                }

                Request->Data.WriteConsoleOutputCharRequest.EndCoord.X = X - Buff->ShowX;
                Request->Data.WriteConsoleOutputCharRequest.EndCoord.Y = (Y + Buff->MaxY - Buff->ShowY) % Buff->MaxY;

                ConioUnlockScreenBuffer(Buff);
            }
          if (Request->Data.WriteConsoleRequest.Unicode)
            {
              RtlFreeHeap(GetProcessHeap(), 0, tmpString);
            }
        }
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
    }
  Request->Data.WriteConsoleOutputCharRequest.NrCharactersWritten = Written;
  return Request->Status = Status;
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
      return Request->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.FillOutputRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = Status;
    }

  X = Request->Data.FillOutputRequest.Position.X + Buff->ShowX;
  Y = (Request->Data.FillOutputRequest.Position.Y + Buff->ShowY) % Buff->MaxY;
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
  Length = Request->Data.FillOutputRequest.Length;
  Request->Data.FillOutputRequest.NrCharactersWritten = Length;
  return Request->Status;
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

  Status = ConioLockConsole(ProcessData, Request->Data.ReadInputRequest.ConsoleHandle, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
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

  return Request->Status = Status;
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
      return Request->Status = STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
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
      return Request->Status = Status;
    }

  X = Request->Data.WriteConsoleOutputAttribRequest.Coord.X + Buff->ShowX;
  Y = (Request->Data.WriteConsoleOutputAttribRequest.Coord.Y + Buff->ShowY) % Buff->MaxY;
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

  Request->Data.WriteConsoleOutputAttribRequest.EndCoord.X = Buff->CurrentX - Buff->ShowX;
  Request->Data.WriteConsoleOutputAttribRequest.EndCoord.Y = (Buff->CurrentY + Buff->MaxY - Buff->ShowY) % Buff->MaxY;

  ConioUnlockScreenBuffer(Buff);

  return Request->Status = STATUS_SUCCESS;
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
      return Request->Status = Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData, Request->Data.FillOutputAttribRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = Status;
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

  return Request->Status = STATUS_SUCCESS;
}


CSR_API(CsrGetCursorInfo)
{
  PCSRSS_SCREEN_BUFFER Buff;
  NTSTATUS Status;

  DPRINT("CsrGetCursorInfo\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.GetCursorInfoRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }
  Request->Data.GetCursorInfoRequest.Info.bVisible = Buff->CursorInfo.bVisible;
  Request->Data.GetCursorInfoRequest.Info.dwSize = Buff->CursorInfo.dwSize;
  ConioUnlockScreenBuffer(Buff);

  return Request->Status = STATUS_SUCCESS;
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
      return Request->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorInfoRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = Status;
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
          return Request->Status = STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Request->Status = STATUS_SUCCESS;
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
      return Request->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = Status;
    }

  ConioPhysicalToLogical(Buff, Buff->CurrentX, Buff->CurrentY, &OldCursorX, &OldCursorY);

  Buff->DefaultAttrib = Request->Data.SetAttribRequest.Attrib;
  if (NULL != Console && Buff == Console->ActiveBuffer)
    {
      if (! ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY))
        {
          ConioUnlockScreenBuffer(Buff);
          ConioUnlockConsole(Console);
          return Request->Status = STATUS_UNSUCCESSFUL;
        }
    }

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  return Request->Status = STATUS_SUCCESS;
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
                             (Object_t **) &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
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
      return Request->Status = STATUS_INVALID_HANDLE;
    }

  Request->Status = STATUS_SUCCESS;

  return Request->Status;
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
                             (Object_t **) &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }
  Request->Status = STATUS_SUCCESS;
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
      Request->Status = STATUS_INVALID_HANDLE;
    }

  return Request->Status;
}

CSR_API(CsrCreateScreenBuffer)
{
  PCSRSS_CONSOLE Console;
  PCSRSS_SCREEN_BUFFER Buff;
  NTSTATUS Status;

  DPRINT("CsrCreateScreenBuffer\n");

  if (ProcessData == NULL)
    {
      return Request->Status = STATUS_INVALID_PARAMETER;
    }

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }
  if (NULL == Console)
    {
      return Request->Status = STATUS_INVALID_HANDLE;
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
      if(! NT_SUCCESS(Status))
        {
          Request->Status = Status;
        }
      else
        {
          Request->Status = Win32CsrInsertObject(ProcessData, &Request->Data.CreateScreenBufferRequest.OutputHandle, &Buff->Header);
        }
    }
  else
    {
      Request->Status = STATUS_INSUFFICIENT_RESOURCES;
    }

  ConioUnlockConsole(Console);
  return Request->Status;
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
      return Request->Status = Status;
    }
  if (NULL == Console)
    {
      DPRINT1("Trying to set screen buffer for app without console\n");
      return Request->Status = STATUS_INVALID_HANDLE;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetScreenBufferRequest.OutputHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      ConioUnlockConsole(Console);
      return Request->Status;
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

  return Request->Status = STATUS_SUCCESS;
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
      return Request->Status = STATUS_INVALID_PARAMETER;
    }

  Status = ConioLockConsole(ProcessData, Request->Data.SetTitleRequest.Console, &Console);
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if(! NT_SUCCESS(Status))
    {
      Request->Status = Status;
    }
  else
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
              Request->Status = STATUS_UNSUCCESSFUL;
            }
          else
            {
              Request->Status = STATUS_SUCCESS;
            }
        }
      else
        {
          Request->Status = STATUS_NO_MEMORY;
        }
    }
  ConioUnlockConsole(Console);

  return Request->Status;
}

CSR_API(CsrGetTitle)
{
  NTSTATUS Status;
  PCSRSS_CONSOLE Console;
  DWORD Length;

  DPRINT("CsrGetTitle\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioLockConsole(ProcessData,
                            Request->Data.GetTitleRequest.ConsoleHandle,
                            &Console);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Can't get console\n");
      return Request->Status = Status;
    }

  /* Copy title of the console to the user title buffer */
  RtlZeroMemory(&Request->Data.GetTitleRequest, sizeof(CSRSS_GET_TITLE));
  Request->Data.GetTitleRequest.ConsoleHandle = Request->Data.GetTitleRequest.ConsoleHandle;
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
  Request->Status = STATUS_SUCCESS;

  return Request->Status;
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
      return Request->Status = Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData,
                                 Request->Data.WriteConsoleOutputRequest.ConsoleHandle,
                                 &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = Status;
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
      return Request->Status = STATUS_ACCESS_VIOLATION;
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
      return Request->Status = STATUS_SUCCESS;
    }

  for (i = 0, Y = WriteRegion.top; Y <= WriteRegion.bottom; i++, Y++)
    {
      CurCharInfo = CharInfo + (i + BufferCoord.Y) * BufferSize.X + BufferCoord.X;
      Offset = (((Y + Buff->ShowY) % Buff->MaxY) * Buff->MaxX + WriteRegion.left) * 2;
      for (X = WriteRegion.left; X <= WriteRegion.right; X++)
        {
          if (Request->Data.WriteConsoleOutputRequest.Unicode)
            {
              CHAR AsciiChar;
              ConsoleUnicodeCharToAnsiChar(Console, &AsciiChar, &CurCharInfo->Char.UnicodeChar);
              SET_CELL_BUFFER(Buff, Offset, AsciiChar, CurCharInfo->Attributes);
            }
          else
            {
              SET_CELL_BUFFER(Buff, Offset, CurCharInfo->Char.AsciiChar, CurCharInfo->Attributes);
            }
          CurCharInfo++;
        }
    }

  if (NULL != Console)
    {
      ConioDrawRegion(Console, &WriteRegion);
    }

  ConioUnlockScreenBuffer(Buff);
  ConioUnlockConsole(Console);

  Request->Data.WriteConsoleOutputRequest.WriteRegion.Right = WriteRegion.left + SizeX - 1;
  Request->Data.WriteConsoleOutputRequest.WriteRegion.Bottom = WriteRegion.top + SizeY - 1;
  Request->Data.WriteConsoleOutputRequest.WriteRegion.Left = WriteRegion.left;
  Request->Data.WriteConsoleOutputRequest.WriteRegion.Top = WriteRegion.top;

  return Request->Status = STATUS_SUCCESS;
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
                            &Console);
  if(! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
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

  return Request->Status = STATUS_SUCCESS;
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
  HANDLE ConsoleHandle;
  BOOLEAN UseClipRectangle;
  COORD DestinationOrigin;
  CHAR_INFO Fill;

  DPRINT("CsrScrollConsoleScreenBuffer\n");

  ConsoleHandle = Request->Data.ScrollConsoleScreenBufferRequest.ConsoleHandle;
  UseClipRectangle = Request->Data.ScrollConsoleScreenBufferRequest.UseClipRectangle;
  DestinationOrigin = Request->Data.ScrollConsoleScreenBufferRequest.DestinationOrigin;
  Fill = Request->Data.ScrollConsoleScreenBufferRequest.Fill;

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Status = ConioLockScreenBuffer(ProcessData, ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = Status;
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
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = STATUS_INVALID_PARAMETER;
    }

  if (UseClipRectangle && ! ConioGetIntersection(&SrcRegion, &SrcRegion, &ClipRectangle))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      ConioUnlockScreenBuffer(Buff);
      return Request->Status = STATUS_SUCCESS;
    }


  ConioInitRect(&DstRegion,
               DestinationOrigin.Y,
               DestinationOrigin.X,
               DestinationOrigin.Y + ConioRectHeight(&ScrollRectangle) - 1,
               DestinationOrigin.X + ConioRectWidth(&ScrollRectangle) - 1);

  /* Make sure destination rectangle is inside the screen buffer */
  if (! ConioGetIntersection(&DstRegion, &DstRegion, &ScreenBuffer))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      ConioUnlockScreenBuffer(Buff);
      return Request->Status = STATUS_INVALID_PARAMETER;
    }

  ConioCopyRegion(Buff, &SrcRegion, &DstRegion);

  /* Get the region that should be filled with the specified character and attributes */

  DoFill = FALSE;

  ConioGetUnion(&FillRegion, &SrcRegion, &DstRegion);

  if (ConioSubtractRect(&FillRegion, &FillRegion, &DstRegion))
    {
      /* FIXME: The subtracted rectangle is off by one line */
      FillRegion.top += 1;

      ConioFillRegion(Console, Buff, &FillRegion, &Fill, Request->Data.ScrollConsoleScreenBufferRequest.Unicode);
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

  return Request->Status = STATUS_SUCCESS;
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
      return Request->Status = Status;
    }

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputCharRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Console)
        {
          ConioUnlockConsole(Console);
        }
      return Request->Status = Status;
    }

  Xpos = Request->Data.ReadConsoleOutputCharRequest.ReadCoord.X + Buff->ShowX;
  Ypos = (Request->Data.ReadConsoleOutputCharRequest.ReadCoord.Y + Buff->ShowY) % Buff->MaxY;

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
  Request->Status = STATUS_SUCCESS;
  Request->Data.ReadConsoleOutputCharRequest.EndCoord.X = Xpos - Buff->ShowX;
  Request->Data.ReadConsoleOutputCharRequest.EndCoord.Y = (Ypos - Buff->ShowY + Buff->MaxY) % Buff->MaxY;

  ConioUnlockScreenBuffer(Buff);
  if (NULL != Console)
    {
      ConioUnlockConsole(Console);
    }

  Request->Data.ReadConsoleOutputCharRequest.CharsRead = (DWORD)((ULONG_PTR)ReadBuffer - (ULONG_PTR)Request->Data.ReadConsoleOutputCharRequest.String) / CharSize;
  if (Request->Data.ReadConsoleOutputCharRequest.CharsRead * CharSize + CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR) > sizeof(CSR_API_MESSAGE))
    {
      Request->Header.u1.s1.TotalLength = Request->Data.ReadConsoleOutputCharRequest.CharsRead * CharSize + CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_CHAR);
      Request->Header.u1.s1.DataLength = Request->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);
    }

  return Request->Status;
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

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputAttribRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
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

  Request->Status = STATUS_SUCCESS;
  Request->Data.ReadConsoleOutputAttribRequest.EndCoord.X = Xpos - Buff->ShowX;
  Request->Data.ReadConsoleOutputAttribRequest.EndCoord.Y = (Ypos - Buff->ShowY + Buff->MaxY) % Buff->MaxY;

  ConioUnlockScreenBuffer(Buff);

  CurrentLength = CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE_OUTPUT_ATTRIB)
                     + Request->Data.ReadConsoleOutputAttribRequest.NumAttrsToRead * sizeof(WORD);
  if (CurrentLength > sizeof(CSR_API_MESSAGE))
    {
      Request->Header.u1.s1.TotalLength = CurrentLength;
      Request->Header.u1.s1.DataLength = CurrentLength - sizeof(PORT_MESSAGE);
    }

  return Request->Status;
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

  Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
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

  Request->Status = STATUS_SUCCESS;
  Request->Data.GetNumInputEventsRequest.NumInputEvents = NumEvents;

  return Request->Status;
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

  Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console);
  if(! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }

  InputRecord = Request->Data.PeekConsoleInputRequest.InputRecord;
  Length = Request->Data.PeekConsoleInputRequest.Length;
  Size = Length * sizeof(INPUT_RECORD);

  if (((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
      || (((ULONG_PTR)InputRecord + Size) > ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockConsole(Console);
      Request->Status = STATUS_ACCESS_VIOLATION;
      return Request->Status ;
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

  Request->Status = STATUS_SUCCESS;
  Request->Data.PeekConsoleInputRequest.Length = NumItems;

  return Request->Status;
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
  DWORD i, Offset;
  LONG X, Y;
  UINT CodePage;

  DPRINT("CsrReadConsoleOutput\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputRequest.ConsoleHandle, &Buff);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
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
      Request->Status = STATUS_ACCESS_VIOLATION;
      return Request->Status ;
    }

  SizeY = min(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&ReadRegion));
  SizeX = min(BufferSize.X - BufferCoord.X, ConioRectWidth(&ReadRegion));
  ReadRegion.bottom = ReadRegion.top + SizeY;
  ReadRegion.right = ReadRegion.left + SizeX;

  ConioInitRect(&ScreenRect, 0, 0, Buff->MaxY, Buff->MaxX);
  if (! ConioGetIntersection(&ReadRegion, &ScreenRect, &ReadRegion))
    {
      ConioUnlockScreenBuffer(Buff);
      Request->Status = STATUS_SUCCESS;
      return Request->Status;
    }

  for (i = 0, Y = ReadRegion.top; Y < ReadRegion.bottom; ++i, ++Y)
    {
      CurCharInfo = CharInfo + (i * BufferSize.X);

      Offset = (((Y + Buff->ShowY) % Buff->MaxY) * Buff->MaxX + ReadRegion.left) * 2;
      for (X = ReadRegion.left; X < ReadRegion.right; ++X)
        {
          if (Request->Data.ReadConsoleOutputRequest.Unicode)
            {
              MultiByteToWideChar(CodePage, 0,
                                  (PCHAR)&GET_CELL_BUFFER(Buff, Offset), 1,
                                  &CurCharInfo->Char.UnicodeChar, 1);
            }
          else
            {
              CurCharInfo->Char.AsciiChar = GET_CELL_BUFFER(Buff, Offset);
            }
          CurCharInfo->Attributes = GET_CELL_BUFFER(Buff, Offset);
          ++CurCharInfo;
        }
    }

  ConioUnlockScreenBuffer(Buff);

  Request->Status = STATUS_SUCCESS;
  Request->Data.ReadConsoleOutputRequest.ReadRegion.Right = ReadRegion.left + SizeX - 1;
  Request->Data.ReadConsoleOutputRequest.ReadRegion.Bottom = ReadRegion.top + SizeY - 1;
  Request->Data.ReadConsoleOutputRequest.ReadRegion.Left = ReadRegion.left;
  Request->Data.ReadConsoleOutputRequest.ReadRegion.Top = ReadRegion.top;

  return Request->Status;
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

  Status = ConioLockConsole(ProcessData, Request->Data.WriteConsoleInputRequest.ConsoleHandle, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }

  InputRecord = Request->Data.WriteConsoleInputRequest.InputRecord;
  Length = Request->Data.WriteConsoleInputRequest.Length;
  Size = Length * sizeof(INPUT_RECORD);

  if (((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
      || (((ULONG_PTR)InputRecord + Size) > ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
      ConioUnlockConsole(Console);
      Request->Status = STATUS_ACCESS_VIOLATION;
      return Request->Status ;
    }

  for (i = 0; i < Length; i++)
    {
      Record = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));
      if (NULL == Record)
        {
          ConioUnlockConsole(Console);
          Request->Status = STATUS_INSUFFICIENT_RESOURCES;
          return Request->Status;
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

  Request->Status = STATUS_SUCCESS;
  Request->Data.WriteConsoleInputRequest.Length = i;

  return Request->Status;
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
                            &Console);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed to get console handle in SetConsoleHardwareState\n");
      return Request->Status = Status;
    }

  switch (Request->Data.ConsoleHardwareStateRequest.SetGet)
    {
      case CONSOLE_HARDWARE_STATE_GET:
        Request->Data.ConsoleHardwareStateRequest.State = Console->HardwareState;
        break;

      case CONSOLE_HARDWARE_STATE_SET:
        DPRINT("Setting console hardware state.\n");
        Request->Status = SetConsoleHardwareState(Console, Request->Data.ConsoleHardwareStateRequest.State);
        break;

      default:
        Request->Status = STATUS_INVALID_PARAMETER_2; /* Client: (handle, [set_get], mode) */
        break;
    }

  ConioUnlockConsole(Console);

  return Request->Status;
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
      return Request->Status = Status;
    }

  Request->Data.GetConsoleWindowRequest.WindowHandle = Console->hWindow;
  ConioUnlockConsole(Console);

  return Request->Status = STATUS_SUCCESS;
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
      return Request->Status = Status;
    }

  Console->hWindowIcon = Request->Data.SetConsoleIconRequest.WindowIcon;
  Request->Status = (ConioChangeIcon(Console) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
  ConioUnlockConsole(Console);

  return Request->Status;
}

CSR_API(CsrGetConsoleCodePage)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrGetConsoleCodePage\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Request->Data.GetConsoleCodePage.CodePage = Console->CodePage;
  ConioUnlockConsole(Console);
  return Request->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleCodePage)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrSetConsoleCodePage\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if (IsValidCodePage(Request->Data.SetConsoleCodePage.CodePage))
    {
      Console->CodePage = Request->Data.SetConsoleCodePage.CodePage;
      ConioUnlockConsole(Console);
      return Request->Status = STATUS_SUCCESS;
    }
  ConioUnlockConsole(Console);
  return Request->Status = STATUS_UNSUCCESSFUL;
}

CSR_API(CsrGetConsoleOutputCodePage)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrGetConsoleOutputCodePage\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  Request->Data.GetConsoleOutputCodePage.CodePage = Console->OutputCodePage;
  ConioUnlockConsole(Console);
  return Request->Status = STATUS_SUCCESS;
}

CSR_API(CsrSetConsoleOutputCodePage)
{
  PCSRSS_CONSOLE Console;
  NTSTATUS Status;

  DPRINT("CsrSetConsoleOutputCodePage\n");

  Status = ConioConsoleFromProcessData(ProcessData, &Console);
  if (! NT_SUCCESS(Status))
    {
      return Request->Status = Status;
    }

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
  if (IsValidCodePage(Request->Data.SetConsoleOutputCodePage.CodePage))
    {
      Console->OutputCodePage = Request->Data.SetConsoleOutputCodePage.CodePage;
      ConioUnlockConsole(Console);
      return Request->Status = STATUS_SUCCESS;
    }
  ConioUnlockConsole(Console);
  return Request->Status = STATUS_UNSUCCESSFUL;
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
    return Request->Status = Status;
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
  return Request->Status = STATUS_SUCCESS;
}

/* EOF */
