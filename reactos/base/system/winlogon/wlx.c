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

static DLGPROC PreviousWindowProc;
static UINT_PTR IdTimer;

static INT_PTR CALLBACK
DefaultWlxWindowProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	if (uMsg == WM_TIMER && (UINT_PTR)wParam == IdTimer)
	{
		EndDialog(hwndDlg, -1);
		KillTimer(hwndDlg, IdTimer);
		return TRUE;
	}
	else if (uMsg == WM_INITDIALOG)
	{
		IdTimer = SetTimer(hwndDlg, 0, WLSession->DialogTimeout * 1000, NULL);
		return PreviousWindowProc(hwndDlg, uMsg, wParam, lParam);;
	}
	else if (uMsg == WM_NCDESTROY)
	{
		BOOL ret;
		ret = PreviousWindowProc(hwndDlg, uMsg, wParam, lParam);
		PreviousWindowProc = NULL;
		return ret;
	}
	else
	{
		return PreviousWindowProc(hwndDlg, uMsg, wParam, lParam);
	}
}

/*
 * @implemented
 */
VOID WINAPI
WlxUseCtrlAltDel(
	HANDLE hWlx)
{
	ULONG_PTR OldValue;
	WlxSetOption(hWlx, WLX_OPTION_USE_CTRL_ALT_DEL, TRUE, &OldValue);
}

/*
 * @implemented
 */
VOID WINAPI
WlxSetContextPointer(
	HANDLE hWlx,
	PVOID pWlxContext)
{
	ULONG_PTR OldValue;
	WlxSetOption(hWlx, WLX_OPTION_CONTEXT_POINTER, (ULONG_PTR)pWlxContext, &OldValue);
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
 * @implemented
 */
BOOL WINAPI
WlxSetTimeout(
	HANDLE hWlx,
	DWORD Timeout)
{
	PWLSESSION Session = (PWLSESSION)hWlx;
	Session->DialogTimeout = Timeout;
	return TRUE;
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
	/* FIXME: Provide a custom window proc to be able to handle timeout */
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
	if (PreviousWindowProc != NULL)
		return -1;
	PreviousWindowProc = dlgprc;
	return (int)DialogBoxW(hInst, lpszTemplate, hwndOwner, DefaultWlxWindowProc);
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
	if (PreviousWindowProc != NULL)
		return -1;
	PreviousWindowProc = dlgprc;
	return (int)DialogBoxParamW(hInst, lpszTemplate, hwndOwner, DefaultWlxWindowProc, dwInitParam);
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
	if (PreviousWindowProc != NULL)
		return -1;
	PreviousWindowProc = dlgprc;
	return (int)DialogBoxIndirectW(hInst, hDialogTemplate, hwndOwner, DefaultWlxWindowProc);
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
	if (PreviousWindowProc != NULL)
		return -1;
	PreviousWindowProc = dlgprc;
	return (int)DialogBoxIndirectParamW(hInst, hDialogTemplate, hwndOwner, DefaultWlxWindowProc, dwInitParam);
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

	switch (Option)
	{
		case WLX_OPTION_USE_CTRL_ALT_DEL:
			*OldValue = (ULONG_PTR)Session->Gina.UseCtrlAltDelete;
			Session->Gina.UseCtrlAltDelete = (BOOL)Value;
			return TRUE;
		case WLX_OPTION_CONTEXT_POINTER:
			*OldValue = (ULONG_PTR)Session->Gina.Context;
			Session->Gina.Context = (PVOID)Value;
			return TRUE;
		case WLX_OPTION_USE_SMART_CARD:
			UNIMPLEMENTED;
			return FALSE;
	}

	return FALSE;
}

/*
 * @implemented
 */
BOOL WINAPI
WlxGetOption(
	HANDLE hWlx,
	DWORD Option,
	ULONG_PTR* Value)
{
	PWLSESSION Session = (PWLSESSION)hWlx;

	switch (Option)
	{
		case WLX_OPTION_USE_CTRL_ALT_DEL:
			*Value = (ULONG_PTR)Session->Gina.UseCtrlAltDelete;
			return TRUE;
		case WLX_OPTION_CONTEXT_POINTER:
		{
			*Value = (ULONG_PTR)Session->Gina.Context;
			return TRUE;
		}
		case WLX_OPTION_USE_SMART_CARD:
		case WLX_OPTION_SMART_CARD_PRESENT:
		case WLX_OPTION_SMART_CARD_INFO:
			UNIMPLEMENTED;
			return FALSE;
		case WLX_OPTION_DISPATCH_TABLE_SIZE:
		{
			switch (Session->Gina.Version)
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

	Status = RegOpenKeyExW(
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
	Status = RegQueryValueExW(
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

static BOOL
LoadGina(
	IN OUT PGINAFUNCTIONS Functions,
	OUT DWORD *DllVersion,
	OUT HMODULE *GinaInstance)
{
	HMODULE hGina = NULL;
	WCHAR GinaDll[MAX_PATH + 1];
	BOOL ret = FALSE;

	GinaDll[0] = '\0';
	if (!GetGinaPath(GinaDll, MAX_PATH))
		goto cleanup;
	/* Terminate string */
	GinaDll[MAX_PATH] = '\0';

	if (!(hGina = LoadLibraryW(GinaDll)))
		goto cleanup;

	Functions->WlxNegotiate = (PFWLXNEGOTIATE)GetProcAddress(hGina, "WlxNegotiate");
	Functions->WlxInitialize = (PFWLXINITIALIZE)GetProcAddress(hGina, "WlxInitialize");

	if (!Functions->WlxInitialize)
		goto cleanup;

	if (!Functions->WlxNegotiate)
	{
		/* Assume current version */
		*DllVersion = WLX_CURRENT_VERSION;
	}
	else if (!Functions->WlxNegotiate(WLX_CURRENT_VERSION, DllVersion))
		goto cleanup;

	if (*DllVersion >= WLX_VERSION_1_0)
	{
		Functions->WlxActivateUserShell = (PFWLXACTIVATEUSERSHELL)GetProcAddress(hGina, "WlxActivateUserShell");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxDisplayLockedNotice = (PFWLXDISPLAYLOCKEDNOTICE)GetProcAddress(hGina, "WlxDisplayLockedNotice");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxDisplaySASNotice = (PFWLXDISPLAYSASNOTICE)GetProcAddress(hGina, "WlxDisplaySASNotice");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxIsLockOk = (PFWLXISLOCKOK)GetProcAddress(hGina, "WlxIsLockOk");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxIsLogoffOk = (PFWLXISLOGOFFOK)GetProcAddress(hGina, "WlxIsLogoffOk");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxLoggedOnSAS = (PFWLXLOGGEDONSAS)GetProcAddress(hGina, "WlxLoggedOnSAS");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxLoggedOutSAS = (PFWLXLOGGEDOUTSAS)GetProcAddress(hGina, "WlxLoggedOutSAS");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxLogoff = (PFWLXLOGOFF)GetProcAddress(hGina, "WlxLogoff");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxShutdown = (PFWLXSHUTDOWN)GetProcAddress(hGina, "WlxShutdown");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxWkstaLockedSAS = (PFWLXWKSTALOCKEDSAS)GetProcAddress(hGina, "WlxWkstaLockedSAS");
		if (!Functions->WlxActivateUserShell) goto cleanup;
	}

	if (*DllVersion >= WLX_VERSION_1_1)
	{
		Functions->WlxScreenSaverNotify = (PFWLXSCREENSAVERNOTIFY)GetProcAddress(hGina, "WlxScreenSaverNotify");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxStartApplication = (PFWLXSTARTAPPLICATION)GetProcAddress(hGina, "WlxStartApplication");
		if (!Functions->WlxActivateUserShell) goto cleanup;
	}

	if (*DllVersion >= WLX_VERSION_1_3)
	{
		Functions->WlxDisplayStatusMessage = (PFWLXDISPLAYSTATUSMESSAGE)GetProcAddress(hGina, "WlxDisplayStatusMessage");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxGetStatusMessage = (PFWLXGETSTATUSMESSAGE)GetProcAddress(hGina, "WlxGetStatusMessage");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxNetworkProviderLoad = (PFWLXNETWORKPROVIDERLOAD)GetProcAddress(hGina, "WlxNetworkProviderLoad");
		if (!Functions->WlxActivateUserShell) goto cleanup;
		Functions->WlxRemoveStatusMessage = (PFWLXREMOVESTATUSMESSAGE)GetProcAddress(hGina, "WlxRemoveStatusMessage");
		if (!Functions->WlxActivateUserShell) goto cleanup;
	}

	ret = TRUE;

cleanup:
	if (!ret)
	{
		if (hGina)
			FreeLibrary(hGina);
	}
	else
		*GinaInstance = hGina;
	return ret;
}

BOOL
GinaInit(
	IN OUT PWLSESSION Session)
{
	DWORD GinaDllVersion;

	if (!LoadGina(&Session->Gina.Functions, &GinaDllVersion, &Session->Gina.hDllInstance))
		return FALSE;

	Session->Gina.Context = NULL;
	Session->Gina.Version = GinaDllVersion;
	Session->Gina.UseCtrlAltDelete = FALSE;
	Session->SuppressStatus = FALSE;
	PreviousWindowProc = NULL;

	return Session->Gina.Functions.WlxInitialize(
		Session->InteractiveWindowStationName,
		(HANDLE)Session,
		NULL,
		(PVOID)&FunctionTable,
		&Session->Gina.Context);
}

BOOL
CreateWindowStationAndDesktops(
	IN OUT PWLSESSION Session)
{
	/*
	 * Create the interactive window station
	 */
	Session->InteractiveWindowStationName = L"WinSta0";
	Session->InteractiveWindowStation = CreateWindowStationW(
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
	Session->ApplicationDesktop = CreateDesktopW(
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
	Session->WinlogonDesktop = CreateDesktopW(
		L"Winlogon",
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
	Session->ScreenSaverDesktop = CreateDesktopW(
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
