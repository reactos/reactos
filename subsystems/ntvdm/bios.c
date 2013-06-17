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

    return TRUE;
}

static COORD BiosVideoAddressToCoord(ULONG Address)
{
    COORD Result = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!GetConsoleScreenBufferInfo(ConsoleOutput, &ConsoleInfo))
    {
        assert(0);
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
    COORD CursorPosition;
    CONSOLE_CURSOR_INFO CursorInfo;
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);
    DWORD Ecx = EmulatorGetRegister(EMULATOR_REG_CX);
    DWORD Edx = EmulatorGetRegister(EMULATOR_REG_DX);

    switch (LOBYTE(Eax))
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
            CursorPosition.X = LOBYTE(Edx);
            CursorPosition.Y = HIBYTE(Edx);

            SetConsoleCursorPosition(ConsoleOutput, CursorPosition);
            break;
        }

        /* Scroll Up Window */
        case 0x06:
        {
            break;
        }

        /* Scroll Down Window */
        case 0x07:
        {
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

/* EOF */
