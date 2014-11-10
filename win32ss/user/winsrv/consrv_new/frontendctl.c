/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontendctl.c
 * PURPOSE:         Terminal Front-Ends Control
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/conio2.h"
#include "conoutput.h"
#include "console.h"
#include "handle.h"

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS **********************************************************/


/* PUBLIC SERVER APIS *********************************************************/

/**********************************************************************
 *  HardwareStateProperty
 *
 *  DESCRIPTION
 *      Set/Get the value of the HardwareState and switch
 *      between direct video buffer ouput and GDI windowed
 *      output.
 *  ARGUMENTS
 *      Client hands us a CONSOLE_GETSETHWSTATE object.
 *      We use the same object to Request.
 *  NOTE
 *      ConsoleHwState has the correct size to be compatible
 *      with NT's, but values are not.
 */
#if 0
static NTSTATUS FASTCALL
SetConsoleHardwareState(PCONSOLE Console, ULONG ConsoleHwState)
{
    DPRINT1("Console Hardware State: %d\n", ConsoleHwState);

    if ((CONSOLE_HARDWARE_STATE_GDI_MANAGED == ConsoleHwState)
            ||(CONSOLE_HARDWARE_STATE_DIRECT == ConsoleHwState))
    {
        if (Console->HardwareState != ConsoleHwState)
        {
            /* TODO: implement switching from full screen to windowed mode */
            /* TODO: or back; now simply store the hardware state */
            Console->HardwareState = ConsoleHwState;
        }

        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER_3; /* Client: (handle, set_get, [mode]) */
}
#endif

CSR_API(SrvGetConsoleHardwareState)
{
#if 0
    NTSTATUS Status;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.HardwareStateRequest;
    PCONSOLE_SCREEN_BUFFER Buff;
    PCONSOLE Console;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   HardwareStateRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_READ,
                                   TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvGetConsoleHardwareState\n");
        return Status;
    }

    Console = Buff->Header.Console;
    HardwareStateRequest->State = Console->HardwareState;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return Status;
#else
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
#endif
}

CSR_API(SrvSetConsoleHardwareState)
{
#if 0
    NTSTATUS Status;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.HardwareStateRequest;
    PCONSOLE_SCREEN_BUFFER Buff;
    PCONSOLE Console;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   HardwareStateRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get console handle in SrvSetConsoleHardwareState\n");
        return Status;
    }

    DPRINT("Setting console hardware state.\n");
    Console = Buff->Header.Console;
    Status = SetConsoleHardwareState(Console, HardwareStateRequest->State);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return Status;
#else
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
#endif
}

CSR_API(SrvGetConsoleDisplayMode)
{
    NTSTATUS Status;
    PCONSOLE_GETDISPLAYMODE GetDisplayModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetDisplayModeRequest;
    PCONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    GetDisplayModeRequest->DisplayMode = ConioGetDisplayMode(Console);

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleDisplayMode)
{
    NTSTATUS Status;
    PCONSOLE_SETDISPLAYMODE SetDisplayModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetDisplayModeRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   SetDisplayModeRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    if (ConioSetDisplayMode(Console, SetDisplayModeRequest->DisplayMode))
    {
        SetDisplayModeRequest->NewSBDim = Buff->ScreenBufferSize;
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return Status;
}

CSR_API(SrvGetLargestConsoleWindowSize)
{
    NTSTATUS Status;
    PCONSOLE_GETLARGESTWINDOWSIZE GetLargestWindowSizeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetLargestWindowSizeRequest;
    PCONSOLE_SCREEN_BUFFER Buff;
    PCONSOLE Console;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   GetLargestWindowSizeRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_READ,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;
    ConioGetLargestConsoleWindowSize(Console, &GetLargestWindowSizeRequest->Size);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvShowConsoleCursor)
{
    NTSTATUS Status;
    PCONSOLE_SHOWCURSOR ShowCursorRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ShowCursorRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   ShowCursorRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    ShowCursorRequest->RefCount = ConioShowMouseCursor(Console, ShowCursorRequest->Show);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleCursor)
{
    NTSTATUS Status;
    BOOL Success;
    PCONSOLE_SETCURSOR SetCursorRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetCursorRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    // FIXME: Tests show that this function is used only for graphics screen buffers
    // and otherwise it returns false + set last error to invalid handle.
    // NOTE: I find that behaviour is ridiculous but ok, let's accept that at the moment...
    Status = ConSrvGetGraphicsBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetCursorRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_WRITE,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    Success = ConioSetMouseCursor(Console, SetCursorRequest->hCursor);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

CSR_API(SrvConsoleMenuControl)
{
    NTSTATUS Status;
    PCONSOLE_MENUCONTROL MenuControlRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.MenuControlRequest;
    PCONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   MenuControlRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    MenuControlRequest->hMenu = ConioMenuControl(Console,
                                                 MenuControlRequest->dwCmdIdLow,
                                                 MenuControlRequest->dwCmdIdHigh);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleMenuClose)
{
    NTSTATUS Status;
    BOOL Success;
    PCONSOLE_SETMENUCLOSE SetMenuCloseRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetMenuCloseRequest;
    PCONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Success = ConioSetMenuClose(Console, SetMenuCloseRequest->Enable);

    ConSrvReleaseConsole(Console, TRUE);
    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

CSR_API(SrvGetConsoleWindow)
{
    NTSTATUS Status;
    PCONSOLE_GETWINDOW GetWindowRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetWindowRequest;
    PCONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    GetWindowRequest->WindowHandle = ConioGetConsoleWindowHandle(Console);

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleIcon)
{
    NTSTATUS Status;
    PCONSOLE_SETICON SetIconRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetIconRequest;
    PCONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = (ConioChangeIcon(Console, SetIconRequest->WindowIcon)
                ? STATUS_SUCCESS
                : STATUS_UNSUCCESSFUL);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleSelectionInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSELECTIONINFO GetSelectionInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetSelectionInfoRequest;
    PCONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), &Console, TRUE);
    if (NT_SUCCESS(Status))
    {
        memset(&GetSelectionInfoRequest->Info, 0, sizeof(CONSOLE_SELECTION_INFO));
        if (Console->Selection.dwFlags != 0)
            GetSelectionInfoRequest->Info = Console->Selection;
        ConSrvReleaseConsole(Console, TRUE);
    }

    return Status;
}

/* EOF */
