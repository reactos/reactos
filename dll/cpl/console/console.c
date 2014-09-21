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

#define NUM_APPLETS 1

LONG APIENTRY InitApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FontProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LayoutProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ColorsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, InitApplet}
};

/*
 * Default 16-color palette for foreground and background
 * (corresponding flags in comments).
 */
const COLORREF s_Colors[16] =
{
    RGB(0, 0, 0),       // (Black)
    RGB(0, 0, 128),     // BLUE
    RGB(0, 128, 0),     // GREEN
    RGB(0, 128, 128),   // BLUE  | GREEN
    RGB(128, 0, 0),     // RED
    RGB(128, 0, 128),   // BLUE  | RED
    RGB(128, 128, 0),   // GREEN | RED
    RGB(192, 192, 192), // BLUE  | GREEN | RED

    RGB(128, 128, 128), // (Grey)  INTENSITY
    RGB(0, 0, 255),     // BLUE  | INTENSITY
    RGB(0, 255, 0),     // GREEN | INTENSITY
    RGB(0, 255, 255),   // BLUE  | GREEN | INTENSITY
    RGB(255, 0, 0),     // RED   | INTENSITY
    RGB(255, 0, 255),   // BLUE  | RED   | INTENSITY
    RGB(255, 255, 0),   // GREEN | RED   | INTENSITY
    RGB(255, 255, 255)  // BLUE  | GREEN | RED | INTENSITY
};
/* Default attributes */
#define DEFAULT_SCREEN_ATTRIB   (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
#define DEFAULT_POPUP_ATTRIB    (FOREGROUND_BLUE | FOREGROUND_RED | \
                                 BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)
/* Cursor size */
#define CSR_DEFAULT_CURSOR_SIZE 25

static VOID
InitPropSheetPage(PROPSHEETPAGEW *psp,
                  WORD idDlg,
                  DLGPROC DlgProc,
                  LPARAM lParam)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGEW));
    psp->dwSize = sizeof(PROPSHEETPAGEW);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCEW(idDlg);
    psp->pfnDlgProc = DlgProc;
    psp->lParam = lParam;
}

PCONSOLE_PROPS
AllocConsoleInfo()
{
    /* Adapted for holding GUI terminal information */
    return HeapAlloc(GetProcessHeap(),
                     HEAP_ZERO_MEMORY,
                     sizeof(CONSOLE_PROPS) + sizeof(GUI_CONSOLE_INFO));
}

VOID
InitConsoleDefaults(PCONSOLE_PROPS pConInfo)
{
    PGUI_CONSOLE_INFO GuiInfo = NULL;

    /* FIXME: Get also the defaults from the registry */

    /* Initialize the default properties */
    pConInfo->ci.HistoryBufferSize = 50;
    pConInfo->ci.NumberOfHistoryBuffers = 4;
    pConInfo->ci.HistoryNoDup = FALSE;
    pConInfo->ci.QuickEdit = FALSE;
    pConInfo->ci.InsertMode = TRUE;
    // pConInfo->ci.InputBufferSize;
    pConInfo->ci.ScreenBufferSize.X = 80;
    pConInfo->ci.ScreenBufferSize.Y = 300;
    pConInfo->ci.ConsoleSize.X = 80;
    pConInfo->ci.ConsoleSize.Y = 25;
    pConInfo->ci.CursorBlinkOn = TRUE;
    pConInfo->ci.ForceCursorOff = FALSE;
    pConInfo->ci.CursorSize = CSR_DEFAULT_CURSOR_SIZE;
    pConInfo->ci.ScreenAttrib = DEFAULT_SCREEN_ATTRIB;
    pConInfo->ci.PopupAttrib  = DEFAULT_POPUP_ATTRIB;
    pConInfo->ci.CodePage = 0;
    pConInfo->ci.ConsoleTitle[0] = L'\0';

    /* Adapted for holding GUI terminal information */
    pConInfo->TerminalInfo.Size = sizeof(GUI_CONSOLE_INFO);
    GuiInfo = pConInfo->TerminalInfo.TermInfo = (PGUI_CONSOLE_INFO)(pConInfo + 1);
    wcsncpy(GuiInfo->FaceName, L"VGA", LF_FACESIZE); // HACK: !!
    // GuiInfo->FaceName[0] = L'\0';
    GuiInfo->FontFamily = FF_DONTCARE;
    GuiInfo->FontSize.X = 0;
    GuiInfo->FontSize.Y = 0;
    GuiInfo->FontWeight = FW_NORMAL; // HACK: !!
    // GuiInfo->FontWeight = FW_DONTCARE;

    GuiInfo->FullScreen   = FALSE;
    GuiInfo->ShowWindow   = SW_SHOWNORMAL;
    GuiInfo->AutoPosition = TRUE;
    GuiInfo->WindowOrigin.x = 0;
    GuiInfo->WindowOrigin.y = 0;

    memcpy(pConInfo->ci.Colors, s_Colors, sizeof(s_Colors));
}

INT_PTR
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
                 PCONSOLE_PROPS pConInfo)
{
    BOOL SetParams  = FALSE;
    BOOL SaveParams = FALSE;

    /*
     * If we are setting the default parameters, just save them,
     * otherwise display the save-confirmation dialog.
     */
    if (pConInfo->ShowDefaultParams)
    {
        SetParams  = TRUE;
        SaveParams = TRUE;
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
        PCONSOLE_PROPS pSharedInfo;

        /*
         * Create a memory section to share with the server, and map it.
         */
        /* Holds data for console.dll + console info + terminal-specific info */
        hSection = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                      NULL,
                                      PAGE_READWRITE,
                                      0,
                                      sizeof(CONSOLE_PROPS) + sizeof(GUI_CONSOLE_INFO),
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
        pConInfo->AppliedConfig = TRUE;

        /*
         * Copy the console information into the section and
         * offsetize the address of terminal-specific information.
         * Do not perform the offsetization in pConInfo as it is
         * likely to be reused later on. Instead, do it in pSharedInfo
         * after having copied all the data.
         */
        RtlCopyMemory(pSharedInfo, pConInfo, sizeof(CONSOLE_PROPS) + sizeof(GUI_CONSOLE_INFO));
        pSharedInfo->TerminalInfo.TermInfo = (PVOID)((ULONG_PTR)pConInfo->TerminalInfo.TermInfo - (ULONG_PTR)pConInfo);

        /* Unmap it */
        UnmapViewOfFile(pSharedInfo);

        /* Signal to the console server that it can apply the new configuration */
        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
        SendMessage(pConInfo->hConsoleWindow,
                    PM_APPLY_CONSOLE_INFO,
                    (WPARAM)hSection,
                    (LPARAM)SaveParams);

        /* Close the section and return */
        CloseHandle(hSection);
    }

    return TRUE;
}

/* First Applet */
LONG APIENTRY
InitApplet(HWND hWnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    HANDLE hSection = (HANDLE)wParam;
    PCONSOLE_PROPS pSharedInfo = NULL;
    PCONSOLE_PROPS pConInfo;
    WCHAR szTitle[MAX_PATH + 1];
    PROPSHEETPAGEW psp[4];
    PROPSHEETHEADERW psh;
    INT i = 0;

    UNREFERENCED_PARAMETER(uMsg);

    /*
     * CONSOLE.DLL shares information with CONSRV with wParam:
     * wParam is a handle to a shared section holding a CONSOLE_PROPS struct.
     *
     * NOTE: lParam is not used.
     */

    /* Allocate a local buffer to hold console information */
    pConInfo = AllocConsoleInfo();
    if (!pConInfo) return 0;

    /* Check whether we were launched from the terminal... */
    if (hSection != NULL)
    {
        /* ... yes, map the shared section */
        pSharedInfo = MapViewOfFile(hSection, FILE_MAP_READ, 0, 0, 0);
        if (pSharedInfo == NULL)
        {
            /* Cleanup */
            HeapFree(GetProcessHeap(), 0, pConInfo);

            /* Close the section */
            CloseHandle(hSection);

            return 0;
        }

        /* Find the console window and whether we set the default parameters */
        pConInfo->hConsoleWindow    = pSharedInfo->hConsoleWindow;
        pConInfo->ShowDefaultParams = pSharedInfo->ShowDefaultParams;
    }
    else
    {
        /* ... no, we were launched as a CPL. Display the default settings. */
        pConInfo->ShowDefaultParams = TRUE;
    }

    if (pConInfo->ShowDefaultParams)
    {
        /* Use defaults */
        InitConsoleDefaults(pConInfo);
    }
    else if (hSection && pSharedInfo)
    {
        /*
         * Copy the shared data into our allocated buffer, and
         * de-offsetize the address of terminal-specific information.
         */

        /* Check that we are really going to modify GUI terminal information */
        // FIXME: Do something clever, for example copy the UI-independent part
        // and init the UI-dependent part to some default values...
        ASSERT(pSharedInfo->TerminalInfo.Size == sizeof(GUI_CONSOLE_INFO));
        ASSERT(pSharedInfo->TerminalInfo.TermInfo);

        RtlCopyMemory(pConInfo, pSharedInfo, sizeof(CONSOLE_PROPS) + sizeof(GUI_CONSOLE_INFO));
        pConInfo->TerminalInfo.TermInfo = (PVOID)((ULONG_PTR)pConInfo + (ULONG_PTR)pConInfo->TerminalInfo.TermInfo);
    }

    if (hSection && pSharedInfo)
    {
        /* Close the section */
        UnmapViewOfFile(pSharedInfo);
        CloseHandle(hSection);
    }

    /* Initialize the property sheet structure */
    ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE | /* PSH_USEHICON */ PSH_USEICONID | PSH_NOAPPLYNOW;

    if (pConInfo->ci.ConsoleTitle[0] != L'\0')
    {
        wcsncpy(szTitle, L"\"", MAX_PATH);
        wcsncat(szTitle, pConInfo->ci.ConsoleTitle, MAX_PATH - wcslen(szTitle));
        wcsncat(szTitle, L"\"", MAX_PATH - wcslen(szTitle));
    }
    else
    {
        wcscpy(szTitle, L"ReactOS Console");
    }
    psh.pszCaption = szTitle;

    psh.hwndParent = pConInfo->hConsoleWindow;
    psh.hInstance = hApplet;
    // psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCEW(IDC_CPLICON));
    psh.pszIcon = MAKEINTRESOURCEW(IDC_CPLICON);
    psh.nPages = 4;
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[i++], IDD_PROPPAGEOPTIONS, (DLGPROC) OptionsProc, (LPARAM)pConInfo);
    InitPropSheetPage(&psp[i++], IDD_PROPPAGEFONT   , (DLGPROC) FontProc   , (LPARAM)pConInfo);
    InitPropSheetPage(&psp[i++], IDD_PROPPAGELAYOUT , (DLGPROC) LayoutProc , (LPARAM)pConInfo);
    InitPropSheetPage(&psp[i++], IDD_PROPPAGECOLORS , (DLGPROC) ColorsProc , (LPARAM)pConInfo);

    return (PropertySheetW(&psh) != -1);
}

/* Control Panel Callback */
LONG CALLBACK
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
            return NUM_APPLETS;

        case CPL_INQUIRE:
        {
            CPLINFO *CPlInfo = (CPLINFO*)lParam2;
            CPlInfo->idIcon  = Applets[0].idIcon;
            CPlInfo->idName  = Applets[0].idName;
            CPlInfo->idInfo  = Applets[0].idDescription;
            break;
        }

        case CPL_DBLCLK:
            InitApplet(hwndCPl, uMsg, lParam1, lParam2);
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
        case DLL_THREAD_ATTACH:
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}
