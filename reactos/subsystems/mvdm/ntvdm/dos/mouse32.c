/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/mouse32.c
 * PURPOSE:         VDM 32-bit compatible PS/2 MOUSE.COM driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

/* Driver Version number and Copyright */
#include <reactos/buildno.h>
#include <reactos/version.h>

#include "ntvdm.h"
#include "emulator.h"

#include "cpu/cpu.h"
#include "int32.h"
#include "hardware/mouse.h"
#include "hardware/ps2.h"
#include "hardware/pic.h"
#include "hardware/video/svga.h"

#include "mouse32.h"
#include "bios/bios.h"
#include "bios/bios32/bios32p.h"

#include "memory.h"
#include "io.h"
#include "dos32krnl/memory.h"

/* PRIVATE VARIABLES **********************************************************/

static const CHAR MouseCopyright[] =
    "ReactOS PS/2 16/32-bit Mouse Driver Compatible MS-MOUSE 6.26\r\n"
    "Version "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")\r\n"
    "Copyright (C) ReactOS Team 1996-"COPYRIGHT_YEAR"\0";

#pragma pack(push, 1)

typedef struct _MOUSE_DRIVER
{
    CHAR Copyright[sizeof(MouseCopyright)];
    WORD Version;
    BYTE MouseContextScratch[TRAMPOLINE_SIZE];
    BYTE MouseDosInt16Stub[Int16To32StubSize];
    BYTE MouseIrqInt16Stub[Int16To32StubSize];
} MOUSE_DRIVER, *PMOUSE_DRIVER;

#pragma pack(pop)

/* Global data contained in guest memory */
static WORD MouseDataSegment;
static PMOUSE_DRIVER MouseData;
static CALLBACK16 MouseContext;

#define MICKEYS_PER_CELL_HORIZ  8
#define MICKEYS_PER_CELL_VERT   16

static BOOLEAN DriverEnabled = FALSE;
static MOUSE_DRIVER_STATE DriverState;
static DWORD OldIrqHandler;
static DWORD OldIntHandler;

static WORD DefaultGfxScreenMask[16] =
{
    0xC3FF,     // 1100001111111111
    0xC0FF,     // 1100000011111111
    0xC07F,     // 1100000001111111
    0xC01F,     // 1100000000011111
    0xC00F,     // 1100000000001111
    0xC007,     // 1100000000000111
    0xC003,     // 1100000000000011
    0xC007,     // 1100000000000111
    0xC01F,     // 1100000000011111
    0xC01F,     // 1100000000011111
    0xC00F,     // 1100000000001111
    0xC60F,     // 1100011000001111
    0xFF07,     // 1111111100000111
    0xFF07,     // 1111111100000111
    0xFF87,     // 1111111110000111
    0xFFCF,     // 1111111111001111
};

static WORD DefaultGfxCursorMask[16] =
{
    0x0000,     // 0000000000000000
    0x1C00,     // 0001110000000000
    0x1F00,     // 0001111100000000
    0x1F80,     // 0001111110000000
    0x1FE0,     // 0001111111100000
    0x1FF0,     // 0001111111110000
    0x1FF8,     // 0001111111111000
    0x1FE0,     // 0001111111100000
    0x1FC0,     // 0001111111000000
    0x1FC0,     // 0001111111000000
    0x19E0,     // 0001100111100000
    0x00E0,     // 0000000011100000
    0x0070,     // 0000000001110000
    0x0070,     // 0000000001110000
    0x0030,     // 0000000000110000
    0x0000,     // 0000000000000000
};

/* PRIVATE FUNCTIONS **********************************************************/

/* static */
VOID BiosPs2Service(UCHAR Function)
{
    /* Save AX and BX */
    USHORT AX = getAX();
    // USHORT BX = getBX();

    /*
     * Set the parameters:
     * AL contains the character to print (already set),
     * BL contains the character attribute,
     * BH contains the video page to use.
     */
    // setBL(DOS_CHAR_ATTRIBUTE);
    // setBH(Bda->VideoPage);
    setAL(Function);

    /* Call the BIOS INT 15h, AH=C2h "Pointing Device BIOS Interface (PS)" */
    setAH(0xC2);
    Int32Call(&MouseContext, BIOS_MISC_INTERRUPT);

    /* Restore AX and BX */
    // setBX(BX);
    setAX(AX);
}



static VOID DosMouseEnable(VOID);
static VOID DosMouseDisable(VOID);


static VOID PaintMouseCursor(VOID)
{
    COORD Position = DriverState.Position;

    /* Apply the clipping rectangle */
    if (Position.X < DriverState.MinX) Position.X = DriverState.MinX;
    if (Position.X > DriverState.MaxX) Position.X = DriverState.MaxX;
    if (Position.Y < DriverState.MinY) Position.Y = DriverState.MinY;
    if (Position.Y > DriverState.MaxY) Position.Y = DriverState.MaxY;

    if (Bda->VideoMode <= 3)
    {
        WORD Character;
        WORD CellX = Position.X / 8;
        WORD CellY = Position.Y / 8;
        DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Bda->VideoPage * Bda->VideoPageSize);

        EmulatorReadMemory(&EmulatorContext,
                           VideoAddress
                           + (CellY * Bda->ScreenColumns + CellX) * sizeof(WORD),
                           (LPVOID)&Character,
                           sizeof(WORD));

        DriverState.Character = Character;
        Character &= DriverState.TextCursor.ScreenMask;
        Character ^= DriverState.TextCursor.CursorMask;

        EmulatorWriteMemory(&EmulatorContext,
                            VideoAddress
                            + (CellY * Bda->ScreenColumns + CellX) * sizeof(WORD),
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
    COORD Position = DriverState.Position;

    /* Apply the clipping rectangle */
    if (Position.X < DriverState.MinX) Position.X = DriverState.MinX;
    if (Position.X > DriverState.MaxX) Position.X = DriverState.MaxX;
    if (Position.Y < DriverState.MinY) Position.Y = DriverState.MinY;
    if (Position.Y > DriverState.MaxY) Position.Y = DriverState.MaxY;

    if (Bda->VideoMode <= 3)
    {
        WORD CellX = Position.X / 8;
        WORD CellY = Position.Y / 8;
        DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Bda->VideoPage * Bda->VideoPageSize);

        EmulatorWriteMemory(&EmulatorContext,
                            VideoAddress
                            + (CellY * Bda->ScreenColumns + CellX) * sizeof(WORD),
                            (LPVOID)&DriverState.Character,
                            sizeof(WORD));
    }
    else
    {
        // TODO: NOT IMPLEMENTED
        UNIMPLEMENTED;
    }
}

static VOID ToMouseCoordinates(PCOORD Position)
{
    COORD Resolution = VgaGetDisplayResolution();
    DWORD Width = DriverState.MaxX - DriverState.MinX + 1;
    DWORD Height = DriverState.MaxY - DriverState.MinY + 1;

    if (!VgaGetDoubleVisionState(NULL, NULL))
    {
        Resolution.X *= 8;
        Resolution.Y *= 8;
    }

    Position->X = DriverState.MinX + ((Position->X * Width) / Resolution.X);
    Position->Y = DriverState.MinY + ((Position->Y * Height) / Resolution.Y);
}

static VOID FromMouseCoordinates(PCOORD Position)
{
    COORD Resolution = VgaGetDisplayResolution();
    DWORD Width = DriverState.MaxX - DriverState.MinX + 1;
    DWORD Height = DriverState.MaxY - DriverState.MinY + 1;

    if (!VgaGetDoubleVisionState(NULL, NULL))
    {
        Resolution.X *= 8;
        Resolution.Y *= 8;
    }

    Position->X = ((Position->X - DriverState.MinX) * Resolution.X) / Width;
    Position->Y = ((Position->Y - DriverState.MinY) * Resolution.Y) / Height;
}

static VOID CallMouseUserHandlers(USHORT CallMask)
{
    USHORT i;
    USHORT AX, BX, CX, DX, BP, SI, DI, DS, ES;
    COORD Position = DriverState.Position;

    ToMouseCoordinates(&Position);

    /* Call handler 0 */
    if ((DriverState.Handler0.CallMask & CallMask) != 0 &&
         DriverState.Handler0.Callback != (ULONG)NULL)
    {
        /*
         * Set the parameters for the callback.
         * NOTE: In text modes, the row and column will be reported
         *       as a multiple of the cell size, typically 8x8 pixels.
         */

        AX = getAX();
        BX = getBX();
        CX = getCX();
        DX = getDX();
        BP = getBP();
        SI = getSI();
        DI = getDI();
        DS = getDS();
        ES = getES();

        setAX(CallMask);
        setBX(DriverState.ButtonState);
        setCX(Position.X);
        setDX(Position.Y);
        setSI(MICKEYS_PER_CELL_HORIZ);
        setDI(MICKEYS_PER_CELL_VERT);

        DPRINT("Calling Handler0 %04X:%04X with CallMask 0x%04X\n",
               HIWORD(DriverState.Handler0.Callback),
               LOWORD(DriverState.Handler0.Callback),
               CallMask);

        /* Call the callback */
        RunCallback16(&MouseContext, DriverState.Handler0.Callback);

        setAX(AX);
        setBX(BX);
        setCX(CX);
        setDX(DX);
        setBP(BP);
        setSI(SI);
        setDI(DI);
        setDS(DS);
        setES(ES);
    }

    for (i = 0; i < ARRAYSIZE(DriverState.Handlers); ++i)
    {
        /* Call the suitable handlers */
        if ((DriverState.Handlers[i].CallMask & CallMask) != 0 &&
             DriverState.Handlers[i].Callback != (ULONG)NULL)
        {
            /*
             * Set the parameters for the callback.
             * NOTE: In text modes, the row and column will be reported
             *       as a multiple of the cell size, typically 8x8 pixels.
             */

            AX = getAX();
            BX = getBX();
            CX = getCX();
            DX = getDX();
            BP = getBP();
            SI = getSI();
            DI = getDI();
            DS = getDS();
            ES = getES();

            setAX(CallMask);
            setBX(DriverState.ButtonState);
            setCX(Position.X);
            setDX(Position.Y);
            setSI(MICKEYS_PER_CELL_HORIZ);
            setDI(MICKEYS_PER_CELL_VERT);

            DPRINT1("Calling Handler[%d] %04X:%04X with CallMask 0x%04X\n",
                    i,
                    HIWORD(DriverState.Handlers[i].Callback),
                    LOWORD(DriverState.Handlers[i].Callback),
                    CallMask);

            /* Call the callback */
            RunCallback16(&MouseContext, DriverState.Handlers[i].Callback);

            setAX(AX);
            setBX(BX);
            setCX(CX);
            setDX(DX);
            setBP(BP);
            setSI(SI);
            setDI(DI);
            setDS(DS);
            setES(ES);
        }
    }
}

static inline VOID DosUpdatePosition(PCOORD NewPosition)
{
    COORD Resolution = VgaGetDisplayResolution();

    /* Check for text mode */
    if (!VgaGetDoubleVisionState(NULL, NULL))
    {
        Resolution.X *= 8;
        Resolution.Y *= 8;
    }

    if (DriverState.ShowCount > 0) EraseMouseCursor();
    DriverState.Position = *NewPosition;
    if (DriverState.ShowCount > 0) PaintMouseCursor();

    /* Call the mouse handlers */
    CallMouseUserHandlers(0x0001); // We use MS MOUSE v1.0+ format
}

static inline VOID DosUpdateButtons(BYTE ButtonState) // WORD ButtonState
{
    USHORT i;
    USHORT CallMask = 0x0000; // We use MS MOUSE v1.0+ format

    for (i = 0; i < NUM_MOUSE_BUTTONS; i++)
    {
        BOOLEAN OldState = (DriverState.ButtonState >> i) & 1;
        BOOLEAN NewState = (ButtonState >> i) & 1;

        if (NewState > OldState)
        {
            /* Mouse press */
            DriverState.PressCount[i]++;
            DriverState.LastPress[i] = DriverState.Position;

            CallMask |= (1 << (2 * i + 1));
        }
        else if (NewState < OldState)
        {
            /* Mouse release */
            DriverState.ReleaseCount[i]++;
            DriverState.LastRelease[i] = DriverState.Position;

            CallMask |= (1 << (2 * i + 2));
        }
    }

    DriverState.ButtonState = ButtonState;

    /* Call the mouse handlers */
    CallMouseUserHandlers(CallMask);
}

static VOID WINAPI DosMouseIrq(LPWORD Stack)
{
    BYTE Flags;
    SHORT DeltaX, DeltaY;
    COORD Position;
    BYTE ButtonState;

    /* Read the whole packet at once */
    Flags = IOReadB(PS2_DATA_PORT);
    PS2PortQueueRead(1); // NOTE: Should be a IOReadB! But see r67231
    DeltaX = IOReadB(PS2_DATA_PORT);
    PS2PortQueueRead(1); // NOTE: Should be a IOReadB! But see r67231
    DeltaY = IOReadB(PS2_DATA_PORT);

    /* Adjust the sign */
    if (Flags & MOUSE_X_SIGN) DeltaX = -DeltaX;
    if (Flags & MOUSE_Y_SIGN) DeltaY = -DeltaY;

    /* Update the counters */
    DriverState.HorizCount += DeltaX;
    DriverState.VertCount  += DeltaY;

    /*
     * Get the absolute position directly from the mouse, this is the only
     * way to perfectly synchronize the host and guest mouse pointer.
     */
    MouseGetDataFast(&Position, &ButtonState);

    /* Call the update subroutines */
    DosUpdatePosition(&Position);
    DosUpdateButtons(ButtonState);

    /* Complete the IRQ */
    PicIRQComplete(LOBYTE(Stack[STACK_INT_NUM]));
}

static VOID WINAPI DosMouseService(LPWORD Stack)
{
    switch (getAX())
    {
        /* Reset Driver */
        case 0x00:
        {
            SHORT i;

            DriverState.ShowCount = 0;
            DriverState.ButtonState = 0;

            /* Initialize the default clipping range */
            DriverState.MinX = 0;
            DriverState.MaxX = MOUSE_MAX_HORIZ - 1;
            DriverState.MinY = 0;
            DriverState.MaxY = MOUSE_MAX_VERT - 1;

            /* Set the default text cursor */
            DriverState.TextCursor.ScreenMask = 0xFFFF; /* Display everything */
            DriverState.TextCursor.CursorMask = 0xFF00; /* ... but with inverted attributes */

            /* Set the default graphics cursor */
            DriverState.GraphicsCursor.HotSpot.X = 3;
            DriverState.GraphicsCursor.HotSpot.Y = 1;

            RtlCopyMemory(DriverState.GraphicsCursor.ScreenMask,
                          DefaultGfxScreenMask,
                          sizeof(DriverState.GraphicsCursor.ScreenMask));

            RtlCopyMemory(DriverState.GraphicsCursor.CursorMask,
                          DefaultGfxCursorMask,
                          sizeof(DriverState.GraphicsCursor.CursorMask));

            /* Initialize the counters */
            DriverState.HorizCount = DriverState.VertCount = 0;

            for (i = 0; i < NUM_MOUSE_BUTTONS; i++)
            {
                DriverState.PressCount[i] = DriverState.ReleaseCount[i] = 0;
            }

            /* Return mouse information */
            setAX(0xFFFF);  // Hardware & driver installed
            setBX(NUM_MOUSE_BUTTONS);

            break;
        }

        /* Show Mouse Cursor */
        case 0x01:
        {
            DriverState.ShowCount++;
            if (DriverState.ShowCount == 1) PaintMouseCursor();

            break;
        }

        /* Hide Mouse Cursor */
        case 0x02:
        {
            DriverState.ShowCount--;
            if (DriverState.ShowCount == 0) EraseMouseCursor();

            break;
        }

        /* Return Position and Button Status */
        case 0x03:
        {
            COORD Position = DriverState.Position;
            ToMouseCoordinates(&Position);

            setBX(DriverState.ButtonState);
            setCX(Position.X);
            setDX(Position.Y);
            break;
        }

        /* Position Mouse Cursor */
        case 0x04:
        {
            COORD Position = { getCX(), getDX() };
            FromMouseCoordinates(&Position);

            DriverState.Position = Position;
            break;
        }

        /* Return Button Press Data */
        case 0x05:
        {
            WORD Button = getBX();
            COORD LastPress = DriverState.LastPress[Button];
            ToMouseCoordinates(&LastPress);

            setAX(DriverState.ButtonState);
            setBX(DriverState.PressCount[Button]);
            setCX(LastPress.X);
            setDX(LastPress.Y);

            /* Reset the counter */
            DriverState.PressCount[Button] = 0;

            break;
        }

        /* Return Button Release Data */
        case 0x06:
        {
            WORD Button = getBX();
            COORD LastRelease = DriverState.LastRelease[Button];
            ToMouseCoordinates(&LastRelease);

            setAX(DriverState.ButtonState);
            setBX(DriverState.ReleaseCount[Button]);
            setCX(LastRelease.X);
            setDX(LastRelease.Y);

            /* Reset the counter */
            DriverState.ReleaseCount[Button] = 0;

            break;

        }

        /* Define Horizontal Cursor Range */
        case 0x07:
        {
            WORD Min = getCX();
            WORD Max = getDX();

            if (!VgaGetDoubleVisionState(NULL, NULL))
            {
                /* Text mode */
                Min &= ~0x07;
                Max |= 0x07;
            }

            DPRINT("Setting mouse horizontal range: %u - %u\n", Min, Max);
            DriverState.MinX = Min;
            DriverState.MaxX = Max;
            break;
        }

        /* Define Vertical Cursor Range */
        case 0x08:
        {
            WORD Min = getCX();
            WORD Max = getDX();

            if (!VgaGetDoubleVisionState(NULL, NULL))
            {
                /* Text mode */
                Min &= ~0x07;
                Max |= 0x07;
            }

            DPRINT("Setting mouse vertical range: %u - %u\n", Min, Max);
            DriverState.MinY = Min;
            DriverState.MaxY = Max;
            break;
        }

        /* Define Graphics Cursor */
        case 0x09:
        {
            PWORD MaskBitmap = (PWORD)SEG_OFF_TO_PTR(getES(), getDX());

            DriverState.GraphicsCursor.HotSpot.X = getBX();
            DriverState.GraphicsCursor.HotSpot.Y = getCX();

            RtlCopyMemory(DriverState.GraphicsCursor.ScreenMask,
                          MaskBitmap,
                          sizeof(DriverState.GraphicsCursor.ScreenMask));

            RtlCopyMemory(DriverState.GraphicsCursor.CursorMask,
                          &MaskBitmap[16],
                          sizeof(DriverState.GraphicsCursor.CursorMask));

            break;
        }

        /* Define Text Cursor */
        case 0x0A:
        {
            USHORT BX = getBX();

            if (BX == 0x0000)
            {
                /* Define software cursor */
                DriverState.TextCursor.ScreenMask = getCX();
                DriverState.TextCursor.CursorMask = getDX();
            }
            else if (BX == 0x0001)
            {
                /* Define hardware cursor */
                DPRINT1("Defining hardware cursor is unimplemented\n");
                UNIMPLEMENTED;
                // CX == start scan line
                // DX == end scan line
            }
            else
            {
                DPRINT1("Invalid BX value 0x%04X\n", BX);
            }

            break;
        }

        /* Read Motion Counters */
        case 0x0B:
        {
            setCX(DriverState.HorizCount);
            setDX(DriverState.VertCount);

            /* Reset the counters */
            DriverState.HorizCount = DriverState.VertCount = 0;

            break;
        }

        /* Define Interrupt Subroutine Parameters, compatible MS MOUSE v1.0+ */
        case 0x0C:
        {
            DriverState.Handler0.CallMask = getCX();
            DriverState.Handler0.Callback = MAKELONG(getDX(), getES()); // Far pointer to the callback
            DPRINT1("Define callback 0x%04X, %04X:%04X\n",
                    DriverState.Handler0.CallMask,
                    HIWORD(DriverState.Handler0.Callback),
                    LOWORD(DriverState.Handler0.Callback));
            break;
        }

        /* Define Mickey/Pixel Ratio */
        case 0x0F:
        {
            /* This call should be completely ignored */
            break;
        }

        /* Set Exclusion Area */
        // http://www.ctyme.com/intr/rb-5972.htm
        // http://www.techhelpmanual.com/849-int_33h_0010h__set_exclusion_area.html
        //case 0x10:
        //{
        //}

        /* Define Double-Speed Threshold */
        case 0x13:
        {
            DPRINT1("INT 33h, AH=13h: Mouse double-speed threshold is UNSUPPORTED\n");
            break;
        }

        /* Exchange Interrupt Subroutines, compatible MS MOUSE v3.0+ (see function 0x0C) */
        case 0x14:
        {
            USHORT OldCallMask = DriverState.Handler0.CallMask;
            ULONG  OldCallback = DriverState.Handler0.Callback;

            DriverState.Handler0.CallMask = getCX();
            DriverState.Handler0.Callback = MAKELONG(getDX(), getES()); // Far pointer to the callback
            DPRINT1("Exchange old callback 0x%04X, %04X:%04X with new callback 0x%04X, %04X:%04X\n",
                    OldCallMask,
                    HIWORD(OldCallback),
                    LOWORD(OldCallback),
                    DriverState.Handler0.CallMask,
                    HIWORD(DriverState.Handler0.Callback),
                    LOWORD(DriverState.Handler0.Callback));

            /* Return old callmask in CX and callback vector in ES:DX */
            setCX(OldCallMask);
            setES(HIWORD(OldCallback));
            setDX(LOWORD(OldCallback));
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
            /* Check whether the user buffer has correct size and fail if not */
            if (getBX() != sizeof(MOUSE_DRIVER_STATE)) break;

            *((PMOUSE_DRIVER_STATE)SEG_OFF_TO_PTR(getES(), getDX())) = DriverState;
            break;
        }

        /* Restore Driver State */
        case 0x17:
        {
            /* Check whether the user buffer has correct size and fail if not */
            if (getBX() != sizeof(MOUSE_DRIVER_STATE)) break;

            DriverState = *((PMOUSE_DRIVER_STATE)SEG_OFF_TO_PTR(getES(), getDX()));
            break;
        }

        /* Set Alternate Mouse User Handler, compatible MS MOUSE v6.0+ */
        case 0x18:
        {
            /*
             * Up to three handlers can be defined by separate calls to this
             * function, each with a different combination of shift states in
             * the call mask; calling this function again with a call mask of
             * 0000h undefines the specified handler (official documentation);
             * specifying the same call mask and an address of 0000h:0000h
             * undefines the handler (real life).
             * See Ralf Brown: http://www.ctyme.com/intr/rb-5981.htm
             * for more information.
             */

            USHORT i;
            USHORT CallMask = getCX();
            ULONG  Callback = MAKELONG(getDX(), getES()); // Far pointer to the callback
            BOOLEAN Success = FALSE;

            DPRINT1("Define v6.0+ callback 0x%04X, %04X:%04X\n",
                    CallMask, HIWORD(Callback), LOWORD(Callback));

            if (CallMask == 0x0000)
            {
                /*
                 * Find the handler entry corresponding to the given
                 * callback and undefine it.
                 */
                for (i = 0; i < ARRAYSIZE(DriverState.Handlers); ++i)
                {
                    if (DriverState.Handlers[i].Callback == Callback)
                    {
                        /* Found it, undefine the handler */
                        DriverState.Handlers[i].CallMask = 0x0000;
                        DriverState.Handlers[i].Callback = (ULONG)NULL;
                        Success = TRUE;
                        break;
                    }
                }
            }
            else if (Callback == (ULONG)NULL)
            {
                /*
                 * Find the handler entry corresponding to the given
                 * callmask and undefine it.
                 */
                for (i = 0; i < ARRAYSIZE(DriverState.Handlers); ++i)
                {
                    if (DriverState.Handlers[i].CallMask == CallMask)
                    {
                        /* Found it, undefine the handler */
                        DriverState.Handlers[i].CallMask = 0x0000;
                        DriverState.Handlers[i].Callback = (ULONG)NULL;
                        Success = TRUE;
                        break;
                    }
                }
            }
            else
            {
                /*
                 * Try to find a handler entry corresponding to the given
                 * callmask to redefine it, otherwise find an empty handler
                 * entry and set the new handler in there.
                 */

                USHORT EmptyHandler = 0xFFFF; // Invalid handler

                for (i = 0; i < ARRAYSIZE(DriverState.Handlers); ++i)
                {
                    /* Find the first empty handler */
                    if (EmptyHandler == 0xFFFF &&
                        DriverState.Handlers[i].CallMask == 0x0000 &&
                        DriverState.Handlers[i].Callback == (ULONG)NULL)
                    {
                        EmptyHandler = i;
                    }

                    if (DriverState.Handlers[i].CallMask == CallMask)
                    {
                        /* Found it, redefine the handler */
                        DriverState.Handlers[i].CallMask = CallMask;
                        DriverState.Handlers[i].Callback = Callback;
                        Success = TRUE;
                        break;
                    }
                }

                /*
                 * If we haven't found anything and we found
                 * an empty handler, set it.
                 */
                if (!Success && EmptyHandler != 0xFFFF
                    /* && EmptyHandler < ARRAYSIZE(DriverState.Handlers) */)
                {
                    DriverState.Handlers[EmptyHandler].CallMask = CallMask;
                    DriverState.Handlers[EmptyHandler].Callback = Callback;
                    Success = TRUE;
                }
            }

            /* If we failed, set error code */
            if (!Success) setAX(0xFFFF);

            break;
        }

        /* Return User Alternate Interrupt Vector, compatible MS MOUSE v6.0+ */
        case 0x19:
        {
            USHORT i;
            USHORT CallMask = getCX();
            ULONG  Callback;
            BOOLEAN Success = FALSE;

            /*
             * Find the handler entry corresponding to the given callmask.
             */
            for (i = 0; i < ARRAYSIZE(DriverState.Handlers); ++i)
            {
                if (DriverState.Handlers[i].CallMask == CallMask)
                {
                    /* Found it */
                    Callback = DriverState.Handlers[i].Callback;
                    Success = TRUE;
                    break;
                }
            }

            if (Success)
            {
                /* Return the callback vector in BX:DX */
                setBX(HIWORD(Callback));
                setDX(LOWORD(Callback));
            }
            else
            {
                /* We failed, set error code */
                setCX(0x0000);
            }

            break;
        }

        /* Set Mouse Sensitivity */
        case 0x1A:
        {
            DPRINT1("INT 33h, AH=1Ah: Mouse sensitivity is UNSUPPORTED\n");

            // FIXME: Do that at runtime!

            // UCHAR BH = getBH();
            // setBH(0x00);
            // BiosPs2Service(0x00);
            // FIXME: Check for return status in AH and CF
            // setBH(BH);

            break;
        }

        /* Return Mouse Sensitivity */
        case 0x1B:
        {
            DPRINT1("INT 33h, AH=1Bh: Mouse sensitivity is UNSUPPORTED\n");

            /* Return default values */
            setBX(50); // Horizontal speed
            setCX(50); //   Vertical speed
            setDX(50); // Double speed threshold

            // FIXME: Get that at runtime!

            // UCHAR BH = getBH();
            // setBH(0x00);
            // BiosPs2Service(0x00);
            // FIXME: Check for return status in AH and CF
            // setBH(BH);

            break;
        }

        /* Disable Mouse Driver */
        case 0x1F:
        {
            /* INT 33h vector before the mouse driver was first installed */
            setES(HIWORD(OldIntHandler));
            setBX(LOWORD(OldIntHandler));

            DosMouseDisable();
            // UCHAR BH = getBH();
            // setBH(0x00);
            // BiosPs2Service(0x00);
            // FIXME: Check for return status in AH and CF
            // setBH(BH);
            break;
        }

        /* Enable Mouse Driver */
        case 0x20:
        {
            DosMouseEnable();
            // UCHAR BH = getBH();
            // setBH(0x01);
            // BiosPs2Service(0x00);
            // FIXME: Check for return status in AH and CF
            // setBH(BH);
            break;
        }

        /* Software Reset */
        case 0x21:
        {
            /*
             * See: http://www.htl-steyr.ac.at/~morg/pcinfo/hardware/interrupts/inte3sq8.htm
             * for detailed information and differences with respect to subfunction 0x00:
             * http://www.htl-steyr.ac.at/~morg/pcinfo/hardware/interrupts/inte3j74.htm
             */

            SHORT i;

            DriverState.ShowCount = 0;
            DriverState.ButtonState = 0;

            /* Initialize the default clipping range */
            DriverState.MinX = 0;
            DriverState.MaxX = MOUSE_MAX_HORIZ - 1;
            DriverState.MinY = 0;
            DriverState.MaxY = MOUSE_MAX_VERT - 1;

            /* Initialize the counters */
            DriverState.HorizCount = DriverState.VertCount = 0;

            for (i = 0; i < NUM_MOUSE_BUTTONS; i++)
            {
                DriverState.PressCount[i] = DriverState.ReleaseCount[i] = 0;
            }

            /* Return mouse information */
            setAX(0xFFFF);  // Hardware & driver installed
            setBX(NUM_MOUSE_BUTTONS);

            break;
        }

        /* Get Software Version, Mouse Type, and IRQ Number, compatible MS MOUSE v6.26+ */
        case 0x24:
        {
            setBX(MOUSE_VERSION); // Version Number

            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-5993.htm
             * for the list of possible values.
             */
            // FIXME: To be determined at runtime!
            setCH(0x04); // PS/2 Type
            setCL(0x00); // PS/2 Interrupt

            break;
        }

        // BIOS Function INT 33h, AX = 0x0025 NOT IMPLEMENTED
        case 0x25:
        {
            setAX(0);
            setBX(0);
            setCX(0);
            setDX(0);
            UNIMPLEMENTED;
            break;
        }

        /* Get Maximum Virtual Coordinates */
        case 0x26:
        {
            setBX(!DriverEnabled);
            // FIXME: In fact the MaxX and MaxY here are
            // theoretical values for the current video mode.
            // They therefore can be different from the current
            // min/max values.
            // See http://www.ctyme.com/intr/rb-5995.htm
            // for more details.
            setCX(DriverState.MaxX);
            setDX(DriverState.MaxY);
            break;
        }

        /* Get Current Minimum/Maximum Virtual Coordinates */
        case 0x31:
        {
            setAX(DriverState.MinX);
            setBX(DriverState.MinY);
            setCX(DriverState.MaxX);
            setDX(DriverState.MaxY);
            break;
        }

#if 0
        case 0x33:
        {
              /*
              * Related to http://www.ctyme.com/intr/rb-5985.htm
              * INT 33h, AX=001Ch "SET INTERRUPT RATE":

              * Values for mouse interrupt rate:
              * BX = rate
                00h    no interrupts allowed
                01h    30 per second
                02h    50 per second
                03h    100 per second
                04h    200 per second
              */
        }
#endif

        /* Return Pointer to Copyright String */
        case 0x4D:
        {
            setES(MouseDataSegment);
            setDI(FIELD_OFFSET(MOUSE_DRIVER, Copyright));
            break;
        }

        /* Get Version String (pointer) */
        case 0x6D:
        {
            /*
             * The format of the version "string" is:
             * Offset  Size    Description
             * 00h     BYTE    major version
             * 01h     BYTE    minor version (BCD)
             */
            setES(MouseDataSegment);
            setDI(FIELD_OFFSET(MOUSE_DRIVER, Version));
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 33h, AX = 0x%04X NOT IMPLEMENTED\n", getAX());
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

static
VOID DosMouseEnable(VOID)
{
    if (DriverEnabled) return;

    DriverEnabled = TRUE;

    /* Get the old IRQ handler */
    OldIrqHandler = ((PDWORD)BaseAddress)[MOUSE_IRQ_INT];

    /* Set the IRQ handler */
    RegisterInt32(MAKELONG(FIELD_OFFSET(MOUSE_DRIVER, MouseIrqInt16Stub), MouseDataSegment),
                  MOUSE_IRQ_INT, DosMouseIrq, NULL);
}

static
VOID DosMouseDisable(VOID)
{
    if (!DriverEnabled) return;

    /* Restore the old IRQ handler */
    ((PDWORD)BaseAddress)[MOUSE_IRQ_INT] = OldIrqHandler;

    DriverEnabled = FALSE;
}

BOOLEAN DosMouseInitialize(VOID)
{
    /* Initialize some memory for storing our data that should be available to DOS */
    MouseDataSegment = DosAllocateMemory(sizeof(MOUSE_DRIVER), NULL);
    if (MouseDataSegment == 0) return FALSE;
    MouseData = (PMOUSE_DRIVER)SEG_OFF_TO_PTR(MouseDataSegment, 0x0000);

    /* Initialize the callback context */
    InitializeContext(&MouseContext, MouseDataSegment, FIELD_OFFSET(MOUSE_DRIVER, MouseContextScratch));

    /* Clear the state */
    RtlZeroMemory(&DriverState, sizeof(DriverState));

    /* Mouse Driver Copyright */
    RtlCopyMemory(MouseData->Copyright, MouseCopyright, sizeof(MouseCopyright)-1);

    /* Mouse Driver Version in BCD format, compatible MS-MOUSE */
    MouseData->Version = MAKEWORD(MOUSE_VERSION/0x0100, MOUSE_VERSION%0x0100);

    /* Get the old mouse service interrupt handler */
    OldIntHandler = ((PDWORD)BaseAddress)[DOS_MOUSE_INTERRUPT];

    /* Initialize the interrupt handler */
    RegisterInt32(MAKELONG(FIELD_OFFSET(MOUSE_DRIVER, MouseDosInt16Stub), MouseDataSegment),
                  DOS_MOUSE_INTERRUPT, DosMouseService, NULL);

    DosMouseEnable();
    // UCHAR BH = getBH();
    // setBH(0x01);
    // BiosPs2Service(0x00);
    // FIXME: Check for return status in AH and CF
    // setBH(BH);

    return TRUE;
}

VOID DosMouseCleanup(VOID)
{
    if (DriverState.ShowCount > 0) EraseMouseCursor();
    DosMouseDisable();
    // UCHAR BH = getBH();
    // setBH(0x00);
    // BiosPs2Service(0x00);
    // FIXME: Check for return status in AH and CF
    // setBH(BH);

    /* Restore the old mouse service interrupt handler */
    ((PDWORD)BaseAddress)[DOS_MOUSE_INTERRUPT] = OldIntHandler;

    DosFreeMemory(MouseDataSegment);
    MouseDataSegment = 0;
    MouseData = 0;
}

/* EOF */
