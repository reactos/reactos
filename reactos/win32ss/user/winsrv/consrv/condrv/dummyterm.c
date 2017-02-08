/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/condrv/dummyterm.c
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



/************ Line discipline ***************/

static NTSTATUS NTAPI
DummyReadStream(IN OUT PTERMINAL This,
                IN BOOLEAN Unicode,
                /**PWCHAR Buffer,**/
                OUT PVOID Buffer,
                IN OUT PCONSOLE_READCONSOLE_CONTROL ReadControl,
                IN PVOID Parameter OPTIONAL,
                IN ULONG NumCharsToRead,
                OUT PULONG NumCharsRead OPTIONAL)
{
    /*
     * We were called because the console was in cooked mode.
     * There is nothing to read, wait until a real terminal
     * is plugged into the console.
     */
    return STATUS_PENDING;
}

static NTSTATUS NTAPI
DummyWriteStream(IN OUT PTERMINAL This,
                 PTEXTMODE_SCREEN_BUFFER Buff,
                 PWCHAR Buffer,
                 DWORD Length,
                 BOOL Attrib)
{
    /*
     * We were called because the console was in cooked mode.
     * There is nothing to write, wait until a real terminal
     * is plugged into the console.
     */

    // /* Stop here if the console is paused */
    // if (Console->UnpauseEvent != NULL) return STATUS_PENDING;

    return STATUS_PENDING;
}

/************ Line discipline ***************/



static VOID NTAPI
DummyDrawRegion(IN OUT PTERMINAL This,
                SMALL_RECT* Region)
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
DummyGetLargestConsoleWindowSize(IN OUT PTERMINAL This,
                                 PCOORD pSize)
{
}

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

    DummyReadStream,
    DummyWriteStream,

    DummyDrawRegion,
    DummySetCursorInfo,
    DummySetScreenInfo,
    DummyResizeTerminal,
    DummySetActiveScreenBuffer,
    DummyReleaseScreenBuffer,
    DummyGetLargestConsoleWindowSize,
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
