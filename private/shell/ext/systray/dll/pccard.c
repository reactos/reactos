/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:	PCCARD.C
*
*  VERSION:     1.0
*
*  AUTHOR:	RAL
*
*  DATE:	11/01/94
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  Nov. 11, 94 RAL Original
*  Oct  23, 95 Shawnb UNICODE enabled
*
*******************************************************************************/

#include "stdafx.h"
#include "systray.h"

#define PCMCIAMENU_PROPERTIES	100
#define PCMCIAMENU_DISABLE		101

#define PCMCIAMENU_SOCKET		200

extern HANDLE g_hPCCARD;
extern HINSTANCE g_hInstance;

static BOOL		g_bPCMCIAEnabled = FALSE;
static BOOL		g_bPCMCIAIconShown = FALSE;
static HICON	g_hPCMCIAIcon = NULL;


#define MAX_DEVNODES 20

static DWORD	g_aDevnodes[MAX_DEVNODES];
static BYTE		g_aSktState[MAX_DEVNODES] = {0};
static UINT		g_numskts = 0;
static DWORD	g_PCMCIAFlags = 0;
static       TCHAR g_szDevNodeKeyFmt[] = REGSTR_PATH_DYNA_ENUM TEXT ("\\%X");
static const TCHAR g_szEnumKeyPrefix[] = REGSTR_PATH_ENUM TEXT ("\\");
static const TCHAR g_szPCMCIAFlags[]   = REGSTR_VAL_SYSTRAYPCCARDFLAGS;
static const TCHAR g_szClass[]         = REGSTR_VAL_CLASS;
static const TCHAR g_szModemClass[]    = REGSTR_KEY_MODEM_CLASS;
#if NOTYET
static const TCHAR g_szDiskDriveClass[] = REGSTR_KEY_DISKDRIVE_CLASS;
#endif

#define SKTSTATE_GOODEJECT	1
#define SKTSTATE_SHOULDWARN	2
#define SKTSTATE_TYPEKNOWN	4


HKEY OpenDevnodeDynKey(DWORD dwDevnode)
{
	TCHAR	szScratch[MAX_PATH];
	HKEY	hkDyn = NULL;

	wsprintf(szScratch, g_szDevNodeKeyFmt, dwDevnode);
	if (RegOpenKey(HKEY_DYN_DATA, szScratch, &hkDyn) != ERROR_SUCCESS) {
		return(NULL);
	}
	return hkDyn;
}


UINT GetDynInfo(DWORD dwDevNode, LPCTSTR lpszValName,
                LPVOID lpBuffer, UINT cbBuffer)
{
	UINT cbSize = 0;
	HKEY hkDyn = OpenDevnodeDynKey(dwDevNode);

	if (hkDyn) {
		if (RegQueryValueEx(hkDyn, lpszValName, NULL, NULL,
						    lpBuffer, &cbBuffer) == ERROR_SUCCESS) {
			cbSize = cbBuffer;
		}
  
		RegCloseKey(hkDyn);
	}
	return(cbSize);
}


HKEY OpenDevnodeHwKey(DWORD dwDevnode)
{
	TCHAR	szScratch[MAX_PATH];
	HKEY    hkDyn, hkHw = NULL;
	UINT    cbSize;
	UINT    cchOffset;

	if ((hkDyn = OpenDevnodeDynKey(dwDevnode)) == NULL) {
		return(NULL);
	}
	lstrcpy(szScratch, g_szEnumKeyPrefix);

	cbSize = sizeof(szScratch) - sizeof(g_szEnumKeyPrefix);
	cchOffset = ARRAYSIZE(g_szEnumKeyPrefix) - 1;

	if (RegQueryValueEx(hkDyn, REGSTR_VAL_HARDWARE_KEY,
						NULL, NULL, (LPSTR)&(szScratch[cchOffset]),
						&cbSize) == ERROR_SUCCESS) {
		if (RegOpenKey(HKEY_LOCAL_MACHINE, szScratch, &hkHw) != ERROR_SUCCESS) {
			hkHw = NULL;
		}
	}
	RegCloseKey(hkDyn);
	return(hkHw);
}


UINT GetHwInfo(DWORD dwDevNode, LPCTSTR lpszValName,
               LPVOID lpBuffer, UINT cbBuffer)
{
	UINT cbSize = 0;
	HKEY hkHw = OpenDevnodeHwKey(dwDevNode);

	if (hkHw) {
		if (RegQueryValueEx(hkHw, lpszValName, NULL, NULL,
						    lpBuffer, &cbBuffer) == ERROR_SUCCESS) {
			cbSize = cbBuffer;
		}
		RegCloseKey(hkHw);
	}
	return(cbSize);
}


void UpdateSktTypes()
{
	UINT i;
	TCHAR szClassName[32];

	for (i = 0; i < g_numskts; i++) {
		if (g_aSktState[i] == 0 && g_aDevnodes[i] != 0) {
			if (GetHwInfo(g_aDevnodes[i], g_szClass,
						  szClassName, sizeof(szClassName))) {
				g_aSktState[i] |= SKTSTATE_TYPEKNOWN;
				if (lstrcmpi(g_szModemClass, szClassName) != 0) {
					g_aSktState[i] |= SKTSTATE_SHOULDWARN;
				}
			}
		}
	}
}


void UpdateSocketInfo()
{
	UINT cbReturned;
	if (DeviceIoControl(g_hPCCARD, PCCARD_IOCTL_GET_DEVNODES,
					    NULL, 0,
						g_aDevnodes, sizeof(g_aDevnodes), &cbReturned, NULL)) {
		g_numskts = cbReturned / 4;
	} else {
		g_numskts = 0;
	}
	UpdateSktTypes();
}



void UpdateGlobalFlags()
{
	HKEY hk;
	if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_SYSTRAY, &hk) ==
		 ERROR_SUCCESS) {
		UINT cb = sizeof(g_PCMCIAFlags);
		if (RegQueryValueEx(hk, g_szPCMCIAFlags, NULL, NULL,
				            (LPSTR)(&g_PCMCIAFlags), &cb) != ERROR_SUCCESS) {
			g_PCMCIAFlags = 0;
		}

		RegCloseKey(hk);
	}
}


BOOL PCMCIA_Init(HWND hWnd)
{
	if (g_hPCCARD == INVALID_HANDLE_VALUE) {
		g_hPCCARD = CreateFile(TEXT ("\\\\.\\PCCARD"), GENERIC_READ | GENERIC_WRITE,
							   FILE_SHARE_READ | FILE_SHARE_WRITE,
							   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		UpdateGlobalFlags();
	}

	return(g_hPCCARD != INVALID_HANDLE_VALUE);
}




//
//  NOTE:  This function expects the caller to have called UpdateSocketInfo
//	   prior to calling it.
//
void PCMCIA_UpdateStatus(HWND hWnd, BOOL bShowIcon, DWORD dnRemove)
{
	if (bShowIcon) {
		UINT  i;
		bShowIcon = FALSE;	// Assume no devnodes
		for (i = 0; i < g_numskts; i++) {
			if (g_aDevnodes[i] != 0 && g_aDevnodes[i] != dnRemove) {
				bShowIcon = TRUE;
				break;
			}
		}
	}

	if (bShowIcon != g_bPCMCIAIconShown) {
		g_bPCMCIAIconShown = bShowIcon;
		if (bShowIcon) {
			LPTSTR pStr = LoadDynamicString(IDS_PCMCIATIP);
			g_hPCMCIAIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_PCMCIA),
									 IMAGE_ICON, 16, 16, 0);
			SysTray_NotifyIcon(hWnd, STWM_NOTIFYPCMCIA, NIM_ADD,
							   g_hPCMCIAIcon, pStr);
			DeleteDynamicString(pStr);
		} else {
			SysTray_NotifyIcon(hWnd, STWM_NOTIFYPCMCIA, NIM_DELETE, NULL, NULL);
			if (g_hPCMCIAIcon) {
				DestroyIcon(g_hPCMCIAIcon);
			}
		}
	}
}


#define DEVNODE_NOT_IN_LIST -1

int FindSocketIndex(DWORD dn)
{
	int i;
	for (i = 0; i < (int)g_numskts; i++) {
		if (g_aDevnodes[i] == dn) {
			return(i);
		}
	}
	return(DEVNODE_NOT_IN_LIST);
}



void PCMCIA_DeviceChange(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	int i;

	#define lpdbd ((PDEV_BROADCAST_DEVNODE)lParam)
	#define DN_STARTED  0x00000008          // WARNING KEEP THIS IN SYNC WITH CONFIGMG.H

	if ((wParam != DBT_DEVICEREMOVEPENDING &&
		wParam != DBT_DEVICEARRIVAL &&
		wParam != DBT_DEVICEREMOVECOMPLETE) ||
		lpdbd->dbcd_devicetype != DBT_DEVTYP_DEVNODE) {
		return;
	}

	switch (wParam) {
	case DBT_DEVICEREMOVEPENDING:    // Query remove succeeded
		i = FindSocketIndex(lpdbd->dbcd_devnode);
		if (i != DEVNODE_NOT_IN_LIST) {
			g_aSktState[i] |= SKTSTATE_GOODEJECT;
		}
		break;

	case DBT_DEVICEARRIVAL:
		UpdateSocketInfo();
		i = FindSocketIndex(lpdbd->dbcd_devnode);
		if (i != DEVNODE_NOT_IN_LIST) {
			g_aSktState[i] = 0;
			UpdateSktTypes();
			PCMCIA_UpdateStatus(hDlg, TRUE, 0);
		}
		break;

	case DBT_DEVICEREMOVECOMPLETE:
	{
		ULONG	Status = 0L;
		ULONG	Size = sizeof(ULONG);
		TCHAR	szDevNode[REGSTR_MAX_VALUE_LENGTH];
		HKEY	hkDevDyna;
       
		wsprintf(szDevNode, TEXT ("%s\\%8X"),REGSTR_PATH_DYNA_ENUM,lpdbd->dbcd_devnode);
		if (RegOpenKey( HKEY_DYN_DATA,
						 szDevNode,
						 &hkDevDyna ) == ERROR_SUCCESS) 
		{
			RegQueryValueEx( hkDevDyna, REGSTR_VAL_STATUS, 0, NULL, (LPSTR)&Status, &Size );
			RegCloseKey(hkDevDyna);
		}
         
		if (Status & DN_STARTED) 
		{
			i = FindSocketIndex(lpdbd->dbcd_devnode);
			if (i != DEVNODE_NOT_IN_LIST) {
				//
				//  Check to see if we're supposed to warn the user about this
				//  eject.  Only warn if NOT good eject and class is one we warn
				//  about.
				//
				BOOL fWarnUser = (g_aSktState[i] &
								 (SKTSTATE_SHOULDWARN | SKTSTATE_GOODEJECT)) ==
								 SKTSTATE_SHOULDWARN;
				g_aSktState[i] = 0;
				UpdateSocketInfo();
				PCMCIA_UpdateStatus(hDlg, TRUE, lpdbd->dbcd_devnode);
				if (fWarnUser) {
					// Make sure the user did not turn this off earlier
					UpdateGlobalFlags();
					if (!(g_PCMCIAFlags & PCMCIA_REGFLAG_NOWARN)) {
						const TCHAR szOpen[]	= TEXT ("open");
						const TCHAR szRunDLL[]	= TEXT ("RUNDLL32.EXE");
						const TCHAR szParams[]	= TEXT ("RUNDLL mspcic.dll,EjectWarningDlg");
                 
						ShellExecute(NULL, szOpen, szRunDLL, 
								     szParams, NULL, SW_SHOW);
					}
				}
			}	                
		}
		break;
	}        
	}
#undef lpdbd
}



//
//  Called at init time and whenever services are enabled/disabled.
//  Returns false if PCMCIA services are not active.
//
BOOL PCMCIA_CheckEnable(HWND hWnd, BOOL bSvcEnabled)
{
	BOOL bEnable = bSvcEnabled && PCMCIA_Init(hWnd);

	if (bEnable != g_bPCMCIAEnabled) {
		g_bPCMCIAEnabled = bEnable;
		UpdateSocketInfo();
		PCMCIA_UpdateStatus(hWnd, bEnable, 0);
		if (!bEnable) {
			CloseIfOpen(&g_hPCCARD);
		}
	}

   return(bEnable);
}


/*----------------------------------------------------------------------------
 * PCMCIA_CreateMenu()
 *
 * build a menu containing all sockets.
 *
 *----------------------------------------------------------------------------*/

//static CRITICAL_SECTION csMenu;
static HMENU _hMenu[2] = {0};

HMENU PCMCIA_CreateMenu(LONG l)
{
	//EnterCriticalSection(&csMenu);

	if (l > 0) {
		if (_hMenu[1] == NULL) {
			HMENU  hmenu = _hMenu[l] = CreatePopupMenu();
			LPTSTR lpszMenu;

			if ((lpszMenu = LoadDynamicString(IDS_PCCARDMENU1)) != NULL) {
				AppendMenu(hmenu,MF_STRING,PCMCIAMENU_PROPERTIES,lpszMenu);
				DeleteDynamicString(lpszMenu);
			}
     
			if ((lpszMenu = LoadDynamicString(IDS_PCCARDMENU2)) != NULL) {
				AppendMenu(hmenu,MF_STRING,PCMCIAMENU_DISABLE,lpszMenu);
				DeleteDynamicString(lpszMenu);
			}
		  
			SetMenuDefaultItem(hmenu,PCMCIAMENU_PROPERTIES,FALSE);
		}
	} else {
		HMENU hMenu;

		if (_hMenu[0]) {
			DestroyMenu(_hMenu[0]);
		}

		hMenu = _hMenu[0] = CreatePopupMenu();

		if (g_hPCCARD != INVALID_HANDLE_VALUE) {
			TCHAR    szDesc[80];
			LPTSTR   lpszMenuText;
			UINT     i;

			UpdateSocketInfo();
			for (i = 0; i < g_numskts; i++) {
				if (g_aDevnodes[i] &&
				    GetHwInfo(g_aDevnodes[i], REGSTR_VAL_DEVDESC, szDesc, sizeof(szDesc))) {
#if NOTYET
					DWORD   dwChild;
					TCHAR   szClassName[32];
					TCHAR   szDriveLetters[32];

					if (GetDynInfo(g_aDevnodes[i], REGSTR_VAL_CHILD,
						           &dwChild, sizeof(dwChild)) &&
					    GetHwInfo(dwChild, g_szClass,
							      szClassName, sizeof(szClassName)) &&
						lstrcmpi(g_szDiskDriveClass, szClassName) == 0 &&
						GetHwInfo(dwChild, REGSTR_VAL_CURDRVLET,
							      szDriveLetters, sizeof(szDriveLetters)) &&
						lstrlen(szDriveLetters) > 0) {
						lpszMenuText = LoadDynamicString(IDS_EJECTFMTDISKDRIVE,
												         szDesc, szDriveLetters[0]);
					} 
					else
#endif
					{
						lpszMenuText = LoadDynamicString(IDS_EJECTFMT, szDesc);
					}

					if (lpszMenuText) {
						AppendMenu(hMenu,MF_STRING,PCMCIAMENU_SOCKET+i,lpszMenuText);
						DeleteDynamicString(lpszMenuText);
					}
				}
			}
		}
	}

	//LeaveCriticalSection(&csMenu);

	return _hMenu[l];
}


void PCMCIA_Menu(HWND hwnd, UINT uMenuNum, UINT uButton)
{
	POINT pt;
	UINT iCmd;

	SetForegroundWindow(hwnd);
	GetCursorPos(&pt);
	iCmd = TrackPopupMenu(PCMCIA_CreateMenu(uMenuNum), uButton | TPM_RETURNCMD | TPM_NONOTIFY,
						  pt.x, pt.y, 0, hwnd, NULL);
	if (iCmd >= PCMCIAMENU_SOCKET) {
		const TCHAR szOpen[]	= TEXT ("open");
		const TCHAR szRunDLL[]	= TEXT ("RUNDLL32.EXE");
		LPTSTR lpszCommand		= LoadDynamicString(IDS_RUNEJECT, iCmd-PCMCIAMENU_SOCKET);
		if (lpszCommand == NULL)
			return;
   
		ShellExecute(NULL, szOpen, szRunDLL,
					 lpszCommand, NULL, SW_SHOW);
   
		DeleteDynamicString(lpszCommand);
	} else {
		switch (iCmd) {
		case PCMCIAMENU_PROPERTIES:
			SysTray_RunProperties(IDS_RUNPCMCIAPROPERTIES);
			break;

		case PCMCIAMENU_DISABLE:
			PostMessage(hwnd, STWM_ENABLESERVICE, STSERVICE_PCMCIA, FALSE);
			break;
		}
	}
}


void PCMCIA_Notify(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	switch (lParam) 
	{
		case WM_RBUTTONUP:
			PCMCIA_Menu(hwnd, 1, TPM_RIGHTBUTTON);
			break;

		case WM_LBUTTONDOWN:
			SetTimer(hwnd, PCMCIA_TIMER_ID, GetDoubleClickTime()+100, NULL);
			break;

		case WM_LBUTTONDBLCLK:
			KillTimer(hwnd, PCMCIA_TIMER_ID);
			SysTray_RunProperties(IDS_RUNPCMCIAPROPERTIES);
			break;
	}
}


void PCMCIA_Timer(HWND hwnd)
{
	KillTimer(hwnd, PCMCIA_TIMER_ID);
	PCMCIA_Menu(hwnd, 0, TPM_LEFTBUTTON);
}
