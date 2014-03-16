/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/graphics.c
 * PURPOSE:         GUI Terminal Front-End - Support for graphics-mode screen-buffers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
GuiCopyFromGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                          PGUI_CONSOLE_DATA GuiData)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    PCONSOLE Console = Buffer->Header.Console;

    HDC hMemDC;
    HBITMAP  hBitmapTarget, hBitmapOld;
    HPALETTE hPalette, hPaletteOld;
    ULONG selWidth, selHeight;

    if (Buffer->BitMap == NULL) return;

    selWidth  = Console->Selection.srSelection.Right - Console->Selection.srSelection.Left + 1;
    selHeight = Console->Selection.srSelection.Bottom - Console->Selection.srSelection.Top + 1;
    DPRINT1("Selection is (%d|%d) to (%d|%d)\n",
           Console->Selection.srSelection.Left,
           Console->Selection.srSelection.Top,
           Console->Selection.srSelection.Right,
           Console->Selection.srSelection.Bottom);

    hMemDC = CreateCompatibleDC(GuiData->hMemDC);
    if (hMemDC == NULL) return;

    /* Allocate a bitmap to be given to the clipboard, so it will not be freed here */
    hBitmapTarget = CreateCompatibleBitmap(GuiData->hMemDC, selWidth, selHeight);
    if (hBitmapTarget == NULL)
    {
        DeleteDC(hMemDC);
        return;
    }

    /* Select the new bitmap */
    hBitmapOld = SelectObject(hMemDC, hBitmapTarget);

    /* Change the palette in hMemDC if the current palette does exist */
    if (Buffer->PaletteHandle == NULL)
        hPalette = GuiData->hSysPalette;
    else
        hPalette = Buffer->PaletteHandle;

    if (hPalette) hPaletteOld = SelectPalette(hMemDC, hPalette, FALSE);

    /* Grab the mutex */
    NtWaitForSingleObject(Buffer->Mutex, FALSE, NULL);

    // The equivalent of a SetDIBitsToDevice call...
    // It seems to be broken: it does not copy the tail of the bitmap.
    // http://wiki.allegro.cc/index.php?title=StretchDIBits
#if 0
    StretchDIBits(hMemDC,
                  0, 0,
                  selWidth, selHeight,
                  Console->Selection.srSelection.Left,
                  Console->Selection.srSelection.Top,
                  selWidth, selHeight,
                  Buffer->BitMap,
                  Buffer->BitMapInfo,
                  Buffer->BitMapUsage,
                  SRCCOPY);
#else
    SetDIBitsToDevice(hMemDC,
                      /* Coordinates / size of the repainted rectangle, in the framebuffer's frame */
                      0, 0,
                      selWidth, selHeight,
                      /* Coordinates / size of the corresponding image portion, in the graphics screen-buffer's frame */
                      Console->Selection.srSelection.Left,
                      Console->Selection.srSelection.Top,
                      0,
                      Buffer->ScreenBufferSize.Y, // == Buffer->BitMapInfo->bmiHeader.biHeight
                      Buffer->BitMap,
                      Buffer->BitMapInfo,
                      Buffer->BitMapUsage);
#endif

    /* Release the mutex */
    NtReleaseMutant(Buffer->Mutex, NULL);

    /* Restore the palette and the old bitmap */
    if (hPalette) SelectPalette(hMemDC, hPaletteOld, FALSE);
    SelectObject(hMemDC, hBitmapOld);

    EmptyClipboard();
    SetClipboardData(CF_BITMAP, hBitmapTarget);

    DeleteDC(hMemDC);
}

VOID
GuiPasteToGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                         PGUI_CONSOLE_DATA GuiData)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    // PCONSOLE Console = Buffer->Header.Console;

    UNIMPLEMENTED;
}

VOID
GuiPaintGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       PRECT rcView,
                       PRECT rcFramebuffer)
{
    if (Buffer->BitMap == NULL) return;

    rcFramebuffer->left   = Buffer->ViewOrigin.X * 1 + rcView->left;
    rcFramebuffer->top    = Buffer->ViewOrigin.Y * 1 + rcView->top;
    rcFramebuffer->right  = Buffer->ViewOrigin.X * 1 + rcView->right;
    rcFramebuffer->bottom = Buffer->ViewOrigin.Y * 1 + rcView->bottom;

    /* Grab the mutex */
    NtWaitForSingleObject(Buffer->Mutex, FALSE, NULL);

    /*
     * The seventh parameter (YSrc) of SetDIBitsToDevice always designates
     * the Y-coordinate of the "lower-left corner" of the image, be the DIB
     * in bottom-up or top-down mode.
     */
    SetDIBitsToDevice(GuiData->hMemDC,
                      /* Coordinates / size of the repainted rectangle, in the framebuffer's frame */
                      rcFramebuffer->left,
                      rcFramebuffer->top,
                      rcFramebuffer->right  - rcFramebuffer->left,
                      rcFramebuffer->bottom - rcFramebuffer->top,
                      /* Coordinates / size of the corresponding image portion, in the graphics screen-buffer's frame */
                      rcFramebuffer->left,
                      rcFramebuffer->top,
                      0,
                      Buffer->ScreenBufferSize.Y, // == Buffer->BitMapInfo->bmiHeader.biHeight
                      Buffer->BitMap,
                      Buffer->BitMapInfo,
                      Buffer->BitMapUsage);

    /* Release the mutex */
    NtReleaseMutant(Buffer->Mutex, NULL);
}

/* EOF */
