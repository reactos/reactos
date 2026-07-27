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
    // if (Console->ConsolePaused) return STATUS_PENDING;

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
    /* Return a standard size */
    if (!pSize) return;
    pSize->X = 80;
    pSize->Y = 25;
}

static BOOL NTAPI
DummySetPalette(IN OUT PTERMINAL This,
                HPALETTE PaletteHandle,
                UINT PaletteUsage)
{
    return TRUE;
}

static BOOL NTAPI
DummySetCodePage(IN OUT PTERMINAL This,
                 UINT CodePage)
{
    return TRUE;
}

static INT NTAPI
DummyShowMouseCursor(IN OUT PTERMINAL This,
                     BOOL Show)
{
    return 0;
}

static BOOL NTAPI
DummySetTitle(IN OUT PTERMINAL This,
              IN PCWSTR Title,
              IN ULONG Length)
{
    return TRUE;
}

static BOOL NTAPI
DummyGetColorTable(IN OUT PTERMINAL This,
                   OUT COLORREF* Colors,
                   IN ULONG Count)
{
    return FALSE;
}

static BOOL NTAPI
DummySetColorTable(IN OUT PTERMINAL This,
                   IN const COLORREF* Colors,
                   IN ULONG Count)
{
    return FALSE;
}

/* No frontend attached, so there is no window station to reach the clipboard through */
static BOOL NTAPI
DummyGetClipboardText(IN OUT PTERMINAL This,
                      OUT PWCHAR* Text,
                      OUT PULONG Length)
{
    return FALSE;
}

static BOOL NTAPI
DummySetClipboardText(IN OUT PTERMINAL This,
                      IN PCWSTR Text,
                      IN ULONG Length)
{
    return FALSE;
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
    DummySetCodePage,
    DummyShowMouseCursor,
    DummySetTitle,
    DummyGetColorTable,
    DummySetColorTable,
    DummyGetClipboardText,
    DummySetClipboardText,
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
