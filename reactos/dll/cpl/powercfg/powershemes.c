/* $Id$
 *
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/powershemes.c
 * PURPOSE:         powerschemes tab of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 *                  Martin Rottensteiner
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include "resource.h"
#include "powercfg.h"
#include "stdio.h"

UINT Sec[]=
{
	60,
	120,
	180,
	300,
	600,
	900,
	1200,
	1500,
	1800,
	2700,
	3600,
	7200,
	10800,
	14400,
	18000,
	0
};

HWND hList = 0;
HWND hPos = 0;

unsigned aps = 0;

#define MAX_POWER_POLICY	20

POWER_POLICY gPP[MAX_POWER_POLICY];
UINT guiIndex = 0;
HWND hwndDialog;

void LoadConfig(HWND hwndDlg);
void Pos_InitPage(HWND hwndDlg);
BOOLEAN Pos_InitData();
void Pos_SaveData(HWND hwndDlg);


BOOLEAN CreateEnergyList(HWND hwnd);

static
BOOLEAN DelScheme(HWND hwnd)
{
	INT iCurSel;
	HWND hList;
	TCHAR szBuf[1024], szBufT[1024];
	UINT DelScheme;
			
	hList = GetDlgItem(hwnd, IDC_ENERGYLIST);
	
	iCurSel = SendMessage(hList, CB_GETCURSEL, 0, 0);
	if (iCurSel == CB_ERR) return FALSE;

	SendMessage(hList, CB_SETCURSEL, iCurSel, 0);
				
	DelScheme = SendMessage(hList, CB_GETITEMDATA, (WPARAM)iCurSel, 0);
	if (DelScheme == (UINT)CB_ERR) return FALSE;

	LoadString(hApplet, IDS_DEL_SCHEME_TITLE, szBufT, sizeof(szBufT) / sizeof(TCHAR));
	LoadString(hApplet, IDS_DEL_SCHEME, szBuf, sizeof(szBuf) / sizeof(TCHAR));
			
	if (MessageBox(hwnd, (LPCTSTR)szBuf, (LPCTSTR)szBufT, MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
	{
		UINT Current;
		
		if (GetActivePwrScheme(&Current))
		{
			SendMessage(hList, CB_SETCURSEL, (WPARAM)0, 0);
			SendMessage(hList, CB_DELETESTRING, (WPARAM)iCurSel, 0);
			if (Current == DelScheme) Pos_SaveData(hwnd);
		}
		
		if (DeletePwrScheme(DelScheme) != 0) return TRUE;
	}
	
	return FALSE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
powershemesProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
		hPos = hwndDlg;
		hwndDialog = hwndDlg;
	    if (!Pos_InitData())
		{
			//TODO
			// initialization failed
			// handle error
			MessageBox(hwndDlg,_T("Pos_InitData failed\n"), NULL, MB_OK);

		}
		if (!CreateEnergyList(GetDlgItem(hwndDlg, IDC_ENERGYLIST)))
		{
			//TODO
			// initialization failed
			// handle error
			MessageBox(hwndDlg,_T("CreateEnergyList failed\n"), NULL, MB_OK);
		}
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_ENERGYLIST:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				LoadConfig(hwndDlg);
				PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
			break;
		case IDC_DELETE_BTN:
		{
			DelScheme(hwndDlg);
		}
		break;
		case IDC_SAVEAS_BTN:
		{
		
		}
		break;
		case IDC_MONITORACLIST:
		case IDC_MONITORDCLIST:
		case IDC_DISKACLIST:
		case IDC_DISKDCLIST:
		case IDC_STANDBYACLIST:
		case IDC_STANDBYDCLIST:
		case IDC_HYBERNATEACLIST:
		case IDC_HYBERNATEDCLIST:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
			break;
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;
			if (lpnm->code == (UINT)PSN_APPLY)
			{
				Pos_SaveData(hwndDlg);
			}
			return TRUE;
		}
		break;
  }
  return FALSE;
}

BOOLEAN Pos_InitData()
{
	SYSTEM_POWER_CAPABILITIES spc;
/*
	RECT rectCtl, rectDlg, rectCtl2;
	LONG movetop = 0;
	LONG moveright = 0;

	if (GetWindowRect(hPos,&rectDlg))
		{
			if (GetWindowRect(GetDlgItem(hPos, IDC_SAT),&rectCtl2))
			{
				if (GetWindowRect(GetDlgItem(hPos, IDC_MONITOR),&rectCtl))
				{
					movetop=rectCtl.top - rectCtl2.top;
					MoveWindow(GetDlgItem(hPos, IDC_MONITOR),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
					if (GetWindowRect(GetDlgItem(hPos, IDC_DISK),&rectCtl))
					{
						MoveWindow(GetDlgItem(hPos, IDC_DISK),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
					}
					if (GetWindowRect(GetDlgItem(hPos, IDC_STANDBY),&rectCtl))
					{
						MoveWindow(GetDlgItem(hPos, IDC_STANDBY),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
					}
					if (GetWindowRect(GetDlgItem(hPos, IDC_HYBERNATE),&rectCtl))
					{
						MoveWindow(GetDlgItem(hPos, IDC_HYBERNATE),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
					}
					if (GetWindowRect(GetDlgItem(hPos, IDC_MONITORDCLIST),&rectCtl2))
					{
						movetop=movetop-8;
						if (GetWindowRect(GetDlgItem(hPos, IDC_MONITORACLIST),&rectCtl))
						{
							moveright=rectCtl.right - rectCtl2.right;
							MoveWindow(GetDlgItem(hPos, IDC_MONITORACLIST),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left-moveright,rectCtl.bottom-rectCtl.top,FALSE);
							if (GetWindowRect(GetDlgItem(hPos, IDC_DISKACLIST),&rectCtl))
							{
								MoveWindow(GetDlgItem(hPos, IDC_DISKACLIST),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left-moveright,rectCtl.bottom-rectCtl.top,FALSE);
							}
							if (GetWindowRect(GetDlgItem(hPos, IDC_STANDBYACLIST),&rectCtl))
							{
								MoveWindow(GetDlgItem(hPos, IDC_STANDBYACLIST),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left-moveright,rectCtl.bottom-rectCtl.top,FALSE);
							}
							if (GetWindowRect(GetDlgItem(hPos, IDC_HYBERNATEACLIST),&rectCtl))
							{
								MoveWindow(GetDlgItem(hPos, IDC_HYBERNATEACLIST),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left-moveright,rectCtl.bottom-rectCtl.top,FALSE);
							}
						}
						if (GetWindowRect(GetDlgItem(hPos, IDC_GRPDETAIL),&rectCtl))
						{
							MoveWindow(GetDlgItem(hPos, IDC_GRPDETAIL),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
						}
					}
				}
			}
		}
	}
*/

	if (!GetPwrCapabilities(&spc))
	{
		return FALSE;
	}

	if (!spc.SystemBatteriesPresent)
	{
		ShowWindow(GetDlgItem(hPos, IDC_SAT),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_IAC),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_SAC),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_IDC),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_SDC),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_MONITORDCLIST),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_DISKDCLIST),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_STANDBYDCLIST),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_HYBERNATEDCLIST),FALSE);
		ShowWindow(GetDlgItem(hPos, IDC_HYBERNATEACLIST), spc.HiberFilePresent);

	}
	else
	{
		ShowWindow(GetDlgItem(hPos, IDC_HYBERNATEDCLIST), spc.HiberFilePresent);
		ShowWindow(GetDlgItem(hPos, IDC_HYBERNATEACLIST), FALSE);
	}


	if (!(spc.SystemS1 ||spc.SystemS2 || spc.SystemS3))
	{
		ShowWindow(GetDlgItem(hPos, IDC_STANDBYACLIST), TRUE);
		ShowWindow(GetDlgItem(hPos, IDC_STANDBYDCLIST), TRUE);
		ShowWindow(GetDlgItem(hPos, IDC_STANDBY), TRUE);
	}

	ShowWindow(GetDlgItem(hPos, IDC_HYBERNATE), spc.HiberFilePresent);

	return TRUE;
}

BOOLEAN CALLBACK callback_EnumPwrScheme(UINT uiIndex, DWORD dwName, LPTSTR sName, DWORD dwDesc,
                                             LPWSTR sDesc, PPOWER_POLICY pp,LPARAM lParam )
{
	int index;

	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(sDesc);
	UNREFERENCED_PARAMETER(dwDesc);
	UNREFERENCED_PARAMETER(dwName);

	if (ValidatePowerPolicies(0,pp))
	{
		if (guiIndex >= MAX_POWER_POLICY)
		{
			//FIXME
			//implement store power policy dynamically
			return FALSE;
		}


		memcpy(&gPP[guiIndex], pp, sizeof(POWER_POLICY));
		guiIndex++;

		index = (int) SendMessage(hList,
			   CB_ADDSTRING,
			   0,
			   (LPARAM)sName);
		if (index == CB_ERR)
			return FALSE;

		SendMessage(hList,
			   CB_SETITEMDATA,
			   index,
			   (LPARAM)uiIndex);

		if (aps == uiIndex)
		{
			SendMessage(hList,
			   CB_SELECTSTRING,
			   TRUE,
			   (LPARAM)sName);
			LoadConfig(GetParent(hList));
		}
	}
	return TRUE;
}

BOOLEAN CreateEnergyList(HWND hwnd)
{
	BOOLEAN retval;
	POWER_POLICY pp;
	SYSTEM_POWER_CAPABILITIES spc;

	hList = hwnd;

	if (!GetActivePwrScheme(&aps))
		return FALSE;

	if (!ReadGlobalPwrPolicy(&gGPP))
		return FALSE;

	if (!ReadPwrScheme(aps,&pp))
		return FALSE;

	if (!ValidatePowerPolicies(&gGPP,0))
		return FALSE;

/*
	if (!SetActivePwrScheme(aps,&gGPP,&pp))
		return FALSE;
*/

	if (!GetPwrCapabilities(&spc))
		return FALSE;

	if (CanUserWritePwrScheme())
	{
		//TODO
		// enable write / delete powerscheme button
	}

	Pos_InitPage(GetParent(hwnd));

	if (!GetActivePwrScheme(&aps))
		return FALSE;

	retval = EnumPwrSchemes(callback_EnumPwrScheme, aps);
	
    if(SendMessage(hwnd, CB_GETCOUNT, 0, 0) > 0)
    {
        EnableWindow(GetDlgItem(hwndDialog, IDC_DELETE_BTN),TRUE);
		EnableWindow(GetDlgItem(hwndDialog, IDC_SAVEAS_BTN),TRUE);
    }

	return retval;
}

void Pos_InitPage(HWND hwndDlg)
{
	int ifrom=0,i=0,imin=0;
	HWND hwnd = NULL;
	TCHAR szName[MAX_PATH];
	LRESULT index;

	for(i=1;i<9;i++)
	{
		switch(i)
		{
		case 1:
			hwnd=GetDlgItem(hwndDlg, IDC_MONITORACLIST);
			imin=IDS_TIMEOUT1;
			break;
		case 2:
			hwnd=GetDlgItem(hwndDlg, IDC_STANDBYACLIST);
			imin=IDS_TIMEOUT1;
			break;
		case 3:
			hwnd=GetDlgItem(hwndDlg, IDC_DISKACLIST);
			imin=IDS_TIMEOUT3;
			break;
		case 4:
			hwnd=GetDlgItem(hwndDlg, IDC_HYBERNATEACLIST);
			imin=IDS_TIMEOUT3;
			break;
		case 5:
			hwnd=GetDlgItem(hwndDlg, IDC_MONITORDCLIST);
			imin=IDS_TIMEOUT1;
			break;
		case 6:
			hwnd=GetDlgItem(hwndDlg, IDC_STANDBYDCLIST);
			imin=IDS_TIMEOUT1;
			break;
		case 7:
			hwnd=GetDlgItem(hwndDlg, IDC_DISKDCLIST);
			imin=IDS_TIMEOUT3;
			break;
		case 8:
			hwnd=GetDlgItem(hwndDlg, IDC_HYBERNATEDCLIST);
			imin=IDS_TIMEOUT3;
			break;
		default:
			return;
		}
		for (ifrom=imin;ifrom<(IDS_TIMEOUT15+1);ifrom++)
		{
			if (LoadString(hApplet, ifrom, szName, MAX_PATH))
			{
				index = SendMessage(hwnd,
									 CB_ADDSTRING,
									 0,
									(LPARAM)szName);

				if (index == CB_ERR)
					return;

				SendMessage(hwnd,
							 CB_SETITEMDATA,
							 index,
							 (LPARAM)Sec[ifrom-IDS_TIMEOUT16]);
			}
		}
		if (LoadString(hApplet, IDS_TIMEOUT16, szName, MAX_PATH))
		{
			index = SendMessage(hwnd,
								 CB_ADDSTRING,
								 0,
								 (LPARAM)szName);
			if (index == CB_ERR)
				return;

			SendMessage(hwnd,
					     CB_SETITEMDATA,
						 index,
						 (LPARAM)Sec[0]);
		}
	}
}

void LoadConfig(HWND hwndDlg)
{
	INT i=0, iCurSel=0;
	UINT uiIndex;
	TCHAR szProfile[MAX_PATH];
	TCHAR szTemp[MAX_PATH];
	TCHAR szConfig[MAX_PATH];
	POWER_POLICY pp;

	iCurSel = (INT)SendDlgItemMessage(hwndDlg, IDC_ENERGYLIST,
		CB_GETCURSEL,
		0,
		0);
	if (iCurSel == CB_ERR)
		return;

	memcpy(&pp, &gPP[iCurSel], sizeof(POWER_POLICY));

	uiIndex = (UINT)SendDlgItemMessage(hwndDlg, IDC_ENERGYLIST, CB_GETCURSEL, 0, 0);
    if(uiIndex != CB_ERR)
	{
		SendDlgItemMessage(hwndDlg, IDC_ENERGYLIST, CB_GETLBTEXT, uiIndex, (LPARAM)szProfile);
		if(LoadString(hApplet, IDS_CONFIG1, szTemp, MAX_PATH))
		{
			_stprintf(szConfig,szTemp,szProfile);
			SetWindowText(GetDlgItem(hwndDlg, IDC_GRPDETAIL),szConfig);
		}
	}

	for(i=0;i<17;i++)
	{
		if (Sec[i]==pp.user.VideoTimeoutAc)
		{
			SendDlgItemMessage(hwndDlg, IDC_MONITORACLIST,
					    CB_SETCURSEL,
						i,
						(LPARAM)0);
		}

		if (Sec[i]==pp.user.VideoTimeoutDc)
		{
			SendDlgItemMessage(hwndDlg, IDC_MONITORDCLIST,
					    CB_SETCURSEL,
						 i,
						 (LPARAM)0);
		}
		if (Sec[i]==pp.user.SpindownTimeoutAc)
		{
			SendDlgItemMessage(hwndDlg, IDC_DISKACLIST,
					   CB_SETCURSEL,
					   i-2,
					   (LPARAM)0);
		}
		if (Sec[i]==pp.user.SpindownTimeoutDc)//IdleTimeoutDc)
		{
			SendDlgItemMessage(hwndDlg, IDC_DISKDCLIST,
					   CB_SETCURSEL,
					   i-2,
					   (LPARAM)0);
		}
		if (Sec[i]==pp.user.IdleTimeoutAc)
		{
			SendDlgItemMessage(hwndDlg, IDC_STANDBYACLIST,
					   CB_SETCURSEL,
					   i,
					   (LPARAM)0);
		}
		if (Sec[i]==pp.user.IdleTimeoutDc)
		{
			SendDlgItemMessage(hwndDlg, IDC_STANDBYDCLIST,
					   CB_SETCURSEL,
					   i,
					   (LPARAM)0);
		}

		if (Sec[i]==pp.mach.DozeS4TimeoutAc)
		{
			SendDlgItemMessage(hwndDlg, IDC_HYBERNATEACLIST,
					   CB_SETCURSEL,
					   i,
					(LPARAM)0);
		}
		if (Sec[i]==pp.mach.DozeS4TimeoutDc)
		{
			SendDlgItemMessage(hwndDlg, IDC_HYBERNATEDCLIST,
					   CB_SETCURSEL,
					   i,
					   (LPARAM)0);
		}
	}

}

void Pos_SaveData(HWND hwndDlg)
{
	INT iCurSel=0,tmp=0;

	iCurSel = (INT) SendDlgItemMessage(hwndDlg, IDC_ENERGYLIST,
		CB_GETCURSEL,
		0,
		0);
	if (iCurSel == CB_ERR)
		return;

    tmp = (INT) SendDlgItemMessage(hwndDlg, IDC_MONITORDCLIST,
				   CB_GETCURSEL,
				   0,
				   (LPARAM)0);
	if (tmp > 0 && tmp < 16)
	{
		gPP[iCurSel].user.VideoTimeoutAc = Sec[tmp];
	}
    tmp = (INT) SendDlgItemMessage(hwndDlg, IDC_MONITORDCLIST,
				   CB_GETCURSEL,
				   0,
				   (LPARAM)0);
	if (tmp > 0 && tmp < 16)
	{
		gPP[iCurSel].user.VideoTimeoutDc = Sec[tmp];
	}
    tmp = (INT) SendDlgItemMessage(hwndDlg, IDC_DISKACLIST,
				   CB_GETCURSEL,
				   0,
				   (LPARAM)0);
	if (tmp > 0 && tmp < 16)
	{
		gPP[iCurSel].user.SpindownTimeoutAc = Sec[tmp+2];
	}
    tmp = (INT) SendDlgItemMessage(hwndDlg, IDC_DISKDCLIST,
				   CB_GETCURSEL,
				   0,
				   (LPARAM)0);
	if (tmp > 0 && tmp < 16)
	{
		gPP[iCurSel].user.SpindownTimeoutDc = Sec[tmp+2];
	}
    tmp = (INT) SendDlgItemMessage(hwndDlg, IDC_STANDBYACLIST,
				   CB_GETCURSEL,
				   0,
				   (LPARAM)0);
	if (tmp > 0 && tmp < 16)
	{
		gPP[iCurSel].user.IdleTimeoutAc = Sec[tmp];
	}
    tmp = (INT) SendDlgItemMessage(hwndDlg, IDC_STANDBYDCLIST,
				   CB_GETCURSEL,
				   0,
				   (LPARAM)0);
	if (tmp > 0 && tmp < 16)
	{
		gPP[iCurSel].user.IdleTimeoutDc = Sec[tmp];
	}
    tmp = (INT) SendDlgItemMessage(hwndDlg, IDC_HYBERNATEACLIST,
				   CB_GETCURSEL,
				   0,
				   (LPARAM)0);
	if (tmp > 0 && tmp < 16)
	{
		gPP[iCurSel].mach.DozeS4TimeoutAc = Sec[tmp];
	}
    tmp = (INT) SendDlgItemMessage(hwndDlg, IDC_HYBERNATEDCLIST,
				   CB_GETCURSEL,
				   0,
				   (LPARAM)0);
	if (tmp > 0 && tmp < 16)
	{
		gPP[iCurSel].mach.DozeS4TimeoutDc = Sec[tmp];
	}

    SetActivePwrScheme(iCurSel,NULL,&gPP[iCurSel]);
	LoadConfig(hwndDlg);
}
