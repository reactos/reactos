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

/* API_NUMBER: ConsolepGetHardwareState */
CON_API(SrvGetConsoleHardwareState,
        CONSOLE_GETSETHWSTATE, HardwareStateRequest)
{
#if 0
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ProcessData,
                                   HardwareStateRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_READ,
                                   TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    HardwareStateRequest->State = Console->HardwareState;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
#else
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
#endif
}

/* API_NUMBER: ConsolepSetHardwareState */
CON_API(SrvSetConsoleHardwareState,
        CONSOLE_GETSETHWSTATE, HardwareStateRequest)
{
#if 0
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ProcessData,
                                   HardwareStateRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    DPRINT("Setting console hardware state.\n");
    Status = SetConsoleHardwareState(Console, HardwareStateRequest->State);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
#else
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
#endif
}

/* API_NUMBER: ConsolepGetDisplayMode */
CON_API(SrvGetConsoleDisplayMode,
        CONSOLE_GETDISPLAYMODE, GetDisplayModeRequest)
{
    GetDisplayModeRequest->DisplayMode = TermGetDisplayMode(Console);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepSetDisplayMode */
CON_API(SrvSetConsoleDisplayMode,
        CONSOLE_SETDISPLAYMODE, SetDisplayModeRequest)
{
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ProcessData,
                                   SetDisplayModeRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

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

/* API_NUMBER: ConsolepGetLargestWindowSize */
CON_API(SrvGetLargestConsoleWindowSize,
        CONSOLE_GETLARGESTWINDOWSIZE, GetLargestWindowSizeRequest)
{
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ProcessData,
                                     GetLargestWindowSizeRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    /*
     * Retrieve the largest possible console window size, without
     * taking into account the size of the console screen buffer
     * (thus differs from ConDrvGetConsoleScreenBufferInfo).
     */
    TermGetLargestConsoleWindowSize(Console, &GetLargestWindowSizeRequest->Size);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepShowCursor */
CON_API(SrvShowConsoleCursor,
        CONSOLE_SHOWCURSOR, ShowCursorRequest)
{
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ProcessData,
                                   ShowCursorRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    ShowCursorRequest->RefCount = TermShowMouseCursor(Console, ShowCursorRequest->Show);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepSetCursor */
CON_API(SrvSetConsoleCursor,
        CONSOLE_SETCURSOR, SetCursorRequest)
{
    NTSTATUS Status;
    BOOL Success;
    PCONSOLE_SCREEN_BUFFER Buff;

    // NOTE: Tests show that this function is used only for graphics screen buffers
    // and otherwise it returns FALSE and sets last error to ERROR_INVALID_HANDLE.
    // I find that behaviour is ridiculous but ok, let's accept it at the moment...
    Status = ConSrvGetGraphicsBuffer(ProcessData,
                                     SetCursorRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_WRITE,
                                     TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    Success = TermSetMouseCursor(Console, SetCursorRequest->CursorHandle);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

/* API_NUMBER: ConsolepMenuControl */
CON_API(SrvConsoleMenuControl,
        CONSOLE_MENUCONTROL, MenuControlRequest)
{
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetScreenBuffer(ProcessData,
                                   MenuControlRequest->OutputHandle,
                                   &Buff,
                                   GENERIC_WRITE,
                                   TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    MenuControlRequest->MenuHandle = TermMenuControl(Console,
                                                     MenuControlRequest->CmdIdLow,
                                                     MenuControlRequest->CmdIdHigh);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepSetMenuClose */
CON_API(SrvSetConsoleMenuClose,
        CONSOLE_SETMENUCLOSE, SetMenuCloseRequest)
{
    return (TermSetMenuClose(Console, SetMenuCloseRequest->Enable)
                ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

/* Used by USERSRV!SrvGetThreadConsoleDesktop() */
NTSTATUS
NTAPI
GetThreadConsoleDesktop(
    IN ULONG_PTR ThreadId,
    OUT HDESK* ConsoleDesktop)
{
    NTSTATUS Status;
    PCSR_THREAD CsrThread;
    PCONSRV_CONSOLE Console;

    /* No console desktop handle by default */
    *ConsoleDesktop = NULL;

    /* Retrieve and lock the thread */
    Status = CsrLockThreadByClientId(ULongToHandle(ThreadId), &CsrThread);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT(CsrThread->Process);

    /* Retrieve the console to which the process is attached, and unlock the thread */
    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrThread->Process),
                              &Console, TRUE);
    CsrUnlockThread(CsrThread);

    if (!NT_SUCCESS(Status))
        return Status;

    /* Retrieve the console desktop handle, and release the console */
    *ConsoleDesktop = TermGetThreadConsoleDesktop(Console);
    ConSrvReleaseConsole(Console, TRUE);

    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepGetConsoleWindow */
CON_API(SrvGetConsoleWindow,
        CONSOLE_GETWINDOW, GetWindowRequest)
{
    GetWindowRequest->WindowHandle = TermGetConsoleWindowHandle(Console);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepSetIcon */
CON_API(SrvSetConsoleIcon,
        CONSOLE_SETICON, SetIconRequest)
{
    return (TermChangeIcon(Console, SetIconRequest->IconHandle)
                ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

/* API_NUMBER: ConsolepGetSelectionInfo */
CON_API(SrvGetConsoleSelectionInfo,
        CONSOLE_GETSELECTIONINFO, GetSelectionInfoRequest)
{
    return (TermGetSelectionInfo(Console, &GetSelectionInfoRequest->Info)
                ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

/* API_NUMBER: ConsolepGetNumberOfFonts */
CON_API(SrvGetConsoleNumberOfFonts,
        CONSOLE_GETNUMFONTS, GetNumFontsRequest)
{
    // FIXME!
    // TermGetNumberOfFonts(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    GetNumFontsRequest->NumFonts = 0;
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepGetFontInfo */
CON_API(SrvGetConsoleFontInfo,
        CONSOLE_GETFONTINFO, GetFontInfoRequest)
{
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ProcessData,
                                     GetFontInfoRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    // FIXME!
    // TermGetFontInfo(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    GetFontInfoRequest->NumFonts = 0;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepGetFontSize */
CON_API(SrvGetConsoleFontSize,
        CONSOLE_GETFONTSIZE, GetFontSizeRequest)
{
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ProcessData,
                                     GetFontSizeRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    // FIXME!
    // TermGetFontSize(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepGetCurrentFont */
CON_API(SrvGetConsoleCurrentFont,
        CONSOLE_GETCURRENTFONT, GetCurrentFontRequest)
{
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ProcessData,
                                     GetCurrentFontRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_READ,
                                     TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    // FIXME!
    // TermGetCurrentFont(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    GetCurrentFontRequest->FontIndex = 0;

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* API_NUMBER: ConsolepSetFont */
CON_API(SrvSetConsoleFont,
        CONSOLE_SETFONT, SetFontRequest)
{
    NTSTATUS Status;
    PCONSOLE_SCREEN_BUFFER Buff;

    Status = ConSrvGetTextModeBuffer(ProcessData,
                                     SetFontRequest->OutputHandle,
                                     &Buff,
                                     GENERIC_WRITE,
                                     TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    ASSERT((PCONSOLE)Console == Buff->Header.Console);

    // FIXME!
    // TermSetFont(Console, ...);
    DPRINT1("%s not yet implemented\n", __FUNCTION__);

    ConSrvReleaseScreenBuffer(Buff, TRUE);
    return STATUS_SUCCESS;
}

/* EOF */
