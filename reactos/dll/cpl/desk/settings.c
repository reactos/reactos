/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/settings.c
 * PURPOSE:         Settings property page
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "desk.h"
#include "monslctl.h"

typedef struct _GLOBAL_DATA
{
	PDISPLAY_DEVICE_ENTRY DisplayDeviceList;
	PDISPLAY_DEVICE_ENTRY CurrentDisplayDevice;
	HBITMAP hBitmap;
	int cxSource;
	int cySource;
} GLOBAL_DATA, *PGLOBAL_DATA;

static VOID
UpdateDisplay(IN HWND hwndDlg, PGLOBAL_DATA pGlobalData, IN BOOL bUpdateThumb)
{
	TCHAR Buffer[64];
	TCHAR Pixel[64];
	DWORD index;

	LoadString(hApplet, IDS_PIXEL, Pixel, sizeof(Pixel) / sizeof(TCHAR));
	_stprintf(Buffer, Pixel, pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth, pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight, Pixel);
	SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION_TEXT, WM_SETTEXT, 0, (LPARAM)Buffer);

	for (index = 0; index < pGlobalData->CurrentDisplayDevice->ResolutionsCount; index++)
	{
		if (pGlobalData->CurrentDisplayDevice->Resolutions[index].dmPelsWidth == pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth &&
		    pGlobalData->CurrentDisplayDevice->Resolutions[index].dmPelsHeight == pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight)
		{
			if (bUpdateThumb)
				SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION, TBM_SETPOS, TRUE, index);
			break;
		}
	}
	if (LoadString(hApplet, (2900 + pGlobalData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel), Buffer, sizeof(Buffer) / sizeof(TCHAR)))
		SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)Buffer);
}

static PSETTINGS_ENTRY
GetPossibleSettings(IN LPCTSTR DeviceName, OUT DWORD* pSettingsCount, OUT PSETTINGS_ENTRY* CurrentSettings)
{
	DEVMODE devmode;
	DWORD NbSettings = 0;
	DWORD iMode = 0;
	DWORD dwFlags = 0;
	PSETTINGS_ENTRY Settings = NULL;
	HDC hDC;
	PSETTINGS_ENTRY Current;
	DWORD bpp, xres, yres, checkbpp;
    DWORD curDispFreq;


	/* Get current settings */
	*CurrentSettings = NULL;
	hDC = CreateIC(NULL, DeviceName, NULL, NULL);
	bpp = GetDeviceCaps(hDC, PLANES);
	bpp *= GetDeviceCaps(hDC, BITSPIXEL);
	xres = GetDeviceCaps(hDC, HORZRES);
	yres = GetDeviceCaps(hDC, VERTRES);
	DeleteDC(hDC);

	/* List all settings */
	devmode.dmSize = (WORD)sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;

	if (!EnumDisplaySettingsEx(DeviceName, ENUM_CURRENT_SETTINGS, &devmode, dwFlags))
		return NULL;

	curDispFreq = devmode.dmDisplayFrequency;

	while (EnumDisplaySettingsEx(DeviceName, iMode, &devmode, dwFlags))
	{
		if ((devmode.dmBitsPerPel==8 ||
			 devmode.dmBitsPerPel==16 ||
			 devmode.dmBitsPerPel==24 ||
			 devmode.dmBitsPerPel==32) &&
			 devmode.dmDisplayFrequency==curDispFreq)
		{
			checkbpp=1;
		}
		else
			checkbpp=0;

		if (devmode.dmPelsWidth < 640 ||
			devmode.dmPelsHeight < 480 || checkbpp == 0)
		{
			iMode++;
			continue;
		}

		Current = HeapAlloc(GetProcessHeap(), 0, sizeof(SETTINGS_ENTRY));
		if (Current != NULL)
		{
			/* Sort resolutions by increasing height, and BPP */
			PSETTINGS_ENTRY Previous = NULL;
			PSETTINGS_ENTRY Next = Settings;
			Current->dmPelsWidth = devmode.dmPelsWidth;
			Current->dmPelsHeight = devmode.dmPelsHeight;
			Current->dmBitsPerPel = devmode.dmBitsPerPel;
			while (Next != NULL && (
			       Next->dmPelsWidth < Current->dmPelsWidth ||
			       (Next->dmPelsWidth == Current->dmPelsWidth && Next->dmPelsHeight < Current->dmPelsHeight) ||
			       (Next->dmPelsHeight == Current->dmPelsHeight &&
			        Next->dmPelsWidth == Current->dmPelsWidth &&
			        Next->dmBitsPerPel < Current->dmBitsPerPel )))
			{
				Previous = Next;
				Next = Next->Flink;
			}
			Current->Blink = Previous;
			Current->Flink = Next;
			if (Previous == NULL)
				Settings = Current;
			else
				Previous->Flink = Current;
			if (Next != NULL)
				Next->Blink = Current;
			if (devmode.dmPelsWidth == xres && devmode.dmPelsHeight == yres && devmode.dmBitsPerPel == bpp)
			{
				*CurrentSettings = Current;
			}
			NbSettings++;
		}
		iMode++;
	}

	*pSettingsCount = NbSettings;
	return Settings;
}

static BOOL
AddDisplayDevice(IN PGLOBAL_DATA pGlobalData, IN const DISPLAY_DEVICE *DisplayDevice)
{
	PDISPLAY_DEVICE_ENTRY newEntry = NULL;
	LPTSTR description = NULL;
	LPTSTR name = NULL;
	LPTSTR key = NULL;
	LPTSTR devid = NULL;
	DWORD descriptionSize, nameSize, keySize, devidSize;
	PSETTINGS_ENTRY Current;
	DWORD ResolutionsCount = 1;
	DWORD i;

	newEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(DISPLAY_DEVICE_ENTRY));
	memset(newEntry, 0, sizeof(DISPLAY_DEVICE_ENTRY));
	if (!newEntry) goto ByeBye;

	newEntry->Settings = GetPossibleSettings(DisplayDevice->DeviceName, &newEntry->SettingsCount, &newEntry->CurrentSettings);
	if (!newEntry->Settings) goto ByeBye;

	newEntry->InitialSettings.dmPelsWidth = newEntry->CurrentSettings->dmPelsWidth;
	newEntry->InitialSettings.dmPelsHeight = newEntry->CurrentSettings->dmPelsHeight;
	newEntry->InitialSettings.dmBitsPerPel = newEntry->CurrentSettings->dmBitsPerPel;

	/* Count different resolutions */
	for (Current = newEntry->Settings; Current != NULL; Current = Current->Flink)
	{
		if (Current->Flink != NULL &&
			((Current->dmPelsWidth != Current->Flink->dmPelsWidth) &&
			(Current->dmPelsHeight != Current->Flink->dmPelsHeight)))
		{
			ResolutionsCount++;
		}
	}

	newEntry->Resolutions = HeapAlloc(GetProcessHeap(), 0, ResolutionsCount * sizeof(RESOLUTION_INFO));
	if (!newEntry->Resolutions) goto ByeBye;

	newEntry->ResolutionsCount = ResolutionsCount;

	/* Fill resolutions infos */
	for (Current = newEntry->Settings, i = 0; Current != NULL; Current = Current->Flink)
	{
		if (Current->Flink == NULL ||
			(Current->Flink != NULL &&
			((Current->dmPelsWidth != Current->Flink->dmPelsWidth) &&
			(Current->dmPelsHeight != Current->Flink->dmPelsHeight))))
		{
			newEntry->Resolutions[i].dmPelsWidth = Current->dmPelsWidth;
			newEntry->Resolutions[i].dmPelsHeight = Current->dmPelsHeight;
			i++;
		}
	}
	descriptionSize = (_tcslen(DisplayDevice->DeviceString) + 1) * sizeof(TCHAR);
	description = HeapAlloc(GetProcessHeap(), 0, descriptionSize);
	if (!description) goto ByeBye;

	nameSize = (_tcslen(DisplayDevice->DeviceName) + 1) * sizeof(TCHAR);
	name = HeapAlloc(GetProcessHeap(), 0, nameSize);
	if (!name) goto ByeBye;

	keySize = (_tcslen(DisplayDevice->DeviceKey) + 1) * sizeof(TCHAR);
	key = HeapAlloc(GetProcessHeap(), 0, keySize);
	if (!key) goto ByeBye;

	devidSize = (_tcslen(DisplayDevice->DeviceID) + 1) * sizeof(TCHAR);
	devid = HeapAlloc(GetProcessHeap(), 0, devidSize);
	if (!devid) goto ByeBye;

	memcpy(description, DisplayDevice->DeviceString, descriptionSize);
	memcpy(name, DisplayDevice->DeviceName, nameSize);
	memcpy(key, DisplayDevice->DeviceKey, keySize);
	memcpy(devid, DisplayDevice->DeviceID, devidSize);
	newEntry->DeviceDescription = description;
	newEntry->DeviceName = name;
	newEntry->DeviceKey = key;
	newEntry->DeviceID = devid;
	newEntry->DeviceStateFlags = DisplayDevice->StateFlags;
	newEntry->Flink = pGlobalData->DisplayDeviceList;
	pGlobalData->DisplayDeviceList = newEntry;
	return TRUE;

ByeBye:
	if (newEntry != NULL)
	{
		if (newEntry->Settings != NULL)
		{
			Current = newEntry->Settings;
			while (Current != NULL)
			{
				PSETTINGS_ENTRY Next = Current->Flink;
				HeapFree(GetProcessHeap(), 0, Current);
				Current = Next;
			}
		}
		if (newEntry->Resolutions != NULL)
			HeapFree(GetProcessHeap(), 0, newEntry->Resolutions);
		HeapFree(GetProcessHeap(), 0, newEntry);
	}
	if (description != NULL)
		HeapFree(GetProcessHeap(), 0, description);
	if (name != NULL)
		HeapFree(GetProcessHeap(), 0, name);
	if (key != NULL)
		HeapFree(GetProcessHeap(), 0, key);
	if (devid != NULL)
		HeapFree(GetProcessHeap(), 0, devid);
	return FALSE;
}

static VOID
OnDisplayDeviceChanged(IN HWND hwndDlg, IN PGLOBAL_DATA pGlobalData, IN PDISPLAY_DEVICE_ENTRY pDeviceEntry)
{
	PSETTINGS_ENTRY Current;
	DWORD index;

	pGlobalData->CurrentDisplayDevice = pDeviceEntry; /* Update global variable */

	/* Fill color depths combo box */
	SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_RESETCONTENT, 0, 0);
	for (Current = pDeviceEntry->Settings; Current != NULL; Current = Current->Flink)
	{
		TCHAR Buffer[64];
		if (LoadString(hApplet, (2900 + Current->dmBitsPerPel), Buffer, sizeof(Buffer) / sizeof(TCHAR)))
		{
			index = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)Buffer);
			if (index == (DWORD)CB_ERR)
			{
				index = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_ADDSTRING, 0, (LPARAM)Buffer);
				SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_SETITEMDATA, index, Current->dmBitsPerPel);
			}
		}
	}

	/* Fill resolutions slider */
	SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION, TBM_CLEARTICS, TRUE, 0);
	SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION, TBM_SETRANGE, TRUE, MAKELONG(0, pDeviceEntry->ResolutionsCount - 1));

	UpdateDisplay(hwndDlg, pGlobalData, TRUE);
}

static VOID
OnInitDialog(IN HWND hwndDlg)
{
	DWORD Result = 0;
	DWORD iDevNum = 0;
	DISPLAY_DEVICE displayDevice;
	BITMAP bitmap;
	PGLOBAL_DATA pGlobalData;

	pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
	if (pGlobalData == NULL)
		return;

	SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

	/* Get video cards list */
	displayDevice.cb = (DWORD)sizeof(DISPLAY_DEVICE);
	while (EnumDisplayDevices(NULL, iDevNum, &displayDevice, 0x1))
	{
		if ((displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0)
		{
			if (AddDisplayDevice(pGlobalData, &displayDevice))
				Result++;
		}
		iDevNum++;
	}
	if (Result == 0)
	{
		/* No adapter found */
		EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_BPP), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_RESOLUTION), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_RESOLUTION_TEXT), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_ADVANCED), FALSE);
	}
	else if (Result == 1)
	{
		MONSL_MONINFO monitors;

		/* Single video adapter */
		SendDlgItemMessage(hwndDlg, IDC_SETTINGS_DEVICE, WM_SETTEXT, 0, (LPARAM)pGlobalData->DisplayDeviceList->DeviceDescription);
		OnDisplayDeviceChanged(hwndDlg, pGlobalData, pGlobalData->DisplayDeviceList);

		monitors.Position.x = monitors.Position.y = 0;
		monitors.Size.cx = pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth;
		monitors.Size.cy = pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight;
		monitors.Flags = 0;
		SendDlgItemMessage(hwndDlg,
						   IDC_SETTINGS_MONSEL,
						   MSLM_SETMONITORSINFO,
						   1,
						   (LPARAM)&monitors);
	}
	else /* FIXME: incomplete! */
	{
		PMONSL_MONINFO pMonitors;
		INT i;

		SendDlgItemMessage(hwndDlg, IDC_SETTINGS_DEVICE, WM_SETTEXT, 0, (LPARAM)pGlobalData->DisplayDeviceList->DeviceDescription);
		OnDisplayDeviceChanged(hwndDlg, pGlobalData, pGlobalData->DisplayDeviceList);

		pMonitors = (PMONSL_MONINFO)HeapAlloc(GetProcessHeap(), 0, sizeof(MONSL_MONINFO) * Result);
		if (pMonitors)
		{
			INT hack = 1280;
			for (i = 0; i < Result; i++)
			{
				pMonitors[i].Position.x = hack * i;
				pMonitors[i].Position.y = 0;
				pMonitors[i].Size.cx = pGlobalData->DisplayDeviceList->CurrentSettings->dmPelsWidth;
				pMonitors[i].Size.cy = pGlobalData->DisplayDeviceList->CurrentSettings->dmPelsHeight;
				pMonitors[i].Flags = 0;
			}

			SendDlgItemMessage(hwndDlg,
							   IDC_SETTINGS_MONSEL,
							   MSLM_SETMONITORSINFO,
							   Result,
							   (LPARAM)pMonitors);

			HeapFree(GetProcessHeap(), 0, pMonitors);
		}
	}

	pGlobalData->hBitmap = LoadImage(hApplet, MAKEINTRESOURCE(IDC_MONITOR), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	if (pGlobalData->hBitmap != NULL)
	{
		GetObject(pGlobalData->hBitmap, sizeof(BITMAP), &bitmap);

		pGlobalData->cxSource = bitmap.bmWidth;
		pGlobalData->cySource = bitmap.bmHeight;
	}
}

static VOID
OnBPPChanged(IN HWND hwndDlg, IN PGLOBAL_DATA pGlobalData)
{
	/* if new BPP is not compatible with resolution:
	 * 1) try to find the nearest smaller matching resolution
	 * 2) otherwise, get the nearest bigger resolution
	 */
	PSETTINGS_ENTRY Current;
	DWORD dmNewBitsPerPel;
	DWORD index;
	TCHAR Buffer[64];

	SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, WM_GETTEXT, (WPARAM)(sizeof(Buffer) / sizeof(TCHAR)), (LPARAM)Buffer);
	index = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)Buffer);
	dmNewBitsPerPel = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_GETITEMDATA, index, 0);

	/* find if new parameters are valid */
	Current = pGlobalData->CurrentDisplayDevice->CurrentSettings;
	if (dmNewBitsPerPel == Current->dmBitsPerPel)
	{
		/* no change */
		return;
	}

	PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

	if (dmNewBitsPerPel < Current->dmBitsPerPel)
	{
		Current = Current->Blink;
		while (Current != NULL)
		{
			if (Current->dmBitsPerPel == dmNewBitsPerPel
			 && Current->dmPelsHeight == pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight
			 && Current->dmPelsWidth == pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth)
			{
				pGlobalData->CurrentDisplayDevice->CurrentSettings = Current;
				UpdateDisplay(hwndDlg, pGlobalData, TRUE);
				return;
			}
			Current = Current->Blink;
		}
	}
	else
	{
		Current = Current->Flink;
		while (Current != NULL)
		{
			if (Current->dmBitsPerPel == dmNewBitsPerPel
			 && Current->dmPelsHeight == pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight
			 && Current->dmPelsWidth == pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth)
			{
				pGlobalData->CurrentDisplayDevice->CurrentSettings = Current;
				UpdateDisplay(hwndDlg, pGlobalData, TRUE);
				return;
			}
			Current = Current->Flink;
		}
	}

	/* search smaller resolution compatible with current color depth */
	Current = pGlobalData->CurrentDisplayDevice->CurrentSettings->Blink;
	while (Current != NULL)
	{
		if (Current->dmBitsPerPel == dmNewBitsPerPel)
		{
			pGlobalData->CurrentDisplayDevice->CurrentSettings = Current;
			UpdateDisplay(hwndDlg, pGlobalData, TRUE);
			return;
		}
		Current = Current->Blink;
	}

	/* search bigger resolution compatible with current color depth */
	Current = pGlobalData->CurrentDisplayDevice->CurrentSettings->Flink;
	while (Current != NULL)
	{
		if (Current->dmBitsPerPel == dmNewBitsPerPel)
		{
			pGlobalData->CurrentDisplayDevice->CurrentSettings = Current;
			UpdateDisplay(hwndDlg, pGlobalData, TRUE);
			return;
		}
		Current = Current->Flink;
	}

	/* we shouldn't go there */
}

static VOID
OnResolutionChanged(IN HWND hwndDlg, IN PGLOBAL_DATA pGlobalData, IN DWORD NewPosition,
                    IN BOOL bUpdateThumb)
{
	/* if new resolution is not compatible with color depth:
	 * 1) try to find the nearest bigger matching color depth
	 * 2) otherwise, get the nearest smaller color depth
	 */
	PSETTINGS_ENTRY Current;
	DWORD dmNewPelsHeight = pGlobalData->CurrentDisplayDevice->Resolutions[NewPosition].dmPelsHeight;
	DWORD dmNewPelsWidth = pGlobalData->CurrentDisplayDevice->Resolutions[NewPosition].dmPelsWidth;

	/* find if new parameters are valid */
	Current = pGlobalData->CurrentDisplayDevice->CurrentSettings;
	if (dmNewPelsHeight == Current->dmPelsHeight && dmNewPelsWidth == Current->dmPelsWidth)
	{
		/* no change */
		return;
	}

	PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

	if (dmNewPelsHeight < Current->dmPelsHeight)
	{
		Current = Current->Blink;
		while (Current != NULL)
		{
			if (Current->dmPelsHeight == dmNewPelsHeight
			 && Current->dmPelsWidth == dmNewPelsWidth
			 && Current->dmBitsPerPel == pGlobalData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel)
			{
				pGlobalData->CurrentDisplayDevice->CurrentSettings = Current;
				UpdateDisplay(hwndDlg, pGlobalData, bUpdateThumb);
				return;
			}
			Current = Current->Blink;
		}
	}
	else
	{
		Current = Current->Flink;
		while (Current != NULL)
		{
			if (Current->dmPelsHeight == dmNewPelsHeight
			 && Current->dmPelsWidth == dmNewPelsWidth
			 && Current->dmBitsPerPel == pGlobalData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel)
			{
				pGlobalData->CurrentDisplayDevice->CurrentSettings = Current;
				UpdateDisplay(hwndDlg, pGlobalData, bUpdateThumb);
				return;
			}
			Current = Current->Flink;
		}
	}

	/* search bigger color depth compatible with current resolution */
	Current = pGlobalData->CurrentDisplayDevice->CurrentSettings->Flink;
	while (Current != NULL)
	{
		if (dmNewPelsHeight == Current->dmPelsHeight && dmNewPelsWidth == Current->dmPelsWidth)
		{
			pGlobalData->CurrentDisplayDevice->CurrentSettings = Current;
			UpdateDisplay(hwndDlg, pGlobalData, bUpdateThumb);
			return;
		}
		Current = Current->Flink;
	}

	/* search smaller color depth compatible with current resolution */
	Current = pGlobalData->CurrentDisplayDevice->CurrentSettings->Blink;
	while (Current != NULL)
	{
		if (dmNewPelsHeight == Current->dmPelsHeight && dmNewPelsWidth == Current->dmPelsWidth)
		{
			pGlobalData->CurrentDisplayDevice->CurrentSettings = Current;
			UpdateDisplay(hwndDlg, pGlobalData, bUpdateThumb);
			return;
		}
		Current = Current->Blink;
	}

	/* we shouldn't go there */
}

/* Property sheet page callback */
UINT CALLBACK
SettingsPageCallbackProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    UINT Ret = 0;

    switch (uMsg)
    {
        case PSPCB_CREATE:
            Ret = RegisterMonitorSelectionControl(hApplet);
            break;

        case PSPCB_RELEASE:
            UnregisterMonitorSelectionControl(hApplet);
            break;
    }

    return Ret;
}

/* Property page dialog callback */
INT_PTR CALLBACK
SettingsPageProc(IN HWND hwndDlg, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam)
{
	PGLOBAL_DATA pGlobalData;

	pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

	switch(uMsg)
	{
		case WM_INITDIALOG:
			OnInitDialog(hwndDlg);
			break;
		case WM_COMMAND:
		{
			DWORD controlId = LOWORD(wParam);
			DWORD command   = HIWORD(wParam);

			if (controlId == IDC_SETTINGS_ADVANCED && command == BN_CLICKED)
				DisplayAdvancedSettings(hwndDlg, pGlobalData->CurrentDisplayDevice);
			else if (controlId == IDC_SETTINGS_BPP && command == CBN_SELCHANGE)
				OnBPPChanged(hwndDlg, pGlobalData);
			break;
		}
		case WM_HSCROLL:
		{
			switch (LOWORD(wParam))
			{
				case TB_LINEUP:
				case TB_LINEDOWN:
				case TB_PAGEUP:
				case TB_PAGEDOWN:
				case TB_TOP:
				case TB_BOTTOM:
				case TB_ENDTRACK:
				{
					DWORD newPosition = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION, TBM_GETPOS, 0, 0);
					OnResolutionChanged(hwndDlg, pGlobalData, newPosition, TRUE);
					break;
				}

				case TB_THUMBTRACK:
					OnResolutionChanged(hwndDlg, pGlobalData, HIWORD(wParam), FALSE);
					break;
			}
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;
			if (lpnm->code == (UINT)PSN_APPLY)
			{
				if (pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth != pGlobalData->CurrentDisplayDevice->InitialSettings.dmPelsWidth
				 || pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight != pGlobalData->CurrentDisplayDevice->InitialSettings.dmPelsHeight
				 || pGlobalData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel != pGlobalData->CurrentDisplayDevice->InitialSettings.dmBitsPerPel)
				{
					/* FIXME: Need to test changes */
					/* Apply new settings */
					LONG rc;
					DEVMODE devmode;
					RtlZeroMemory(&devmode, sizeof(DEVMODE));
					devmode.dmSize = (WORD)sizeof(DEVMODE);
					devmode.dmPelsWidth = pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth;
					devmode.dmPelsHeight = pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight;
					devmode.dmBitsPerPel = pGlobalData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel;
					devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
					rc = ChangeDisplaySettingsEx(
						pGlobalData->CurrentDisplayDevice->DeviceName,
						&devmode,
						NULL,
						CDS_UPDATEREGISTRY,
						NULL);
					switch (rc)
					{
						case DISP_CHANGE_SUCCESSFUL:
							pGlobalData->CurrentDisplayDevice->InitialSettings.dmPelsWidth = pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth;
							pGlobalData->CurrentDisplayDevice->InitialSettings.dmPelsHeight = pGlobalData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight;
							pGlobalData->CurrentDisplayDevice->InitialSettings.dmBitsPerPel = pGlobalData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel;
							break;
						case DISP_CHANGE_FAILED:
							MessageBox(NULL, TEXT("Failed to apply new settings..."), TEXT("Display settings"), MB_OK | MB_ICONSTOP);
							break;
						case DISP_CHANGE_RESTART:
							MessageBox(NULL, TEXT("You need to restart your computer to apply changes."), TEXT("Display settings"), MB_OK | MB_ICONINFORMATION);
							break;
						default:
							MessageBox(NULL, TEXT("Unknown error when applying new settings..."), TEXT("Display settings"), MB_OK | MB_ICONSTOP);
							break;
					}
				}
			}
			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc, hdcMem;

			hdc = BeginPaint(hwndDlg, &ps);

			hdcMem = CreateCompatibleDC(hdc);
			SelectObject(hdcMem, pGlobalData->hBitmap);
/*
			TransparentBlt(hdc, 98, 0,
				       pGlobalData->cxSource,
				       pGlobalData->cySource, hdcMem, 0, 0,
				       pGlobalData->cxSource,
				       pGlobalData->cySource, 0xFF80FF);
*/
			DeleteDC(hdcMem);
			EndPaint(hwndDlg, &ps);

			break;
		}

		case WM_DESTROY:
		{
			PDISPLAY_DEVICE_ENTRY Current = pGlobalData->DisplayDeviceList;
			while (Current != NULL)
			{
				PDISPLAY_DEVICE_ENTRY Next = Current->Flink;
				PSETTINGS_ENTRY CurrentSettings = Current->Settings;
				while (CurrentSettings != NULL)
				{
					PSETTINGS_ENTRY NextSettings = CurrentSettings->Flink;
					HeapFree(GetProcessHeap(), 0, CurrentSettings);
					CurrentSettings = NextSettings;
				}
				HeapFree(GetProcessHeap(), 0, Current);
				Current = Next;
			}

			DeleteObject(pGlobalData->hBitmap);

			HeapFree(GetProcessHeap(), 0, pGlobalData);
		}
	}
	return FALSE;
}
