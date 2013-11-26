/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ps2.c
 * PURPOSE:         PS/2 controller emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "io.h"
#include "ps2.h"
#include "pic.h"

/* PRIVATE VARIABLES **********************************************************/

static BYTE KeyboardQueue[KEYBOARD_BUFFER_SIZE];
static BOOLEAN KeyboardQueueEmpty = TRUE;
static UINT KeyboardQueueStart = 0;
static UINT KeyboardQueueEnd = 0;
static BYTE KeyboardData = 0, KeyboardResponse = 0;
static BOOLEAN KeyboardReadResponse = FALSE, KeyboardWriteResponse = FALSE;
static BYTE KeyboardConfig = PS2_DEFAULT_CONFIG;

static HANDLE InputThread = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN KeyboardQueuePush(BYTE ScanCode)
{
    /* Check if the keyboard queue is full */
    if (!KeyboardQueueEmpty && (KeyboardQueueStart == KeyboardQueueEnd))
    {
        return FALSE;
    }

    /* Insert the value in the queue */
    KeyboardQueue[KeyboardQueueEnd] = ScanCode;
    KeyboardQueueEnd++;
    KeyboardQueueEnd %= KEYBOARD_BUFFER_SIZE;

    /* Since we inserted a value, it's not empty anymore */
    KeyboardQueueEmpty = FALSE;

    return TRUE;
}

static BOOLEAN KeyboardQueuePop(BYTE *ScanCode)
{
    /* Make sure the keyboard queue is not empty */
    if (KeyboardQueueEmpty) return FALSE;

    /* Get the scan code */
    *ScanCode = KeyboardQueue[KeyboardQueueStart];

    /* Remove the value from the queue */
    KeyboardQueueStart++;
    KeyboardQueueStart %= KEYBOARD_BUFFER_SIZE;

    /* Check if the queue is now empty */
    if (KeyboardQueueStart == KeyboardQueueEnd)
    {
        KeyboardQueueEmpty = TRUE;
    }

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BYTE KeyboardReadStatus()
{
    BYTE Status = 0;

    /* Set the first bit if the data can be read */
    if (KeyboardReadResponse || !KeyboardQueueEmpty) Status |= 1 << 0;

    /* Always set bit 2 */
    Status |= 1 << 2;

    /* Set bit 3 if the next byte goes to the controller */
    if (KeyboardWriteResponse) Status |= 1 << 3;

    return Status;
}

VOID KeyboardWriteCommand(BYTE Command)
{
    switch (Command)
    {
        /* Read configuration byte */
        case 0x20:
        {
            KeyboardResponse = KeyboardConfig;
            KeyboardReadResponse = TRUE;

            break;
        }

        /* Write configuration byte */
        case 0x60:
        /* Write controller output port */
        case 0xD1:
        /* Write keyboard output buffer */
        case 0xD2:
        /* Write mouse output buffer */
        case 0xD3:
        /* Write mouse input buffer */
        case 0xD4:
        {
            /* These commands require a response */
            KeyboardResponse = Command;
            KeyboardWriteResponse = TRUE;

            break;
        }

        /* Disable mouse */
        case 0xA7:
        {
            // TODO: Mouse support

            break;
        }

        /* Enable mouse */
        case 0xA8:
        {
            // TODO: Mouse support

            break;
        }

        /* Test mouse port */
        case 0xA9:
        {
            KeyboardResponse = 0;
            KeyboardReadResponse = TRUE;

            break;
        }

        /* Test PS/2 controller */
        case 0xAA:
        {
            KeyboardResponse = 0x55;
            KeyboardReadResponse = TRUE;

            break;
        }

        /* Disable keyboard */
        case 0xAD:
        {
            // TODO: Not implemented
            break;
        }

        /* Enable keyboard */
        case 0xAE:
        {
            // TODO: Not implemented
            break;
        }

        /* Read controller output port */
        case 0xD0:
        {
            // TODO: Not implemented
            break;
        }

        /* CPU Reset */
        case 0xF0:
        case 0xF2:
        case 0xF4:
        case 0xF6:
        case 0xF8:
        case 0xFA:
        case 0xFC:
        case 0xFE:
        {
            /* Stop the simulation */
            VdmRunning = FALSE;

            break;
        }
    }
}

BYTE KeyboardReadData()
{
    /* If there was a response byte from the controller, return it */
    if (KeyboardReadResponse)
    {
        KeyboardReadResponse = FALSE;
        KeyboardData = KeyboardResponse;
    }
    else
    {
        /* Otherwise, read the data from the queue */
        KeyboardQueuePop(&KeyboardData);
    }

    return KeyboardData;
}

VOID KeyboardWriteData(BYTE Data)
{
    /* Check if the controller is waiting for a response */
    if (KeyboardWriteResponse)
    {
        KeyboardWriteResponse = FALSE;

        /* Check which command it was */
        switch (KeyboardResponse)
        {
            /* Write configuration byte */
            case 0x60:
            {
                KeyboardConfig = Data;
                break;
            }

            /* Write controller output */
            case 0xD1:
            {
                /* Check if bit 0 is unset */
                if (!(Data & (1 << 0)))
                {
                    /* CPU disabled - end simulation */
                    VdmRunning = FALSE;
                }

                /* Update the A20 line setting */
                EmulatorSetA20(Data & (1 << 1));

                break;
            }
            
            case 0xD2:
            {
                /* Push the data byte to the keyboard queue */
                KeyboardQueuePush(Data);

                break;
            }

            case 0xD3:
            {
                // TODO: Mouse support
                break;
            }

            case 0xD4:
            {
                // TODO: Mouse support
                break;
            }
        }

        return;
    }

    // TODO: Implement PS/2 device commands
}

BYTE WINAPI PS2ReadPort(ULONG Port)
{
    if (Port == PS2_CONTROL_PORT)
        return KeyboardReadStatus();
    else if (Port == PS2_DATA_PORT)
        return KeyboardReadData();
    else
        return 0;
}

VOID WINAPI PS2WritePort(ULONG Port, BYTE Data)
{
    if (Port == PS2_CONTROL_PORT)
        KeyboardWriteCommand(Data);
    else if (Port == PS2_DATA_PORT)
        KeyboardWriteData(Data);
}

DWORD WINAPI InputThreadProc(LPVOID Parameter)
{
    INT i;
    HANDLE ConsoleInput = (HANDLE)Parameter;
    INPUT_RECORD InputRecord;
    DWORD Count;

    while (VdmRunning)
    {
        /* Wait for an input record */
        if (!ReadConsoleInput(ConsoleInput, &InputRecord, 1, &Count))
        {
            DWORD LastError = GetLastError();
            DPRINT1("Error reading console input (0x%p, %lu) - Error %lu\n", ConsoleInput, Count, LastError);
            return LastError;
        }

        ASSERT(Count != 0);

        /* Check the event type */
        switch (InputRecord.EventType)
        {
            case KEY_EVENT:
            {
                BYTE ScanCode = (BYTE)InputRecord.Event.KeyEvent.wVirtualScanCode;

                /* If this is a key release, set the highest bit in the scan code */
                if (!InputRecord.Event.KeyEvent.bKeyDown) ScanCode |= 0x80;

                /* Push the scan code onto the keyboard queue */
                for (i = 0; i < InputRecord.Event.KeyEvent.wRepeatCount; i++)
                {
                    KeyboardQueuePush(ScanCode);
                }

                /* Keyboard IRQ */
                PicInterruptRequest(1);

                break;
            }

            case MOUSE_EVENT:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            default:
            {
                /* Ignored */
                break;
            }
        }
    }

    return 0;
}

BOOLEAN PS2Initialize(HANDLE ConsoleInput)
{
    /* Start the input thread */
    InputThread = CreateThread(NULL, 0, &InputThreadProc, ConsoleInput, 0, NULL);

    // if (InputThread == NULL) return FALSE;

    /* Register the I/O Ports */
    RegisterIoPort(PS2_CONTROL_PORT, PS2ReadPort, PS2WritePort);
    RegisterIoPort(PS2_DATA_PORT   , PS2ReadPort, PS2WritePort);

    return TRUE;
}

VOID PS2Cleanup(VOID)
{
    /* Close the input thread handle */
    if (InputThread != NULL) CloseHandle(InputThread);
    InputThread = NULL;
}

/* EOF */
