/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "bios.h"
#include "emulator.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"

/* PRIVATE VARIABLES **********************************************************/

static PBIOS_DATA_AREA Bda;
static BYTE BiosKeyboardMap[256];
static HANDLE BiosConsoleInput = INVALID_HANDLE_VALUE;
static HANDLE BiosConsoleOutput = INVALID_HANDLE_VALUE;
static HANDLE BiosGraphicsOutput = NULL;
static LPVOID ConsoleFramebuffer = NULL;
static HANDLE ConsoleMutex = NULL;
static BYTE CurrentVideoMode, CurrentVideoPage;
static BOOLEAN VideoNeedsUpdate = TRUE;
static SMALL_RECT UpdateRectangle = { 0, 0, 0, 0 };
static CONSOLE_SCREEN_BUFFER_INFO BiosSavedBufferInfo;

static VIDEO_MODE VideoModes[] =
{
    /* Width | Height | Text | Bpp   | Gray | Pages | Segment */
    { 40,       25,     TRUE,   16,     TRUE,   8,      0xB800}, /* Mode 00h */
    { 40,       25,     TRUE,   16,     FALSE,  8,      0xB800}, /* Mode 01h */
    { 80,       25,     TRUE,   16,     TRUE,   8,      0xB800}, /* Mode 02h */
    { 80,       25,     TRUE,   16,     FALSE,  8,      0xB800}, /* Mode 03h */
    { 320,      200,    FALSE,  2,      FALSE,  1,      0xB800}, /* Mode 04h */
    { 320,      200,    FALSE,  2,      TRUE,   1,      0xB800}, /* Mode 05h */
    { 640,      200,    FALSE,  1,      FALSE,  1,      0xB800}, /* Mode 06h */
    { 80,       25,     TRUE,   8,      FALSE,  1,      0xB000}, /* Mode 07h */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 08h - not used */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 09h - not used */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 0Ah - not used */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 0Bh - not used */
    { 0,        0,      FALSE,  0,      FALSE,  0,      0x0000}, /* Mode 0Ch - not used */
    { 320,      200,    FALSE,  4,      FALSE,  1,      0xA000}, /* Mode 0Dh */
    { 640,      200,    FALSE,  4,      FALSE,  1,      0xA000}, /* Mode 0Eh */
    { 640,      350,    FALSE,  1,      FALSE,  1,      0xA000}, /* Mode 0Fh */
    { 640,      350,    FALSE,  4,      FALSE,  1,      0xA000}, /* Mode 10h */
    { 640,      480,    FALSE,  1,      FALSE,  1,      0xA000}, /* Mode 11h */
    { 640,      480,    FALSE,  4,      FALSE,  1,      0xA000}, /* Mode 12h */
    { 320,      200,    FALSE,  8,      FALSE,  1,      0xA000}  /* Mode 13h */
};

/* PRIVATE FUNCTIONS **********************************************************/

static DWORD BiosGetVideoPageSize(VOID)
{
    INT i;
    DWORD BufferSize = VideoModes[CurrentVideoMode].Width
                       * VideoModes[CurrentVideoMode].Height
                       * VideoModes[CurrentVideoMode].Bpp
                       / 8;
    
    for (i = 0; i < 32; i++) if ((1 << i) >= BufferSize) break;

    return 1 << i;
}

static BYTE BiosVideoAddressToPage(ULONG Address)
{
    return (Address - BiosGetVideoMemoryStart())
            / BiosGetVideoPageSize();
}

static COORD BiosVideoAddressToCoord(ULONG Address)
{
    COORD Result = {0, 0};
    DWORD PageStart = BiosVideoAddressToPage(Address) * BiosGetVideoPageSize();
    DWORD Offset = Address - BiosGetVideoMemoryStart() - PageStart;

    if (VideoModes[CurrentVideoMode].Text)
    {
        Result.X = (Offset / sizeof(WORD)) % VideoModes[CurrentVideoMode].Width;
        Result.Y = (Offset / sizeof(WORD)) / VideoModes[CurrentVideoMode].Width;
    }
    else
    {
        Result.X = ((Offset * 8) / VideoModes[CurrentVideoMode].Bpp)
                   % VideoModes[CurrentVideoMode].Width;
        Result.Y = ((Offset * 8) / VideoModes[CurrentVideoMode].Bpp)
                   / VideoModes[CurrentVideoMode].Width;
    }

    return Result;
}

static BOOLEAN BiosKbdBufferPush(WORD Data)
{
    /* Get the location of the element after the head */
    WORD NextElement = Bda->KeybdBufferHead + 2;

    /* Wrap it around if it's at or beyond the end */
    if (NextElement >= Bda->KeybdBufferEnd) NextElement = Bda->KeybdBufferStart;

    /* If it's full, fail */
    if (NextElement == Bda->KeybdBufferTail) return FALSE;

    /* Put the value in the queue */
    *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferTail)) = Data;
    Bda->KeybdBufferTail += sizeof(WORD);

    /* Check if we are at, or have passed, the end of the buffer */
    if (Bda->KeybdBufferTail >= Bda->KeybdBufferEnd)
    {
        /* Return it to the beginning */
        Bda->KeybdBufferTail = Bda->KeybdBufferStart;
    }

    /* Return success */
    return TRUE;
}

static BOOLEAN BiosKbdBufferTop(LPWORD Data)
{
    /* If it's empty, fail */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return FALSE;

    /* Otherwise, get the value and return success */
    *Data = *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferHead));

    return TRUE;
}

static BOOLEAN BiosKbdBufferPop(VOID)
{
    /* If it's empty, fail */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return FALSE;

    /* Remove the value from the queue */
    Bda->KeybdBufferHead += sizeof(WORD);

    /* Check if we are at, or have passed, the end of the buffer */
    if (Bda->KeybdBufferHead >= Bda->KeybdBufferEnd)
    {
        /* Return it to the beginning */
        Bda->KeybdBufferHead = Bda->KeybdBufferStart;
    }

    /* Return success */
    return TRUE;
}

static BOOLEAN BiosCreateGraphicsBuffer(BYTE ModeNumber)
{
    INT i;
    CONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo;
    LPBITMAPINFO BitmapInfo;
    LPWORD PaletteIndex;

    /* Allocate a bitmap info structure */
    BitmapInfo = (LPBITMAPINFO)HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         sizeof(BITMAPINFOHEADER)
                                         + (1 << VideoModes[ModeNumber].Bpp)
                                         * sizeof(WORD));
    if (BitmapInfo == NULL) return FALSE;

    /* Fill the bitmap info header */
    ZeroMemory(&BitmapInfo->bmiHeader, sizeof(BITMAPINFOHEADER));
    BitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    BitmapInfo->bmiHeader.biWidth = VideoModes[ModeNumber].Width;
    BitmapInfo->bmiHeader.biHeight = VideoModes[ModeNumber].Height;
    BitmapInfo->bmiHeader.biPlanes = 1;
    BitmapInfo->bmiHeader.biCompression = BI_RGB;
    BitmapInfo->bmiHeader.biBitCount = VideoModes[ModeNumber].Bpp;

    /* Calculate the image size */
    BitmapInfo->bmiHeader.biSizeImage = BitmapInfo->bmiHeader.biWidth
                                        * BitmapInfo->bmiHeader.biHeight
                                        * (BitmapInfo->bmiHeader.biBitCount >> 3);

    /* Fill the palette data */
    PaletteIndex = (LPWORD)((ULONG_PTR)BitmapInfo + sizeof(BITMAPINFOHEADER));
    for (i = 0; i < (1 << VideoModes[ModeNumber].Bpp); i++)
    {
        PaletteIndex[i] = i;
    }

    /* Fill the console graphics buffer info */
    GraphicsBufferInfo.dwBitMapInfoLength = sizeof(BITMAPINFOHEADER)
                                            + (1 << VideoModes[ModeNumber].Bpp)
                                            * sizeof(WORD);
    GraphicsBufferInfo.lpBitMapInfo = BitmapInfo;
    GraphicsBufferInfo.dwUsage = DIB_PAL_COLORS;

    /* Create the buffer */
    BiosGraphicsOutput = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                   NULL,
                                                   CONSOLE_GRAPHICS_BUFFER,
                                                   &GraphicsBufferInfo);

    /* Save the framebuffer address and mutex */
    ConsoleFramebuffer = GraphicsBufferInfo.lpBitMap;
    ConsoleMutex = GraphicsBufferInfo.hMutex;

    /* Free the bitmap information */
    HeapFree(GetProcessHeap(), 0, BitmapInfo);

    return TRUE;
}

static VOID BiosDestroyGraphicsBuffer(VOID)
{
    CloseHandle(ConsoleMutex);
    CloseHandle(BiosGraphicsOutput);
}

/* PUBLIC FUNCTIONS ***********************************************************/

BYTE BiosGetVideoMode(VOID)
{
    return CurrentVideoMode;
}

BOOLEAN BiosSetVideoMode(BYTE ModeNumber)
{
    COORD Coord;

    /* Make sure this is a valid video mode */
    if (ModeNumber > BIOS_MAX_VIDEO_MODE) return FALSE;
    if (VideoModes[ModeNumber].Pages == 0) return FALSE;

    /* Set the new video mode size */
    Coord.X = VideoModes[ModeNumber].Width;
    Coord.Y = VideoModes[ModeNumber].Height;

    if (VideoModes[ModeNumber].Text && VideoModes[CurrentVideoMode].Text)
    {
        /* Switching from text mode to another text mode */

        /* Resize the text buffer */
        SetConsoleScreenBufferSize(BiosConsoleOutput, Coord);
    }
    else if (VideoModes[ModeNumber].Text && !VideoModes[CurrentVideoMode].Text)
    {
        /* Switching from graphics mode to text mode */

        /* Resize the text buffer */
        SetConsoleScreenBufferSize(BiosConsoleOutput, Coord);

        /* Change the active screen buffer to the text buffer */
        SetConsoleActiveScreenBuffer(BiosConsoleOutput);

        /* Cleanup the graphics buffer */
        BiosDestroyGraphicsBuffer();
    }
    else if (!VideoModes[ModeNumber].Text && VideoModes[CurrentVideoMode].Text)
    {
        /* Switching from text mode to graphics mode */
        if (!BiosCreateGraphicsBuffer(ModeNumber)) return FALSE;

        SetConsoleActiveScreenBuffer(BiosGraphicsOutput);
    }
    else if (!VideoModes[ModeNumber].Text && !VideoModes[CurrentVideoMode].Text)
    {
        /* Switching from graphics mode to another graphics mode */
    
        /* Temporarily switch to the text mode buffer */
        SetConsoleActiveScreenBuffer(BiosConsoleOutput);

        /* Cleanup the current graphics mode buffer */
        BiosDestroyGraphicsBuffer();

        /* Create a new graphics mode buffer */
        if (!BiosCreateGraphicsBuffer(ModeNumber)) return FALSE;

        /* Switch to it */
        SetConsoleActiveScreenBuffer(BiosGraphicsOutput);
    }

    /* Change the mode number */
    CurrentVideoMode = ModeNumber;
    CurrentVideoPage = 0;

    /* Update the BDA */
    Bda->VideoMode = CurrentVideoMode;
    Bda->VideoPage = CurrentVideoPage;
    Bda->VideoPageSize = BiosGetVideoPageSize();
    Bda->VideoPageOffset = 0;
    Bda->ScreenColumns = VideoModes[ModeNumber].Width;

    return TRUE;
}

BOOLEAN BiosSetVideoPage(BYTE PageNumber)
{
    ULONG PageStart;
    COORD Coordinates;
    CONSOLE_SCREEN_BUFFER_INFO BufferInfo;

    /* Make sure this is a valid page number */
    if (PageNumber >= VideoModes[CurrentVideoMode].Pages) return FALSE;

    /* Save the current console buffer in the video memory */
    PageStart = BiosGetVideoMemoryStart() + CurrentVideoPage * BiosGetVideoPageSize();
    BiosUpdateVideoMemory(PageStart, PageStart + BiosGetVideoPageSize());

    /* Save the cursor */
    if (!GetConsoleScreenBufferInfo(BiosConsoleOutput, &BufferInfo)) return FALSE;
    Bda->CursorPosition[CurrentVideoPage] = MAKEWORD(BufferInfo.dwCursorPosition.X,
                                                     BufferInfo.dwCursorPosition.Y);

    /* Set the page */
    CurrentVideoPage = PageNumber;

    /* Update the BDA */
    Bda->VideoPage = CurrentVideoPage;
    Bda->VideoPageSize = BiosGetVideoPageSize();
    Bda->VideoPageOffset = CurrentVideoPage * Bda->VideoPageSize;

    /* Update the console */
    PageStart = BiosGetVideoMemoryStart() + Bda->VideoPage * BiosGetVideoPageSize();
    BiosUpdateConsole(PageStart, PageStart + BiosGetVideoPageSize());

    /* Set the cursor */
    Coordinates.X = LOBYTE(Bda->CursorPosition[Bda->VideoPage]);
    Coordinates.Y = HIBYTE(Bda->CursorPosition[Bda->VideoPage]);
    SetConsoleCursorPosition(BiosConsoleOutput, Coordinates);

    return TRUE;
}

inline DWORD BiosGetVideoMemoryStart(VOID)
{
    return (VideoModes[CurrentVideoMode].Segment << 4);
}

inline VOID BiosVerticalRefresh(VOID)
{
    /* Ignore if we're in text mode */
    if (VideoModes[CurrentVideoMode].Text) return;

    /* Ignore if there's nothing to update */
    if (!VideoNeedsUpdate) return;

    /* Redraw the screen */
    InvalidateConsoleDIBits(BiosGraphicsOutput, &UpdateRectangle);

    /* Clear the update flag */
    VideoNeedsUpdate = FALSE;
}

BOOLEAN BiosInitialize(VOID)
{
    INT i;
    WORD Offset = 0;
    LPWORD IntVecTable = (LPWORD)((ULONG_PTR)BaseAddress);
    LPBYTE BiosCode = (LPBYTE)((ULONG_PTR)BaseAddress + TO_LINEAR(BIOS_SEGMENT, 0));

    /* Initialize the BDA */
    Bda = (PBIOS_DATA_AREA)((ULONG_PTR)BaseAddress + TO_LINEAR(BDA_SEGMENT, 0));
    Bda->EquipmentList = BIOS_EQUIPMENT_LIST;
    Bda->KeybdBufferStart = FIELD_OFFSET(BIOS_DATA_AREA, KeybdBuffer);
    Bda->KeybdBufferEnd = Bda->KeybdBufferStart + BIOS_KBD_BUFFER_SIZE * sizeof(WORD);

    /* Generate ISR stubs and fill the IVT */
    for (i = 0; i < 256; i++)
    {
        IntVecTable[i * 2] = Offset;
        IntVecTable[i * 2 + 1] = BIOS_SEGMENT;

        BiosCode[Offset++] = 0xFA; // cli

        BiosCode[Offset++] = 0x6A; // push i
        BiosCode[Offset++] = (BYTE)i;

        BiosCode[Offset++] = LOBYTE(EMULATOR_BOP); // BOP sequence
        BiosCode[Offset++] = HIBYTE(EMULATOR_BOP);
        BiosCode[Offset++] = LOBYTE(EMULATOR_INT_BOP);
        BiosCode[Offset++] = HIBYTE(EMULATOR_INT_BOP);

        BiosCode[Offset++] = 0x83; // add sp, 2
        BiosCode[Offset++] = 0xC4;
        BiosCode[Offset++] = 0x02;

        BiosCode[Offset++] = 0xCF; // iret
    }

    /* Get the input and output handles to the real console */
    BiosConsoleInput = CreateFile(TEXT("CONIN$"),
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL);

    BiosConsoleOutput = CreateFile(TEXT("CONOUT$"),
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);

    /* Make sure it was successful */
    if ((BiosConsoleInput == INVALID_HANDLE_VALUE)
        || (BiosConsoleOutput == INVALID_HANDLE_VALUE))
    {
        return FALSE;
    }

    /* Save the console screen buffer information */
    if (!GetConsoleScreenBufferInfo(BiosConsoleOutput, &BiosSavedBufferInfo))
    {
        return FALSE;
    }

    /* Store the cursor position */
    Bda->CursorPosition[0] = MAKEWORD(BiosSavedBufferInfo.dwCursorPosition.X,
                                      BiosSavedBufferInfo.dwCursorPosition.Y);
    
    /* Set the default video mode */
    BiosSetVideoMode(BIOS_DEFAULT_VIDEO_MODE);

    /* Set the console input mode */
    SetConsoleMode(BiosConsoleInput, ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);

    /* Initialize the PIC */
    PicWriteCommand(PIC_MASTER_CMD, PIC_ICW1 | PIC_ICW1_ICW4);
    PicWriteCommand(PIC_SLAVE_CMD, PIC_ICW1 | PIC_ICW1_ICW4);

    /* Set the interrupt offsets */
    PicWriteData(PIC_MASTER_DATA, BIOS_PIC_MASTER_INT);
    PicWriteData(PIC_SLAVE_DATA, BIOS_PIC_SLAVE_INT);

    /* Tell the master PIC there is a slave at IRQ 2 */
    PicWriteData(PIC_MASTER_DATA, 1 << 2);
    PicWriteData(PIC_SLAVE_DATA, 2);

    /* Make sure the PIC is in 8086 mode */
    PicWriteData(PIC_MASTER_DATA, PIC_ICW4_8086);
    PicWriteData(PIC_SLAVE_DATA, PIC_ICW4_8086);

    /* Clear the masks for both PICs */
    PicWriteData(PIC_MASTER_DATA, 0x00);
    PicWriteData(PIC_SLAVE_DATA, 0x00);

    PitWriteCommand(0x34);
    PitWriteData(0, 0x00);
    PitWriteData(0, 0x00);

    return TRUE;
}

VOID BiosCleanup(VOID)
{
    /* Restore the old screen buffer */
    SetConsoleActiveScreenBuffer(BiosConsoleOutput);

    /* Restore the screen buffer size */
    SetConsoleScreenBufferSize(BiosConsoleOutput, BiosSavedBufferInfo.dwSize);

    /* Free the graphics buffer */
    if (!VideoModes[CurrentVideoMode].Text) BiosDestroyGraphicsBuffer();

    /* Close the console handles */
    if (BiosConsoleInput != INVALID_HANDLE_VALUE) CloseHandle(BiosConsoleInput);
    if (BiosConsoleOutput != INVALID_HANDLE_VALUE) CloseHandle(BiosConsoleOutput);
}

VOID BiosUpdateConsole(ULONG StartAddress, ULONG EndAddress)
{
    ULONG i;
    COORD Coordinates;
    COORD Origin = { 0, 0 };
    COORD UnitSize = { 1, 1 };
    CHAR_INFO Character;
    SMALL_RECT Rect;

    /* Start from the character address */
    StartAddress &= ~1;

    if (VideoModes[CurrentVideoMode].Text)
    {
        /* Loop through all the addresses */
        for (i = StartAddress; i < EndAddress; i += 2)
        {
            /* Get the coordinates */
            Coordinates = BiosVideoAddressToCoord(i);

            /* Make sure this is the current page */
            if (BiosVideoAddressToPage(i) != CurrentVideoPage) continue;

            /* Fill the rectangle structure */
            Rect.Left = Coordinates.X;
            Rect.Top = Coordinates.Y;
            Rect.Right = Rect.Left;
            Rect.Bottom = Rect.Top;

            /* Fill the character data */
            Character.Char.AsciiChar = *((PCHAR)((ULONG_PTR)BaseAddress + i));
            Character.Attributes = *((PBYTE)((ULONG_PTR)BaseAddress + i + 1));

            /* Write the character */
            WriteConsoleOutputA(BiosConsoleOutput,
                                &Character,
                                UnitSize,
                                Origin,
                                &Rect);
        }
    }
    else
    {
        /* Wait for the mutex object */
        WaitForSingleObject(ConsoleMutex, INFINITE);

        /* Copy the data to the framebuffer */
        RtlCopyMemory((LPVOID)((ULONG_PTR)ConsoleFramebuffer
                      + StartAddress - BiosGetVideoMemoryStart()),
                      (LPVOID)((ULONG_PTR)BaseAddress + StartAddress),
                      EndAddress - StartAddress);

        /* Release the mutex */
        ReleaseMutex(ConsoleMutex);

        /* Check if this is the first time the rectangle is updated */
        if (!VideoNeedsUpdate)
        {
            UpdateRectangle.Left = UpdateRectangle.Top = (SHORT)0x7FFF;
            UpdateRectangle.Right = UpdateRectangle.Bottom = (SHORT)0x8000;
        }

        /* Expand the update rectangle */
        for (i = StartAddress; i < EndAddress; i++)
        {
            /* Get the coordinates */
            Coordinates = BiosVideoAddressToCoord(i);

            /* Expand the rectangle to include the point */
            UpdateRectangle.Left = min(UpdateRectangle.Left, Coordinates.X);
            UpdateRectangle.Right = max(UpdateRectangle.Right, Coordinates.X);
            UpdateRectangle.Top = min(UpdateRectangle.Top, Coordinates.Y);
            UpdateRectangle.Bottom = max(UpdateRectangle.Bottom, Coordinates.Y);
        }

        /* Set the update flag */
        VideoNeedsUpdate = TRUE;
    }
}

VOID BiosUpdateVideoMemory(ULONG StartAddress, ULONG EndAddress)
{
    ULONG i;
    COORD Coordinates;
    WORD Attribute;
    DWORD CharsWritten;

    if (VideoModes[CurrentVideoMode].Text)
    {
        /* Loop through all the addresses */
        for (i = StartAddress; i < EndAddress; i++)
        {
            /* Get the coordinates */
            Coordinates = BiosVideoAddressToCoord(i);

            /* Make sure this is the current page */
            if (BiosVideoAddressToPage(i) != CurrentVideoPage) continue;

            /* Check if this is a character byte or an attribute byte */
            if ((i - BiosGetVideoMemoryStart()) % 2 == 0)
            {
                /* This is a regular character */
                ReadConsoleOutputCharacterA(BiosConsoleOutput,
                                            (LPSTR)((ULONG_PTR)BaseAddress + i),
                                            sizeof(CHAR),
                                            Coordinates,
                                            &CharsWritten);
            }
            else
            {
                /*  This is an attribute */
                ReadConsoleOutputAttribute(BiosConsoleOutput,
                                           &Attribute,
                                           sizeof(CHAR),
                                           Coordinates,
                                           &CharsWritten);

                *(PCHAR)((ULONG_PTR)BaseAddress + i) = LOBYTE(Attribute);
            }
        }
    }
    else
    {
        /* Wait for the mutex object */
        WaitForSingleObject(ConsoleMutex, INFINITE);

        /* Copy the data to the emulator memory */
        RtlCopyMemory((LPVOID)((ULONG_PTR)BaseAddress + StartAddress),
                      (LPVOID)((ULONG_PTR)ConsoleFramebuffer
                      + StartAddress - BiosGetVideoMemoryStart()),
                      EndAddress - StartAddress);

        /* Release the mutex */
        ReleaseMutex(ConsoleMutex);
    }
}

WORD BiosPeekCharacter(VOID)
{
    WORD CharacterData;
    
    /* Check if there is a key available */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return 0xFFFF;

    /* Get the key from the queue, but don't remove it */
    BiosKbdBufferTop(&CharacterData);

    return CharacterData;
}

WORD BiosGetCharacter(VOID)
{
    WORD CharacterData;
    INPUT_RECORD InputRecord;
    DWORD Count;

    /* Check if there is a key available */
    if (Bda->KeybdBufferHead != Bda->KeybdBufferTail)
    {
        /* Get the key from the queue, and remove it */
        BiosKbdBufferTop(&CharacterData);
        BiosKbdBufferPop();
    }
    else
    {
        while (TRUE)
        {
            /* Wait for a console event */
            WaitForSingleObject(BiosConsoleInput, INFINITE);
    
            /* Read the event, and make sure it's a keypress */
            if (!ReadConsoleInput(BiosConsoleInput, &InputRecord, 1, &Count)) continue;
            if (InputRecord.EventType != KEY_EVENT) continue;
            if (!InputRecord.Event.KeyEvent.bKeyDown) continue;

            /* Save the scan code and end the loop */
            CharacterData = (InputRecord.Event.KeyEvent.wVirtualScanCode << 8)
                            | InputRecord.Event.KeyEvent.uChar.AsciiChar;

            break;
        }
    }

    return CharacterData;
}

VOID BiosVideoService(LPWORD Stack)
{
    INT i, CursorHeight;
    BOOLEAN Invisible = FALSE;
    COORD Position;
    CONSOLE_CURSOR_INFO CursorInfo;
    CHAR_INFO Character;
    SMALL_RECT Rect;
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);
    DWORD Ecx = EmulatorGetRegister(EMULATOR_REG_CX);
    DWORD Edx = EmulatorGetRegister(EMULATOR_REG_DX);
    DWORD Ebx = EmulatorGetRegister(EMULATOR_REG_BX);

    switch (HIBYTE(Eax))
    {
        /* Set Video Mode */
        case 0x00:
        {
            BiosSetVideoMode(LOBYTE(Eax));
            break;
        }

        /* Set Text-Mode Cursor Shape */
        case 0x01:
        {
            /* Retrieve and validate the input */
            Invisible = ((HIBYTE(Ecx) >> 5) & 0x03) ? TRUE : FALSE;
            CursorHeight = (HIBYTE(Ecx) & 0x1F) - (LOBYTE(Ecx) & 0x1F);
            if (CursorHeight < 1) CursorHeight = 1;
            if (CursorHeight > 100) CursorHeight = 100;

            /* Update the BDA */
            Bda->CursorStartLine = HIBYTE(Ecx);
            Bda->CursorEndLine = LOBYTE(Ecx) & 0x1F;

            /* Set the cursor */
            CursorInfo.dwSize = (CursorHeight * 100) / CONSOLE_FONT_HEIGHT;
            CursorInfo.bVisible = !Invisible;
            SetConsoleCursorInfo(BiosConsoleOutput, &CursorInfo);

            break;
        }

        /* Set Cursor Position */
        case 0x02:
        {
            /* Make sure the selected video page exists */
            if (HIBYTE(Ebx) >= VideoModes[CurrentVideoMode].Pages) break;

            Bda->CursorPosition[HIBYTE(Ebx)] = LOWORD(Edx);

            /* Check if this is the current video page */
            if (HIBYTE(Ebx) == CurrentVideoPage)
            {
                /* Yes, change the actual cursor */
                Position.X = LOBYTE(Edx);
                Position.Y = HIBYTE(Edx);
                SetConsoleCursorPosition(BiosConsoleOutput, Position);
            }

            break;
        }

        /* Get Cursor Position */
        case 0x03:
        {
            /* Make sure the selected video page exists */
            if (HIBYTE(Ebx) >= VideoModes[CurrentVideoMode].Pages) break;

            /* Return the result */
            EmulatorSetRegister(EMULATOR_REG_AX, 0);
            EmulatorSetRegister(EMULATOR_REG_CX,
                                (Bda->CursorStartLine << 8) | Bda->CursorEndLine);
            EmulatorSetRegister(EMULATOR_REG_DX, Bda->CursorPosition[HIBYTE(Ebx)]);

            break;
        }

        /* Select Active Display Page */
        case 0x05:
        {
            /* Check if the page exists */
            if (LOBYTE(Eax) >= VideoModes[CurrentVideoMode].Pages) break;

            /* Check if this is the same page */
            if (LOBYTE(Eax) == CurrentVideoPage) break;

            /* Change the video page */
            BiosSetVideoPage(LOBYTE(Eax));

            break;
        }

        /* Scroll Up/Down Window */
        // TODO: Implement for different pages
        case 0x06:
        case 0x07:
        {
            BYTE Lines = LOBYTE(Eax);

            Rect.Top = HIBYTE(Ecx);
            Rect.Left = LOBYTE(Ecx);
            Rect.Bottom = HIBYTE(Edx);
            Rect.Right = LOBYTE(Edx);
            Character.Char.UnicodeChar = L' ';
            Character.Attributes = HIBYTE(Ebx);
            Position.X = Rect.Left;

            /* 0 means clear entire window */
            if (Lines == 0) Lines = Rect.Bottom - Rect.Top;

            if (HIBYTE(Eax) == 0x06) Position.Y = Rect.Top - Lines;
            else Position.Y = Rect.Top + Lines;

            ScrollConsoleScreenBuffer(BiosConsoleOutput,
                                      &Rect,
                                      &Rect,
                                      Position,
                                      &Character);

            break;
        }

        /* Read Character And Attribute At Cursor Position */
        case 0x08:
        {
            DWORD Address;
            
            /* Make sure this is text mode */
            if (!VideoModes[CurrentVideoMode].Text) break;

            /* Make sure the selected video page exists */
            if (HIBYTE(Ebx) >= VideoModes[CurrentVideoMode].Pages) break;
            
            /* Find the address */
            Address = BiosGetVideoMemoryStart()
                      + HIBYTE(Ebx) * BiosGetVideoPageSize()
                      + (HIBYTE(Bda->CursorPosition[HIBYTE(Ebx)])
                      * VideoModes[CurrentVideoMode].Height
                      + LOBYTE(Bda->CursorPosition[HIBYTE(Ebx)]))
                      * VideoModes[CurrentVideoMode].Bpp / 8;

            /* Update the video memory at that address */
            BiosUpdateVideoMemory(Address,
                                  Address + VideoModes[CurrentVideoMode].Bpp / 8);

            /* Return the result in AX */
            EmulatorSetRegister(EMULATOR_REG_AX,
                                *((LPWORD)((ULONG_PTR)BaseAddress + Address)));

            break;
        }                EmulatorSetFlag(EMULATOR_FLAG_ZF);


        /* Write Character And Attribute At Cursor Position */
        case 0x09:
        case 0x0A:
        {
            BYTE PixelSize = VideoModes[CurrentVideoMode].Bpp / 8;
            WORD Data = (LOBYTE(Ebx) << 8) | LOBYTE(Eax);
            WORD Repeat = LOWORD(Ecx);
            DWORD Address = BiosGetVideoMemoryStart()
                            + CurrentVideoPage * BiosGetVideoPageSize()
                            + (HIBYTE(Bda->CursorPosition[CurrentVideoPage])
                            * VideoModes[CurrentVideoMode].Height
                            + LOBYTE(Bda->CursorPosition[CurrentVideoPage]))
                            * PixelSize;

            /* Make sure this is text mode */
            if (!VideoModes[CurrentVideoMode].Text) break;

            /* Make sure the selected video page exists */
            if (HIBYTE(Ebx) >= VideoModes[CurrentVideoMode].Pages) break;

            /* Make sure we don't write over the end of video memory */
            Repeat = min(Repeat,
                        (CONSOLE_VIDEO_MEM_END - Address)
                        / PixelSize);

            /* Copy the values to the memory */
            for (i = 0; i < Repeat; i++)
            {
                if (PixelSize == sizeof(BYTE) || HIBYTE(Eax) == 0x0A)
                {
                    /* Just characters, no attributes */
                    *((LPBYTE)((ULONG_PTR)BaseAddress + Address) + i * PixelSize) = LOBYTE(Data);
                }
                else if (PixelSize == sizeof(WORD))
                {
                    /* First byte for characters, second for attributes */
                    *((LPWORD)((ULONG_PTR)BaseAddress + Address) + i) = Data;
                }
            }

            /* Update the range */
            BiosUpdateConsole(Address,
                              Address + Repeat * (VideoModes[CurrentVideoMode].Bpp / 8));

            break;
        }

        /* Teletype Output */
        case 0x0E:
        {
            CHAR Character = LOBYTE(Eax);
            DWORD NumWritten;

            /* Make sure the page exists */
            if (HIBYTE(Ebx) >= VideoModes[CurrentVideoMode].Pages) break;

            /* Set the attribute */
            SetConsoleTextAttribute(BiosConsoleOutput, LOBYTE(Ebx));

            /* Write the character */
            WriteConsoleA(BiosConsoleOutput,
                          &Character,
                          sizeof(CHAR),
                          &NumWritten,
                          NULL);

            break;
        }

        /* Get Current Video Mode */
        case 0x0F:
        {
            EmulatorSetRegister(EMULATOR_REG_AX,
                                MAKEWORD(Bda->VideoMode, Bda->ScreenColumns));
            EmulatorSetRegister(EMULATOR_REG_BX,
                                MAKEWORD(LOBYTE(Ebx), Bda->VideoPage));

            break;
        }

        /* Scroll Window */
        case 0x12:
        {
            Rect.Top = HIBYTE(Ecx);
            Rect.Left = LOBYTE(Ecx);
            Rect.Bottom = HIBYTE(Edx);
            Rect.Right = LOBYTE(Edx);
            Character.Char.UnicodeChar = L' ';
            Character.Attributes = 0x07;
            Position.X = Rect.Left;
            Position.Y = Rect.Top;

            if (LOBYTE(Ebx) == 0) Position.Y -= LOBYTE(Eax);
            else if (LOBYTE(Ebx) == 1) Position.Y += LOBYTE(Eax);
            else if (LOBYTE(Ebx) == 2) Position.X -= LOBYTE(Eax);
            else if (LOBYTE(Ebx) == 3) Position.X += LOBYTE(Eax);

            ScrollConsoleScreenBuffer(BiosConsoleOutput,
                                      &Rect,
                                      &Rect,
                                      Position,
                                      &Character);

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 10h, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosKeyboardService(LPWORD Stack)
{
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);

    switch (HIBYTE(Eax))
    {
        case 0x00:
        {
            /* Read the character (and wait if necessary) */
            EmulatorSetRegister(EMULATOR_REG_AX, BiosGetCharacter());

            break;
        }

        case 0x01:
        {
            WORD Data = BiosPeekCharacter();

            if (Data != 0xFFFF)
            {
                /* There is a character, clear ZF and return it */
                EmulatorSetRegister(EMULATOR_REG_AX, Data);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;
            }
            else
            {
                /* No character, set ZF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_ZF;
            }

            break;
        }
        
        default:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosTimeService(LPWORD Stack)
{
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);
    DWORD Ecx = EmulatorGetRegister(EMULATOR_REG_CX);
    DWORD Edx = EmulatorGetRegister(EMULATOR_REG_DX);

    switch (HIBYTE(Eax))
    {
        case 0x00:
        {
            /* Set AL to 1 if midnight had passed, 0 otherwise */
            Eax &= 0xFFFFFF00;
            if (Bda->MidnightPassed) Eax |= 1;

            /* Return the tick count in CX:DX */
            EmulatorSetRegister(EMULATOR_REG_AX, Eax);
            EmulatorSetRegister(EMULATOR_REG_CX, HIWORD(Bda->TickCounter));
            EmulatorSetRegister(EMULATOR_REG_DX, LOWORD(Bda->TickCounter));

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        case 0x01:
        {
            /* Set the tick count to CX:DX */
            Bda->TickCounter = MAKELONG(LOWORD(Edx), LOWORD(Ecx));

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 1Ah, AH = 0x%02X NOT IMPLEMENTED\n",
                    HIBYTE(Eax));
        }
    }
}

VOID BiosSystemTimerInterrupt(LPWORD Stack)
{
    /* Increase the system tick count */
    Bda->TickCounter++;
}

VOID BiosEquipmentService(LPWORD Stack)
{
    /* Return the equipment list */
    EmulatorSetRegister(EMULATOR_REG_AX, Bda->EquipmentList);
}

VOID BiosHandleIrq(BYTE IrqNumber, LPWORD Stack)
{
    switch (IrqNumber)
    {
        /* PIT IRQ */
        case 0:
        {
            /* Perform the system timer interrupt */
            EmulatorInterrupt(BIOS_SYS_TIMER_INTERRUPT);

            break;
        }

        /* Keyboard IRQ */
        case 1:
        {
            BYTE ScanCode, VirtualKey;
            WORD Character;
            
            /* Check if there is a scancode available */
            if (!(KeyboardReadStatus() & 1)) break;

            /* Get the scan code and virtual key code */
            ScanCode = KeyboardReadData();
            VirtualKey = MapVirtualKey(ScanCode & 0x7F, MAPVK_VSC_TO_VK);

            /* Check if this is a key press or release */
            if (!(ScanCode & (1 << 7)))
            {
                /* Key press */
                if (VirtualKey == VK_NUMLOCK
                    || VirtualKey == VK_CAPITAL
                    || VirtualKey == VK_SCROLL)
                {
                    /* For toggle keys, toggle the lowest bit in the keyboard map */
                    BiosKeyboardMap[VirtualKey] ^= ~(1 << 0);
                }

                /* Set the highest bit */
                BiosKeyboardMap[VirtualKey] |= (1 << 7);

                /* Find out which character this is */
                if (ToAscii(VirtualKey, ScanCode, BiosKeyboardMap, &Character, 0) > 0)
                {
                    /* Push it onto the BIOS keyboard queue */
                    BiosKbdBufferPush((ScanCode << 8) | (Character & 0xFF));
                }
            }
            else
            {
                /* Key release, unset the highest bit */
                BiosKeyboardMap[VirtualKey] &= ~(1 << 7);
            }

            break;
        }
    }

    /* Send End-of-Interrupt to the PIC */
    if (IrqNumber > 8) PicWriteCommand(PIC_SLAVE_CMD, PIC_OCW2_EOI);
    PicWriteCommand(PIC_MASTER_CMD, PIC_OCW2_EOI);
}

/* EOF */
