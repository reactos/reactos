/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "bios.h"
#include "emulator.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"

/* PRIVATE VARIABLES **********************************************************/

static BYTE CursorRow, CursorCol;
static WORD ConsoleWidth, ConsoleHeight;
static BYTE BiosKeyboardMap[256];
static WORD BiosKbdBuffer[BIOS_KBD_BUFFER_SIZE];
static UINT BiosKbdBufferStart = 0, BiosKbdBufferEnd = 0;
static BOOLEAN BiosKbdBufferEmpty = TRUE;

/* PRIVATE FUNCTIONS **********************************************************/

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

static BOOLEAN BiosKbdBufferPush(WORD Data)
{
    /* If it's full, fail */
    if (!BiosKbdBufferEmpty && (BiosKbdBufferStart == BiosKbdBufferEnd))
    {
        return FALSE;
    }

    /* Otherwise, add the value to the queue */
    BiosKbdBuffer[BiosKbdBufferEnd] = Data;
    BiosKbdBufferEnd++;
    BiosKbdBufferEnd %= BIOS_KBD_BUFFER_SIZE;
    BiosKbdBufferEmpty = FALSE;

    /* Return success */
    return TRUE;
}

static BOOLEAN BiosKbdBufferTop(LPWORD Data)
{
    /* If it's empty, fail */
    if (BiosKbdBufferEmpty) return FALSE;

    /* Otherwise, get the value and return success */
    *Data = BiosKbdBuffer[BiosKbdBufferStart];
    return TRUE;
}

static BOOLEAN BiosKbdBufferPop()
{
    /* If it's empty, fail */
    if (BiosKbdBufferEmpty) return FALSE;

    /* Otherwise, remove the value and return success */
    BiosKbdBufferStart++;
    BiosKbdBufferStart %= BIOS_KBD_BUFFER_SIZE;
    if (BiosKbdBufferStart == BiosKbdBufferEnd) BiosKbdBufferEmpty = TRUE;

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN BiosInitialize()
{
    INT i;
    WORD Offset = 0;
    HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
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

    /* Set the console input mode */
    SetConsoleMode(ConsoleInput, ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);

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

WORD BiosPeekCharacter()
{
    WORD CharacterData;
    
    /* Check if there is a key available */
    if (BiosKbdBufferEmpty) return 0xFFFF;

    /* Get the key from the queue, but don't remove it */
    BiosKbdBufferTop(&CharacterData);

    return CharacterData;
}

WORD BiosGetCharacter()
{
    WORD CharacterData;
    HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD InputRecord;
    DWORD Count;

    /* Check if there is a key available */
    if (!BiosKbdBufferEmpty)
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
            WaitForSingleObject(ConsoleInput, INFINITE);
    
            /* Read the event, and make sure it's a keypress */
            if (!ReadConsoleInput(ConsoleInput, &InputRecord, 1, &Count)) continue;
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

VOID BiosKeyboardService()
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
                EmulatorClearFlag(EMULATOR_FLAG_ZF);
            }
            else
            {
                /* No character, set ZF */
                EmulatorSetFlag(EMULATOR_FLAG_ZF);
            }

            break;
        }
    }
}

VOID BiosHandleIrq(BYTE IrqNumber)
{
    switch (IrqNumber)
    {
        /* PIT IRQ */
        case 0:
        {
            /* Perform the system timer interrupt */
            EmulatorInterrupt(0x1C);

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
            VirtualKey = MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK);

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
                ToAscii(ScanCode, VirtualKey, BiosKeyboardMap, &Character, 0);

                /* Push it onto the BIOS keyboard queue */
                BiosKbdBufferPush((ScanCode << 8) | (Character & 0xFF));
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
