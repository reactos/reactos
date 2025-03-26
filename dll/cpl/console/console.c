/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/console/console.c
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

/* Local copy of the console information */
PCONSOLE_STATE_INFO ConInfo = NULL;
/* What to do with the console information */
static BOOL SetConsoleInfo  = FALSE;
static BOOL SaveConsoleInfo = FALSE;

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
    // FIXME: Also retrieve the value of REG_DWORD CurrentPage.
    ConCfgGetDefaultSettings(pConInfo);
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

VOID
ApplyConsoleInfo(HWND hwndDlg)
{
    static BOOL ConsoleInfoAlreadySaved = FALSE;

    /*
     * We already applied all the console properties (and saved if needed).
     * Nothing more needs to be done.
     */
    if (ConsoleInfoAlreadySaved)
        goto Done;

    /*
     * If we are setting the default parameters, just save them,
     * otherwise display the confirmation & apply dialog.
     */
    if (ConInfo->hWnd == NULL)
    {
        SetConsoleInfo  = FALSE;
        SaveConsoleInfo = TRUE;
    }
    else
    {
        INT_PTR res = DialogBoxW(hApplet, MAKEINTRESOURCEW(IDD_APPLYOPTIONS), hwndDlg, ApplyProc);

        SetConsoleInfo  = (res != IDCANCEL);
        SaveConsoleInfo = (res == IDC_RADIO_APPLY_ALL);

        if (!SetConsoleInfo)
        {
            /* Don't destroy when the user presses cancel */
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            return;
        }
    }

    /*
     * We applied all the console properties (and saved if needed).
     * Set the flag so that if this function is called again, we won't
     * need to redo everything again.
     */
    ConsoleInfoAlreadySaved = TRUE;

Done:
    /* Options have been applied */
    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
    return;
}

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDC_CPLICON));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
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

    /* Initialize the font support -- additional TrueType font cache and current preview font */
    InitTTFontCache();
    RefreshFontPreview(&FontPreview, ConInfo);

    /* Initialize the property sheet structure */
    ZeroMemory(&psh, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE | /* PSH_USEHICON | */ PSH_USEICONID | PSH_NOAPPLYNOW | PSH_USECALLBACK;

    if (ConInfo->ConsoleTitle[0] != UNICODE_NULL)
    {
        StringCchPrintfW(szTitle, ARRAYSIZE(szTitle), L"\"%s\"", ConInfo->ConsoleTitle);
        psh.pszCaption = szTitle;
    }
    else
    {
        psh.pszCaption = MAKEINTRESOURCEW(IDS_CPLNAME);
    }

    if (pSharedInfo != NULL)
    {
        /* We were started from a console window: this is our parent (or ConInfo->hWnd is NULL) */
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
    psh.pfnCallback = PropSheetProc;

    InitPropSheetPage(&psp[i++], IDD_PROPPAGEOPTIONS, OptionsProc);
    InitPropSheetPage(&psp[i++], IDD_PROPPAGEFONT   , FontProc   );
    InitPropSheetPage(&psp[i++], IDD_PROPPAGELAYOUT , LayoutProc );
    InitPropSheetPage(&psp[i++], IDD_PROPPAGECOLORS , ColorsProc );

    /* Display the property sheet */
    RegisterWinPrevClass(hApplet);
    Result = PropertySheetW(&psh);
    UnRegisterWinPrevClass(hApplet);

    /* Clear the font support */
    ResetFontPreview(&FontPreview);
    ClearTTFontCache();

    /* Apply the console settings if necessary */
    if (SetConsoleInfo)
    {
        HANDLE hSection;

        /*
         * Create a memory section to share with CONSRV, and map it.
         */
        hSection = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                      NULL,
                                      PAGE_READWRITE,
                                      0,
                                      ConInfo->cbSize,
                                      NULL);
        if (!hSection)
        {
            DPRINT1("Error when creating file mapping, error = %d\n", GetLastError());
            goto Quit;
        }

        pSharedInfo = MapViewOfFile(hSection, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!pSharedInfo)
        {
            DPRINT1("Error when mapping view of file, error = %d\n", GetLastError());
            CloseHandle(hSection);
            goto Quit;
        }

        /* Copy the console information into the section and unmap it */
        RtlCopyMemory(pSharedInfo, ConInfo, ConInfo->cbSize);
        UnmapViewOfFile(pSharedInfo);

        /*
         * Signal to CONSRV that it can apply the new settings.
         *
         * NOTE: SetConsoleInfo set to TRUE by ApplyConsoleInfo()
         * *only* when ConInfo->hWnd != NULL and the user did not
         * press IDCANCEL in the confirmation dialog.
         */
        ASSERT(ConInfo->hWnd);
        SendMessageW(ConInfo->hWnd, WM_SETCONSOLEINFO, (WPARAM)hSection, 0);

        /* Close the section and return */
        CloseHandle(hSection);
    }

    /* Save the console settings */
    if (SaveConsoleInfo)
    {
        /* Default settings saved when ConInfo->hWnd == NULL */
        ConCfgWriteUserSettings(ConInfo, ConInfo->hWnd == NULL);
    }

Quit:
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
