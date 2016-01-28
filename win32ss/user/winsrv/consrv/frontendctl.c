/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontendctl.c
 * PURPOSE:         Terminal Front-Ends Control
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#define NDEBUG
#include <debug.h>

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
static NTSTATUS
SetConsoleHardwareState(PCONSRV_CONSOLE Console, ULONG ConsoleHwState)
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
    PCONSRV_CONSOLE Console;

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
    PCONSRV_CONSOLE Console;

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
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    GetDisplayModeRequest->DisplayMode = TermGetDisplayMode(Console);

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleDisplayMode)
{
    NTSTATUS Status;
    PCONSOLE_SETDISPLAYMODE SetDisplayModeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetDisplayModeRequest;
    PCONSRV_CONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   SetDisplayModeRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = (PCONSRV_CONSOLE)Buff->Header.Console;

    if (TermSetDisplayMode(Console, SetDisplayModeRequest->DisplayMode))
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
    PCONSOLE /*PCONSRV_CONSOLE*/ Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     GetLargestWindowSizeRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;
    TermGetLargestConsoleWindowSize(Console, &GetLargestWindowSizeRequest->Size);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvShowConsoleCursor)
{
    NTSTATUS Status;
    PCONSOLE_SHOWCURSOR ShowCursorRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ShowCursorRequest;
    PCONSOLE /*PCONSRV_CONSOLE*/ Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   ShowCursorRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = Buff->Header.Console;

    ShowCursorRequest->RefCount = TermShowMouseCursor(Console, ShowCursorRequest->Show);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleCursor)
{
    NTSTATUS Status;
    BOOL Success;
    PCONSOLE_SETCURSOR SetCursorRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetCursorRequest;
    PCONSRV_CONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    // NOTE: Tests show that this function is used only for graphics screen buffers
    // and otherwise it returns FALSE and sets last error to ERROR_INVALID_HANDLE.
    // I find that behaviour is ridiculous but ok, let's accept it at the moment...
    Status = ConSrvGetGraphicsBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetCursorRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_WRITE,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = (PCONSRV_CONSOLE)Buff->Header.Console;

    Success = TermSetMouseCursor(Console, SetCursorRequest->CursorHandle);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

CSR_API(SrvConsoleMenuControl)
{
    NTSTATUS Status;
    PCONSOLE_MENUCONTROL MenuControlRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.MenuControlRequest;
    PCONSRV_CONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   MenuControlRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = (PCONSRV_CONSOLE)Buff->Header.Console;

    MenuControlRequest->MenuHandle = TermMenuControl(Console,
                                                     MenuControlRequest->CmdIdLow,
                                                     MenuControlRequest->CmdIdHigh);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleMenuClose)
{
    NTSTATUS Status;
    BOOL Success;
    PCONSOLE_SETMENUCLOSE SetMenuCloseRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetMenuCloseRequest;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Success = TermSetMenuClose(Console, SetMenuCloseRequest->Enable);

    ConSrvReleaseConsole(Console, TRUE);
    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

CSR_API(SrvGetConsoleWindow)
{
    NTSTATUS Status;
    PCONSOLE_GETWINDOW GetWindowRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetWindowRequest;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    GetWindowRequest->WindowHandle = TermGetConsoleWindowHandle(Console);

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleIcon)
{
    NTSTATUS Status;
    PCONSOLE_SETICON SetIconRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetIconRequest;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = (TermChangeIcon(Console, SetIconRequest->IconHandle)
                ? STATUS_SUCCESS
                : STATUS_UNSUCCESSFUL);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvGetConsoleSelectionInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSELECTIONINFO GetSelectionInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetSelectionInfoRequest;
    PCONSRV_CONSOLE Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = (TermGetSelectionInfo(Console, &GetSelectionInfoRequest->Info)
                ? STATUS_SUCCESS
                : STATUS_UNSUCCESSFUL);

    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}



CSR_API(SrvGetConsoleNumberOfFonts)
{
    NTSTATUS Status;
    PCONSOLE_GETNUMFONTS GetNumFontsRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetNumFontsRequest;
    PCONSOLE /*PCONSRV_CONSOLE*/ Console;

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    // FIXME!
    // TermGetNumberOfFonts(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    GetNumFontsRequest->NumFonts = 0;

    ConSrvReleaseConsole(Console, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleFontInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETFONTINFO GetFontInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetFontInfoRequest;
    // PCONSOLE /*PCONSRV_CONSOLE*/ Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     GetFontInfoRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    // FIXME!
    // Console = Buff->Header.Console;
    // TermGetFontInfo(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    GetFontInfoRequest->NumFonts = 0;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleFontSize)
{
    NTSTATUS Status;
    PCONSOLE_GETFONTSIZE GetFontSizeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetFontSizeRequest;
    // PCONSOLE /*PCONSRV_CONSOLE*/ Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     GetFontSizeRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    // FIXME!
    // Console = Buff->Header.Console;
    // TermGetFontSize(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleCurrentFont)
{
    NTSTATUS Status;
    PCONSOLE_GETCURRENTFONT GetCurrentFontRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetCurrentFontRequest;
    // PCONSOLE /*PCONSRV_CONSOLE*/ Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     GetCurrentFontRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    // FIXME!
    // Console = Buff->Header.Console;
    // TermGetCurrentFont(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    GetCurrentFontRequest->FontIndex = 0;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvSetConsoleFont)
{
    NTSTATUS Status;
    PCONSOLE_SETFONT SetFontRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetFontRequest;
    // PCONSOLE /*PCONSRV_CONSOLE*/ Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetFontRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_WRITE,
                                     TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    // FIXME!
    // Console = Buff->Header.Console;
    // TermSetFont(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* EOF */
