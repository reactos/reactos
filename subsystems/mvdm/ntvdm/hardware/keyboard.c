/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/keyboard.c
 * PURPOSE:         Keyboard emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "keyboard.h"
#include "ps2.h"

/* PRIVATE VARIABLES **********************************************************/

static BOOLEAN KeyboardReporting = FALSE;
static BYTE KeyboardId = 0; // We only support basic old-type keyboard
static BYTE KbdDataByteWait = 0;

static BYTE KbdPS2Port = 0;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI KeyboardCommand(LPVOID Param, BYTE Command)
{
    /* Check if we were waiting for a data byte */
    if (KbdDataByteWait)
    {
        PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);

        switch (KbdDataByteWait)
        {
            /* Set/Reset Mode Indicators */
            case 0xED:
            {
                // Ignore setting the keyboard LEDs
                break;
            }

            /* PS/2 Select/Read Alternate Scan Code Sets */
            case 0xF0:
            /* Set Typematic Rate/Delay */
            case 0xF3:
            {
                // FIXME: UNIMPLEMENTED; just return ACKnowledge.
                // This unblocks some programs that want to initialize
                // the keyboard by sending keyboard commands and then
                // performing polling on the port until "valid" data
                // comes out.
                DPRINT1("KeyboardCommand(0x%02X) NOT IMPLEMENTED\n", KbdDataByteWait);
                break;
            }

            default:
            {
                /* Shouldn't happen */
                ASSERT(FALSE);
            }
        }

        KbdDataByteWait = 0;
        return;
    }

    switch (Command)
    {
        /* Set/Reset Mode Indicators */
        case 0xED:
        /* PS/2 Select/Read Alternate Scan Code Sets */
        case 0xF0:
        /* Set Typematic Rate/Delay */
        case 0xF3:
        {
            KbdDataByteWait = Command;
            PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);
            break;
        }

        /* Echo test command */
        case 0xEE:
        {
            PS2QueuePush(KbdPS2Port, 0xEE);
            break;
        }

        /* Get Keyboard ID */
        case 0xF2:
        {
            PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);
            PS2QueuePush(KbdPS2Port, KeyboardId);
            break;
        }

        /* Enable Reporting */
        case 0xF4:
        {
            KeyboardReporting = TRUE;
            PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);
            break;
        }

        /* Disable Reporting */
        case 0xF5:
        {
            KeyboardReporting = FALSE;
            PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);
            break;
        }

        /* Set Defaults */
        case 0xF6:
        {
            // So far, nothing to reset
            PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);
            break;
        }

        /* PS/2 Typematic & Make/Break key modes */
        case 0xF7: case 0xF8: case 0xF9:
        case 0xFA: case 0xFB: case 0xFC: case 0xFD:
        {
            /*
             * Unsupported on PC-AT, they are just ignored
             * and acknowledged as discussed in:
             * https://stanislavs.org/helppc/keyboard_commands.html
             */
            PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);
        }

        /* Resend */
        case 0xFE:
        {
            PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);
            UNIMPLEMENTED;
            break;
        }

        /* Reset */
        case 0xFF:
        {
            /* Send ACKnowledge */
            PS2QueuePush(KbdPS2Port, KEYBOARD_ACK);

            // So far, nothing to reset

            /* Send the Basic Assurance Test success code and the device ID */
            PS2QueuePush(KbdPS2Port, KEYBOARD_BAT_SUCCESS);
            PS2QueuePush(KbdPS2Port, KeyboardId);
            break;
        }

        /* Unknown command */
        default:
        {
            PS2QueuePush(KbdPS2Port, KEYBOARD_ERROR);
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID KeyboardEventHandler(PKEY_EVENT_RECORD KeyEvent)
{
    WORD i;
    BYTE ScanCode = (BYTE)KeyEvent->wVirtualScanCode;

    /* Check if we're not reporting */
    if (!KeyboardReporting) return;

    /* If this is a key release, set the highest bit in the scan code */
    if (!KeyEvent->bKeyDown) ScanCode |= 0x80;

    /* Push the scan code into the PS/2 queue */
    for (i = 0; i < KeyEvent->wRepeatCount; i++)
    {
        if (KeyEvent->dwControlKeyState & ENHANCED_KEY) PS2QueuePush(KbdPS2Port, 0xE0);
        PS2QueuePush(KbdPS2Port, ScanCode);
    }

    DPRINT("Press 0x%X\n", ScanCode);
}

BOOLEAN KeyboardInit(BYTE PS2Connector)
{
    /* Finish to plug the keyboard to the specified PS/2 port */
    KbdPS2Port = PS2Connector;
    PS2SetDeviceCmdProc(KbdPS2Port, NULL, KeyboardCommand);
    return TRUE;
}
