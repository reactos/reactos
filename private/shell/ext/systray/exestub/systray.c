/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       SYSTRAY.C
*
*  VERSION:     2.0
*
*  AUTHOR:      TCS/RAL
*
*  DATE:        08 Feb 1994
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  08 Feb 1994 TCS Original implementation.
*  11 Nov 1994 RAL Converted from batmeter to systray
*  11 Aug 1995 JEM Split batmeter functions into power.c & minor enahncements
*  23 Oct 1995 Shawnb Unicode enabled
*  07 Aug 1998 dsheldon Created systray.dll and made this into a stub exe
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <systrayp.h>
#include <initguid.h>
#include <stclsid.h>

//  Global instance handle of this application.
HINSTANCE g_hInstance;

static const TCHAR g_szWindowClassName[]     = SYSTRAY_CLASSNAME;

/**************************************************************************/

INT intval(LPCTSTR lpsz)
{
    INT i = 0;
    while (*lpsz >= TEXT ('0') && *lpsz <= TEXT ('9'))
    {
        i = i * 10 + (int)(*lpsz - TEXT ('0'));
        lpsz++;
    }
    return(i);
}

// stolen from the CRT, used to shrink our code

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine ();

    if ( *pszCmdLine == TEXT ('\"') )
    {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != TEXT ('\"')) )
            ;

        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT ('\"') )
            pszCmdLine++;
    }
    else
    {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' '))
    {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo (&si);

    i = WinMain(GetModuleHandle(NULL), NULL, (LPSTR)pszCmdLine,
                si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;    // We never come here.
}



/*******************************************************************************
*
*  WinMain
*
*  DESCRIPTION:
*
*  PARAMETERS:
*       if lpCmdLine contains an integer value then we'll enable that service
*
*******************************************************************************/
int
    PASCAL
    WinMain(
           HINSTANCE hInstance,
           HINSTANCE hPrevInstance,
           LPSTR lpszCmdLine,
           int nCmdShow
           )
{
    HWND hWnd;

    HWND hExistWnd = FindWindow(g_szWindowClassName, NULL);
    UINT iEnableServ = intval((LPTSTR)lpszCmdLine);
    g_hInstance = hInstance;

    if (hExistWnd)
    {
        //
        // NOTE: Send an enable message even if the command line parameter
        //       is 0 to force us to re-check for all enabled services.
        //
        PostMessage(hExistWnd, STWM_ENABLESERVICE, iEnableServ, TRUE);
        goto ExitMain;
    }
    else
    {
        int i;

        // We have to inject systray.dll into the explorer process
        if (SUCCEEDED(SHLoadInProc(&CLSID_SysTrayInvoker)))
        {
            // Wait for up to 30 seconds for the window to be created, 
            // send our message every second
        
            for (i = 0; i < 30; i ++)
            {
                Sleep(1000);
                hExistWnd = FindWindow(g_szWindowClassName, NULL);
                if (hExistWnd)
                {
                    PostMessage(hExistWnd, STWM_ENABLESERVICE, iEnableServ, TRUE);
                    goto ExitMain;        
                }
            }
        }
    }

ExitMain:
    return 0;
}

