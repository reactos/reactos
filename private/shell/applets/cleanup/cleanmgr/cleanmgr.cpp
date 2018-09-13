/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    cleanmgr.cpp
**
** Purpose: WinMain for the Disk Cleanup applet.
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"

#define CPP_FUNCTIONS
#include "crtfree.h"

#include "dmgrinfo.h"

#include "diskguid.h"
#include "resource.h"
#include "textout.h"
#include "dmgrdlg.h"
#include "msprintf.h"
#include "diskutil.h"
#include "seldrive.h"
#include "drivlist.h"
/*
**------------------------------------------------------------------------------
** Global Defines
**------------------------------------------------------------------------------
*/
#define SWITCH_HIDEUI               'N'
#define SWITCH_HIDEMOREOPTIONS      'M'
#define SWITCH_DRIVE                'D'

#define SZ_SAGESET                  TEXT("/SAGESET")
#define SZ_SAGERUN                  TEXT("/SAGERUN")
#define SZ_TUNEUP                   TEXT("/TUNEUP")

/*
**------------------------------------------------------------------------------
** Global variables
**------------------------------------------------------------------------------
*/
HINSTANCE   g_hInstance = NULL;
HWND        g_hDlg = NULL;
BOOL        g_bAlreadyRunning = FALSE;

/*
**------------------------------------------------------------------------------
** ParseCommandLine
**
** Purpose:    Parses command line for switches
** Parameters:
**    lpCmdLine command line string
**    pdwFlags  pointer to flags DWORD
**    pDrive    pointer to a character that the drive letter
**              is returned in
** Return:     TRUE if command line contains /SAGESET or
**              /SAGERUN
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
BOOL
ParseCommandLine(
    LPTSTR  lpCmdLine,
    PDWORD  pdwFlags,
    PULONG  pulProfile
    )
{
    LPTSTR  lpStr = lpCmdLine;
    BOOL    bRet = FALSE;
    int     i;
    TCHAR   szProfile[4];

    *pulProfile = 0;

    //
    //Look for /SAGESET:n on the command line
    //
    if ((lpStr = StrStrI(lpCmdLine, SZ_SAGESET)) != NULL)
    {
        lpStr += lstrlen(SZ_SAGESET);
        if (*lpStr && *lpStr == ':')
        {
            lpStr++;
            i = 0;
            while (*lpStr && *lpStr != ' ' && i < 4)
            {
                szProfile[i] = *lpStr;
                lpStr++;
                i++;
            }

            *pulProfile = StrToInt(szProfile);
        }

        *pdwFlags = FLAG_SAGESET;
        bRet = TRUE;
    }

    //
    //Look for /SAGERUN:n on the command line
    //
    else if ((lpStr = StrStrI(lpCmdLine, SZ_SAGERUN)) != NULL)
    {
        lpStr += lstrlen(SZ_SAGERUN);
        if (*lpStr && *lpStr == ':')
        {
            lpStr++;
            i = 0;
            while (*lpStr && *lpStr != ' ' && i < 4)
            {
                szProfile[i] = *lpStr;
                lpStr++;
                i++;
            }

            *pulProfile = StrToInt(szProfile);
        }

        *pdwFlags = FLAG_SAGERUN;
        bRet = TRUE;
    }

    //
    //Look for /TUNEUP:n
    //
    else if ((lpStr = StrStrI(lpCmdLine, SZ_TUNEUP)) != NULL)
    {
        lpStr += lstrlen(SZ_TUNEUP);
        if (*lpStr && *lpStr == ':')
        {
            lpStr++;
            i = 0;
            while (*lpStr && *lpStr != ' ' && i < 4)
            {
                szProfile[i] = *lpStr;
                lpStr++;
                i++;
            }

            *pulProfile = StrToInt(szProfile);
        }

        *pdwFlags = FLAG_TUNEUP | FLAG_SAGESET;
        bRet = TRUE;
    }

    return bRet;
}

/*
**------------------------------------------------------------------------------
** ParseForDrive
**
** Purpose:    Parses command line for switches
** Parameters:
**    lpCmdLine command line string
**    pDrive    Buffer that the drive string will be returned
**              in, the format will be x:\
** Return:     TRUE on sucess
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
BOOL 
ParseForDrive(
    LPTSTR lpCmdLine,
    PTCHAR pDrive
    )
{
    LPTSTR  lpStr = lpCmdLine;

    GetBootDrive(pDrive, 4);

    while (*lpStr)
    {
        //
        //Did we find a '-' or a '/'?
        //
        if ((*lpStr == '-') || (*lpStr == '/'))
        {
            lpStr++;

            //
            //Is this the Drive switch?
            //
            if (*lpStr && (toupper(*lpStr) == SWITCH_DRIVE))
            {
                //
                //Skip any white space
                //
                                lpStr++;
                while (*lpStr && *lpStr == ' ')
                                        lpStr++;

                //
                //The next character is the driver letter
                //
                if (*lpStr)
                {
                    pDrive[0] = (TCHAR)toupper(*lpStr);
                    pDrive[1] = ':';
                    pDrive[2] = '\\';
                    pDrive[3] = '\0';
                    return TRUE;
                }
            }
        }

        lpStr++;
    }

    return FALSE;
}

BOOL CALLBACK EnumWindowsProc(
    HWND hWnd,
    LPARAM lParam
    )
{
    TCHAR   szWindowTitle[260];

    GetWindowText(hWnd, szWindowTitle, sizeof(szWindowTitle));
    if (StrCmp(szWindowTitle, (LPTSTR)lParam) == 0)
    {
        MiDebugMsg((0, "There is already an instance of cleanmgr.exe running on this drive!"));
        SetForegroundWindow(hWnd);
        g_bAlreadyRunning = TRUE;
        return FALSE;
    }

    return TRUE;
}

int APIENTRY
WinMainT(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine,
    int nCmdShow
    )
{
    DWORD           dwFlags = 0;
    CleanupMgrInfo  *pcmi = NULL;
    TCHAR           szDrive[4];
    ULONG           ulProfile = 0;
    WNDCLASS        cls;
    TCHAR           *psz;
    TCHAR           szVolumeName[MAX_PATH];
    int             RetCode = RETURN_SUCCESS;

    g_hInstance = hInstance;

    InitCommonControls();

    //
    //Initialize support classes
    //
    CleanupMgrInfo::Register(hInstance);

    //
    //Register the CLEANMGR class
    //
    if (hPrevInstance == NULL)
    {
        cls.lpszClassName  = SZ_CLASSNAME;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_CLEANMGR));
        cls.lpszMenuName   = NULL;
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInstance;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = DefDlgProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = DLGWINDOWEXTRA;
        if (!RegisterClass(&cls))
            return FALSE;
    }

    //
    //Parse the command line
    //
    ParseCommandLine(lpCmdLine, &dwFlags, &ulProfile);

    if (!ParseForDrive(lpCmdLine, szDrive) && 
        !(dwFlags & FLAG_SAGESET) &&
        !(dwFlags & FLAG_SAGERUN))
    {
PromptForDisk:
        if (!SelectDrive(szDrive, dvlANYLOCAL))
            goto Cleanup_Exit;
    }
    
    //
    //Create window title for comparison
    //
    if (dwFlags & FLAG_SAGESET)
    {
        psz = SHFormatMessage( MSG_APP_SETTINGS_TITLE );
    }
    else
    {
        GetVolumeInformation(szDrive, szVolumeName, ARRAYSIZE(szVolumeName), NULL, NULL, NULL, NULL, 0);
        psz = SHFormatMessage( MSG_APP_TITLE, szVolumeName, szDrive[0] );
    }

    EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)psz);
    
    LocalFree(psz);

    if (g_bAlreadyRunning)
    {
        return FALSE;
    }

    if (dwFlags & FLAG_SAGERUN)
    {
        TCHAR   c;

        szDrive[1] = TCHAR(':');
        szDrive[2] = TCHAR('\\');
        szDrive[3] = TCHAR('\0');

        for (c = 'A'; c <= 'Z'; c++)
        {
            szDrive[0] = c;

            //
            //Create CleanupMgrInfo object for this drive
            //
            pcmi = new CleanupMgrInfo(szDrive, dwFlags, ulProfile);
            if (pcmi != NULL && pcmi->isAbortScan() == FALSE  && pcmi->isValid())
            {
                pcmi->purgeClients();
            }

            //
            //Destroy the CleanupMgrInfo object for this drive
            //
            if (pcmi)
            {
                RetCode = pcmi->dwReturnCode;
                delete pcmi;
                pcmi = NULL;
            }
        }
    }

    else
    {
        //
        //Create CleanupMgrInfo object
        //
        pcmi = new CleanupMgrInfo(szDrive, dwFlags, ulProfile);
        if (pcmi != NULL && pcmi->isAbortScan() == FALSE)
        {
            //
            //User specified an invalid drive letter
            //
            if (!(pcmi->isValid()))
            {
                // dismiss the dialog first
                if ( pcmi->hAbortScanWnd )
                {
                    pcmi->bAbortScan = TRUE;

                    //
                    //Wait for scan thread to finish
                    //  
                    WaitForSingleObject(pcmi->hAbortScanThread, INFINITE);

                    pcmi->bAbortScan = FALSE;
                }
                
                TCHAR   szWarningTitle[256];
                TCHAR   *pszWarning;
                pszWarning = SHFormatMessage( MSG_BAD_DRIVE_LETTER, szDrive );
                LoadString(g_hInstance, IDS_TITLE, szWarningTitle, ARRAYSIZE(szWarningTitle));

                MessageBox(NULL, pszWarning, szWarningTitle, MB_OK | MB_SETFOREGROUND);
                LocalFree(pszWarning);

                if (pcmi)
                {
                    delete pcmi;
                    pcmi = NULL;
                    goto PromptForDisk;
                }
            }

            //
            //Bring up the main dialog
            //
            else
            {
                int nResult;

                nResult = DisplayCleanMgrProperties(NULL, (LPARAM)pcmi);

                if (nResult)
                {
                    pcmi->dwUIFlags |= FLAG_SAVE_STATE;
                
                    //
                    //Need to purge the clients if we are NOT
                    //in the SAGE settings mode.
                    //
                    if (!(dwFlags & FLAG_SAGESET) && !(dwFlags & FLAG_TUNEUP)  && pcmi->bPurgeFiles)
                        pcmi->purgeClients();
                }   
            }
        }

        //
        //Destroy the CleanupMgrInfo object
        //
        if (pcmi)
        {
            RetCode = pcmi->dwReturnCode;
            delete pcmi;
            pcmi = NULL;
        }
    }

    //
    //Cleanup support classes
    //
Cleanup_Exit:
    CleanupMgrInfo::Unregister();

    return RetCode;
}


STDAPI_(int) ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPTSTR pszCmdLine;

    pszCmdLine = GetCommandLine();

    //
    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail
    //

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    // Since we now have a way for an extension to tell us when it is finished,
    // we will terminate all processes when the main thread goes away.

    return i;
}

void _cdecl main()
{
    ModuleEntry();
}
