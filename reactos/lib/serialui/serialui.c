/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SerialUI DLL
 * FILE:        serialui.c
 * PUROPSE:     A dialog box to cunfigure COM port.
 *              Functions to set(and get too) default configuration.
 * PROGRAMMERS: Saveliy Tretiakov (saveliyt@mail.ru)
 * REVISIONS:
 *              ST   (05/04/2005) Created. Implemented drvCommConfigDialog.
 */

#include <serialui.h>

static HINSTANCE hDllInstance;

/************************************
 *
 *  DATA
 *
 ************************************/

struct
{
	DWORD Baud;
	CHAR *Str;
} Bauds[] =
{
	{ CBR_110, "110" },
	{ CBR_300, "300" },
	{ CBR_600, "600" },
	{ CBR_1200, "1200" },
	{ CBR_2400, "2400" },
	{ CBR_4800, "4800" },
	{ CBR_9600, "9600" },
	{ CBR_14400, "14400" },
	{ CBR_19200, "19200" },
	{ CBR_38400, "38400" },
	{ CBR_56000, "56000" },
	{ CBR_57600, "57600" },
	{ CBR_115200, "115200" },
	{ CBR_128000, "128000" },
	{ CBR_256000, "256000" },
	{ 0, 0 }
};

struct
{
 BYTE ByteSize;
 CHAR *Str;
} ByteSizes[] =
{
	{ 5, "5" },
	{ 6, "6" },
	{ 7, "7" },
	{ 8, "8" },
	{ 0, 0 }
};

struct
{
 BYTE Parity;
 UINT StrId;
} Paritys[] =
{
	{ EVENPARITY, IDS_EVENPARITY },
	{ MARKPARITY, IDS_MARKPARITY },
	{ NOPARITY, IDS_NOPARITY },
	{ ODDPARITY, IDS_ODDPARITY },
	{ SPACEPARITY, IDS_SPACEPARITY },
	{ 0, 0 }
};

struct
{
 BYTE StopBit;
 UINT StrId;
} StopBits[] =
{
	{ ONESTOPBIT, IDS_ONESTOPBIT },
	{ ONE5STOPBITS, IDS_ONE5STOPBITS },
	{ TWOSTOPBITS, IDS_TWOSTOPBITS },
	{ 0, 0 }
};


/************************************
 *
 *  DLLMAIN
 *
 ************************************/

BOOL
STDCALL
DllMain(HINSTANCE hInstance,
	DWORD dwReason,
	LPVOID reserved)
{
	if(dwReason==DLL_PROCESS_ATTACH)
	{
		hDllInstance = hInstance;
	}
	else if(dwReason==DLL_THREAD_ATTACH)
	{
		DisableThreadLibraryCalls(hInstance);
	}

	return TRUE;
}


/************************************
 *
 *  EXPORTS
 *
 ************************************/

/*
 * @implemented
 */
BOOL WINAPI drvCommConfigDialogW(LPCWSTR lpszDevice,
	HWND hWnd,
	LPCOMMCONFIG lpCommConfig)
{
	DIALOG_INFO DialogInfo;

	if(!lpszDevice || !lpCommConfig)
	{
		return FALSE;
	}

	DialogInfo.lpszDevice = lpszDevice;
	DialogInfo.lpCC = lpCommConfig;

	return DialogBoxParamW(hDllInstance, MAKEINTRESOURCEW(IDD_COMMDLG),
					hWnd, (DLGPROC)CommDlgProc, (LPARAM)&DialogInfo);
}

/*
 * @implemented
 */
BOOL WINAPI drvCommConfigDialogA(LPCSTR lpszDevice,
	HWND hWnd,
	LPCOMMCONFIG lpCommConfig)
{
	BOOL result;
	UINT len;
	WCHAR *wstr;

	len = MultiByteToWideChar(CP_ACP, 0, lpszDevice, -1, NULL, 0);
	if((wstr = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR))))
	{
		MultiByteToWideChar(CP_ACP, 0, lpszDevice, -1, wstr, len);
		result = drvCommConfigDialogW(wstr, hWnd, lpCommConfig);
		HeapFree(GetProcessHeap(), 0, wstr);
		return result;
	}
	else
		return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI drvSetDefaultCommConfigW(LPCWSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	DWORD dwSize)
{
	UNIMPLEMENTED
}

/*
 * @unimplemented
 */
BOOL WINAPI drvSetDefaultCommConfigA(LPCSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	DWORD dwSize)
{
	UNIMPLEMENTED
}

/*
 * @unimplemented
 */
BOOL WINAPI drvGetDefaultCommConfigW(LPCWSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	LPDWORD lpdwSize)
{
	UNIMPLEMENTED
}

/*
 * @unimplemented
 */
BOOL WINAPI drvGetDefaultCommConfigA(LPCSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	LPDWORD lpdwSize)
{
	UNIMPLEMENTED
}


/************************************
 *
 *  INTERNALS
 *
 ************************************/

LRESULT CommDlgProc(HWND hDlg,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	LPDIALOG_INFO lpDlgInfo = NULL;
	HWND hBox;

	switch (Msg)
	{

		case WM_INITDIALOG:
		{
			WCHAR wstr[255], *format;
			CHAR str[255];
			RECT rc, rcDlg, rcOwner;
			HWND hOwner;
			INT i, len;

			lpDlgInfo = (LPDIALOG_INFO)lParam;
			SetWindowLong(hDlg, DWL_USER, (LONG)lpDlgInfo);

			/* Set title */
			LoadStringA(hDllInstance, IDS_TITLE, str, 254);
			len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
			if((format = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR))))
			{
				MultiByteToWideChar(CP_ACP, 0, str, -1, format, len);
				wnsprintfW(wstr, 254, format, lpDlgInfo->lpszDevice);
				HeapFree(GetProcessHeap(), 0, format);
				SetWindowTextW(hDlg, wstr);
			}

			if(!(hOwner = GetParent(hDlg)))
				hOwner = GetDesktopWindow();

			/* Position dialog in the center of owner window */
			GetWindowRect(hOwner, &rcOwner);
			GetWindowRect(hDlg, &rcDlg);
			CopyRect(&rc, &rcOwner);
			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
			OffsetRect(&rc, -rc.left, -rc.top);
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
			SetWindowPos(hDlg, HWND_TOP,
				rcOwner.left + (rc.right / 2),
				rcOwner.top + (rc.bottom / 2),
				0, 0, SWP_NOSIZE);

			/* Initialize baud rate combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_BAUDRATE)))
				EndDialog(hDlg, 0);

			for(i = 0; Bauds[i].Str; i++)
			{
				SendMessageA(hBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)Bauds[i].Str);
				if(Bauds[i].Baud == lpDlgInfo->lpCC->dcb.BaudRate)
					SendMessageA(hBox, CB_SETCURSEL, (WPARAM)i, 0);
			}

			if(SendMessage(hBox, CB_GETCURSEL, 0, 0)==CB_ERR)
				SendMessageA(hBox, CB_SETCURSEL, DEFAULT_BAUD_INDEX, 0);

			/* Initialize byte size combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_BYTESIZE)))
				EndDialog(hDlg, 0);

			for(i = 0; ByteSizes[i].Str; i++)
			{
				SendMessageA(hBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)ByteSizes[i].Str);
				if(ByteSizes[i].ByteSize == lpDlgInfo->lpCC->dcb.ByteSize)
					SendMessageA(hBox, CB_SETCURSEL, (WPARAM)i, 0);
			}

			if(SendMessage(hBox, CB_GETCURSEL, 0, 0)==CB_ERR)
				SendMessageA(hBox, CB_SETCURSEL, DEFAULT_BYTESIZE_INDEX, 0);

			/* Initialize parity combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_PARITY)))
				EndDialog(hDlg, 0);

			for(i = 0; Paritys[i].StrId; i++)
			{
				if(LoadStringA(hDllInstance, Paritys[i].StrId, str, 254))
				{
					SendMessageA(hBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)str);
					if(Paritys[i].Parity == lpDlgInfo->lpCC->dcb.Parity)
						SendMessageA(hBox, CB_SETCURSEL, (WPARAM)i, 0);
				}
			}

			if(SendMessage(hBox, CB_GETCURSEL, 0, 0)==CB_ERR)
				SendMessageA(hBox, CB_SETCURSEL, DEFAULT_PARITY_INDEX, 0);

			/* Initialize stop bits combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_STOPBITS)))
				EndDialog(hDlg, 0);

			for(i = 0; StopBits[i].StrId; i++)
			{
				if(LoadStringA(hDllInstance,StopBits[i].StrId, str, 254))
				{
					SendMessageA(hBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)str);
					if(StopBits[i].StopBit == lpDlgInfo->lpCC->dcb.StopBits)
						SendMessageA(hBox, CB_SETCURSEL, (WPARAM)i, 0);
				}
			}

			if(SendMessage(hBox, CB_GETCURSEL, 0, 0)==CB_ERR)
				SendMessageA(hBox, CB_SETCURSEL, DEFAULT_STOPBITS_INDEX, 0);

			/* Initialize flow control combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_FLOW)))
				EndDialog(hDlg, 0);

			if(LoadStringA(hDllInstance,IDS_FC_NO, str, 254))
			{
				SendMessageA(hBox, CB_INSERTSTRING, 0, (LPARAM)str);
				SendMessageA(hBox, CB_SETCURSEL, 0, 0);
				lpDlgInfo->InitialFlowIndex = 0;
			}


			if(LoadStringA(hDllInstance,IDS_FC_CTSRTS, str, 254))
			{
				SendMessageA(hBox, CB_INSERTSTRING, 1, (LPARAM)str);
				if(lpDlgInfo->lpCC->dcb.fRtsControl == RTS_CONTROL_HANDSHAKE
					|| lpDlgInfo->lpCC->dcb.fOutxCtsFlow == TRUE)
				{
					SendMessageA(hBox, CB_SETCURSEL, 1, 0);
					lpDlgInfo->InitialFlowIndex = 1;
				}
			}

			if(LoadStringA(hDllInstance,IDS_FC_XONXOFF, str, 254))
			{
				SendMessageA(hBox, CB_INSERTSTRING, 2, (LPARAM)str);
				if(lpDlgInfo->lpCC->dcb.fOutX || lpDlgInfo->lpCC->dcb.fInX)
				{
					SendMessageA(hBox, CB_SETCURSEL, 2, 0);
					lpDlgInfo->InitialFlowIndex = 2;
				}
			}

			/* Set focus */
			SetFocus(GetDlgItem(hDlg, IDC_OKBTN));

			return FALSE;
		} /* WM_INITDIALOG */

		case WM_COMMAND:
		{
			switch(wParam)
			{
				case IDC_CANCELBTN:
					EndDialog(hDlg, FALSE);
					break;
				case IDC_OKBTN:
					OkButton(hDlg);
					EndDialog(hDlg, TRUE);
					break;
			}
			return TRUE;
		} /* WM_COMMAND */

		case WM_CLOSE:
		{
			EndDialog(hDlg, FALSE);
			return TRUE;
		} /* WM_CLOSE */

			default:
			return FALSE;
	}

}


VOID OkButton(HWND hDlg)
{
	LPDIALOG_INFO lpDlgInfo;
	UINT Index;

	lpDlgInfo = (LPDIALOG_INFO) GetWindowLong(hDlg, DWL_USER);

	/* Baud rate */
	Index = SendMessage(GetDlgItem(hDlg, IDC_BAUDRATE), CB_GETCURSEL, 0, 0);
	lpDlgInfo->lpCC->dcb.BaudRate = Bauds[Index].Baud;

	/* Byte size */
	Index = SendMessage(GetDlgItem(hDlg, IDC_BYTESIZE), CB_GETCURSEL, 0, 0);
	lpDlgInfo->lpCC->dcb.ByteSize = ByteSizes[Index].ByteSize;

	/* Parity */
	Index = SendMessage(GetDlgItem(hDlg, IDC_PARITY), CB_GETCURSEL, 0, 0);
	lpDlgInfo->lpCC->dcb.Parity = Paritys[Index].Parity;

	/* Stop bits */
	Index = SendMessage(GetDlgItem(hDlg, IDC_STOPBITS), CB_GETCURSEL, 0, 0);
	lpDlgInfo->lpCC->dcb.StopBits = StopBits[Index].StopBit;
	
	/* Flow Control */
	Index = SendMessage(GetDlgItem(hDlg, IDC_FLOW), CB_GETCURSEL, 0, 0);
	if(lpDlgInfo->InitialFlowIndex != Index)
	{
		switch(Index)
		{
			case 0: /* NO */
				lpDlgInfo->lpCC->dcb.fDtrControl = DTR_CONTROL_DISABLE;
				lpDlgInfo->lpCC->dcb.fRtsControl = RTS_CONTROL_DISABLE;
				lpDlgInfo->lpCC->dcb.fOutxCtsFlow = FALSE;
				lpDlgInfo->lpCC->dcb.fOutxDsrFlow = FALSE;
				lpDlgInfo->lpCC->dcb.fOutX = FALSE;
				lpDlgInfo->lpCC->dcb.fInX = FALSE;
				break;
			case 1: /* CTS/RTS */
				lpDlgInfo->lpCC->dcb.fDtrControl = DTR_CONTROL_DISABLE;
				lpDlgInfo->lpCC->dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
				lpDlgInfo->lpCC->dcb.fOutxCtsFlow = TRUE;
				lpDlgInfo->lpCC->dcb.fOutxDsrFlow = FALSE;
				lpDlgInfo->lpCC->dcb.fOutX = FALSE;
				lpDlgInfo->lpCC->dcb.fInX = FALSE;
				break;
			case 2: /* XON/XOFF */
				lpDlgInfo->lpCC->dcb.fDtrControl = DTR_CONTROL_DISABLE;
				lpDlgInfo->lpCC->dcb.fRtsControl = RTS_CONTROL_DISABLE;
				lpDlgInfo->lpCC->dcb.fOutxCtsFlow = FALSE;
				lpDlgInfo->lpCC->dcb.fOutxDsrFlow = FALSE;
				lpDlgInfo->lpCC->dcb.fOutX = TRUE;
				lpDlgInfo->lpCC->dcb.fInX = TRUE;
				break;
		}
	}
}
