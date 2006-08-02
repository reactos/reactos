/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            services/winlogon/wlx.c
 * PURPOSE:         Logon
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Ge van Geldorp (gvg@reactos.com)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

#define YDEBUG
#include <wine/debug.h>

//#define UNIMPLEMENTED DbgPrint("WL: %S() at %S:%i unimplemented!\n", __FUNCTION__, __FILE__, __LINE__)

#define WINLOGON_DESKTOP   L"Winlogon"

/*
 * @implemented
 */
VOID WINAPI
WlxUseCtrlAltDel(
	HANDLE hWlx)
{
	WlxSetOption(hWlx, WLX_OPTION_USE_CTRL_ALT_DEL, TRUE, NULL);
}

/*
 * @implemented
 */
VOID WINAPI
WlxSetContextPointer(
	HANDLE hWlx,
	PVOID pWlxContext)
{
	WlxSetOption(hWlx, WLX_OPTION_CONTEXT_POINTER, (ULONG_PTR)pWlxContext, NULL);
}

/*
 * @implemented
 */
VOID WINAPI
WlxSasNotify(
	HANDLE hWlx,
	DWORD dwSasType)
{
	DispatchSAS((PWLSESSION)hWlx, dwSasType);
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxSetTimeout(
	HANDLE hWlx,
	DWORD Timeout)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
int WINAPI
WlxAssignShellProtection(
	HANDLE hWlx,
	HANDLE hToken,
	HANDLE hProcess,
	HANDLE hThread)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @implemented
 */
int WINAPI
WlxMessageBox(
	HANDLE hWlx,
	HWND hwndOwner,
	LPWSTR lpszText,
	LPWSTR lpszTitle,
	UINT fuStyle)
{
	return MessageBoxW(hwndOwner, lpszText, lpszTitle, fuStyle);
}

/*
 * @implemented
 */
int WINAPI
WlxDialogBox(
	HANDLE hWlx,
	HANDLE hInst,
	LPWSTR lpszTemplate,
	HWND hwndOwner,
	DLGPROC dlgprc)
{
	return (int)DialogBox(hInst, lpszTemplate, hwndOwner, dlgprc);
}

/*
 * @implemented
 */
int WINAPI
WlxDialogBoxParam(
	HANDLE hWlx,
	HANDLE hInst,
	LPWSTR lpszTemplate,
	HWND hwndOwner,
	DLGPROC dlgprc,
	LPARAM dwInitParam)
{
	return (int)DialogBoxParam(hInst, lpszTemplate, hwndOwner, dlgprc, dwInitParam);
}

/*
 * @implemented
 */
int WINAPI
WlxDialogBoxIndirect(
	HANDLE hWlx,
	HANDLE hInst,
	LPCDLGTEMPLATE hDialogTemplate,
	HWND hwndOwner,
	DLGPROC dlgprc)
{
	return (int)DialogBoxIndirect(hInst, hDialogTemplate, hwndOwner, dlgprc);
}

/*
 * @implemented
 */
int WINAPI
WlxDialogBoxIndirectParam(
	HANDLE hWlx,
	HANDLE hInst,
	LPCDLGTEMPLATE hDialogTemplate,
	HWND hwndOwner,
	DLGPROC dlgprc,
	LPARAM dwInitParam)
{
	return (int)DialogBoxIndirectParam(hInst, hDialogTemplate, hwndOwner, dlgprc, dwInitParam);
}

/*
 * @implemented
 */
int WINAPI
WlxSwitchDesktopToUser(
	HANDLE hWlx)
{
	PWLSESSION Session = (PWLSESSION)hWlx;
	return (int)SwitchDesktop(Session->ApplicationDesktop);
}

/*
 * @implemented
 */
int WINAPI
WlxSwitchDesktopToWinlogon(
	HANDLE hWlx)
{
	PWLSESSION Session = (PWLSESSION)hWlx;
	return (int)SwitchDesktop(Session->WinlogonDesktop);
}

/*
 * @unimplemented
 */
int WINAPI
WlxChangePasswordNotify(
	HANDLE hWlx,
	PWLX_MPR_NOTIFY_INFO pMprInfo,
	DWORD dwChangeInfo)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxGetSourceDesktop(
	HANDLE hWlx,
	PWLX_DESKTOP* ppDesktop)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxSetReturnDesktop(
	HANDLE hWlx,
	PWLX_DESKTOP pDesktop)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxCreateUserDesktop(
	HANDLE hWlx,
	HANDLE hToken,
	DWORD Flags,
	PWSTR pszDesktopName,
	PWLX_DESKTOP* ppDesktop)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
int WINAPI
WlxChangePasswordNotifyEx(
	HANDLE hWlx,
	PWLX_MPR_NOTIFY_INFO pMprInfo,
	DWORD dwChangeInfo,
	PWSTR ProviderName,
	PVOID Reserved)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxCloseUserDesktop(
	HANDLE hWlx,
	PWLX_DESKTOP pDesktop,
	HANDLE hToken)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxSetOption(
	HANDLE hWlx,
	DWORD Option,
	ULONG_PTR Value,
	ULONG_PTR* OldValue)
{
	PWLSESSION Session = (PWLSESSION)hWlx;
	UNIMPLEMENTED;

	if (Session || !Value)
	{
		switch (Option)
		{
			case WLX_OPTION_USE_CTRL_ALT_DEL:
				return TRUE;
			case WLX_OPTION_CONTEXT_POINTER:
			{
				*OldValue = (ULONG_PTR)Session->MsGina.Context;
				Session->MsGina.Context = (PVOID)Value;
				return TRUE;
			}
			case WLX_OPTION_USE_SMART_CARD:
				return FALSE;
		}
	}

	return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxGetOption(
	HANDLE hWlx,
	DWORD Option,
	ULONG_PTR* Value)
{
	PMSGINAINSTANCE Instance = (PMSGINAINSTANCE)hWlx;
	UNIMPLEMENTED;

	if (Instance || !Value)
	{
		switch (Option)
		{
			case WLX_OPTION_USE_CTRL_ALT_DEL:
				return TRUE;
			case WLX_OPTION_CONTEXT_POINTER:
			{
				*Value = (ULONG_PTR)Instance->Context;
				return TRUE;
			}
			case WLX_OPTION_USE_SMART_CARD:
			case WLX_OPTION_SMART_CARD_PRESENT:
			case WLX_OPTION_SMART_CARD_INFO:
				*Value = 0;
				return FALSE;
			case WLX_OPTION_DISPATCH_TABLE_SIZE:
			{
			switch (Instance->Version)
			{
				case WLX_VERSION_1_0:
					*Value = sizeof(WLX_DISPATCH_VERSION_1_0);
					break;
				case WLX_VERSION_1_1:
					*Value = sizeof(WLX_DISPATCH_VERSION_1_1);
					break;
				case WLX_VERSION_1_2:
					*Value = sizeof(WLX_DISPATCH_VERSION_1_2);
					break;
				case WLX_VERSION_1_3:
					*Value = sizeof(WLX_DISPATCH_VERSION_1_3);
					break;
				case WLX_VERSION_1_4:
					*Value = sizeof(WLX_DISPATCH_VERSION_1_4);
					break;
				default:
					return FALSE;
				}
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*
 * @unimplemented
 */
VOID WINAPI
WlxWin31Migrate(
	HANDLE hWlx)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxQueryClientCredentials(
	PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxQueryInetConnectorCredentials(
	PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxDisconnect(VOID)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
DWORD WINAPI
WlxQueryTerminalServicesData(
	HANDLE hWlx,
	PWLX_TERMINAL_SERVICES_DATA pTSData,
	WCHAR* UserName,
	WCHAR* Domain)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
DWORD WINAPI
WlxQueryConsoleSwitchCredentials(
	PWLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0 pCred)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxQueryTsLogonCredentials(
	PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCred)
{
	UNIMPLEMENTED;
	return FALSE;
}

static const
WLX_DISPATCH_VERSION_1_4 FunctionTable = {
	WlxUseCtrlAltDel,
	WlxSetContextPointer,
	WlxSasNotify,
	WlxSetTimeout,
	WlxAssignShellProtection,
	WlxMessageBox,
	WlxDialogBox,
	WlxDialogBoxParam,
	WlxDialogBoxIndirect,
	WlxDialogBoxIndirectParam,
	WlxSwitchDesktopToUser,
	WlxSwitchDesktopToWinlogon,
	WlxChangePasswordNotify,
	WlxGetSourceDesktop,
	WlxSetReturnDesktop,
	WlxCreateUserDesktop,
	WlxChangePasswordNotifyEx,
	WlxCloseUserDesktop,
	WlxSetOption,
	WlxGetOption,
	WlxWin31Migrate,
	WlxQueryClientCredentials,
	WlxQueryInetConnectorCredentials,
	WlxDisconnect,
	WlxQueryTerminalServicesData,
	WlxQueryConsoleSwitchCredentials,
	WlxQueryTsLogonCredentials
};

/******************************************************************************/

static BOOL
GetGinaPath(
	OUT LPWSTR Path,
	IN DWORD Len)
{
	LONG Status;
	DWORD Type, Size;
	HKEY hKey;

	Status = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
		0,
		KEY_QUERY_VALUE,
		&hKey);
	if (Status != ERROR_SUCCESS)
	{
		/* Default value */
		wcsncpy(Path, L"msgina.dll", Len);
		return TRUE;
	}

	Size = Len * sizeof(WCHAR);
	Status = RegQueryValueEx(
		hKey,
		L"GinaDLL",
		NULL,
		&Type,
		(LPBYTE)Path,
		&Size);
	if (Status != ERROR_SUCCESS || Type != REG_SZ || Size == 0)
		wcsncpy(Path, L"msgina.dll", Len);
	RegCloseKey(hKey);
	return TRUE;
}

static INT_PTR CALLBACK
GinaLoadFailedWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hwndDlg, IDOK);
					break;
			}
			break;
		}
		case WM_INITDIALOG:
		{
			int len;
			WCHAR templateText[MAX_PATH], text[MAX_PATH];

			len = GetDlgItemText(hwndDlg, IDC_GINALOADFAILED, templateText, MAX_PATH);
			if (len)
			{
				wsprintf(text, templateText, (LPWSTR)lParam);
				SetDlgItemText(hwndDlg, IDC_GINALOADFAILED, text);
			}
			SetFocus(GetDlgItem(hwndDlg, IDOK));
			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, IDCANCEL);
			return TRUE;
		}
	}

	return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

#define FAIL_AND_RETURN() return DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_GINALOADFAILED), 0, GinaLoadFailedWindowProc, (LPARAM)GinaDll) && FALSE
static BOOL
LoadGina(
	IN OUT PMSGINAFUNCTIONS Functions,
	OUT DWORD *DllVersion,
	OUT HMODULE *GinaInstance)
{
	HMODULE hGina;
	WCHAR GinaDll[MAX_PATH + 1];

	GinaDll[0] = '\0';
	if (!GetGinaPath(GinaDll, MAX_PATH))
		FAIL_AND_RETURN();
	/* Terminate string */
	GinaDll[MAX_PATH] = '\0';

	if (!(hGina = LoadLibrary(GinaDll)))
		FAIL_AND_RETURN();
	*GinaInstance = hGina;

	Functions->WlxNegotiate = (PFWLXNEGOTIATE)GetProcAddress(hGina, "WlxNegotiate");
	Functions->WlxInitialize = (PFWLXINITIALIZE)GetProcAddress(hGina, "WlxInitialize");

	if (!Functions->WlxInitialize)
		FAIL_AND_RETURN();

	if (!Functions->WlxNegotiate)
	{
		/* Assume current version */
		*DllVersion = WLX_CURRENT_VERSION;
	}
	else if (!Functions->WlxNegotiate(WLX_CURRENT_VERSION, DllVersion))
		FAIL_AND_RETURN();

	if (*DllVersion >= WLX_VERSION_1_0)
	{
		Functions->WlxActivateUserShell = (PFWLXACTIVATEUSERSHELL)GetProcAddress(hGina, "WlxActivateUserShell");
		if (!Functions->WlxActivateUserShell) FAIL_AND_RETURN();
		Functions->WlxDisplayLockedNotice = (PFWLXDISPLAYLOCKEDNOTICE)GetProcAddress(hGina, "WlxDisplayLockedNotice");
		if (!Functions->WlxDisplayLockedNotice) FAIL_AND_RETURN();
		Functions->WlxDisplaySASNotice = (PFWLXDISPLAYSASNOTICE)GetProcAddress(hGina, "WlxDisplaySASNotice");
		if (!Functions->WlxDisplaySASNotice) FAIL_AND_RETURN();
		Functions->WlxIsLockOk = (PFWLXISLOCKOK)GetProcAddress(hGina, "WlxIsLockOk");
		if (!Functions->WlxIsLockOk) FAIL_AND_RETURN();
		Functions->WlxIsLogoffOk = (PFWLXISLOGOFFOK)GetProcAddress(hGina, "WlxIsLogoffOk");
		if (!Functions->WlxIsLogoffOk) FAIL_AND_RETURN();
		Functions->WlxLoggedOnSAS = (PFWLXLOGGEDONSAS)GetProcAddress(hGina, "WlxLoggedOnSAS");
		if (!Functions->WlxLoggedOnSAS) FAIL_AND_RETURN();
		Functions->WlxLoggedOutSAS = (PFWLXLOGGEDOUTSAS)GetProcAddress(hGina, "WlxLoggedOutSAS");
		if (!Functions->WlxLoggedOutSAS) FAIL_AND_RETURN();
		Functions->WlxLogoff = (PFWLXLOGOFF)GetProcAddress(hGina, "WlxLogoff");
		if (!Functions->WlxLogoff) FAIL_AND_RETURN();
		Functions->WlxShutdown = (PFWLXSHUTDOWN)GetProcAddress(hGina, "WlxShutdown");
		if (!Functions->WlxShutdown) FAIL_AND_RETURN();
		Functions->WlxWkstaLockedSAS = (PFWLXWKSTALOCKEDSAS)GetProcAddress(hGina, "WlxWkstaLockedSAS");
		if (!Functions->WlxWkstaLockedSAS) FAIL_AND_RETURN();
	}

	if (*DllVersion >= WLX_VERSION_1_1)
	{
		Functions->WlxScreenSaverNotify = (PFWLXSCREENSAVERNOTIFY)GetProcAddress(hGina, "WlxScreenSaverNotify");
		if (!Functions->WlxScreenSaverNotify) FAIL_AND_RETURN();
		Functions->WlxStartApplication = (PFWLXSTARTAPPLICATION)GetProcAddress(hGina, "WlxStartApplication");
		if (!Functions->WlxStartApplication) FAIL_AND_RETURN();
	}

	if (*DllVersion >= WLX_VERSION_1_3)
	{
		Functions->WlxDisplayStatusMessage = (PFWLXDISPLAYSTATUSMESSAGE)GetProcAddress(hGina, "WlxDisplayStatusMessage");
		if (!Functions->WlxDisplayStatusMessage) FAIL_AND_RETURN();
		Functions->WlxGetStatusMessage = (PFWLXGETSTATUSMESSAGE)GetProcAddress(hGina, "WlxGetStatusMessage");
		if (!Functions->WlxGetStatusMessage) FAIL_AND_RETURN();
		Functions->WlxNetworkProviderLoad = (PFWLXNETWORKPROVIDERLOAD)GetProcAddress(hGina, "WlxNetworkProviderLoad");
		if (!Functions->WlxNetworkProviderLoad) FAIL_AND_RETURN();
		Functions->WlxRemoveStatusMessage = (PFWLXREMOVESTATUSMESSAGE)GetProcAddress(hGina, "WlxRemoveStatusMessage");
		if (!Functions->WlxRemoveStatusMessage) FAIL_AND_RETURN();
	}

	return TRUE;
}
#undef FAIL_AND_RETURN

BOOL
GinaInit(
	IN OUT PWLSESSION Session)
{
	DWORD GinaDllVersion;

	if (!LoadGina(&Session->MsGina.Functions, &GinaDllVersion, &Session->MsGina.hDllInstance))
		return FALSE;

	Session->MsGina.Context = NULL;
	Session->MsGina.Version = GinaDllVersion;
	Session->SuppressStatus = FALSE;

	return Session->MsGina.Functions.WlxInitialize(
		Session->InteractiveWindowStationName,
		(HANDLE)Session,
		NULL,
		(PVOID)&FunctionTable,
		&Session->MsGina.Context);
}

BOOL
CreateWindowStationAndDesktops(
	IN OUT PWLSESSION Session)
{
	/*
	 * Create the interactive window station
	 */
	Session->InteractiveWindowStationName = L"WinSta0";
	Session->InteractiveWindowStation = CreateWindowStation(
		Session->InteractiveWindowStationName,
		0,
		WINSTA_CREATEDESKTOP,
		NULL);
	if (!Session->InteractiveWindowStation)
	{
		ERR("WL: Failed to create window station (%lu)\n", GetLastError());
		return FALSE;
	}
	SetProcessWindowStation(Session->InteractiveWindowStation);

	/*
	 * Create the application desktop
	 */
	Session->ApplicationDesktop = CreateDesktop(
		L"Default",
		NULL,
		NULL,
		0, /* FIXME: Set some flags */
		GENERIC_ALL,
		NULL);
	if (!Session->ApplicationDesktop)
	{
		ERR("WL: Failed to create Default desktop (%lu)\n", GetLastError());
		return FALSE;
	}

	/*
	 * Create the winlogon desktop
	 */
	Session->WinlogonDesktop = CreateDesktop(
		WINLOGON_DESKTOP,
		NULL,
		NULL,
		0, /* FIXME: Set some flags */
		GENERIC_ALL,
		NULL);
	if (!Session->WinlogonDesktop)
	{
		ERR("WL: Failed to create Winlogon desktop (%lu)\n", GetLastError());
		return FALSE;
	}

	/*
	 * Create the screen saver desktop
	 */
	Session->ScreenSaverDesktop = CreateDesktop(
		L"Screen-Saver",
		NULL,
		NULL,
		0, /* FIXME: Set some flags */
		GENERIC_ALL,
		NULL);
	if(!Session->ScreenSaverDesktop)
	{
		ERR("WL: Failed to create Screen-Saver desktop (%lu)\n", GetLastError());
		return FALSE;
	}

	/* FIXME: big HACK */
	Session->WinlogonDesktop = Session->ApplicationDesktop;

	/*
	 * Switch to winlogon desktop
	*/
	if (!SetThreadDesktop(Session->WinlogonDesktop) ||
	    !SwitchDesktop(Session->WinlogonDesktop))
	{
		ERR("WL: Cannot switch to Winlogon desktop (%lu)\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}
