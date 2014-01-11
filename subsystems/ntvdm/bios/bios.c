/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "bios.h"

#include "io.h"
#include "hardware/pic.h"
#include "hardware/ps2.h"
#include "hardware/timer.h"

#include "int32.h"
#include "registers.h"

/* PRIVATE VARIABLES **********************************************************/

PBIOS_DATA_AREA Bda;
static BYTE BiosKeyboardMap[256];

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN BiosKbdBufferPush(WORD Data)
{
    /* Get the location of the element after the tail */
    WORD NextElement = Bda->KeybdBufferTail + sizeof(WORD);

    /* Wrap it around if it's at or beyond the end */
    if (NextElement >= Bda->KeybdBufferEnd) NextElement = Bda->KeybdBufferStart;

    /* If it's full, fail */
    if (NextElement == Bda->KeybdBufferHead) return FALSE;

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

static VOID WINAPI BiosEquipmentService(LPWORD Stack)
{
    /* Return the equipment list */
    setAX(Bda->EquipmentList);
}

static VOID WINAPI BiosGetMemorySize(LPWORD Stack)
{
    /* Return the conventional memory size in kB, typically 640 kB */
    setAX(Bda->MemorySize);
}

static VOID WINAPI BiosMiscService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Copy Extended Memory */
        case 0x87:
        {
            DWORD Count = (DWORD)getCX() * 2;
            PFAST486_GDT_ENTRY Gdt = (PFAST486_GDT_ENTRY)SEG_OFF_TO_PTR(getES(), getSI());
            DWORD SourceBase = Gdt[2].Base + (Gdt[2].BaseMid << 16) + (Gdt[2].BaseHigh << 24);
            DWORD SourceLimit = Gdt[2].Limit + (Gdt[2].LimitHigh << 16);
            DWORD DestBase = Gdt[3].Base + (Gdt[3].BaseMid << 16) + (Gdt[3].BaseHigh << 24);
            DWORD DestLimit = Gdt[3].Limit + (Gdt[3].LimitHigh << 16);

            /* Check for flags */
            if (Gdt[2].Granularity) SourceLimit = (SourceLimit << 12) | 0xFFF;
            if (Gdt[3].Granularity) DestLimit = (DestLimit << 12) | 0xFFF;

            if ((Count > SourceLimit) || (Count > DestLimit))
            {
                setAX(0x80);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                break;
            }

            /* Copy */
            RtlMoveMemory((PVOID)((ULONG_PTR)BaseAddress + DestBase),
                          (PVOID)((ULONG_PTR)BaseAddress + SourceBase),
                          Count);

            setAX(ERROR_SUCCESS);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Get Extended Memory Size */
        case 0x88:
        {
            /* Return the number of KB of RAM after 1 MB */
            setAX((MAX_ADDRESS - 0x100000) / 1024);

            /* Clear CF */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 15h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosKeyboardService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Wait for keystroke and read */
        case 0x00:
        /* Wait for extended keystroke and read */
        case 0x10:  // FIXME: Temporarily do the same as INT 16h, 00h
        {
            /* Read the character (and wait if necessary) */
            setAX(BiosGetCharacter());
            break;
        }

        /* Get keystroke status */
        case 0x01:
        /* Get extended keystroke status */
        case 0x11:  // FIXME: Temporarily do the same as INT 16h, 01h
        {
            WORD Data = BiosPeekCharacter();

            if (Data != 0xFFFF)
            {
                /* There is a character, clear ZF and return it */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;
                setAX(Data);
            }
            else
            {
                /* No character, set ZF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_ZF;
            }

            break;
        }

        /* Get shift status */
        case 0x02:
        {
            /* Return the lower byte of the keyboard shift status word */
            setAL(LOBYTE(Bda->KeybdShiftFlags));
            break;
        }

        /* Reserved */
        case 0x04:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x04 is RESERVED\n");
            break;
        }

        /* Push keystroke */
        case 0x05:
        {
            /* Return 0 if success, 1 if failure */
            setAL(BiosKbdBufferPush(getCX()) == FALSE);
            break;
        }

        /* Get extended shift status */
        case 0x12:
        {
            /*
             * Be careful! The returned word is similar to Bda->KeybdShiftFlags
             * but the high byte is organized differently:
             * the bytes 2 and 3 of the high byte are not the same...
             */
            WORD KeybdShiftFlags = (Bda->KeybdShiftFlags & 0xF3FF);

            /* Return the extended keyboard shift status word */
            setAX(KeybdShiftFlags);
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosTimeService(LPWORD Stack)
{
    switch (getAH())
    {
        case 0x00:
        {
            /* Set AL to 1 if midnight had passed, 0 otherwise */
            setAL(Bda->MidnightPassed ? 0x01 : 0x00);

            /* Return the tick count in CX:DX */
            setCX(HIWORD(Bda->TickCounter));
            setDX(LOWORD(Bda->TickCounter));

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        case 0x01:
        {
            /* Set the tick count to CX:DX */
            Bda->TickCounter = MAKELONG(getDX(), getCX());

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 1Ah, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosSystemTimerInterrupt(LPWORD Stack)
{
    /* Increase the system tick count */
    Bda->TickCounter++;
}

/* PUBLIC FUNCTIONS ***********************************************************/

WORD BiosPeekCharacter(VOID)
{
    WORD CharacterData = 0;

    /* Get the key from the queue, but don't remove it */
    if (BiosKbdBufferTop(&CharacterData)) return CharacterData;
    else return 0xFFFF;
}

WORD BiosGetCharacter(VOID)
{
    WORD CharacterData = 0;

    /* Check if there is a key available */
    if (BiosKbdBufferTop(&CharacterData))
    {
        /* A key was available, remove it from the queue */
        BiosKbdBufferPop();
    }
    else
    {
        /* No key available. Set the handler CF to repeat the BOP */
        setCF(1);
        // CharacterData = 0xFFFF;
    }

    return CharacterData;
}

BOOLEAN BiosInitialize(HANDLE ConsoleInput, HANDLE ConsoleOutput)
{
    /* Initialize the BDA */
    Bda = (PBIOS_DATA_AREA)SEG_OFF_TO_PTR(BDA_SEGMENT, 0);
    Bda->EquipmentList = BIOS_EQUIPMENT_LIST;
    /*
     * Conventional memory size is 640 kB,
     * see: http://webpages.charter.net/danrollins/techhelp/0184.HTM
     * and see Ralf Brown: http://www.ctyme.com/intr/rb-0598.htm
     * for more information.
     */
    Bda->MemorySize = 0x0280;
    Bda->KeybdBufferStart = FIELD_OFFSET(BIOS_DATA_AREA, KeybdBuffer);
    Bda->KeybdBufferEnd = Bda->KeybdBufferStart + BIOS_KBD_BUFFER_SIZE * sizeof(WORD);
    Bda->KeybdBufferHead = Bda->KeybdBufferTail = 0;

    /* Initialize the 32-bit Interrupt system */
    InitializeInt32(BIOS_SEGMENT);

    /* Register the BIOS 32-bit Interrupts */
    RegisterInt32(BIOS_EQUIPMENT_INTERRUPT, BiosEquipmentService    );
    RegisterInt32(BIOS_MEMORY_SIZE        , BiosGetMemorySize       );
    RegisterInt32(BIOS_MISC_INTERRUPT     , BiosMiscService         );
    RegisterInt32(BIOS_KBD_INTERRUPT      , BiosKeyboardService     );
    RegisterInt32(BIOS_TIME_INTERRUPT     , BiosTimeService         );
    RegisterInt32(BIOS_SYS_TIMER_INTERRUPT, BiosSystemTimerInterrupt);

    /* Some interrupts are in fact addresses to tables */
    ((PDWORD)BaseAddress)[0x1D] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x1E] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x1F] = (DWORD)NULL;

    ((PDWORD)BaseAddress)[0x41] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x43] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x44] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x46] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x48] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x49] = (DWORD)NULL;

    /* Initialize the Video BIOS */
    if (!VidBiosInitialize(ConsoleOutput)) return FALSE;

    /* Set the console input mode */
    SetConsoleMode(ConsoleInput, ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);

    /* Initialize PS2 */
    PS2Initialize(ConsoleInput);

    /*
     * The POST (Power On-Self Test)
     */

    /* Initialize the PIC */
    IOWriteB(PIC_MASTER_CMD, PIC_ICW1 | PIC_ICW1_ICW4);
    IOWriteB(PIC_SLAVE_CMD , PIC_ICW1 | PIC_ICW1_ICW4);

    /* Set the interrupt offsets */
    IOWriteB(PIC_MASTER_DATA, BIOS_PIC_MASTER_INT);
    IOWriteB(PIC_SLAVE_DATA , BIOS_PIC_SLAVE_INT);

    /* Tell the master PIC there is a slave at IRQ 2 */
    IOWriteB(PIC_MASTER_DATA, 1 << 2);
    IOWriteB(PIC_SLAVE_DATA , 2);

    /* Make sure the PIC is in 8086 mode */
    IOWriteB(PIC_MASTER_DATA, PIC_ICW4_8086);
    IOWriteB(PIC_SLAVE_DATA , PIC_ICW4_8086);

    /* Clear the masks for both PICs */
    IOWriteB(PIC_MASTER_DATA, 0x00);
    IOWriteB(PIC_SLAVE_DATA , 0x00);

    PitWriteCommand(0x34);
    PitWriteData(0, 0x00);
    PitWriteData(0, 0x00);

    return TRUE;
}

VOID BiosCleanup(VOID)
{
    PS2Cleanup();
    VidBiosCleanup();
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

            /* Get the scan code and virtual key code */
            ScanCode = IOReadB(PS2_DATA_PORT);
            VirtualKey = MapVirtualKey(ScanCode & 0x7F, MAPVK_VSC_TO_VK);

            /* Check if this is a key press or release */
            if (!(ScanCode & (1 << 7)))
            {
                /* Key press */
                if (VirtualKey == VK_NUMLOCK ||
                    VirtualKey == VK_CAPITAL ||
                    VirtualKey == VK_SCROLL  ||
                    VirtualKey == VK_INSERT)
                {
                    /* For toggle keys, toggle the lowest bit in the keyboard map */
                    BiosKeyboardMap[VirtualKey] ^= ~(1 << 0);
                }

                /* Set the highest bit */
                BiosKeyboardMap[VirtualKey] |= (1 << 7);

                /* Find out which character this is */
                Character = 0;
                if (ToAscii(VirtualKey, ScanCode, BiosKeyboardMap, &Character, 0) == 0)
                {
                    /* Not ASCII */
                    Character = 0;
                }

                /* Push it onto the BIOS keyboard queue */
                BiosKbdBufferPush(MAKEWORD(Character, ScanCode));
            }
            else
            {
                /* Key release, unset the highest bit */
                BiosKeyboardMap[VirtualKey] &= ~(1 << 7);
            }

            /* Clear the keyboard flags */
            Bda->KeybdShiftFlags = 0;

            /* Set the appropriate flags based on the state */
            if (BiosKeyboardMap[VK_RSHIFT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RSHIFT;
            if (BiosKeyboardMap[VK_LSHIFT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LSHIFT;
            if (BiosKeyboardMap[VK_CONTROL]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CTRL;
            if (BiosKeyboardMap[VK_MENU]     & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_ALT;
            if (BiosKeyboardMap[VK_SCROLL]   & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL_ON;
            if (BiosKeyboardMap[VK_NUMLOCK]  & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK_ON;
            if (BiosKeyboardMap[VK_CAPITAL]  & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK_ON;
            if (BiosKeyboardMap[VK_INSERT]   & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT_ON;
            if (BiosKeyboardMap[VK_RMENU]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RALT;
            if (BiosKeyboardMap[VK_LMENU]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LALT;
            if (BiosKeyboardMap[VK_SNAPSHOT] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SYSRQ;
            if (BiosKeyboardMap[VK_PAUSE]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_PAUSE;
            if (BiosKeyboardMap[VK_SCROLL]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL;
            if (BiosKeyboardMap[VK_NUMLOCK]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK;
            if (BiosKeyboardMap[VK_CAPITAL]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK;
            if (BiosKeyboardMap[VK_INSERT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT;

            break;
        }
    }

    /* Send End-of-Interrupt to the PIC */
    if (IrqNumber >= 8) IOWriteB(PIC_SLAVE_CMD, PIC_OCW2_EOI);
    IOWriteB(PIC_MASTER_CMD, PIC_OCW2_EOI);
}

/* EOF */
