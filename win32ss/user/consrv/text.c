/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/text.c
 * PURPOSE:         Console Output Functions for text-mode screen-buffers
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
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


/* PRIVATE FUNCTIONS **********************************************************/

CONSOLE_IO_OBJECT_TYPE
TEXTMODE_BUFFER_GetType(PCONSOLE_SCREEN_BUFFER This)
{
    // return This->Header.Type;
    return TEXTMODE_BUFFER;
}

static CONSOLE_SCREEN_BUFFER_VTBL TextVtbl =
{
    TEXTMODE_BUFFER_GetType,
};


static VOID FASTCALL
ClearLineBuffer(PTEXTMODE_SCREEN_BUFFER Buff);


NTSTATUS
CONSOLE_SCREEN_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                                 IN OUT PCONSOLE Console,
                                 IN SIZE_T Size);
VOID
CONSOLE_SCREEN_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer);


NTSTATUS
TEXTMODE_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                           IN OUT PCONSOLE Console,
                           IN PTEXTMODE_BUFFER_INFO TextModeInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTEXTMODE_SCREEN_BUFFER NewBuffer = NULL;

    if (Console == NULL || Buffer == NULL || TextModeInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    *Buffer = NULL;

    Status = CONSOLE_SCREEN_BUFFER_Initialize((PCONSOLE_SCREEN_BUFFER*)&NewBuffer,
                                              Console,
                                              sizeof(TEXTMODE_SCREEN_BUFFER));
    if (!NT_SUCCESS(Status)) return Status;
    NewBuffer->Header.Type = TEXTMODE_BUFFER;
    NewBuffer->Vtbl = &TextVtbl;

    NewBuffer->Buffer = ConsoleAllocHeap(HEAP_ZERO_MEMORY,
                                         TextModeInfo->ScreenBufferSize.X *
                                         TextModeInfo->ScreenBufferSize.Y *
                                            sizeof(CHAR_INFO));
    if (NewBuffer->Buffer == NULL)
    {
        CONSOLE_SCREEN_BUFFER_Destroy((PCONSOLE_SCREEN_BUFFER)NewBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewBuffer->ScreenBufferSize = NewBuffer->OldScreenBufferSize
                                = TextModeInfo->ScreenBufferSize;
    NewBuffer->ViewSize = NewBuffer->OldViewSize
                        = Console->ConsoleSize;

    NewBuffer->ViewOrigin.X = NewBuffer->ViewOrigin.Y = 0;
    NewBuffer->VirtualY = 0;

    NewBuffer->CursorBlinkOn = NewBuffer->ForceCursorOff = FALSE;
    NewBuffer->CursorInfo.bVisible = (TextModeInfo->IsCursorVisible && (TextModeInfo->CursorSize != 0));
    NewBuffer->CursorInfo.dwSize   = min(max(TextModeInfo->CursorSize, 0), 100);

    NewBuffer->ScreenDefaultAttrib = TextModeInfo->ScreenAttrib;
    NewBuffer->PopupDefaultAttrib  = TextModeInfo->PopupAttrib;

    /* Initialize buffer to be empty with default attributes */
    for (NewBuffer->CursorPosition.Y = 0 ; NewBuffer->CursorPosition.Y < NewBuffer->ScreenBufferSize.Y; NewBuffer->CursorPosition.Y++)
    {
        ClearLineBuffer(NewBuffer);
    }
    NewBuffer->CursorPosition.X = NewBuffer->CursorPosition.Y = 0;

    NewBuffer->Mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;

    *Buffer = (PCONSOLE_SCREEN_BUFFER)NewBuffer;
    return STATUS_SUCCESS;
}

VOID
TEXTMODE_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer)
{
    PTEXTMODE_SCREEN_BUFFER Buff = (PTEXTMODE_SCREEN_BUFFER)Buffer;

    /*
     * IMPORTANT !! Reinitialize the type so that we don't enter a recursive
     * infinite loop when calling CONSOLE_SCREEN_BUFFER_Destroy.
     */
    Buffer->Header.Type = SCREEN_BUFFER;

    ConsoleFreeHeap(Buff->Buffer);

    CONSOLE_SCREEN_BUFFER_Destroy(Buffer);
}


PCHAR_INFO
ConioCoordToPointer(PTEXTMODE_SCREEN_BUFFER Buff, ULONG X, ULONG Y)
{
    return &Buff->Buffer[((Y + Buff->VirtualY) % Buff->ScreenBufferSize.Y) * Buff->ScreenBufferSize.X + X];
}

static VOID FASTCALL
ClearLineBuffer(PTEXTMODE_SCREEN_BUFFER Buff)
{
    PCHAR_INFO Ptr = ConioCoordToPointer(Buff, 0, Buff->CursorPosition.Y);
    SHORT Pos;

    for (Pos = 0; Pos < Buff->ScreenBufferSize.X; Pos++, Ptr++)
    {
        /* Fill the cell */
        Ptr->Char.UnicodeChar = L' ';
        Ptr->Attributes = Buff->ScreenDefaultAttrib;
    }
}

static __inline BOOLEAN ConioGetIntersection(
    SMALL_RECT* Intersection,
    SMALL_RECT* Rect1,
    SMALL_RECT* Rect2)
{
    if ( ConioIsRectEmpty(Rect1) ||
         ConioIsRectEmpty(Rect2) ||
        (Rect1->Top  > Rect2->Bottom) ||
        (Rect1->Left > Rect2->Right)  ||
        (Rect1->Bottom < Rect2->Top)  ||
        (Rect1->Right  < Rect2->Left) )
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

static __inline BOOLEAN ConioGetUnion(
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

static VOID FASTCALL
ConioComputeUpdateRect(PTEXTMODE_SCREEN_BUFFER Buff, SMALL_RECT* UpdateRect, PCOORD Start, UINT Length)
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

/*
 * Move from one rectangle to another. We must be careful about the order that
 * this is done, to avoid overwriting parts of the source before they are moved.
 */
static VOID FASTCALL
ConioMoveRegion(PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                SMALL_RECT* SrcRegion,
                SMALL_RECT* DstRegion,
                SMALL_RECT* ClipRegion,
                CHAR_INFO FillChar)
{
    int Width  = ConioRectWidth(SrcRegion);
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
        PCHAR_INFO SRow = ConioCoordToPointer(ScreenBuffer, 0, SY);
        PCHAR_INFO DRow = ConioCoordToPointer(ScreenBuffer, 0, DY);

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
            CHAR_INFO Cell = SRow[SX];
            if (SX >= ClipRegion->Left && SX <= ClipRegion->Right &&
                SY >= ClipRegion->Top  && SY <= ClipRegion->Bottom)
            {
                SRow[SX] = FillChar;
            }
            if (DX >= ClipRegion->Left && DX <= ClipRegion->Right &&
                DY >= ClipRegion->Top  && DY <= ClipRegion->Bottom)
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

DWORD FASTCALL
ConioEffectiveCursorSize(PCONSOLE Console, DWORD Scale)
{
    DWORD Size = (Console->ActiveBuffer->CursorInfo.dwSize * Scale + 99) / 100;
    /* If line input in progress, perhaps adjust for insert toggle */
    if (Console->LineBuffer && !Console->LineComplete && Console->LineInsertToggle)
        return (Size * 2 <= Scale) ? (Size * 2) : (Size / 2);
    return Size;
}

NTSTATUS
ConioResizeBuffer(PCONSOLE Console,
                  PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                  COORD Size)
{
    PCHAR_INFO Buffer;
    DWORD Offset = 0;
    PCHAR_INFO ptr;
    USHORT CurrentY;
    PCHAR_INFO OldBuffer;
#ifdef HAVE_WMEMSET
    USHORT value = MAKEWORD(' ', ScreenBuffer->ScreenDefaultAttrib);
#else
    DWORD i;
#endif
    DWORD diff;

    /* Buffer size is not allowed to be smaller than the view size */
    if (Size.X < ScreenBuffer->ViewSize.X || Size.Y < ScreenBuffer->ViewSize.Y)
        return STATUS_INVALID_PARAMETER;

    if (Size.X == ScreenBuffer->ScreenBufferSize.X && Size.Y == ScreenBuffer->ScreenBufferSize.Y)
    {
        // FIXME: Trigger a buffer resize event ??
        return STATUS_SUCCESS;
    }

    if (Console->FixedSize)
    {
        /*
         * The console is in fixed-size mode, so we cannot resize anything
         * at the moment. However, keep those settings somewhere so that
         * we can try to set them up when we will be allowed to do so.
         */
        ScreenBuffer->OldScreenBufferSize = Size;
        return STATUS_NOT_SUPPORTED; // STATUS_SUCCESS
    }

    Buffer = ConsoleAllocHeap(HEAP_ZERO_MEMORY, Size.X * Size.Y * sizeof(CHAR_INFO));
    if (!Buffer) return STATUS_NO_MEMORY;

    DPRINT1("Resizing (%d,%d) to (%d,%d)\n", ScreenBuffer->ScreenBufferSize.X, ScreenBuffer->ScreenBufferSize.Y, Size.X, Size.Y);
    OldBuffer = ScreenBuffer->Buffer;

    for (CurrentY = 0; CurrentY < ScreenBuffer->ScreenBufferSize.Y && CurrentY < Size.Y; CurrentY++)
    {
        ptr = ConioCoordToPointer(ScreenBuffer, 0, CurrentY);
        if (Size.X <= ScreenBuffer->ScreenBufferSize.X)
        {
            /* Reduce size */
            RtlCopyMemory(Buffer + Offset, ptr, Size.X * sizeof(CHAR_INFO));
            Offset += Size.X;
        }
        else
        {
            /* Enlarge size */
            RtlCopyMemory(Buffer + Offset, ptr, ScreenBuffer->ScreenBufferSize.X * sizeof(CHAR_INFO));
            Offset += ScreenBuffer->ScreenBufferSize.X;

            diff = Size.X - ScreenBuffer->ScreenBufferSize.X;
            /* Zero new part of it */
#ifdef HAVE_WMEMSET
            wmemset((PWCHAR)&Buffer[Offset], value, diff);
#else
            for (i = 0; i < diff; i++)
            {
                ptr = Buffer + Offset;
                ptr->Char.UnicodeChar = L' ';
                ptr->Attributes = ScreenBuffer->ScreenDefaultAttrib;
                ++Offset;
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
            ptr = Buffer + Offset;
            ptr->Char.UnicodeChar = L' ';
            ptr->Attributes = ScreenBuffer->ScreenDefaultAttrib;
            ++Offset;
        }
#endif
    }

    (void)InterlockedExchangePointer((PVOID volatile*)&ScreenBuffer->Buffer, Buffer);
    ConsoleFreeHeap(OldBuffer);
    ScreenBuffer->ScreenBufferSize = ScreenBuffer->OldScreenBufferSize = Size;
    ScreenBuffer->VirtualY = 0;

    /* Ensure cursor and window are within buffer */
    if (ScreenBuffer->CursorPosition.X >= Size.X)
        ScreenBuffer->CursorPosition.X = Size.X - 1;
    if (ScreenBuffer->CursorPosition.Y >= Size.Y)
        ScreenBuffer->CursorPosition.Y = Size.Y - 1;
    if (ScreenBuffer->ViewOrigin.X > Size.X - ScreenBuffer->ViewSize.X)
        ScreenBuffer->ViewOrigin.X = Size.X - ScreenBuffer->ViewSize.X;
    if (ScreenBuffer->ViewOrigin.Y > Size.Y - ScreenBuffer->ViewSize.Y)
        ScreenBuffer->ViewOrigin.Y = Size.Y - ScreenBuffer->ViewSize.Y;

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

    return STATUS_SUCCESS;
}

static VOID FASTCALL
ConioNextLine(PTEXTMODE_SCREEN_BUFFER Buff, SMALL_RECT* UpdateRect, UINT *ScrolledLines)
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

NTSTATUS
ConioWriteConsole(PCONSOLE Console,
                  PTEXTMODE_SCREEN_BUFFER Buff,
                  PWCHAR Buffer,
                  DWORD Length,
                  BOOL Attrib)
{
    UINT i;
    PCHAR_INFO Ptr;
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
            if (Buffer[i] == L'\r')
            {
                Buff->CursorPosition.X = 0;
                UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);
                UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);
                continue;
            }
            /* --- LF --- */
            else if (Buffer[i] == L'\n')
            {
                Buff->CursorPosition.X = 0;
                ConioNextLine(Buff, &UpdateRect, &ScrolledLines);
                continue;
            }
            /* --- BS --- */
            else if (Buffer[i] == L'\b')
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
                    Ptr->Char.UnicodeChar = L' ';
                    Ptr->Attributes  = Buff->ScreenDefaultAttrib;
                    UpdateRect.Left  = min(UpdateRect.Left, Buff->CursorPosition.X);
                    UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);
                }
                continue;
            }
            /* --- TAB --- */
            else if (Buffer[i] == L'\t')
            {
                UINT EndX;

                UpdateRect.Left = min(UpdateRect.Left, Buff->CursorPosition.X);
                EndX = (Buff->CursorPosition.X + TAB_WIDTH) & ~(TAB_WIDTH - 1);
                EndX = min(EndX, (UINT)Buff->ScreenBufferSize.X);
                Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
                while (Buff->CursorPosition.X < EndX)
                {
                    Ptr->Char.UnicodeChar = L' ';
                    Ptr->Attributes = Buff->ScreenDefaultAttrib;
                    ++Ptr;
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
            // else if (Buffer[i] == L'\a')
            // {
                // // FIXME: This MUST BE moved to the terminal emulator frontend!!
                // DPRINT1("Bell\n");
                // // SendNotifyMessage(Console->hWindow, PM_CONSOLE_BEEP, 0, 0);
                // continue;
            // }
        }
        UpdateRect.Left  = min(UpdateRect.Left, Buff->CursorPosition.X);
        UpdateRect.Right = max(UpdateRect.Right, Buff->CursorPosition.X);

        Ptr = ConioCoordToPointer(Buff, Buff->CursorPosition.X, Buff->CursorPosition.Y);
        Ptr->Char.UnicodeChar = Buffer[i];
        if (Attrib) Ptr->Attributes = Buff->ScreenDefaultAttrib;

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

    if (!ConioIsRectEmpty(&UpdateRect) && (PCONSOLE_SCREEN_BUFFER)Buff == Console->ActiveBuffer)
    {
        ConioWriteStream(Console, &UpdateRect, CursorStartX, CursorStartY,
                         ScrolledLines, Buffer, Length);
    }

    return STATUS_SUCCESS;
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
    PTEXTMODE_SCREEN_BUFFER Buff;
    PVOID Buffer;
    DWORD Written = 0;
    ULONG Length;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(ClientThread->Process), WriteConsoleRequest->OutputHandle, &Buff, GENERIC_WRITE, FALSE);
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
            Buffer = WriteConsoleRequest->Buffer;
        }
        else
        {
            Length = MultiByteToWideChar(Console->OutputCodePage, 0,
                                         (PCHAR)WriteConsoleRequest->Buffer,
                                         WriteConsoleRequest->NrCharactersToWrite,
                                         NULL, 0);
            Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
            if (Buffer)
            {
                MultiByteToWideChar(Console->OutputCodePage, 0,
                                    (PCHAR)WriteConsoleRequest->Buffer,
                                    WriteConsoleRequest->NrCharactersToWrite,
                                    (PWCHAR)Buffer, Length);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }

        if (Buffer)
        {
            if (NT_SUCCESS(Status))
            {
                Status = ConioWriteConsole(Console,
                                           Buff,
                                           Buffer,
                                           WriteConsoleRequest->NrCharactersToWrite,
                                           TRUE);
                if (NT_SUCCESS(Status))
                {
                    Written = WriteConsoleRequest->NrCharactersToWrite;
                }
            }

            if (!WriteConsoleRequest->Unicode)
                RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
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
    PTEXTMODE_SCREEN_BUFFER Buff;
    SHORT SizeX, SizeY;
    NTSTATUS Status;
    COORD BufferSize;
    COORD BufferCoord;
    SMALL_RECT ReadRegion;
    SMALL_RECT ScreenRect;
    DWORD i;
    PCHAR_INFO Ptr;
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

    Status = ConSrvGetTextModeBuffer(ProcessData, ReadOutputRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
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
                CurCharInfo->Char.UnicodeChar = Ptr->Char.UnicodeChar;
            }
            else
            {
                // ConsoleUnicodeCharToAnsiChar(ProcessData->Console, &CurCharInfo->Char.AsciiChar, &Ptr->Char.UnicodeChar);
                WideCharToMultiByte(CodePage, 0, &Ptr->Char.UnicodeChar, 1,
                                    &CurCharInfo->Char.AsciiChar, 1, NULL, NULL);
            }
            CurCharInfo->Attributes = Ptr->Attributes;
            ++Ptr;
            ++CurCharInfo;
        }
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    ReadOutputRequest->ReadRegion.Right  = ReadRegion.Left + SizeX - 1;
    ReadOutputRequest->ReadRegion.Bottom = ReadRegion.Top  + SizeY - 1;
    ReadOutputRequest->ReadRegion.Left   = ReadRegion.Left;
    ReadOutputRequest->ReadRegion.Top    = ReadRegion.Top;

    return STATUS_SUCCESS;
}

CSR_API(SrvWriteConsoleOutput)
{
    PCONSOLE_WRITEOUTPUT WriteOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteOutputRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    SHORT i, X, Y, SizeX, SizeY;
    PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;
    SMALL_RECT ScreenBuffer;
    PCHAR_INFO CurCharInfo;
    SMALL_RECT WriteRegion;
    PCHAR_INFO CharInfo;
    COORD BufferCoord;
    COORD BufferSize;
    NTSTATUS Status;
    PCHAR_INFO Ptr;

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

    Status = ConSrvGetTextModeBuffer(ProcessData,
                                     WriteOutputRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_WRITE,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    WriteRegion = WriteOutputRequest->WriteRegion;

    SizeY = min(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&WriteRegion));
    SizeX = min(BufferSize.X - BufferCoord.X, ConioRectWidth(&WriteRegion));
    WriteRegion.Bottom = WriteRegion.Top  + SizeY - 1;
    WriteRegion.Right  = WriteRegion.Left + SizeX - 1;

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
            if (WriteOutputRequest->Unicode)
            {
                Ptr->Char.UnicodeChar = CurCharInfo->Char.UnicodeChar;
            }
            else
            {
                ConsoleAnsiCharToUnicodeChar(Console, &Ptr->Char.UnicodeChar, &CurCharInfo->Char.AsciiChar);
            }
            Ptr->Attributes = CurCharInfo->Attributes;
            ++Ptr;
            ++CurCharInfo;
        }
    }

    ConioDrawRegion(Console, &WriteRegion);

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    WriteOutputRequest->WriteRegion.Right  = WriteRegion.Left + SizeX - 1;
    WriteOutputRequest->WriteRegion.Bottom = WriteRegion.Top  + SizeY - 1;
    WriteOutputRequest->WriteRegion.Left   = WriteRegion.Left;
    WriteOutputRequest->WriteRegion.Top    = WriteRegion.Top;

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

CSR_API(SrvReadConsoleOutputString)
{
    NTSTATUS Status;
    PCONSOLE_READOUTPUTCODE ReadOutputCodeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadOutputCodeRequest;
    PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;
    USHORT CodeType;
    SHORT Xpos, Ypos;
    PVOID ReadBuffer;
    DWORD i;
    ULONG CodeSize;
    PCHAR_INFO Ptr;

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

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     ReadOutputCodeRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
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
    // Ptr = ConioCoordToPointer(Buff, Xpos, Ypos); // Doesn't work
    for (i = 0; i < min(ReadOutputCodeRequest->NumCodesToRead, Buff->ScreenBufferSize.X * Buff->ScreenBufferSize.Y); ++i)
    {
        // Ptr = ConioCoordToPointer(Buff, Xpos, Ypos); // Doesn't work either
        Ptr = &Buff->Buffer[Xpos + Ypos * Buff->ScreenBufferSize.X];

        switch (CodeType)
        {
            case CODE_ASCII:
                ConsoleUnicodeCharToAnsiChar(Console, (PCHAR)ReadBuffer, &Ptr->Char.UnicodeChar);
                break;

            case CODE_UNICODE:
                *(PWCHAR)ReadBuffer = Ptr->Char.UnicodeChar;
                break;

            case CODE_ATTRIBUTE:
                *(PWORD)ReadBuffer = Ptr->Attributes;
                break;
        }
        ReadBuffer = (PVOID)((ULONG_PTR)ReadBuffer + CodeSize);
        // ++Ptr;

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
    PTEXTMODE_SCREEN_BUFFER Buff;
    USHORT CodeType;
    PVOID ReadBuffer = NULL;
    PWCHAR tmpString = NULL;
    DWORD X, Y, Length; // , Written = 0;
    ULONG CodeSize;
    SMALL_RECT UpdateRect;
    PCHAR_INFO Ptr;

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

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     WriteOutputCodeRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_WRITE,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    if (CodeType == CODE_ASCII)
    {
        /* Convert the ASCII string into Unicode before writing it to the console */
        Length = MultiByteToWideChar(Console->OutputCodePage, 0,
                                     WriteOutputCodeRequest->pCode.AsciiChar,
                                     WriteOutputCodeRequest->Length,
                                     NULL, 0);
        tmpString = ReadBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
        if (ReadBuffer)
        {
            MultiByteToWideChar(Console->OutputCodePage, 0,
                                WriteOutputCodeRequest->pCode.AsciiChar,
                                WriteOutputCodeRequest->Length,
                                (PWCHAR)ReadBuffer, Length);
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        /* For CODE_UNICODE or CODE_ATTRIBUTE, we are already OK */
        ReadBuffer = WriteOutputCodeRequest->pCode.pCode;
    }

    if (ReadBuffer == NULL || !NT_SUCCESS(Status)) goto Cleanup;

    X = WriteOutputCodeRequest->Coord.X;
    Y = (WriteOutputCodeRequest->Coord.Y + Buff->VirtualY) % Buff->ScreenBufferSize.Y;
    Length = WriteOutputCodeRequest->Length;
    // Ptr = ConioCoordToPointer(Buff, X, Y); // Doesn't work
    // Ptr = &Buff->Buffer[X + Y * Buff->ScreenBufferSize.X]; // May work

    while (Length--)
    {
        // Ptr = ConioCoordToPointer(Buff, X, Y); // Doesn't work either
        Ptr = &Buff->Buffer[X + Y * Buff->ScreenBufferSize.X];

        switch (CodeType)
        {
            case CODE_ASCII:
            case CODE_UNICODE:
                Ptr->Char.UnicodeChar = *(PWCHAR)ReadBuffer;
                break;

            case CODE_ATTRIBUTE:
                Ptr->Attributes = *(PWORD)ReadBuffer;
                break;
        }
        ReadBuffer = (PVOID)((ULONG_PTR)ReadBuffer + CodeSize);
        // ++Ptr;

        // Written++;
        if (++X == Buff->ScreenBufferSize.X)
        {
            X = 0;

            if (++Y == Buff->ScreenBufferSize.Y)
            {
                Y = 0;
            }
        }
    }

    if ((PCONSOLE_SCREEN_BUFFER)Buff == Console->ActiveBuffer)
    {
        ConioComputeUpdateRect(Buff, &UpdateRect, &WriteOutputCodeRequest->Coord,
                               WriteOutputCodeRequest->Length);
        ConioDrawRegion(Console, &UpdateRect);
    }

    // WriteOutputCodeRequest->EndCoord.X = X;
    // WriteOutputCodeRequest->EndCoord.Y = (Y + Buff->ScreenBufferSize.Y - Buff->VirtualY) % Buff->ScreenBufferSize.Y;

Cleanup:
    if (tmpString)
        RtlFreeHeap(RtlGetProcessHeap(), 0, tmpString);

    ConSrvReleaseScreenBuffer(Buff, TRUE);

    // WriteOutputCodeRequest->NrCharactersWritten = Written;
    return Status;
}

CSR_API(SrvFillConsoleOutput)
{
    NTSTATUS Status;
    PCONSOLE_FILLOUTPUTCODE FillOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.FillOutputRequest;
    PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;
    DWORD X, Y, Length; // , Written = 0;
    USHORT CodeType;
    PVOID Code = NULL;
    PCHAR_INFO Ptr;
    SMALL_RECT UpdateRect;

    DPRINT("SrvFillConsoleOutput\n");

    CodeType = FillOutputRequest->CodeType;
    if ( (CodeType != CODE_ASCII    ) &&
         (CodeType != CODE_UNICODE  ) &&
         (CodeType != CODE_ATTRIBUTE) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     FillOutputRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_WRITE,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    switch (CodeType)
    {
        case CODE_ASCII:
            /* On-place conversion from the ASCII char to the UNICODE char */
            ConsoleAnsiCharToUnicodeChar(Console, &FillOutputRequest->Code.UnicodeChar, &FillOutputRequest->Code.AsciiChar);
        /* Fall through */
        case CODE_UNICODE:
            Code = &FillOutputRequest->Code.UnicodeChar;
            break;

        case CODE_ATTRIBUTE:
            Code = &FillOutputRequest->Code.Attribute;
            break;
    }

    X = FillOutputRequest->Coord.X;
    Y = (FillOutputRequest->Coord.Y + Buff->VirtualY) % Buff->ScreenBufferSize.Y;
    Length = FillOutputRequest->Length;
    // Ptr = ConioCoordToPointer(Buff, X, Y); // Doesn't work
    // Ptr = &Buff->Buffer[X + Y * Buff->ScreenBufferSize.X]; // May work

    while (Length--)
    {
        // Ptr = ConioCoordToPointer(Buff, X, Y); // Doesn't work either
        Ptr = &Buff->Buffer[X + Y * Buff->ScreenBufferSize.X];

        switch (CodeType)
        {
            case CODE_ASCII:
            case CODE_UNICODE:
                Ptr->Char.UnicodeChar = *(PWCHAR)Code;
                break;

            case CODE_ATTRIBUTE:
                Ptr->Attributes = *(PWORD)Code;
                break;
        }
        // ++Ptr;

        // Written++;
        if (++X == Buff->ScreenBufferSize.X)
        {
            X = 0;

            if (++Y == Buff->ScreenBufferSize.Y)
            {
                Y = 0;
            }
        }
    }

    if ((PCONSOLE_SCREEN_BUFFER)Buff == Console->ActiveBuffer)
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

CSR_API(SrvGetConsoleScreenBufferInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSCREENBUFFERINFO ScreenBufferInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ScreenBufferInfoRequest;
    // PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;
    PCONSOLE_SCREEN_BUFFER_INFO pInfo = &ScreenBufferInfoRequest->Info;

    DPRINT("SrvGetConsoleScreenBufferInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), ScreenBufferInfoRequest->OutputHandle, &Buff, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    // Console = Buff->Header.Console;

    pInfo->dwSize = Buff->ScreenBufferSize;
    pInfo->dwCursorPosition = Buff->CursorPosition;
    pInfo->wAttributes      = Buff->ScreenDefaultAttrib;
    pInfo->srWindow.Left    = Buff->ViewOrigin.X;
    pInfo->srWindow.Top     = Buff->ViewOrigin.Y;
    pInfo->srWindow.Right   = Buff->ViewOrigin.X + Buff->ViewSize.X - 1;
    pInfo->srWindow.Bottom  = Buff->ViewOrigin.Y + Buff->ViewSize.Y - 1;
    pInfo->dwMaximumWindowSize = Buff->ScreenBufferSize; // TODO: Refine the computation

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleTextAttribute)
{
    NTSTATUS Status;
    PCONSOLE_SETTEXTATTRIB SetTextAttribRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetTextAttribRequest;
    PTEXTMODE_SCREEN_BUFFER Buff;

    DPRINT("SrvSetConsoleTextAttribute\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetTextAttribRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Buff->ScreenDefaultAttrib = SetTextAttribRequest->Attrib;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleScreenBufferSize)
{
    NTSTATUS Status;
    PCONSOLE_SETSCREENBUFFERSIZE SetScreenBufferSizeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetScreenBufferSizeRequest;
    PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), SetScreenBufferSizeRequest->OutputHandle, &Buff, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    Status = ConioResizeBuffer(Console, Buff, SetScreenBufferSizeRequest->Size);
    if (NT_SUCCESS(Status)) ConioResizeTerminal(Console);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return Status;
}

CSR_API(SrvScrollConsoleScreenBuffer)
{
    PCONSOLE_SCROLLSCREENBUFFER ScrollScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ScrollScreenBufferRequest;
    PCONSOLE Console;
    PTEXTMODE_SCREEN_BUFFER Buff;
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
    CHAR_INFO FillChar;

    DPRINT("SrvScrollConsoleScreenBuffer\n");

    OutputHandle = ScrollScreenBufferRequest->OutputHandle;
    UseClipRectangle = ScrollScreenBufferRequest->UseClipRectangle;
    DestinationOrigin = ScrollScreenBufferRequest->DestinationOrigin;
    FillChar = ScrollScreenBufferRequest->Fill;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), OutputHandle, &Buff, GENERIC_WRITE, TRUE);
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

    if (!ScrollScreenBufferRequest->Unicode)
        ConsoleAnsiCharToUnicodeChar(Console, &FillChar.Char.UnicodeChar, &FillChar.Char.AsciiChar);

    ConioMoveRegion(Buff, &SrcRegion, &DstRegion, &ClipRectangle, FillChar);

    if ((PCONSOLE_SCREEN_BUFFER)Buff == Console->ActiveBuffer)
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

/* EOF */
