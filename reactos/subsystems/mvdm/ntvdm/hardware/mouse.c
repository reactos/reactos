/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            mouse.c
 * PURPOSE:         Mouse emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "mouse.h"
#include "ps2.h"
#include "clock.h"
#include "video/vga.h"

/* PRIVATE VARIABLES **********************************************************/

static const BYTE ScrollMagic[3]      = { 200, 100, 80 };
static const BYTE ExtraButtonMagic[3] = { 200, 200, 80 };

static HANDLE MouseMutex;
static PHARDWARE_TIMER StreamTimer;
static MOUSE_PACKET LastPacket;
static MOUSE_MODE Mode, PreviousMode;
static COORD Position;
static BYTE Resolution; /* Completely ignored */
static BOOLEAN Scaling; /* Completely ignored */
static BOOLEAN Reporting;
static BYTE MouseId;
static ULONG ButtonState;
static SHORT HorzCounter;
static SHORT VertCounter;
static CHAR ScrollCounter;
static BOOLEAN EventsOccurred = FALSE;
static BYTE DataByteWait = 0;
static BYTE ScrollMagicCounter = 0, ExtraButtonMagicCounter = 0;

static BYTE PS2Port = 1;

/* PUBLIC VARIABLES ***********************************************************/

UINT MouseCycles = 10;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID MouseResetConfig(VOID)
{
    /* Reset the configuration to defaults */
    MouseCycles = 10;
    Resolution = 4;
    Scaling = FALSE;
    Reporting = FALSE;
}

static VOID MouseResetCounters(VOID)
{
    /* Reset all flags and counters */
    HorzCounter = VertCounter = ScrollCounter = 0;
}

static VOID MouseReset(VOID)
{
    /* Reset everything */
    MouseResetConfig();
    MouseResetCounters();

    /* Enter streaming mode and the reset the mouse ID */
    Mode = MOUSE_STREAMING_MODE;
    MouseId = 0;
    ScrollMagicCounter = ExtraButtonMagicCounter = 0;

    /* Send the Basic Assurance Test success code and the device ID */
    PS2QueuePush(PS2Port, MOUSE_BAT_SUCCESS);
    PS2QueuePush(PS2Port, MouseId);
}

static VOID MouseGetPacket(PMOUSE_PACKET Packet)
{
    /* Clear the packet */
    RtlZeroMemory(Packet, sizeof(*Packet));

    /* Acquire the mutex */
    WaitForSingleObject(MouseMutex, INFINITE);

    Packet->Flags |= MOUSE_ALWAYS_SET;

    /* Set the sign flags */
    if (HorzCounter < 0)
    {
        Packet->Flags |= MOUSE_X_SIGN;
        HorzCounter = -HorzCounter;
    }

    if (VertCounter < 0)
    {
        Packet->Flags |= MOUSE_Y_SIGN;
        VertCounter = -VertCounter;
    }

    /* Check for horizontal overflows */
    if (HorzCounter > MOUSE_MAX)
    {
        HorzCounter = MOUSE_MAX;
        Packet->Flags |= MOUSE_X_OVERFLOW;
    }

    /* Check for vertical overflows */
    if (VertCounter > MOUSE_MAX)
    {
        VertCounter = MOUSE_MAX;
        Packet->Flags |= MOUSE_Y_OVERFLOW;
    }

    /* Set the button flags */
    if (ButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) Packet->Flags |= MOUSE_LEFT_BUTTON;
    if (ButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) Packet->Flags |= MOUSE_MIDDLE_BUTTON;
    if (ButtonState & RIGHTMOST_BUTTON_PRESSED) Packet->Flags |= MOUSE_RIGHT_BUTTON;

    if (MouseId == 4)
    {
        if (ButtonState & FROM_LEFT_3RD_BUTTON_PRESSED) Packet->Extra |= MOUSE_4TH_BUTTON;
        if (ButtonState & FROM_LEFT_4TH_BUTTON_PRESSED) Packet->Extra |= MOUSE_5TH_BUTTON;
    }

    if (MouseId >= 3)
    {
        /* Set the scroll counter */
        Packet->Extra |= (UCHAR)ScrollCounter & 0x0F;
    }

    /* Store the counters in the packet */
    Packet->HorzCounter = LOBYTE(HorzCounter);
    Packet->VertCounter = LOBYTE(VertCounter);

    /* Reset the counters */
    MouseResetCounters();

    /* Release the mutex */
    ReleaseMutex(MouseMutex);
}

static VOID MouseDispatchPacket(PMOUSE_PACKET Packet)
{
    PS2QueuePush(PS2Port, Packet->Flags);
    PS2QueuePush(PS2Port, Packet->HorzCounter);
    PS2QueuePush(PS2Port, Packet->VertCounter);
    if (MouseId >= 3) PS2QueuePush(PS2Port, Packet->Extra);
}

static VOID WINAPI MouseCommand(LPVOID Param, BYTE Command)
{
    /* Check if we were waiting for a data byte */
    if (DataByteWait)
    {
        PS2QueuePush(PS2Port, MOUSE_ACK);

        switch (DataByteWait)
        {
            /* Set Resolution */
            case 0xE8:
            {
                Resolution = Command;
                break;
            }

            /* Set Sample Rate */
            case 0xF3:
            {
                /* Check for the scroll wheel enabling sequence */
                if (MouseId == 0)
                {
                    if (Command == ScrollMagic[ScrollMagicCounter])
                    {
                        ScrollMagicCounter++;
                        if (ScrollMagicCounter == 3) MouseId = 3;
                    }
                    else
                    {
                        ScrollMagicCounter = 0;
                    }
                }

                /* Check for the 5-button enabling sequence */
                if (MouseId == 3)
                {
                    if (Command == ExtraButtonMagic[ExtraButtonMagicCounter])
                    {
                        ExtraButtonMagicCounter++;
                        if (ExtraButtonMagicCounter == 3) MouseId = 4;
                    }
                    else
                    {
                        ExtraButtonMagicCounter = 0;
                    }
                }

                MouseCycles = 1000 / (UINT)Command;
                break;
            }

            default:
            {
                /* Shouldn't happen */
                ASSERT(FALSE);
            }
        }

        DataByteWait = 0;
    }

    /* Check if we're in wrap mode */
    if (Mode == MOUSE_WRAP_MODE)
    {
        /*
         * In this mode, we just echo whatever byte we get,
         * except for the 0xEC and 0xFF commands.
         */
        if (Command != 0xEC && Command != 0xFF)
        {
            PS2QueuePush(PS2Port, Command);
            return;
        }
    }

    switch (Command)
    {
        /* Set 1:1 Scaling */
        case 0xE6:
        {
            Scaling = FALSE;
            PS2QueuePush(PS2Port, MOUSE_ACK);
            break;
        }

        /* Set 2:1 Scaling */
        case 0xE7:
        {
            Scaling = TRUE;
            PS2QueuePush(PS2Port, MOUSE_ACK);
            break;
        }

        /* Set Resolution */
        case 0xE8:
        /* Set Sample Rate */
        case 0xF3:
        {
            PS2QueuePush(PS2Port, MOUSE_ACK);
            DataByteWait = Command;
            break;
        }

        /* Read Status */
        case 0xE9:
        {
            BYTE Status = ButtonState & 7;
            PS2QueuePush(PS2Port, MOUSE_ACK);

            if (Scaling) Status |= 1 << 4;
            if (Reporting) Status |= 1 << 5;
            if (Mode == MOUSE_REMOTE_MODE) Status |= 1 << 6;

            PS2QueuePush(PS2Port, Status);
            PS2QueuePush(PS2Port, Resolution);
            PS2QueuePush(PS2Port, (BYTE)(1000 / MouseCycles));
            break;
        }

        /* Enter Streaming Mode */
        case 0xEA:
        {
            MouseResetCounters();
            Mode = MOUSE_STREAMING_MODE;

            PS2QueuePush(PS2Port, MOUSE_ACK);
            break;
        }

        /* Read Packet */
        case 0xEB:
        {
            PS2QueuePush(PS2Port, MOUSE_ACK);
            MouseGetPacket(&LastPacket);
            MouseDispatchPacket(&LastPacket);
            break;
        }

        /* Return From Wrap Mode */
        case 0xEC:
        {
            if (Mode == MOUSE_WRAP_MODE)
            {
                /* Restore the previous mode */
                MouseResetCounters();
                Mode = PreviousMode;
                PS2QueuePush(PS2Port, MOUSE_ACK);
            }
            else PS2QueuePush(PS2Port, MOUSE_ERROR);

            break;
        }

        /* Enter Wrap Mode */
        case 0xEE:
        {
            if (Mode != MOUSE_WRAP_MODE)
            {
                /* Save the previous mode */
                PreviousMode = Mode;
            }

            MouseResetCounters();
            Mode = MOUSE_WRAP_MODE;

            PS2QueuePush(PS2Port, MOUSE_ACK);
            break;
        }

        /* Enter Remote Mode */
        case 0xF0:
        {
            MouseResetCounters();
            Mode = MOUSE_REMOTE_MODE;

            PS2QueuePush(PS2Port, MOUSE_ACK);
            break;
        }

        /* Get Mouse ID */
        case 0xF2:
        {
            PS2QueuePush(PS2Port, MOUSE_ACK);
            PS2QueuePush(PS2Port, MouseId);
            break;
        }

        /* Enable Reporting */
        case 0xF4:
        {
            Reporting = TRUE;
            PS2QueuePush(PS2Port, MOUSE_ACK);
            break;
        }

        /* Disable Reporting */
        case 0xF5:
        {
            Reporting = FALSE;
            PS2QueuePush(PS2Port, MOUSE_ACK);
            break;
        }

        /* Set Defaults */
        case 0xF6:
        {
            /* Reset the configuration and counters */
            MouseResetConfig();
            MouseResetCounters();
            break;
        }

        /* Resend */
        case 0xFE:
        {
            PS2QueuePush(PS2Port, MOUSE_ACK);
            MouseDispatchPacket(&LastPacket);
            break;
        }

        /* Reset */
        case 0xFF:
        {
            MouseReset();
            break;
        }

        /* Unknown command */
        default:
        {
            PS2QueuePush(PS2Port, MOUSE_ERROR);
        }
    }
}

static VOID FASTCALL MouseStreamingCallback(ULONGLONG ElapsedTime)
{
    UNREFERENCED_PARAMETER(ElapsedTime);

    /* Check if we're not in streaming mode, not reporting, or there's nothing to report */
    if (Mode != MOUSE_STREAMING_MODE || !Reporting || !EventsOccurred) return;

    MouseGetPacket(&LastPacket);
    MouseDispatchPacket(&LastPacket);

    EventsOccurred = FALSE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID MouseGetDataFast(PCOORD CurrentPosition, PBYTE CurrentButtonState)
{
    WaitForSingleObject(MouseMutex, INFINITE);
    *CurrentPosition = Position;
    *CurrentButtonState = LOBYTE(ButtonState);
    ReleaseMutex(MouseMutex);
}

VOID MouseEventHandler(PMOUSE_EVENT_RECORD MouseEvent)
{
    COORD NewPosition = MouseEvent->dwMousePosition;
    BOOLEAN DoubleWidth = FALSE, DoubleHeight = FALSE;

    if (!VgaGetDoubleVisionState(&DoubleWidth, &DoubleHeight))
    {
        /* Text mode */
        NewPosition.X *= 8;
        NewPosition.Y *= 8;
    }

    /* Adjust for double vision */
    if (DoubleWidth) NewPosition.X /= 2;
    if (DoubleHeight) NewPosition.Y /= 2;

    WaitForSingleObject(MouseMutex, INFINITE);

    /* Update the counters */
    HorzCounter += NewPosition.X - Position.X;
    VertCounter += NewPosition.Y - Position.Y;

    /* Update the position */
    Position = NewPosition;

    /* Update the button state */
    ButtonState = MouseEvent->dwButtonState;

    if (MouseEvent->dwEventFlags & MOUSE_WHEELED)
    {
        ScrollCounter += (SHORT)HIWORD(MouseEvent->dwButtonState);
    }

    EventsOccurred = TRUE;
    ReleaseMutex(MouseMutex);
}

BOOLEAN MouseInit(BYTE PS2Connector)
{
    /* Finish to plug the mouse to the specified PS/2 port */
    PS2Port = PS2Connector;
    PS2SetDeviceCmdProc(PS2Port, NULL, MouseCommand);

    MouseMutex = CreateMutex(NULL, FALSE, NULL);
    if (MouseMutex == NULL) return FALSE;

    StreamTimer = CreateHardwareTimer(HARDWARE_TIMER_ENABLED, 10, MouseStreamingCallback);

    MouseReset();
    return TRUE;
}
