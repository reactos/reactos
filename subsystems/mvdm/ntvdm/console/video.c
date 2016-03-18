/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/video/console.c
 * PURPOSE:         Console driver for the video subsystem
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

/** HACK!! **/
#if 0

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "svga.h"

#include "console.h"

#endif
/** HACK!! **/


/* PRIVATE VARIABLES **********************************************************/

static CONSOLE_CURSOR_INFO         OrgConsoleCursorInfo;
static CONSOLE_SCREEN_BUFFER_INFO  OrgConsoleBufferInfo;


static HANDLE ScreenBufferHandle = NULL;
static PVOID  OldConsoleFramebuffer = NULL;


/*
 * Text mode -- we always keep a valid text mode framebuffer
 * even if we are in graphics mode. This is needed in order
 * to keep a consistent VGA state. However, each time the VGA
 * detaches from the console (and reattaches to it later on),
 * this text mode framebuffer is recreated.
 */
static HANDLE TextConsoleBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
static COORD  TextResolution = {0};
/// static PCHAR_CELL TextFramebuffer = NULL;

/*
 * Graphics mode
 */
static HANDLE GraphicsConsoleBuffer = NULL;
/// static PVOID  GraphicsFramebuffer = NULL;
static HANDLE ConsoleMutex = NULL;
/* DoubleVision support */
static BOOLEAN DoubleWidth  = FALSE;
static BOOLEAN DoubleHeight = FALSE;



/*
 * Activate this line if you want to use the real
 * RegisterConsoleVDM API of ReactOS/Windows.
 */
// #define USE_REAL_REGISTERCONSOLEVDM

static HANDLE StartEvent = NULL;
static HANDLE EndEvent   = NULL;
static HANDLE AnotherEvent = NULL;

/* RegisterConsoleVDM EMULATION ***********************************************/

#include <ntddvdeo.h>

#ifdef USE_REAL_REGISTERCONSOLEVDM

#define __RegisterConsoleVDM        RegisterConsoleVDM
#define __InvalidateConsoleDIBits   InvalidateConsoleDIBits

#else

/*
 * This private buffer, per-console, is used by
 * RegisterConsoleVDM and InvalidateConsoleDIBits.
 */
static COORD VDMBufferSize  = {0};
static PCHAR_CELL VDMBuffer = NULL;

static PCHAR_INFO CharBuff  = NULL; // This is a hack, which is unneeded
                                    // for the real RegisterConsoleVDM and
                                    // InvalidateConsoleDIBits

BOOL
WINAPI
__RegisterConsoleVDM(IN DWORD dwRegisterFlags,
                     IN HANDLE hStartHardwareEvent,
                     IN HANDLE hEndHardwareEvent,
                     IN HANDLE hErrorHardwareEvent,
                     IN DWORD dwUnusedVar,
                     OUT LPDWORD lpVideoStateLength,
                     OUT PVOID* lpVideoState, // PVIDEO_HARDWARE_STATE_HEADER*
                     IN PVOID lpUnusedBuffer,
                     IN DWORD dwUnusedBufferLength,
                     IN COORD dwVDMBufferSize,
                     OUT PVOID* lpVDMBuffer)
{
    UNREFERENCED_PARAMETER(hErrorHardwareEvent);
    UNREFERENCED_PARAMETER(dwUnusedVar);
    UNREFERENCED_PARAMETER(lpVideoStateLength);
    UNREFERENCED_PARAMETER(lpVideoState);
    UNREFERENCED_PARAMETER(lpUnusedBuffer);
    UNREFERENCED_PARAMETER(dwUnusedBufferLength);

    SetLastError(0);
    DPRINT1("__RegisterConsoleVDM(%d)\n", dwRegisterFlags);

    if (lpVDMBuffer == NULL) return FALSE;

    if (dwRegisterFlags != 0)
    {
        // if (hStartHardwareEvent == NULL || hEndHardwareEvent == NULL) return FALSE;
        if (VDMBuffer != NULL) return FALSE;

        VDMBufferSize = dwVDMBufferSize;

        /* HACK: Cache -- to be removed in the real implementation */
        CharBuff = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   VDMBufferSize.X * VDMBufferSize.Y
                                                   * sizeof(*CharBuff));
        ASSERT(CharBuff);

        VDMBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    VDMBufferSize.X * VDMBufferSize.Y
                                                    * sizeof(*VDMBuffer));
        *lpVDMBuffer = VDMBuffer;
        return (VDMBuffer != NULL);
    }
    else
    {
        /* HACK: Cache -- to be removed in the real implementation */
        if (CharBuff) RtlFreeHeap(RtlGetProcessHeap(), 0, CharBuff);
        CharBuff = NULL;

        if (VDMBuffer) RtlFreeHeap(RtlGetProcessHeap(), 0, VDMBuffer);
        VDMBuffer = NULL;

        VDMBufferSize.X = VDMBufferSize.Y = 0;

        return TRUE;
    }
}

BOOL
__InvalidateConsoleDIBits(IN HANDLE hConsoleOutput,
                          IN PSMALL_RECT lpRect)
{
    if ((hConsoleOutput == TextConsoleBuffer) && (VDMBuffer != NULL))
    {
        /* HACK: Write the cached data to the console */

        COORD Origin = { lpRect->Left, lpRect->Top };
        SHORT i, j;

        ASSERT(CharBuff);

        for (i = 0; i < VDMBufferSize.Y; i++)
        {
            for (j = 0; j < VDMBufferSize.X; j++)
            {
                CharBuff[i * VDMBufferSize.X + j].Char.AsciiChar = VDMBuffer[i * VDMBufferSize.X + j].Char;
                CharBuff[i * VDMBufferSize.X + j].Attributes     = VDMBuffer[i * VDMBufferSize.X + j].Attributes;
            }
        }

        WriteConsoleOutputA(hConsoleOutput,
                            CharBuff,
                            VDMBufferSize,
                            Origin,
                            lpRect);
    }

    return InvalidateConsoleDIBits(hConsoleOutput, lpRect);
}

#endif


/* PRIVATE FUNCTIONS **********************************************************/


/*********/
static VOID VgaUpdateTextCursor(VOID);
static inline DWORD VgaGetAddressSize(VOID);
/*********/




static VOID ResizeTextConsole(PCOORD Resolution, PSMALL_RECT WindowSize OPTIONAL)
{
    BOOL Success;
    SMALL_RECT ConRect;
    SHORT oldWidth, oldHeight;

    /*
     * Use this trick to effectively resize the console buffer and window,
     * because:
     * - SetConsoleScreenBufferSize fails if the new console screen buffer size
     *   is smaller than the current console window size, and:
     * - SetConsoleWindowInfo fails if the new console window size is larger
     *   than the current console screen buffer size.
     */


    /* Retrieve the latest console information */
    GetConsoleScreenBufferInfo(TextConsoleBuffer, &ConsoleInfo);

    oldWidth  = ConsoleInfo.srWindow.Right  - ConsoleInfo.srWindow.Left + 1;
    oldHeight = ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top  + 1;

    /*
     * If the current console window is too large to hold the full contents
     * of the new screen buffer, resize it first.
     */
    if (oldWidth > Resolution->X || oldHeight > Resolution->Y)
    {
        //
        // NOTE: This is not a problem if we move the window back to (0,0)
        // because when we resize the screen buffer, the window will move back
        // to where the cursor is. Or, if the screen buffer is not resized,
        // when we readjust again the window, we will move back to a correct
        // position. This is what we wanted after all...
        //

        ConRect.Left   = ConRect.Top = 0;
        ConRect.Right  = ConRect.Left + min(oldWidth , Resolution->X) - 1;
        ConRect.Bottom = ConRect.Top  + min(oldHeight, Resolution->Y) - 1;

        Success = SetConsoleWindowInfo(TextConsoleBuffer, TRUE, &ConRect);
        if (!Success) DPRINT1("(resize) SetConsoleWindowInfo(1) failed with error %d\n", GetLastError());
    }

    /* Resize the screen buffer if needed */
    if (Resolution->X != ConsoleInfo.dwSize.X || Resolution->Y != ConsoleInfo.dwSize.Y)
    {
        /*
         * SetConsoleScreenBufferSize automatically takes into account the current
         * cursor position when it computes starting which row it should copy text
         * when resizing the sceenbuffer, and scrolls the console window such that
         * the cursor is placed in it again. We therefore do not need to care about
         * the cursor position and do the maths ourselves.
         */
        Success = SetConsoleScreenBufferSize(TextConsoleBuffer, *Resolution);
        if (!Success) DPRINT1("(resize) SetConsoleScreenBufferSize failed with error %d\n", GetLastError());

        /*
         * Setting a new screen buffer size can change other information,
         * so update the saved console information.
         */
        GetConsoleScreenBufferInfo(TextConsoleBuffer, &ConsoleInfo);
    }

    if (!WindowSize)
    {
        ConRect.Left   = 0;
        ConRect.Right  = ConRect.Left + Resolution->X - 1;
        ConRect.Bottom = max(ConsoleInfo.dwCursorPosition.Y, Resolution->Y - 1);
        ConRect.Top    = ConRect.Bottom - Resolution->Y + 1;

        // NOTE: We may take ConsoleInfo.dwMaximumWindowSize into account
    }
    else
    {
        ConRect.Left   = ConRect.Top = 0;
        ConRect.Right  = ConRect.Left + WindowSize->Right  - WindowSize->Left;
        ConRect.Bottom = ConRect.Top  + WindowSize->Bottom - WindowSize->Top ;
    }

    Success = SetConsoleWindowInfo(TextConsoleBuffer, TRUE, &ConRect);
    if (!Success) DPRINT1("(resize) SetConsoleWindowInfo(2) failed with error %d\n", GetLastError());

    /* Update the saved console information */
    GetConsoleScreenBufferInfo(TextConsoleBuffer, &ConsoleInfo);
}

static VOID UpdateCursorPosition(VOID)
{
    /*
     * Update the cursor position in the VGA registers.
     */
    WORD Offset = ConsoleInfo.dwCursorPosition.Y * TextResolution.X +
                  ConsoleInfo.dwCursorPosition.X;

    VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_LOW_REG]  = LOBYTE(Offset);
    VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_HIGH_REG] = HIBYTE(Offset);

    VgaUpdateTextCursor();
}

static BOOL AttachToConsoleInternal(PCOORD Resolution)
{
    BOOL Success;
    ULONG Length = 0;
    PVIDEO_HARDWARE_STATE_HEADER State;

#ifdef USE_REAL_REGISTERCONSOLEVDM
    PCHAR_INFO CharBuff = NULL;
#endif
    SHORT i, j;
    DWORD AddressSize, ScanlineSize;
    DWORD Address = 0;
    DWORD CurrentAddr;
    SMALL_RECT ConRect;
    COORD Origin = { 0, 0 };

    ASSERT(TextFramebuffer == NULL);

    TextResolution = *Resolution;

    /*
     * Windows 2k3 winsrv.dll calls NtVdmControl(VdmQueryVdmProcess == 14, &ConsoleHandle);
     * in the two following APIs:
     * SrvRegisterConsoleVDM  (corresponding Win32 API: RegisterConsoleVDM)
     * SrvVDMConsoleOperation (corresponding Win32 API: VDMConsoleOperation)
     * to check whether the current process is a VDM process, and fails otherwise
     * with the error 0xC0000022 (STATUS_ACCESS_DENIED).
     *
     * It is worth it to notice that also basesrv.dll does the same only for the
     * BaseSrvIsFirstVDM API...
     */

    /* Register with the console server */
    Success =
    __RegisterConsoleVDM(1,
                         StartEvent,
                         EndEvent,
                         AnotherEvent, // NULL,
                         0,
                         &Length, // NULL, <-- putting this (and null in the next var) makes the API returning error 12 "ERROR_INVALID_ACCESS"
                         (PVOID*)&State, // NULL,
                         NULL,
                         0,
                         TextResolution,
                         (PVOID*)&TextFramebuffer);
    if (!Success)
    {
        DisplayMessage(L"RegisterConsoleVDM failed with error %d\n", GetLastError());
        EmulatorTerminate();
        return FALSE;
    }

#ifdef USE_REAL_REGISTERCONSOLEVDM
    CharBuff = RtlAllocateHeap(RtlGetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               TextResolution.X * TextResolution.Y
                                                * sizeof(*CharBuff));
    ASSERT(CharBuff);
#endif

    /* Resize the console */
    ResizeTextConsole(Resolution, NULL);

    /* Update the saved console information */
    GetConsoleScreenBufferInfo(TextConsoleBuffer, &ConsoleInfo);

    /*
     * Copy console data into VGA memory
     */

    /* Read the data from the console into the framebuffer... */
    ConRect.Left   = ConRect.Top = 0;
    ConRect.Right  = TextResolution.X;
    ConRect.Bottom = TextResolution.Y;

    ReadConsoleOutputA(TextConsoleBuffer,
                       CharBuff,
                       TextResolution,
                       Origin,
                       &ConRect);

    /* ... and copy the framebuffer into the VGA memory */
    AddressSize  = VgaGetAddressSize();
    ScanlineSize = (DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG] * 2;

    /* Loop through the scanlines */
    for (i = 0; i < TextResolution.Y; i++)
    {
        /* Loop through the characters */
        for (j = 0; j < TextResolution.X; j++)
        {
            CurrentAddr = LOWORD((Address + j) * AddressSize);

            /* Store the character in plane 0 */
            VgaMemory[CurrentAddr] = CharBuff[i * TextResolution.X + j].Char.AsciiChar;

            /* Store the attribute in plane 1 */
            VgaMemory[CurrentAddr + VGA_BANK_SIZE] = (BYTE)CharBuff[i * TextResolution.X + j].Attributes;
        }

        /* Move to the next scanline */
        Address += ScanlineSize;
    }

#ifdef USE_REAL_REGISTERCONSOLEVDM
    if (CharBuff) RtlFreeHeap(RtlGetProcessHeap(), 0, CharBuff);
#endif

    UpdateCursorPosition();

    return TRUE;
}

static VOID DetachFromConsoleInternal(VOID)
{
    ULONG dummyLength;
    PVOID dummyPtr;
    COORD dummySize = {0};

    /* Deregister with the console server */
    __RegisterConsoleVDM(0,
                         NULL,
                         NULL,
                         NULL,
                         0,
                         &dummyLength,
                         &dummyPtr,
                         NULL,
                         0,
                         dummySize,
                         &dummyPtr);

    TextFramebuffer = NULL;
}

static BOOL IsConsoleHandle(HANDLE hHandle)
{
    DWORD dwMode;

    /* Check whether the handle may be that of a console... */
    if ((GetFileType(hHandle) & ~FILE_TYPE_REMOTE) != FILE_TYPE_CHAR)
        return FALSE;

    /*
     * It may be. Perform another test... The idea comes from the
     * MSDN description of the WriteConsole API:
     *
     * "WriteConsole fails if it is used with a standard handle
     *  that is redirected to a file. If an application processes
     *  multilingual output that can be redirected, determine whether
     *  the output handle is a console handle (one method is to call
     *  the GetConsoleMode function and check whether it succeeds).
     *  If the handle is a console handle, call WriteConsole. If the
     *  handle is not a console handle, the output is redirected and
     *  you should call WriteFile to perform the I/O."
     */
    return GetConsoleMode(hHandle, &dwMode);
}

static VOID SetActiveScreenBuffer(HANDLE ScreenBuffer)
{
    ASSERT(ScreenBuffer);

    /* Set the active buffer and reattach the VDM UI to it */
    SetConsoleActiveScreenBuffer(ScreenBuffer);
    ConsoleReattach(ScreenBuffer);
}

VOID ScreenEventHandler(PWINDOW_BUFFER_SIZE_RECORD ScreenEvent)
{
    /*
     * This function monitors and allows console resizings only if they are triggered by us.
     * User-driven resizings via the console properties, or programmatical console resizings
     * made by explicit calls to SetConsoleScreenBufferSize by external applications, are forbidden.
     * In that case only a console window resize is done in case the size is reduced.
     * This protection is enabled in CONSRV side when NTVDM registers as a VDM to CONSRV,
     * but we also implement it there in case we are running in STANDALONE mode without
     * CONSRV registration.
     *
     * The only potential problem we have is that, when this handler is called,
     * the console is already resized. In case this corresponds to a forbidden resize,
     * we resize the console back to its original size from inside the handler.
     * This will trigger a recursive call to the handler, that should be detected.
     */

    if (CurrResolution.X == ScreenEvent->dwSize.X &&
        CurrResolution.Y == ScreenEvent->dwSize.Y)
    {
        /* Allowed resize, we are OK */
        return;
    }

    DPRINT1("ScreenEventHandler - Detected forbidden resize! Reset console screenbuffer size back to (X = %d ; Y = %d)\n", CurrResolution.X, CurrResolution.Y);

    // FIXME: If we're detaching, then stop monitoring for changes!!

    /* Restore the original console size */
    ResizeTextConsole(&CurrResolution, NULL);

    /* Force refresh of all the screen */
    NeedsUpdate = TRUE;
    UpdateRectangle.Left = 0;
    UpdateRectangle.Top  = 0;
    UpdateRectangle.Right  = CurrResolution.X;
    UpdateRectangle.Bottom = CurrResolution.Y;
    VgaRefreshDisplay();
}

BOOLEAN VgaGetDoubleVisionState(PBOOLEAN Horizontal, PBOOLEAN Vertical)
{
    if (GraphicsConsoleBuffer == NULL) return FALSE;
    if (Horizontal) *Horizontal = DoubleWidth;
    if (Vertical)   *Vertical   = DoubleHeight;
    return TRUE;
}

BOOL VgaAttachToConsole(VOID)
{
    if (TextResolution.X == 0 || TextResolution.Y == 0)
        DPRINT1("VgaAttachToConsole -- TextResolution uninitialized\n");

    if (TextResolution.X == 0) TextResolution.X = 80;
    if (TextResolution.Y == 0) TextResolution.Y = 25;

    // DetachFromConsoleInternal();

    /*
     * AttachToConsoleInternal sets TextResolution
     * to the new resolution and updates ConsoleInfo.
     */
    if (!AttachToConsoleInternal(&TextResolution))
    {
        DisplayMessage(L"An unexpected error occurred!\n");
        EmulatorTerminate();
        return FALSE;
    }

    /* Restore the original screen buffer */
    SetActiveScreenBuffer(ScreenBufferHandle);
    ScreenBufferHandle = NULL;

    /* Restore the screen state */
    if (ScreenMode == TEXT_MODE)
    {
        /* The text mode framebuffer was recreated */
        ActiveFramebuffer = TextFramebuffer;
    }
    else
    {
        /* The graphics mode framebuffer is unchanged */
        ActiveFramebuffer = OldConsoleFramebuffer;
    }
    OldConsoleFramebuffer = NULL;

    return TRUE;
}

VOID VgaDetachFromConsole(VOID)
{
    DetachFromConsoleInternal();

    /* Save the screen state */
    if (ScreenMode == TEXT_MODE)
        ScreenBufferHandle = TextConsoleBuffer;
    else
        ScreenBufferHandle = GraphicsConsoleBuffer;

    /* Reset the active framebuffer */
    OldConsoleFramebuffer = ActiveFramebuffer;
    ActiveFramebuffer = NULL;

    /* Restore the original console size */
    ResizeTextConsole(&OrgConsoleBufferInfo.dwSize, &OrgConsoleBufferInfo.srWindow);

    /* Restore the original cursor shape */
    SetConsoleCursorInfo(TextConsoleBuffer, &OrgConsoleCursorInfo);

    // FIXME: Should we copy back the screen data to the screen buffer??
    // WriteConsoleOutputA(...);

    // FIXME: Should we change cursor POSITION??
    // VgaUpdateTextCursor();

    ///* Update the physical cursor */
    //SetConsoleCursorInfo(TextConsoleBuffer, &CursorInfo);
    //SetConsoleCursorPosition(TextConsoleBuffer, Position /*OrgConsoleBufferInfo.dwCursorPosition*/);

    /* Restore the old text-mode screen buffer */
    SetActiveScreenBuffer(TextConsoleBuffer);
}




VOID
VgaConsoleUpdateTextCursor(BOOL CursorVisible,
                           BYTE CursorStart,
                           BYTE CursorEnd,
                           BYTE TextSize,
                           DWORD ScanlineSize,
                           WORD Location)
{
    COORD Position;
    CONSOLE_CURSOR_INFO CursorInfo;

    if (CursorStart < CursorEnd)
    {
        /* Visible cursor */
        CursorInfo.bVisible = CursorVisible;
        CursorInfo.dwSize   = (100 * (CursorEnd - CursorStart)) / TextSize;
    }
    else
    {
        /* Hidden cursor */
        CursorInfo.bVisible = FALSE;
        CursorInfo.dwSize   = 1; // The size needs to be non-zero for SetConsoleCursorInfo to succeed.
    }

    /* Find the coordinates of the new position */
    Position.X = (SHORT)(Location % ScanlineSize);
    Position.Y = (SHORT)(Location / ScanlineSize);

    DPRINT("VgaConsoleUpdateTextCursor: (X = %d ; Y = %d)\n", Position.X, Position.Y);

    /* Update the physical cursor */
    SetConsoleCursorInfo(TextConsoleBuffer, &CursorInfo);
    SetConsoleCursorPosition(TextConsoleBuffer, Position);
}

BOOL
VgaConsoleCreateGraphicsScreen(// OUT PBYTE* GraphicsFramebuffer,
                               IN PCOORD Resolution,
                               IN HANDLE PaletteHandle)
{
    DWORD i;
    CONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo;
    BYTE BitmapInfoBuffer[VGA_BITMAP_INFO_SIZE];
    LPBITMAPINFO BitmapInfo = (LPBITMAPINFO)BitmapInfoBuffer;
    LPWORD PaletteIndex = (LPWORD)(BitmapInfo->bmiColors);

    LONG Width  = Resolution->X;
    LONG Height = Resolution->Y;

    /* Use DoubleVision mode if the resolution is too small */
    DoubleWidth = (Width < VGA_MINIMUM_WIDTH);
    if (DoubleWidth) Width *= 2;
    DoubleHeight = (Height < VGA_MINIMUM_HEIGHT);
    if (DoubleHeight) Height *= 2;

    /* Fill the bitmap info header */
    RtlZeroMemory(&BitmapInfo->bmiHeader, sizeof(BitmapInfo->bmiHeader));
    BitmapInfo->bmiHeader.biSize   = sizeof(BitmapInfo->bmiHeader);
    BitmapInfo->bmiHeader.biWidth  = Width;
    BitmapInfo->bmiHeader.biHeight = Height;
    BitmapInfo->bmiHeader.biBitCount = 8;
    BitmapInfo->bmiHeader.biPlanes   = 1;
    BitmapInfo->bmiHeader.biCompression = BI_RGB;
    BitmapInfo->bmiHeader.biSizeImage   = Width * Height /* * 1 == biBitCount / 8 */;

    /* Fill the palette data */
    for (i = 0; i < (VGA_PALETTE_SIZE / 3); i++) PaletteIndex[i] = (WORD)i;

    /* Fill the console graphics buffer info */
    GraphicsBufferInfo.dwBitMapInfoLength = VGA_BITMAP_INFO_SIZE;
    GraphicsBufferInfo.lpBitMapInfo = BitmapInfo;
    GraphicsBufferInfo.dwUsage = DIB_PAL_COLORS;

    /* Create the buffer */
    GraphicsConsoleBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                      NULL,
                                                      CONSOLE_GRAPHICS_BUFFER,
                                                      &GraphicsBufferInfo);
    if (GraphicsConsoleBuffer == INVALID_HANDLE_VALUE) return FALSE;

    /* Save the framebuffer address and mutex */
    // *GraphicsFramebuffer = GraphicsBufferInfo.lpBitMap;
    GraphicsFramebuffer = GraphicsBufferInfo.lpBitMap;
    ConsoleMutex = GraphicsBufferInfo.hMutex;

    /* Clear the framebuffer */
    // RtlZeroMemory(*GraphicsFramebuffer, BitmapInfo->bmiHeader.biSizeImage);
    RtlZeroMemory(GraphicsFramebuffer, BitmapInfo->bmiHeader.biSizeImage);

    /* Set the graphics mode palette */
    SetConsolePalette(GraphicsConsoleBuffer,
                      PaletteHandle,
                      SYSPAL_NOSTATIC256);

    /* Set the active buffer */
    SetActiveScreenBuffer(GraphicsConsoleBuffer);

    return TRUE;
}

VOID VgaConsoleDestroyGraphicsScreen(VOID)
{
    /* Release the console framebuffer mutex */
    ReleaseMutex(ConsoleMutex);

    /* Switch back to the default console text buffer */
    // SetActiveScreenBuffer(TextConsoleBuffer);

    /* Cleanup the video data */
    CloseHandle(ConsoleMutex);
    ConsoleMutex = NULL;
    // GraphicsFramebuffer = NULL;
    CloseHandle(GraphicsConsoleBuffer);
    GraphicsConsoleBuffer = NULL;

    // /* Reset the active framebuffer */
    // ActiveFramebuffer = NULL;

    DoubleWidth  = FALSE;
    DoubleHeight = FALSE;
}

BOOL
VgaConsoleCreateTextScreen(// OUT PCHAR_CELL* TextFramebuffer,
                           IN PCOORD Resolution,
                           IN HANDLE PaletteHandle)
{
    /* Switch to the text buffer */
    // FIXME: Wouldn't it be preferrable to switch to it AFTER we reset everything??
    SetActiveScreenBuffer(TextConsoleBuffer);

    /* Adjust the text framebuffer if we changed the resolution */
    if (TextResolution.X != Resolution->X ||
        TextResolution.Y != Resolution->Y)
    {
        DetachFromConsoleInternal();

        /*
         * AttachToConsoleInternal sets TextResolution
         * to the new resolution and updates ConsoleInfo.
         */
        if (!AttachToConsoleInternal(Resolution))
        {
            DisplayMessage(L"An unexpected error occurred!\n");
            EmulatorTerminate();
            return FALSE;
        }
    }
    else
    {
        UpdateCursorPosition();
    }

    /*
     * Set the text mode palette.
     *
     * INFORMATION: This call should fail on Windows (and therefore
     * we get the default palette and our external behaviour is
     * just like Windows' one), but it should success on ReactOS
     * (so that we get console palette changes even for text-mode
     * screen buffers, which is a new feature on ReactOS).
     */
    SetConsolePalette(TextConsoleBuffer,
                      PaletteHandle,
                      SYSPAL_NOSTATIC256);

    return TRUE;
}

VOID VgaConsoleDestroyTextScreen(VOID)
{
}



VOID VgaConsoleRepaintScreen(PSMALL_RECT Rect)
{
    HANDLE ConsoleBufferHandle = NULL;
    SMALL_RECT UpdateRectangle = *Rect;

    /* Check if we are in text or graphics mode */
    if (ScreenMode == GRAPHICS_MODE)
    {
        /* Graphics mode */
        ConsoleBufferHandle = GraphicsConsoleBuffer;

        /* In DoubleVision mode, scale the update rectangle */
        if (DoubleWidth)
        {
            UpdateRectangle.Left *= 2;
            UpdateRectangle.Right = UpdateRectangle.Right * 2 + 1;
        }
        if (DoubleHeight)
        {
            UpdateRectangle.Top *= 2;
            UpdateRectangle.Bottom = UpdateRectangle.Bottom * 2 + 1;
        }
    }
    else
    {
        /* Text mode */
        ConsoleBufferHandle = TextConsoleBuffer;
    }

    /* Redraw the screen */
    __InvalidateConsoleDIBits(ConsoleBufferHandle, &UpdateRectangle);
}

BOOLEAN VgaConsoleInitialize(HANDLE TextHandle)
{
    /* Save the default text-mode console output handle */
    if (!IsConsoleHandle(TextHandle)) return FALSE;
    TextConsoleBuffer = TextHandle;

    /* Save the original cursor and console screen buffer information */
    if (!GetConsoleCursorInfo(TextConsoleBuffer, &OrgConsoleCursorInfo) ||
        !GetConsoleScreenBufferInfo(TextConsoleBuffer, &OrgConsoleBufferInfo))
    {
        return FALSE;
    }
    ConsoleInfo = OrgConsoleBufferInfo;

    /* Switch to the text buffer, but do not enter into a text mode */
    SetActiveScreenBuffer(TextConsoleBuffer);

    return TRUE;
}

VOID VgaConsoleCleanup(VOID)
{
    VgaDetachFromConsole();

    CloseHandle(AnotherEvent);
    CloseHandle(EndEvent);
    CloseHandle(StartEvent);
}
