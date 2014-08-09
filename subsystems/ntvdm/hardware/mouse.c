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

/* PRIVATE VARIABLES **********************************************************/

static MOUSE_MODE Mode, PreviousMode;
static COORD Position;
static ULONG WidthMm, HeightMm, WidthPixels, HeightPixels;
static ULONG SampleRate;
static ULONG Resolution;
static BOOLEAN Scaling;
static BOOLEAN Reporting;
static BYTE MouseId;
static ULONG ButtonState;
static SHORT HorzCounter;
static SHORT VertCounter;
static CHAR ScrollCounter;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID MouseResetConfig(VOID)
{
    /* Reset the configuration to defaults */
    SampleRate = 100;
    Resolution = 4;
    Scaling = FALSE;
    Reporting = FALSE;
}

static VOID MouseResetCounters(VOID)
{
    /* Reset all flags and counters */
    ButtonState = HorzCounter = VertCounter = ScrollCounter = 0;
}

static VOID MouseReset(VOID)
{
    /* Reset everything */
    MouseResetConfig();
    MouseResetCounters();

    /* Enter streaming mode and the reset the mouse ID */
    Mode = MOUSE_STREAMING_MODE;
    MouseId = 0;

    /* Send the Basic Assurance Test success code and the device ID */
    KeyboardQueuePush(MOUSE_BAT_SUCCESS);
    KeyboardQueuePush(MouseId);
}

#if 0
static VOID MouseGetPacket(PMOUSE_PACKET Packet)
{
    /* Clear the packet */
    ZeroMemory(Packet, sizeof(MOUSE_PACKET));

    Packet->Flags |= MOUSE_ALWAYS_SET;

    /* Check for horizontal overflows */
    if ((HorzCounter < MOUSE_MIN) || (HorzCounter > MOUSE_MAX))
    {
        if (HorzCounter > MOUSE_MAX) HorzCounter = MOUSE_MAX;
        if (HorzCounter < MOUSE_MIN) HorzCounter = MOUSE_MIN;

        Packet->Flags |= MOUSE_X_OVERFLOW;
    }

    /* Check for vertical overflows */
    if ((VertCounter < MOUSE_MIN) || (VertCounter > MOUSE_MAX))
    {
        if (VertCounter > MOUSE_MIN) VertCounter = MOUSE_MIN;
        if (VertCounter < MOUSE_MIN) VertCounter = MOUSE_MIN;

        Packet->Flags |= MOUSE_Y_OVERFLOW;
    }

    /* Set the sign flags */
    if (HorzCounter & MOUSE_SIGN_BIT) Packet->Flags |= MOUSE_X_SIGN;
    if (HorzCounter & MOUSE_SIGN_BIT) Packet->Flags |= MOUSE_Y_SIGN;

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
}
#endif

/* PUBLIC FUNCTIONS ***********************************************************/

VOID MouseUpdatePosition(PCOORD NewPosition)
{
    /* Update the counters */
    HorzCounter += ((NewPosition->X - Position.X) * WidthMm * Resolution) / WidthPixels;
    VertCounter += ((NewPosition->Y - Position.Y) * HeightMm * Resolution) / HeightPixels;

    /* Update the position */
    Position = *NewPosition;
}

VOID MouseUpdateButtons(ULONG NewButtonState)
{
    ButtonState = NewButtonState;
}

VOID MouseScroll(LONG Direction)
{
    ScrollCounter += Direction;
}

COORD MouseGetPosition(VOID)
{
    return Position;
}

VOID MouseCommand(BYTE Command)
{
    switch (Command)
    {
        /* Set 1:1 Scaling */
        case 0xE6:
        {
            Scaling = FALSE;
            KeyboardQueuePush(MOUSE_ACK);
            break;
        }

        /* Set 2:1 Scaling */
        case 0xE7:
        {
            Scaling = TRUE;
            KeyboardQueuePush(MOUSE_ACK);
            break;
        }

        /* Set Resolution */
        case 0xE8:
        {
            // TODO: NOT IMPLEMENTED
            UNIMPLEMENTED;
            break;
        }

        /* Read Status */
        case 0xE9:
        {
            // TODO: NOT IMPLEMENTED
            UNIMPLEMENTED;
            break;
        }

        /* Enter Streaming Mode */
        case 0xEA:
        {
            MouseResetCounters();
            Mode = MOUSE_STREAMING_MODE;

            KeyboardQueuePush(MOUSE_ACK);
            break;
        }

        /* Read Packet */
        case 0xEB:
        {
            // TODO: NOT IMPLEMENTED
            UNIMPLEMENTED;
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
                KeyboardQueuePush(MOUSE_ACK);
            }
            else KeyboardQueuePush(MOUSE_ERROR);

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

            KeyboardQueuePush(MOUSE_ACK);
            break;
        }

        /* Enter Remote Mode */
        case 0xF0:
        {
            MouseResetCounters();
            Mode = MOUSE_REMOTE_MODE;

            KeyboardQueuePush(MOUSE_ACK);
            break;
        }

        /* Get Mouse ID */
        case 0xF2:
        {
            KeyboardQueuePush(MOUSE_ACK);
            KeyboardQueuePush(MouseId);
            break;
        }

        /* Set Sample Rate */
        case 0xF3:
        {
            // TODO: NOT IMPLEMENTED
            UNIMPLEMENTED;
            break;
        }

        /* Enable Reporting */
        case 0xF4:
        {
            Reporting = TRUE;
            KeyboardQueuePush(MOUSE_ACK);
            break;
        }

        /* Disable Reporting */
        case 0xF5:
        {
            Reporting = FALSE;
            KeyboardQueuePush(MOUSE_ACK);
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
            // TODO: NOT IMPLEMENTED
            UNIMPLEMENTED;
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
            KeyboardQueuePush(MOUSE_ERROR);
        }
    }
}

BOOLEAN MouseInit(VOID)
{
    HWND hWnd;
    HDC hDC;

    /* Get the console window */
    hWnd = GetConsoleWindow();
    if (hWnd == NULL) return FALSE;

    /* Get the console window's device context */
    hDC = GetWindowDC(hWnd);
    if (hDC == NULL) return FALSE;

    /* Get the parameters */
    WidthMm      = (ULONG)GetDeviceCaps(hDC, HORZSIZE);
    HeightMm     = (ULONG)GetDeviceCaps(hDC, VERTSIZE);
    WidthPixels  = (ULONG)GetDeviceCaps(hDC, HORZRES);
    HeightPixels = (ULONG)GetDeviceCaps(hDC, VERTRES);

    /* Release the device context */
    ReleaseDC(hWnd, hDC);

    MouseReset();
    return TRUE;
}
