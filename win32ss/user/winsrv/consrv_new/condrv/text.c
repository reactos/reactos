/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            win32ss/user/winsrv/consrv_new/condrv/text.c
 * PURPOSE:         Console Output Functions for text-mode screen-buffers
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/conio2.h"
#include "conoutput.h"
#include "handle.h"

#define NDEBUG
#include <debug.h>


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

static __inline BOOLEAN
ConioGetIntersection(OUT PSMALL_RECT Intersection,
                     IN PSMALL_RECT Rect1,
                     IN PSMALL_RECT Rect2)
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
                  max(Rect1->Top   , Rect2->Top   ),
                  max(Rect1->Left  , Rect2->Left  ),
                  min(Rect1->Bottom, Rect2->Bottom),
                  min(Rect1->Right , Rect2->Right ));

    return TRUE;
}

static __inline BOOLEAN
ConioGetUnion(OUT PSMALL_RECT Union,
              IN PSMALL_RECT Rect1,
              IN PSMALL_RECT Rect2)
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
                      min(Rect1->Top   , Rect2->Top   ),
                      min(Rect1->Left  , Rect2->Left  ),
                      max(Rect1->Bottom, Rect2->Bottom),
                      max(Rect1->Right , Rect2->Right ));
    }

    return TRUE;
}

static VOID FASTCALL
ConioComputeUpdateRect(IN PTEXTMODE_SCREEN_BUFFER Buff,
                       IN OUT PSMALL_RECT UpdateRect,
                       IN PCOORD Start,
                       IN UINT Length)
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
                PSMALL_RECT SrcRegion,
                PSMALL_RECT DstRegion,
                PSMALL_RECT ClipRegion,
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
    WORD CurrentAttribute;
    USHORT CurrentY;
    PCHAR_INFO OldBuffer;
    DWORD i;
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

            /* The attribute to be used is the one of the last cell of the current line */
            CurrentAttribute = ConioCoordToPointer(ScreenBuffer,
                                                   ScreenBuffer->ScreenBufferSize.X - 1,
                                                   CurrentY)->Attributes;

            diff = Size.X - ScreenBuffer->ScreenBufferSize.X;

            /* Zero-out the new part of the buffer */
            for (i = 0; i < diff; i++)
            {
                ptr = Buffer + Offset;
                ptr->Char.UnicodeChar = L' ';
                ptr->Attributes = CurrentAttribute;
                ++Offset;
            }
        }
    }

    if (Size.Y > ScreenBuffer->ScreenBufferSize.Y)
    {
        diff = Size.X * (Size.Y - ScreenBuffer->ScreenBufferSize.Y);

        /* Zero-out the new part of the buffer */
        for (i = 0; i < diff; i++)
        {
            ptr = Buffer + Offset;
            ptr->Char.UnicodeChar = L' ';
            ptr->Attributes = ScreenBuffer->ScreenDefaultAttrib;
            ++Offset;
        }
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
ConioNextLine(PTEXTMODE_SCREEN_BUFFER Buff, PSMALL_RECT UpdateRect, PUINT ScrolledLines)
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


/* PUBLIC DRIVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvReadConsoleOutput(IN PCONSOLE Console,
                        IN PTEXTMODE_SCREEN_BUFFER Buffer,
                        IN BOOLEAN Unicode,
                        OUT PCHAR_INFO CharInfo/*Buffer*/,
                        IN PCOORD BufferSize,
                        IN PCOORD BufferCoord,
                        IN OUT PSMALL_RECT ReadRegion)
{
    PCHAR_INFO CurCharInfo;
    SHORT SizeX, SizeY;
    SMALL_RECT CapturedReadRegion;
    SMALL_RECT ScreenRect;
    DWORD i;
    PCHAR_INFO Ptr;
    LONG X, Y;
    UINT CodePage;

    if (Console == NULL || Buffer == NULL || CharInfo == NULL ||
        BufferSize == NULL || BufferCoord == NULL || ReadRegion == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    CapturedReadRegion = *ReadRegion;

    /* FIXME: Is this correct? */
    CodePage = Console->OutputCodePage;

    SizeX = min(BufferSize->X - BufferCoord->X, ConioRectWidth(&CapturedReadRegion));
    SizeY = min(BufferSize->Y - BufferCoord->Y, ConioRectHeight(&CapturedReadRegion));
    CapturedReadRegion.Right  = CapturedReadRegion.Left + SizeX;
    CapturedReadRegion.Bottom = CapturedReadRegion.Top  + SizeY;

    ConioInitRect(&ScreenRect, 0, 0, Buffer->ScreenBufferSize.Y, Buffer->ScreenBufferSize.X);
    if (!ConioGetIntersection(&CapturedReadRegion, &ScreenRect, &CapturedReadRegion))
    {
        return STATUS_SUCCESS;
    }

    for (i = 0, Y = CapturedReadRegion.Top; Y < CapturedReadRegion.Bottom; ++i, ++Y)
    {
        CurCharInfo = CharInfo + (i * BufferSize->X);

        Ptr = ConioCoordToPointer(Buffer, CapturedReadRegion.Left, Y);
        for (X = CapturedReadRegion.Left; X < CapturedReadRegion.Right; ++X)
        {
            if (Unicode)
            {
                CurCharInfo->Char.UnicodeChar = Ptr->Char.UnicodeChar;
            }
            else
            {
                // ConsoleUnicodeCharToAnsiChar(Console, &CurCharInfo->Char.AsciiChar, &Ptr->Char.UnicodeChar);
                WideCharToMultiByte(CodePage, 0, &Ptr->Char.UnicodeChar, 1,
                                    &CurCharInfo->Char.AsciiChar, 1, NULL, NULL);
            }
            CurCharInfo->Attributes = Ptr->Attributes;
            ++Ptr;
            ++CurCharInfo;
        }
    }

    ReadRegion->Left   = CapturedReadRegion.Left;
    ReadRegion->Top    = CapturedReadRegion.Top ;
    ReadRegion->Right  = CapturedReadRegion.Left + SizeX - 1;
    ReadRegion->Bottom = CapturedReadRegion.Top  + SizeY - 1;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvWriteConsoleOutput(IN PCONSOLE Console,
                         IN PTEXTMODE_SCREEN_BUFFER Buffer,
                         IN BOOLEAN Unicode,
                         IN PCHAR_INFO CharInfo/*Buffer*/,
                         IN PCOORD BufferSize,
                         IN PCOORD BufferCoord,
                         IN OUT PSMALL_RECT WriteRegion)
{
    SHORT i, X, Y, SizeX, SizeY;
    SMALL_RECT ScreenBuffer;
    PCHAR_INFO CurCharInfo;
    SMALL_RECT CapturedWriteRegion;
    PCHAR_INFO Ptr;

    if (Console == NULL || Buffer == NULL || CharInfo == NULL ||
        BufferSize == NULL || BufferCoord == NULL || WriteRegion == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    CapturedWriteRegion = *WriteRegion;

    SizeX = min(BufferSize->X - BufferCoord->X, ConioRectWidth(&CapturedWriteRegion));
    SizeY = min(BufferSize->Y - BufferCoord->Y, ConioRectHeight(&CapturedWriteRegion));
    CapturedWriteRegion.Right  = CapturedWriteRegion.Left + SizeX - 1;
    CapturedWriteRegion.Bottom = CapturedWriteRegion.Top  + SizeY - 1;

    /* Make sure WriteRegion is inside the screen buffer */
    ConioInitRect(&ScreenBuffer, 0, 0, Buffer->ScreenBufferSize.Y - 1, Buffer->ScreenBufferSize.X - 1);
    if (!ConioGetIntersection(&CapturedWriteRegion, &ScreenBuffer, &CapturedWriteRegion))
    {
        /*
         * It is okay to have a WriteRegion completely outside
         * the screen buffer. No data is written then.
         */
        return STATUS_SUCCESS;
    }

    for (i = 0, Y = CapturedWriteRegion.Top; Y <= CapturedWriteRegion.Bottom; i++, Y++)
    {
        CurCharInfo = CharInfo + (i + BufferCoord->Y) * BufferSize->X + BufferCoord->X;

        Ptr = ConioCoordToPointer(Buffer, CapturedWriteRegion.Left, Y);
        for (X = CapturedWriteRegion.Left; X <= CapturedWriteRegion.Right; X++)
        {
            if (Unicode)
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

    ConioDrawRegion(Console, &CapturedWriteRegion);

    WriteRegion->Left   = CapturedWriteRegion.Left;
    WriteRegion->Top    = CapturedWriteRegion.Top ;
    WriteRegion->Right  = CapturedWriteRegion.Left + SizeX - 1;
    WriteRegion->Bottom = CapturedWriteRegion.Top  + SizeY - 1;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvWriteConsole(IN PCONSOLE Console,
                   IN PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                   IN BOOLEAN Unicode,
                   IN PVOID StringBuffer,
                   IN ULONG NumCharsToWrite,
                   OUT PULONG NumCharsWritten OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWCHAR Buffer = NULL;
    ULONG Written = 0;
    ULONG Length;

    if (Console == NULL || ScreenBuffer == NULL /* || StringBuffer == NULL */)
        return STATUS_INVALID_PARAMETER;

    /* Validity checks */
    ASSERT(Console == ScreenBuffer->Header.Console);
    ASSERT( (StringBuffer != NULL && NumCharsToWrite >= 0) ||
            (StringBuffer == NULL && NumCharsToWrite == 0) );

    // if (Console->PauseFlags & (PAUSED_FROM_KEYBOARD | PAUSED_FROM_SCROLLBAR | PAUSED_FROM_SELECTION))
    if (Console->PauseFlags && Console->UnpauseEvent != NULL)
    {
        return STATUS_PENDING;
    }

    if (Unicode)
    {
        Buffer = StringBuffer;
    }
    else
    {
        Length = MultiByteToWideChar(Console->OutputCodePage, 0,
                                     (PCHAR)StringBuffer,
                                     NumCharsToWrite,
                                     NULL, 0);
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
        if (Buffer)
        {
            MultiByteToWideChar(Console->OutputCodePage, 0,
                                (PCHAR)StringBuffer,
                                NumCharsToWrite,
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
                                       ScreenBuffer,
                                       Buffer,
                                       NumCharsToWrite,
                                       TRUE);
            if (NT_SUCCESS(Status))
            {
                Written = NumCharsToWrite;
            }
        }

        if (!Unicode) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    }

    if (NumCharsWritten) *NumCharsWritten = Written;

    return Status;
}

NTSTATUS NTAPI
ConDrvReadConsoleOutputString(IN PCONSOLE Console,
                              IN PTEXTMODE_SCREEN_BUFFER Buffer,
                              IN CODE_TYPE CodeType,
                              OUT PVOID StringBuffer,
                              IN ULONG NumCodesToRead,
                              IN PCOORD ReadCoord,
                              OUT PCOORD EndCoord,
                              OUT PULONG CodesRead)
{
    SHORT Xpos, Ypos;
    PVOID ReadBuffer;
    ULONG i;
    ULONG CodeSize;
    PCHAR_INFO Ptr;

    if (Console == NULL || Buffer == NULL ||
        ReadCoord == NULL || EndCoord == NULL || CodesRead == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity checks */
    ASSERT(Console == Buffer->Header.Console);
    ASSERT( (StringBuffer != NULL && NumCodesToRead >= 0) ||
            (StringBuffer == NULL && NumCodesToRead == 0) );

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

    ReadBuffer = StringBuffer;
    Xpos = ReadCoord->X;
    Ypos = (ReadCoord->Y + Buffer->VirtualY) % Buffer->ScreenBufferSize.Y;

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
    // Ptr = ConioCoordToPointer(Buffer, Xpos, Ypos); // Doesn't work
    for (i = 0; i < min(NumCodesToRead, Buffer->ScreenBufferSize.X * Buffer->ScreenBufferSize.Y); ++i)
    {
        // Ptr = ConioCoordToPointer(Buffer, Xpos, Ypos); // Doesn't work either
        Ptr = &Buffer->Buffer[Xpos + Ypos * Buffer->ScreenBufferSize.X];

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

        if (Xpos == Buffer->ScreenBufferSize.X)
        {
            Xpos = 0;
            Ypos++;

            if (Ypos == Buffer->ScreenBufferSize.Y)
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

    EndCoord->X = Xpos;
    EndCoord->Y = (Ypos - Buffer->VirtualY + Buffer->ScreenBufferSize.Y) % Buffer->ScreenBufferSize.Y;

    *CodesRead = (ULONG)((ULONG_PTR)ReadBuffer - (ULONG_PTR)StringBuffer) / CodeSize;
    // <= NumCodesToRead

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvWriteConsoleOutputString(IN PCONSOLE Console,
                               IN PTEXTMODE_SCREEN_BUFFER Buffer,
                               IN CODE_TYPE CodeType,
                               IN PVOID StringBuffer,
                               IN ULONG NumCodesToWrite,
                               IN PCOORD WriteCoord /*,
                               OUT PCOORD EndCoord,
                               OUT PULONG CodesWritten */)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID WriteBuffer = NULL;
    PWCHAR tmpString = NULL;
    DWORD X, Y, Length; // , Written = 0;
    ULONG CodeSize;
    SMALL_RECT UpdateRect;
    PCHAR_INFO Ptr;

    if (Console == NULL || Buffer == NULL ||
        WriteCoord == NULL /* || EndCoord == NULL || CodesWritten == NULL */)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity checks */
    ASSERT(Console == Buffer->Header.Console);
    ASSERT( (StringBuffer != NULL && NumCodesToWrite >= 0) ||
            (StringBuffer == NULL && NumCodesToWrite == 0) );

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

    if (CodeType == CODE_ASCII)
    {
        /* Convert the ASCII string into Unicode before writing it to the console */
        Length = MultiByteToWideChar(Console->OutputCodePage, 0,
                                     (PCHAR)StringBuffer,
                                     NumCodesToWrite,
                                     NULL, 0);
        tmpString = WriteBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
        if (WriteBuffer)
        {
            MultiByteToWideChar(Console->OutputCodePage, 0,
                                (PCHAR)StringBuffer,
                                NumCodesToWrite,
                                (PWCHAR)WriteBuffer, Length);
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        /* For CODE_UNICODE or CODE_ATTRIBUTE, we are already OK */
        WriteBuffer = StringBuffer;
    }

    if (WriteBuffer == NULL || !NT_SUCCESS(Status)) goto Cleanup;

    X = WriteCoord->X;
    Y = (WriteCoord->Y + Buffer->VirtualY) % Buffer->ScreenBufferSize.Y;
    Length = NumCodesToWrite;
    // Ptr = ConioCoordToPointer(Buffer, X, Y); // Doesn't work
    // Ptr = &Buffer->Buffer[X + Y * Buffer->ScreenBufferSize.X]; // May work

    while (Length--)
    {
        // Ptr = ConioCoordToPointer(Buffer, X, Y); // Doesn't work either
        Ptr = &Buffer->Buffer[X + Y * Buffer->ScreenBufferSize.X];

        switch (CodeType)
        {
            case CODE_ASCII:
            case CODE_UNICODE:
                Ptr->Char.UnicodeChar = *(PWCHAR)WriteBuffer;
                break;

            case CODE_ATTRIBUTE:
                Ptr->Attributes = *(PWORD)WriteBuffer;
                break;
        }
        WriteBuffer = (PVOID)((ULONG_PTR)WriteBuffer + CodeSize);
        // ++Ptr;

        // Written++;
        if (++X == Buffer->ScreenBufferSize.X)
        {
            X = 0;

            if (++Y == Buffer->ScreenBufferSize.Y)
            {
                Y = 0;
            }
        }
    }

    if ((PCONSOLE_SCREEN_BUFFER)Buffer == Console->ActiveBuffer)
    {
        ConioComputeUpdateRect(Buffer, &UpdateRect, WriteCoord, NumCodesToWrite);
        ConioDrawRegion(Console, &UpdateRect);
    }

    // EndCoord->X = X;
    // EndCoord->Y = (Y + Buffer->ScreenBufferSize.Y - Buffer->VirtualY) % Buffer->ScreenBufferSize.Y;

Cleanup:
    if (tmpString) RtlFreeHeap(RtlGetProcessHeap(), 0, tmpString);

    // CodesWritten = Written;
    return Status;
}

NTSTATUS NTAPI
ConDrvFillConsoleOutput(IN PCONSOLE Console,
                        IN PTEXTMODE_SCREEN_BUFFER Buffer,
                        IN CODE_TYPE CodeType,
                        IN PVOID Code,
                        IN ULONG NumCodesToWrite,
                        IN PCOORD WriteCoord /*,
                        OUT PULONG CodesWritten */)
{
    DWORD X, Y, Length; // , Written = 0;
    PCHAR_INFO Ptr;
    SMALL_RECT UpdateRect;

    if (Console == NULL || Buffer == NULL || Code == NULL ||
        WriteCoord == NULL /* || CodesWritten == NULL */)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

#if 0
    switch (CodeType)
    {
        case CODE_ASCII:
            /* On-place conversion from the ASCII char to the UNICODE char */
            ConsoleAnsiCharToUnicodeChar(Console, &Code->UnicodeChar, &Code->AsciiChar);
        /* Fall through */
        case CODE_UNICODE:
            Code = &Code->UnicodeChar;
            break;

        case CODE_ATTRIBUTE:
            Code = &Code->Attribute;
            break;
    }
#else
    if (CodeType == CODE_ASCII)
    {
        /* On-place conversion from the ASCII char to the UNICODE char */
        // FIXME: What if Code points to an invalid memory zone ??
        ConsoleAnsiCharToUnicodeChar(Console, (PWCHAR)Code, (PCHAR)Code);
    }
#endif

    X = WriteCoord->X;
    Y = (WriteCoord->Y + Buffer->VirtualY) % Buffer->ScreenBufferSize.Y;
    Length = NumCodesToWrite;
    // Ptr = ConioCoordToPointer(Buffer, X, Y); // Doesn't work
    // Ptr = &Buffer->Buffer[X + Y * Buffer->ScreenBufferSize.X]; // May work

    while (Length--)
    {
        // Ptr = ConioCoordToPointer(Buffer, X, Y); // Doesn't work either
        Ptr = &Buffer->Buffer[X + Y * Buffer->ScreenBufferSize.X];

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
        if (++X == Buffer->ScreenBufferSize.X)
        {
            X = 0;

            if (++Y == Buffer->ScreenBufferSize.Y)
            {
                Y = 0;
            }
        }
    }

    if ((PCONSOLE_SCREEN_BUFFER)Buffer == Console->ActiveBuffer)
    {
        ConioComputeUpdateRect(Buffer, &UpdateRect, WriteCoord, NumCodesToWrite);
        ConioDrawRegion(Console, &UpdateRect);
    }

    // CodesWritten = Written; // NumCodesToWrite;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleScreenBufferInfo(IN PCONSOLE Console,
                                 IN PTEXTMODE_SCREEN_BUFFER Buffer,
                                 OUT PCONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo)
{
    if (Console == NULL || Buffer == NULL || ScreenBufferInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    ScreenBufferInfo->dwSize              = Buffer->ScreenBufferSize;
    ScreenBufferInfo->dwCursorPosition    = Buffer->CursorPosition;
    ScreenBufferInfo->wAttributes         = Buffer->ScreenDefaultAttrib;
    ScreenBufferInfo->srWindow.Left       = Buffer->ViewOrigin.X;
    ScreenBufferInfo->srWindow.Top        = Buffer->ViewOrigin.Y;
    ScreenBufferInfo->srWindow.Right      = Buffer->ViewOrigin.X + Buffer->ViewSize.X - 1;
    ScreenBufferInfo->srWindow.Bottom     = Buffer->ViewOrigin.Y + Buffer->ViewSize.Y - 1;

    // FIXME: Refine the computation
    ScreenBufferInfo->dwMaximumWindowSize = Buffer->ScreenBufferSize;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsoleTextAttribute(IN PCONSOLE Console,
                              IN PTEXTMODE_SCREEN_BUFFER Buffer,
                              IN WORD Attribute)
{
    if (Console == NULL || Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    Buffer->ScreenDefaultAttrib = Attribute;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsoleScreenBufferSize(IN PCONSOLE Console,
                                 IN PTEXTMODE_SCREEN_BUFFER Buffer,
                                 IN PCOORD Size)
{
    NTSTATUS Status;

    if (Console == NULL || Buffer == NULL || Size == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    Status = ConioResizeBuffer(Console, Buffer, *Size);
    if (NT_SUCCESS(Status)) ConioResizeTerminal(Console);

    return Status;
}

NTSTATUS NTAPI
ConDrvScrollConsoleScreenBuffer(IN PCONSOLE Console,
                                IN PTEXTMODE_SCREEN_BUFFER Buffer,
                                IN BOOLEAN Unicode,
                                IN PSMALL_RECT ScrollRectangle,
                                IN BOOLEAN UseClipRectangle,
                                IN PSMALL_RECT ClipRectangle OPTIONAL,
                                IN PCOORD DestinationOrigin,
                                IN CHAR_INFO FillChar)
{
    COORD CapturedDestinationOrigin;
    SMALL_RECT ScreenBuffer;
    SMALL_RECT SrcRegion;
    SMALL_RECT DstRegion;
    SMALL_RECT UpdateRegion;
    SMALL_RECT CapturedClipRectangle;

    if (Console == NULL || Buffer == NULL || ScrollRectangle == NULL ||
        (UseClipRectangle ? ClipRectangle == NULL : FALSE) || DestinationOrigin == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    CapturedDestinationOrigin = *DestinationOrigin;

    /* Make sure the source rectangle is inside the screen buffer */
    ConioInitRect(&ScreenBuffer, 0, 0, Buffer->ScreenBufferSize.Y - 1, Buffer->ScreenBufferSize.X - 1);
    if (!ConioGetIntersection(&SrcRegion, &ScreenBuffer, ScrollRectangle))
    {
        return STATUS_SUCCESS;
    }

    /* If the source was clipped on the left or top, adjust the destination accordingly */
    if (ScrollRectangle->Left < 0)
    {
        CapturedDestinationOrigin.X -= ScrollRectangle->Left;
    }
    if (ScrollRectangle->Top < 0)
    {
        CapturedDestinationOrigin.Y -= ScrollRectangle->Top;
    }

    if (UseClipRectangle)
    {
        CapturedClipRectangle = *ClipRectangle;
        if (!ConioGetIntersection(&CapturedClipRectangle, &CapturedClipRectangle, &ScreenBuffer))
        {
            return STATUS_SUCCESS;
        }
    }
    else
    {
        CapturedClipRectangle = ScreenBuffer;
    }

    ConioInitRect(&DstRegion,
                  CapturedDestinationOrigin.Y,
                  CapturedDestinationOrigin.X,
                  CapturedDestinationOrigin.Y + ConioRectHeight(&SrcRegion) - 1,
                  CapturedDestinationOrigin.X + ConioRectWidth(&SrcRegion ) - 1);

    if (!Unicode)
        ConsoleAnsiCharToUnicodeChar(Console, &FillChar.Char.UnicodeChar, &FillChar.Char.AsciiChar);

    ConioMoveRegion(Buffer, &SrcRegion, &DstRegion, &CapturedClipRectangle, FillChar);

    if ((PCONSOLE_SCREEN_BUFFER)Buffer == Console->ActiveBuffer)
    {
        ConioGetUnion(&UpdateRegion, &SrcRegion, &DstRegion);
        if (ConioGetIntersection(&UpdateRegion, &UpdateRegion, &CapturedClipRectangle))
        {
            /* Draw update region */
            ConioDrawRegion(Console, &UpdateRegion);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsoleWindowInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           IN BOOLEAN Absolute,
                           IN PSMALL_RECT WindowRect)
{
    SMALL_RECT CapturedWindowRect;

    if (Console == NULL || Buffer == NULL || WindowRect == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    CapturedWindowRect = *WindowRect;

    if (Absolute == FALSE)
    {
        /* Relative positions given. Transform them to absolute ones */
        CapturedWindowRect.Left   += Buffer->ViewOrigin.X;
        CapturedWindowRect.Top    += Buffer->ViewOrigin.Y;
        CapturedWindowRect.Right  += Buffer->ViewOrigin.X + Buffer->ViewSize.X - 1;
        CapturedWindowRect.Bottom += Buffer->ViewOrigin.Y + Buffer->ViewSize.Y - 1;
    }

    /* See MSDN documentation on SetConsoleWindowInfo about the performed checks */
    if ( (CapturedWindowRect.Left < 0) || (CapturedWindowRect.Top < 0)  ||
         (CapturedWindowRect.Right  >= Buffer->ScreenBufferSize.X)      ||
         (CapturedWindowRect.Bottom >= Buffer->ScreenBufferSize.Y)      ||
         (CapturedWindowRect.Right  <= CapturedWindowRect.Left)         ||
         (CapturedWindowRect.Bottom <= CapturedWindowRect.Top) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    Buffer->ViewOrigin.X = CapturedWindowRect.Left;
    Buffer->ViewOrigin.Y = CapturedWindowRect.Top;

    Buffer->ViewSize.X = CapturedWindowRect.Right - CapturedWindowRect.Left + 1;
    Buffer->ViewSize.Y = CapturedWindowRect.Bottom - CapturedWindowRect.Top + 1;

    return STATUS_SUCCESS;
}

/* EOF */
