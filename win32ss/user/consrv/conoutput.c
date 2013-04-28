/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/conoutput.c
 * PURPOSE:         Console Output functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "console.h"
#include "include/conio.h"
#include "conio.h"
#include "conoutput.h"
#include "handle.h"

#define NDEBUG
#include <debug.h>

/*
// Define wmemset(...)
#include <wchar.h>
#define HAVE_WMEMSET
*/


/* GLOBALS ********************************************************************/

#define TAB_WIDTH   8

#define ConioInitRect(Rect, top, left, bottom, right) \
do {    \
    ((Rect)->Top) = top;    \
    ((Rect)->Left) = left;  \
    ((Rect)->Bottom) = bottom;  \
    ((Rect)->Right) = right;    \
} while (0)

#define ConioIsRectEmpty(Rect) \
    (((Rect)->Left > (Rect)->Right) || ((Rect)->Top > (Rect)->Bottom))

#define ConsoleUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->OutputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->OutputCodePage, 0, (sChar), 1, (dWChar), 1)


/* PRIVATE FUNCTIONS **********************************************************/

PBYTE FASTCALL
ConioCoordToPointer(PCONSOLE_SCREEN_BUFFER Buff, ULONG X, ULONG Y)
{
    return &Buff->Buffer[2 * (((Y + Buff->VirtualY) % Buff->ScreenBufferSize.Y) * Buff->ScreenBufferSize.X + X)];
}

static VOID FASTCALL
ClearLineBuffer(PCONSOLE_SCREEN_BUFFER Buff)
{
    PBYTE Ptr = ConioCoordToPointer(Buff, 0, Buff->CursorPosition.Y);
    SHORT Pos;

    for (Pos = 0; Pos < Buff->ScreenBufferSize.X; Pos++)
    {
        /* Fill the cell */
        *Ptr++ = ' ';
        *Ptr++ = (BYTE)Buff->ScreenDefaultAttrib;
    }
}

NTSTATUS FASTCALL
ConSrvCreateScreenBuffer(IN OUT PCONSOLE Console,
                         OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                         IN COORD ScreenBufferSize,
                         IN USHORT ScreenAttrib,
                         IN USHORT PopupAttrib,
                         IN ULONG DisplayMode,
                         IN BOOLEAN IsCursorVisible,
                         IN ULONG CursorSize)
{
    if (Console == NULL || Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    *Buffer = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY, sizeof(CONSOLE_SCREEN_BUFFER));
    if (NULL == *Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    (*Buffer)->Header.Type = SCREEN_BUFFER;
    (*Buffer)->Header.Console = Console;
    (*Buffer)->Header.HandleCount = 0;
    (*Buffer)->ScreenBufferSize = ScreenBufferSize;

    (*Buffer)->Buffer = RtlAllocateHeap(ConSrvHeap, HEAP_ZERO_MEMORY, (*Buffer)->ScreenBufferSize.X * (*Buffer)->ScreenBufferSize.Y * 2);
    if (NULL == (*Buffer)->Buffer)
    {
        RtlFreeHeap(ConSrvHeap, 0, *Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    (*Buffer)->ShowX = 0;
    (*Buffer)->ShowY = 0;
    (*Buffer)->VirtualY = 0;

    (*Buffer)->CursorInfo.bVisible = (IsCursorVisible && (CursorSize != 0));
    (*Buffer)->CursorInfo.dwSize   = min(max(CursorSize, 0), 100);

    (*Buffer)->ScreenDefaultAttrib = ScreenAttrib;
    (*Buffer)->PopupDefaultAttrib  = PopupAttrib;
    /* initialize buffer to be empty with default attributes */
    for ((*Buffer)->CursorPosition.Y = 0 ; (*Buffer)->CursorPosition.Y < (*Buffer)->ScreenBufferSize.Y; (*Buffer)->CursorPosition.Y++)
    {
        ClearLineBuffer(*Buffer);
    }
    (*Buffer)->CursorPosition.X = 0;
    (*Buffer)->CursorPosition.Y = 0;

    (*Buffer)->Mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    (*Buffer)->DisplayMode = DisplayMode;

    InsertHeadList(&Console->BufferList, &(*Buffer)->ListEntry);
    return STATUS_SUCCESS;
}

static VOID FASTCALL
ConioNextLine(PCONSOLE_SCREEN_BUFFER Buff, SMALL_RECT* UpdateRect, UINT *ScrolledLines)
{
    /* If we hit bottom, slide the viewable screen */
    if (++Buff->CursorPosition.Y == Buff->ScreenBufferSize.Y)
    {
        Buff->CursorPosition.Y--;
        if (++Buff->VirtualY == Buff->ScreenBufferSize.Y)
        {
            Buff->VirtualY = 0;
        }
        (*ScrolledLines)++;
        ClearLineBuffer(Buff);
        if (UpdateRect->Top != 0)
        {
            UpdateRect->Top--;
        }
    }
    UpdateRect->Left = 0;
    UpdateRect->Right = Buff->ScreenBufferSize.X - 1;
    UpdateRect->Bottom = Buff->CursorPosition.Y;
}

NTSTATUS FASTCALL
ConioWriteConsole(PCONSOLE Console, PCONSOLE_SCREEN_BUFFER Buff,
                  CHAR *Buffer, DWORD Length, BOOL Attrib)
{
    UINT i;
    PBYTE Ptr;
    SMALL_RECT UpdateRect;
    SHORT CursorStartX, CursorStartY;
    UINT ScrolledLines;

    CursorStartX = Buff->CursorPosition.X;
    CursorStartY = Buff->CursorPosition.Y;
    UpdateRect.Left = Buff->ScreenBufferSize.X;
    UpdateRect.Top = Buff->CursorPosition.Y;
    UpdateRect.Right = -1;
    UpdateRect.Bottom = Buff->CursorPosition.Y;
    ScrolledLines = 0;

    for (i = 0; i < Length; i++)
    {
        /*
         * If we are in processed mode, interpret special characters and
         * display them correctly. Otherwise, just put them into the buffer.
         */
        if (Buff->Mode & ENABLE_PROCESSED_OUTPUT)
        {
            /* --- CR --- */
            if (Buffer[i] == '\r')
            {
                Buff->CursorPosition.X = 0;
                UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);
                UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);
                continue;
            }
            /* --- LF --- */
            else if (Buffer[i] == '\n')
            {
                Buff->CursorPosition.X = 0;
                ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
                continue;
            }
            /* --- BS --- */
            else if (Buffer[i] == '\b')
            {
                /* Only handle BS if we're not on the first pos of the first line */
                if (0 != Buff->CursorPosition.X || 0 != Buff->CursorPosition.Y)
                {
                    if (0 == Buff->CursorPosition.X)
                    {
                        /* slide virtual position up */
                        Buff->CursorPosition.X = Buff->ScreenBufferSize.X - 1;
                        Buff->CursorPosition.Y--;
                        UpdateRect.Top = min(UpdateRect.Top, Buff->CursorPosition.Y);
                    }
                    else
                    {
                        Buff->CursorPosition.X--;
                    }
                    Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
                    Ptr[0] = ' ';
                    Ptr[1] = (BYTE)Buff->ScreenDefaultAttrib;
                    UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);
                    UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);
                }
                continue;
            }
            /* --- TAB --- */
            else if (Buffer[i] == '\t')
            {
                UINT EndX;

                UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);
                EndX = (Buff->CursorPosition.X + TAB_WIDTH) & ~(TAB_WIDTH - 1);
                EndX = min(EndX, (UINT)Buff->ScreenBufferSize.X);
                Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
                while (Buff->CursorPosition.X < EndX)
                {
                    *Ptr++ = ' ';
                    *Ptr++ = (BYTE)Buff->ScreenDefaultAttrib;
                    Buff->CursorPosition.X++;
                }
                UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X - 1);
                if (Buff->CursorPosition.X == Buff->ScreenBufferSize.X)
                {
                    if (Buff->Mode & ENABLE_WRAP_AT_EOL_OUTPUT)
                    {
                        Buff->CursorPosition.X = 0;
                        ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
                    }
                    else
                    {
                        Buff->CursorPosition.X--;
                    }
                }
                continue;
            }
            // /* --- BEL ---*/
            // else if (Buffer[i] == '\a')
            // {
                // // FIXME: This MUST BE moved to the terminal emulator frontend!!
                // DPRINT1("Bell\n");
                // // SendNotifyMessage(Console->hWindow, PM_CONSOLE_BEEP, 0, 0);
                // continue;
            // }
        }
        UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);
        UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);
        Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
        Ptr[0] = Buffer[i];
        if (Attrib)
        {
            Ptr[1] = (BYTE)Buff->ScreenDefaultAttrib;
        }
        Buff->CursorPosition.X++;
        if (Buff->CursorPosition.X == Buff->ScreenBufferSize.X)
        {
            if (Buff->Mode & ENABLE_WRAP_AT_EOL_OUTPUT)
            {
                Buff->CursorPosition.X = 0;
                ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
            }
            else
            {
                Buff->CursorPosition.X = CursorStartX;
            }
        }
    }

    if (!ConioIsRectEmpty(&UpdateRect) && Buff == Console->ActiveBuffer)
    {
        ConioWriteStream(Console, &UpdateRect, CursorStartX, CursorStartY,
                         ScrolledLines, Buffer, Length);
    }

    return STATUS_SUCCESS;
}

__inline BOOLEAN ConioGetIntersection(
    SMALL_RECT* Intersection,
    SMALL_RECT* Rect1,
    SMALL_RECT* Rect2)
{
    if (ConioIsRectEmpty(Rect1) ||
            (ConioIsRectEmpty(Rect2)) ||
            (Rect1->Top > Rect2->Bottom) ||
            (Rect1->Left > Rect2->Right) ||
            (Rect1->Bottom < Rect2->Top) ||
            (Rect1->Right < Rect2->Left))
    {
        /* The rectangles do not intersect */
        ConioInitRect(Intersection, 0, -1, 0, -1);
        return FALSE;
    }

    ConioInitRect(Intersection,
                  max(Rect1->Top, Rect2->Top),
                  max(Rect1->Left, Rect2->Left),
                  min(Rect1->Bottom, Rect2->Bottom),
                  min(Rect1->Right, Rect2->Right));

    return TRUE;
}

__inline BOOLEAN ConioGetUnion(
    SMALL_RECT* Union,
    SMALL_RECT* Rect1,
    SMALL_RECT* Rect2)
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
                      min(Rect1->Top, Rect2->Top),
                      min(Rect1->Left, Rect2->Left),
                      max(Rect1->Bottom, Rect2->Bottom),
                      max(Rect1->Right, Rect2->Right));
    }

    return TRUE;
}

/*
 * Move from one rectangle to another. We must be careful about the order that
 * this is done, to avoid overwriting parts of the source before they are moved.
 */
static VOID FASTCALL
ConioMoveRegion(PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                SMALL_RECT* SrcRegion,
                SMALL_RECT* DstRegion,
                SMALL_RECT* ClipRegion,
                WORD Fill)
{
    int Width = ConioRectWidth(SrcRegion);
    int Height = ConioRectHeight(SrcRegion);
    int SX, SY;
    int DX, DY;
    int XDelta, YDelta;
    int i, j;

    SY = SrcRegion->Top;
    DY = DstRegion->Top;
    YDelta = 1;
    if (SY < DY)
    {
        /* Moving down: work from bottom up */
        SY = SrcRegion->Bottom;
        DY = DstRegion->Bottom;
        YDelta = -1;
    }
    for (i = 0; i < Height; i++)
    {
        PWORD SRow = (PWORD)ConioCoordToPointer(ScreenBuffer, 0, SY);
        PWORD DRow = (PWORD)ConioCoordToPointer(ScreenBuffer, 0, DY);

        SX = SrcRegion->Left;
        DX = DstRegion->Left;
        XDelta = 1;
        if (SX < DX)
        {
            /* Moving right: work from right to left */
            SX = SrcRegion->Right;
            DX = DstRegion->Right;
            XDelta = -1;
        }
        for (j = 0; j < Width; j++)
        {
            WORD Cell = SRow[SX];
            if (SX >= ClipRegion->Left && SX <= ClipRegion->Right
                && SY >= ClipRegion->Top && SY <= ClipRegion->Bottom)
            {
                SRow[SX] = Fill;
            }
            if (DX >= ClipRegion->Left && DX <= ClipRegion->Right
                && DY >= ClipRegion->Top && DY <= ClipRegion->Bottom)
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

NTSTATUS FASTCALL
ConioResizeBuffer(PCONSOLE Console,
                  PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                  COORD Size)
{
    BYTE * Buffer;
    DWORD Offset = 0;
    BYTE * OldPtr;
    USHORT CurrentY;
    BYTE * OldBuffer;
#ifdef HAVE_WMEMSET
    USHORT value = MAKEWORD(' ', ScreenBuffer->ScreenDefaultAttrib);
#else
    DWORD i;
#endif
    DWORD diff;

    /* Buffer size is not allowed to be smaller than window size */
    if (Size.X < Console->ConsoleSize.X || Size.Y < Console->ConsoleSize.Y)
        return STATUS_INVALID_PARAMETER;

    if (Size.X == ScreenBuffer->ScreenBufferSize.X && Size.Y == ScreenBuffer->ScreenBufferSize.Y)
    {
        // FIXME: Trigger a buffer resize event ??
        return STATUS_SUCCESS;
    }

    if (!ConioIsBufferResizeSupported(Console)) return STATUS_NOT_SUPPORTED;

    Buffer = RtlAllocateHeap(ConSrvHeap, 0, Size.X * Size.Y * 2);
    if (!Buffer) return STATUS_NO_MEMORY;

    DPRINT1("Resizing (%d,%d) to (%d,%d)\n", ScreenBuffer->ScreenBufferSize.X, ScreenBuffer->ScreenBufferSize.Y, Size.X, Size.Y);
    OldBuffer = ScreenBuffer->Buffer;

    for (CurrentY = 0; CurrentY < ScreenBuffer->ScreenBufferSize.Y && CurrentY < Size.Y; CurrentY++)
    {
        OldPtr = ConioCoordToPointer(ScreenBuffer, 0, CurrentY);
        if (Size.X <= ScreenBuffer->ScreenBufferSize.X)
        {
            /* reduce size */
            RtlCopyMemory(&Buffer[Offset], OldPtr, Size.X * 2);
            Offset += (Size.X * 2);
        }
        else
        {
            /* enlarge size */
            RtlCopyMemory(&Buffer[Offset], OldPtr, ScreenBuffer->ScreenBufferSize.X * 2);
            Offset += (ScreenBuffer->ScreenBufferSize.X * 2);

            diff = Size.X - ScreenBuffer->ScreenBufferSize.X;
            /* zero new part of it */
#ifdef HAVE_WMEMSET
            wmemset((PWCHAR)&Buffer[Offset], value, diff);
#else
            for (i = 0; i < diff; i++)
            {
                Buffer[Offset++] = ' ';
                Buffer[Offset++] = (BYTE)ScreenBuffer->ScreenDefaultAttrib;
            }
#endif
        }
    }

    if (Size.Y > ScreenBuffer->ScreenBufferSize.Y)
    {
        diff = Size.X * (Size.Y - ScreenBuffer->ScreenBufferSize.Y);
#ifdef HAVE_WMEMSET
        wmemset((PWCHAR)&Buffer[Offset], value, diff);
#else
        for (i = 0; i < diff; i++)
        {
            Buffer[Offset++] = ' ';
            Buffer[Offset++] = (BYTE)ScreenBuffer->ScreenDefaultAttrib;
        }
#endif
    }

    (void)InterlockedExchangePointer((PVOID volatile*)&ScreenBuffer->Buffer, Buffer);
    RtlFreeHeap(ConSrvHeap, 0, OldBuffer);
    ScreenBuffer->ScreenBufferSize = Size;
    ScreenBuffer->VirtualY = 0;

    /* Ensure cursor and window are within buffer */
    if (ScreenBuffer->CursorPosition.X >= Size.X)
        ScreenBuffer->CursorPosition.X = Size.X - 1;
    if (ScreenBuffer->CursorPosition.Y >= Size.Y)
        ScreenBuffer->CursorPosition.Y = Size.Y - 1;
    if (ScreenBuffer->ShowX > Size.X - Console->ConsoleSize.X)
        ScreenBuffer->ShowX = Size.X - Console->ConsoleSize.X;
    if (ScreenBuffer->ShowY > Size.Y - Console->ConsoleSize.Y)
        ScreenBuffer->ShowY = Size.Y - Console->ConsoleSize.Y;

    /*
     * Trigger a buffer resize event
     */
    if (Console->InputBuffer.Mode & ENABLE_WINDOW_INPUT)
    {
        INPUT_RECORD er;

        er.EventType = WINDOW_BUFFER_SIZE_EVENT;
        er.Event.WindowBufferSizeEvent.dwSize = ScreenBuffer->ScreenBufferSize;

        ConioProcessInputEvent(Console, &er);
    }

    /* TODO: Should update scrollbar, but can't use anything that
     * calls SendMessage or it could cause deadlock --> Use PostMessage */
    // TODO: Tell the terminal to resize its scrollbars.

    return STATUS_SUCCESS;
}

VOID WINAPI
ConioDeleteScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer)
{
    PCONSOLE Console = Buffer->Header.Console;

    RemoveEntryList(&Buffer->ListEntry);
    if (Buffer == Console->ActiveBuffer)
    {
        /* Deleted active buffer; switch to most recently created */
        Console->ActiveBuffer = NULL;
        if (!IsListEmpty(&Console->BufferList))
        {
            Console->ActiveBuffer = CONTAINING_RECORD(Console->BufferList.Flink, CONSOLE_SCREEN_BUFFER, ListEntry);
            ConioDrawConsole(Console);
        }
    }

    RtlFreeHeap(ConSrvHeap, 0, Buffer->Buffer);
    RtlFreeHeap(ConSrvHeap, 0, Buffer);
}

VOID FASTCALL
ConioDrawConsole(PCONSOLE Console)
{
    SMALL_RECT Region;

    ConioInitRect(&Region, 0, 0, Console->ConsoleSize.Y - 1, Console->ConsoleSize.X - 1);
    ConioDrawRegion(Console, &Region);
}

static VOID FASTCALL
ConioComputeUpdateRect(PCONSOLE_SCREEN_BUFFER Buff, SMALL_RECT* UpdateRect, PCOORD Start, UINT Length)
{
    if (Buff->ScreenBufferSize.X <= Start->X + Length)
    {
        UpdateRect->Left = 0;
    }
    else
    {
        UpdateRect->Left = Start->X;
    }
    if (Buff->ScreenBufferSize.X <= Start->X + Length)
    {
        UpdateRect->Right = Buff->ScreenBufferSize.X - 1;
    }
    else
    {
        UpdateRect->Right = Start->X + Length - 1;
    }
    UpdateRect->Top = Start->Y;
    UpdateRect->Bottom = Start->Y + (Start->X + Length - 1) / Buff->ScreenBufferSize.X;
    if (Buff->ScreenBufferSize.Y <= UpdateRect->Bottom)
    {
        UpdateRect->Bottom = Buff->ScreenBufferSize.Y - 1;
    }
}

DWORD FASTCALL
ConioEffectiveCursorSize(PCONSOLE Console, DWORD Scale)
{
    DWORD Size = (Console->ActiveBuffer->CursorInfo.dwSize * Scale + 99) / 100;
    /* If line input in progress, perhaps adjust for insert toggle */
    if (Console->LineBuffer && !Console->LineComplete && Console->LineInsertToggle)
        return (Size * 2 <= Scale) ? (Size * 2) : (Size / 2);
    return Size;
}

static NTSTATUS
DoWriteConsole(IN PCSR_API_MESSAGE ApiMessage,
               IN PCSR_THREAD ClientThread,
               IN BOOL CreateWaitBlock OPTIONAL);

// Wait function CSR_WAIT_FUNCTION
static BOOLEAN
WriteConsoleThread(IN PLIST_ENTRY WaitList,
                   IN PCSR_THREAD WaitThread,
                   IN PCSR_API_MESSAGE WaitApiMessage,
                   IN PVOID WaitContext,
                   IN PVOID WaitArgument1,
                   IN PVOID WaitArgument2,
                   IN ULONG WaitFlags)
{
    NTSTATUS Status;

    DPRINT("WriteConsoleThread - WaitContext = 0x%p, WaitArgument1 = 0x%p, WaitArgument2 = 0x%p, WaitFlags = %lu\n", WaitContext, WaitArgument1, WaitArgument2, WaitFlags);

    /*
     * If we are notified of the process termination via a call
     * to CsrNotifyWaitBlock triggered by CsrDestroyProcess or
     * CsrDestroyThread, just return.
     */
    if (WaitFlags & CsrProcessTerminating)
    {
        Status = STATUS_THREAD_IS_TERMINATING;
        goto Quit;
    }

    Status = DoWriteConsole(WaitApiMessage,
                            WaitThread,
                            FALSE);

Quit:
    if (Status != STATUS_PENDING)
    {
        WaitApiMessage->Status = Status;
    }

    return (Status == STATUS_PENDING ? FALSE : TRUE);
}

static NTSTATUS
DoWriteConsole(IN PCSR_API_MESSAGE ApiMessage,
               IN PCSR_THREAD ClientThread,
               IN BOOL CreateWaitBlock OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_WRITECONSOLE WriteConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteConsoleRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    PCHAR Buffer;
    DWORD Written = 0;
    ULONG Length;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(ClientThread->Process), WriteConsoleRequest->OutputHandle, &Buff, GENERIC_WRITE, FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    // if (Console->PauseFlags & (PAUSED_FROM_KEYBOARD | PAUSED_FROM_SCROLLBAR | PAUSED_FROM_SELECTION))
    if (Console->PauseFlags && Console->UnpauseEvent != NULL)
    {
        if (CreateWaitBlock)
        {
            if (!CsrCreateWait(&Console->WriteWaitQueue,
                               WriteConsoleThread,
                               ClientThread,
                               ApiMessage,
                               NULL,
                               NULL))
            {
                /* Fail */
                ConSrvReleaseScreenBuffer(Buff, FALSE);
                return STATUS_NO_MEMORY;
            }
        }

        /* Wait until we un-pause the console */
        Status = STATUS_PENDING;
    }
    else
    {
        if (WriteConsoleRequest->Unicode)
        {
            Length = WideCharToMultiByte(Console->OutputCodePage, 0,
                                         (PWCHAR)WriteConsoleRequest->Buffer,
                                         WriteConsoleRequest->NrCharactersToWrite,
                                         NULL, 0, NULL, NULL);
            Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
            if (Buffer)
            {
                WideCharToMultiByte(Console->OutputCodePage, 0,
                                    (PWCHAR)WriteConsoleRequest->Buffer,
                                    WriteConsoleRequest->NrCharactersToWrite,
                                    Buffer, Length, NULL, NULL);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }
        else
        {
            Buffer = (PCHAR)WriteConsoleRequest->Buffer;
        }

        if (Buffer)
        {
            if (NT_SUCCESS(Status))
            {
                Status = ConioWriteConsole(Console, Buff, Buffer,
                                           WriteConsoleRequest->NrCharactersToWrite, TRUE);
                if (NT_SUCCESS(Status))
                {
                    Written = WriteConsoleRequest->NrCharactersToWrite;
                }
            }
            if (WriteConsoleRequest->Unicode)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
            }
        }

        WriteConsoleRequest->NrCharactersWritten = Written;
    }

    ConSrvReleaseScreenBuffer(Buff, FALSE);
    return Status;
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvReadConsoleOutput)
{
    PCONSOLE_READOUTPUT ReadOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadOutputRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCHAR_INFO CharInfo;
    PCHAR_INFO CurCharInfo;
    PCONSOLE_SCREEN_BUFFER Buff;
    SHORT SizeX, SizeY;
    NTSTATUS Status;
    COORD BufferSize;
    COORD BufferCoord;
    SMALL_RECT ReadRegion;
    SMALL_RECT ScreenRect;
    DWORD i;
    PBYTE Ptr;
    LONG X, Y;
    UINT CodePage;

    DPRINT("SrvReadConsoleOutput\n");

    CharInfo = ReadOutputRequest->CharInfo;
    ReadRegion = ReadOutputRequest->ReadRegion;
    BufferSize = ReadOutputRequest->BufferSize;
    BufferCoord = ReadOutputRequest->BufferCoord;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&ReadOutputRequest->CharInfo,
                                  BufferSize.X * BufferSize.Y,
                                  sizeof(CHAR_INFO)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetScreenBuffer(ProcessData, ReadOutputRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* FIXME: Is this correct? */
    CodePage = ProcessData->Console->OutputCodePage;

    SizeY = min(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&ReadRegion));
    SizeX = min(BufferSize.X - BufferCoord.X, ConioRectWidth(&ReadRegion));
    ReadRegion.Bottom = ReadRegion.Top + SizeY;
    ReadRegion.Right = ReadRegion.Left + SizeX;

    ConioInitRect(&ScreenRect, 0, 0, Buff->ScreenBufferSize.Y, Buff->ScreenBufferSize.X);
    if (!ConioGetIntersection(&ReadRegion, &ScreenRect, &ReadRegion))
    {
        ConSrvReleaseScreenBuffer(Buff, TRUE);
        return STATUS_SUCCESS;
    }

    for (i = 0, Y = ReadRegion.Top; Y < ReadRegion.Bottom; ++i, ++Y)
    {
        CurCharInfo = CharInfo + (i * BufferSize.X);

        Ptr = ConioCoordToPointer(Buff, ReadRegion.Left, Y);
        for (X = ReadRegion.Left; X < ReadRegion.Right; ++X)
        {
            if (ReadOutputRequest->Unicode)
            {
                // ConsoleAnsiCharToUnicodeChar(ProcessData->Console, (PCHAR)Ptr++, &CurCharInfo->Char.UnicodeChar);
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

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    ReadOutputRequest->ReadRegion.Right = ReadRegion.Left + SizeX - 1;
    ReadOutputRequest->ReadRegion.Bottom = ReadRegion.Top + SizeY - 1;
    ReadOutputRequest->ReadRegion.Left = ReadRegion.Left;
    ReadOutputRequest->ReadRegion.Top = ReadRegion.Top;

    return STATUS_SUCCESS;
}

CSR_API(SrvWriteConsole)
{
    NTSTATUS Status;
    PCONSOLE_WRITECONSOLE WriteConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteConsoleRequest;

    DPRINT("SrvWriteConsole\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID)&WriteConsoleRequest->Buffer,
                                  WriteConsoleRequest->BufferSize,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = DoWriteConsole(ApiMessage,
                            CsrGetClientThread(),
                            TRUE);

    if (Status == STATUS_PENDING)
        *ReplyCode = CsrReplyPending;

    return Status;
}

CSR_API(SrvWriteConsoleOutput)
{
    PCONSOLE_WRITEOUTPUT WriteOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteOutputRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    SHORT i, X, Y, SizeX, SizeY;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    SMALL_RECT ScreenBuffer;
    CHAR_INFO* CurCharInfo;
    SMALL_RECT WriteRegion;
    CHAR_INFO* CharInfo;
    COORD BufferCoord;
    COORD BufferSize;
    NTSTATUS Status;
    PBYTE Ptr;

    DPRINT("SrvWriteConsoleOutput\n");

    BufferSize = WriteOutputRequest->BufferSize;
    BufferCoord = WriteOutputRequest->BufferCoord;
    CharInfo = WriteOutputRequest->CharInfo;

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&WriteOutputRequest->CharInfo,
                                  BufferSize.X * BufferSize.Y,
                                  sizeof(CHAR_INFO)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetScreenBuffer(ProcessData,
                                  WriteOutputRequest->OutputHandle,
                                  &Buff,
                                  GENERIC_WRITE,
                                  TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    WriteRegion = WriteOutputRequest->WriteRegion;

    SizeY = min(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&WriteRegion));
    SizeX = min(BufferSize.X - BufferCoord.X, ConioRectWidth(&WriteRegion));
    WriteRegion.Bottom = WriteRegion.Top + SizeY - 1;
    WriteRegion.Right = WriteRegion.Left + SizeX - 1;

    /* Make sure WriteRegion is inside the screen buffer */
    ConioInitRect(&ScreenBuffer, 0, 0, Buff->ScreenBufferSize.Y - 1, Buff->ScreenBufferSize.X - 1);
    if (!ConioGetIntersection(&WriteRegion, &ScreenBuffer, &WriteRegion))
    {
        ConSrvReleaseScreenBuffer(Buff, TRUE);

        /* It is okay to have a WriteRegion completely outside the screen buffer.
           No data is written then. */
        return STATUS_SUCCESS;
    }

    for (i = 0, Y = WriteRegion.Top; Y <= WriteRegion.Bottom; i++, Y++)
    {
        CurCharInfo = CharInfo + (i + BufferCoord.Y) * BufferSize.X + BufferCoord.X;
        Ptr = ConioCoordToPointer(Buff, WriteRegion.Left, Y);
        for (X = WriteRegion.Left; X <= WriteRegion.Right; X++)
        {
            CHAR AsciiChar;
            if (WriteOutputRequest->Unicode)
            {
                ConsoleUnicodeCharToAnsiChar(Console, &AsciiChar, &CurCharInfo->Char.UnicodeChar);
            }
            else
            {
                AsciiChar = CurCharInfo->Char.AsciiChar;
            }
            *Ptr++ = AsciiChar;
            *Ptr++ = (BYTE)CurCharInfo->Attributes;
            CurCharInfo++;
        }
    }

    ConioDrawRegion(Console, &WriteRegion);

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    WriteOutputRequest->WriteRegion.Right = WriteRegion.Left + SizeX - 1;
    WriteOutputRequest->WriteRegion.Bottom = WriteRegion.Top + SizeY - 1;
    WriteOutputRequest->WriteRegion.Left = WriteRegion.Left;
    WriteOutputRequest->WriteRegion.Top = WriteRegion.Top;

    return STATUS_SUCCESS;
}

CSR_API(SrvReadConsoleOutputString)
{
    NTSTATUS Status;
    PCONSOLE_READOUTPUTCODE ReadOutputCodeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadOutputCodeRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    USHORT CodeType;
    SHORT Xpos, Ypos;
    PVOID ReadBuffer;
    DWORD i;
    ULONG CodeSize;
    BYTE Code;

    DPRINT("SrvReadConsoleOutputString\n");

    CodeType = ReadOutputCodeRequest->CodeType;
    switch (CodeType)
    {
        case CODE_ASCII:
            CodeSize = sizeof(CHAR);
            break;

        case CODE_UNICODE:
            CodeSize = sizeof(WCHAR);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = sizeof(WORD);
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&ReadOutputCodeRequest->pCode.pCode,
                                  ReadOutputCodeRequest->NumCodesToRead,
                                  CodeSize))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), ReadOutputCodeRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    ReadBuffer = ReadOutputCodeRequest->pCode.pCode;
    Xpos = ReadOutputCodeRequest->ReadCoord.X;
    Ypos = (ReadOutputCodeRequest->ReadCoord.Y + Buff->VirtualY) % Buff->ScreenBufferSize.Y;

    /*
     * MSDN (ReadConsoleOutputAttribute and ReadConsoleOutputCharacter) :
     *
     * If the number of attributes (resp. characters) to be read from extends
     * beyond the end of the specified screen buffer row, attributes (resp.
     * characters) are read from the next row. If the number of attributes
     * (resp. characters) to be read from extends beyond the end of the console
     * screen buffer, attributes (resp. characters) up to the end of the console
     * screen buffer are read.
     *
     * TODO: Do NOT loop up to NumCodesToRead, but stop before
     * if we are going to overflow...
     */
    for (i = 0; i < ReadOutputCodeRequest->NumCodesToRead; ++i)
    {
        Code = Buff->Buffer[2 * (Xpos + Ypos * Buff->ScreenBufferSize.X) + (CodeType == CODE_ATTRIBUTE ? 1 : 0)];

        switch (CodeType)
        {
            case CODE_UNICODE:
                ConsoleAnsiCharToUnicodeChar(Console, (PWCHAR)ReadBuffer, (PCHAR)&Code);
                break;

            case CODE_ASCII:
                *(PCHAR)ReadBuffer = (CHAR)Code;
                break;

            case CODE_ATTRIBUTE:
                *(PWORD)ReadBuffer = (WORD)Code;
                break;
        }
        ReadBuffer = (PVOID)((ULONG_PTR)ReadBuffer + CodeSize);

        Xpos++;

        if (Xpos == Buff->ScreenBufferSize.X)
        {
            Xpos = 0;
            Ypos++;

            if (Ypos == Buff->ScreenBufferSize.Y)
            {
                Ypos = 0;
            }
        }
    }

    // switch (CodeType)
    // {
        // case CODE_UNICODE:
            // *(PWCHAR)ReadBuffer = 0;
            // break;

        // case CODE_ASCII:
            // *(PCHAR)ReadBuffer = 0;
            // break;

        // case CODE_ATTRIBUTE:
            // *(PWORD)ReadBuffer = 0;
            // break;
    // }

    ReadOutputCodeRequest->EndCoord.X = Xpos;
    ReadOutputCodeRequest->EndCoord.Y = (Ypos - Buff->VirtualY + Buff->ScreenBufferSize.Y) % Buff->ScreenBufferSize.Y;

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    ReadOutputCodeRequest->CodesRead = (DWORD)((ULONG_PTR)ReadBuffer - (ULONG_PTR)ReadOutputCodeRequest->pCode.pCode) / CodeSize;
    // <= ReadOutputCodeRequest->NumCodesToRead

    return STATUS_SUCCESS;
}

CSR_API(SrvWriteConsoleOutputString)
{
    NTSTATUS Status;
    PCONSOLE_WRITEOUTPUTCODE WriteOutputCodeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteOutputCodeRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    USHORT CodeType;
    PBYTE Buffer; // PUCHAR
    PCHAR String, tmpString = NULL;
    DWORD X, Y, Length; // , Written = 0;
    ULONG CodeSize;
    SMALL_RECT UpdateRect;

    DPRINT("SrvWriteConsoleOutputString\n");

    CodeType = WriteOutputCodeRequest->CodeType;
    switch (CodeType)
    {
        case CODE_ASCII:
            CodeSize = sizeof(CHAR);
            break;

        case CODE_UNICODE:
            CodeSize = sizeof(WCHAR);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = sizeof(WORD);
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&WriteOutputCodeRequest->pCode.pCode,
                                  WriteOutputCodeRequest->Length,
                                  CodeSize))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                  WriteOutputCodeRequest->OutputHandle,
                                  &Buff,
                                  GENERIC_WRITE,
                                  TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    switch (CodeType)
    {
        case CODE_UNICODE:
        {
            Length = WideCharToMultiByte(Console->OutputCodePage, 0,
                                         (PWCHAR)WriteOutputCodeRequest->pCode.UnicodeChar,
                                         WriteOutputCodeRequest->Length,
                                         NULL, 0, NULL, NULL);
            tmpString = String = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
            if (String)
            {
                WideCharToMultiByte(Console->OutputCodePage, 0,
                                    (PWCHAR)WriteOutputCodeRequest->pCode.UnicodeChar,
                                    WriteOutputCodeRequest->Length,
                                    String, Length, NULL, NULL);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }

            break;
        }

        case CODE_ASCII:
            String = (PCHAR)WriteOutputCodeRequest->pCode.AsciiChar;
            break;

        case CODE_ATTRIBUTE:
        default:
            // *(ReadBuffer++) = Code;
            String = (PCHAR)WriteOutputCodeRequest->pCode.Attribute;
            break;
    }

    if (String && NT_SUCCESS(Status))
    {
        X = WriteOutputCodeRequest->Coord.X;
        Y = (WriteOutputCodeRequest->Coord.Y + Buff->VirtualY) % Buff->ScreenBufferSize.Y;
        Length = WriteOutputCodeRequest->Length;
        Buffer = &Buff->Buffer[2 * (Y * Buff->ScreenBufferSize.X + X) + (CodeType == CODE_ATTRIBUTE ? 1 : 0)];

        while (Length--)
        {
            *Buffer = *String++;
            // ReadBuffer = (PVOID)((ULONG_PTR)ReadBuffer + CodeSize);
            String = (PCHAR)((ULONG_PTR)String + CodeSize);
            // Written++;
            Buffer += 2;
            if (++X == Buff->ScreenBufferSize.X)
            {
                if (++Y == Buff->ScreenBufferSize.Y)
                {
                    Y = 0;
                    Buffer = Buff->Buffer + (CodeType == CODE_ATTRIBUTE ? 1 : 0);
                }
                X = 0;
            }
        }

        if (Buff == Console->ActiveBuffer)
        {
            ConioComputeUpdateRect(Buff, &UpdateRect, &WriteOutputCodeRequest->Coord,
                                   WriteOutputCodeRequest->Length);
            ConioDrawRegion(Console, &UpdateRect);
        }

        // WriteOutputCodeRequest->EndCoord.X = X;
        // WriteOutputCodeRequest->EndCoord.Y = (Y + Buff->ScreenBufferSize.Y - Buff->VirtualY) % Buff->ScreenBufferSize.Y;
    }

    if (tmpString)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, tmpString);
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    // WriteOutputCodeRequest->NrCharactersWritten = Written;
    return Status;
}

CSR_API(SrvFillConsoleOutput)
{
    NTSTATUS Status;
    PCONSOLE_FILLOUTPUTCODE FillOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.FillOutputRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    DWORD X, Y, Length; // , Written = 0;
    USHORT CodeType;
    BYTE Code;
    PBYTE Buffer;
    SMALL_RECT UpdateRect;

    DPRINT("SrvFillConsoleOutput\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), FillOutputRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    CodeType = FillOutputRequest->CodeType;

    X = FillOutputRequest->Coord.X;
    Y = (FillOutputRequest->Coord.Y + Buff->VirtualY) % Buff->ScreenBufferSize.Y;
    Length = FillOutputRequest->Length;
    Buffer = &Buff->Buffer[2 * (Y * Buff->ScreenBufferSize.X + X) + (CodeType == CODE_ATTRIBUTE ? 1 : 0)];

    switch (CodeType)
    {
        case CODE_ASCII:
            Code = (BYTE)FillOutputRequest->Code.AsciiChar;
            break;

        case CODE_UNICODE:
            ConsoleUnicodeCharToAnsiChar(Console, (PCHAR)&Code, &FillOutputRequest->Code.UnicodeChar);
            break;

        case CODE_ATTRIBUTE:
            Code = (BYTE)FillOutputRequest->Code.Attribute;
            break;

        default:
            ConSrvReleaseScreenBuffer(Buff, TRUE);
            return STATUS_INVALID_PARAMETER;
    }

    while (Length--)
    {
        *Buffer = Code;
        Buffer += 2;
        // Written++;
        if (++X == Buff->ScreenBufferSize.X)
        {
            if (++Y == Buff->ScreenBufferSize.Y)
            {
                Y = 0;
                Buffer = Buff->Buffer + (CodeType == CODE_ATTRIBUTE ? 1 : 0);
            }
            X = 0;
        }
    }

    if (Buff == Console->ActiveBuffer)
    {
        ConioComputeUpdateRect(Buff, &UpdateRect, &FillOutputRequest->Coord,
                               FillOutputRequest->Length);
        ConioDrawRegion(Console, &UpdateRect);
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);
/*
    Length = FillOutputRequest->Length;
    FillOutputRequest->NrCharactersWritten = Length;
*/
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleCursorInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CursorInfoRequest;
    PCONSOLE_SCREEN_BUFFER Buff;

    DPRINT("SrvGetConsoleCursorInfo\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), CursorInfoRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    CursorInfoRequest->Info.bVisible = Buff->CursorInfo.bVisible;
    CursorInfoRequest->Info.dwSize = Buff->CursorInfo.dwSize;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleCursorInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CursorInfoRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    DWORD Size;
    BOOL Visible;

    DPRINT("SrvSetConsoleCursorInfo\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), CursorInfoRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    Size = CursorInfoRequest->Info.dwSize;
    Visible = CursorInfoRequest->Info.bVisible;
    if (Size < 1)
    {
        Size = 1;
    }
    if (100 < Size)
    {
        Size = 100;
    }

    if ( (Size != Buff->CursorInfo.dwSize)          ||
         (Visible && ! Buff->CursorInfo.bVisible)   ||
         (! Visible && Buff->CursorInfo.bVisible) )
    {
        Buff->CursorInfo.dwSize = Size;
        Buff->CursorInfo.bVisible = Visible;

        if (!ConioSetCursorInfo(Console, Buff))
        {
            ConSrvReleaseScreenBuffer(Buff, TRUE);
            return STATUS_UNSUCCESSFUL;
        }
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleCursorPosition)
{
    NTSTATUS Status;
    PCONSOLE_SETCURSORPOSITION SetCursorPositionRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetCursorPositionRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    SHORT OldCursorX, OldCursorY;
    SHORT NewCursorX, NewCursorY;

    DPRINT("SrvSetConsoleCursorPosition\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetCursorPositionRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    NewCursorX = SetCursorPositionRequest->Position.X;
    NewCursorY = SetCursorPositionRequest->Position.Y;
    if ( NewCursorX < 0 || NewCursorX >= Buff->ScreenBufferSize.X ||
         NewCursorY < 0 || NewCursorY >= Buff->ScreenBufferSize.Y )
    {
        ConSrvReleaseScreenBuffer(Buff, TRUE);
        return STATUS_INVALID_PARAMETER;
    }
    OldCursorX = Buff->CursorPosition.X;
    OldCursorY = Buff->CursorPosition.Y;
    Buff->CursorPosition.X = NewCursorX;
    Buff->CursorPosition.Y = NewCursorY;
    if (Buff == Console->ActiveBuffer)
    {
        if (!ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY))
        {
            ConSrvReleaseScreenBuffer(Buff, TRUE);
            return STATUS_UNSUCCESSFUL;
        }
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleTextAttribute)
{
    NTSTATUS Status;
    PCONSOLE_SETTEXTATTRIB SetTextAttribRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetTextAttribRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    DPRINT("SrvSetConsoleTextAttribute\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetTextAttribRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    Buff->ScreenDefaultAttrib = SetTextAttribRequest->Attrib;
    if (Buff == Console->ActiveBuffer)
    {
        if (!ConioUpdateScreenInfo(Console, Buff))
        {
            ConSrvReleaseScreenBuffer(Buff, TRUE);
            return STATUS_UNSUCCESSFUL;
        }
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvCreateConsoleScreenBuffer)
{
    NTSTATUS Status;
    PCONSOLE_CREATESCREENBUFFER CreateScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CreateScreenBufferRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    COORD   ScreenBufferSize = {80, 25};
    USHORT  ScreenAttrib     = DEFAULT_SCREEN_ATTRIB;
    USHORT  PopupAttrib      = DEFAULT_POPUP_ATTRIB;
    ULONG   DisplayMode      = CONSOLE_WINDOWED_MODE;
    BOOLEAN IsCursorVisible  = TRUE;
    ULONG   CursorSize       = CSR_DEFAULT_CURSOR_SIZE;

    DPRINT("SrvCreateConsoleScreenBuffer\n");

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /*
    if (Console->ActiveBuffer)
    {
        ScreenBufferSize = Console->ActiveBuffer->ScreenBufferSize;
        if (ScreenBufferSize.X == 0) ScreenBufferSize.X = 80;
        if (ScreenBufferSize.Y == 0) ScreenBufferSize.Y = 25;

        ScreenAttrib = Console->ActiveBuffer->ScreenDefaultAttrib;
        PopupAttrib  = Console->ActiveBuffer->PopupDefaultAttrib;

        IsCursorVisible = Console->ActiveBuffer->CursorInfo.bVisible;
        CursorSize      = Console->ActiveBuffer->CursorInfo.dwSize;
    }
    */

    // This is Windows' behaviour
    {
        ScreenBufferSize = Console->ConsoleSize; // Use the current console size
        if (ScreenBufferSize.X == 0) ScreenBufferSize.X = 1;
        if (ScreenBufferSize.Y == 0) ScreenBufferSize.Y = 1;

        if (Console->ActiveBuffer)
        {
            ScreenAttrib = Console->ActiveBuffer->ScreenDefaultAttrib;
            PopupAttrib  = Console->ActiveBuffer->PopupDefaultAttrib;
            DisplayMode  = Console->ActiveBuffer->DisplayMode;

            IsCursorVisible = Console->ActiveBuffer->CursorInfo.bVisible;
            CursorSize      = Console->ActiveBuffer->CursorInfo.dwSize;
        }
    }

    Status = ConSrvCreateScreenBuffer(Console,
                                      &Buff,
                                      ScreenBufferSize,
                                      ScreenAttrib,
                                      PopupAttrib,
                                      DisplayMode,
                                      IsCursorVisible,
                                      CursorSize);
    if (NT_SUCCESS(Status))
    {
        RtlEnterCriticalSection(&ProcessData->HandleTableLock);

        /* Insert the new handle inside the process handles table */
        Status = ConSrvInsertObject(ProcessData,
                                    &CreateScreenBufferRequest->OutputHandle,
                                    &Buff->Header,
                                    CreateScreenBufferRequest->Access,
                                    CreateScreenBufferRequest->Inheritable,
                                    CreateScreenBufferRequest->ShareMode);

        RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    }

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleScreenBufferInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSCREENBUFFERINFO ScreenBufferInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ScreenBufferInfoRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    PCONSOLE_SCREEN_BUFFER_INFO pInfo = &ScreenBufferInfoRequest->Info;

    DPRINT("SrvGetConsoleScreenBufferInfo\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), ScreenBufferInfoRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    pInfo->dwSize = Buff->ScreenBufferSize;
    pInfo->dwCursorPosition = Buff->CursorPosition;
    pInfo->wAttributes = Buff->ScreenDefaultAttrib;
    pInfo->srWindow.Left = Buff->ShowX;
    pInfo->srWindow.Right = Buff->ShowX + Console->ConsoleSize.X - 1;
    pInfo->srWindow.Top = Buff->ShowY;
    pInfo->srWindow.Bottom = Buff->ShowY + Console->ConsoleSize.Y - 1;
    pInfo->dwMaximumWindowSize = Buff->ScreenBufferSize;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleActiveScreenBuffer)
{
    NTSTATUS Status;
    PCONSOLE_SETACTIVESCREENBUFFER SetScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetScreenBufferRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    DPRINT("SrvSetConsoleActiveScreenBuffer\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetScreenBufferRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    if (Buff == Console->ActiveBuffer)
    {
        ConSrvReleaseScreenBuffer(Buff, TRUE);
        return STATUS_SUCCESS;
    }

    /* If old buffer has no handles, it's now unreferenced */
    if (Console->ActiveBuffer->Header.HandleCount == 0)
    {
        ConioDeleteScreenBuffer(Console->ActiveBuffer);
    }

    /* Tie console to new buffer */
    Console->ActiveBuffer = Buff;

    /* Redraw the console */
    ConioDrawConsole(Console);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvScrollConsoleScreenBuffer)
{
    PCONSOLE_SCROLLSCREENBUFFER ScrollScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ScrollScreenBufferRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;
    SMALL_RECT ScreenBuffer;
    SMALL_RECT SrcRegion;
    SMALL_RECT DstRegion;
    SMALL_RECT UpdateRegion;
    SMALL_RECT ScrollRectangle;
    SMALL_RECT ClipRectangle;
    NTSTATUS Status;
    HANDLE OutputHandle;
    BOOLEAN UseClipRectangle;
    COORD DestinationOrigin;
    CHAR_INFO Fill;
    CHAR FillChar;

    DPRINT("SrvScrollConsoleScreenBuffer\n");

    OutputHandle = ScrollScreenBufferRequest->OutputHandle;
    UseClipRectangle = ScrollScreenBufferRequest->UseClipRectangle;
    DestinationOrigin = ScrollScreenBufferRequest->DestinationOrigin;
    Fill = ScrollScreenBufferRequest->Fill;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    ScrollRectangle = ScrollScreenBufferRequest->ScrollRectangle;

    /* Make sure source rectangle is inside the screen buffer */
    ConioInitRect(&ScreenBuffer, 0, 0, Buff->ScreenBufferSize.Y - 1, Buff->ScreenBufferSize.X - 1);
    if (!ConioGetIntersection(&SrcRegion, &ScreenBuffer, &ScrollRectangle))
    {
        ConSrvReleaseScreenBuffer(Buff, TRUE);
        return STATUS_SUCCESS;
    }

    /* If the source was clipped on the left or top, adjust the destination accordingly */
    if (ScrollRectangle.Left < 0)
    {
        DestinationOrigin.X -= ScrollRectangle.Left;
    }
    if (ScrollRectangle.Top < 0)
    {
        DestinationOrigin.Y -= ScrollRectangle.Top;
    }

    if (UseClipRectangle)
    {
        ClipRectangle = ScrollScreenBufferRequest->ClipRectangle;
        if (!ConioGetIntersection(&ClipRectangle, &ClipRectangle, &ScreenBuffer))
        {
            ConSrvReleaseScreenBuffer(Buff, TRUE);
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

    if (ScrollScreenBufferRequest->Unicode)
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

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleScreenBufferSize)
{
    NTSTATUS Status;
    PCONSOLE_SETSCREENBUFFERSIZE SetScreenBufferSizeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetScreenBufferSizeRequest;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetScreenBufferSizeRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConioResizeBuffer(Buff->Header.Console, Buff, SetScreenBufferSizeRequest->Size);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return Status;
}

/* EOF */
