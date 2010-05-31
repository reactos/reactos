/*
 * reactos/subsys/csrss/win32csr/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include "w32csr.h"
#include <debug.h>

/* GLOBALS *******************************************************************/

#define ConioInitRect(Rect, top, left, bottom, right) \
    ((Rect)->Top) = top; \
    ((Rect)->Left) = left; \
    ((Rect)->Bottom) = bottom; \
    ((Rect)->Right) = right

#define ConioIsRectEmpty(Rect) \
    (((Rect)->Left > (Rect)->Right) || ((Rect)->Top > (Rect)->Bottom))

#define ConsoleUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->OutputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->OutputCodePage, 0, (sChar), 1, (dWChar), 1)

/* FUNCTIONS *****************************************************************/

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

NTSTATUS FASTCALL
CsrInitConsoleScreenBuffer(PCSRSS_CONSOLE Console,
                           PCSRSS_SCREEN_BUFFER Buffer)
{
    DPRINT("CsrInitConsoleScreenBuffer Size X %d Size Y %d\n", Buffer->MaxX, Buffer->MaxY);

    Buffer->Header.Type = CONIO_SCREEN_BUFFER_MAGIC;
    Buffer->Header.Console = Console;
    Buffer->Header.HandleCount = 0;
    Buffer->ShowX = 0;
    Buffer->ShowY = 0;
    Buffer->VirtualY = 0;
    Buffer->Buffer = HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, Buffer->MaxX * Buffer->MaxY * 2);
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
    Buffer->Mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    Buffer->CurrentX = 0;
    Buffer->CurrentY = 0;

    InsertHeadList(&Console->BufferList, &Buffer->ListEntry);
    return STATUS_SUCCESS;
}

static VOID FASTCALL
ConioNextLine(PCSRSS_SCREEN_BUFFER Buff, SMALL_RECT *UpdateRect, UINT *ScrolledLines)
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
        if (UpdateRect->Top != 0)
        {
            UpdateRect->Top--;
        }
    }
    UpdateRect->Left = 0;
    UpdateRect->Right = Buff->MaxX - 1;
    UpdateRect->Bottom = Buff->CurrentY;
}

NTSTATUS FASTCALL
ConioWriteConsole(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER Buff,
                  CHAR *Buffer, DWORD Length, BOOL Attrib)
{
    UINT i;
    PBYTE Ptr;
    SMALL_RECT UpdateRect;
    LONG CursorStartX, CursorStartY;
    UINT ScrolledLines;

    CursorStartX = Buff->CurrentX;
    CursorStartY = Buff->CurrentY;
    UpdateRect.Left = Buff->MaxX;
    UpdateRect.Top = Buff->CurrentY;
    UpdateRect.Right = -1;
    UpdateRect.Bottom = Buff->CurrentY;
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
                        UpdateRect.Top = min(UpdateRect.Top, (LONG)Buff->CurrentY);
                    }
                    else
                    {
                        Buff->CurrentX--;
                    }
                    Ptr = ConioCoordToPointer(Buff, Buff->CurrentX, Buff->CurrentY);
                    Ptr[0] = ' ';
                    Ptr[1] = Buff->DefaultAttrib;
                    UpdateRect.Left = min(UpdateRect.Left, (LONG) Buff->CurrentX);
                    UpdateRect.Right = max(UpdateRect.Right, (LONG) Buff->CurrentX);
                }
                continue;
            }
            /* --- CR --- */
            else if (Buffer[i] == '\r')
            {
                Buff->CurrentX = 0;
                UpdateRect.Left = min(UpdateRect.Left, (LONG) Buff->CurrentX);
                UpdateRect.Right = max(UpdateRect.Right, (LONG) Buff->CurrentX);
                continue;
            }
            /* --- TAB --- */
            else if (Buffer[i] == '\t')
            {
                UINT EndX;

                UpdateRect.Left = min(UpdateRect.Left, (LONG)Buff->CurrentX);
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
                UpdateRect.Right = max(UpdateRect.Right, (LONG) Buff->CurrentX - 1);
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
        UpdateRect.Left = min(UpdateRect.Left, (LONG)Buff->CurrentX);
        UpdateRect.Right = max(UpdateRect.Right, (LONG) Buff->CurrentX);
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

__inline BOOLEAN ConioGetIntersection(
    SMALL_RECT *Intersection,
    SMALL_RECT *Rect1,
    SMALL_RECT *Rect2)
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
    SMALL_RECT *Union,
    SMALL_RECT *Rect1,
    SMALL_RECT *Rect2)
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

/* Move from one rectangle to another. We must be careful about the order that
 * this is done, to avoid overwriting parts of the source before they are moved. */
static VOID FASTCALL
ConioMoveRegion(PCSRSS_SCREEN_BUFFER ScreenBuffer,
                SMALL_RECT *SrcRegion,
                SMALL_RECT *DstRegion,
                SMALL_RECT *ClipRegion,
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

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.WriteConsoleRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

    if (Console->UnpauseEvent)
    {
        Status = NtDuplicateObject(GetCurrentProcess(), Console->UnpauseEvent,
                                   ProcessData->Process, &Request->Data.WriteConsoleRequest.UnpauseEvent,
                                   SYNCHRONIZE, 0, 0);
        ConioUnlockScreenBuffer(Buff);
        return NT_SUCCESS(Status) ? STATUS_PENDING : Status;
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
        if (NT_SUCCESS(Status))
        {
            Status = ConioWriteConsole(Console, Buff, Buffer,
                                       Request->Data.WriteConsoleRequest.NrCharactersToWrite, TRUE);
            if (NT_SUCCESS(Status))
            {
                Written = Request->Data.WriteConsoleRequest.NrCharactersToWrite;
            }
        }
        if (Request->Data.WriteConsoleRequest.Unicode)
        {
            RtlFreeHeap(GetProcessHeap(), 0, Buffer);
        }
    }
    ConioUnlockScreenBuffer(Buff);

    Request->Data.WriteConsoleRequest.NrCharactersWritten = Written;

    return Status;
}

VOID WINAPI
ConioDeleteScreenBuffer(PCSRSS_SCREEN_BUFFER Buffer)
{
    PCSRSS_CONSOLE Console = Buffer->Header.Console;

    RemoveEntryList(&Buffer->ListEntry);
    if (Buffer == Console->ActiveBuffer)
    {
        /* Deleted active buffer; switch to most recently created */
        Console->ActiveBuffer = NULL;
        if (!IsListEmpty(&Console->BufferList))
        {
            Console->ActiveBuffer = CONTAINING_RECORD(Console->BufferList.Flink, CSRSS_SCREEN_BUFFER, ListEntry);
            ConioDrawConsole(Console);
        }
    }

    HeapFree(Win32CsrApiHeap, 0, Buffer->Buffer);
    HeapFree(Win32CsrApiHeap, 0, Buffer);
}

VOID FASTCALL
ConioDrawConsole(PCSRSS_CONSOLE Console)
{
    SMALL_RECT Region;

    ConioInitRect(&Region, 0, 0, Console->Size.Y - 1, Console->Size.X - 1);

    ConioDrawRegion(Console, &Region);
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

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.ScreenBufferInfoRequest.ConsoleHandle, &Buff, GENERIC_READ);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;
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

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

    NewCursorX = Request->Data.SetCursorRequest.Position.X;
    NewCursorY = Request->Data.SetCursorRequest.Position.Y;
    if (NewCursorX < 0 || NewCursorX >= Buff->MaxX ||
            NewCursorY < 0 || NewCursorY >= Buff->MaxY)
    {
        ConioUnlockScreenBuffer(Buff);
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
            return STATUS_UNSUCCESSFUL;
        }
    }

    ConioUnlockScreenBuffer(Buff);

    return STATUS_SUCCESS;
}

static VOID FASTCALL
ConioComputeUpdateRect(PCSRSS_SCREEN_BUFFER Buff, SMALL_RECT *UpdateRect, COORD *Start, UINT Length)
{
    if (Buff->MaxX <= Start->X + Length)
    {
        UpdateRect->Left = 0;
    }
    else
    {
        UpdateRect->Left = Start->X;
    }
    if (Buff->MaxX <= Start->X + Length)
    {
        UpdateRect->Right = Buff->MaxX - 1;
    }
    else
    {
        UpdateRect->Right = Start->X + Length - 1;
    }
    UpdateRect->Top = Start->Y;
    UpdateRect->Bottom = Start->Y+ (Start->X + Length - 1) / Buff->MaxX;
    if (Buff->MaxY <= UpdateRect->Bottom)
    {
        UpdateRect->Bottom = Buff->MaxY - 1;
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
    SMALL_RECT UpdateRect;

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

    Status = ConioLockScreenBuffer(ProcessData,
                                    Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle,
                                    &Buff,
                                    GENERIC_WRITE);
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    if (NT_SUCCESS(Status))
    {
        Console = Buff->Header.Console;
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

            }
            if (Request->Data.WriteConsoleRequest.Unicode)
            {
                RtlFreeHeap(GetProcessHeap(), 0, tmpString);
            }
        }
        ConioUnlockScreenBuffer(Buff);
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
    SMALL_RECT UpdateRect;

    DPRINT("CsrFillOutputChar\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.FillOutputRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

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
    Length = Request->Data.FillOutputRequest.Length;
    Request->Data.FillOutputRequest.NrCharactersWritten = Length;
    return STATUS_SUCCESS;
}

CSR_API(CsrWriteConsoleOutputAttrib)
{
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;
    PUCHAR Buffer;
    PWORD Attribute;
    int X, Y, Length;
    NTSTATUS Status;
    SMALL_RECT UpdateRect;

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

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockScreenBuffer(ProcessData,
                                   Request->Data.WriteConsoleOutputAttribRequest.ConsoleHandle,
                                   &Buff,
                                   GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

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
    SMALL_RECT UpdateRect;
    PCSRSS_CONSOLE Console;

    DPRINT("CsrFillOutputAttrib\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Status = ConioLockScreenBuffer(ProcessData, Request->Data.FillOutputAttribRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

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

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorInfoRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

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
            return STATUS_UNSUCCESSFUL;
        }
    }

    ConioUnlockScreenBuffer(Buff);

    return STATUS_SUCCESS;
}

CSR_API(CsrSetTextAttrib)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;

    DPRINT("CsrSetTextAttrib\n");

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, &Buff, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

    Buff->DefaultAttrib = Request->Data.SetAttribRequest.Attrib;
    if (Buff == Console->ActiveBuffer)
    {
        if (! ConioUpdateScreenInfo(Console, Buff))
        {
            ConioUnlockScreenBuffer(Buff);
            return STATUS_UNSUCCESSFUL;
        }
    }

    ConioUnlockScreenBuffer(Buff);

    return STATUS_SUCCESS;
}

CSR_API(CsrCreateScreenBuffer)
{
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;
    NTSTATUS Status;

    DPRINT("CsrCreateScreenBuffer\n");

    RtlEnterCriticalSection(&ProcessData->HandleTableLock);
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
            Buff->CursorInfo.dwSize = CSR_DEFAULT_CURSOR_SIZE;
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
        if (NT_SUCCESS(Status))
        {
            Status = Win32CsrInsertObject(ProcessData,
                                          &Request->Data.CreateScreenBufferRequest.OutputHandle,
                                          &Buff->Header,
                                          Request->Data.CreateScreenBufferRequest.Access,
                                          Request->Data.CreateScreenBufferRequest.Inheritable,
                                          Request->Data.CreateScreenBufferRequest.ShareMode);
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ConioUnlockConsole(Console);
    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
    return Status;
}

CSR_API(CsrSetScreenBuffer)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;

    DPRINT("CsrSetScreenBuffer\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetScreenBufferRequest.OutputHandle, &Buff, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

    if (Buff == Console->ActiveBuffer)
    {
        ConioUnlockScreenBuffer(Buff);
        return STATUS_SUCCESS;
    }

    /* If old buffer has no handles, it's now unreferenced */
    if (Console->ActiveBuffer->Header.HandleCount == 0)
    {
        ConioDeleteScreenBuffer(Console->ActiveBuffer);
    }
    /* tie console to new buffer */
    Console->ActiveBuffer = Buff;
    /* Redraw the console */
    ConioDrawConsole(Console);

    ConioUnlockScreenBuffer(Buff);

    return STATUS_SUCCESS;
}

CSR_API(CsrWriteConsoleOutput)
{
    SHORT i, X, Y, SizeX, SizeY;
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;
    SMALL_RECT ScreenBuffer;
    CHAR_INFO* CurCharInfo;
    SMALL_RECT WriteRegion;
    CHAR_INFO* CharInfo;
    COORD BufferCoord;
    COORD BufferSize;
    NTSTATUS Status;
    PBYTE Ptr;
    DWORD PSize;

    DPRINT("CsrWriteConsoleOutput\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Status = ConioLockScreenBuffer(ProcessData,
                                   Request->Data.WriteConsoleOutputRequest.ConsoleHandle,
                                   &Buff,
                                   GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

    BufferSize = Request->Data.WriteConsoleOutputRequest.BufferSize;
    PSize = BufferSize.X * BufferSize.Y * sizeof(CHAR_INFO);
    BufferCoord = Request->Data.WriteConsoleOutputRequest.BufferCoord;
    CharInfo = Request->Data.WriteConsoleOutputRequest.CharInfo;
    if (((PVOID)CharInfo < ProcessData->CsrSectionViewBase) ||
            (((ULONG_PTR)CharInfo + PSize) >
             ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
        ConioUnlockScreenBuffer(Buff);
        return STATUS_ACCESS_VIOLATION;
    }
    WriteRegion = Request->Data.WriteConsoleOutputRequest.WriteRegion;

    SizeY = min(BufferSize.Y - BufferCoord.Y, ConioRectHeight(&WriteRegion));
    SizeX = min(BufferSize.X - BufferCoord.X, ConioRectWidth(&WriteRegion));
    WriteRegion.Bottom = WriteRegion.Top + SizeY - 1;
    WriteRegion.Right = WriteRegion.Left + SizeX - 1;

    /* Make sure WriteRegion is inside the screen buffer */
    ConioInitRect(&ScreenBuffer, 0, 0, Buff->MaxY - 1, Buff->MaxX - 1);
    if (! ConioGetIntersection(&WriteRegion, &ScreenBuffer, &WriteRegion))
    {
        ConioUnlockScreenBuffer(Buff);

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

    Request->Data.WriteConsoleOutputRequest.WriteRegion.Right = WriteRegion.Left + SizeX - 1;
    Request->Data.WriteConsoleOutputRequest.WriteRegion.Bottom = WriteRegion.Top + SizeY - 1;
    Request->Data.WriteConsoleOutputRequest.WriteRegion.Left = WriteRegion.Left;
    Request->Data.WriteConsoleOutputRequest.WriteRegion.Top = WriteRegion.Top;

    return STATUS_SUCCESS;
}

CSR_API(CsrScrollConsoleScreenBuffer)
{
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;
    SMALL_RECT ScreenBuffer;
    SMALL_RECT SrcRegion;
    SMALL_RECT DstRegion;
    SMALL_RECT UpdateRegion;
    SMALL_RECT ScrollRectangle;
    SMALL_RECT ClipRectangle;
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

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Status = ConioLockScreenBuffer(ProcessData, ConsoleHandle, &Buff, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

    ScrollRectangle = Request->Data.ScrollConsoleScreenBufferRequest.ScrollRectangle;

    /* Make sure source rectangle is inside the screen buffer */
    ConioInitRect(&ScreenBuffer, 0, 0, Buff->MaxY - 1, Buff->MaxX - 1);
    if (! ConioGetIntersection(&SrcRegion, &ScreenBuffer, &ScrollRectangle))
    {
        ConioUnlockScreenBuffer(Buff);
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
        ClipRectangle = Request->Data.ScrollConsoleScreenBufferRequest.ClipRectangle;
        if (!ConioGetIntersection(&ClipRectangle, &ClipRectangle, &ScreenBuffer))
        {
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

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.ReadConsoleOutputCharRequest.ConsoleHandle, &Buff, GENERIC_READ);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

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
    SMALL_RECT ReadRegion;
    SMALL_RECT ScreenRect;
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
    ReadRegion = Request->Data.ReadConsoleOutputRequest.ReadRegion;
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
    ReadRegion.Bottom = ReadRegion.Top + SizeY;
    ReadRegion.Right = ReadRegion.Left + SizeX;

    ConioInitRect(&ScreenRect, 0, 0, Buff->MaxY, Buff->MaxX);
    if (! ConioGetIntersection(&ReadRegion, &ScreenRect, &ReadRegion))
    {
        ConioUnlockScreenBuffer(Buff);
        return STATUS_SUCCESS;
    }

    for (i = 0, Y = ReadRegion.Top; Y < ReadRegion.Bottom; ++i, ++Y)
    {
        CurCharInfo = CharInfo + (i * BufferSize.X);

        Ptr = ConioCoordToPointer(Buff, ReadRegion.Left, Y);
        for (X = ReadRegion.Left; X < ReadRegion.Right; ++X)
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

    Request->Data.ReadConsoleOutputRequest.ReadRegion.Right = ReadRegion.Left + SizeX - 1;
    Request->Data.ReadConsoleOutputRequest.ReadRegion.Bottom = ReadRegion.Top + SizeY - 1;
    Request->Data.ReadConsoleOutputRequest.ReadRegion.Left = ReadRegion.Left;
    Request->Data.ReadConsoleOutputRequest.ReadRegion.Top = ReadRegion.Top;

    return STATUS_SUCCESS;
}

CSR_API(CsrSetScreenBufferSize)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    PCSRSS_SCREEN_BUFFER Buff;

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockScreenBuffer(ProcessData, Request->Data.SetScreenBufferSize.OutputHandle, &Buff, GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Console = Buff->Header.Console;

    Status = ConioResizeBuffer(Console, Buff, Request->Data.SetScreenBufferSize.Size);
    ConioUnlockScreenBuffer(Buff);

    return Status;
}

/* EOF */
