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

const DWORD Bauds[] = {
	CBR_110,
	CBR_300,
	CBR_600,
	CBR_1200,
	CBR_2400,
	CBR_4800,
	CBR_9600,
	CBR_14400,
	CBR_19200,
	CBR_38400,
	CBR_56000,
	CBR_57600,
	CBR_115200,
	CBR_128000,
	CBR_256000,
	0
};

const BYTE ByteSizes[] = {
	5,
	6,
	7,
	8,
	0
};


const PARITY_INFO Parities[] = {
	{ EVENPARITY, IDS_EVENPARITY },
	{ MARKPARITY, IDS_MARKPARITY },
	{ NOPARITY, IDS_NOPARITY },
	{ ODDPARITY, IDS_ODDPARITY },
	{ SPACEPARITY, IDS_SPACEPARITY },
	{ 0, 0 }
};

const STOPBIT_INFO StopBits[] = {
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
DWORD WINAPI drvCommConfigDialogW(LPCWSTR lpszDevice,
	HWND hWnd,
	LPCOMMCONFIG lpCommConfig)
{
	DIALOG_INFO DialogInfo;

	if(!lpszDevice || !lpCommConfig)
	{
		return ERROR_INVALID_PARAMETER;
	}

	DialogInfo.lpszDevice = lpszDevice;
	DialogInfo.lpCC = lpCommConfig;

	return DialogBoxParamW(hDllInstance, MAKEINTRESOURCEW(IDD_COMMDLG),
					hWnd, (DLGPROC)CommDlgProc, (LPARAM)&DialogInfo);
}

/*
 * @implemented
 */
DWORD WINAPI drvCommConfigDialogA(LPCSTR lpszDevice,
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
		return ERROR_NOT_ENOUGH_MEMORY;
}

/*
 * @unimplemented
 */
DWORD WINAPI drvSetDefaultCommConfigW(LPCWSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	DWORD dwSize)
{
	UNIMPLEMENTED
}

/*
 * @unimplemented
 */
DWORD WINAPI drvSetDefaultCommConfigA(LPCSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	DWORD dwSize)
{
	UNIMPLEMENTED
}

/*
 * @unimplemented
 */
DWORD WINAPI drvGetDefaultCommConfigW(LPCWSTR lpszDevice,
	LPCOMMCONFIG lpCommConfig,
	LPDWORD lpdwSize)
{
	UNIMPLEMENTED
}

/*
 * @unimplemented
 */
DWORD WINAPI drvGetDefaultCommConfigA(LPCSTR lpszDevice,
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
			WCHAR wstr[255];
			RECT rc, rcDlg, rcOwner;
			HWND hOwner;
			INT i;

			lpDlgInfo = (LPDIALOG_INFO)lParam;
			SetWindowLongPtrW(hDlg, DWL_USER, (LONG_PTR)lpDlgInfo);

			/* Set title */
			if(LoadStringW(hDllInstance, IDS_TITLE, wstr, sizeof(wstr) / sizeof(wstr[0])))
			{
    				SetWindowTextW(hDlg, wstr);
			}

                        /* FIXME - this won't work correctly systems with multiple monitors! */
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
				EndDialog(hDlg, ERROR_CANCELLED);

			for(i = 0; Bauds[i]; i++)
			{
                                wsprintf(wstr, L"%d", Bauds[i]);
                                SendMessageW(hBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)wstr);
				if(Bauds[i] == lpDlgInfo->lpCC->dcb.BaudRate)
					SendMessageW(hBox, CB_SETCURSEL, (WPARAM)i, 0);
			}

			if(SendMessageW(hBox, CB_GETCURSEL, 0, 0) == CB_ERR)
				SendMessageW(hBox, CB_SETCURSEL, DEFAULT_BAUD_INDEX, 0);

			/* Initialize byte size combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_BYTESIZE)))
				EndDialog(hDlg, ERROR_CANCELLED);

			for(i = 0; ByteSizes[i]; i++)
			{
                                wsprintf(wstr, L"%d", Bauds[i]);
                                SendMessageW(hBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)wstr);
				if(ByteSizes[i] == lpDlgInfo->lpCC->dcb.ByteSize)
					SendMessageW(hBox, CB_SETCURSEL, (WPARAM)i, 0);
			}

			if(SendMessageW(hBox, CB_GETCURSEL, 0, 0) == CB_ERR)
				SendMessageW(hBox, CB_SETCURSEL, DEFAULT_BYTESIZE_INDEX, 0);

			/* Initialize parity combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_PARITY)))
				EndDialog(hDlg, ERROR_CANCELLED);

			for(i = 0; Parities[i].StrId; i++)
			{
				if(LoadStringW(hDllInstance, Parities[i].StrId, wstr, sizeof(wstr) / sizeof(wstr[0])))
				{
					SendMessageW(hBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)wstr);
					if(Parities[i].Parity == lpDlgInfo->lpCC->dcb.Parity)
						SendMessageW(hBox, CB_SETCURSEL, (WPARAM)i, 0);
				}
			}

			if(SendMessageW(hBox, CB_GETCURSEL, 0, 0)==CB_ERR)
				SendMessageW(hBox, CB_SETCURSEL, DEFAULT_PARITY_INDEX, 0);

			/* Initialize stop bits combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_STOPBITS)))
				EndDialog(hDlg, ERROR_CANCELLED);

			for(i = 0; StopBits[i].StrId; i++)
			{
				if(LoadStringW(hDllInstance, StopBits[i].StrId, wstr, sizeof(wstr) / sizeof(wstr[0])))
				{
					SendMessageW(hBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)wstr);
					if(StopBits[i].StopBit == lpDlgInfo->lpCC->dcb.StopBits)
						SendMessageW(hBox, CB_SETCURSEL, (WPARAM)i, 0);
				}
			}

			if(SendMessageW(hBox, CB_GETCURSEL, 0, 0)==CB_ERR)
				SendMessageW(hBox, CB_SETCURSEL, DEFAULT_STOPBITS_INDEX, 0);

			/* Initialize flow control combo */
			if(!(hBox = GetDlgItem(hDlg, IDC_FLOW)))
				EndDialog(hDlg, ERROR_CANCELLED);

			if(LoadStringW(hDllInstance, IDS_FC_NO, wstr, sizeof(wstr) / sizeof(wstr[0])))
			{
				SendMessageW(hBox, CB_INSERTSTRING, 0, (LPARAM)wstr);
				SendMessageW(hBox, CB_SETCURSEL, 0, 0);
				lpDlgInfo->InitialFlowIndex = 0;
			}


			if(LoadStringW(hDllInstance, IDS_FC_CTSRTS, wstr, sizeof(wstr) / sizeof(wstr[0])))
			{
				SendMessageW(hBox, CB_INSERTSTRING, 1, (LPARAM)wstr);
				if(lpDlgInfo->lpCC->dcb.fRtsControl == RTS_CONTROL_HANDSHAKE
					|| lpDlgInfo->lpCC->dcb.fOutxCtsFlow == TRUE)
				{
					SendMessageW(hBox, CB_SETCURSEL, 1, 0);
					lpDlgInfo->InitialFlowIndex = 1;
				}
			}

			if(LoadStringW(hDllInstance, IDS_FC_XONXOFF, wstr, sizeof(wstr) / sizeof(wstr[0])))
			{
				SendMessageW(hBox, CB_INSERTSTRING, 2, (LPARAM)wstr);
				if(lpDlgInfo->lpCC->dcb.fOutX || lpDlgInfo->lpCC->dcb.fInX)
				{
					SendMessageW(hBox, CB_SETCURSEL, 2, 0);
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
					EndDialog(hDlg, ERROR_CANCELLED);
					break;
				case IDC_OKBTN:
					OkButton(hDlg);
					EndDialog(hDlg, ERROR_SUCCESS);
					break;
			}
			return TRUE;
		} /* WM_COMMAND */

		case WM_CLOSE:
		{
			EndDialog(hDlg, ERROR_CANCELLED);
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

	lpDlgInfo = (LPDIALOG_INFO) GetWindowLongPtrW(hDlg, DWL_USER);

	/* Baud rate */
	Index = SendMessageW(GetDlgItem(hDlg, IDC_BAUDRATE), CB_GETCURSEL, 0, 0);
	lpDlgInfo->lpCC->dcb.BaudRate = Bauds[Index];

	/* Byte size */
	Index = SendMessageW(GetDlgItem(hDlg, IDC_BYTESIZE), CB_GETCURSEL, 0, 0);
	lpDlgInfo->lpCC->dcb.ByteSize = ByteSizes[Index];

	/* Parity */
	Index = SendMessageW(GetDlgItem(hDlg, IDC_PARITY), CB_GETCURSEL, 0, 0);
	lpDlgInfo->lpCC->dcb.Parity = Parities[Index].Parity;

	/* Stop bits */
	Index = SendMessageW(GetDlgItem(hDlg, IDC_STOPBITS), CB_GETCURSEL, 0, 0);
	lpDlgInfo->lpCC->dcb.StopBits = StopBits[Index].StopBit;

	/* Flow Control */
	Index = SendMessageW(GetDlgItem(hDlg, IDC_FLOW), CB_GETCURSEL, 0, 0);
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
