/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            frontends/terminal.c
 * PURPOSE:         ConSrv terminal.
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#ifdef TUITERM_COMPILE
#include "frontends/tui/tuiterm.h"
#endif

#define NDEBUG
#include <debug.h>

/* CONSRV TERMINAL FRONTENDS INTERFACE ****************************************/

/***************/
#ifdef TUITERM_COMPILE
NTSTATUS NTAPI
TuiLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                IN OUT PCONSOLE_INFO ConsoleInfo,
                IN OUT PVOID ExtraConsoleInfo,
                IN ULONG ProcessId);
NTSTATUS NTAPI
TuiUnloadFrontEnd(IN OUT PFRONTEND FrontEnd);
#endif

NTSTATUS NTAPI
GuiLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                IN OUT PCONSOLE_INFO ConsoleInfo,
                IN OUT PVOID ExtraConsoleInfo,
                IN ULONG ProcessId);
NTSTATUS NTAPI
GuiUnloadFrontEnd(IN OUT PFRONTEND FrontEnd);
/***************/

typedef
NTSTATUS (NTAPI *FRONTEND_LOAD)(IN OUT PFRONTEND FrontEnd,
                                IN OUT PCONSOLE_INFO ConsoleInfo,
                                IN OUT PVOID ExtraConsoleInfo,
                                IN ULONG ProcessId);

typedef
NTSTATUS (NTAPI *FRONTEND_UNLOAD)(IN OUT PFRONTEND FrontEnd);

/*
 * If we are not in GUI-mode, start the text-mode terminal emulator.
 * If we fail, try to start the GUI-mode terminal emulator.
 *
 * Try to open the GUI-mode terminal emulator. Two cases are possible:
 * - We are in GUI-mode, therefore GuiMode == TRUE, the previous test-case
 *   failed and we start GUI-mode terminal emulator.
 * - We are in text-mode, therefore GuiMode == FALSE, the previous test-case
 *   succeeded BUT we failed at starting text-mode terminal emulator.
 *   Then GuiMode was switched to TRUE in order to try to open the GUI-mode
 *   terminal emulator (Win32k will automatically switch to graphical mode,
 *   therefore no additional code is needed).
 */

/*
 * NOTE: Each entry of the table should be retrieved when loading a front-end
 *       (examples of the CSR servers which register some data for CSRSS).
 */
struct
{
    CHAR            FrontEndName[80];
    FRONTEND_LOAD   FrontEndLoad;
    FRONTEND_UNLOAD FrontEndUnload;
} FrontEndLoadingMethods[] =
{
#ifdef TUITERM_COMPILE
    {"TUI", TuiLoadFrontEnd,    TuiUnloadFrontEnd},
#endif
    {"GUI", GuiLoadFrontEnd,    GuiUnloadFrontEnd},

//  {"Not found", 0, NULL}
};


/* static */ NTSTATUS
ConSrvLoadFrontEnd(IN OUT PFRONTEND FrontEnd,
                   IN OUT PCONSOLE_INFO ConsoleInfo,
                   IN OUT PVOID ExtraConsoleInfo,
                   IN ULONG ProcessId)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    /*
     * Choose an adequate terminal front-end to load, and load it
     */
    for (i = 0; i < sizeof(FrontEndLoadingMethods) / sizeof(FrontEndLoadingMethods[0]); ++i)
    {
        DPRINT("CONSRV: Trying to load %s frontend...\n",
               FrontEndLoadingMethods[i].FrontEndName);
        Status = FrontEndLoadingMethods[i].FrontEndLoad(FrontEnd,
                                                        ConsoleInfo,
                                                        ExtraConsoleInfo,
                                                        ProcessId);
        if (NT_SUCCESS(Status))
        {
            /* Save the unload callback */
            FrontEnd->UnloadFrontEnd = FrontEndLoadingMethods[i].FrontEndUnload;

            DPRINT("CONSRV: %s frontend loaded successfully\n",
                   FrontEndLoadingMethods[i].FrontEndName);
            break;
        }
        else
        {
            DPRINT1("CONSRV: Loading %s frontend failed, Status = 0x%08lx , continuing...\n",
                    FrontEndLoadingMethods[i].FrontEndName, Status);
        }
    }

    return Status;
}

/* static */ NTSTATUS
ConSrvUnloadFrontEnd(IN PFRONTEND FrontEnd)
{
    if (FrontEnd == NULL) return STATUS_INVALID_PARAMETER;
    // return FrontEnd->Vtbl->UnloadFrontEnd(FrontEnd);
    return FrontEnd->UnloadFrontEnd(FrontEnd);
}


/* DUMMY FRONTEND INTERFACE ***************************************************/

#if 0

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

static VOID NTAPI
DummySetActiveScreenBuffer(IN OUT PFRONTEND This)
{
}

static VOID NTAPI
DummyReleaseScreenBuffer(IN OUT PFRONTEND This,
                         IN PCONSOLE_SCREEN_BUFFER ScreenBuffer)
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
                HICON IconHandle)
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

static BOOL NTAPI
DummyGetSelectionInfo(IN OUT PFRONTEND This,
                      PCONSOLE_SELECTION_INFO pSelectionInfo)
{
    return TRUE;
}

static BOOL NTAPI
DummySetPalette(IN OUT PFRONTEND This,
                HPALETTE PaletteHandle,
                UINT PaletteUsage)
{
    return TRUE;
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
                    HCURSOR CursorHandle)
{
    return TRUE;
}

static HMENU NTAPI
DummyMenuControl(IN OUT PFRONTEND This,
                 UINT CmdIdLow,
                 UINT CmdIdHigh)
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
    DummySetActiveScreenBuffer,
    DummyReleaseScreenBuffer,
    DummyProcessKeyCallback,
    DummyRefreshInternalInfo,
    DummyChangeTitle,
    DummyChangeIcon,
    DummyGetConsoleWindowHandle,
    DummyGetLargestConsoleWindowSize,
    DummyGetSelectionInfo,
    DummySetPalette,
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
    RtlZeroMemory(&Console->FrontEndIFace, sizeof(Console->FrontEndIFace));
    Console->FrontEndIFace.Vtbl = &DummyVtbl;
}

#endif

/* EOF */
