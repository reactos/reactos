/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            moubios32.c
 * PURPOSE:         VDM Mouse 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "callback.h"

#include "moubios32.h"
#include "bios32p.h"

#include "io.h"
#include "hardware/mouse.h"

/* PRIVATE VARIABLES **********************************************************/

static BOOLEAN DriverEnabled = TRUE;
static MOUSE_DRIVER_STATE DriverState;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID PaintMouseCursor(VOID)
{
    if (Bda->VideoMode <= 3)
    {
        WORD Character;
        DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Bda->VideoPage * Bda->VideoPageSize);

        EmulatorReadMemory(&EmulatorContext,
                           VideoAddress
                           + (DriverState.Position.Y * Bda->ScreenColumns
                           + DriverState.Position.X) * sizeof(WORD),
                           (LPVOID)&Character,
                           sizeof(WORD));

        DriverState.Character = Character;
        Character &= DriverState.TextCursor.ScreenMask;
        Character ^= DriverState.TextCursor.CursorMask;

        EmulatorWriteMemory(&EmulatorContext,
                            VideoAddress
                            + (DriverState.Position.Y * Bda->ScreenColumns
                            + DriverState.Position.X) * sizeof(WORD),
                            (LPVOID)&Character,
                            sizeof(WORD));
    }
    else
    {
        // TODO: NOT IMPLEMENTED
        UNIMPLEMENTED;
    }
}

static VOID EraseMouseCursor(VOID)
{
    if (Bda->VideoMode <= 3)
    {
        DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Bda->VideoPage * Bda->VideoPageSize);

        EmulatorWriteMemory(&EmulatorContext,
                            VideoAddress
                            + (DriverState.Position.Y * Bda->ScreenColumns
                            + DriverState.Position.X) * sizeof(WORD),
                            (LPVOID)&DriverState.Character,
                            sizeof(WORD));
    }
    else
    {
        // TODO: NOT IMPLEMENTED
        UNIMPLEMENTED;
    }
}

static VOID WINAPI BiosMouseService(LPWORD Stack)
{
    switch (getAX())
    {
        /* Reset Driver */
        case 0x00:
        {
            DriverEnabled = TRUE;
            DriverState.ShowCount = 0;

            /* Set the default text cursor */
            DriverState.TextCursor.ScreenMask = 0xFFFF; /* Display everything */
            DriverState.TextCursor.CursorMask = 0xFF00; /* ... but with inverted attributes */

            /* Set the default graphics cursor */
            DriverState.GraphicsCursor.HotSpot.X = 3;
            DriverState.GraphicsCursor.HotSpot.Y = 1;

            DriverState.GraphicsCursor.ScreenMask[0] = 0xC3FF;  // 1100001111111111
            DriverState.GraphicsCursor.ScreenMask[1] = 0xC0FF;  // 1100000011111111
            DriverState.GraphicsCursor.ScreenMask[2] = 0xC07F;  // 1100000001111111
            DriverState.GraphicsCursor.ScreenMask[3] = 0xC01F;  // 1100000000011111
            DriverState.GraphicsCursor.ScreenMask[4] = 0xC00F;  // 1100000000001111
            DriverState.GraphicsCursor.ScreenMask[5] = 0xC007;  // 1100000000000111
            DriverState.GraphicsCursor.ScreenMask[6] = 0xC003;  // 1100000000000011
            DriverState.GraphicsCursor.ScreenMask[7] = 0xC007;  // 1100000000000111
            DriverState.GraphicsCursor.ScreenMask[8] = 0xC01F;  // 1100000000011111
            DriverState.GraphicsCursor.ScreenMask[9] = 0xC01F;  // 1100000000011111
            DriverState.GraphicsCursor.ScreenMask[10] = 0xC00F; // 1100000000001111
            DriverState.GraphicsCursor.ScreenMask[11] = 0xC60F; // 1100011000001111
            DriverState.GraphicsCursor.ScreenMask[12] = 0xFF07; // 1111111100000111
            DriverState.GraphicsCursor.ScreenMask[13] = 0xFF07; // 1111111100000111
            DriverState.GraphicsCursor.ScreenMask[14] = 0xFF87; // 1111111110000111
            DriverState.GraphicsCursor.ScreenMask[15] = 0xFFCF; // 1111111111001111

            DriverState.GraphicsCursor.CursorMask[0] = 0x0000;  // 0000000000000000
            DriverState.GraphicsCursor.CursorMask[1] = 0x1C00;  // 0001110000000000
            DriverState.GraphicsCursor.CursorMask[2] = 0x1F00;  // 0001111100000000
            DriverState.GraphicsCursor.CursorMask[3] = 0x1F80;  // 0001111110000000
            DriverState.GraphicsCursor.CursorMask[4] = 0x1FE0;  // 0001111111100000
            DriverState.GraphicsCursor.CursorMask[5] = 0x1FF0;  // 0001111111110000
            DriverState.GraphicsCursor.CursorMask[6] = 0x1FF8;  // 0001111111111000
            DriverState.GraphicsCursor.CursorMask[7] = 0x1FE0;  // 0001111111100000
            DriverState.GraphicsCursor.CursorMask[8] = 0x1FC0;  // 0001111111000000
            DriverState.GraphicsCursor.CursorMask[9] = 0x1FC0;  // 0001111111000000
            DriverState.GraphicsCursor.CursorMask[10] = 0x19E0; // 0001100111100000
            DriverState.GraphicsCursor.CursorMask[11] = 0x00E0; // 0000000011100000
            DriverState.GraphicsCursor.CursorMask[12] = 0x0070; // 0000000001110000
            DriverState.GraphicsCursor.CursorMask[13] = 0x0070; // 0000000001110000
            DriverState.GraphicsCursor.CursorMask[14] = 0x0030; // 0000000000110000
            DriverState.GraphicsCursor.CursorMask[15] = 0x0000; // 0000000000000000

            break;
        }

        /* Show Mouse Cursor */
        case 0x01:
        {
            DriverState.ShowCount++;
            if (DriverState.ShowCount > 0) PaintMouseCursor();

            break;
        }

        /* Hide Mouse Cursor */
        case 0x02:
        {
            DriverState.ShowCount--;
            if (DriverState.ShowCount <= 0) EraseMouseCursor();

            break;
        }

        /* Return Position And Button Status */
        case 0x03:
        {
            setBX(DriverState.ButtonState);
            setCX(DriverState.Position.X);
            setDX(DriverState.Position.Y);

            break;
        }

        /* Position Mouse Cursor */
        case 0x04:
        {
            POINT Point;

            Point.x = getCX();
            Point.y = getDX();

            ClientToScreen(GetConsoleWindow(), &Point);
            SetCursorPos(Point.x, Point.y);

            break;
        }

        /* Return Button Press Data */
        case 0x05:
        {
            WORD Button = getBX();

            setAX(DriverState.ButtonState);
            setBX(DriverState.PressCount[Button]);
            setCX(DriverState.LastPress[Button].X);
            setDX(DriverState.LastPress[Button].Y);

            /* Reset the counter */
            DriverState.PressCount[Button] = 0;

            break;
        }

        /* Return Button Release Data */
        case 0x06:
        {
            WORD Button = getBX();

            setAX(DriverState.ButtonState);
            setBX(DriverState.ReleaseCount[Button]);
            setCX(DriverState.LastRelease[Button].X);
            setDX(DriverState.LastRelease[Button].Y);

            /* Reset the counter */
            DriverState.ReleaseCount[Button] = 0;

            break;

        }

        /* Define Graphics Cursor */
        case 0x09:
        {
            PWORD MaskBitmap = (PWORD)SEG_OFF_TO_PTR(getES(), getDX());

            DriverState.GraphicsCursor.HotSpot.X = getBX();
            DriverState.GraphicsCursor.HotSpot.Y = getCX();

            RtlMoveMemory(DriverState.GraphicsCursor.ScreenMask,
                          MaskBitmap,
                          sizeof(DriverState.GraphicsCursor.ScreenMask));

            RtlMoveMemory(DriverState.GraphicsCursor.CursorMask,
                          &MaskBitmap[16],
                          sizeof(DriverState.GraphicsCursor.CursorMask));

            break;
        }

        /* Define Text Cursor */
        case 0x0A:
        {
            DriverState.TextCursor.ScreenMask = getCX();
            DriverState.TextCursor.CursorMask = getDX();

            break;
        }

        /* Return Driver Storage Requirements */
        case 0x15:
        {
            setBX(sizeof(MOUSE_DRIVER_STATE));
            break;
        }

        /* Save Driver State */
        case 0x16:
        {
            *((PMOUSE_DRIVER_STATE)SEG_OFF_TO_PTR(getES(), getDX())) = DriverState;
            break;
        }

        /* Restore Driver State */
        case 0x17:
        {
            DriverState = *((PMOUSE_DRIVER_STATE)SEG_OFF_TO_PTR(getES(), getDX()));
            break;
        }

        /* Disable Mouse Driver */
        case 0x1F:
        {
            setES(0x0000);
            setBX(0x0000);

            DriverEnabled = FALSE;
            break;
        }

        /* Enable Mouse Driver */
        case 0x20:
        {
            DriverEnabled = TRUE;
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 33h, AX = 0x%04X NOT IMPLEMENTED\n", getAX());
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID MouseBiosUpdatePosition(PCOORD NewPosition)
{
    if (DriverEnabled && (DriverState.ShowCount > 0))
    {
        EraseMouseCursor();
        DriverState.Position = *NewPosition;
        PaintMouseCursor();
    }
}

VOID MouseBiosUpdateButtons(WORD ButtonState)
{
    WORD i;

    if (!DriverEnabled) return;

    for (i = 0; i < NUM_MOUSE_BUTTONS; i++)
    {
        BOOLEAN OldState = (DriverState.ButtonState >> i) & 1;
        BOOLEAN NewState = (ButtonState >> i) & 1;

        if (NewState > OldState)
        {
            /* Mouse press */
            DriverState.PressCount[i]++;
            DriverState.LastPress[i] = DriverState.Position;
        }
        else if (NewState < OldState)
        {
            /* Mouse release */
            DriverState.ReleaseCount[i]++;
            DriverState.LastRelease[i] = DriverState.Position;
        }
    }

    DriverState.ButtonState = ButtonState;
}

BOOLEAN MouseBios32Initialize(VOID)
{
    /* Clear the state */
    ZeroMemory(&DriverState, sizeof(DriverState));

    /* Initialize the interrupt handler */
    RegisterBiosInt32(BIOS_MOUSE_INTERRUPT, BiosMouseService);

    return TRUE;
}

VOID MouseBios32Cleanup(VOID)
{
    if (DriverState.ShowCount > 0) EraseMouseCursor();
}
