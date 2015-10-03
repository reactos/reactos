/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/bios32/kbdbios32.c
 * PURPOSE:         VDM 32-bit PS/2 Keyboard BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE VARIABLES **********************************************************/

static BYTE BiosKeyboardMap[256];

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN BiosKbdBufferPush(WORD Data)
{
    /* Get the location of the element after the tail */
    WORD NextElement = Bda->KeybdBufferTail + sizeof(WORD);

    /* Wrap it around if it's at or beyond the end */
    if (NextElement >= Bda->KeybdBufferEnd) NextElement = Bda->KeybdBufferStart;

    /* If it's full, fail */
    if (NextElement == Bda->KeybdBufferHead)
    {
        DPRINT1("BIOS keyboard buffer full.\n");
        return FALSE;
    }

    /* Put the value in the queue */
    *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferTail)) = Data;
    Bda->KeybdBufferTail = NextElement;

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

/* static */
VOID WINAPI BiosKeyboardService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Wait for keystroke and read */
        case 0x00:
        /* Wait for extended keystroke and read */
        case 0x10:
        {
            WORD Character;

            /* Read the character (and wait if necessary) */
            if (!BiosKbdBufferTop(&Character))
            {
                /* No key available. Set the handler CF to repeat the BOP */
                setCF(1);
                break;
            }

            if (getAH() == 0x00 && LOBYTE(Character) == 0xE0)
            {
                /* Clear the extended code */
                Character &= 0xFF00;
            }

            BiosKbdBufferPop();
            setAX(Character);
            setCF(0);

            break;
        }

        /* Get keystroke status */
        case 0x01:
        /* Get extended keystroke status */
        case 0x11:
        {
            WORD Character;

            if (BiosKbdBufferTop(&Character))
            {
                /* There is a character, clear ZF and return it */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;

                if (getAH() == 0x01 && LOBYTE(Character) == 0xE0)
                {
                    /* Clear the extended code */
                    Character &= 0xFF00;
                }

                setAX(Character);
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
             * Be careful! The returned word is similar to 'Bda->KeybdShiftFlags'
             * but the high byte is organized differently:
             * the bits 2 and 3 of the high byte are not the same:
             * instead they correspond to the right CTRL and ALT keys as specified
             * in bits 2 and 3 of LOBYTE(Bda->KeybdStatusFlags).
             */
            // Bda->KeybdShiftFlags & 0xF3FF;
            WORD KeybdShiftFlags = MAKEWORD(LOBYTE(Bda->KeybdShiftFlags),
                                               (HIBYTE(Bda->KeybdShiftFlags ) & 0xF3) |
                                               (LOBYTE(Bda->KeybdStatusFlags) & 0x0C));

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

// Keyboard IRQ 1
/* static */
VOID WINAPI BiosKeyboardIrq(LPWORD Stack)
{
    static BOOLEAN Extended = FALSE;
    BOOLEAN SkipScanCode;
    BYTE ScanCode, VirtualKey;
    WORD Character;

    /*
     * Get the scan code from the PS/2 port, then call the
     * INT 15h, AH=4Fh Keyboard Intercept function to try to
     * translate the scan code. CF must be set before the call.
     * In return, if CF is set we continue processing the scan code
     * stored in AL, and if not, we skip it.
     */
    WORD AX = getAX();
    WORD CX = getCX();
    WORD DX = getDX();
    WORD BX = getBX();
    WORD BP = getBP();
    WORD SI = getSI();
    WORD DI = getDI();
    WORD DS = getDS();
    WORD ES = getES();

    setCF(1);
    setAL(IOReadB(PS2_DATA_PORT));
    setAH(0x4F);
    Int32Call(&BiosContext, BIOS_MISC_INTERRUPT);

    /* Retrieve the modified scan code in AL */
    SkipScanCode = (getCF() == 0);
    ScanCode = getAL();

    setAX(AX);
    setCX(CX);
    setDX(DX);
    setBX(BX);
    setBP(BP);
    setSI(SI);
    setDI(DI);
    setDS(DS);
    setES(ES);
    setCF(0);

    if (ScanCode == 0xE0)
    {
        Extended = TRUE;
        Bda->KeybdStatusFlags |= 0x02;
        goto Quit;
    }

    // FIXME: For diagnostic purposes. We should decide what to do then!!
    if (ScanCode == 0xE1)
        DPRINT1("BiosKeyboardIrq, ScanCode == 0xE1\n");

    /* Check whether CF is clear. If so, skip the scan code. */
    if (SkipScanCode) goto Quit;

    /* Get the corresponding virtual key code */
    VirtualKey = MapVirtualKey(ScanCode & 0x7F, MAPVK_VSC_TO_VK);

    /* Check if this is a key press or release */
    if (!(ScanCode & (1 << 7)))
    {
        /* Key press, set the highest bit */
        BiosKeyboardMap[VirtualKey] |= (1 << 7);

        switch (VirtualKey)
        {
            case VK_NUMLOCK:
            case VK_CAPITAL:
            case VK_SCROLL:
            case VK_INSERT:
            {
                /* For toggle keys, toggle the lowest bit in the keyboard map */
                BiosKeyboardMap[VirtualKey] ^= ~(1 << 0);
                break;
            }

            case VK_SHIFT:
            case VK_LSHIFT:
            case VK_RSHIFT:
            case VK_CONTROL:
            case VK_RCONTROL:
            case VK_LCONTROL:
            case VK_MENU:
            case VK_LMENU:
            case VK_RMENU:
            {
                /* Modifier keys don't go in the buffer */
                break;
            }

            default:
            {
                Character = Extended ? 0xE0 : 0x00;

                /* If this is not an extended scancode, and ALT isn't held down, find out which character this is */
                if (!Extended && !(Bda->KeybdShiftFlags & (BDA_KBDFLAG_ALT | BDA_KBDFLAG_LALT | BDA_KBDFLAG_RALT)))
                {
                    if (ToAscii(VirtualKey, ScanCode, BiosKeyboardMap, &Character, 0) == 0)
                    {
                        /* Not ASCII */
                        Character = 0;
                    }
                }

                /* Push it onto the BIOS keyboard queue */
                BiosKbdBufferPush(MAKEWORD(Character, ScanCode));
            }
        }
    }
    else
    {
        /* Key release, unset the highest bit */
        BiosKeyboardMap[VirtualKey] &= ~(1 << 7);
    }

    /* Clear the keyboard flags */
    Bda->KeybdShiftFlags = 0;
    // Release right CTRL and ALT keys
    Bda->KeybdStatusFlags &= ~(BDA_KBDFLAG_RCTRL | BDA_KBDFLAG_RALT);

    /* Set the appropriate flags based on the state */
    // SHIFT
    if (BiosKeyboardMap[VK_RSHIFT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RSHIFT;
    if (BiosKeyboardMap[VK_LSHIFT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LSHIFT;
    if (BiosKeyboardMap[VK_SHIFT]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LSHIFT;
    // CTRL
    if (BiosKeyboardMap[VK_RCONTROL] & (1 << 7)) Bda->KeybdStatusFlags |= BDA_KBDFLAG_RCTRL;
    if (BiosKeyboardMap[VK_LCONTROL] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LCTRL;
    if (BiosKeyboardMap[VK_CONTROL]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CTRL;
    // ALT
    if (BiosKeyboardMap[VK_RMENU]    & (1 << 7)) Bda->KeybdStatusFlags |= BDA_KBDFLAG_RALT;
    if (BiosKeyboardMap[VK_LMENU]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LALT;
    if (BiosKeyboardMap[VK_MENU]     & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_ALT;
    // Others
    if (BiosKeyboardMap[VK_SCROLL]   & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL_ON;
    if (BiosKeyboardMap[VK_NUMLOCK]  & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK_ON;
    if (BiosKeyboardMap[VK_CAPITAL]  & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK_ON;
    if (BiosKeyboardMap[VK_INSERT]   & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT_ON;
    if (BiosKeyboardMap[VK_SNAPSHOT] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SYSRQ;
    if (BiosKeyboardMap[VK_PAUSE]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_PAUSE;
    if (BiosKeyboardMap[VK_SCROLL]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL;
    if (BiosKeyboardMap[VK_NUMLOCK]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK;
    if (BiosKeyboardMap[VK_CAPITAL]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK;
    if (BiosKeyboardMap[VK_INSERT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT;

    /* Clear the extended key flag */
    Extended = FALSE;
    Bda->KeybdStatusFlags &= ~0x02; // Remove the 0xE0 code flag
    // Bda->KeybdStatusFlags &= ~0x01; // Remove the 0xE1 code flag

    DPRINT("BiosKeyboardIrq - Character = 0x%X, ScanCode = 0x%X, KeybdShiftFlags = 0x%X\n",
           Character, ScanCode, Bda->KeybdShiftFlags);

Quit:
    PicIRQComplete(LOBYTE(Stack[STACK_INT_NUM]));
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID KbdBios32Post(VOID)
{
    /* Initialize the BDA */
    Bda->KeybdBufferStart = FIELD_OFFSET(BIOS_DATA_AREA, KeybdBuffer);
    Bda->KeybdBufferEnd   = Bda->KeybdBufferStart + BIOS_KBD_BUFFER_SIZE * sizeof(WORD);
    Bda->KeybdBufferHead  = Bda->KeybdBufferTail = Bda->KeybdBufferStart;

    // FIXME: Fill the keyboard buffer with invalid values for diagnostic purposes...
    RtlFillMemory(((LPVOID)((ULONG_PTR)Bda + Bda->KeybdBufferStart)),
                  BIOS_KBD_BUFFER_SIZE * sizeof(WORD), 'A');

    Bda->KeybdShiftFlags  = 0;
    Bda->KeybdStatusFlags = (1 << 4); // 101/102 enhanced keyboard installed
    Bda->KeybdLedFlags    = 0;

    /*
     * Register the BIOS 32-bit Interrupts:
     * - Software vector handler
     * - HW vector interrupt
     */
    RegisterBiosInt32(BIOS_KBD_INTERRUPT, BiosKeyboardService);
    EnableHwIRQ(1, BiosKeyboardIrq);
}

/* EOF */
