/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "ntvdm.h"

BYTE CursorRow, CursorCol;
WORD ConsoleWidth, ConsoleHeight;

BOOLEAN BiosInitialize()
{
    INT i;
    WORD Offset = 0;
    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    LPWORD IntVecTable = (LPWORD)((ULONG_PTR)BaseAddress);
    LPBYTE BiosCode = (LPBYTE)((ULONG_PTR)BaseAddress + TO_LINEAR(BIOS_SEGMENT, 0));

    /* Generate ISR stubs and fill the IVT */
    for (i = 0; i < 256; i++)
    {
        IntVecTable[i * 2] = Offset;
        IntVecTable[i * 2 + 1] = BIOS_SEGMENT;

        if (i != SPECIAL_INT_NUM)
        {
            BiosCode[Offset++] = 0xFA; // cli

            BiosCode[Offset++] = 0x6A; // push i
            BiosCode[Offset++] = (BYTE)i;

            BiosCode[Offset++] = 0xCD; // int SPECIAL_INT_NUM
            BiosCode[Offset++] = SPECIAL_INT_NUM;

            BiosCode[Offset++] = 0x83; // add sp, 2
            BiosCode[Offset++] = 0xC4;
            BiosCode[Offset++] = 0x02;
        }

        BiosCode[Offset++] = 0xCF; // iret
    }

    /* Get the console buffer info */
    if (!GetConsoleScreenBufferInfo(ConsoleOutput, &ConsoleInfo))
    {
        return FALSE;
    }

    /* Set the initial cursor position and console size */
    CursorCol = ConsoleInfo.dwCursorPosition.X;
    CursorRow = ConsoleInfo.dwCursorPosition.Y;
    ConsoleWidth = ConsoleInfo.dwSize.X;
    ConsoleHeight = ConsoleInfo.dwSize.Y;
    
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

static COORD BiosVideoAddressToCoord(ULONG Address)
{
    COORD Result = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!GetConsoleScreenBufferInfo(ConsoleOutput, &ConsoleInfo))
    {
        ASSERT(FALSE);
        return Result;
    }

    Result.X = ((Address - CONSOLE_VIDEO_MEM_START) >> 1) % ConsoleInfo.dwSize.X;
    Result.Y = ((Address - CONSOLE_VIDEO_MEM_START) >> 1) / ConsoleInfo.dwSize.X;

    return Result;
}

VOID BiosUpdateConsole(ULONG StartAddress, ULONG EndAddress)
{
    ULONG i;
    COORD Coordinates;
    DWORD CharsWritten;
    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Loop through all the addresses */
    for (i = StartAddress; i < EndAddress; i++)
    {
        /* Get the coordinates */
        Coordinates = BiosVideoAddressToCoord(i);

        /* Check if this is a character byte or an attribute byte */
        if ((i - CONSOLE_VIDEO_MEM_START) % 2 == 0)
        {
            /* This is a regular character */
            FillConsoleOutputCharacterA(ConsoleOutput,
                                        *(PCHAR)((ULONG_PTR)BaseAddress + i),
                                        sizeof(CHAR),
                                        Coordinates,
                                        &CharsWritten);
        }
        else
        {
            /*  This is an attribute */
            FillConsoleOutputAttribute(ConsoleOutput,
                                       *(PCHAR)((ULONG_PTR)BaseAddress + i),
                                       sizeof(CHAR),
                                       Coordinates,
                                       &CharsWritten);
        }
    }
}

VOID BiosUpdateVideoMemory(ULONG StartAddress, ULONG EndAddress)
{
    ULONG i;
    COORD Coordinates;
    WORD Attribute;
    DWORD CharsWritten;
    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Loop through all the addresses */
    for (i = StartAddress; i < EndAddress; i++)
    {
        /* Get the coordinates */
        Coordinates = BiosVideoAddressToCoord(i);

        /* Check if this is a character byte or an attribute byte */
        if ((i - CONSOLE_VIDEO_MEM_START) % 2 == 0)
        {
            /* This is a regular character */
            ReadConsoleOutputCharacterA(ConsoleOutput,
                                        (LPSTR)((ULONG_PTR)BaseAddress + i),
                                        sizeof(CHAR),
                                        Coordinates,
                                        &CharsWritten);
        }
        else
        {
            /*  This is an attribute */
            ReadConsoleOutputAttribute(ConsoleOutput,
                                       &Attribute,
                                       sizeof(CHAR),
                                       Coordinates,
                                       &CharsWritten);

            *(PCHAR)((ULONG_PTR)BaseAddress + i) = LOBYTE(Attribute);
        }
    }
}

VOID BiosVideoService()
{
    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    INT CursorHeight;
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
        /* Set Text-Mode Cursor Shape */
        case 0x01:
        {
            /* Retrieve and validate the input */
            Invisible = ((HIBYTE(Ecx) >> 5) & 0x03) ? TRUE : FALSE;
            CursorHeight = (HIBYTE(Ecx) & 0x1F) - (LOBYTE(Ecx) & 0x1F);
            if (CursorHeight < 1) CursorHeight = 1;
            if (CursorHeight > 100) CursorHeight = 100;

            /* Set the cursor */
            CursorInfo.dwSize = (CursorHeight * 100) / CONSOLE_FONT_HEIGHT;
            CursorInfo.bVisible = !Invisible;
            SetConsoleCursorInfo(ConsoleOutput, &CursorInfo);

            break;
        }

        /* Set Cursor Position */
        case 0x02:
        {
            Position.X = LOBYTE(Edx);
            Position.Y = HIBYTE(Edx);

            SetConsoleCursorPosition(ConsoleOutput, Position);
            break;
        }

        /* Scroll Up/Down Window */
        case 0x06:
        case 0x07:
        {
            Rect.Top = HIBYTE(Ecx);
            Rect.Left = LOBYTE(Ecx);
            Rect.Bottom = HIBYTE(Edx);
            Rect.Right = LOBYTE(Edx);
            Character.Char.UnicodeChar = L' ';
            Character.Attributes = HIBYTE(Ebx);
            Position.X = Rect.Left;
            if (HIBYTE(Eax) == 0x06) Position.Y = Rect.Top - LOBYTE(Eax);
            else Position.Y = Rect.Top + LOBYTE(Eax);
            
            ScrollConsoleScreenBuffer(ConsoleOutput,
                                      &Rect,
                                      &Rect,
                                      Position,
                                      &Character);
            break;
        }

        /* Read Character And Attribute At Cursor Position */
        case 0x08:
        {
            break;
        }

        /* Write Character And Attribute At Cursor Position */
        case 0x09:
        {
            break;
        }

        /* Write Character Only At Cursor Position */
        case 0x0A:
        {
            break;
        }
    }
}

VOID BiosHandleIrq(BYTE IrqNumber)
{
    PicWriteCommand(PIC_MASTER_CMD, PIC_OCW2_EOI);
}

/* EOF */
