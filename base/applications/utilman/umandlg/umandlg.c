/*
 * PROJECT:         ReactOS Utility Manager Resources DLL (UManDlg.dll)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Main DLL code file
 * COPYRIGHT:       Copyright 2019-2020 George Bișoc (george.bisoc@reactos.org)
 *                  Copyright 2019 Hermes Belusca-Maito
 */

/* INCLUDES *******************************************************************/

#include "umandlg.h"

/* GLOBALS ********************************************************************/

UTILMAN_GLOBALS Globals;

/* DECLARATIONS ***************************************************************/

UTILMAN_STATE EntriesList[] =
{
    {L"magnify.exe", IDS_MAGNIFIER, L"", FALSE},
    {L"osk.exe", IDS_OSK, L"", FALSE}
};

/* FUNCTIONS ******************************************************************/

/**
 * @InitUtilsList
 *
 * Initializes the list of accessibility utilities.
 *
 * @param[in]   bInitGui
 *     Whether we are initializing the UI list (TRUE) or the internal array (FALSE).
 *
 * @return
 *     Nothing.
 */
VOID InitUtilsList(IN BOOL bInitGui)
{
    UINT i;

    if (!bInitGui)
    {
        // TODO: Load the list dynamically from the registry key
        // hklm\software\microsoft\windows nt\currentversion\accessibility

        /* Initialize the resource utility strings only once */
        for (i = 0; i < _countof(EntriesList); ++i)
        {
            LoadStringW(Globals.hInstance, EntriesList[i].uNameId,
                        EntriesList[i].szResource, _countof(EntriesList[i].szResource));

            EntriesList[i].bState = FALSE;
        }
    }
    else
    {
        INT iItem;
        BOOL bIsRunning;
        WCHAR szFormat[MAX_BUFFER];

        /* Reset the listbox */
        SendMessageW(Globals.hListDlg, LB_RESETCONTENT, 0, 0);

        /* Add the utilities in the listbox */
        for (i = 0; i < _countof(EntriesList); ++i)
        {
            bIsRunning = IsProcessRunning(EntriesList[i].lpszProgram);
            EntriesList[i].bState = bIsRunning;

            /* Load the string and append the utility's name to the format */
            StringCchPrintfW(szFormat, _countof(szFormat),
                             (bIsRunning ? Globals.szRunning : Globals.szNotRunning),
                             EntriesList[i].szResource);

            /* Add the item in the listbox */
            iItem = (INT)SendMessageW(Globals.hListDlg, LB_ADDSTRING, 0, (LPARAM)szFormat);
            if (iItem != LB_ERR)
                SendMessageW(Globals.hListDlg, LB_SETITEMDATA, iItem, (LPARAM)&EntriesList[i]);
        }
    }
}

/**
 * @DlgInitHandler
 *
 * Function which processes several operations for WM_INITDIALOG.
 *
 * @param[in]   hDlg
 *     The handle object of the dialog.
 *
 * @return
 *     TRUE to inform the system that WM_INITDIALOG has been processed and
 *     that it should set the keyboard focus to the control.
 *
 */
BOOL DlgInitHandler(IN HWND hDlg)
{
    INT PosX, PosY;
    RECT rc;
    WCHAR szAboutDlg[MAX_BUFFER];
    WCHAR szAppPath[MAX_BUFFER];
    HMENU hSysMenu;

    /* Save the dialog handle */
    Globals.hMainDlg = hDlg;

    /* Center the dialog on the screen */
    GetWindowRect(hDlg, &rc);
    PosX = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
    PosY = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;
    SetWindowPos(hDlg, 0, PosX, PosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    /* Extract the icon resource from the executable process */
    GetModuleFileNameW(NULL, szAppPath, _countof(szAppPath));
    Globals.hIcon = ExtractIconW(Globals.hInstance, szAppPath, 0);

    /* Set the icon within the dialog's title bar */
    if (Globals.hIcon)
    {
        SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)Globals.hIcon);
    }

    /* Retrieve the system menu and append the "About" menu item onto it */
    hSysMenu = GetSystemMenu(hDlg, FALSE);
    if (hSysMenu != NULL)
    {
        if (LoadStringW(Globals.hInstance, IDM_ABOUT, szAboutDlg, _countof(szAboutDlg)))
        {
            AppendMenuW(hSysMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hSysMenu, MF_STRING, IDM_ABOUT, szAboutDlg);
        }
    }

    /* Get the dialog items, specifically the dialog list box, the Start and Stop buttons */
    Globals.hListDlg = GetDlgItem(hDlg, IDC_LISTBOX);
    Globals.hDlgCtlStart = GetDlgItem(hDlg, IDC_START);
    Globals.hDlgCtlStop = GetDlgItem(hDlg, IDC_STOP);

    /* Initialize the GUI listbox */
    InitUtilsList(TRUE);

    /* Set the selection to the first item */
    Globals.iSelectedIndex = 0;

    /* Refresh the list */
    ListBoxRefreshContents();

    /* Create a timer, we'll use it to control the state of our items in the listbox */
    Globals.iTimer = SetTimer(hDlg, 0, 400, NULL);

    return TRUE;
}

/**
 * @ShowAboutDlg
 *
 * Displays the Shell "About" dialog box.
 *
 * @param[in]   hDlgParent
 *     A handle to the parent dialog window.
 *
 * @return
 *     Nothing.
 *
 */
VOID ShowAboutDlg(IN HWND hDlgParent)
{
    WCHAR szApp[MAX_BUFFER];
    WCHAR szAuthors[MAX_BUFFER];

    LoadStringW(Globals.hInstance, IDS_APP_NAME, szApp, _countof(szApp));
    LoadStringW(Globals.hInstance, IDS_AUTHORS, szAuthors, _countof(szAuthors));

    ShellAboutW(hDlgParent, szApp, szAuthors, Globals.hIcon);
}

/**
 * @GroupBoxUpdateTitle
 *
 * Updates the title of the groupbox.
 *
 * @return
 *     Nothing.
 *
 */
VOID GroupBoxUpdateTitle(VOID)
{
    WCHAR szFormat[MAX_BUFFER];

    /* Format the string with the utility's name and set it to the listbox's title */
    StringCchPrintfW(szFormat, _countof(szFormat), Globals.szGrpBoxTitle, EntriesList[Globals.iSelectedIndex].szResource);
    SetWindowTextW(GetDlgItem(Globals.hMainDlg, IDC_GROUPBOX), szFormat);
}

/**
 * @UpdateUtilityState
 *
 * Checks the state of the given accessibility tool.
 *
 * @param[in]   bUtilState
 *     State condition (boolean TRUE: started / FALSE: stopped).
 *
 * @return
 *     Nothing.
 *
 */
VOID UpdateUtilityState(IN BOOL bUtilState)
{
    Button_Enable(Globals.hDlgCtlStart, !bUtilState);
    Button_Enable(Globals.hDlgCtlStop,   bUtilState);

    /* Update the groupbox's title based on the selected utility item */
    GroupBoxUpdateTitle();
}

/**
 * @ListBoxRefreshContents
 *
 * Handle the tasks on a periodic cycle. This function handles WM_TIMER message.
 *
 * @return
 *     Returns 0 to inform the system that WM_TIMER has been processed.
 *
 */
INT ListBoxRefreshContents(VOID)
{
    UINT i;
    INT iItem;
    BOOL bIsRunning;
    WCHAR szFormat[MAX_BUFFER];

    /* Disable listbox redraw */
    SendMessageW(Globals.hListDlg, WM_SETREDRAW, FALSE, 0);

    for (i = 0; i < _countof(EntriesList); ++i)
    {
        /* Check the utility's state */
        bIsRunning = IsProcessRunning(EntriesList[i].lpszProgram);
        if (bIsRunning != EntriesList[i].bState)
        {
            /* The utility's state has changed, save it */
            EntriesList[i].bState = bIsRunning;

            /* Update the corresponding item in the listbox */
            StringCchPrintfW(szFormat, _countof(szFormat),
                             (bIsRunning ? Globals.szRunning : Globals.szNotRunning),
                             EntriesList[i].szResource);

            SendMessageW(Globals.hListDlg, LB_DELETESTRING, (LPARAM)i, 0);
            iItem = SendMessageW(Globals.hListDlg, LB_INSERTSTRING, (LPARAM)i, (LPARAM)szFormat);
            if (iItem != LB_ERR)
                SendMessageW(Globals.hListDlg, LB_SETITEMDATA, iItem, (LPARAM)&EntriesList[i]);
        }
    }

    /* Re-enable listbox redraw */
    SendMessageW(Globals.hListDlg, WM_SETREDRAW, TRUE, 0);

    /*
     * Check the previously selected item. This will help us determine what
     * item has been selected and set its focus selection back. Furthermore, check
     * the state of each accessibility tool and enable/disable the buttons.
     */
    SendMessageW(Globals.hListDlg, LB_SETCURSEL, (WPARAM)Globals.iSelectedIndex, 0);
    UpdateUtilityState(EntriesList[Globals.iSelectedIndex].bState);

    return 0;
}

/**
 * @DlgProc
 *
 * Main dialog application procedure function.
 *
 * @param[in]   hDlg
 *     The handle object of the dialog.
 *
 * @param[in]   Msg
 *     Message events (in unsigned int).
 *
 * @param[in]   wParam
 *     Message parameter (in UINT_PTR).
 *
 * @param[in]   lParam
 *     Message parameter (in LONG_PTR).
 *
 * @return
 *     Returns 0 to inform the system that the procedure has been handled.
 *
 */
INT_PTR APIENTRY DlgProc(
    IN HWND hDlg,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    switch (Msg)
    {
        case WM_INITDIALOG:
            DlgInitHandler(hDlg);
            return TRUE;

        case WM_CLOSE:
            KillTimer(hDlg, Globals.iTimer);
            DestroyIcon(Globals.hIcon);
            EndDialog(hDlg, FALSE);
            break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_OK:
                case IDC_CANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDC_LISTBOX:
                {
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        {
                            /* Retrieve the index of the current selected item */
                            INT iIndex = SendMessageW(Globals.hListDlg, LB_GETCURSEL, 0, 0);
                            if ((iIndex == LB_ERR) || (iIndex >= _countof(EntriesList)))
                                break;

                            /* Assign the selected index and check the utility's state */
                            Globals.iSelectedIndex = iIndex;
                            UpdateUtilityState(EntriesList[Globals.iSelectedIndex].bState);
                            break;
                        }
                        break;
                    }
                    break;
                }

                case IDC_START:
                    LaunchProcess(EntriesList[Globals.iSelectedIndex].lpszProgram);
                    break;

                case IDC_STOP:
                    CloseProcess(EntriesList[Globals.iSelectedIndex].lpszProgram);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_TIMER:
            ListBoxRefreshContents();
            return 0;

        case WM_SYSCOMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDM_ABOUT:
                    ShowAboutDlg(hDlg);
                    break;
            }
            break;
        }
    }

    return 0;
}

/**
 * @UManStartDlg
 *
 * Executes the dialog initialization mechanism and starts Utility Manager.
 * The function is exported for use by the main process.
 *
 * @return
 *     Returns TRUE when the operation has succeeded, FALSE otherwise.
 *
 */
BOOL WINAPI UManStartDlg(VOID)
{
    HANDLE hMutex;
    DWORD dwError;
    INITCOMMONCONTROLSEX iccex;

    /* Create a mutant object for the program. */
    hMutex = CreateMutexW(NULL, FALSE, L"Utilman");
    if (hMutex)
    {
        /* Check if there's already a mutex for the program */
        dwError = GetLastError();
        if (dwError == ERROR_ALREADY_EXISTS)
        {
            /*
                The program's instance is already here. That means
                the program is running and we should not set a new instance
                and mutex object.
            */
            CloseHandle(hMutex);
            return FALSE;
        }
    }

    /* Load the common controls for the program */
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&iccex);

    LoadStringW(Globals.hInstance, IDS_RUNNING,
                Globals.szRunning, _countof(Globals.szRunning));
    LoadStringW(Globals.hInstance, IDS_NOTRUNNING,
                Globals.szNotRunning, _countof(Globals.szNotRunning));
    LoadStringW(Globals.hInstance, IDS_GROUPBOX_OPTIONS_TITLE,
                Globals.szGrpBoxTitle, _countof(Globals.szGrpBoxTitle));

    /* Initialize the list of accessibility utilities */
    InitUtilsList(FALSE);

    /* Create the dialog box of the program */
    DialogBoxW(Globals.hInstance,
               MAKEINTRESOURCEW(IDD_MAIN_DIALOG),
               GetDesktopWindow(),
               DlgProc);

    /* Delete the mutex */
    if (hMutex)
    {
        CloseHandle(hMutex);
    }

    return TRUE;
}

/**
 * @DllMain
 *
 * Core routine of the Utility Manager's library.
 *
 * @param[in]   hDllInstance
 *      The entry point instance of the library.
 *
 * @param[in]   fdwReason
 *      The reason argument to indicate the motive DllMain
 *      is being called.
 *
 * @param[in]   lpvReserved
 *      Reserved.
 *
 * @return
 *     Returns TRUE when main call initialization has succeeded, FALSE
 *     otherwise.
 *
 */
BOOL WINAPI DllMain(IN HINSTANCE hDllInstance,
                    IN DWORD fdwReason,
                    IN LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            /* We don't care for DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications */
            DisableThreadLibraryCalls(hDllInstance);

            /* Initialize the globals */
            ZeroMemory(&Globals, sizeof(Globals));
            Globals.hInstance = hDllInstance;
            break;
        }

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
