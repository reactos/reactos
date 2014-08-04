/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            condrv/dummyterm.c
 * PURPOSE:         Dummy Terminal used when no terminal
 *                  is attached to the specified console.
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

/* DUMMY TERMINAL INTERFACE ***************************************************/

static NTSTATUS NTAPI
DummyInitTerminal(IN OUT PTERMINAL This,
                  IN PCONSOLE Console)
{
    return STATUS_SUCCESS;
}

static VOID NTAPI
DummyDeinitTerminal(IN OUT PTERMINAL This)
{
}

static VOID NTAPI
DummyDrawRegion(IN OUT PTERMINAL This,
                SMALL_RECT* Region)
{
}

static VOID NTAPI
DummyWriteStream(IN OUT PTERMINAL This,
                 SMALL_RECT* Region,
                 SHORT CursorStartX,
                 SHORT CursorStartY,
                 UINT ScrolledLines,
                 PWCHAR Buffer,
                 UINT Length)
{
}

static BOOL NTAPI
DummySetCursorInfo(IN OUT PTERMINAL This,
                   PCONSOLE_SCREEN_BUFFER ScreenBuffer)
{
    return TRUE;
}

static BOOL NTAPI
DummySetScreenInfo(IN OUT PTERMINAL This,
                   PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                   SHORT OldCursorX,
                   SHORT OldCursorY)
{
    return TRUE;
}

static VOID NTAPI
DummyResizeTerminal(IN OUT PTERMINAL This)
{
}

static VOID NTAPI
DummySetActiveScreenBuffer(IN OUT PTERMINAL This)
{
}

static VOID NTAPI
DummyReleaseScreenBuffer(IN OUT PTERMINAL This,
                         IN PCONSOLE_SCREEN_BUFFER ScreenBuffer)
{
}

static VOID NTAPI
DummyChangeTitle(IN OUT PTERMINAL This)
{
}

static VOID NTAPI
DummyGetLargestConsoleWindowSize(IN OUT PTERMINAL This,
                                 PCOORD pSize)
{
}

/*
static BOOL NTAPI
DummyGetSelectionInfo(IN OUT PTERMINAL This,
                      PCONSOLE_SELECTION_INFO pSelectionInfo)
{
    return TRUE;
}
*/

static BOOL NTAPI
DummySetPalette(IN OUT PTERMINAL This,
                HPALETTE PaletteHandle,
                UINT PaletteUsage)
{
    return TRUE;
}

static INT NTAPI
DummyShowMouseCursor(IN OUT PTERMINAL This,
                     BOOL Show)
{
    return 0;
}

static TERMINAL_VTBL DummyVtbl =
{
    DummyInitTerminal,
    DummyDeinitTerminal,
    DummyDrawRegion,
    DummyWriteStream,
    DummySetCursorInfo,
    DummySetScreenInfo,
    DummyResizeTerminal,
    DummySetActiveScreenBuffer,
    DummyReleaseScreenBuffer,
    DummyChangeTitle,
    DummyGetLargestConsoleWindowSize,
    // DummyGetSelectionInfo,
    DummySetPalette,
    DummyShowMouseCursor,
};

VOID
ResetTerminal(IN PCONSOLE Console)
{
    if (!Console) return;

    /* Reinitialize the terminal interface */
    RtlZeroMemory(&Console->TermIFace, sizeof(Console->TermIFace));
    Console->TermIFace.Vtbl = &DummyVtbl;
}

/* EOF */
