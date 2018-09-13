#include "iexplore.h"
#include "unixstuff.h"

//
// BOOL ConnectRemoteIE(LPTSTR pszCommandLine)
//
// This function will be called when -remote parameter is specified during the 
// invokation of IE. That's the same format as Netscape uses, see 
// http://home.netscape.com/newsref/std/x-remote.html. 
// For now, the only special action supported is openURL(URL), because we need 
// it for the NetShow. We just put it as the URL was specified as iexplorer 
// param. To be done - connection to the existent browser.
// Returns TRUE if succeed to connect to the existent browser.
//
#define c_szSpace TEXT(' ')

static BOOL IsOpenURL(LPCTSTR pszBeginCommand, LPCTSTR pszEndCommand, LPTSTR pszURL)
{
    const TCHAR c_szOpenURL[] = TEXT("openURL");
    const TCHAR c_szLBracket  = TEXT('(');
    const TCHAR c_szRBracket  = TEXT(')');
    const TCHAR c_szSQuote    = TEXT('\'');
    const TCHAR c_szDQuote  = TEXT('\"');
    LPCTSTR pszBeginURL, pszEndURL;
    BOOL bRet = TRUE;

    // Skip the leading/trailing spaces.
    while (*pszBeginCommand == c_szSpace) pszBeginCommand++;
    while ((*pszEndCommand == c_szSpace) && (pszBeginCommand <= pszEndCommand))
        pszEndCommand--;

    // Now, parse the value and replace in the cmd line, 
    // if there is openURL there. More formats later...
    if (StrCmpNI(pszBeginCommand, c_szOpenURL, lstrlen(c_szOpenURL)) ||
        (*pszEndCommand != c_szRBracket)) {
        pszBeginURL = pszBeginCommand;
        bRet = FALSE;
	pszEndURL = pszEndCommand;
    }
    else{
        pszBeginURL = pszBeginCommand+lstrlen(c_szOpenURL);
	while (*pszBeginURL == c_szSpace) pszBeginURL++;    
	if ((*pszBeginURL != c_szLBracket) || 
	    (pszBeginURL == pszEndCommand-1)) {
	    pszURL[0] = '\0';
	    return FALSE;
	}
	pszBeginURL++;
	pszEndURL = pszEndCommand-1;
    }

    // Skip the leading/trailing spaces.
    while (*pszBeginURL == c_szSpace) pszBeginURL++;    
    while (*pszEndURL == c_szSpace) pszEndURL--;

    // Take off quotes.
    if (((*pszBeginURL == c_szSQuote) && (*pszEndURL == c_szSQuote)) || 
	((*pszBeginURL == c_szDQuote) && (*pszEndURL == c_szDQuote))) {
        while (*pszBeginURL == c_szSpace) pszBeginURL++;    
	while (*pszEndURL == c_szSpace) pszEndURL--;
	if (pszBeginURL >= pszEndURL) {
	    pszURL[0] = '\0';
	    return FALSE;
	}
    }

    StrCpyN(pszURL, pszBeginURL, (pszEndURL-pszBeginURL)/sizeof(TCHAR) +2); 
    if (bRet) 
        bRet = pszURL[0];

    return bRet;
}


static BOOL ConnectExistentIE(LPCTSTR pszURL, HINSTANCE hInstance)
{
    HWND hwnd; 
   
    if (hwnd = FindWindow(IEREMOTECLASS, NULL))
    {
        COPYDATASTRUCT cds;
        cds.dwData = IEREMOTE_CMDLINE;
        cds.cbData = pszURL ? (lstrlen(pszURL)+1)*sizeof(TCHAR) : 0;
        cds.lpData = pszURL;
        SetForegroundWindow(hwnd);
        SendMessage(hwnd, WM_COPYDATA, (WPARAM)WMC_DISPATCH, (LPARAM)&cds);
	ExitProcess(0);
    }
    return FALSE;
}

BOOL ConnectRemoteIE(LPTSTR pszCmdLine, HINSTANCE hInstance)
{
    const TCHAR c_szDblQuote  = TEXT('"');
    const TCHAR c_szQuote     = TEXT('\'');

    LPTSTR pszBeginRemote, pszEndRemote;
    LPTSTR pszBeginCommand, pszEndCommand;
    TCHAR  szURL[INTERNET_MAX_URL_LENGTH];
    TCHAR  szRestCmdLine[INTERNET_MAX_URL_LENGTH * 2];

    // If we start with a quote, finish with a quote.
    // If we start with something else, finish 1 symbol before space
    // or end of string.
    pszBeginRemote = pszBeginCommand = pszCmdLine;
    
    if (*pszBeginCommand == c_szQuote || *pszBeginCommand == c_szDblQuote) {
        pszEndRemote = pszEndCommand = StrChr(pszBeginCommand+1, (WORD)(*pszBeginCommand));
        pszBeginCommand++;       
    }
    else {
        pszEndCommand = StrChr(pszBeginCommand, (WORD)c_szSpace);
        if (pszEndCommand == NULL)
            pszEndCommand = pszBeginCommand+lstrlen(pszBeginCommand);
       pszEndRemote = pszEndCommand-1;
    }

    if ((pszEndCommand == NULL) || (lstrlen(pszBeginCommand) <= 1))
        return FALSE;
    pszEndCommand--;

    //
    // Now, check the remote command and execute.
    // For now, we just replace the URL in the cmd line, 
    // if there is openURL there. More formats later...
    IsOpenURL(pszBeginCommand, pszEndCommand, szURL);
    if (ConnectExistentIE(szURL, hInstance))
        return TRUE;
    StrCpyN(szRestCmdLine, pszEndRemote+1, ARRAYSIZE(szRestCmdLine));
    *pszBeginRemote = '\0';   
    StrCat(pszCmdLine, szURL);
    StrCat(pszCmdLine, szRestCmdLine);

    // No connection with an existent IE was done.
    return FALSE;

}

#if 0
#define WMC_UNIX_NEWWINDOW            (WM_USER + 0x0400)
BOOL RemoteIENewWindow(LPTSTR pszCmdLine)
{
    HWND hwnd; 
    LPTSTR pszCurrent = pszCmdLine;

    while (*pszCurrent == TEXT(' '))
        pszCurrent++;
    if (*pszCurrent == TEXT('-'))
        return FALSE;
   
    if (hwnd = FindWindow(IEREMOTECLASS, NULL))
    {
        COPYDATASTRUCT cds;
        cds.dwData = IEREMOTE_CMDLINE;
        cds.cbData = pszCmdLine ? (lstrlen(pszCmdLine)+1)*sizeof(TCHAR) : 0;
        cds.lpData = pszCmdLine;
        SetForegroundWindow(hwnd);
        SendMessage(hwnd, WM_COPYDATA, (WPARAM)WMC_UNIX_NEWWINDOW, (LPARAM)&cds);
	printf("Opening a new window in the currently running Internet Explorer.\n");
	printf("To start a new instance of Internet Explorer, type \"iexplorer -new\".\n");
	return TRUE;
    }
    return FALSE;    
}
#endif

// Entry point for Mainwin is WinMain so create this function and call
// ModuleEntry() from here.

#if defined(MAINWIN)
EXTERN_C int _stdcall ModuleEntry(void);

EXTERN_C int WINAPI WinMain( HINSTANCE hinst, HINSTANCE hprev, LPSTR lpcmdline, int cmd )
{
        return ModuleEntry ();
}
#endif

