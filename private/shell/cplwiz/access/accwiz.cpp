#include "pch.hxx" // PCH
#pragma hdrstop

#include "AccWiz.h"

#include "resource.h"

#include "pgfinish.h"
#include "pgGenric.h"

#include "pgWizOpt.h"
#include "pgWizWiz.h"

// Color pages
#include "pgLokPrv.h"
//#include "pgHghCon.h"

// Sound pages
#include "pgSndSen.h"

// Keyboard pages
#include "pgStkKey.h"
#include "pgFltKey.h"
#include "pgTglKey.h"
#include "pgShwHlp.h"
//#include "pgSerKey.h"

// Mouse pages
#include "pgMseKey.h"
#include "pgMseCur.h"
#include "pgMseBut.h"

// Text size pages
#include "pgMinTxt.h"
#include "pgMinTx2.h"
#include "pgScrBar.h"
#include "pgIconSz.h"


#include "pgHotKey.h"
#include "pgTmeOut.h"
#include "pgSveDef.h"
#include "pgSveFil.h"


#include "LookPrev.h"

// Declaration of the global options variable
CAccWizOptions g_Options;
HINSTANCE g_hInstDll = NULL;    // DLL instance handle.

int WINAPI WinMain( 
				   HINSTANCE hInstance, // handle to current instance 
				   HINSTANCE hPrevInstance, // handle to previous instance 
				   LPSTR lpCmdLine, // pointer to command line 
				   int nCmdShow // show state of window 
				   )
{
	g_hInstDll = hInstance;
	VERIFY(CLookPrev::sm_Globals.Initialize()); // Make sure this has been initialized

	AccWiz_RunDllA(NULL, hInstance, lpCmdLine, nCmdShow);
	return 0;
}


HRESULT
CreateAndRunWizard(
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

	HRESULT hResult = CreateAndRunWizard(hwndParent);

	if(!SUCCEEDED(hResult))
	{
		// TODO: Put out of memory message here
		_ASSERTE(FALSE);
#pragma message("Put Out of Memory message here")
	}
}



const INT MAX_PAGES  = 29;

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
	rgpwp[nCountPages++] = new CWizWizPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CWizardOptionsPg(psp + nCountPages);
	
	// Size
	rgpwp[nCountPages++] = new CLookPreviewMinText1Pg(psp + nCountPages);
	rgpwp[nCountPages++] = new CLookPreviewMinText2Pg(psp + nCountPages);
	rgpwp[nCountPages++] = new CLookPreviewMinText3Pg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMinTextPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMinText2Pg(psp + nCountPages);

	rgpwp[nCountPages++] = new CLookPreviewScrollBar1Pg(psp + nCountPages);
	rgpwp[nCountPages++] = new CLookPreviewScrollBar2Pg(psp + nCountPages);
	rgpwp[nCountPages++] = new CScrollBarPg(psp + nCountPages);

	rgpwp[nCountPages++] = new CLookPreviewBorder1Pg(psp + nCountPages);
	rgpwp[nCountPages++] = new CLookPreviewBorder2Pg(psp + nCountPages);

	rgpwp[nCountPages++] = new CIconSizePg(psp + nCountPages);


	// Color	
	rgpwp[nCountPages++] = new CLookPreviewColorPg(psp + nCountPages);
//	rgpwp[nCountPages++] = new CHighContrastPg(psp + nCountPages);
	
	// Sound
	rgpwp[nCountPages++] = new CSoundSentryShowSoundsPg(psp + nCountPages);
	
	// Keyboard
	rgpwp[nCountPages++] = new CStickyKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CFilterKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CToggleKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CShowKeyboardHelpPg(psp + nCountPages);
//	rgpwp[nCountPages++] = new CSerialKeysPg(psp + nCountPages);
	
	// Mouse
	rgpwp[nCountPages++] = new CMouseKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseCursorPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseButtonPg(psp + nCountPages);
	
	// Standard Wizard pages
	rgpwp[nCountPages++] = new CGenericWizPg(psp + nCountPages, IDD_WIZNOOPTIONSSELECTED, IDS_WIZNOOPTIONSSELECTEDTITLE, IDS_WIZNOOPTIONSSELECTEDSUBTITLE);
	rgpwp[nCountPages++] = new CHotKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CAccessTimeOutPg1(psp + nCountPages);
	rgpwp[nCountPages++] = new CAccessTimeOutPg2(psp + nCountPages);
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
						IDD_WIZWIZ,
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
		| PSH_WIZARD97 /*| PSH_WATERMARK |*/ /*PSH_HEADER | *//*PSH_STRETCHWATERMARK*/;
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
			_ASSERTE(IDD_WIZWIZ == WizardPage::sm_WizPageOrder.GetFirstPage()); // Change this if we remove the wiz wiz page
			PropSheet_SetCurSelByID(hwnd, IDD_WIZOPTIONS);
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




