
/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/startrec.c
 * PURPOSE:     Computer settings for startup and recovery
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include "precomp.h"

static TCHAR m_szFreeldrIni[MAX_PATH + 15];

void SetTimeout(HWND hwndDlg, int Timeout)
{
	if (Timeout == 0)
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECLISTUPDWN), FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECLISTUPDWN), TRUE);
	}
	SendDlgItemMessage(hwndDlg, IDC_STRRECLISTUPDWN, UDM_SETPOS, (WPARAM) 0, (LPARAM) MAKELONG((short) Timeout, 0));
}

/* Property page dialog callback */
INT_PTR CALLBACK
StartRecDlgProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
	TCHAR *szSystemDrive;
	TCHAR szDefaultOS[MAX_PATH];
	TCHAR szDefaultOSName[MAX_PATH];
	TCHAR szTimeout[10];
	int iTimeout;

	UNREFERENCED_PARAMETER(lParam);
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			DWORD dwBufSize;

			/* get Path to freeldr.ini or boot.ini */
			szSystemDrive = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(TCHAR));
			if (szSystemDrive != NULL)
			{
				dwBufSize = GetEnvironmentVariable(_T("SystemDrive"), szSystemDrive, MAX_PATH);
				if (dwBufSize > MAX_PATH)
				{
					TCHAR *szTmp;
					DWORD dwBufSize2;

					szTmp = HeapReAlloc(GetProcessHeap(), 0, szSystemDrive, dwBufSize * sizeof(TCHAR));
					if (szTmp == NULL)
						goto FailGetSysDrive;

					szSystemDrive = szTmp;

					dwBufSize2 = GetEnvironmentVariable(_T("SystemDrive"), szSystemDrive, dwBufSize);
					if (dwBufSize2 > dwBufSize || dwBufSize2 == 0)
						goto FailGetSysDrive;
				}
				else if (dwBufSize == 0)
				{
FailGetSysDrive:
					HeapFree(GetProcessHeap(), 0, szSystemDrive);
					szSystemDrive = NULL;
				}

				if (szSystemDrive != NULL)
				{
					if (m_szFreeldrIni != NULL)
					{
						_tcscpy(m_szFreeldrIni, szSystemDrive);
						_tcscat(m_szFreeldrIni, _T("\\freeldr.ini"));
						if (!PathFileExists(m_szFreeldrIni))
						{
							_tcscpy(m_szFreeldrIni, szSystemDrive);
							_tcscat(m_szFreeldrIni, _T("\\boot.ini"));
						}
					}
					HeapFree(GetProcessHeap(), 0, szSystemDrive);
				}
			}
   
			SetDlgItemText(hwndDlg, IDC_STRRECDUMPFILE, _T("%SystemRoot%\\MiniDump"));

			/* load settings from freeldr.ini */
			GetPrivateProfileString(_T("boot loader"), _T("default"), NULL, szDefaultOS, MAX_PATH, m_szFreeldrIni);
			GetPrivateProfileString(_T("operating systems"), szDefaultOS, NULL, szDefaultOSName, MAX_PATH, m_szFreeldrIni);
			SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM)szDefaultOSName);
			SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

			/* timeout */
			iTimeout = GetPrivateProfileInt(_T("boot loader"), _T("timeout"), 0, m_szFreeldrIni);
			SetTimeout(hwndDlg, iTimeout);
			if (iTimeout != 0)
				SendDlgItemMessage(hwndDlg, IDC_STRECLIST, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_STRRECEDIT:
				{
					ShellExecute(0, _T("open"), _T("notepad"), m_szFreeldrIni, NULL, SW_SHOWNORMAL);
					break;
				}	
				case IDOK:
				{
					/* save timeout */
					if (SendDlgItemMessage(hwndDlg, IDC_STRECLIST, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
						iTimeout = SendDlgItemMessage(hwndDlg, IDC_STRRECLISTUPDWN, UDM_GETPOS, (WPARAM)0, (LPARAM)0);
					else
						iTimeout = 0;
					_stprintf(szTimeout, _T("%i"), iTimeout);
					WritePrivateProfileString(_T("boot loader"), _T("timeout"), szTimeout, m_szFreeldrIni);
				}
				case IDCANCEL:
				{
					EndDialog(hwndDlg,
							  LOWORD(wParam));
					return TRUE;
					break;
				}
				case IDC_STRECLIST:
				{
					if (SendDlgItemMessage(hwndDlg, IDC_STRECLIST, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
						SetTimeout(hwndDlg, 30);
					else
						SetTimeout(hwndDlg, 0);
				}
			}
		}
		break;
  }
  return FALSE;
}
