/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/graphics.c
 * PURPOSE:         GUI Terminal Front-End - Support for graphics-mode screen-buffers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/settings.h"
#include "guisettings.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

VOID
GuiCopyFromGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    // PCONSOLE Console = Buffer->Header.Console;

    UNIMPLEMENTED;
}

VOID
GuiPasteToGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer)
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
                       HDC hDC,
                       PRECT rc)
{
    if (Buffer->BitMap == NULL) return;

    /* Grab the mutex */
    NtWaitForSingleObject(Buffer->Mutex, FALSE, NULL);

    /*
     * The seventh parameter (YSrc) of SetDIBitsToDevice always designates
     * the Y-coordinate of the "lower-left corner" of the image, be the DIB
     * in bottom-up or top-down mode.
     */
    SetDIBitsToDevice(hDC,
                      /* Coordinates / size of the repainted rectangle, in the view's frame */
                      rc->left,
                      rc->top,
                      rc->right  - rc->left,
                      rc->bottom - rc->top,
                      /* Coordinates / size of the corresponding image portion, in the graphics screen-buffer's frame */
                      Buffer->ViewOrigin.X + rc->left,
                      Buffer->ViewOrigin.Y + rc->top,
                      0,
                      Buffer->ScreenBufferSize.Y, // == Buffer->BitMapInfo->bmiHeader.biHeight
                      Buffer->BitMap,
                      Buffer->BitMapInfo,
                      Buffer->BitMapUsage);

    /* Release the mutex */
    NtReleaseMutant(Buffer->Mutex, NULL);
}

/* EOF */
