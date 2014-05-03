/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            frontends/terminal.c
 * PURPOSE:         ConSrv terminal.
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

// #include "frontends/gui/guiterm.h"
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

static NTSTATUS
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

static NTSTATUS
ConSrvUnloadFrontEnd(IN PFRONTEND FrontEnd)
{
    if (FrontEnd == NULL) return STATUS_INVALID_PARAMETER;
    // return FrontEnd->Vtbl->UnloadFrontEnd(FrontEnd);
    return FrontEnd->UnloadFrontEnd(FrontEnd);
}

// See after...
static TERMINAL_VTBL ConSrvTermVtbl;

NTSTATUS NTAPI
ConSrvInitTerminal(IN OUT PTERMINAL Terminal,
                   IN OUT PCONSOLE_INFO ConsoleInfo,
                   IN OUT PVOID ExtraConsoleInfo,
                   IN ULONG ProcessId)
{
    NTSTATUS Status;
    PFRONTEND FrontEnd;

    /* Load a suitable frontend for the ConSrv terminal */
    FrontEnd = ConsoleAllocHeap(HEAP_ZERO_MEMORY, sizeof(*FrontEnd));
    if (!FrontEnd) return STATUS_NO_MEMORY;

    Status = ConSrvLoadFrontEnd(FrontEnd,
                                ConsoleInfo,
                                ExtraConsoleInfo,
                                ProcessId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CONSRV: Failed to initialize a frontend, Status = 0x%08lx\n", Status);
        ConsoleFreeHeap(FrontEnd);
        return Status;
    }
    DPRINT("CONSRV: Frontend initialized\n");

    /* Initialize the ConSrv terminal */
    Terminal->Vtbl = &ConSrvTermVtbl;
    // Terminal->Console will be initialized by ConDrvRegisterTerminal
    Terminal->Data = FrontEnd; /* We store the frontend pointer in the terminal private data */

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConSrvDeinitTerminal(IN OUT PTERMINAL Terminal)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFRONTEND FrontEnd = Terminal->Data;

    /* Reset the ConSrv terminal */
    Terminal->Data = NULL;
    Terminal->Vtbl = NULL;

    /* Unload the frontend */
    if (FrontEnd != NULL)
    {
        Status = ConSrvUnloadFrontEnd(FrontEnd);
        ConsoleFreeHeap(FrontEnd);
    }

    return Status;
}


/* CONSRV TERMINAL INTERFACE **************************************************/

static NTSTATUS NTAPI
ConSrvTermInitTerminal(IN OUT PTERMINAL This,
                  IN PCONSOLE Console)
{
    NTSTATUS Status;
    PFRONTEND FrontEnd = This->Data;

    /* Initialize the console pointer for our frontend */
    FrontEnd->Console = Console;

    /** HACK HACK!! Copy FrontEnd into the console!! **/
    DPRINT1("Using FrontEndIFace HACK(1), should be removed after proper implementation!\n");
    Console->FrontEndIFace = *FrontEnd;

    Status = FrontEnd->Vtbl->InitFrontEnd(FrontEnd, FrontEnd->Console);

    /** HACK HACK!! Be sure FrontEndIFace is correctly updated in the console!! **/
    DPRINT1("Using FrontEndIFace HACK(2), should be removed after proper implementation!\n");
    Console->FrontEndIFace = *FrontEnd;

    return Status;
}

static VOID NTAPI
ConSrvTermDeinitTerminal(IN OUT PTERMINAL This)
{
    PFRONTEND FrontEnd = This->Data;
    FrontEnd->Vtbl->DeinitFrontEnd(FrontEnd);
}

static VOID NTAPI
ConSrvTermDrawRegion(IN OUT PTERMINAL This,
                SMALL_RECT* Region)
{
    PFRONTEND FrontEnd = This->Data;
    FrontEnd->Vtbl->DrawRegion(FrontEnd, Region);
}

static VOID NTAPI
ConSrvTermWriteStream(IN OUT PTERMINAL This,
                 SMALL_RECT* Region,
                 SHORT CursorStartX,
                 SHORT CursorStartY,
                 UINT ScrolledLines,
                 PWCHAR Buffer,
                 UINT Length)
{
    PFRONTEND FrontEnd = This->Data;
    FrontEnd->Vtbl->WriteStream(FrontEnd,
                                Region,
                                CursorStartX,
                                CursorStartY,
                                ScrolledLines,
                                Buffer,
                                Length);
}

static BOOL NTAPI
ConSrvTermSetCursorInfo(IN OUT PTERMINAL This,
                   PCONSOLE_SCREEN_BUFFER ScreenBuffer)
{
    PFRONTEND FrontEnd = This->Data;
    return FrontEnd->Vtbl->SetCursorInfo(FrontEnd, ScreenBuffer);
}

static BOOL NTAPI
ConSrvTermSetScreenInfo(IN OUT PTERMINAL This,
                   PCONSOLE_SCREEN_BUFFER ScreenBuffer,
                   SHORT OldCursorX,
                   SHORT OldCursorY)
{
    PFRONTEND FrontEnd = This->Data;
    return FrontEnd->Vtbl->SetScreenInfo(FrontEnd,
                                         ScreenBuffer,
                                         OldCursorX,
                                         OldCursorY);
}

static VOID NTAPI
ConSrvTermResizeTerminal(IN OUT PTERMINAL This)
{
    PFRONTEND FrontEnd = This->Data;
    FrontEnd->Vtbl->ResizeTerminal(FrontEnd);
}

static VOID NTAPI
ConSrvTermSetActiveScreenBuffer(IN OUT PTERMINAL This)
{
    PFRONTEND FrontEnd = This->Data;
    FrontEnd->Vtbl->SetActiveScreenBuffer(FrontEnd);
}

static VOID NTAPI
ConSrvTermReleaseScreenBuffer(IN OUT PTERMINAL This,
                         IN PCONSOLE_SCREEN_BUFFER ScreenBuffer)
{
    PFRONTEND FrontEnd = This->Data;
    FrontEnd->Vtbl->ReleaseScreenBuffer(FrontEnd, ScreenBuffer);
}

static VOID NTAPI
ConSrvTermChangeTitle(IN OUT PTERMINAL This)
{
    PFRONTEND FrontEnd = This->Data;
    FrontEnd->Vtbl->ChangeTitle(FrontEnd);
}

static VOID NTAPI
ConSrvTermGetLargestConsoleWindowSize(IN OUT PTERMINAL This,
                                 PCOORD pSize)
{
    PFRONTEND FrontEnd = This->Data;
    FrontEnd->Vtbl->GetLargestConsoleWindowSize(FrontEnd, pSize);
}

/*
static BOOL NTAPI
ConSrvTermGetSelectionInfo(IN OUT PTERMINAL This,
                      PCONSOLE_SELECTION_INFO pSelectionInfo)
{
    PFRONTEND FrontEnd = This->Data;
    return FrontEnd->Vtbl->GetSelectionInfo(FrontEnd, pSelectionInfo);
}
*/

static BOOL NTAPI
ConSrvTermSetPalette(IN OUT PTERMINAL This,
                HPALETTE PaletteHandle,
                UINT PaletteUsage)
{
    PFRONTEND FrontEnd = This->Data;
    return FrontEnd->Vtbl->SetPalette(FrontEnd, PaletteHandle, PaletteUsage);
}

static INT NTAPI
ConSrvTermShowMouseCursor(IN OUT PTERMINAL This,
                     BOOL Show)
{
    PFRONTEND FrontEnd = This->Data;
    return FrontEnd->Vtbl->ShowMouseCursor(FrontEnd, Show);
}

static TERMINAL_VTBL ConSrvTermVtbl =
{
    ConSrvTermInitTerminal,
    ConSrvTermDeinitTerminal,
    ConSrvTermDrawRegion,
    ConSrvTermWriteStream,
    ConSrvTermSetCursorInfo,
    ConSrvTermSetScreenInfo,
    ConSrvTermResizeTerminal,
    ConSrvTermSetActiveScreenBuffer,
    ConSrvTermReleaseScreenBuffer,
    ConSrvTermChangeTitle,
    ConSrvTermGetLargestConsoleWindowSize,
    // ConSrvTermGetSelectionInfo,
    ConSrvTermSetPalette,
    ConSrvTermShowMouseCursor,
};

#if 0
VOID
ResetFrontEnd(IN PCONSOLE Console)
{
    if (!Console) return;

    /* Reinitialize the frontend interface */
    RtlZeroMemory(&Console->FrontEndIFace, sizeof(Console->FrontEndIFace));
    Console->FrontEndIFace.Vtbl = &ConSrvTermVtbl;
}
#endif

/* EOF */
