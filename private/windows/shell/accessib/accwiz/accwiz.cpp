#include "pch.hxx" // PCH
#pragma hdrstop

#include "AccWiz.h"

#include "resource.h"

#include "pgfinish.h"
#include "pgGenric.h"

// Welcome page
#include "pgnWelCome.h"
#include "pgWelcom.h"
#include "pgWelco2.h"

// Options page
#include "pgWizOpt.h"

// Color pages
#include "pgLokPrv.h"
//#include "pgHghCon.h"

// Sound pages
//#include "pgSndSen.h"

// Keyboard pages
//#include "pgStkKey.h"
//#include "pgFltKey.h"
//#include "pgTglKey.h"
//#include "pgShwHlp.h"
//#include "pgSerKey.h"

// Mouse pages
//#include "pgMseKey.h"
#include "pgMseCur.h"
#include "pgMseBut.h"

// Text size pages
//#include "pgScrBar.h"
//#include "pgIconSz.h"
//#include "pgIcon2.h"


//#include "pgHotKey.h"
#include "pgTmeOut.h"
#include "pgSveDef.h"
#include "pgSveFil.h"

#include "pgExtras.h"



#include "LookPrev.h"
#include "Select.h"

// Link Window
// #include "shlobj.h"
// #include "shlobjp.h"
EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;

// Declaration of the global options variable
CAccWizOptions g_Options;
HINSTANCE g_hInstDll = NULL;    // DLL instance handle.

BOOL g_bHACKHACKSavedOptions = FALSE;
HANDLE              g_hAccwizRunning;

int WINAPI WinMain( 
				   HINSTANCE hInstance, // handle to current instance 
				   HINSTANCE hPrevInstance, // handle to previous instance 
				   LPSTR lpCmdLine, // pointer to command line 
				   int nCmdShow // show state of window 
				   )
{
	g_hInstDll = hInstance;
    
    SetLastError(0);
    // Allow only ONE instance of the program to run.
    // The mutex is automatically destroyed when Accwiz exits
    g_hAccwizRunning = CreateMutex(NULL, TRUE, TEXT("AK:AccwizRunning:KHALI"));
    if ( (g_hAccwizRunning == NULL) ||
        (GetLastError() == ERROR_ALREADY_EXISTS) )
    {
        return 0;
    }
	
	// Required for Link Window OLE marshalling :AK
	CoInitialize(NULL);

	// for the Link Window in finish page...
	LinkWindow_RegisterClass();

	VERIFY(CLookPrev::sm_Globals.Initialize()); // Make sure this has been initialized
	// VERIFY(CSelection::Initialize()); // Make sure this has been initialized: chnage this!

	// Get the commandline so that it works for MUI/Unicode
	LPTSTR lpCmdLineW = GetCommandLine();
  
	if ( *lpCmdLineW == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++lpCmdLineW && (*lpCmdLineW
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *lpCmdLineW == TEXT('\"') )
            lpCmdLineW++;
    }
    else {
        while (*lpCmdLineW > TEXT(' '))
            lpCmdLineW++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*lpCmdLineW && (*lpCmdLineW <= TEXT(' '))) {
        lpCmdLineW++;
    }

	if(NULL != lpCmdLineW && lstrlen(lpCmdLineW))
	{
		TCHAR szFileName[_MAX_PATH];
		lstrcpy(szFileName, lpCmdLineW);
		StrTrim(szFileName, TEXT("\"\0"));

		// Load the settings file back in.
		HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwRead;
			WIZSCHEME schemeTemp;
			ReadFile(hFile, (LPVOID)&schemeTemp, sizeof(schemeTemp), &dwRead, NULL);
			DWORD dwFileSize = GetFileSize(hFile, NULL);
			CloseHandle(hFile);
			if(sizeof(schemeTemp) != dwRead || sizeof(schemeTemp) != dwFileSize)
			{
				StringTableMessageBox(NULL, IDS_WIZERRORLOADINGFILETEXT, IDS_WIZERRORLOADINGFILETITLE, MB_OK);
				return 0;
			}

			// IMPORTANT: For loaded schemes, we always want to change to the windows default font
			schemeTemp.m_PortableNonClientMetrics.m_nFontFaces = 1;

			g_Options.m_schemePreview = schemeTemp;
			g_bHACKHACKSavedOptions = TRUE;
			g_Options.ApplyPreview();
		}
		else
		{
			StringTableMessageBox(NULL, IDS_WIZERRORLOADINGFILETEXT, IDS_WIZERRORLOADINGFILETITLE, MB_OK);
			return 0;
		}

	}

#ifdef UNICODE	
	AccWiz_RunDllW(NULL, hInstance, lpCmdLineW, nCmdShow);
#else
	AccWiz_RunDllA(NULL, hInstance, lpCmdLineW, nCmdShow);
#endif

	return 0;
}


HRESULT
CreateAndRunWizard(
				   HWND hwndParent);

HRESULT
CreateAndRunWizard2(
				   HWND hwndParent);

HRESULT
OnProcessAttach(
				HINSTANCE hInstDll);

HRESULT
OnProcessDetach(
				VOID);

INT
PropSheetCallback(
				  HWND hwnd,
				  UINT uMsg,
				  LPARAM lParam);


VOID WINAPI AccWiz_RunDllA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLineA, INT nCmdShow)
{
	if (NULL != pszCmdLineA)
	{
		LPWSTR pszCmdLineW = NULL;
		INT cchCmdLine = MultiByteToWideChar(CP_ACP,
											 0,
											 pszCmdLineA,
											 -1,
											 NULL,
											 0);
		pszCmdLineW = new WCHAR[cchCmdLine];
		if (NULL != pszCmdLineW)
		{
			MultiByteToWideChar(CP_ACP,
								0,
								pszCmdLineA,
								-1,
								pszCmdLineW,
								cchCmdLine);

			AccWiz_RunDllW(hwnd, hInstance, pszCmdLineW, nCmdShow);

			delete[] pszCmdLineW;
		}
	}
}


VOID WINAPI AccWiz_RunDllW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLineW, INT nCmdShow)
{
	HWND hwndParent   = GetDesktopWindow();

	HRESULT hResult;
	if(!g_bHACKHACKSavedOptions)
		hResult = CreateAndRunWizard(hwndParent);
	else
		hResult = CreateAndRunWizard2(hwndParent);


	if(!SUCCEEDED(hResult))
	{
		// TODO: Put out of memory message here
		_ASSERTE(FALSE);
#pragma message("Put Out of Memory message here")
	}
}



const INT MAX_PAGES  = 25;

HRESULT
CreateAndRunWizard(
				   HWND hwndParent)
{
	HRESULT hResult = E_OUTOFMEMORY;
	
	PROPSHEETPAGE psp[MAX_PAGES];
	WizardPage *rgpwp[MAX_PAGES];
	
	// Zero init the arrays
	memset(&psp, 0, sizeof(psp));
	memset(&rgpwp, 0, sizeof(rgpwp));
	
	// ///////////////////////
	// Create Pages Here - NOTE: Order does not matter - we'll control it with our own list
	//
	int nCountPages = 0;
	rgpwp[nCountPages++] = new CWizWelcomePg(psp + nCountPages);
	rgpwp[nCountPages++] = new CWelcomePg(psp + nCountPages);
//	rgpwp[nCountPages++] = new CGenericWizPg(psp + nCountPages, IDD_WIZLAUNCHMAGNIFY);
//	rgpwp[nCountPages++] = new CGenericWizPg(psp + nCountPages, IDD_WIZLAUNCHSCREENREADER);
//	rgpwp[nCountPages++] = new CGenericWizPg(psp + nCountPages, IDD_WIZLAUNCHONSCREENKEYBOARD);
	rgpwp[nCountPages++] = new CWelcome2Pg(psp + nCountPages);

	rgpwp[nCountPages++] = new CWizardOptionsPg(psp + nCountPages);
	
	rgpwp[nCountPages++] = new CScrollBarPg(psp + nCountPages);

	rgpwp[nCountPages++] = new CIconSizePg(psp + nCountPages);
//	rgpwp[nCountPages++] = new CIcon2Pg(psp + nCountPages);


	// Color	
	rgpwp[nCountPages++] = new CLookPreviewColorPg(psp + nCountPages);
//	rgpwp[nCountPages++] = new CHighContrastPg(psp + nCountPages);
	
	// Sound
//	rgpwp[nCountPages++] = new CSoundSentryShowSoundsPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CSoundSentryPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CShowSoundsPg(psp + nCountPages);
	
	// Keyboard
	rgpwp[nCountPages++] = new CStickyKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CFilterKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CFilterKeysSettingsPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CToggleKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CShowKeyboardHelpPg(psp + nCountPages);
//	rgpwp[nCountPages++] = new CSerialKeysPg(psp + nCountPages);
	
	// Mouse
	rgpwp[nCountPages++] = new CMouseKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseKeysSettingsPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseCursorPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseButtonPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseSpeedPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseTrailsPg(psp + nCountPages);

	
	// Standard Wizard pages
	rgpwp[nCountPages++] = new CGenericWizPg(psp + nCountPages, IDD_WIZNOOPTIONSSELECTED, IDS_WIZNOOPTIONSSELECTEDTITLE, IDS_WIZNOOPTIONSSELECTEDSUBTITLE);
//	rgpwp[nCountPages++] = new CHotKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CAccessTimeOutPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CSaveForDefaultUserPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CSaveToFilePg(psp + nCountPages);
	rgpwp[nCountPages++] = new FinishWizPg(psp + nCountPages);
	
	// Make sure we have the correct number of pages in our wizard
	_ASSERTE(MAX_PAGES == nCountPages);
	
	// Make sure pages were created
	for (int i = 0; i < nCountPages; i++)
	{
		if (NULL == rgpwp[i])
			break;
	}
	
	if(i<nCountPages)
	{
		// We didn't have enough memory to create all the pages
		// Clean out allocated pages and return
		for(int i=0;i<nCountPages;i++)
			if(rgpwp[i])
				delete rgpwp[i];
			return E_OUTOFMEMORY;
	}


	
	// Create the orders for the pages to be run
	DWORD rgdwMainPath[] = {
                        IDD_WIZNEWWELCOME,
						IDD_WIZWELCOME,
//						IDD_WIZLAUNCHMAGNIFY,
//						IDD_WIZLAUNCHSCREENREADER,
//						IDD_WIZLAUNCHONSCREENKEYBOARD,
						IDD_WIZWELCOME2,
						IDD_WIZOPTIONS,
						IDD_WIZFINISH // We need this placeholder here so we get a 'NEXT' button on IDD_WIZOPTIONS
							};

	if(!WizardPage::sm_WizPageOrder.AddPages(0xFFFFFFFF, rgdwMainPath, ARRAYSIZE(rgdwMainPath)))
		return E_OUTOFMEMORY;

	/////////////////////////////////////////////
	// See if we need the 16 or 256 color bitmap
	BOOL bUse256ColorBmp = FALSE;
	HDC hdc = GetDC(NULL);
	if(hdc)
	{
		if(GetDeviceCaps(hdc,BITSPIXEL) >= 8)
			bUse256ColorBmp = TRUE;
		ReleaseDC(NULL, hdc);
	}


	////////////////////////////////
	// Do the property sheet

	PROPSHEETHEADER psh;
	memset(&psh, 0, sizeof(psh));
	psh.dwSize		= sizeof(PROPSHEETHEADER);
	psh.dwFlags 	= PSH_USECALLBACK | PSH_WIZARD | PSH_PROPSHEETPAGE
		| PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER /*| *//*PSH_STRETCHWATERMARK*/;
	psh.hwndParent	= hwndParent;
	psh.hInstance	= g_hInstDll;
	psh.pszIcon 	= NULL;
	psh.pszCaption	= NULL;
	psh.nPages		= MAX_PAGES;
	psh.nStartPage	= 54331; // We will actually set it in PropSheetCallback to rgdwMainPath[0]
	// NOTE: Bug - This only works if nStartPage is non-zero
	psh.ppsp		= psp;
	psh.pfnCallback = PropSheetCallback;

#if 0
	psh.nStartPage	= 0; // We will actually set it in PropSheetCallback to rgdwMainPath[0]
	psh.pfnCallback = NULL;
	psh.dwFlags 	= PSH_WIZARD | PSH_PROPSHEETPAGE;
#endif
	
	psh.pszbmWatermark = MAKEINTRESOURCE(IDB_ACCWIZ);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_ACCMARK);

#if 0 // Right now, no watermarks
	psh.pszbmWatermark = bUse256ColorBmp?MAKEINTRESOURCE(IDB_WATERMARK256):MAKEINTRESOURCE(IDB_WATERMARK16);
	psh.pszbmHeader = bUse256ColorBmp?MAKEINTRESOURCE(IDB_BANNER256):MAKEINTRESOURCE(IDB_BANNER16);
#endif
   
	
    if (-1 != PropertySheet(&psh))
		hResult = NO_ERROR;
	else
		hResult = E_FAIL;

    // Clean up memory allocated for WizardPage's
	for(i=0;i<nCountPages;i++)
		if(rgpwp[i])
			delete rgpwp[i];
		
	
	return hResult;
}












HRESULT
CreateAndRunWizard2(
				   HWND hwndParent)
{
	HRESULT hResult = E_OUTOFMEMORY;
	
	PROPSHEETPAGE psp[1];
	WizardPage *rgpwp[1];
	
	// Zero init the arrays
	memset(&psp, 0, sizeof(psp));
	memset(&rgpwp, 0, sizeof(rgpwp));
	
	// ///////////////////////
	// Create Pages Here - NOTE: Order does not matter - we'll control it with our own list
	//
	int nCountPages = 0;
	rgpwp[nCountPages++] = new FinishWizPg(psp + nCountPages);
	
	// Make sure pages were created
	for (int i = 0; i < nCountPages; i++)
	{
		if (NULL == rgpwp[i])
			break;
	}
	
	if(i<nCountPages)
	{
		// We didn't have enough memory to create all the pages
		// Clean out allocated pages and return
		for(int i=0;i<nCountPages;i++)
			if(rgpwp[i])
				delete rgpwp[i];
			return E_OUTOFMEMORY;
	}


	
	// Create the orders for the pages to be run
	DWORD rgdwMainPath[] = {
						IDD_WIZFINISH // We need this placeholder here so we get a 'NEXT' button on IDD_WIZOPTIONS
							};

	if(!WizardPage::sm_WizPageOrder.AddPages(0xFFFFFFFF, rgdwMainPath, ARRAYSIZE(rgdwMainPath)))
		return E_OUTOFMEMORY;

	/////////////////////////////////////////////
	// See if we need the 16 or 256 color bitmap
	BOOL bUse256ColorBmp = FALSE;
	HDC hdc = GetDC(NULL);
	if(hdc)
	{
		if(GetDeviceCaps(hdc,BITSPIXEL) >= 8)
			bUse256ColorBmp = TRUE;
		ReleaseDC(NULL, hdc);
	}


	////////////////////////////////
	// Do the property sheet

	PROPSHEETHEADER psh;
	memset(&psh, 0, sizeof(psh));
	psh.dwSize		= sizeof(PROPSHEETHEADER);
	psh.dwFlags 	= PSH_USECALLBACK | PSH_WIZARD | PSH_PROPSHEETPAGE
		| PSH_WIZARD97 | PSH_WATERMARK |PSH_HEADER /*| *//*PSH_STRETCHWATERMARK*/;
	psh.hwndParent	= hwndParent;
	psh.hInstance	= g_hInstDll;
	psh.pszIcon 	= NULL;
	psh.pszCaption	= NULL;
	psh.nPages		= 1;
	psh.nStartPage	= 54331; // We will actually set it in PropSheetCallback to rgdwMainPath[0]
	// NOTE: Bug - This only works if nStartPage is non-zero
	psh.ppsp		= psp;
	psh.pfnCallback = PropSheetCallback;

#if 0
	psh.nStartPage	= 0; // We will actually set it in PropSheetCallback to rgdwMainPath[0]
	psh.pfnCallback = NULL;
	psh.dwFlags 	= PSH_WIZARD | PSH_PROPSHEETPAGE;
#endif
	
	psh.pszbmWatermark = MAKEINTRESOURCE(IDB_ACCWIZ);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_ACCMARK);

#if 0 // Right now, no watermarks
	psh.pszbmWatermark = bUse256ColorBmp?MAKEINTRESOURCE(IDB_WATERMARK256):MAKEINTRESOURCE(IDB_WATERMARK16);
	psh.pszbmHeader = bUse256ColorBmp?MAKEINTRESOURCE(IDB_BANNER256):MAKEINTRESOURCE(IDB_BANNER16);
#endif
	
	if (-1 != PropertySheet(&psh))
		hResult = NO_ERROR;
	else
		hResult = E_FAIL;
	
	// Clean up memory allocated for WizardPage's
	for(i=0;i<nCountPages;i++)
		if(rgpwp[i])
			delete rgpwp[i];
		
		
		
		
		
	return hResult;
}















INT
PropSheetCallback(
				  HWND hwnd,
				  UINT uMsg,
				  LPARAM lParam
				  )
{
	switch(uMsg)
	{
	case PSCB_PRECREATE:
		break;
		
	case PSCB_INITIALIZED:
        {
            // Set the first page according to are global list of page orders
            //			PropSheet_SetCurSelByID(hwnd, WizardPage::sm_WizPageOrder.GetFirstPage());
            // HACK - Set TO Options page since we added WIZWIZ page
            
            // HACK. Remove Context Sensitive help
            LONG Style = GetWindowLong(hwnd, GWL_EXSTYLE);
            
            if(0 == Style)
            {
                // DbgTrace((DEBUG_ERROR, "GetWindowLong failed. WizDlgs.cpp\n"));
                // DbgTraceSystemError(GetLastError());
            }
            if(0 == SetWindowLong(hwnd, GWL_EXSTYLE, Style & ~WS_EX_CONTEXTHELP))
            {
                // DbgTrace((DEBUG_ERROR, "SetWindowLong failed. WizDlgs.cpp\n"));
                // DbgTraceSystemError(GetLastError());
            }

#ifdef WIZWIZ
			_ASSERTE(IDD_WIZWIZ == WizardPage::sm_WizPageOrder.GetFirstPage()); // Change this if we remove the wiz wiz page
			PropSheet_SetCurSelByID(hwnd, IDD_WIZWELCOME);
#endif
		}
		break;
	}
	return 0;
}


// Helper functions
// Helper function
void LoadArrayFromStringTable(int nIdString, int *rgnValues, int *pnCountValues)
{
	// This function load the allowed value array from the string table
	// If the values are not stored in the string table, the function
	// can be overridden in a derived class
	// Load in allowed sizes for scroll bar from string table

	_ASSERTE(nIdString); // Make sure we were passed a string

	TCHAR szArray[255];
    LoadString(g_hInstDll, nIdString, szArray, ARRAYSIZE(szArray));

	// Assume at most MAX_DISTINCT_VALUES sizes
	LPTSTR szCurrentLocation = szArray;
	for(int i=0;i<MAX_DISTINCT_VALUES;i++)
	{
		if(!szCurrentLocation)
			break;
		_stscanf(szCurrentLocation, __TEXT("%i"), &rgnValues[i]);

		// Find the next space
		// NOTE: If there are more than one spaces between characters, this will read the same entry twice
		szCurrentLocation = _tcschr(++szCurrentLocation, __TEXT(' '));
	}
	*pnCountValues = i;
	_ASSERTE(*pnCountValues);
}



// This function is prototyped in the pre-compiled header
int StringTableMessageBox(HWND hWnd, int nText, int nCaption, UINT uType)
{
	TCHAR szTitle[1024];
	TCHAR szText[1024];
	LoadString(g_hInstDll, nCaption, szTitle, ARRAYSIZE(szTitle));
	LoadString(g_hInstDll, nText, szText, ARRAYSIZE(szText));
	return MessageBox(hWnd, szText, szTitle, uType);
}

void CAccWizOptions::ApplyWindowsDefault()
{
    HKEY hkey;
    DWORD dwDisposition;
    DWORD len;
    
    m_schemeCurrent.ApplyChanges(m_schemeWindowsDefault);
    
    // BUG: Update the preview scheme. Else will put back the old 
    // color scheme if something changes
    m_schemePreview = m_schemeWindowsDefault;
    
#if 0
    
    HIGHCONTRAST l_hc;
    l_hc.cbSize = sizeof(l_hc);
    
    // Get the current HighContrast settings. 
    SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(l_hc), 
        &l_hc, SPIF_UPDATEINIFILE);
    
    l_hc.dwFlags = 126;
    l_hc.dwFlags |= 0x10000000;
    
    // This is the correct scheme name.
    lstrcpy(l_hc.lpszDefaultScheme, g_winScheme);
    
    // In case the selected scheme was a high contrast scheme
    // call SystemParametersInfo with SPI_SETHCFLAG/ SPI_UNUSED39
    SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof(l_hc), 
        &l_hc, SPIF_UPDATEINIFILE);
    
#endif
}
