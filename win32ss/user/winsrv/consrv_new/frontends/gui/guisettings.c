/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/guisettings.c
 * PURPOSE:         GUI Terminal Front-End Settings Management
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/settings.h"
#include "guisettings.h"

#define NDEBUG
#include <debug.h>


VOID GuiConsoleMoveWindow(PGUI_CONSOLE_DATA GuiData);

/* FUNCTIONS ******************************************************************/

BOOL
GuiConsoleReadUserSettings(IN OUT PGUI_CONSOLE_INFO TermInfo,
                           IN LPCWSTR ConsoleTitle,
                           IN DWORD ProcessId)
{
    /*****************************************************
     * Adapted from ConSrvReadUserSettings in settings.c *
     *****************************************************/

    BOOL  RetVal = FALSE;
    HKEY  hKey;
    DWORD dwNumSubKeys = 0;
    DWORD dwIndex;
    DWORD dwType;
    WCHAR szValueName[MAX_PATH];
    DWORD dwValueName;
    WCHAR szValue[LF_FACESIZE] = L"\0";
    DWORD Value;
    DWORD dwValue;

    if (!ConSrvOpenUserSettings(ProcessId,
                                ConsoleTitle,
                                &hKey, KEY_READ,
                                FALSE))
    {
        DPRINT("ConSrvOpenUserSettings failed\n");
        return FALSE;
    }

    if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                        &dwNumSubKeys, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        DPRINT("GuiConsoleReadUserSettings: RegQueryInfoKey failed\n");
        RegCloseKey(hKey);
        return FALSE;
    }

    DPRINT("GuiConsoleReadUserSettings entered dwNumSubKeys %d\n", dwNumSubKeys);

    for (dwIndex = 0; dwIndex < dwNumSubKeys; dwIndex++)
    {
        dwValue = sizeof(Value);
        dwValueName = MAX_PATH; // sizeof(szValueName)/sizeof(szValueName[0])

        if (RegEnumValueW(hKey, dwIndex, szValueName, &dwValueName, NULL, &dwType, (BYTE*)&Value, &dwValue) != ERROR_SUCCESS)
        {
            if (dwType == REG_SZ)
            {
                /*
                 * Retry in case of string value
                 */
                dwValue = sizeof(szValue);
                dwValueName = MAX_PATH; // sizeof(szValueName)/sizeof(szValueName[0])
                if (RegEnumValueW(hKey, dwIndex, szValueName, &dwValueName, NULL, NULL, (BYTE*)szValue, &dwValue) != ERROR_SUCCESS)
                    break;
            }
            else
            {
                break;
            }
        }

        if (!wcscmp(szValueName, L"FaceName"))
        {
            SIZE_T Length = min(wcslen(szValue) + 1, LF_FACESIZE); // wcsnlen
            wcsncpy(TermInfo->FaceName, szValue, LF_FACESIZE);
            TermInfo->FaceName[Length] = L'\0';
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"FontFamily"))
        {
            TermInfo->FontFamily = Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"FontSize"))
        {
            TermInfo->FontSize = Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"FontWeight"))
        {
            TermInfo->FontWeight = Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"FullScreen"))
        {
            TermInfo->FullScreen = Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"WindowPosition"))
        {
            TermInfo->AutoPosition = FALSE;
            TermInfo->WindowOrigin.x = LOWORD(Value);
            TermInfo->WindowOrigin.y = HIWORD(Value);
            RetVal = TRUE;
        }
    }

    RegCloseKey(hKey);
    return RetVal;
}

BOOL
GuiConsoleWriteUserSettings(IN OUT PGUI_CONSOLE_INFO TermInfo,
                            IN LPCWSTR ConsoleTitle,
                            IN DWORD ProcessId)
{
    /******************************************************
     * Adapted from ConSrvWriteUserSettings in settings.c *
     ******************************************************/

    BOOL GlobalSettings = (ConsoleTitle[0] == L'\0');
    HKEY hKey;
    DWORD Storage = 0;

#define SetConsoleSetting(SettingName, SettingType, SettingSize, Setting, DefaultValue)         \
do {                                                                                            \
    if (GlobalSettings || (!GlobalSettings && (*(Setting) != (DefaultValue))))                  \
    {                                                                                           \
        RegSetValueExW(hKey, (SettingName), 0, (SettingType), (PBYTE)(Setting), (SettingSize)); \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        RegDeleteValue(hKey, (SettingName));                                                    \
    }                                                                                           \
} while (0)

    if (!ConSrvOpenUserSettings(ProcessId,
                                ConsoleTitle,
                                &hKey, KEY_WRITE,
                                TRUE))
    {
        return FALSE;
    }

    SetConsoleSetting(L"FaceName", REG_SZ, (wcslen(TermInfo->FaceName) + 1) * sizeof(WCHAR), TermInfo->FaceName, L'\0'); // wcsnlen
    SetConsoleSetting(L"FontFamily", REG_DWORD, sizeof(DWORD), &TermInfo->FontFamily, FF_DONTCARE);
    SetConsoleSetting(L"FontSize", REG_DWORD, sizeof(DWORD), &TermInfo->FontSize, 0);
    SetConsoleSetting(L"FontWeight", REG_DWORD, sizeof(DWORD), &TermInfo->FontWeight, FW_DONTCARE);

    Storage = TermInfo->FullScreen;
    SetConsoleSetting(L"FullScreen", REG_DWORD, sizeof(DWORD), &Storage, FALSE);

    if (TermInfo->AutoPosition == FALSE)
    {
        Storage = MAKELONG(TermInfo->WindowOrigin.x, TermInfo->WindowOrigin.y);
        RegSetValueExW(hKey, L"WindowPosition", 0, REG_DWORD, (PBYTE)&Storage, sizeof(DWORD));
    }
    else
    {
        RegDeleteValue(hKey, L"WindowPosition");
    }

    RegCloseKey(hKey);
    return TRUE;
}

VOID
GuiConsoleGetDefaultSettings(IN OUT PGUI_CONSOLE_INFO TermInfo,
                             IN DWORD ProcessId)
{
    /*******************************************************
     * Adapted from ConSrvGetDefaultSettings in settings.c *
     *******************************************************/

    if (TermInfo == NULL) return;

    /*
     * 1. Load the default values
     */
    // wcsncpy(TermInfo->FaceName, L"DejaVu Sans Mono", LF_FACESIZE);
    // TermInfo->FontSize = MAKELONG(12, 8); // 0x0008000C; // font is 8x12
    // TermInfo->FontSize = MAKELONG(16, 16); // font is 16x16
    // TermInfo->FontWeight = FW_NORMAL;

    wcsncpy(TermInfo->FaceName, L"VGA", LF_FACESIZE); // HACK: !!
    // TermInfo->FaceName[0] = L'\0';
    TermInfo->FontFamily = FF_DONTCARE;
    TermInfo->FontSize   = 0;
    TermInfo->FontWeight = FW_DONTCARE;
    TermInfo->UseRasterFonts = TRUE;

    TermInfo->FullScreen   = FALSE;
    TermInfo->ShowWindow   = SW_SHOWNORMAL;
    TermInfo->AutoPosition = TRUE;
    TermInfo->WindowOrigin.x = 0;
    TermInfo->WindowOrigin.y = 0;

    /*
     * 2. Overwrite them with the ones stored in HKCU\Console.
     *    If the HKCU\Console key doesn't exist, create it
     *    and store the default values inside.
     */
    if (!GuiConsoleReadUserSettings(TermInfo, L"", ProcessId))
    {
        GuiConsoleWriteUserSettings(TermInfo, L"", ProcessId);
    }
}

VOID
GuiConsoleShowConsoleProperties(PGUI_CONSOLE_DATA GuiData,
                                BOOL Defaults)
{
    NTSTATUS Status;
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = Console->ActiveBuffer;
    PCONSOLE_PROCESS_DATA ProcessData;
    HANDLE hSection = NULL, hClientSection = NULL;
    LARGE_INTEGER SectionSize;
    ULONG ViewSize = 0;
    SIZE_T Length = 0;
    PCONSOLE_PROPS pSharedInfo = NULL;
    PGUI_CONSOLE_INFO GuiInfo = NULL;

    DPRINT("GuiConsoleShowConsoleProperties entered\n");

    /*
     * Create a memory section to share with the applet, and map it.
     */
    /* Holds data for console.dll + console info + terminal-specific info */
    SectionSize.QuadPart = sizeof(CONSOLE_PROPS) + sizeof(GUI_CONSOLE_INFO);
    Status = NtCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &SectionSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Impossible to create a shared section ; Status = %lu\n", Status);
        return;
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
        DPRINT1("Error: Impossible to map the shared section ; Status = %lu\n", Status);
        NtClose(hSection);
        return;
    }


    /*
     * Setup the shared console properties structure.
     */

    /* Header */
    pSharedInfo->hConsoleWindow = GuiData->hWindow;
    pSharedInfo->ShowDefaultParams = Defaults;

    /*
     * We fill-in the fields only if we display
     * our properties, not the default ones.
     */
    if (!Defaults)
    {
        /* Console information */
        pSharedInfo->ci.HistoryBufferSize = Console->HistoryBufferSize;
        pSharedInfo->ci.NumberOfHistoryBuffers = Console->NumberOfHistoryBuffers;
        pSharedInfo->ci.HistoryNoDup = Console->HistoryNoDup;
        pSharedInfo->ci.QuickEdit = Console->QuickEdit;
        pSharedInfo->ci.InsertMode = Console->InsertMode;
        pSharedInfo->ci.InputBufferSize = 0;
        pSharedInfo->ci.ScreenBufferSize = ActiveBuffer->ScreenBufferSize;
        pSharedInfo->ci.ConsoleSize = ActiveBuffer->ViewSize;
        pSharedInfo->ci.CursorBlinkOn;
        pSharedInfo->ci.ForceCursorOff;
        pSharedInfo->ci.CursorSize = ActiveBuffer->CursorInfo.dwSize;
        if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
        {
            PTEXTMODE_SCREEN_BUFFER Buffer = (PTEXTMODE_SCREEN_BUFFER)ActiveBuffer;

            pSharedInfo->ci.ScreenAttrib = Buffer->ScreenDefaultAttrib;
            pSharedInfo->ci.PopupAttrib  = Buffer->PopupDefaultAttrib;
        }
        else // if (GetType(ActiveBuffer) == GRAPHICS_BUFFER)
        {
            // PGRAPHICS_SCREEN_BUFFER Buffer = (PGRAPHICS_SCREEN_BUFFER)ActiveBuffer;
            DPRINT1("GuiConsoleShowConsoleProperties - Graphics buffer\n");

            // FIXME: Gather defaults from the registry ?
            pSharedInfo->ci.ScreenAttrib = DEFAULT_SCREEN_ATTRIB;
            pSharedInfo->ci.PopupAttrib  = DEFAULT_POPUP_ATTRIB ;
        }
        pSharedInfo->ci.CodePage;

        /* GUI Information */
        pSharedInfo->TerminalInfo.Size = sizeof(GUI_CONSOLE_INFO);
        GuiInfo = pSharedInfo->TerminalInfo.TermInfo = (PGUI_CONSOLE_INFO)(pSharedInfo + 1);
        Length = min(wcslen(GuiData->GuiInfo.FaceName) + 1, LF_FACESIZE); // wcsnlen
        wcsncpy(GuiInfo->FaceName, GuiData->GuiInfo.FaceName, LF_FACESIZE);
        GuiInfo->FaceName[Length] = L'\0';
        GuiInfo->FontFamily = GuiData->GuiInfo.FontFamily;
        GuiInfo->FontSize   = GuiData->GuiInfo.FontSize;
        GuiInfo->FontWeight = GuiData->GuiInfo.FontWeight;
        GuiInfo->UseRasterFonts = GuiData->GuiInfo.UseRasterFonts;
        GuiInfo->FullScreen = GuiData->GuiInfo.FullScreen;
        /// GuiInfo->WindowPosition = GuiData->GuiInfo.WindowPosition;
        GuiInfo->AutoPosition = GuiData->GuiInfo.AutoPosition;
        GuiInfo->WindowOrigin = GuiData->GuiInfo.WindowOrigin;
        /* Offsetize */
        pSharedInfo->TerminalInfo.TermInfo = (PVOID)((ULONG_PTR)GuiInfo - (ULONG_PTR)pSharedInfo);

        /* Palette */
        memcpy(pSharedInfo->ci.Colors, Console->Colors, sizeof(Console->Colors));

        /* Title of the console, original one corresponding to the one set by the console leader */
        Length = min(sizeof(pSharedInfo->ci.ConsoleTitle) / sizeof(pSharedInfo->ci.ConsoleTitle[0]) - 1,
                     Console->OriginalTitle.Length / sizeof(WCHAR));
        wcsncpy(pSharedInfo->ci.ConsoleTitle, Console->OriginalTitle.Buffer, Length);
    }
    else
    {
        Length = 0;
        // FIXME: Load the default parameters from the registry.
    }

    /* Null-terminate the title */
    pSharedInfo->ci.ConsoleTitle[Length] = L'\0';


    /* Unmap the view */
    NtUnmapViewOfSection(NtCurrentProcess(), pSharedInfo);

    /* Get the console leader process, our client */
    ProcessData = CONTAINING_RECORD(Console->ProcessList.Blink,
                                    CONSOLE_PROCESS_DATA,
                                    ConsoleLink);

    /* Duplicate the section handle for the client */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               hSection,
                               ProcessData->Process->ProcessHandle,
                               &hClientSection,
                               0, 0, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Impossible to duplicate section handle for client ; Status = %lu\n", Status);
        goto Quit;
    }

    /* Start the properties dialog */
    if (ProcessData->PropDispatcher)
    {
        _SEH2_TRY
        {
            HANDLE Thread = NULL;

            _SEH2_TRY
            {
                Thread = CreateRemoteThread(ProcessData->Process->ProcessHandle, NULL, 0,
                                            ProcessData->PropDispatcher,
                                            (PVOID)hClientSection, 0, NULL);
                if (NULL == Thread)
                {
                    DPRINT1("Failed thread creation (Error: 0x%x)\n", GetLastError());
                }
                else
                {
                    DPRINT("ProcessData->PropDispatcher remote thread creation succeeded, ProcessId = %x, Process = 0x%p\n", ProcessData->Process->ClientId.UniqueProcess, ProcessData->Process);
                    /// WaitForSingleObject(Thread, INFINITE);
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
            DPRINT1("GuiConsoleShowConsoleProperties - Caught an exception, Status = %08X\n", Status);
        }
        _SEH2_END;
    }

Quit:
    /* We have finished, close the section handle */
    NtClose(hSection);
    return;
}

NTSTATUS
GuiApplyUserSettings(PGUI_CONSOLE_DATA GuiData,
                     HANDLE hClientSection,
                     BOOL SaveSettings)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE Console = GuiData->Console;
    PCONSOLE_PROCESS_DATA ProcessData;
    HANDLE hSection = NULL;
    ULONG ViewSize = 0;
    PCONSOLE_PROPS pConInfo = NULL;
    PCONSOLE_INFO  ConInfo  = NULL;
    PTERMINAL_INFO TermInfo = NULL;
    PGUI_CONSOLE_INFO GuiInfo = NULL;

    /* Get the console leader process, our client */
    ProcessData = CONTAINING_RECORD(Console->ProcessList.Blink,
                                    CONSOLE_PROCESS_DATA,
                                    ConsoleLink);

    /* Duplicate the section handle for ourselves */
    Status = NtDuplicateObject(ProcessData->Process->ProcessHandle,
                               hClientSection,
                               NtCurrentProcess(),
                               &hSection,
                               0, 0, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error when mapping client handle, Status = %lu\n", Status);
        return Status;
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
                                PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error when mapping view of file, Status = %lu\n", Status);
        NtClose(hSection);
        return Status;
    }

    _SEH2_TRY
    {
        /* Check that the section is well-sized */
        if ( (ViewSize < sizeof(CONSOLE_PROPS)) ||
             (pConInfo->TerminalInfo.Size != sizeof(GUI_CONSOLE_INFO)) ||
             (ViewSize < sizeof(CONSOLE_PROPS) + pConInfo->TerminalInfo.Size) )
        {
            DPRINT1("Error: section bad-sized: sizeof(Section) < sizeof(CONSOLE_PROPS) + sizeof(Terminal_specific_info)\n");
            Status = STATUS_INVALID_VIEW_SIZE;
            _SEH2_YIELD(goto Quit);
        }

        // TODO: Check that GuiData->hWindow == pConInfo->hConsoleWindow

        /* Retrieve terminal informations */
        ConInfo  = &pConInfo->ci;
        TermInfo = &pConInfo->TerminalInfo;
        GuiInfo  = TermInfo->TermInfo = (PVOID)((ULONG_PTR)pConInfo + (ULONG_PTR)TermInfo->TermInfo);

        /*
         * If we don't set the default parameters,
         * apply them, otherwise just save them.
         */
        if (pConInfo->ShowDefaultParams == FALSE)
        {
            /* Set the console informations */
            ConSrvApplyUserSettings(Console, ConInfo);

            /* Set the terminal informations */

            // memcpy(&GuiData->GuiInfo, GuiInfo, sizeof(GUI_CONSOLE_INFO));

            /* Move the window to the user's values */
            GuiData->GuiInfo.AutoPosition = GuiInfo->AutoPosition;
            GuiData->GuiInfo.WindowOrigin = GuiInfo->WindowOrigin;
            GuiConsoleMoveWindow(GuiData);

            InvalidateRect(GuiData->hWindow, NULL, TRUE);

            /*
             * Apply full-screen mode.
             */
            GuiData->GuiInfo.FullScreen = GuiInfo->FullScreen;
            // TODO: Apply it really
        }

        /*
         * Save settings if needed
         */
        // FIXME: Do it in the console properties applet ??
        if (SaveSettings)
        {
            DWORD ProcessId = HandleToUlong(ProcessData->Process->ClientId.UniqueProcess);
            ConSrvWriteUserSettings(ConInfo, ProcessId);
            GuiConsoleWriteUserSettings(GuiInfo, ConInfo->ConsoleTitle, ProcessId);
        }

        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("GuiApplyUserSettings - Caught an exception, Status = %08X\n", Status);
    }
    _SEH2_END;

Quit:
    /* Finally, close the section and return */
    NtUnmapViewOfSection(NtCurrentProcess(), pConInfo);
    NtClose(hSection);
    return Status;
}

/* EOF */
