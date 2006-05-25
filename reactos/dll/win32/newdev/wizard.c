/*
 * New device installer (newdev.dll)
 *
 * Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define YDEBUG
#include "newdev.h"

WINE_DEFAULT_DEBUG_CHANNEL(newdev);

HANDLE hThread;

static VOID
CenterWindow(
	IN HWND hWnd)
{
	HWND hWndParent;
	RECT rcParent;
	RECT rcWindow;

	hWndParent = GetParent(hWnd);
	if (hWndParent == NULL)
		hWndParent = GetDesktopWindow();

	GetWindowRect(hWndParent, &rcParent);
	GetWindowRect(hWnd, &rcWindow);

	SetWindowPos(
		hWnd,
		HWND_TOP,
		((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
		((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
		0,
		0,
		SWP_NOSIZE);
}

static BOOL
CanDisableDevice(
	IN DEVINST DevInst,
	IN HMACHINE hMachine,
	OUT BOOL *CanDisable)
{
#if 0
	/* hpoussin, Dec 2005. I've disabled this code because
	 * ntoskrnl never sets the DN_DISABLEABLE flag.
	 */
	CONFIGRET cr;
	ULONG Status, ProblemNumber;
	BOOL Ret = FALSE;

	cr = CM_Get_DevNode_Status_Ex(
		&Status,
		&ProblemNumber,
		DevInst,
		0,
		hMachine);
	if (cr == CR_SUCCESS)
	{
		*CanDisable = ((Status & DN_DISABLEABLE) != 0);
	Ret = TRUE;
	}

	return Ret;
#else
	*CanDisable = TRUE;
	return TRUE;
#endif
}

static BOOL
IsDeviceStarted(
	IN DEVINST DevInst,
	IN HMACHINE hMachine,
	OUT BOOL *IsEnabled)
{
	CONFIGRET cr;
	ULONG Status, ProblemNumber;
	BOOL Ret = FALSE;

	cr = CM_Get_DevNode_Status_Ex(
		&Status,
		&ProblemNumber,
		DevInst,
		0,
		hMachine);
	if (cr == CR_SUCCESS)
	{
		*IsEnabled = ((Status & DN_STARTED) != 0);
	Ret = TRUE;
	}

	return Ret;
}

static BOOL
StartDevice(
	IN HDEVINFO DeviceInfoSet,
	IN PSP_DEVINFO_DATA DevInfoData OPTIONAL,
	IN BOOL bEnable,
	IN DWORD HardwareProfile OPTIONAL,
	OUT BOOL *bNeedReboot OPTIONAL)
{
	SP_PROPCHANGE_PARAMS pcp;
	SP_DEVINSTALL_PARAMS dp;
	DWORD LastErr;
	BOOL Ret = FALSE;

	pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
	pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
	pcp.HwProfile = HardwareProfile;

	if (bEnable)
	{
		/* try to enable the device on the global profile */
		pcp.StateChange = DICS_ENABLE;
		pcp.Scope = DICS_FLAG_GLOBAL;

		/* ignore errors */
		LastErr = GetLastError();
		if (SetupDiSetClassInstallParams(
			DeviceInfoSet,
			DevInfoData,
			&pcp.ClassInstallHeader,
			sizeof(SP_PROPCHANGE_PARAMS)))
		{
			SetupDiCallClassInstaller(
				DIF_PROPERTYCHANGE,
				DeviceInfoSet,
				DevInfoData);
		}
		SetLastError(LastErr);
	}

	/* try config-specific */
	pcp.StateChange = (bEnable ? DICS_ENABLE : DICS_DISABLE);
	pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;

	if (SetupDiSetClassInstallParams(
			DeviceInfoSet,
			DevInfoData,
			&pcp.ClassInstallHeader,
			sizeof(SP_PROPCHANGE_PARAMS)) &&
		SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
			DeviceInfoSet,
			DevInfoData))
	{
		dp.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
		if (SetupDiGetDeviceInstallParams(
			DeviceInfoSet,
			DevInfoData,
			&dp))
		{
			if (bNeedReboot != NULL)
			{
				*bNeedReboot = ((dp.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)) != 0);
			}

			Ret = TRUE;
		}
	}
	return Ret;
}

static BOOL
PrepareFoldersToScan(
	IN PDEVINSTDATA DevInstData,
	IN HWND hwndDlg)
{
	FIXME("Include removable devices: %s\n", IsDlgButtonChecked(hwndDlg, IDC_CHECK_MEDIA) ? "yes" : "no");
	FIXME("Include custom path      : %s\n", IsDlgButtonChecked(hwndDlg, IDC_CHECK_PATH) ? "yes" : "no");
	return TRUE;
}

static DWORD WINAPI
FindDriverProc(
	IN LPVOID lpParam)
{
	TCHAR drive[] = {'?',':',0};
	size_t nType;
	DWORD dwDrives;
	PDEVINSTDATA DevInstData;
	DWORD config_flags;
	UINT i = 1;

	DevInstData = (PDEVINSTDATA)lpParam;
	DPRINT1("FindDriverProc(%p)\n", DevInstData->CustomSearchPath);
	if (DevInstData->CustomSearchPath)
	{
		LPCTSTR Path;
		for (Path = DevInstData->CustomSearchPath; *Path != '\0'; Path += _tcslen(Path) + 1)
			DPRINT1("Path %S\n", Path);
	}

	dwDrives = GetLogicalDrives();
	for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
	{
		if (dwDrives & i)
		{
			nType = GetDriveType(drive);
			if (nType == DRIVE_REMOVABLE || nType == DRIVE_CDROM)
			{
				/* search for valid inf file */
				if (SearchDriverRecursive(DevInstData, drive))
				{
					InstallCurrentDriver(DevInstData);
					PostMessage(DevInstData->hDialog, WM_SEARCH_FINISHED, 1, 0);
					return 0;
				}
			}
		}
		i <<= 1;
	}

	/* update device configuration */
	if (SetupDiGetDeviceRegistryProperty(
		DevInstData->hDevInfo,
		&DevInstData->devInfoData,
		SPDRP_CONFIGFLAGS,
		NULL,
		(BYTE *)&config_flags,
		sizeof(config_flags),
		NULL))
	{
		config_flags |= CONFIGFLAG_FAILEDINSTALL;
		SetupDiSetDeviceRegistryProperty(
			DevInstData->hDevInfo,
			&DevInstData->devInfoData,
			SPDRP_CONFIGFLAGS,
			(BYTE *)&config_flags, sizeof(config_flags));
	}

	PostMessage(DevInstData->hDialog, WM_SEARCH_FINISHED, 0, 0);
	return 0;
}

static INT_PTR CALLBACK
WelcomeDlgProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	PDEVINSTDATA DevInstData;

	/* Retrieve pointer to the global setup data */
	DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hwndControl;
			DWORD dwStyle;

			/* Get pointer to the global setup data */
			DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

			hwndControl = GetParent(hwndDlg);

			/* Center the wizard window */
			CenterWindow(hwndControl);

			/* Hide the system menu */
			dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
			SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

			/* Set title font */
			SendDlgItemMessage(
				hwndDlg,
				IDC_WELCOMETITLE,
				WM_SETFONT,
				(WPARAM)DevInstData->hTitleFont,
				(LPARAM)TRUE);

			SendDlgItemMessage(
				hwndDlg,
				IDC_DEVICE,
				WM_SETTEXT,
				0,
				(LPARAM)DevInstData->buffer);

			SendDlgItemMessage(
				hwndDlg,
				IDC_RADIO_AUTO,
				BM_SETCHECK,
				(WPARAM)TRUE,
				(LPARAM)0);
			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;

			switch (lpnm->code)
			{
				case PSN_SETACTIVE:
					/* Enable the Next button */
					PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
					break;

				case PSN_WIZNEXT:
					/* Handle a Next button click, if necessary */
					if (SendDlgItemMessage(hwndDlg, IDC_RADIO_AUTO, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
						PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_SEARCHDRV);
					return TRUE;

				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}

static void
IncludePath(HWND Dlg, BOOL Enabled)
{
	EnableWindow(GetDlgItem(Dlg, IDC_COMBO_PATH), Enabled);
	EnableWindow(GetDlgItem(Dlg, IDC_BROWSE), /*FIXME: Enabled*/ FALSE);
}

static void
AutoDriver(HWND Dlg, BOOL Enabled)
{
	EnableWindow(GetDlgItem(Dlg, IDC_CHECK_MEDIA), Enabled);
	EnableWindow(GetDlgItem(Dlg, IDC_CHECK_PATH), Enabled);
	IncludePath(Dlg, Enabled & IsDlgButtonChecked(Dlg, IDC_CHECK_PATH));
}

static INT_PTR CALLBACK
CHSourceDlgProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	PDEVINSTDATA DevInstData;

	/* Retrieve pointer to the global setup data */
	DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hwndControl;
			DWORD dwStyle;

			/* Get pointer to the global setup data */
			DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

			hwndControl = GetParent(hwndDlg);

			/* Center the wizard window */
			CenterWindow(hwndControl);

			/* Hide the system menu */
			dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
			SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

			SendDlgItemMessage(
				hwndDlg,
				IDC_RADIO_SEARCHHERE,
				BM_SETCHECK,
				(WPARAM)TRUE,
				(LPARAM)0);
			AutoDriver(hwndDlg, TRUE);
			IncludePath(hwndDlg, FALSE);

			/* Disable manual driver choice for now */
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_CHOOSE), FALSE);
			/* Disable custom path for now */
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_PATH), FALSE);
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_RADIO_SEARCHHERE:
					AutoDriver(hwndDlg, TRUE);
					return TRUE;

				case IDC_RADIO_CHOOSE:
					AutoDriver(hwndDlg, FALSE);
					return TRUE;

				case IDC_CHECK_PATH:
					IncludePath(hwndDlg, IsDlgButtonChecked(hwndDlg, IDC_CHECK_PATH));
					return TRUE;

				case IDC_BROWSE:
					/* FIXME: set the IDC_COMBO_PATH text */
					FIXME("Should display browse folder dialog\n");
					return FALSE;
			}
			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;

			switch (lpnm->code)
			{
				case PSN_SETACTIVE:
					/* Enable the Next and Back buttons */
					PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
					break;

				case PSN_WIZNEXT:
					/* Handle a Next button click, if necessary */
					if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_SEARCHHERE))
					{
						HeapFree(GetProcessHeap(), 0, DevInstData->CustomSearchPath);
						DevInstData->CustomSearchPath = NULL;
						if (PrepareFoldersToScan(DevInstData, hwndDlg))
							PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_SEARCHDRV);
					}
					else
						/* FIXME */;
					return TRUE;

				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}

static INT_PTR CALLBACK
SearchDrvDlgProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	PDEVINSTDATA DevInstData;
	DWORD dwThreadId;

	/* Retrieve pointer to the global setup data */
	DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hwndControl;
			DWORD dwStyle;

			/* Get pointer to the global setup data */
			DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

			DevInstData->hDialog = hwndDlg;
			hwndControl = GetParent(hwndDlg);

			/* Center the wizard window */
			CenterWindow(hwndControl);

			SendDlgItemMessage(
				hwndDlg,
				IDC_DEVICE,
				WM_SETTEXT,
				0,
				(LPARAM)DevInstData->buffer);

			/* Hide the system menu */
			dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
			SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
			break;
		}

		case WM_SEARCH_FINISHED:
		{
			CloseHandle(hThread);
			hThread = 0;
			if (wParam == 0)
				PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_NODRIVER);
			else
				PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_FINISHPAGE);
			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;

			switch (lpnm->code)
			{
				case PSN_SETACTIVE:
					PropSheet_SetWizButtons(GetParent(hwndDlg), !PSWIZB_NEXT | !PSWIZB_BACK);
					hThread = CreateThread(NULL, 0, FindDriverProc, DevInstData, 0, &dwThreadId);
					break;

				case PSN_KILLACTIVE:
					if (hThread != 0)
					{
						SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
						return TRUE;
					}
					break;

				case PSN_WIZNEXT:
					/* Handle a Next button click, if necessary */
					break;

				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}

static INT_PTR CALLBACK
InstallDrvDlgProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	return FALSE;
}

static INT_PTR CALLBACK
NoDriverDlgProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	PDEVINSTDATA DevInstData;

	/* Get pointer to the global setup data */
	DevInstData = (PDEVINSTDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hwndControl;
			BOOL DisableableDevice = FALSE;

			DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

			hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
			ShowWindow (hwndControl, SW_HIDE);
			EnableWindow (hwndControl, FALSE);

			/* Set title font */
			SendDlgItemMessage(
				hwndDlg,
				IDC_FINISHTITLE,
				WM_SETFONT,
				(WPARAM)DevInstData->hTitleFont,
				(LPARAM)TRUE);

			/* disable the "do not show this dialog anymore" checkbox
			 if the device cannot be disabled */
			CanDisableDevice(
				DevInstData->devInfoData.DevInst,
				NULL,
				&DisableableDevice);
			EnableWindow(
				GetDlgItem(hwndDlg, IDC_DONOTSHOWDLG),
				DisableableDevice);
			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;

			switch (lpnm->code)
			{
				case PSN_SETACTIVE:
					/* Enable the correct buttons on for the active page */
					PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
					break;

				case PSN_WIZBACK:
					/* Handle a Back button click, if necessary */
					PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_WELCOMEPAGE);
					return TRUE;

				case PSN_WIZFINISH:
				{
					BOOL DisableableDevice = FALSE;
					BOOL IsStarted = FALSE;

					if (CanDisableDevice(DevInstData->devInfoData.DevInst,
							NULL,
							&DisableableDevice) &&
						DisableableDevice &&
						IsDeviceStarted(
							DevInstData->devInfoData.DevInst,
							NULL,
							&IsStarted) &&
						!IsStarted &&
						SendDlgItemMessage(
							hwndDlg,
							IDC_DONOTSHOWDLG,
							BM_GETCHECK,
							(WPARAM)0, (LPARAM)0) == BST_CHECKED)
					{
						/* disable the device */
						StartDevice(
							DevInstData->hDevInfo,
							&DevInstData->devInfoData,
							FALSE,
							0,
							NULL);
					}
					break;
				}

				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}

static INT_PTR CALLBACK
FinishDlgProc(
	IN HWND hwndDlg,
	IN UINT uMsg,
	IN WPARAM wParam,
	IN LPARAM lParam)
{
	PDEVINSTDATA DevInstData;

	/* Retrieve pointer to the global setup data */
	DevInstData = (PDEVINSTDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hwndControl;

			/* Get pointer to the global setup data */
			DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

			hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
			ShowWindow (hwndControl, SW_HIDE);
			EnableWindow (hwndControl, FALSE);

			SendDlgItemMessage(
				hwndDlg,
				IDC_DEVICE,
				WM_SETTEXT,
				0,
				(LPARAM)DevInstData->drvInfoData.Description);

			/* Set title font */
			SendDlgItemMessage(
				hwndDlg,
				IDC_FINISHTITLE,
				WM_SETFONT,
				(WPARAM)DevInstData->hTitleFont,
				(LPARAM)TRUE);
			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;

			switch (lpnm->code)
			{
				case PSN_SETACTIVE:
					/* Enable the correct buttons on for the active page */
					PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
					break;

				case PSN_WIZBACK:
					/* Handle a Back button click, if necessary */
					break;

				case PSN_WIZFINISH:
					/* Handle a Finish button click, if necessary */
					break;

				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}

static HFONT
CreateTitleFont(VOID)
{
	NONCLIENTMETRICS ncm;
	LOGFONT LogFont;
	HDC hdc;
	INT FontSize;
	HFONT hFont;

	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LogFont = ncm.lfMessageFont;
	LogFont.lfWeight = FW_BOLD;
	_tcscpy(LogFont.lfFaceName, _T("MS Shell Dlg"));

	hdc = GetDC(NULL);
	FontSize = 12;
	LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
	hFont = CreateFontIndirect(&LogFont);
	ReleaseDC(NULL, hdc);

	return hFont;
}

BOOL
DisplayWizard(
	IN PDEVINSTDATA DevInstData,
	IN HWND hwndParent,
	IN UINT startPage)
{
	PROPSHEETHEADER psh;
	HPROPSHEETPAGE ahpsp[IDD_FINISHPAGE + 1];
	PROPSHEETPAGE psp;

	/* Create the Welcome page */
	ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
	psp.hInstance = hDllInstance;
	psp.lParam = (LPARAM)DevInstData;
	psp.pfnDlgProc = WelcomeDlgProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOMEPAGE);
	ahpsp[IDD_WELCOMEPAGE] = CreatePropertySheetPage(&psp);

	/* Create the Select Source page */
	psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	psp.pfnDlgProc = CHSourceDlgProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CHSOURCE);
	ahpsp[IDD_CHSOURCE] = CreatePropertySheetPage(&psp);

	/* Create the Search driver page */
	psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	psp.pfnDlgProc = SearchDrvDlgProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_SEARCHDRV);
	ahpsp[IDD_SEARCHDRV] = CreatePropertySheetPage(&psp);

	/* Create the Install driver page */
	psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	psp.pfnDlgProc = InstallDrvDlgProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_INSTALLDRV);
	ahpsp[IDD_INSTALLDRV] = CreatePropertySheetPage(&psp);

	/* Create the Install failed page */
	psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
	psp.pfnDlgProc = NoDriverDlgProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_NODRIVER);
	ahpsp[IDD_NODRIVER] = CreatePropertySheetPage(&psp);

	/* Create the Finish page */
	psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
	psp.pfnDlgProc = FinishDlgProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
	ahpsp[IDD_FINISHPAGE] = CreatePropertySheetPage(&psp);

	/* Create the property sheet */
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
	psh.hInstance = hDllInstance;
	psh.hwndParent = hwndParent;
	psh.nPages = 7;
	psh.nStartPage = startPage;
	psh.phpage = ahpsp;
	psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
	psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

	/* Create title font */
	DevInstData->hTitleFont = CreateTitleFont();

	/* Display the wizard */
	PropertySheet(&psh);

	DeleteObject(DevInstData->hTitleFont);

	return TRUE;
}
