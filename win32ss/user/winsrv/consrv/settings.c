/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/settings.c
 * PURPOSE:         Console settings management
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

extern const COLORREF s_Colors[16];


/* FUNCTIONS ******************************************************************/

NTSTATUS NTAPI
ConDrvChangeScreenBufferAttributes(IN PCONSOLE Console,
                                   IN PTEXTMODE_SCREEN_BUFFER Buffer,
                                   IN USHORT NewScreenAttrib,
                                   IN USHORT NewPopupAttrib);
/*
 * NOTE: This function explicitely references Console->ActiveBuffer.
 * It is possible that it should go into some frontend...
 */
VOID
ConSrvApplyUserSettings(IN PCONSOLE Console,
                        IN PCONSOLE_STATE_INFO ConsoleInfo)
{
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = Console->ActiveBuffer;

    /*
     * Apply terminal-edition settings:
     * - QuickEdit and Insert modes,
     * - history settings.
     */
    Console->QuickEdit  = !!ConsoleInfo->QuickEdit;
    Console->InsertMode = !!ConsoleInfo->InsertMode;

    /* Copy the new console palette */
    // FIXME: Possible buffer overflow if s_colors is bigger than ConsoleInfo->ColorTable.
    RtlCopyMemory(Console->Colors, ConsoleInfo->ColorTable, sizeof(s_Colors));

    /* Apply cursor size */
    ActiveBuffer->CursorInfo.bVisible = (ConsoleInfo->CursorSize != 0);
    ActiveBuffer->CursorInfo.dwSize   = min(max(ConsoleInfo->CursorSize, 0), 100);

    /* Update the code page */
    if ((Console->OutputCodePage != ConsoleInfo->CodePage) &&
        IsValidCodePage(ConsoleInfo->CodePage))
    {
        Console->InputCodePage = Console->OutputCodePage = ConsoleInfo->CodePage;
        // ConDrvSetConsoleCP(Console, ConsoleInfo->CodePage, TRUE);    // Output
        // ConDrvSetConsoleCP(Console, ConsoleInfo->CodePage, FALSE);   // Input
    }

    // FIXME: Check ConsoleInfo->WindowSize with respect to
    // TermGetLargestConsoleWindowSize(...).

    if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
    {
        PTEXTMODE_SCREEN_BUFFER Buffer = (PTEXTMODE_SCREEN_BUFFER)ActiveBuffer;
        COORD BufSize;

        /* Resize its active screen-buffer */
        BufSize = ConsoleInfo->ScreenBufferSize;

        if (Console->FixedSize)
        {
            /*
             * The console is in fixed-size mode, so we cannot resize anything
             * at the moment. However, keep those settings somewhere so that
             * we can try to set them up when we will be allowed to do so.
             */
            if (ConsoleInfo->WindowSize.X != Buffer->OldViewSize.X ||
                ConsoleInfo->WindowSize.Y != Buffer->OldViewSize.Y)
            {
                Buffer->OldViewSize = ConsoleInfo->WindowSize;
            }

            /* Buffer size is not allowed to be smaller than the view size */
            if (BufSize.X >= Buffer->OldViewSize.X && BufSize.Y >= Buffer->OldViewSize.Y)
            {
                if (BufSize.X != Buffer->OldScreenBufferSize.X ||
                    BufSize.Y != Buffer->OldScreenBufferSize.Y)
                {
                    /*
                     * The console is in fixed-size mode, so we cannot resize anything
                     * at the moment. However, keep those settings somewhere so that
                     * we can try to set them up when we will be allowed to do so.
                     */
                    Buffer->OldScreenBufferSize = BufSize;
                }
            }
        }
        else
        {
            BOOL SizeChanged = FALSE;

            /* Resize the console */
            if (ConsoleInfo->WindowSize.X != Buffer->ViewSize.X ||
                ConsoleInfo->WindowSize.Y != Buffer->ViewSize.Y)
            {
                Buffer->ViewSize = ConsoleInfo->WindowSize;
                SizeChanged = TRUE;
            }

            /* Resize the screen-buffer */
            if (BufSize.X != Buffer->ScreenBufferSize.X ||
                BufSize.Y != Buffer->ScreenBufferSize.Y)
            {
                if (NT_SUCCESS(ConioResizeBuffer(Console, Buffer, BufSize)))
                    SizeChanged = TRUE;
            }

            if (SizeChanged) TermResizeTerminal(Console);
        }

        /* Apply foreground and background colors for both screen and popup */
        ConDrvChangeScreenBufferAttributes(Console,
                                           Buffer,
                                           ConsoleInfo->ScreenAttributes,
                                           ConsoleInfo->PopupAttributes);
    }
    else // if (GetType(ActiveBuffer) == GRAPHICS_BUFFER)
    {
        PGRAPHICS_SCREEN_BUFFER Buffer = (PGRAPHICS_SCREEN_BUFFER)ActiveBuffer;

        /*
         * In any case we do NOT modify the size of the graphics screen-buffer.
         * We just allow resizing the view only if the new size is smaller
         * than the older one.
         */

        if (Console->FixedSize)
        {
            /*
             * The console is in fixed-size mode, so we cannot resize anything
             * at the moment. However, keep those settings somewhere so that
             * we can try to set them up when we will be allowed to do so.
             */
            if (ConsoleInfo->WindowSize.X <= Buffer->ViewSize.X ||
                ConsoleInfo->WindowSize.Y <= Buffer->ViewSize.Y)
            {
                Buffer->OldViewSize = ConsoleInfo->WindowSize;
            }
        }
        else
        {
            /* Resize the view if its size is bigger than the specified size */
            if (ConsoleInfo->WindowSize.X <= Buffer->ViewSize.X ||
                ConsoleInfo->WindowSize.Y <= Buffer->ViewSize.Y)
            {
                Buffer->ViewSize = ConsoleInfo->WindowSize;
                // SizeChanged = TRUE;
            }
        }
    }
}

/* EOF */
