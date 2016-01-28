/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/condrv/dummyfrontend.c
 * PURPOSE:         Dummy Terminal Front-End used when no frontend
 *                  is attached to the specified console.
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"


/* DUMMY FRONTEND INTERFACE ***************************************************/

static NTSTATUS NTAPI
DummyInitFrontEnd(IN OUT PFRONTEND This,
                  IN PCONSOLE Console)
{
    /* Load some settings ?? */
    return STATUS_SUCCESS;
}

static VOID NTAPI
DummyDeinitFrontEnd(IN OUT PFRONTEND This)
{
    /* Free some settings ?? */
}

static VOID NTAPI
DummyDrawRegion(IN OUT PFRONTEND This,
                SMALL_RECT* Region)
{
}

static VOID NTAPI
DummyWriteStream(IN OUT PFRONTEND This,
                 SMALL_RECT* Region,
                 SHORT CursorStartX,
                 SHORT CursorStartY,
                 UINT ScrolledLines,
                 PWCHAR Buffer,
                 UINT Length)
{
}

static BOOL NTAPI
DummySetCursorInfo(IN OUT PFRONTEND This,
                   PCONSOLE_SCREEN_BUFFER Buff)
{
    return TRUE;
}

static BOOL NTAPI
DummySetScreenInfo(IN OUT PFRONTEND This,
                   PCONSOLE_SCREEN_BUFFER Buff,
                   SHORT OldCursorX,
                   SHORT OldCursorY)
{
    return TRUE;
}

static VOID NTAPI
DummyResizeTerminal(IN OUT PFRONTEND This)
{
}

static BOOL NTAPI
DummyProcessKeyCallback(IN OUT PFRONTEND This,
                        MSG* msg,
                        BYTE KeyStateMenu,
                        DWORD ShiftState,
                        UINT VirtualKeyCode,
                        BOOL Down)
{
    return FALSE;
}

static VOID NTAPI
DummyRefreshInternalInfo(IN OUT PFRONTEND This)
{
}

static VOID NTAPI
DummyChangeTitle(IN OUT PFRONTEND This)
{
}

static BOOL NTAPI
DummyChangeIcon(IN OUT PFRONTEND This,
                HICON hWindowIcon)
{
    return TRUE;
}

static HWND NTAPI
DummyGetConsoleWindowHandle(IN OUT PFRONTEND This)
{
    return NULL;
}

static VOID NTAPI
DummyGetLargestConsoleWindowSize(IN OUT PFRONTEND This,
                                 PCOORD pSize)
{
}

static ULONG NTAPI
DummyGetDisplayMode(IN OUT PFRONTEND This)
{
    return 0;
}

static BOOL NTAPI
DummySetDisplayMode(IN OUT PFRONTEND This,
                    ULONG NewMode)
{
    return TRUE;
}

static INT NTAPI
DummyShowMouseCursor(IN OUT PFRONTEND This,
                     BOOL Show)
{
    return 0;
}

static BOOL NTAPI
DummySetMouseCursor(IN OUT PFRONTEND This,
                    HCURSOR hCursor)
{
    return TRUE;
}

static HMENU NTAPI
DummyMenuControl(IN OUT PFRONTEND This,
                 UINT cmdIdLow,
                 UINT cmdIdHigh)
{
    return NULL;
}

static BOOL NTAPI
DummySetMenuClose(IN OUT PFRONTEND This,
                  BOOL Enable)
{
    return TRUE;
}

static FRONTEND_VTBL DummyVtbl =
{
    DummyInitFrontEnd,
    DummyDeinitFrontEnd,
    DummyDrawRegion,
    DummyWriteStream,
    DummySetCursorInfo,
    DummySetScreenInfo,
    DummyResizeTerminal,
    DummyProcessKeyCallback,
    DummyRefreshInternalInfo,
    DummyChangeTitle,
    DummyChangeIcon,
    DummyGetConsoleWindowHandle,
    DummyGetLargestConsoleWindowSize,
    DummyGetDisplayMode,
    DummySetDisplayMode,
    DummyShowMouseCursor,
    DummySetMouseCursor,
    DummyMenuControl,
    DummySetMenuClose,
};

VOID
ResetFrontEnd(IN PCONSOLE Console)
{
    if (!Console) return;

    /* Reinitialize the frontend interface */
    RtlZeroMemory(&Console->TermIFace, sizeof(Console->TermIFace));
    Console->TermIFace.Vtbl = &DummyVtbl;
}

/* EOF */
