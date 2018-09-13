/****************************** Module Header ******************************\
* Module Name: access.c
*
* Copyright (c) 1997, Microsoft Corporation
*
* Accessibility notification dialogs
*
* History:
* 02-01-97  Fritz Sands   Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Notification Dialog Stuff
 */

#define cchBuf 1024                       // plenty of room for title
#define cchTitle 128
typedef struct tagACCESSINFO {
    UINT  Feature;
    UINT  TitleID;
    PTERMINAL pTerm;
    BOOL NoHotKeys;
    HANDLE hDesk;
    WCHAR  wcTitle[cchTitle];
	UINT uID;
} ACCESSINFO, *PACCESSINFO;

BOOL IsNotifReq(ACCESSINFO*, PWINDOWSTATION);
void SaveUidSettings(ACCESSINFO* pAccessInfo);

#define NOTIF_KEY                __TEXT("Control Panel\\Accessibility")
#define NOTIFY_VALUE   __TEXT("Warning Sounds")

#define HOTKEYCODE                    100
#define UTILMANSERVICE_NAME     __TEXT("UtilMan")
#define UM_SERVICE_CONTROL_SHOWDIALOG 128
#define UTILMAN_START_BYHOTKEY   __TEXT("/Hotkey") 

#define ID_STICKYKEYNAME	NOTIF_KEY __TEXT("\\StickyKeys")
#define	ID_TOGGLEKEYS		NOTIF_KEY __TEXT("\\ToggleKeys")
#define ID_HIGHCONTROST		NOTIF_KEY __TEXT("\\HighContrast")
#define	ID_MOUSEKEYS		NOTIF_KEY __TEXT("\\MouseKeys")
#define	ID_SERIALKEYS		NOTIF_KEY __TEXT("\\SerialKeys")

/***************************************************************************
 *                                                                         *
 * ConfirmHandler_InitDialog                                               *
 *                                                                         *
 * Input: hWnd = dialog window handle                                      *
 *                  uiTitle = resource ID of dialog box title              *
 *                  uiTitle+1 through uiTitle+n = resource ID of dialog box text *
 * Output: Returns TRUE on success, FALSE on failure.                      *
 *                                                                         *
 ***************************************************************************/

BOOL ConfirmHandler_InitDialog(HWND hWnd, HDESK hDesk, UINT uiTitle, WCHAR *pszTitle) {
    RECT    rc;   // Current window size
    WCHAR *pszBuf;
    WCHAR *pszNext;
    int cchBufLeft;
    int cchHelpText;
    int fSuccess = 0;
    WCHAR szDesktop[MAX_PATH];
    int Len1 = MAX_PATH;
    BOOL b;

    szDesktop[0] = 0;
    b = GetUserObjectInformation(hDesk, UOI_NAME, szDesktop, MAX_PATH, &Len1);
    SetWindowText(hWnd, pszTitle);                                    // Init title bar

    pszBuf = (WCHAR *)LocalAlloc(LMEM_FIXED, cchBuf * sizeof (WCHAR));
    if (!pszBuf) goto Exit;

    pszNext = pszBuf; cchBufLeft = cchBuf;
    while (cchHelpText = LoadString(g_hInstance, ++uiTitle, pszNext, cchBufLeft)) {
        pszNext += cchHelpText;
        cchBufLeft -= cchHelpText;
    }

    // Everybody gets a "do you want to continue?" appended.
    if (!LoadString(g_hInstance, ID_CONTINUE, pszNext, cchBufLeft)) goto FreeExit;

    SetDlgItemText(hWnd, ID_HELPTEXT, pszBuf);       // Init help text

    if (b && (0 == wcscmp(szDesktop,L"Winlogon"))) {
        EnableWindow(GetDlgItem(hWnd, IDHELP), FALSE);

    }

// Make us a topmost window and center ourselves.

    GetWindowRect(hWnd, &rc);                                               // Get size of dialog

// Center dialog and make it topmost
    SetWindowPos(hWnd,
                 HWND_TOPMOST,
                 (GetSystemMetrics(SM_CXFULLSCREEN)/2) - (rc.right - rc.left)/2,
                 (GetSystemMetrics(SM_CYFULLSCREEN)/2) - (rc.bottom - rc.top)/2,
                 0,0, SWP_NOSIZE );

       // Make sure we're active!
// Lets try setting this to be the foreground window.
	// SetForegroundWindow(hWnd);

    fSuccess = 1;
FreeExit:
    LocalFree((HLOCAL)pszBuf);
Exit:
    return fSuccess;
}

/***************************************************************************
 *                                                                         *
 *                                                                         *
 * ConfirmHandler                                                          *
 *                                                                         *
 * Input: Std Window messages                                              *
 * Output: IDOK if success, IDCANCEL if we should abort                    *
 *                                                                         *
 *                                                                         *
 * Put up the main dialog to tell the user what is happening and to get    *
 * permission to continue.                                                 *
 ***************************************************************************/


INT_PTR CALLBACK ConfirmHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR buf[100];
    WCHAR     szDesktop[MAX_PATH];
    int Len1, Len2;
    PACCESSINFO pAccessInfo;

    switch(message) {
    case WM_INITDIALOG:
       SetWindowLongPtr(hWnd, DWLP_USER, lParam);
       pAccessInfo = (PACCESSINFO)lParam;
	   
	   if(pAccessInfo->uID)
	   {
		   SendMessage(hWnd, DM_SETDEFID, pAccessInfo->uID, 0L);
		   SendDlgItemMessage(hWnd, pAccessInfo->uID, BM_SETSTYLE, (WPARAM)BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));
		   
		   if(pAccessInfo->uID == IDCANCEL)
		   {
			   SendDlgItemMessage(hWnd, ID_NOHOTKEY, BM_SETCHECK, BST_CHECKED, 0);
			   pAccessInfo->NoHotKeys = TRUE;
		   }
		   else if(pAccessInfo->uID == IDOK)
		   {
			   SendDlgItemMessage(hWnd, ID_NOHOTKEY, BM_SETCHECK, BST_UNCHECKED, 0);
			   pAccessInfo->NoHotKeys = FALSE;
		   }
		   
	    }

		return ConfirmHandler_InitDialog(hWnd, pAccessInfo->hDesk, pAccessInfo->TitleID, pAccessInfo->wcTitle);

    case WM_COMMAND:
       pAccessInfo = (PACCESSINFO)GetWindowLongPtr(hWnd, DWLP_USER);

       switch (LOWORD(wParam)) {
       case IDOK:
		    // If the selection is OK, Donot disable the shortcut key
			pAccessInfo->NoHotKeys = FALSE;

       case IDCANCEL:
   		   if(pAccessInfo->uID != LOWORD(wParam))
		   {
			   pAccessInfo->uID = LOWORD(wParam);

			   SaveUidSettings(pAccessInfo);
		   }

            if (pAccessInfo->NoHotKeys)
                  wParam +=HOTKEYCODE;
            EndDialog(hWnd, LOWORD(wParam));

            return TRUE;

       case ID_NOHOTKEY:
            pAccessInfo->NoHotKeys ^= 1;
            break;

       case IDHELP:
            if (pAccessInfo->NoHotKeys)
                 EndDialog(hWnd,IDCANCEL+HOTKEYCODE);
             else
                 EndDialog(hWnd, IDCANCEL);

//
// Spawn the correct help
//
            lstrcpy(buf,L"rundll32.exe Shell32.dll,Control_RunDLL access.cpl,,");
            switch (pAccessInfo->Feature) {
            case ACCESS_STICKYKEYS:
            case ACCESS_FILTERKEYS:
            case ACCESS_TOGGLEKEYS:
            default:
                 lstrcat(buf,L"1");
                 break;

            case ACCESS_MOUSEKEYS:
                 lstrcat(buf,L"4");
                 break;

            case ACCESS_HIGHCONTRAST:
                 lstrcat(buf,L"3");
                 break;
            }
            wsprintfW (szDesktop, L"%s\\", pAccessInfo->pTerm->pWinStaWinlogon->lpWinstaName);

            Len1 = wcslen(szDesktop);

            GetUserObjectInformation(pAccessInfo->hDesk, UOI_NAME, &szDesktop[Len1], MAX_PATH - Len1, &Len2);

            StartApplication(pAccessInfo->pTerm,
                             szDesktop,
                             pAccessInfo->pTerm->pWinStaWinlogon->UserProcessData.pEnvironment,
                             buf);


            return TRUE;
            break;

       default:
            return FALSE;
       }
       break;
	
    default:
       return FALSE;
    }
    return FALSE;
}

DWORD MakeAccessDlg(PACCESSINFO pAccessInfo) {
    DWORD iRet = 0;
    HDESK  hDeskOld;

    pAccessInfo->NoHotKeys = FALSE;
    hDeskOld = GetThreadDesktop(GetCurrentThreadId());
    if (hDeskOld == NULL) return 0;

    pAccessInfo->hDesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (pAccessInfo->hDesk == NULL) return 0;

    if (LoadString(g_hInstance, pAccessInfo->TitleID, pAccessInfo->wcTitle, cchTitle)) {
        SetThreadDesktop(pAccessInfo->hDesk);
        if (!FindWindowEx(GetDesktopWindow(), NULL, (LPCTSTR)0x8002, pAccessInfo->wcTitle)) {
            iRet = (DWORD)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(DLG_CONFIRM), NULL, ConfirmHandler, (LPARAM)pAccessInfo);
        }
        SetThreadDesktop(hDeskOld);
    }
    CloseDesktop(pAccessInfo->hDesk);

    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI StickyKeysNotificationThread(LPVOID lpv) {
    DWORD iRet;
    ACCESSINFO AccessInfo;
    STICKYKEYS sticky;
    DWORD dwS;
    BOOL b;
	PWINDOWSTATION pWS ;
    HANDLE ImpersonationHandle;
    // UINT uiShellHook;

    AccessInfo.pTerm   = (PTERMINAL)lpv;
    AccessInfo.Feature = ACCESS_STICKYKEYS;
    AccessInfo.TitleID = ID_STICKY_TITLE;

	pWS = AccessInfo.pTerm->pWinStaWinlogon;    
	ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);

    if ( IsNotifReq(&AccessInfo, pWS) )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
		

        sticky.cbSize = sizeof sticky;
        b = SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof sticky, &sticky, 0);
        ASSERT(b);
        dwS= sticky.dwFlags;

        if (iRet & HOTKEYCODE) {
            sticky.dwFlags &= ~SKF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof sticky, &sticky, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }
        if (iRet == IDOK) {
            sticky.dwFlags |= SKF_STICKYKEYSON;
        }

        if (dwS != sticky.dwFlags) {
            b = SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof sticky, &sticky, 0);
            ASSERT(b);

		   SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETSTICKYKEYS, FALSE, 
			   SMTO_ABORTIFHUNG, 5000, NULL);
        }

        iRet = 1;
    }

	StopImpersonating(ImpersonationHandle);    

    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI FilterKeysNotificationThread(LPVOID lpv) {
    DWORD iRet;
    ACCESSINFO AccessInfo;
    FILTERKEYS filter;
    DWORD dwF;
    BOOL b;
	PWINDOWSTATION pWS ;
    HANDLE ImpersonationHandle;

    AccessInfo.pTerm   = (PTERMINAL)lpv;
    AccessInfo.Feature = ACCESS_FILTERKEYS;
    AccessInfo.TitleID = ID_FILTER_TITLE;
	pWS = AccessInfo.pTerm->pWinStaWinlogon;    
	ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);

    if ( IsNotifReq(&AccessInfo, pWS) )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
        filter.cbSize = sizeof filter;
        b = SystemParametersInfo(SPI_GETFILTERKEYS, sizeof filter, &filter, 0);
        ASSERT(b);
        dwF = filter.dwFlags;

        if (iRet & HOTKEYCODE) {
            filter.dwFlags &= ~FKF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETFILTERKEYS, sizeof filter, &filter, SPIF_UPDATEINIFILE);
            ASSERT(b);
            iRet &= ~HOTKEYCODE;
        }
        if (iRet == IDOK) {
            filter.dwFlags |= FKF_FILTERKEYSON;
        }
        if (dwF !=filter.dwFlags) {
            b = SystemParametersInfo(SPI_SETFILTERKEYS, sizeof filter, &filter, 0);
            ASSERT(b);
			// Broadcast a message. Being extra safe not to turn on filter keys 
			// during logon. Send message to notify all specially systray: a-anilk 
		   SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETFILTERKEYS, FALSE, 
			   SMTO_ABORTIFHUNG, 5000, NULL);

        }
        iRet = 1;
    }
	
	StopImpersonating(ImpersonationHandle);    

    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI ToggleKeysNotificationThread(LPVOID lpv) {
    DWORD iRet;
    ACCESSINFO AccessInfo;
    TOGGLEKEYS toggle;
    DWORD dwT;
    BOOL b;
	PWINDOWSTATION pWS ;
    HANDLE ImpersonationHandle;

    toggle.cbSize = sizeof toggle;

    AccessInfo.pTerm   = (PTERMINAL)lpv;
    AccessInfo.Feature = ACCESS_TOGGLEKEYS;
    AccessInfo.TitleID = ID_TOGGLE_TITLE;
	pWS = AccessInfo.pTerm->pWinStaWinlogon;    
	ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);

    if ( IsNotifReq(&AccessInfo, pWS) )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
        toggle.cbSize = sizeof toggle;
        b = SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof toggle, &toggle, 0);
        ASSERT(b);
        dwT = toggle.dwFlags;

        if (iRet & HOTKEYCODE) {
            toggle.dwFlags &= ~TKF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof toggle, &toggle, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }
        if (iRet == IDOK) {
            toggle.dwFlags |= TKF_TOGGLEKEYSON;
        }

        if (toggle.dwFlags != dwT) {
            b = SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof toggle, &toggle, 0);
            ASSERT(b);
			// Not required to send message, As it currently has no indicators...
        }
        iRet = 1;
    }
	
	StopImpersonating(ImpersonationHandle);    

    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI MouseKeysNotificationThread(LPVOID lpv) {
    DWORD iRet;
    ACCESSINFO AccessInfo;
    MOUSEKEYS mouse;
    DWORD dwM;
    BOOL b;
	PWINDOWSTATION pWS ;
    HANDLE ImpersonationHandle;

    AccessInfo.pTerm   = (PTERMINAL)lpv;
    AccessInfo.Feature = ACCESS_MOUSEKEYS;
    AccessInfo.TitleID = ID_MOUSE_TITLE;
	pWS = AccessInfo.pTerm->pWinStaWinlogon;    
	ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);

    if ( IsNotifReq(&AccessInfo, pWS) )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
        mouse.cbSize = sizeof mouse;
        b = SystemParametersInfo(SPI_GETMOUSEKEYS, sizeof mouse, &mouse, 0);
        ASSERT(b);
        dwM = mouse.dwFlags;

        if (iRet & HOTKEYCODE) {
            mouse.dwFlags &= ~MKF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETMOUSEKEYS, sizeof mouse, &mouse, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }

        if (iRet == IDOK) {
            mouse.dwFlags |= MKF_MOUSEKEYSON;
        }

        if (mouse.dwFlags != dwM) {
            b = SystemParametersInfo(SPI_SETMOUSEKEYS, sizeof mouse, &mouse, 0);
            ASSERT(b);

			SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETMOUSEKEYS, FALSE, 
			   SMTO_ABORTIFHUNG, 5000, NULL);
        }

        iRet = 1;
    }
	
	StopImpersonating(ImpersonationHandle);    
    return iRet;
}

/***************************************************************************
 *                                                                         *
 * The thread opens the input desktopn, connects to it, and calls the      *
 * notification dialog for the accessibility feature.                      *
 *                                                                         *
 ***************************************************************************/
DWORD WINAPI HighContNotificationThread(LPVOID lpv) {
    BOOL fRc = 0;
    DWORD iRet;
    ACCESSINFO AccessInfo;
    HIGHCONTRAST  hc;
    DWORD dwH;
    BOOL b;
	PWINDOWSTATION pWS ;
    HANDLE ImpersonationHandle;

    AccessInfo.pTerm   = (PTERMINAL)lpv;
    AccessInfo.Feature = ACCESS_HIGHCONTRAST;
    AccessInfo.TitleID = ID_HC_TITLE;
	pWS = AccessInfo.pTerm->pWinStaWinlogon;    
	ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);

    if ( IsNotifReq(&AccessInfo, pWS) )
    {
        iRet = MakeAccessDlg(&AccessInfo);
    }
    else
        iRet = IDOK;

    if (iRet) {
        hc.cbSize = sizeof hc;
        b = SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof hc, &hc, 0);
        ASSERT(b);
        dwH = hc.dwFlags;

        if (iRet & HOTKEYCODE) {
            hc.dwFlags &= ~HCF_HOTKEYACTIVE;
            b = SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof hc, &hc, SPIF_UPDATEINIFILE);
            iRet &= ~HOTKEYCODE;
        }

        if (iRet == IDOK) {
            hc.dwFlags |= HCF_HIGHCONTRASTON;
        }

        if (hc.dwFlags != dwH) {
            b = SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof hc, &hc, 0);
            ASSERT(b);

			SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETHIGHCONTRAST, FALSE, 
			   SMTO_ABORTIFHUNG, 5000, NULL);
        }
        iRet = 1;
    }

	StopImpersonating(ImpersonationHandle);    

    return iRet;
}

// Helper method: Returns the value of Notification On/Off values
// Also adds the correct state flag in pAccessInfo
// Added by a-anilk on 10-22-98. 
BOOL IsNotifReq(ACCESSINFO* pAccessInfo, PWINDOWSTATION pWS)
{
    HKEY hKey;
   
    // Let default action be to show the Notification dialog
    BOOL retValue = TRUE;

    DWORD result;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType;
    
    if ( NT_SUCCESS( RtlOpenCurrentUser( KEY_READ, &pWS->UserProcessData.hCurrentUser ) ) )
    {
        if (ERROR_SUCCESS == RegOpenKeyEx( 
            pWS->UserProcessData.hCurrentUser, 
            NOTIF_KEY, 0, KEY_READ, &hKey) )
        {
            long lret = RegQueryValueEx(hKey, NOTIFY_VALUE, 0, &dwType, (LPBYTE)&result, &dwSize );
            
            if ( (lret == ERROR_SUCCESS) && (result == 0) )
                retValue = FALSE;
            
            RegCloseKey(hKey);
        }

		// Read the last user choice, IDOK or IDCANCEL and fill out the 
		// ACCESSINFO structure
		if(retValue)
		{

			//Read the default button from registry
			LPTSTR lpkeyName;

			switch(pAccessInfo->Feature)
			{
			case ACCESS_FILTERKEYS:
				lpkeyName = ID_SERIALKEYS;
				break;

			case ACCESS_TOGGLEKEYS:
				lpkeyName = ID_TOGGLEKEYS;
				break;

			case ACCESS_MOUSEKEYS:
				lpkeyName = ID_MOUSEKEYS;
				break;

			case ACCESS_HIGHCONTRAST:
				lpkeyName = ID_HIGHCONTROST;
				break;

			case ACCESS_STICKYKEYS:
			default:
				lpkeyName = ID_STICKYKEYNAME;
				break;			
			}

			if (ERROR_SUCCESS == RegOpenKeyEx(pWS->UserProcessData.hCurrentUser, lpkeyName, 0, KEY_READ, &hKey) )
			{
				long lret;
				dwSize = sizeof(DWORD);
				lret = RegQueryValueEx(hKey, __TEXT("DefaultButton"), 0, &dwType, (LPBYTE)&pAccessInfo->uID, &dwSize );
            
				if (lret != ERROR_SUCCESS)
					pAccessInfo->uID = 0;            
				RegCloseKey(hKey);
			}
		}
    }

    return retValue;
}

void SaveUidSettings(ACCESSINFO* pAccessInfo)
{
    HKEY hKey;   
    PWINDOWSTATION pWS ;
    HANDLE ImpersonationHandle;

    pWS = pAccessInfo->pTerm->pWinStaWinlogon;    
    ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);
    
    if ( NT_SUCCESS( RtlOpenCurrentUser( KEY_READ, &pWS->UserProcessData.hCurrentUser ) ) )
    {
		LPTSTR lpkeyName;

		switch(pAccessInfo->Feature)
		{
		case ACCESS_FILTERKEYS:
			lpkeyName = ID_SERIALKEYS;
			break;

		case ACCESS_TOGGLEKEYS:
			lpkeyName = ID_TOGGLEKEYS;
			break;

		case ACCESS_MOUSEKEYS:
			lpkeyName = ID_MOUSEKEYS;
			break;

		case ACCESS_HIGHCONTRAST:
			lpkeyName = ID_HIGHCONTROST;
			break;

		case ACCESS_STICKYKEYS:
		default:
			lpkeyName = ID_STICKYKEYNAME;
			break;			
		}

        if (ERROR_SUCCESS == RegOpenKeyEx(pWS->UserProcessData.hCurrentUser, lpkeyName, 0, KEY_WRITE, &hKey) )
        {
            RegSetValueEx(hKey, __TEXT("DefaultButton"), 0, REG_DWORD, (LPBYTE)&pAccessInfo->uID, sizeof(DWORD));            
            RegCloseKey(hKey);
        }

    }

    StopImpersonating(ImpersonationHandle);    
    
}


// Sends a message to UtilMan service to open the Uman dialog
// If the service is not started, Then it starts it and then sends 
// the message. Added by a-anilk on 11-22-98. 
DWORD WINAPI UtilManStartThread(LPVOID lpv) 
{
    SC_HANDLE hSCMan, hService;
    SERVICE_STATUS serStat, ssStatus;
    int waitCount = 0;

    hSCMan = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if ( hSCMan )
    {
        hService = OpenService( hSCMan, UTILMANSERVICE_NAME, SERVICE_ALL_ACCESS);

        if ( hService )
        {
            QueryServiceStatus(hService, &ssStatus);

            if (ssStatus.dwCurrentState != SERVICE_RUNNING)
	        { 
  	            // If UM is not running Then start it
                LPCTSTR args[3];
		        args[0] = UTILMAN_START_BYHOTKEY;
		        args[1] = 0;
	    
                StartService(hService,1,args);


                // We need a WAIT here
                while(QueryServiceStatus(hService, &ssStatus) )
                {
                    if ( ssStatus.dwCurrentState == SERVICE_RUNNING)
                        break;

                    Sleep(500);
                    waitCount++;
                    
                    // We cannot afford to wait for more than 10 sec
                    if (waitCount > 20)
                        break;
                }
            }
      
            if ( ssStatus.dwCurrentState == SERVICE_RUNNING )
                ControlService(hService, UM_SERVICE_CONTROL_SHOWDIALOG, &serStat);
        
            CloseServiceHandle(hService);
        } 
    
        CloseServiceHandle(hSCMan);
    }
                            
    return 1;
}



