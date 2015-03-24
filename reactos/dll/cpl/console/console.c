/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/console.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>

INT_PTR CALLBACK OptionsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FontProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LayoutProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ColorsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HINSTANCE hApplet = NULL;
BOOLEAN AppliedConfig = FALSE;

/* Local copy of the console informations */
PCONSOLE_STATE_INFO ConInfo = NULL;

static VOID
InitPropSheetPage(PROPSHEETPAGEW *psp,
                  WORD idDlg,
                  DLGPROC DlgProc)
{
    ZeroMemory(psp, sizeof(*psp));
    psp->dwSize      = sizeof(*psp);
    psp->dwFlags     = PSP_DEFAULT;
    psp->hInstance   = hApplet;
    psp->pszTemplate = MAKEINTRESOURCEW(idDlg);
    psp->pfnDlgProc  = DlgProc;
    psp->lParam      = 0;
}

static VOID
InitDefaultConsoleInfo(PCONSOLE_STATE_INFO pConInfo)
{
    /* FIXME: Get also the defaults from the registry */
    ConCfgInitDefaultSettings(pConInfo);
}

static INT_PTR
CALLBACK
ApplyProc(HWND hwndDlg,
          UINT uMsg,
          WPARAM wParam,
          LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            CheckDlgButton(hwndDlg, IDC_RADIO_APPLY_CURRENT, BST_CHECKED);
            return TRUE;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK)
            {
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_APPLY_CURRENT) == BST_CHECKED)
                    EndDialog(hwndDlg, IDC_RADIO_APPLY_CURRENT);
                else
                    EndDialog(hwndDlg, IDC_RADIO_APPLY_ALL);
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, IDCANCEL);
            }
            break;
        }
        default:
            break;
    }

    return FALSE;
}

BOOL
ApplyConsoleInfo(HWND hwndDlg,
                 PCONSOLE_STATE_INFO pConInfo)
{
    BOOL SetParams  = FALSE;
    BOOL SaveParams = FALSE;

    /*
     * If we are setting the default parameters, just save them,
     * otherwise display the save-confirmation dialog.
     */
    if (pConInfo->hWnd == NULL)
    {
        SetParams  = FALSE;
        SaveParams = TRUE; // FIXME: What happens if one clicks on CANCEL??
    }
    else
    {
        INT_PTR res = DialogBoxW(hApplet, MAKEINTRESOURCEW(IDD_APPLYOPTIONS), hwndDlg, ApplyProc);

        SetParams  = (res != IDCANCEL);
        SaveParams = (res == IDC_RADIO_APPLY_ALL);

        if (SetParams == FALSE)
        {
            /* Don't destroy when user presses cancel */
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            // return TRUE;
        }
    }

    if (SetParams)
    {
        HANDLE hSection;
        PCONSOLE_STATE_INFO pSharedInfo;

        /*
         * Create a memory section to share with CONSRV, and map it.
         */
        hSection = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                      NULL,
                                      PAGE_READWRITE,
                                      0,
                                      pConInfo->cbSize,
                                      NULL);
        if (!hSection)
        {
            DPRINT1("Error when creating file mapping, error = %d\n", GetLastError());
            return FALSE;
        }

        pSharedInfo = MapViewOfFile(hSection, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!pSharedInfo)
        {
            DPRINT1("Error when mapping view of file, error = %d\n", GetLastError());
            CloseHandle(hSection);
            return FALSE;
        }

        /* We are applying the chosen configuration */
        AppliedConfig = TRUE;

        /* Copy the console information into the section */
        RtlCopyMemory(pSharedInfo, pConInfo, pConInfo->cbSize);

        /* Unmap it */
        UnmapViewOfFile(pSharedInfo);

        /* Signal to CONSRV that it can apply the new configuration */
        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
        SendMessage(pConInfo->hWnd,
                    WM_SETCONSOLEINFO,
                    (WPARAM)hSection, 0);

        /* Close the section and return */
        CloseHandle(hSection);
    }

    if (SaveParams)
    {
        ConCfgWriteUserSettings(pConInfo);
    }

    return TRUE;
}

/* First Applet */
static LONG
APIENTRY
InitApplet(HANDLE hSectionOrWnd)
{
    INT_PTR Result;
    PCONSOLE_STATE_INFO pSharedInfo = NULL;
    WCHAR szTitle[MAX_PATH + 1];
    PROPSHEETPAGEW psp[4];
    PROPSHEETHEADERW psh;
    INT i = 0;

    /*
     * Because of Windows compatibility, we need to behave the same concerning
     * information sharing with CONSRV. For some obscure reason the designers
     * decided to use the CPlApplet hWnd parameter as being either a handle to
     * the applet's parent caller's window (in case we ask for displaying
     * the global console settings), or a handle to a shared section holding
     * a CONSOLE_STATE_INFO structure (they don't use the extra l/wParams).
     */

    /*
     * Try to open the shared section via the handle parameter. If we succeed,
     * it means we were called by CONSRV for retrieving/setting parameters for
     * a given console. If we fail, it means we are retrieving/setting default
     * global parameters (and we were either called by CONSRV or directly by
     * the user via the Control Panel, etc...)
     */
    pSharedInfo = MapViewOfFile(hSectionOrWnd, FILE_MAP_READ, 0, 0, 0);
    if (pSharedInfo != NULL)
    {
        /*
         * We succeeded. We were called by CONSRV and are retrieving
         * parameters for a given console.
         */

        /* Copy the shared data into our allocated buffer */
        DPRINT1("pSharedInfo->cbSize == %lu ; sizeof(CONSOLE_STATE_INFO) == %u\n",
                pSharedInfo->cbSize, sizeof(CONSOLE_STATE_INFO));
        ASSERT(pSharedInfo->cbSize >= sizeof(CONSOLE_STATE_INFO));

        /* Allocate a local buffer to hold console information */
        ConInfo = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            pSharedInfo->cbSize);
        if (ConInfo)
            RtlCopyMemory(ConInfo, pSharedInfo, pSharedInfo->cbSize);

        /* Close the section */
        UnmapViewOfFile(pSharedInfo);
        CloseHandle(hSectionOrWnd);

        if (!ConInfo) return 0;
    }
    else
    {
        /*
         * We failed. We are retrieving the default global parameters.
         */

        /* Allocate a local buffer to hold console information */
        ConInfo = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(CONSOLE_STATE_INFO));
        if (!ConInfo) return 0;

        /*
         * Setting the console window handle to NULL indicates we are
         * retrieving/setting the default console parameters.
         */
        ConInfo->hWnd = NULL;
        ConInfo->ConsoleTitle[0] = UNICODE_NULL;

        /* Use defaults */
        InitDefaultConsoleInfo(ConInfo);
    }

    /* Initialize the property sheet structure */
    ZeroMemory(&psh, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE | /* PSH_USEHICON */ PSH_USEICONID | PSH_NOAPPLYNOW;

    if (ConInfo->ConsoleTitle[0] != UNICODE_NULL)
    {
        wcsncpy(szTitle, L"\"", MAX_PATH);
        wcsncat(szTitle, ConInfo->ConsoleTitle, MAX_PATH - wcslen(szTitle));
        wcsncat(szTitle, L"\"", MAX_PATH - wcslen(szTitle));
    }
    else
    {
        wcscpy(szTitle, L"ReactOS Console");
    }
    psh.pszCaption = szTitle;

    if (/* pSharedInfo != NULL && */ ConInfo->hWnd != NULL)
    {
        /* We were started from a console window: this is our parent. */
        psh.hwndParent = ConInfo->hWnd;
    }
    else
    {
        /* We were started in another way (--> default parameters). Caller's window is our parent. */
        psh.hwndParent = (HWND)hSectionOrWnd;
    }

    psh.hInstance = hApplet;
    // psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCEW(IDC_CPLICON));
    psh.pszIcon = MAKEINTRESOURCEW(IDC_CPLICON);
    psh.nPages = ARRAYSIZE(psp);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[i++], IDD_PROPPAGEOPTIONS, OptionsProc);
    InitPropSheetPage(&psp[i++], IDD_PROPPAGEFONT   , FontProc   );
    InitPropSheetPage(&psp[i++], IDD_PROPPAGELAYOUT , LayoutProc );
    InitPropSheetPage(&psp[i++], IDD_PROPPAGECOLORS , ColorsProc );

    Result = PropertySheetW(&psh);

    /* Cleanup */
    HeapFree(GetProcessHeap(), 0, ConInfo);
    ConInfo = NULL;

    return (Result != -1);
}

/* Control Panel Callback */
LONG
CALLBACK
CPlApplet(HWND hwndCPl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_EXIT:
            // TODO: Free allocated memory
            break;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
        {
            CPLINFO *CPlInfo = (CPLINFO*)lParam2;
            CPlInfo->idIcon  = IDC_CPLICON;
            CPlInfo->idName  = IDS_CPLNAME;
            CPlInfo->idInfo  = IDS_CPLDESCRIPTION;
            break;
        }

        case CPL_DBLCLK:
            InitApplet((HANDLE)hwndCPl);
            break;
    }

    return FALSE;
}

INT
WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD     dwReason,
        LPVOID    lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hApplet = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
