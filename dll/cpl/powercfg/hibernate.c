/* $Id$
 *
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/hibernate.c
 * PURPOSE:         hibernate tab of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 *                  Martin Rottensteiner
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 */

//#ifndef NSTATUS
//typedef long NTSTATUS;
//#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <stdio.h>
#include <tchar.h>

#include "resource.h"
#include "powercfg.h"


BOOLEAN Pos_InitData();
void Adv_InitDialog();


static VOID
Hib_InitDialog(HWND hwndDlg)
{
	SYSTEM_POWER_CAPABILITIES PowerCaps;
	MEMORYSTATUSEX msex;
	TCHAR szSize[MAX_PATH];
	TCHAR szTemp[MAX_PATH];
	ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;

	if (GetPwrCapabilities(&PowerCaps))
	{
		CheckDlgButton(hwndDlg,
			       IDC_HIBERNATEFILE,
			       PowerCaps.HiberFilePresent ? BST_CHECKED : BST_UNCHECKED);

		msex.dwLength = sizeof(msex);
		if (!GlobalMemoryStatusEx(&msex))
		{
			return; //FIXME
		}

		if (GetWindowsDirectory(szTemp,MAX_PATH))
		{
			if (!GetDiskFreeSpaceEx(szTemp,&FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
				TotalNumberOfFreeBytes.QuadPart = 0;
		}
		else
		{
			if (!GetDiskFreeSpaceEx(NULL,&FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
				TotalNumberOfFreeBytes.QuadPart = 0;
		}

		if (TotalNumberOfFreeBytes.QuadPart > 0x100000)
		{
			if (LoadString(hApplet, IDS_SIZEMB, szTemp, MAX_PATH))
			{
				_stprintf(szSize,szTemp,TotalNumberOfFreeBytes.QuadPart / 0x100000);
				SetDlgItemText(hwndDlg, IDC_FREESPACE, szSize);
			}
		}
		else
		{
			if (LoadString(hApplet, IDS_SIZEBYTS, szTemp, MAX_PATH))
			{
				_stprintf(szSize,szTemp,TotalNumberOfFreeBytes.QuadPart);
				SetDlgItemText(hwndDlg, IDC_FREESPACE, szSize);
			}
		}

		if (msex.ullTotalPhys>0x100000)
		{
			if (LoadString(hApplet, IDS_SIZEMB, szTemp, MAX_PATH))
			{
				_stprintf(szSize,szTemp,msex.ullTotalPhys/0x100000);
				SetDlgItemText(hwndDlg, IDC_SPACEFORHIBERNATEFILE,szSize);
			}
		}
		else
		{
			if (LoadString(hApplet, IDS_SIZEBYTS, szTemp, MAX_PATH))
			{
				_stprintf(szSize,szTemp,msex.ullTotalPhys);
				SetDlgItemText(hwndDlg, IDC_SPACEFORHIBERNATEFILE, szSize);
			}
		}

		if (TotalNumberOfFreeBytes.QuadPart < msex.ullTotalPhys && !PowerCaps.HiberFilePresent)
		{
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), FALSE);
			ShowWindow(GetDlgItem(hwndDlg, IDC_TOLESSFREESPACE), TRUE);
		}
		else
		{
			ShowWindow(GetDlgItem(hwndDlg, IDC_TOLESSFREESPACE), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), TRUE);
		}
	}
}

INT_PTR
Hib_SaveData(HWND hwndDlg)
{
	BOOLEAN bHibernate;

	bHibernate = (BOOLEAN)(IsDlgButtonChecked(hwndDlg, IDC_HIBERNATEFILE) == BST_CHECKED);

	if (CallNtPowerInformation(SystemReserveHiberFile,&bHibernate, sizeof(bHibernate), NULL, 0) == STATUS_SUCCESS)
	{
		Pos_InitData();
		Adv_InitDialog();
		Hib_InitDialog(hwndDlg);
		return TRUE;
	}

	return FALSE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
HibernateDlgProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
		Hib_InitDialog(hwndDlg);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_HIBERNATEFILE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;
			if (lpnm->code == (UINT)PSN_APPLY)
			{
				return Hib_SaveData(hwndDlg);
			}
		}
  }
  return FALSE;
}
