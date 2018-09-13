#include "precomp.hxx"
#pragma hdrstop



HRESULT GetShell32Version(LPDWORD pdwMajor, LPDWORD pdwMinor);



struct BAUD_RATES {
    DWORD   m_dwBitFlag;
    PSTR    m_pszName;
    DWORD   m_dwBaudRate;
    float   m_eBaudRate;
};                                          
                                            
BAUD_RATES rgBaudRates[] = {                
    { BAUD_075,     "75",       75,         75 },
    { BAUD_110,     "110",      110,        110 },
    { BAUD_134_5,   "134.5",    134,        134.5 },
    { BAUD_150,     "150",      150,        150 },
    { BAUD_300,     "300",      300,        300 },
    { BAUD_600,     "600",      600,        600 },        
    { BAUD_1200,    "1200",     1200,       1200 },
    { BAUD_1800,    "1800",     1800,       1800 },
    { BAUD_2400,    "2400",     2400,       2400 },
    { BAUD_4800,    "4800",     4800,       4800 },
    { BAUD_7200,    "7200",     7200,       7200 },
    { BAUD_9600,    "9600",     9600,       9600 },
    { BAUD_14400,   "14400",    14400,      14400 },
    { BAUD_19200,   "19200",    19200,      19200 },
    { BAUD_38400,   "38400",    38400,      38400 },
    { BAUD_56K,     "56K",      56 * 1024,  56 * 1024 },
    { BAUD_128K,    "128K",     128 * 1024, 128 * 1024 },
    { BAUD_115200,  "115200",   115200,     115200 },
    { BAUD_57600,   "57600",    57600,      57600 },
    { BAUD_USER,    WKSP_DynaLoadString(g_hInst, IDS_USER_DEFINED_BAUD_RATE), 0, 0 }
};



VOID
SetControlFont(
    IN HFONT    hFont, 
    IN HWND     hDlg, 
    IN INT      nId
    )
{
    if ( hFont ) {
        HWND hwndControl = GetDlgItem(hDlg, nId);

        if ( hwndControl ) {
            SetWindowFont(hwndControl, hFont, TRUE);
        }
    }
}

VOID 
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    )
{
    //
    // Create the fonts we need based on the dialog font
    //
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
    LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
    // Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
    BoldLogFont.lfWeight      = FW_BOLD;

    TCHAR FontSizeString[_MAX_PATH];
    INT FontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if (!LoadString(hInstance,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE))  {
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"));
    }

    if (LoadString(hInstance,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR)))  {
        FontSize = _tcstoul( FontSizeString, NULL, 10 );
    } else {
        FontSize = 12;
    }

    HDC hdc = GetDC( hwnd );

    if ( hdc ) {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

        *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
        *pBoldFont    = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);
    }
}

VOID 
DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    )
{
    if ( hBigBoldFont ) {
        DeleteObject( hBigBoldFont );
    }

    if ( hBoldFont ) {
        DeleteObject( hBoldFont );
    }
}


int
CALLBACK 
WizardStub_DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{   
    extern PAGEID g_nNewPageIdx;

    PAGE_DEF * pPageDef = (PAGE_DEF *) GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pPageDef) {
        Assert(FIRST_PAGEID <= g_nNewPageIdx);
        Assert(g_nNewPageIdx < MAX_NUM_PAGEID);
        Assert(g_rgpPageDefs[g_nNewPageIdx]);    
        Assert(g_nNewPageIdx == g_rgpPageDefs[g_nNewPageIdx]->m_nPageId);

        pPageDef = g_rgpPageDefs[g_nNewPageIdx];
        Assert(pPageDef);
    
        Assert(NULL == pPageDef->m_hDlg);
        pPageDef->m_hDlg = hDlg;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pPageDef);
        g_nNewPageIdx = NULL_PAGEID;
    }

    Assert(pPageDef);


    if (g_bRunning_IE_3) {
        if (WM_INITDIALOG == uMsg) {
            PSTR psz;
            
            // Set header
            psz = WKSP_DynaLoadString(g_hInst, pPageDef->m_nHeaderTitleStrId);
            SetDlgItemText(hDlg, IDC_STXT_HDR, psz);
            free(psz);

            // Set sub-header
            psz = WKSP_DynaLoadString(g_hInst, pPageDef->m_nHeaderSubTitleStrId);
            SetDlgItemText(hDlg, IDC_STXT_SUB_HDR, psz);
            free(psz);
        }
    }

    return pPageDef->DlgProc(hDlg, uMsg, wParam, lParam);
}


//
// Create the wizard pages and display the UI.
//

BOOL
DoWizard(
    HINSTANCE   hInstance           
  )
{
    UINT            i                           = 0;
    BOOL            bStatus                     = TRUE;
    PROPSHEETHEADER psh                         = {0};
    HPROPSHEETPAGE  rghpsp[1]                   = {0};

    Assert(FIRST_PAGEID == 0);

    for (i = 0; i < MAX_NUM_PAGEID; i++) {
        Assert(g_rgpPageDefs[i]);
        Assert(PAGEID(i) == g_rgpPageDefs[i]->m_nPageId);
        g_rgpPageDefs[i]->CreatePropPage();
    }
    rghpsp[0] = g_rgpPageDefs[0]->m_hpsp;
    Assert(rghpsp[0]);
    
    if (g_bRunning_IE_3) {
        psh.dwSize = sizeof(IE_3_PROP_SHEET);
        psh.dwFlags = PSH_WIZARD;
    } else {
        psh.dwSize = sizeof( psh );
#if (_WIN32_IE >= 0x0400)
        psh.dwFlags = PSH_WIZARD | PSH_WIZARD97;
#else
        psh.dwFlags = PSH_WIZARD;
#endif
                                    //| PSH_WATERMARK; // | PSH_HEADER;
        //psh.pszbmWatermark      = MAKEINTRESOURCE(IDB_WATERMARK);
        //psh.pszbmHeader         = MAKEINTRESOURCE(IDB_BANNER);
    }
                    
    psh.hInstance           = hInstance;
    psh.hwndParent          = NULL;
    psh.pszCaption          = NULL;
    psh.phpage              = rghpsp;
    psh.nStartPage          = 0;
    // The other pages are added dynamically
    psh.nPages              = 1; 


    //
    // Create the bold fonts.
    // 
    SetupFonts( hInstance, NULL, &g_hBigBoldFont, &g_hBoldFont);

    //
    // Validate all the pages.
    //
    /*for ( i = 0; i < MAX_NUM_PAGEID; i++ ) {
        if ( rghpsp[i] == 0 ) {
            bStatus = FALSE;
        }
    }*/

    //
    // Display the wizard.
    //
    if ( bStatus ) {   
        if ( PropertySheet( &psh ) == -1 ) {
            bStatus = FALSE;
        }
    }

    if ( !bStatus ) {
        //
        // Manually destroy the pages if something failed.
        //
        for ( i = 0; i < psh.nPages; i++) {
            if ( rghpsp[i] ) {
                DestroyPropertySheetPage( rghpsp[i] );
            }
        }
    }

    //
    // Destroy the fonts that were created.
    //
    DestroyFonts( g_hBigBoldFont, g_hBoldFont );

    return bStatus;
}


int
WINAPI 
WinMain(
    HINSTANCE   hInstance,          
    HINSTANCE   hPrevInstance,      
    LPSTR       lpszCmdLine,        
    INT         nCmdShow            
    )   
{
    DWORD dwMajor;
    DWORD dwMinor;

    g_hInst = hInstance;

    // Get the shell version
    if (S_OK != GetShell32Version(&dwMajor, &dwMinor)) {
        PSTR pszError = WKSP_FormatLastErrorMessage();
        MessageBox(NULL, pszError, NULL, MB_OK | MB_ICONERROR | MB_TASKMODAL);
        free(pszError);
        return -1;
    }

    g_bRunning_IE_3 = (4 == dwMajor && 0 == dwMinor);


    if (!InitPageDefs()) {
        return -1;
    }

    return DoWizard(hInstance);
}


void
ShowAssert(
           LPSTR condition,
           UINT line,
           LPSTR file
           )
{
    static char text[8 * 1024] = {0};

    //Build line, show assertion and exit program

    sprintf(text, "Line:%u, File:%Fs, Condition:%Fs\nAbort - exit\nRetry - break\nIgnore - continue",
        line, file, condition);

    Assert(strlen(text) < sizeof(text));

    switch(MessageBox(NULL, text, NULL, MB_ABORTRETRYIGNORE | MB_TASKMODAL)) {
    case IDABORT:
        exit(3);
        break;
    
    case IDIGNORE:
        break;

    case IDRETRY:
    default:
        DebugBreak();
    }
}                                       /* ShowAssert() */



DWORD
GetNumPropSheetPages(
    HWND hwndPropSheet
    )
{
    Assert(hwndPropSheet);

    HWND hwndTabCtrl = (HWND) SendMessage(hwndPropSheet, PSM_GETTABCONTROL, 0, 0);
    Assert(hwndTabCtrl);

    DWORD dwNum = (DWORD)SendMessage(hwndTabCtrl, TCM_GETITEMCOUNT, 0, 0);
    Assert(dwNum);

    return dwNum;
}


PSTR 
BaudRateBitFlagToText(
    DWORD dwBitFlagBaudRate
    )
{
/*
    if (!rgBaudRates[ sizeof(rgBaudRates) / sizeof(BAUD_RATES)-1 ].m_pszName) {
        rgBaudRates[ sizeof(rgBaudRates) / sizeof(BAUD_RATES)-1 ].m_pszName 
        = WKSP_DynaLoadString(g_hInst, IDS_USER_DEFINED_BAUD_RATE);
    }
*/

    for (int i=0; i < sizeof(rgBaudRates)/sizeof(BAUD_RATES); i++) {
        if (dwBitFlagBaudRate == rgBaudRates[i].m_dwBitFlag) {
            return rgBaudRates[i].m_pszName;
        }
    }

    Assert(!"Not supposed to happen");
    return NULL;
}


DWORD
BaudRateTextToBitFlag(
    PSTR pszBaudRate
    )
{
    for (int i=0; i < sizeof(rgBaudRates)/sizeof(BAUD_RATES); i++) {
        if (!strcmp(pszBaudRate, rgBaudRates[i].m_pszName)) {
            return rgBaudRates[i].m_dwBitFlag;
        }
    }

    Assert(!"Not supposed to happen");
    return 0;
}


HRESULT 
GetShell32Version(
    LPDWORD pdwMajor, 
    LPDWORD pdwMinor
    )
{
    Assert(pdwMajor);
    Assert(pdwMinor);
    
    HINSTANCE   hShell32;
    
    *pdwMajor = 0;
    *pdwMinor = 0;
    
    //Load the DLL.
    hShell32 = LoadLibrary(TEXT("shell32.dll"));
    if(hShell32) {
        HRESULT           hr = S_OK;   
        DLLGETVERSIONPROC pDllGetVersion;      
        // You must get this function explicitly because earlier versions of the DLL 
        // don't implement this function. That makes the lack of implementation of the 
        // function a version marker in itself.
        
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hShell32, TEXT("DllGetVersion"));

        if(pDllGetVersion) {      
            DLLVERSIONINFO    dvi;      
            ZeroMemory(&dvi, sizeof(dvi));      
            dvi.cbSize = sizeof(dvi);   
            hr = (*pDllGetVersion)(&dvi);
            
            if(SUCCEEDED(hr)) {
                *pdwMajor = dvi.dwMajorVersion;
                *pdwMinor = dvi.dwMinorVersion;
            }      
        } else {      
        // If GetProcAddress failed, the DLL is a version previous to the one 
        // shipped with IE 3.x.
            *pdwMajor = 4;
            *pdwMinor = 0;
        }      
        FreeLibrary(hShell32);
        return hr;
    }
    return E_FAIL;
}
