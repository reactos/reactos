/******************************************************************************\
*
*	CDCACHE.EXE
*
*	Synopsis:
*		An AutoRun EXE to enable easy addition of CD-ROM content
*		into the Internet Explorer (WININET) Persistent URL Cache.
*
*	Usage:
*		Place an AUTORUN.INF at the root of the CD-ROM which has content
*		that you want to register with the WININET Persistent URL Cache.
*		Contents of AUTORUN.INF:
*
*			[autorun]
*			open=cdcache.exe
*			icon=cdcache.exe, 1
*
*		Additionally create a CDCACHE.INF at the root of the CD-ROM.
*		Typical contents:
*
*			[Add.CacheContainer]
*			<Friendly Unique Vendor Name>=<INF Section Name>
*
*			[INF Section Name]
*			CachePrefix=<string>
*			CacheRoot=<relative path on CD-ROM of data>
*			KBCacheLimit=<numerical amount in KB>
*			AutoDelete=Yes|No (default)
*			IncludeSubDirs=Yes (default) |No
*			NoDesktopInit=Yes|No (default)
*
*
*		CMD Line Options:
*		/Silent		Install Cache Container without showing UI
*		/Remove     Uninstall the cache container
*		/Uninstall  same as /Remove
*
*	History
*		23June97	robgil				created
*		06Aug97		robgil				add IE4 wininet.dll checks
*		26Aug97		robgil				manual register if no IE4
*
*	Copyright (C) 1994-1997 Microsoft Corporation.
*	All rights reserved.
*
\******************************************************************************/
#include "stdhdr.h"

/////////////////////////////////////////////////////////////////////////
// Defines and Type Declarations

#define STRING_BUFFER_SIZE		256

#define CACHE_ACTION_INSTALL			0
#define CACHE_ACTION_REMOVE				1
#define CACHE_ACTION_FILL_LB			2
#define CACHE_ACTION_MAKE_REG_ENTRIES	3

typedef BOOL (CALLBACK* LPFNCREATEURLCACHECONTAINER)(LPCSTR,LPCSTR,LPCSTR,DWORD,DWORD,DWORD,LPVOID,LPDWORD);
typedef BOOL (CALLBACK* LPFNDELETEURLCACHECONTAINER)(LPCSTR,DWORD);
typedef HANDLE (CALLBACK* LPFNFINDFIRSTURLCACHECONTAINER)(LPDWORD,LPINTERNET_CACHE_CONTAINER_INFO,LPDWORD,DWORD);
typedef BOOL (CALLBACK* LPFNFINDNEXTURLCACHECONTAINER)(HANDLE,LPINTERNET_CACHE_CONTAINER_INFO,LPDWORD);
typedef BOOL (CALLBACK* LPFNFINDCLOSEURLCACHE)(HANDLE);
typedef BOOL (CALLBACK* LPFNGETURLCACHECONFIGINFO)(LPINTERNET_CACHE_CONFIG_INFO,LPDWORD,DWORD);

/////////////////////////////////////////////////////////////////////////
// Global Variables:

HINSTANCE g_hInst;			// current instance
BOOL g_fRunSilent = FALSE;	// True = show no UI
BOOL g_fRemove    = FALSE;	// True = remove the cache containers in INF
//BOOL g_fNoIE4Msg  = FALSE;	// True = do not show UI saying IE4 WININET is required
BOOL g_fNoIE4	  = FALSE;	// IE4 WININET is not available

TCHAR  gszIniValTrue[]			= INI_TRUE ;
TCHAR  gszIniValFalse[]			= INI_FALSE ;
TCHAR  gszIniValOn[]			= INI_ON ;
TCHAR  gszIniValOff[]			= INI_OFF ;

TCHAR  gszIniValYes[]			= INI_YES ;
TCHAR  gszIniValNo[]			= INI_NO ;

LPFNCREATEURLCACHECONTAINER		lpfnCreateUrlCacheContainer		= NULL;
LPFNDELETEURLCACHECONTAINER		lpfnDeleteUrlCacheContainer		= NULL;
LPFNFINDFIRSTURLCACHECONTAINER	lpfnFindFirstUrlCacheContainer	= NULL;
LPFNFINDNEXTURLCACHECONTAINER	lpfnFindNextUrlCacheContainer	= NULL;
LPFNFINDCLOSEURLCACHE			lpfnFindCloseUrlCache			= NULL;
LPFNGETURLCACHECONFIGINFO		lpfnGetUrlCacheConfigInfo		= NULL;

/////////////////////////////////////////////////////////////////////////
// Foward declarations of functions included in this code module:

INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CenterWindow (HWND, HWND);
int OnInitDialog(HWND hDlg);

BOOL LoadWininet(void);
BOOL WininetLoaded(void);
BOOL CacheContainer(DWORD *dwTotal, DWORD *dwInstalled, DWORD dwAction, HWND hListBox);

HRESULT	ExpandEntry(
    LPSTR szSrc,
    LPSTR szBuf,
    DWORD cbBuffer,
    const char * szVars[],
    const char * szValues[]);

HRESULT ExpandVar(
        LPSTR& pchSrc,          // passed by ref!
        LPSTR& pchOut,          // passed by ref!
        DWORD& cbLen,           // passed by ref!
        DWORD cbBuffer,         // size of out buffer
        const char * szVars[],  // array of variable names eg. %EXE_ROOT%
        const char * szValues[]);// corresponding values to expand of vars

LPSTR GetINFDir(LPSTR lpBuffer, int nBuffSize);
LPSTR GetINFDrive(LPSTR lpBuffer, int nBuffSize);
WORD GetProfileBooleanWord(LPCTSTR szIniSection, LPCTSTR szKeyName, LPCTSTR szIniFile);
DWORD CreateAdditionalEntries(LPCSTR lpszUniqueVendorName, LPCSTR lpszVolumeTitle, LPCSTR lpszVolumeLabel, LPCSTR lpszPrefixMap);
DWORD GetPrefixMapEntry(LPCSTR lpszUniqueVendorName, LPSTR lpszPrefixMap, DWORD cbPrefixMap);
BOOL UrlCacheContainerExists(LPCSTR lpszUniqueVendorName, LPCSTR lpszCachePrefix, LPCSTR lpszPrefixMap);

// WININET CreateUrlCacheContainer WRAPPER
// Wraps up the hacks in one spot - see f() header for details
BOOL _CreateUrlCacheContainer(
     IN LPCSTR lpszUniqueVendorName,
     IN LPCSTR lpszCachePrefix,
     IN LPCSTR lpszPrefixMap,			// New - part of WRAPPER
     IN LPCSTR lpszVolumeTitle,	        // New - part of WRAPPER
     IN LPCSTR lpszVolumeLabel,         // New - part of Wrapper.
     IN DWORD KBCacheLimit,
     IN DWORD dwContainerType,
     IN DWORD dwOptions
     );


/************************************************************************\
 *    FUNCTION: WinMain
\************************************************************************/

int APIENTRY WinMain(HINSTANCE g_hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LPSTR   lpszCmd = NULL;
	DWORD	dwTotal = 0;
	DWORD	dwInstalled = 0;

	g_hInst = g_hInstance;

	// Parse lpCmdLine looking for options we understand
    TCHAR szTokens[] = _T("-/ ");
    LPTSTR lpszToken = _tcstok(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (_tcsicmp(lpszToken, _T("Silent"))==0)
            g_fRunSilent = TRUE;
        else if (_tcsicmp(lpszToken, _T("Remove"))==0)
			g_fRemove = TRUE;
		else if (_tcsicmp(lpszToken, _T("Uninstall"))==0)
			g_fRemove = TRUE;
//		else if (_tcsicmp(lpszToken, _T("NoIE4Msg"))==0)
//			g_fNoIE4Msg = TRUE;

        lpszToken = _tcstok(NULL, szTokens);
    }

	
	// Check for IE4 or higher WININET.DLL version
	// and dynamically load it and init global function pointers
	// to WININET f() used in this application
	// This will avoid Undefined Dynalink errors when run on a
	// system without IE4
	if (!LoadWininet())
	{
		g_fNoIE4 = TRUE;

		// Put up message about requiring IE4 WININET

		/* Since we workaround not having IE4 - no need for message
		 
		if (!g_fNoIE4Msg)
		{
			char szString[128];		// Keep string 70% larger for localization
			char szCaption[128];	// Keep string 70% larger for localization

			LoadString (g_hInst, ID_APPNAME, szCaption, sizeof(szCaption));
			LoadString (g_hInst, IDM_NEEDIE4WININET, szString, sizeof(szString));
			MessageBox(NULL, szString, szCaption, MB_OK);
		}
		*/

		// Can't call WININET
		// Need to make registry entries to install cache containers
		//
		if (!CacheContainer(&dwTotal, &dwInstalled, CACHE_ACTION_FILL_LB, NULL))
		{
            if (g_fRunSilent)
            {
                // Create cache entries in silent mode.
                CacheContainer(&dwTotal, &dwInstalled, CACHE_ACTION_MAKE_REG_ENTRIES, NULL);

            }
            else
            {
                // Otherwise run app.
                DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MAINAPP), NULL, DlgProc);
            }
            return(FALSE);
		}

		return 0;	// Quit and go home
	}


	if (!g_fRunSilent)
	{
		// Only want to put up UI if any of the containers are NOT installed
		// (this includes those containers that are installed but the
		//  PrefixMap entry is incorrect - i.e. wrong drive)

		if (!CacheContainer(&dwTotal, &dwInstalled, CACHE_ACTION_FILL_LB, NULL))
		{
			DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MAINAPP), NULL, DlgProc);
			return(FALSE);
		}
		else
		{
			// All the CacheContainers are already installed or there is no INF
			// so check if we want to uninstall
			// OnInitDialog checks the g_fRemove flags and POST's a msg
			// to dialog to initiate the Uninstall steps
			if (g_fRemove)
				DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_MAINAPP), NULL, DlgProc);
		}
	}
	else
	{
		DWORD	dwAction = CACHE_ACTION_INSTALL;	// default action is INSTALL

		// We're running silent and deep - all quiet on board
		// we don't need no stinkin window

		if (g_fRemove)
			dwAction = CACHE_ACTION_REMOVE;

		if (!CacheContainer(&dwTotal, &dwInstalled, dwAction, NULL))
		{
			// BUGBUG: Since we're running silent what
			// should we do on failure?
		}

		return 0;
	}
	
	return 0;
}

/************************************************************************\
 *    FUNCTION: DlgProc
\************************************************************************/

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{

		case WM_INITDIALOG:
			return OnInitDialog(hDlg);

		case WM_COMMAND:
			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!
		
			switch (wmId)
			{
				case IDM_INSTALL:
				{
					DWORD	dwError = 0;
					DWORD	dwTotal = 0;
					DWORD	dwInstalled = 0;
					DWORD	dwAction = 0;

					if (g_fNoIE4)
						dwAction = CACHE_ACTION_MAKE_REG_ENTRIES;
					else
						dwAction = CACHE_ACTION_INSTALL;

					if (!CacheContainer(&dwTotal, &dwInstalled, dwAction, NULL))
					{
						dwError = GetLastError();
					}

					if (dwInstalled > 0)
					{
						char szString[128];	// Keep string 70% larger for localization
						char szBuffer[256];

						// Successfully installed a cache container
						// though not necessarily all of them.
						LoadString (g_hInst, IDM_SUCCESS, szString, sizeof(szString));
						wsprintf(szBuffer, szString, dwInstalled, dwTotal);
						LoadString (g_hInst, ID_APPNAME, szString, sizeof(szString));
						MessageBox(hDlg, szBuffer, szString, MB_OK);

						// We're done close this app
						PostMessage (hDlg, WM_CLOSE, 0, 0);
					}
					else
					{
						char szString[128];	// Keep string 70% larger for localization
						char szBuffer[256];

						// Unable to install any of the cache containers successfully
						LoadString (g_hInst, IDM_FAILED, szString, sizeof(szString));
						wsprintf(szBuffer, szString, dwTotal);
						LoadString (g_hInst, ID_APPNAME, szString, sizeof(szString));
						MessageBox(hDlg, szBuffer, szString, MB_OK);
					}
					break;
				}

				case IDM_UNINSTALL:
				{
					DWORD	dwError = 0;
					DWORD	dwTotal = 0;
					DWORD	dwRemoved = 0;

					if (g_fNoIE4)
					{
						char szString[128];	// Keep string 70% larger for localization
						char szBuffer[256];

						// Uninstall of cache containers requires IE4
						LoadString (g_hInst, IDM_ERR_IE4REQFORUNINSTALL, szString, sizeof(szString));
						wsprintf(szBuffer, szString, dwRemoved, dwTotal);
						LoadString (g_hInst, ID_APPNAME, szString, sizeof(szString));
						MessageBox(hDlg, szBuffer, szString, MB_OK);
					}
					else
					{

						if (!CacheContainer(&dwTotal, &dwRemoved, CACHE_ACTION_REMOVE, NULL))
						{
							dwError = GetLastError();
						}

						if (dwRemoved > 0)
						{
							char szString[128];	// Keep string 70% larger for localization
							char szBuffer[256];

							// Successfully UnInstalled a cache container
							// though not necessarily all of them.
							LoadString (g_hInst, IDM_SUCCESS_REMOVE, szString, sizeof(szString));
							wsprintf(szBuffer, szString, dwRemoved, dwTotal);
							LoadString (g_hInst, ID_APPNAME, szString, sizeof(szString));
							MessageBox(hDlg, szBuffer, szString, MB_OK);
						}
						else
						{
							char szString[128];	// Keep string 70% larger for localization
							char szBuffer[256];

							// Unable to install any of the cache containers successfully
							LoadString (g_hInst, IDM_FAILED_REMOVE, szString, sizeof(szString));
							wsprintf(szBuffer, szString, dwTotal);
							LoadString (g_hInst, ID_APPNAME, szString, sizeof(szString));
							MessageBox(hDlg, szBuffer, szString, MB_OK);
						}
					}

					if (g_fRemove)
					{
						// We're done close this app
						PostMessage (hDlg, WM_CLOSE, 0, 0);
					}

					break;
				}

				case IDCANCEL:
					EndDialog(hDlg, TRUE);
					break;

				default:
					return (FALSE);
			}
			break;

		default:
			return (FALSE);
	}
	return (TRUE);
}


/************************************************************************\
 *    FUNCTION: CenterWindow
\************************************************************************/
// This is a 'utility' function I find usefull. It will center one
// window over another. It also makes sure that the placement is within
// the 'working area', meaning that it is both within the display limits
// of the screen, -and- not obscured by the tray or other frameing
// elements of the desktop.
BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
	RECT    rChild, rParent, rWorkArea = {0,0,0,0};
	int     wChild, hChild, wParent, hParent;
	int     wScreen, hScreen, xScreen, yScreen, xNew, yNew;
	BOOL bResult;

	// Get the Height and Width of the child window
	GetWindowRect (hwndChild, &rChild);
	wChild = rChild.right - rChild.left;
	hChild = rChild.bottom - rChild.top;

	// Get the Height and Width of the parent window
	GetWindowRect (hwndParent, &rParent);
	wParent = rParent.right - rParent.left;
	hParent = rParent.bottom - rParent.top;

	// Get the limits of the 'workarea'
#if !defined(SPI_GETWORKAREA)
#define SPI_GETWORKAREA 48
#endif
	bResult = SystemParametersInfo(
    	SPI_GETWORKAREA,	// system parameter to query or set
    	sizeof(RECT),	// depends on action to be taken
    	&rWorkArea,	// depends on action to be taken
    	0);	

	wScreen = rWorkArea.right - rWorkArea.left;
	hScreen = rWorkArea.bottom - rWorkArea.top;
	xScreen = rWorkArea.left;
	yScreen = rWorkArea.top;

	// On Windows NT, the above metrics aren't valid (yet), so they all return
	// '0'. Lets deal with that situation properly:
	if (wScreen==0 && hScreen==0) {
		wScreen = GetSystemMetrics(SM_CXSCREEN);
		hScreen = GetSystemMetrics(SM_CYSCREEN);
		xScreen = 0; // These values should already be '0', but just in case
		yScreen = 0;
	}

	// Calculate new X position, then adjust for screen
	xNew = rParent.left + ((wParent - wChild) /2);
	if (xNew < xScreen) {
		xNew = xScreen;
	} else if ((xNew+wChild) > wScreen) {
		xNew = (xScreen + wScreen) - wChild;
	}

	// Calculate new Y position, then adjust for screen
	yNew = rParent.top  + ((hParent - hChild) /2);
	if (yNew < yScreen) {
		yNew = yScreen;
	} else if ((yNew+hChild) > hScreen) {
		yNew = (yScreen + hScreen) - hChild;
	}

	// Set it, and return
	return SetWindowPos (hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

int OnInitDialog(HWND hDlg)
{
	HWND	hListBox;
	DWORD	dwRemoved = 0;
	DWORD	dwTotal = 0;

	CenterWindow (hDlg, GetDesktopWindow ());

	hListBox = GetDlgItem(hDlg, IDC_LIST);

	// Populate list box with Cache Container list
	CacheContainer(&dwTotal, &dwRemoved, CACHE_ACTION_FILL_LB, hListBox);

	// #57353 - after adding don't show UI if already installed
	// we forgot to account for /uninstall on cmd line
	if (g_fRemove)
		PostMessage(hDlg, WM_COMMAND, IDM_UNINSTALL, 0L);

	return FALSE;
}

/************************************************************************\
*   FUNCTION: LoadWininet()
*
*	If IE4 or greater version of WININET then load it up and establish
*	function pointers to use in rest of application.
*
*	returns BOOL
*		TRUE  - Sufficient version of WININET.DLL is available
*		FALSE - WININET.DLL is not new enough for our purposes
*
\************************************************************************/
BOOL LoadWininet()
{
	HINSTANCE	hDll;

	hDll = LoadLibrary("WININET.DLL");

	if (hDll != NULL)
	{
		lpfnCreateUrlCacheContainer = (LPFNCREATEURLCACHECONTAINER)GetProcAddress(hDll, "CreateUrlCacheContainerA");
		lpfnDeleteUrlCacheContainer = (LPFNDELETEURLCACHECONTAINER)GetProcAddress(hDll, "DeleteUrlCacheContainerA");
		lpfnFindFirstUrlCacheContainer = (LPFNFINDFIRSTURLCACHECONTAINER)GetProcAddress(hDll, "FindFirstUrlCacheContainerA");
		lpfnFindNextUrlCacheContainer = (LPFNFINDNEXTURLCACHECONTAINER)GetProcAddress(hDll, "FindNextUrlCacheContainerA");
		lpfnFindCloseUrlCache = (LPFNFINDCLOSEURLCACHE)GetProcAddress(hDll, "FindCloseUrlCache");
		lpfnGetUrlCacheConfigInfo = (LPFNGETURLCACHECONFIGINFO)GetProcAddress(hDll, "GetUrlCacheConfigInfoA");

		if ( (!lpfnCreateUrlCacheContainer) ||
			 (!lpfnDeleteUrlCacheContainer) ||
			 (!lpfnFindFirstUrlCacheContainer) ||
			 (!lpfnFindNextUrlCacheContainer) ||
			 (!lpfnFindCloseUrlCache) ||
			 (!lpfnGetUrlCacheConfigInfo) )
		{
			
			lpfnCreateUrlCacheContainer = NULL;
			lpfnDeleteUrlCacheContainer = NULL;
			lpfnFindFirstUrlCacheContainer = NULL;
			lpfnFindNextUrlCacheContainer = NULL;
			lpfnFindCloseUrlCache = NULL;
			lpfnGetUrlCacheConfigInfo = NULL;

			FreeLibrary(hDll);

			return FALSE;
		}
	}
	else
		return FALSE;

	return TRUE;
}

/************************************************************************\
*   FUNCTION: WininetLoaded()
*
*	returns BOOL
*		TRUE  - Sufficient version of WININET.DLL is available
*		FALSE - WININET.DLL is not new enough for our purposes
*
\************************************************************************/
BOOL WininetLoaded()
{
	if (lpfnCreateUrlCacheContainer)
		return TRUE;

	return FALSE;
}

/************************************************************************\
*   FUNCTION: UrlCacheContainerExists()
*
*
*	returns BOOL
*		TRUE  - This cache container is already installed and PrefixMap
*				location is correct
*		FALSE - Cache container is not installed or it's PrefixMap
*				location is different
*
\************************************************************************/

BOOL UrlCacheContainerExists(LPCSTR lpszUniqueVendorName, LPCSTR lpszCachePrefix, LPCSTR lpszPrefixMap)
{
	BYTE	bBuf[4096];
	LPINTERNET_CACHE_CONTAINER_INFO lpCCI = (LPINTERNET_CACHE_CONTAINER_INFO) bBuf;
	DWORD	cbCEI = sizeof(bBuf);
	DWORD	dwModified = 0;
	HANDLE	hEnum = NULL;
	BOOL	bFound = FALSE;

	BOOL	bReturn = FALSE;

	if (!WininetLoaded())
		return FALSE;

	// Look for our cache container, then determine if it already exists
	// also need to make sure PrefixMap entry is correct
	// for situation when CD is placed into a different drive
	// after it's already been installed
	hEnum = lpfnFindFirstUrlCacheContainer(&dwModified, lpCCI, &cbCEI, 0);

	if (0 == lstrcmpi(lpszUniqueVendorName, lpCCI->lpszName))
		bFound = TRUE;
	else
	{
		while (hEnum && lpfnFindNextUrlCacheContainer(hEnum, lpCCI, &cbCEI))
		{
			if (0 == lstrcmpi(lpszUniqueVendorName, lpCCI->lpszName))
			{
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound)
	{
		// Now check if URL CachePrefix pattern is the same
		if (0 == lstrcmpi(lpszCachePrefix, lpCCI->lpszCachePrefix))
		{
			char	lpBuffer[256];
			DWORD	cbBuffer = sizeof(lpBuffer);

			// Now check if PrefixMap entry is OK
			GetPrefixMapEntry(lpszUniqueVendorName, lpBuffer, cbBuffer);
			
			if (0 == lstrcmpi(lpBuffer, lpszPrefixMap))
				bReturn = TRUE;
			else
				bReturn = FALSE;

			// If both CachePrefix and PrefixMap match
			// then we consider this entry to already exist
			// and is correctly installed.
		}
	}

	if (hEnum)
		lpfnFindCloseUrlCache(hEnum);

	return bReturn;
}

/************************************************************************\
*   FUNCTION: _CreateUrlCacheContainer()
*
*	Wrapper around WININET CreateUrlCacheContainer()
*
*	Parameters:
*
*	REMOVED
*   lpszUserLocalCachePath
*					Don't need to pass it in since can figure it out
*					using GetUrlCacheConfigInfo()
*
*   ADDED
*	lpszPrefixMap	Param added to wrapper, is missing from WININET f()
*					Specifies the location root path of the data
*					provided by the cache container.
*
*	Workaround #1 - Pre-poplulate registry with PrefixMap
*	-----------------------------------------------------
*	In order to work properly must pre-populate registry
*	with the PrefixMap entry. Otherwise WININET CreateUrlCacheContainer()
*	will not install the cache container.
*
*	STEP #1:
*	========
*	Must setup registry entry in
*	HKCU\Software\Microsoft\Windows\CurrentVersion\
*		Internet Settings\Cache\Extensible Cache
*
*	For PrefixMap
*	Key = <Unique Vendor Name>
*		PrefixMap		= <string>
*
*
*	Other Entries include:
*		CacheLimit		= <DWORD>
*		CacheOptions	= <DWORD>
*		CachePath		= <string>
*		CachePrefix		= <string>
*	These should be put there by the call to CreateUrlCacheContainer()
*
*	STEP #2
*	=======
*	Call CreateUrlCacheContainer()
*
*	Locates all the 'workarounds' to one function.
\************************************************************************/
BOOL _CreateUrlCacheContainer(
     IN LPCSTR lpszUniqueVendorName,
     IN LPCSTR lpszCachePrefix,
	 IN LPCSTR lpszPrefixMap,			// New - part of WRAPPER
	 IN LPCSTR lpszVolumeTitle,	        // New - part of WRAPPER
     IN LPCSTR lpszVolumeLabel,         // New - part of WRAPPER
     IN DWORD KBCacheLimit,
     IN DWORD dwContainerType,			// Not used by WININET currently
     IN DWORD dwOptions
     )
{
	// Enough size to get our info first time without having to realloc
    BYTE bBuf[4096];
    LPINTERNET_CACHE_CONFIG_INFO lpCCI = (LPINTERNET_CACHE_CONFIG_INFO) bBuf;
    DWORD cbCEI = sizeof(bBuf);

	DWORD dwError = 0;
	char szCachePath[MAX_PATH];

    DWORD	dwResult = ERROR_SUCCESS;

	if (!WininetLoaded())
		return FALSE;
    
    
    // Figure out local user cache location directory
	if (!lpfnGetUrlCacheConfigInfo(lpCCI, &cbCEI, CACHE_CONFIG_CONTENT_PATHS_FC))
	{
		// Look for ERROR_INSUFFICIENT_BUFFER and allocate enough
		if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
		{
			// BUGBUG: TODO: Handle insufficient buffer case
			// Try again using required size returned in cbCEI
			//lpCCI = new INTERNET_CACHE_CONFIG_INFO[cbCEI];
		}
		else
			dwError = GetLastError();
	}
	else
	{
		if (lpCCI->dwNumCachePaths > 0)
			lstrcpy(szCachePath, lpCCI->CachePaths[0].CachePath);
	}

	// Add Cache Container Unique Vendor Name to CachePath
	// All container content will be stored in this location
	lstrcat(szCachePath, lpszUniqueVendorName);

	// Manually put PrefixMap into Registry
	// HKCU\Software\Microsoft\Windows\CurrentVersion\
	//		Internet Settings\Cache\Extensible Cache
	CreateAdditionalEntries(lpszUniqueVendorName, lpszVolumeTitle, lpszVolumeLabel, lpszPrefixMap);

	// BUGBUG: Currently CreateUrlCacheContainer() fails if the entry
	// already exists. The returned GetLastError() is ERROR_INVALID_PARAM
	// Need to workaround this for now by enumerating the existing
	// cache containers and if found remove it and then re-add it.

	if (!lpfnCreateUrlCacheContainer(lpszUniqueVendorName, lpszCachePrefix,
				szCachePath, KBCacheLimit, dwContainerType,
				dwOptions, NULL, 0))
	{
		BYTE	bBuf[4096];
		LPINTERNET_CACHE_CONTAINER_INFO lpCCI = (LPINTERNET_CACHE_CONTAINER_INFO) bBuf;
		DWORD	cbCEI = sizeof(bBuf);
		DWORD	dwModified = 0;
		HANDLE	hEnum = NULL;
		int		nCount = 0;

		// Assume we failed because cache container already exists
		// Look for our cache container, delete it, and re-create it
		hEnum = lpfnFindFirstUrlCacheContainer(&dwModified, lpCCI, &cbCEI, 0);

		if (0 == lstrcmpi(lpszUniqueVendorName, lpCCI->lpszName))
		{
			// BUGBUG: Need to specify any options?
			if (!lpfnDeleteUrlCacheContainer(lpszUniqueVendorName, 0))
			{
				dwResult = GetLastError();
			}
			else
			{
				CreateAdditionalEntries(lpszUniqueVendorName, lpszVolumeTitle, lpszVolumeLabel, lpszPrefixMap);

				if (!lpfnCreateUrlCacheContainer(lpszUniqueVendorName, lpszCachePrefix,
							szCachePath, KBCacheLimit, dwContainerType,
							dwOptions, NULL, 0))
				{
					dwResult = GetLastError();
				}
			}
		}
		else
		{
			while (hEnum && lpfnFindNextUrlCacheContainer(hEnum, lpCCI, &cbCEI))
			{
				if (0 == lstrcmpi(lpszUniqueVendorName, lpCCI->lpszName))
				{
					if (!lpfnDeleteUrlCacheContainer(lpszUniqueVendorName, 0))
					{
						dwResult = GetLastError();
					}
					else
					{
						CreateAdditionalEntries(lpszUniqueVendorName, lpszVolumeTitle, lpszVolumeLabel, lpszPrefixMap);

						if (!lpfnCreateUrlCacheContainer(lpszUniqueVendorName, lpszCachePrefix,
									szCachePath, KBCacheLimit, dwContainerType,
									dwOptions, NULL, 0))
						{
							dwResult = GetLastError();
						}

						break;
					}
				}

				nCount++;
			}
		}

		if (hEnum)
			lpfnFindCloseUrlCache(hEnum);

	}

	if (dwResult != ERROR_SUCCESS)
		return (FALSE);
	else
		return (TRUE);

//	return lpfnCreateUrlCacheContainer(lpszUniqueVendorName, lpszCachePrefix,
//				szCachePath, KBCacheLimit, dwContainerType,
//				dwOptions, NULL, 0);
}

/************************************************************************\
*   FUNCTION: CreateAdditionalEntries()
*
*	Add the PrefixMap registry entry to the correct location in the
*	registry. A requirement to workaround this param missing from
*	the CreateUrlCacheContainer() WININET API.
*
\************************************************************************/

DWORD CreateAdditionalEntries(LPCSTR lpszUniqueVendorName, LPCSTR lpszVolumeTitle, 
                              LPCSTR lpszVolumeLabel, LPCSTR lpszPrefixMap)
{
    const static char *szKeyPrefixMap   = "PrefixMap";
    const static char *szKeyVolumeLabel	= "VolumeLabel";
    const static char *szKeyVolumeTitle	= "VolumeTitle";
	const static char *szExtCacheRoot = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Extensible Cache";
    
	HKEY hKeyRoot	  = HKEY_CURRENT_USER;	// default to current user
	HKEY hKeyCacheExt = 0;
	HKEY hKeyVendor   = 0;
	DWORD dwDisposition = 0;
	DWORD	dwResult = ERROR_SUCCESS;
    CHAR szCurDir[MAX_PATH];
    CHAR szVolumeLabel[MAX_PATH];

	// Manually put PrefixMap into Registry
	//
	// BUGBUG: cache containers are per user if user profiles are enabled
	// so on NT they are always per user, on Win95 however 
	// Need to use HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE below
	// depending on what's enabled.
	//
	// Hack on top of a Hack for Win95 ONLY
	// Since this entire function is to workaround the lack of a param
	// for PrefixMap in CreateUrlCacheContainer() another hack shouldn't
	// matter since it's only temporary
	// On Win95 need to check this entry
	// HKEY_LOCAL_MACHINE\Network\Logon
	//		UserProfiles=DWORD:00000001
	// which says if UserProfiles are turned on
	// If they are turned on we use HKEY_CURRENT_USER
	// otherwise use HKEY_LOCAL_MACHINE
	
	OSVERSIONINFO	osvInfo;

	memset(&osvInfo, 0, sizeof(osvInfo));
	osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    if (GetVersionEx(&osvInfo))
	{
		if (VER_PLATFORM_WIN32_WINDOWS == osvInfo.dwPlatformId)
		{
			// We're running on Win95 so default to HKLM
			hKeyRoot = HKEY_LOCAL_MACHINE;
		}
		else
			hKeyRoot = HKEY_CURRENT_USER;	// else assume NT and default to HKCU

		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		DWORD dwUserProfiles = 0;

		HKEY hKeyProfiles = 0;

		// But now have to see if User Profiles are enabled
		if ((dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Network\\Logon",
								NULL, KEY_ALL_ACCESS, &hKeyProfiles)) == ERROR_SUCCESS)
		{
			if ((dwResult = RegQueryValueEx(hKeyProfiles, "UserProfiles",
								NULL, &dwType, (unsigned char *)&dwUserProfiles,
								&dwSize)) == ERROR_SUCCESS)
			{
				if ( (dwResult != ERROR_MORE_DATA) &&
					 (1L == dwUserProfiles) )
							hKeyRoot = HKEY_CURRENT_USER;
				else
					hKeyRoot = HKEY_LOCAL_MACHINE;
			}
		}
	}


	if ( (dwResult = RegCreateKeyEx(hKeyRoot, szExtCacheRoot,
			0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &hKeyCacheExt, &dwDisposition)) == ERROR_SUCCESS)
	{
	
		if ( (dwResult = RegOpenKeyEx(hKeyCacheExt, lpszUniqueVendorName,
				0, KEY_ALL_ACCESS, &hKeyVendor)) != ERROR_SUCCESS)
		{
			// Key didn't exist

			// Let's try to create it
			dwResult = RegCreateKeyEx(hKeyCacheExt, lpszUniqueVendorName,
				0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
				NULL, &hKeyVendor, &dwDisposition);

        }    
    
    }

    if (dwResult == ERROR_SUCCESS)
    {
        RegSetValueEx(hKeyVendor, szKeyPrefixMap, 0, REG_SZ,
            (CONST UCHAR *) lpszPrefixMap, lstrlen(lpszPrefixMap)+1);
            
        RegSetValueEx(hKeyVendor, szKeyVolumeLabel, 0, REG_SZ,
            (CONST UCHAR *) lpszVolumeLabel, lstrlen(lpszVolumeLabel)+1);
            
        RegSetValueEx(hKeyVendor, szKeyVolumeTitle, 0, REG_SZ,
            (CONST UCHAR *) lpszVolumeTitle, lstrlen(lpszVolumeTitle)+1);
    }        
    
    
        
	return dwResult;
}

/************************************************************************\
*   FUNCTION: GetPrefixMapEntry()
*
*	Get the PrefixMap registry entry from the correct location in the
*	registry.
*
*	Returns: PrefixMap entry in lpszPrefixMap
*			 or NULL if no enty is found.
*
\************************************************************************/

DWORD GetPrefixMapEntry(LPCSTR lpszUniqueVendorName, LPSTR lpszPrefixMap, DWORD cbPrefixMap)
{
    const static char *szKeyPrefixMap = "PrefixMap";
	const static char *szExtCacheRoot = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Extensible Cache";

	HKEY hKeyRoot	  = HKEY_CURRENT_USER;	// default to current user
	HKEY hKeyCacheExt = 0;
	HKEY hKeyVendor   = 0;
	DWORD dwDisposition = 0;
	unsigned long	ulVal = 0;
	DWORD	dwResult = ERROR_SUCCESS;

	// Manually put PrefixMap into Registry
	//
	// BUGBUG: cache containers are per user if user profiles are enabled
	// so on NT they are always per user, on Win95 however 
	// Need to use HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE below
	// depending on what's enabled.
	//
	// Hack on top of a Hack for Win95 ONLY
	// Since this entire function is to workaround the lack of a param
	// for PrefixMap in CreateUrlCacheContainer() another hack shouldn't
	// matter since it's only temporary
	// On Win95 need to check this entry
	// HKEY_LOCAL_MACHINE\Network\Logon
	//		UserProfiles=DWORD:00000001
	// which says if UserProfiles are turned on
	// If they are turned on we use HKEY_CURRENT_USER
	// otherwise use HKEY_LOCAL_MACHINE
	
	OSVERSIONINFO	osvInfo;

	memset(&osvInfo, 0, sizeof(osvInfo));
	osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&osvInfo))
	{
		if (VER_PLATFORM_WIN32_WINDOWS == osvInfo.dwPlatformId)
		{
			// We're running on Win95 so default to HKLM
			hKeyRoot = HKEY_LOCAL_MACHINE;
		}
		else
			hKeyRoot = HKEY_CURRENT_USER;	// else assume NT and default to HKCU

		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		DWORD dwUserProfiles = 0;

		HKEY hKeyProfiles = 0;

		// But now have to see if User Profiles are enabled
		if ((dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Network\\Logon",
								NULL, KEY_ALL_ACCESS, &hKeyProfiles)) == ERROR_SUCCESS)
		{
			if ((dwResult = RegQueryValueEx(hKeyProfiles, "UserProfiles",
								NULL, &dwType, (unsigned char *)&dwUserProfiles,
								&dwSize)) == ERROR_SUCCESS)
			{
				if ( (dwResult != ERROR_MORE_DATA) &&
					 (1L == dwUserProfiles) )
							hKeyRoot = HKEY_CURRENT_USER;
				else
					hKeyRoot = HKEY_LOCAL_MACHINE;
			}
		}
	}


	if ( (dwResult = RegCreateKeyEx(hKeyRoot, szExtCacheRoot,
			0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &hKeyCacheExt, &dwDisposition)) == ERROR_SUCCESS)
	{
	
		if ( (dwResult = RegOpenKeyEx(hKeyCacheExt, lpszUniqueVendorName,
				0, KEY_ALL_ACCESS, &hKeyVendor)) != ERROR_SUCCESS)
		{
			// Key didn't exist
			lpszPrefixMap[0] = '\0';
		}
		else	// key did exist so lets return it in lpszPrefixMap
		{
			// Vendor name must be unique so is it ok to assume uniqueness?
			if ( (dwResult = RegQueryValueEx(hKeyVendor, szKeyPrefixMap, 0, &ulVal,
				(LPBYTE) lpszPrefixMap, &cbPrefixMap ))
				 == ERROR_SUCCESS )
			{
			}
			else
				lpszPrefixMap[0] = '\0';
		}
	}
	else
		lpszPrefixMap[0] = '\0';

	return dwResult;
}

/************************************************************************\
*   FUNCTION: WriteCacheContainerEntry()
*
* Manually write all the registry entries that WININET CreateUrlCacheContainer
* would normally write.
*
* This f() is used when IE4 WININET is not yet installed.
*
\************************************************************************/

DWORD WriteCacheContainerEntry(
     IN LPCSTR lpszUniqueVendorName,
     IN LPCSTR lpszCachePrefix,
     IN LPCSTR lpszPrefixMap,			// New - part of WRAPPER
     IN LPCSTR lpszVolumeTitle,	        // New - part of WRAPPER
     IN LPCSTR lpszVolumeLabel,	        // New - part of WRAPPER
     IN DWORD KBCacheLimit,
     IN DWORD dwContainerType,			// Not used by WININET currently
     IN DWORD dwOptions
	 )

{
    const static char *szCachePrefix    = "CachePrefix";
    const static char *szKeyPrefixMap   = "PrefixMap";
    const static char *szKeyVolumeLabel	= "VolumeLabel";
    const static char *szKeyVolumeTitle	= "VolumeTitle";
    const static char *szCacheLimit     = "CacheLimit";
    const static char *szCacheOptions   = "CacheOptions";
    const static char *szCachePath      = "CachePath";
	const static char *szExtCacheRoot = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Cache\\Extensible Cache";
    
	HKEY hKeyRoot	  = HKEY_CURRENT_USER;	// default to current user
	HKEY hKeyCacheExt = 0;
	HKEY hKeyVendor   = 0;
	DWORD dwDisposition = 0;
	DWORD	dwResult = ERROR_SUCCESS;
	CHAR lpszCachePath[MAX_PATH];

	OSVERSIONINFO	osvInfo;

	memset(&osvInfo, 0, sizeof(osvInfo));
	osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    if (GetVersionEx(&osvInfo))
	{
		if (VER_PLATFORM_WIN32_WINDOWS == osvInfo.dwPlatformId)
		{
			// We're running on Win95 so default to HKLM
			hKeyRoot = HKEY_LOCAL_MACHINE;
		}
		else
			hKeyRoot = HKEY_CURRENT_USER;	// else assume NT and default to HKCU

		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		DWORD dwUserProfiles = 0;

		HKEY hKeyProfiles = 0;

		BYTE bBuf[4096];
		LPINTERNET_CACHE_CONFIG_INFO lpCCI = (LPINTERNET_CACHE_CONFIG_INFO) bBuf;
		DWORD cbCEI = sizeof(bBuf);

		if (!lpfnGetUrlCacheConfigInfo)
		{
			HINSTANCE	hDll;

			hDll = LoadLibrary("WININET.DLL");

			if (hDll != NULL)
			{
				lpfnGetUrlCacheConfigInfo = (LPFNGETURLCACHECONFIGINFO)GetProcAddress(hDll, "GetUrlCacheConfigInfoA");

				if (!lpfnGetUrlCacheConfigInfo)
				{
					FreeLibrary(hDll);
					dwResult = -1;		// Indicate failure
				}
			}
		}

		if (lpfnGetUrlCacheConfigInfo)
		{
			// Figure out local user cache location directory
			// Note: Need to use IE3 backward compatible flag
			// IE3:   CACHE_CONFIG_DISK_CACHE_PATHS_FC
			// IE4:   CACHE_CONFIG_CONTENT_PATHS_FC
			if (lpfnGetUrlCacheConfigInfo(lpCCI, &cbCEI, CACHE_CONFIG_DISK_CACHE_PATHS_FC))
			{
				// Now need to parse the returned CachePath to remove trailing 'cache1\'
				// "c:\windows\Temporary Internet Files\cache1\"
				// look for backslash starting from end of string
				int i = lstrlen(lpCCI->CachePaths[0].CachePath);

				while( (lpCCI->CachePaths[0].CachePath[i] != '\\') && (i >= 0) )
					   i--;

				if (lpCCI->CachePaths[0].CachePath[i] == '\\')
					lpCCI->CachePaths[0].CachePath[i+1] = '\0';		// Leave '\' intact for later strcat

				if (lpCCI->dwNumCachePaths > 0)
					lstrcpy(lpszCachePath, lpCCI->CachePaths[0].CachePath);

				// Add Cache Container Unique Vendor Name to CachePath
				// All container content will be stored in this location
				lstrcat(lpszCachePath, lpszUniqueVendorName);
			}
		}
		else
		{
			// No IE3 or IE4 WININET present
			// so synthesize CachePath from GetWinDir() + "Temporary Internet Files"

			if ( GetWindowsDirectory(lpszCachePath, MAX_PATH) > 0)
			{
				if ('\\' == lpszCachePath[lstrlen(lpszCachePath)-1])
					lstrcat(lpszCachePath, _T("Temporary Internet Files"));
				else
				{
					lstrcat(lpszCachePath, _T("\\"));
					lstrcat(lpszCachePath, _T("Temporary Internet Files"));
				}
			}

		}

		// But now have to see if User Profiles are enabled
		if ((dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Network\\Logon",
								NULL, KEY_ALL_ACCESS, &hKeyProfiles)) == ERROR_SUCCESS)
		{
			if ((dwResult = RegQueryValueEx(hKeyProfiles, "UserProfiles",
								NULL, &dwType, (unsigned char *)&dwUserProfiles,
								&dwSize)) == ERROR_SUCCESS)
			{
				if ( (dwResult != ERROR_MORE_DATA) &&
					 (1L == dwUserProfiles) )
							hKeyRoot = HKEY_CURRENT_USER;
				else
					hKeyRoot = HKEY_LOCAL_MACHINE;
			}
		}
	}


	if ( (dwResult = RegCreateKeyEx(hKeyRoot, szExtCacheRoot,
			0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &hKeyCacheExt, &dwDisposition)) == ERROR_SUCCESS)
	{
	
		if ( (dwResult = RegOpenKeyEx(hKeyCacheExt, lpszUniqueVendorName,
				0, KEY_ALL_ACCESS, &hKeyVendor)) != ERROR_SUCCESS)
		{
			// Key didn't exist

			// Let's try to create it
			dwResult = RegCreateKeyEx(hKeyCacheExt, lpszUniqueVendorName,
				0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
				NULL, &hKeyVendor, &dwDisposition);

        }    
    
    }

    if (dwResult == ERROR_SUCCESS)
    {
        RegSetValueEx(hKeyVendor, szKeyPrefixMap, 0, REG_SZ,
            (CONST UCHAR *) lpszPrefixMap, lstrlen(lpszPrefixMap)+1);
            
        RegSetValueEx(hKeyVendor, szKeyVolumeLabel, 0, REG_SZ,
            (CONST UCHAR *) lpszVolumeLabel, lstrlen(lpszVolumeLabel)+1);
            
        RegSetValueEx(hKeyVendor, szKeyVolumeTitle, 0, REG_SZ,
            (CONST UCHAR *) lpszVolumeTitle, lstrlen(lpszVolumeTitle)+1);

        RegSetValueEx(hKeyVendor, szCachePrefix, 0, REG_SZ,
            (CONST UCHAR *) lpszCachePrefix, lstrlen(lpszCachePrefix)+1);

        RegSetValueEx(hKeyVendor, szCachePath, 0, REG_SZ,
            (CONST UCHAR *) lpszCachePath, lstrlen(lpszCachePath)+1);

        RegSetValueEx(hKeyVendor, szCacheLimit, 0, REG_DWORD,
            (unsigned char *)&KBCacheLimit, sizeof(DWORD));

        RegSetValueEx(hKeyVendor, szCacheOptions, 0, REG_DWORD,
            (unsigned char *)&dwOptions, sizeof(DWORD));
	
	}        
    
    
	if (dwResult != ERROR_SUCCESS)
		return (FALSE);
	else
		return (TRUE);
}

/************************************************************************\
*    FUNCTION: CacheContainer()
*
*	 Parameters:
*		dwAction - flag indicating what to do
*					CACHE_ACTION_INSTALL
*					CACHE_ACTION_REMOVE
*					CACHE_ACTION_FILL_LB
*
*		hListBox - HWND to ListBox to fill in with Container names
*
*
*		Note:
*				if dwAction == CACHE_ACTION_FILL_LB then if hListBox  
*				is NULL then return TRUE if ALL Containers installed
*				correctly or FALSE if not
*  
*		Additionally create a CDCACHE.INF at the root of the CD-ROM.
*		Typical contents:
*
*			[Add.CacheContainer]
*			<Unique Vendor Name>=<INF Section Name>
*			Encarta 97=EncartaCD
*
*			[INF Section Name]
*			VolumeLabel=<string>
*			VolumeTitle=<string>
*			CachePrefix=<string>
*			CacheRoot=<relative path on CD-ROM of data>
*			KBCacheLimit=<numerical amount in KB>
*			AutoDelete=Yes|No (default)
*			IncludeSubDirs=Yes|No (default)
*			NoDesktopInit=Yes|No (default)
*
*			[EncartaCD]
*			VolumeLabel=MSENCART97
*			VolumeTitle=Microsoft Encarta CD 97
*			CachePrefix=http://www.microsoft.com/encarta
*			CacheRoot=%EXE_ROOT%\data\http
*			KBCacheLimit=500
*			AutoDelete=Yes
*			IncludeSubDirs=Yes
*
*	NOTE: %EXE_ROOT% is a replaceable param that gets set to the
*		path this EXE was ran from, such as E: or E:\BIN
*
*
*	Calls _CreateUrlCacheContainer()
\************************************************************************/
BOOL CacheContainer(DWORD *dwTotal, DWORD *dwInstalled, DWORD dwAction, HWND hListBox)
{
    BOOL	bRet = FALSE;
    BOOL    bVolumeLabel = FALSE;
    DWORD	dwRes = 0;
	HRESULT hr = 0;
	
	int nSectionSize = 4096;	// Limit each INF section to 4K
	char szSections[4096];
	char *lpSections = (char *)szSections;

    const static char *szAddCacheContainerSection = "Add.CacheContainer";
    const static char *szKey_Name			= "Name";
    const static char *szKey_VolumeTitle	= "VolumeTitle";
    const static char *szKey_Prefix			= "CachePrefix";
    const static char *szKey_Root			= "CacheRoot";
    const static char *szKey_CacheLimit		= "KBCacheLimit";
    const static char *szKey_AutoDelete		= "AutoDelete";
    const static char *szKey_IncludeSubDirs = "IncludeSubDirs";
    const static char *szKey_NoDesktopInit	= "NoDesktopInit";
	char szDefault[] = "*Unknown*";
    DWORD len;

	char szInf[STRING_BUFFER_SIZE];
	char szInfPath[MAX_PATH];
	char szContainerName[STRING_BUFFER_SIZE];
	char szCachePrefix[STRING_BUFFER_SIZE];
	char szCacheRoot[MAX_PATH];
	char szPrefixMap[MAX_PATH];
    char szVolumeLabel[MAX_PATH];
    char szMapDrive[4];
    char szVolumeTitle[MAX_PATH];
	char szAutoDelete[STRING_BUFFER_SIZE];
	char szIncludeSubDirs[STRING_BUFFER_SIZE];
	char szNoDesktopInit[STRING_BUFFER_SIZE];

	int			nDefault = 0;
	int			nCacheLimit = 0;
	BOOL		bResult;
	HANDLE		hFile;

#define SIZE_CMD_LINE   2048

    char szBuf[SIZE_CMD_LINE];  // enough for commandline

    // BEGIN NOTE: add vars and values in matching order
    // add a var by adding a new define VAR_NEW_VAR = NUM_VARS++
    const char *szVars[] =
	{
#define VAR_EXE_ROOT     0       // Replace with drive+path (ex. "D:" or "D:\PATH") of this EXE
        "%EXE_ROOT%",

#define VAR_EXE_DRIVE    1       // Replace with drive (ex. "D:") of this EXE
        "%EXE_DRIVE%",

#define NUM_VARS        2
        ""
    };

	int nValBuffSize = MAX_PATH;
    char lpValBuffer[MAX_PATH];
	int nDriveBuffSize = MAX_PATH;
    char lpDriveBuffer[MAX_PATH];
    const char *szValues[NUM_VARS + 1];
    szValues[VAR_EXE_ROOT] = GetINFDir(lpValBuffer, nValBuffSize);
	szValues[VAR_EXE_DRIVE] = GetINFDrive(lpDriveBuffer, nDriveBuffSize);
    szValues[NUM_VARS] = NULL;
    // END NOTE: add vars and values in matching order

	CWaitCursor wait;

	// Look for INF
	//
	LoadString (g_hInst, ID_INFNAME, szInf, sizeof(szInf));
	lstrcpy(szInfPath, GetINFDir(szInfPath, sizeof(szInfPath)) );
	strcat (szInfPath, "\\");
	strcat (szInfPath, szInf);
	strcat (szInfPath, ".INF");
	hFile = CreateFile(szInfPath, GENERIC_READ, FILE_SHARE_READ, NULL,
					   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
		hFile = NULL;

		// Is there a [Add.CacheContainer] section

		// BUGBUG: GetPrivateProfileSection() fails on Win95
		// Workaround for GetPrivateProfileSection() failure on Win95
		szDefault[0] = '\0';
		len = GetPrivateProfileString(szAddCacheContainerSection, NULL, szDefault,
									lpSections, nSectionSize, szInfPath);


		if (!len)
		{
			// no CD-ROM Cache Container sections in INF
			// BUGBUG: Display a message if in NON Silent mode?

			// This is case where AUTORUN.INF has no [Add.Container] section

		}
		else
		{
			// lpBuffer now has list of key strings (as in key=value)
			// final pair terminated with extra NULL
			//
			// Loop through each cache container entry
			while (*lpSections)
			{
				WORD  dResult   = 0;
                
                // Init flags for this container to map-able.
                DWORD dwOptions = INTERNET_CACHE_CONTAINER_MAP_ENABLED;	

				GetPrivateProfileString(szAddCacheContainerSection, lpSections, szDefault,
								szContainerName, STRING_BUFFER_SIZE, szInfPath);

				if (szContainerName)
				{
					(*dwTotal)++;	// Keep track of how many cache containers in INF

					// Build PrefixMap
					//
					// BUGBUG: Default to root?
					lstrcpy(szDefault, "%EXE_ROOT%");
					// Get the PrefixMap entry
					dwRes = GetPrivateProfileString(szContainerName, szKey_Root, szDefault,
											szCacheRoot, MAX_PATH, szInfPath);

					// Replace any %parameters%
					// S_OK indicates that something was expanded
					if (S_OK == (hr = ExpandEntry(szCacheRoot, szBuf, SIZE_CMD_LINE, szVars, szValues)))
						lstrcpy(szPrefixMap, szBuf);
					else
						lstrcpy(szPrefixMap, szCacheRoot);


                    memcpy(szMapDrive, szPrefixMap, 2);
                    memcpy(szMapDrive + 2, "\\", sizeof("\\"));
                    if (GetVolumeInformation(szMapDrive, szVolumeLabel, MAX_PATH, 
                             NULL, NULL, NULL, NULL, 0))
                    {
                        bVolumeLabel = TRUE;
                    }
                    else
                    {
                        *szVolumeLabel = '\0';
                        bVolumeLabel = FALSE;
                    }

					lstrcpy(szDefault, "");
					GetPrivateProfileString(szContainerName, szKey_Prefix, szDefault,
											szCachePrefix, STRING_BUFFER_SIZE, szInfPath);

                    lstrcpy(szDefault, "");
					GetPrivateProfileString(szContainerName, szKey_VolumeTitle, szDefault,
											szVolumeTitle, STRING_BUFFER_SIZE, szInfPath);
 
					// Now trim off trailing backslash '\' from szCachePrefix
					// workaround for #43375
					int i = lstrlen(szCachePrefix);

					if (i > 0)
						if ('\\' == szCachePrefix[i - 1])
							szCachePrefix[i - 1] = '\0';

					// BUGBUG: Should create custom Profile f() to
					// read/return DWORD value rather than int
					nDefault = 500;	// 500K Cache Limit
					nCacheLimit = GetPrivateProfileInt(szContainerName, szKey_CacheLimit,
													   nDefault, szInfPath);

					dResult = GetProfileBooleanWord(szContainerName, szKey_AutoDelete, szInfPath);
					switch (dResult)
					{
						case -1:	// The key did not exist in INF
							break;	// default is No/False for AutoDelete
						case FALSE:
							break;
						case TRUE:
							dwOptions |= INTERNET_CACHE_CONTAINER_AUTODELETE;
							break;
					}

					dResult = GetProfileBooleanWord(szContainerName, szKey_IncludeSubDirs, szInfPath);
					switch (dResult)
					{
						case -1:	// The key did not exist in INF
							break;	// default is Yes/True for IncludeSubDirs
						case FALSE:
							dwOptions |= INTERNET_CACHE_CONTAINER_NOSUBDIRS;	// Don't include subdirs in cacheview
							break;
						case TRUE:
							break;
					}

					dResult = GetProfileBooleanWord(szContainerName, szKey_NoDesktopInit, szInfPath);
					switch (dResult)
					{
						case -1:	// The key did not exist in INF
							break;	// default is No/False for NoDesktopInit
						case FALSE:
							break;
						case TRUE:
							dwOptions |= INTERNET_CACHE_CONTAINER_NODESKTOPINIT;
							break;
					}


					switch (dwAction)
					{
					case CACHE_ACTION_INSTALL:
						// Call CreateUrlCacheContainer WRAPPER
                        if (bVolumeLabel)
                        {
                            bRet = _CreateUrlCacheContainer(lpSections, szCachePrefix, szPrefixMap, 
                                                            szVolumeTitle, szVolumeLabel, nCacheLimit, 0, dwOptions);
                        }
                        else
                        {
                            bRet = FALSE;
                        }

						break;
					case CACHE_ACTION_REMOVE:
						if (!WininetLoaded())
							return FALSE;

						bRet = lpfnDeleteUrlCacheContainer(lpSections, dwOptions);
						break;
					case CACHE_ACTION_FILL_LB:
						// Fill listbox hListBox

						if (hListBox)
						{
							SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)lpSections);
						}
						else
						{
							// hListBox is NULL
							//
							//	if dwAction == CACHE_ACTION_FILL_LB then if hListBox  
							//	is NULL then return TRUE if ALL Containers installed
							//	correctly or FALSE if not
							//

							if (UrlCacheContainerExists(lpSections, szCachePrefix, szPrefixMap))
								bRet = TRUE;
							else
								return FALSE;	// One container is not installed so bail out
						}

						break;
					case CACHE_ACTION_MAKE_REG_ENTRIES:
                        if (bVolumeLabel)
                        {
						    bRet = WriteCacheContainerEntry(lpSections, szCachePrefix, szPrefixMap, szVolumeTitle, 
                                                            szVolumeLabel, nCacheLimit, 0, dwOptions);
                        }
                        else
                            bRet = FALSE;


						break;
					}

					if (bRet)
						(*dwInstalled)++;	// Keep track of successful installs
				}
				//else empty section entry, ignore and move to next
				
				// Get Next Section name
				while ( (*(lpSections++) != '\0')  );

			}
		}
	}
	else
	{
		// Couldn't find INF file
		// BUGBUG: need to do anything else here?
	}

	return bRet;
}

/************************************************************************\
*    FUNCTION: ExpandEntry()
*
* Borrowed from urlmon\download\hooks.cxx
\************************************************************************/
HRESULT	ExpandEntry(
    LPSTR szSrc,
    LPSTR szBuf,
    DWORD cbBuffer,
    const char * szVars[],
    const char * szValues[])
{
	//Assert(szSrc);

    HRESULT hr = S_FALSE;

    LPSTR pchSrc = szSrc;     // start parsing at begining of cmdline

    LPSTR pchOut = szBuf;       // set at begin of out buffer
    DWORD cbLen = 0;

    while (*pchSrc) {

        // look for match of any of our env vars
        if (*pchSrc == '%') {

            HRESULT hr1 = ExpandVar(pchSrc, pchOut, cbLen, // all passed by ref!
                cbBuffer, szVars, szValues);  

            if (FAILED(hr1)) {
                hr = hr1;
                goto Exit;
            }


            if (hr1 == S_OK) {    // expand var expanded this
                hr = hr1;
                continue;
            }
        }
            
        // copy till the next % or nul
        if ((cbLen + 1) < cbBuffer) {

            *pchOut++ = *pchSrc++;
            cbLen++;

        } else {

            // out of buffer space
            *pchOut = '\0'; // term
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;

        }


    }

    *pchOut = '\0'; // term


Exit:

    return hr;

}

/************************************************************************\
*    FUNCTION: ExpandVar()
*
* Borrowed from urlmon\download\hooks.cxx
\************************************************************************/
HRESULT ExpandVar(
    LPSTR& pchSrc,          // passed by ref!
    LPSTR& pchOut,          // passed by ref!
    DWORD& cbLen,           // passed by ref!
    DWORD cbBuffer,
    const char * szVars[],
    const char * szValues[])
{
    HRESULT hr = S_FALSE;
    int cbvar = 0;

    //Assert (*pchSrc == '%');

    for (int i=0; szVars[i] && (cbvar = lstrlen(szVars[i])) ; i++) { // for each variable

        int cbneed = 0;

        if ( (szValues[i] == NULL) || !(cbneed = lstrlen(szValues[i])))
            continue;

        cbneed++;   // add for nul

        if (0 == strncmp(szVars[i], pchSrc, cbvar)) {

            // found something we can expand

                if ((cbLen + cbneed) >= cbBuffer) {
                    // out of buffer space
                    *pchOut = '\0'; // term
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    goto Exit;
                }

                lstrcpy(pchOut, szValues[i]);
                cbLen += (cbneed -1); //don't count the nul

                pchSrc += cbvar;        // skip past the var in pchSrc
                pchOut += (cbneed -1);  // skip past dir in pchOut

                hr = S_OK;
                goto Exit;

        }
    }

Exit:

    return hr;
    
}

// Return drive+path without trailing backslash
LPSTR GetINFDir(LPSTR lpBuffer, int nBuffSize)
{
	// Figure out what directory we've been started in
	GetModuleFileName(g_hInst, lpBuffer, nBuffSize);

	// Now trim off trailing backslash '\' if any
	int i = lstrlen(lpBuffer);

	if (i > 0)
		if ('\\' == lpBuffer[i - 1])
			lpBuffer[i - 1] = '\0';

	// Get rid of executable name
	i = lstrlen(lpBuffer);

	while( (lpBuffer[i] != '\\') && (i >= 0) )
		   i--;

	if (lpBuffer[i] == '\\')
		lpBuffer[i] = '\0';


	return lpBuffer;
}


// Return drive without trailing backslash
LPSTR GetINFDrive(LPSTR lpBuffer, int nBuffSize)
{
	// Figure out what directory we've been started in
	GetModuleFileName(g_hInst, lpBuffer, nBuffSize);

	if (!lpBuffer)
		return NULL;

	LPSTR lpSaveBuffer = lpBuffer;

	// Now trim off everything after first colon ':'
	if (':' == lpBuffer[1])
		lpBuffer[2] = '\0';
	else
	{
		// assumption that lpBuffer of form "D:\path" failed
		// so actually parse it
		// #48022 robgil - add check for end of lpBuffer string
		while (*lpBuffer != '\0' && *lpBuffer != ':')
			lpBuffer++;

		if (':' == *lpBuffer)
			*(lpBuffer + 1) = '\0';
		else
		{
			// #48022
			// Need to return \\server\share
			// for Drive when a UNC path
			lpBuffer = lpSaveBuffer;

			if ('\\' == lpBuffer[0] && '\\' == lpBuffer[1])
			{
				lpBuffer += 2;	// move past leading '\\'

				while (*lpBuffer != '\0' && *lpBuffer != '\\')
					lpBuffer++;

				if ('\\' == *lpBuffer)
				{
					lpBuffer++;

					while (*lpBuffer != '\0' && *lpBuffer != '\\')
						lpBuffer++;

					if ('\\' == *lpBuffer)
						*lpBuffer = '\0';
				}
			}

		}

	}

	return lpSaveBuffer;
}


//------------------------------------------------------------------------
//  BOOL GetProfileBooleanWord
//
//  Description:
//     Retrieves the value associated with szKeyName and
//     evaluates to a TRUE or FALSE.  If a value is not
//     associated with the key, -1 is returned.
//
//  Parameters:
//     LPSTR szKeyName
//        pointer to key name
//
//  Return Value:
//     WORD
//        -1, if a setting for the given key does not exist
//        TRUE, if value evaluates to a "positive" or "true"
//        FALSE, otherwise
//
//------------------------------------------------------------------------

WORD GetProfileBooleanWord
(
	LPCTSTR			szIniSection,
	LPCTSTR         szKeyName,
    LPCTSTR			szIniFile
)
{
	TCHAR	szTemp[10];

	GetPrivateProfileString( szIniSection,
							 szKeyName, _T(""), szTemp, sizeof( szTemp ),
                             szIniFile ) ;

	if (0 == lstrlen( szTemp ))
		return ( (WORD) -1 ) ;

	if ((0 == lstrcmpi( szTemp, gszIniValTrue )) ||
	   (0 == lstrcmpi( szTemp, gszIniValYes )) ||
	   (0 == lstrcmpi( szTemp, gszIniValOn )))
		return ( TRUE ) ;

	// Try and convert something numeric
	if (0 != _ttoi(szTemp))		// atoi (via tchar.h)
		return ( TRUE );

	return ( FALSE ) ;

} // end of GetProfileBooleanWord()

