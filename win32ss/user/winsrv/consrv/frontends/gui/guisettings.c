/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/guisettings.c
 * PURPOSE:         GUI Terminal Front-End Settings Management
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#define NDEBUG
#include <debug.h>

#include "concfg/font.h"
#include "guiterm.h"
#include "guisettings.h"

/* FUNCTIONS ******************************************************************/

BOOL
GuiConsoleReadUserSettings(IN OUT PGUI_CONSOLE_INFO TermInfo)
{
    /* Do nothing */
    return TRUE;
}

BOOL
GuiConsoleWriteUserSettings(IN OUT PGUI_CONSOLE_INFO TermInfo)
{
    /* Do nothing */
    return TRUE;
}

VOID
GuiConsoleGetDefaultSettings(IN OUT PGUI_CONSOLE_INFO TermInfo)
{
    /* Do nothing */
}

VOID
GuiConsoleShowConsoleProperties(PGUI_CONSOLE_DATA GuiData,
                                BOOL Defaults)
{
    NTSTATUS Status;
    PCONSRV_CONSOLE Console = GuiData->Console;
    PCONSOLE_PROCESS_DATA ProcessData;
    HANDLE hSection = NULL, hClientSection = NULL;
    PVOID ThreadParameter = NULL; // Is either hClientSection or the console window handle,
                                  // depending on whether we display the default settings or
                                  // the settings of a particular console.

    DPRINT("GuiConsoleShowConsoleProperties entered\n");

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    /* Get the console leader process, our client */
    ProcessData = ConSrvGetConsoleLeaderProcess(Console);

    /*
     * Be sure we effectively have a properties dialog routine (that launches
     * the console control panel applet). It resides in kernel32.dll (client).
     */
    if (ProcessData->PropRoutine == NULL) goto Quit;

    /*
     * Create a memory section to be shared with the console control panel applet
     * in the case we are displaying the settings of a particular console.
     * In that case the ThreadParameter is the hClientSection handle.
     * In the case we display the default console parameters, we don't need to
     * create a memory section. We just need to open the applet, and in this case
     * the ThreadParameter is the parent window handle of the applet's window,
     * that is, the console window.
     */
    if (!Defaults)
    {
        PCONSOLE_SCREEN_BUFFER ActiveBuffer = GuiData->ActiveBuffer;
        LARGE_INTEGER SectionSize;
        SIZE_T ViewSize = 0;
        PCONSOLE_STATE_INFO pSharedInfo = NULL;

        /*
         * Create a memory section to share with the applet, and map it.
         */
        SectionSize.QuadPart  = sizeof(CONSOLE_STATE_INFO);    // Standard size
        SectionSize.QuadPart += Console->OriginalTitle.Length; // Add the length in bytes of the console title string

        Status = NtCreateSection(&hSection,
                                 SECTION_ALL_ACCESS,
                                 NULL,
                                 &SectionSize,
                                 PAGE_READWRITE,
                                 SEC_COMMIT,
                                 NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Error: Impossible to create a shared section, Status = 0x%08lx\n", Status);
            goto Quit;
        }

        Status = NtMapViewOfSection(hSection,
                                    NtCurrentProcess(),
                                    (PVOID*)&pSharedInfo,
                                    0,
                                    0,
                                    NULL,
                                    &ViewSize,
                                    ViewUnmap,
                                    0,
                                    PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Error: Impossible to map the shared section, Status = 0x%08lx\n", Status);
            goto Quit;
        }


        /*
         * Setup the shared console properties structure.
         */

        /* Store the real size of the structure */
        pSharedInfo->cbSize = SectionSize.QuadPart;

        /*
         * When we setup the settings of a particular console, the parent window
         * of the applet's window is the console window, and it is given via the
         * hWnd member of the shared console info structure.
         */
        pSharedInfo->hWnd = GuiData->hWindow;

        /* Console information */
        pSharedInfo->QuickEdit = Console->QuickEdit;
        pSharedInfo->InsertMode = Console->InsertMode;
        pSharedInfo->NumberOfHistoryBuffers = Console->MaxNumberOfHistoryBuffers;
        pSharedInfo->HistoryBufferSize = Console->HistoryBufferSize;
        pSharedInfo->HistoryNoDup = Console->HistoryNoDup;
        /// pSharedInfo->InputBufferSize = 0;
        pSharedInfo->ScreenBufferSize = ActiveBuffer->ScreenBufferSize;
        pSharedInfo->WindowSize = ActiveBuffer->ViewSize;
        pSharedInfo->CursorSize = ActiveBuffer->CursorInfo.dwSize;
        if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
        {
            PTEXTMODE_SCREEN_BUFFER Buffer = (PTEXTMODE_SCREEN_BUFFER)ActiveBuffer;

            pSharedInfo->ScreenAttributes = Buffer->ScreenDefaultAttrib;
            pSharedInfo->PopupAttributes  = Buffer->PopupDefaultAttrib;
        }
        else // if (GetType(ActiveBuffer) == GRAPHICS_BUFFER)
        {
            // PGRAPHICS_SCREEN_BUFFER Buffer = (PGRAPHICS_SCREEN_BUFFER)ActiveBuffer;

            // FIXME: Gather defaults from the registry ?
            pSharedInfo->ScreenAttributes = DEFAULT_SCREEN_ATTRIB;
            pSharedInfo->PopupAttributes  = DEFAULT_POPUP_ATTRIB;
        }

        /* We display the output code page only */
        pSharedInfo->CodePage = Console->OutputCodePage;

        /* GUI Information */
        StringCchCopyNW(pSharedInfo->FaceName, ARRAYSIZE(pSharedInfo->FaceName),
                        GuiData->GuiInfo.FaceName, ARRAYSIZE(GuiData->GuiInfo.FaceName));
        pSharedInfo->FontWeight = GuiData->GuiInfo.FontWeight;
        pSharedInfo->FontFamily = GuiData->GuiInfo.FontFamily;
        pSharedInfo->FontSize   = GuiData->GuiInfo.FontSize;
        pSharedInfo->FullScreen = GuiData->GuiInfo.FullScreen;
        pSharedInfo->AutoPosition   = GuiData->GuiInfo.AutoPosition;
        pSharedInfo->WindowPosition = GuiData->GuiInfo.WindowOrigin;

        /* Palette */
        RtlCopyMemory(pSharedInfo->ColorTable,
                      Console->Colors, sizeof(Console->Colors));

        /* Copy the original title of the console and null-terminate it */
        RtlCopyMemory(pSharedInfo->ConsoleTitle,
                      Console->OriginalTitle.Buffer,
                      Console->OriginalTitle.Length);

        pSharedInfo->ConsoleTitle[Console->OriginalTitle.Length / sizeof(WCHAR)] = UNICODE_NULL;


        /* Unmap the view */
        NtUnmapViewOfSection(NtCurrentProcess(), pSharedInfo);

        /* Duplicate the section handle for the client */
        Status = NtDuplicateObject(NtCurrentProcess(),
                                   hSection,
                                   ProcessData->Process->ProcessHandle,
                                   &hClientSection,
                                   0, 0, DUPLICATE_SAME_ACCESS);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Error: Impossible to duplicate section handle for client, Status = 0x%08lx\n", Status);
            goto Quit;
        }

        /* For the settings of a particular console, use the shared client section handle as the thread parameter */
        ThreadParameter = (PVOID)hClientSection;
    }
    else
    {
        /* For the default settings, use the console window handle as the thread parameter */
        ThreadParameter = (PVOID)GuiData->hWindow;
    }

    /* Start the console control panel applet */
    _SEH2_TRY
    {
        HANDLE Thread = NULL;

        _SEH2_TRY
        {
            Thread = CreateRemoteThread(ProcessData->Process->ProcessHandle, NULL, 0,
                                        ProcessData->PropRoutine,
                                        ThreadParameter, 0, NULL);
            if (NULL == Thread)
            {
                DPRINT1("Failed thread creation (Error: 0x%x)\n", GetLastError());
            }
            else
            {
                DPRINT("ProcessData->PropRoutine remote thread creation succeeded, ProcessId = %x, Process = 0x%p\n",
                       ProcessData->Process->ClientId.UniqueProcess, ProcessData->Process);
            }
        }
        _SEH2_FINALLY
        {
            CloseHandle(Thread);
        }
        _SEH2_END;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("GuiConsoleShowConsoleProperties - Caught an exception, Status = 0x%08lx\n", Status);
    }
    _SEH2_END;

Quit:
    /* We have finished, close the section handle if any */
    if (hSection) NtClose(hSection);

    LeaveCriticalSection(&Console->Lock);
    return;
}

/*
 * Function for dealing with the undocumented message and structure used by
 * Windows' console.dll for setting console info.
 * See https://web.archive.org/web/20220808235525/https://www.catch22.net/assets/files/source/files/setconsoleinfo.c
 * and https://dl.packetstormsecurity.net/papers/win/MSBugPaper.pdf
 * for more information.
 */
VOID
GuiApplyUserSettings(PGUI_CONSOLE_DATA GuiData,
                     HANDLE hClientSection)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSRV_CONSOLE Console = GuiData->Console;
    PCONSOLE_PROCESS_DATA ProcessData;
    HANDLE hSection = NULL;
    SIZE_T ViewSize = 0;
    PCONSOLE_STATE_INFO pConInfo = NULL;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE)) return;

    /* Get the console leader process, our client */
    ProcessData = ConSrvGetConsoleLeaderProcess(Console);

    /* Duplicate the section handle for ourselves */
    Status = NtDuplicateObject(ProcessData->Process->ProcessHandle,
                               hClientSection,
                               NtCurrentProcess(),
                               &hSection,
                               0, 0, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error when mapping client handle, Status = 0x%08lx\n", Status);
        goto Quit;
    }

    /* Get a view of the shared section */
    Status = NtMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                (PVOID*)&pConInfo,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error when mapping view of file, Status = 0x%08lx\n", Status);
        goto Quit;
    }

    _SEH2_TRY
    {
        /* Check that the section is well-sized */
        if ( (ViewSize < sizeof(CONSOLE_STATE_INFO)) ||
             (pConInfo->cbSize < sizeof(CONSOLE_STATE_INFO)) )
        {
            DPRINT1("Error: section bad-sized: sizeof(Section) < sizeof(CONSOLE_STATE_INFO)\n");
            Status = STATUS_INVALID_VIEW_SIZE;
            _SEH2_YIELD(goto Quit);
        }

        // TODO: Check that GuiData->hWindow == pConInfo->hWnd

        /* Console information */

        /*
         * Apply the settings
         */

        /* Refresh the additional TrueType fonts cache now,
         * as ConSrvApplyUserSettings() could change the output
         * code page and trigger a font change in the terminal. */
        RefreshTTFontCache();

        /* Apply the generic console settings */
        ConSrvApplyUserSettings(Console, pConInfo);

        /* Set the terminal settings */

        /* Now, attempt to change the font to what the user specified */
        InitFonts(GuiData,
                  pConInfo->FaceName,
                  pConInfo->FontWeight,
                  pConInfo->FontFamily,
                  pConInfo->FontSize,
                  0, FALSE);

        /* Move the window to the user's values */
        GuiData->GuiInfo.AutoPosition = !!pConInfo->AutoPosition;
        GuiData->GuiInfo.WindowOrigin = pConInfo->WindowPosition;
        GuiConsoleMoveWindow(GuiData);

        InvalidateRect(GuiData->hWindow, NULL, TRUE);

        /*
         * Apply full-screen mode.
         */
        if (!!pConInfo->FullScreen != GuiData->GuiInfo.FullScreen)
        {
            SwitchFullScreen(GuiData, !!pConInfo->FullScreen);
        }

        /*
         * The settings are saved in the registry by console.dll itself, if needed.
         */
        // if (SaveSettings)
        // {
            // GuiConsoleWriteUserSettings(GuiInfo);
        // }

        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("GuiApplyUserSettings - Caught an exception, Status = 0x%08lx\n", Status);
    }
    _SEH2_END;

Quit:
    /* Finally, close the section and return */
    if (hSection)
    {
        NtUnmapViewOfSection(NtCurrentProcess(), pConInfo);
        NtClose(hSection);
    }

    LeaveCriticalSection(&Console->Lock);
    return;
}

/* EOF */
