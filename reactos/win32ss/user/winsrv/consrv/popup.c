/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/popup.c
 * PURPOSE:         Console popup windows
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE:            Strongly inspired by the DrawBox function
 *                  from base/setup/usetup/interface/usetup.c, written by:
 *                  Eric Kohl (revision 3753)
 *                  Hervé Poussineau (revision 24718)
 *                  and *UiDisplayMenu from FreeLdr.
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "popup.h"

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS NTAPI
ConDrvFillConsoleOutput(IN PCONSOLE Console,
                        IN PTEXTMODE_SCREEN_BUFFER Buffer,
                        IN CODE_TYPE CodeType,
                        IN CODE_ELEMENT Code,
                        IN ULONG NumCodesToWrite,
                        IN PCOORD WriteCoord,
                        OUT PULONG NumCodesWritten OPTIONAL);
NTSTATUS NTAPI
ConDrvReadConsoleOutput(IN PCONSOLE Console,
                        IN PTEXTMODE_SCREEN_BUFFER Buffer,
                        IN BOOLEAN Unicode,
                        OUT PCHAR_INFO CharInfo/*Buffer*/,
                        IN OUT PSMALL_RECT ReadRegion);
NTSTATUS NTAPI
ConDrvWriteConsoleOutput(IN PCONSOLE Console,
                         IN PTEXTMODE_SCREEN_BUFFER Buffer,
                         IN BOOLEAN Unicode,
                         IN PCHAR_INFO CharInfo/*Buffer*/,
                         IN OUT PSMALL_RECT WriteRegion);


static VOID
DrawBox(PTEXTMODE_SCREEN_BUFFER Buffer,
        IN SHORT xLeft,
        IN SHORT yTop,
        IN SHORT Width,
        IN SHORT Height)
{
    COORD coPos;
    DWORD Written;
    CODE_ELEMENT Code;

    /* Set screen attributes */
    coPos.X = xLeft;
    for (coPos.Y = yTop; coPos.Y < yTop + Height; coPos.Y++)
    {
        Code.Attribute = Buffer->PopupDefaultAttrib;
        ConDrvFillConsoleOutput(Buffer->Header.Console,
                                Buffer,
                                CODE_ATTRIBUTE,
                                Code,
                                Width,
                                &coPos,
                                &Written);
    }

    /* draw upper left corner */
    coPos.X = xLeft;
    coPos.Y = yTop;
    Code.AsciiChar = 0xDA; // '+'
    ConDrvFillConsoleOutput(Buffer->Header.Console,
                            Buffer,
                            CODE_ASCII,
                            Code,
                            1,
                            &coPos,
                            &Written);

    /* draw upper edge */
    coPos.X = xLeft + 1;
    coPos.Y = yTop;
    Code.AsciiChar = 0xC4; // '-'
    ConDrvFillConsoleOutput(Buffer->Header.Console,
                            Buffer,
                            CODE_ASCII,
                            Code,
                            Width - 2,
                            &coPos,
                            &Written);

    /* draw upper right corner */
    coPos.X = xLeft + Width - 1;
    coPos.Y = yTop;
    Code.AsciiChar = 0xBF; // '+'
    ConDrvFillConsoleOutput(Buffer->Header.Console,
                            Buffer,
                            CODE_ASCII,
                            Code,
                            1,
                            &coPos,
                            &Written);

    /* Draw right edge, inner space and left edge */
    for (coPos.Y = yTop + 1; coPos.Y < yTop + Height - 1; coPos.Y++)
    {
        coPos.X = xLeft;
        Code.AsciiChar = 0xB3; // '|'
        ConDrvFillConsoleOutput(Buffer->Header.Console,
                                Buffer,
                                CODE_ASCII,
                                Code,
                                1,
                                &coPos,
                                &Written);

        coPos.X = xLeft + 1;
        Code.AsciiChar = ' ';
        ConDrvFillConsoleOutput(Buffer->Header.Console,
                                Buffer,
                                CODE_ASCII,
                                Code,
                                Width - 2,
                                &coPos,
                                &Written);

        coPos.X = xLeft + Width - 1;
        Code.AsciiChar = 0xB3; // '|'
        ConDrvFillConsoleOutput(Buffer->Header.Console,
                                Buffer,
                                CODE_ASCII,
                                Code,
                                1,
                                &coPos,
                                &Written);
    }

    /* draw lower left corner */
    coPos.X = xLeft;
    coPos.Y = yTop + Height - 1;
    Code.AsciiChar = 0xC0; // '+'
    ConDrvFillConsoleOutput(Buffer->Header.Console,
                            Buffer,
                            CODE_ASCII,
                            Code,
                            1,
                            &coPos,
                            &Written);

    /* draw lower edge */
    coPos.X = xLeft + 1;
    coPos.Y = yTop + Height - 1;
    Code.AsciiChar = 0xC4; // '-'
    ConDrvFillConsoleOutput(Buffer->Header.Console,
                            Buffer,
                            CODE_ASCII,
                            Code,
                            Width - 2,
                            &coPos,
                            &Written);

    /* draw lower right corner */
    coPos.X = xLeft + Width - 1;
    coPos.Y = yTop + Height - 1;
    Code.AsciiChar = 0xD9; // '+'
    ConDrvFillConsoleOutput(Buffer->Header.Console,
                            Buffer,
                            CODE_ASCII,
                            Code,
                            1,
                            &coPos,
                            &Written);
}


/* PUBLIC FUNCTIONS ***********************************************************/

PPOPUP_WINDOW
CreatePopupWindow(PCONSRV_CONSOLE Console,
                  PTEXTMODE_SCREEN_BUFFER Buffer,
                  SHORT xLeft,
                  SHORT yTop,
                  SHORT Width,
                  SHORT Height)
{
    PPOPUP_WINDOW Popup;
    SMALL_RECT Region;

    ASSERT((PCONSOLE)Console == Buffer->Header.Console);

    /* Create the popup window */
    Popup = ConsoleAllocHeap(HEAP_ZERO_MEMORY, sizeof(*Popup));
    if (Popup == NULL) return NULL;

    Popup->ScreenBuffer = Buffer;
    Popup->Origin.X = xLeft;
    Popup->Origin.Y = yTop;
    Popup->Size.X = Width;
    Popup->Size.Y = Height;

    /* Save old contents */
    Popup->OldContents = ConsoleAllocHeap(HEAP_ZERO_MEMORY,
                                          Popup->Size.X * Popup->Size.Y *
                                            sizeof(*Popup->OldContents));
    if (Popup->OldContents == NULL)
    {
        ConsoleFreeHeap(Popup);
        return NULL;
    }
    Region.Left   = Popup->Origin.X;
    Region.Top    = Popup->Origin.Y;
    Region.Right  = Popup->Origin.X + Popup->Size.X - 1;
    Region.Bottom = Popup->Origin.Y + Popup->Size.Y - 1;
    ConDrvReadConsoleOutput(Buffer->Header.Console,
                            Buffer,
                            TRUE,
                            Popup->OldContents,
                            &Region);

    /* Draw it */
    DrawBox(Buffer,
            xLeft, yTop,
            Width, Height);

    /* Add it into the list of popups */
    InsertTailList(&Console->PopupWindows, &Popup->ListEntry);

    return Popup;
}

VOID
DestroyPopupWindow(PPOPUP_WINDOW Popup)
{
    SMALL_RECT Region;

    if (Popup == NULL) return;

    /* Remove it from the list of popups */
    RemoveEntryList(&Popup->ListEntry);

    /* Restore the old screen-buffer contents */
    Region.Left   = Popup->Origin.X;
    Region.Top    = Popup->Origin.Y;
    Region.Right  = Popup->Origin.X + Popup->Size.X - 1;
    Region.Bottom = Popup->Origin.Y + Popup->Size.Y - 1;
    ConDrvWriteConsoleOutput(Popup->ScreenBuffer->Header.Console,
                             Popup->ScreenBuffer,
                             TRUE,
                             Popup->OldContents,
                             &Region);

    /* Free memory */
    ConsoleFreeHeap(Popup->OldContents);
    ConsoleFreeHeap(Popup);
}

/* EOF */
