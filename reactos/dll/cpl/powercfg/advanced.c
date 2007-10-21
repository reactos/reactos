/* $Id$
 *
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/advanced.c
 * PURPOSE:         advanced tab of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 *                  Martin Rottensteiner
 */

//#ifndef NSTATUS
//typedef long NTSTATUS;
//#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include "resource.h"
#include "powercfg.h"

HWND hAdv=0;

void Adv_InitDialog();
void Adv_SaveData(HWND hwndDlg);

static POWER_ACTION g_SystemBatteries[3];
static POWER_ACTION g_PowerButton[5];
static POWER_ACTION g_SleepButton[5];

/* Property page dialog callback */
INT_PTR CALLBACK
advancedProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
		hAdv = hwndDlg;
		Adv_InitDialog();
		return TRUE;
      break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_SYSTRAYBATTERYMETER:
		case IDC_PASSWORDLOGON:
		case IDC_VIDEODIMDISPLAY:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
			break;
		case IDC_LIDCLOSE:
		case IDC_POWERBUTTON:
		case IDC_SLEEPBUTTON:
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
				Adv_SaveData(hwndDlg);
			}
			return TRUE;
		}
  }
  return FALSE;
}

static void AddItem(HWND hDlgCtrl, int ResourceId, LPARAM lParam, POWER_ACTION * lpAction)
{
  TCHAR szBuffer[MAX_PATH];
  LRESULT Index;
  if (LoadString(hApplet, ResourceId, szBuffer, MAX_PATH) < MAX_PATH)
  {
	Index = SendMessage(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)szBuffer);
    if (Index != CB_ERR)
	{
		SendMessage(hDlgCtrl, CB_SETITEMDATA, (WPARAM)Index, lParam);
		lpAction[Index] = (POWER_ACTION)lParam;
	}
  }
}

static int FindActionIndex(POWER_ACTION * lpAction, DWORD dwActionSize, POWER_ACTION poAction)
{
  int Index;

  for (Index = 0; Index < (int) dwActionSize; Index++)
  {
	if (lpAction[Index] == poAction)
	    return Index;
  }

  return -1;
}

static BOOLEAN IsBatteryUsed()
{
	SYSTEM_BATTERY_STATE sbs;

	if (CallNtPowerInformation(SystemBatteryState,NULL, (ULONG)0, &sbs, sizeof(SYSTEM_BATTERY_STATE)) == STATUS_SUCCESS)
	{
		if (sbs.BatteryPresent)
		{
			if (sbs.AcOnLine)
			{
				return FALSE;
			}
			return TRUE;
		}
	}

	return FALSE;
}

POWER_ACTION GetPowerActionFromPolicy(POWER_ACTION_POLICY * Policy)
{
	POWER_ACTION poAction = PowerActionNone;
	/*

	TCHAR szBuffer[MAX_PATH];

	// Note: Windows XP SP2+ does not return the PowerAction code
	// for PowerActionWarmEject + PowerActionShutdown but sets it
	// to PowerActionNone and sets the Flags & EventCode


	 _stprintf(szBuffer, L"Action: %x EventCode %x Flags %x",Policy->Action, Policy->EventCode, Policy->Flags);
	 MessageBoxW(NULL, szBuffer, NULL, MB_OK);

	*/

	if (Policy->Action == PowerActionNone)
	{
		if (Policy->Flags == (POWER_ACTION_UI_ALLOWED | POWER_ACTION_QUERY_ALLOWED))
		{
			if (Policy->EventCode  == POWER_FORCE_TRIGGER_RESET)
			{
				poAction = PowerActionNone;
			}
			else if (Policy->EventCode  == POWER_USER_NOTIFY_BUTTON)
			{
				poAction = PowerActionWarmEject;
			}
			else if (Policy->EventCode == POWER_USER_NOTIFY_SHUTDOWN)
			{
				poAction = PowerActionShutdown;
			}
		}
	}
	else
	{
		poAction = Policy->Action;
	}

	return poAction;
}

void ShowCurrentPowerActionPolicy(HWND hDlgCtrl,
									POWER_ACTION * lpAction,
									DWORD dwActionSize,
									POWER_ACTION_POLICY * Policy)
{
	int poActionIndex;
	POWER_ACTION poAction;

	poAction = GetPowerActionFromPolicy(Policy);
	poActionIndex = FindActionIndex(lpAction, dwActionSize, poAction);

	if (poActionIndex < 0)
	{
		return;
	}

	SendMessage(hDlgCtrl, CB_SETCURSEL, (WPARAM)poActionIndex, (LPARAM)0);
}

BOOLEAN SaveCurrentPowerActionPolicy(IN HWND hDlgCtrl,
				 OUT POWER_ACTION_POLICY * Policy)
{
	LRESULT Index;
	LRESULT ItemData;

	Index = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
	if (Index == CB_ERR)
		return FALSE;

	ItemData = SendMessage(hDlgCtrl, CB_GETITEMDATA, (WPARAM)Index, 0);
	if (ItemData == CB_ERR)
		return FALSE;

	switch(ItemData)
	{
	case PowerActionNone:
		Policy->Action = PowerActionNone;
		Policy->EventCode  = POWER_FORCE_TRIGGER_RESET;
		break;
	case PowerActionWarmEject:
		Policy->Action = PowerActionNone;
		Policy->EventCode  = POWER_USER_NOTIFY_BUTTON;
		break;
	case PowerActionShutdown:
		Policy->Action = PowerActionNone;
		Policy->EventCode = POWER_USER_NOTIFY_SHUTDOWN;
		break;
	case PowerActionSleep:
	case PowerActionHibernate:
		Policy->Action = (POWER_ACTION)ItemData;
		Policy->EventCode = 0;
		break;
	default:
		return FALSE;
	}
	Policy->Flags = (POWER_ACTION_UI_ALLOWED | POWER_ACTION_QUERY_ALLOWED);

	return TRUE;
}




//-------------------------------------------------------------------

void ShowCurrentPowerActionPolicies(HWND hwndDlg)
{
	TCHAR szAction[MAX_PATH];

	if (!IsBatteryUsed())
	{
#if 0
		/* expiremental code */
//		ShowCurrentPowerButtonAcAction(hList2,
		ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON),
									   g_SystemBatteries,
									   sizeof(g_SystemBatteries) / sizeof(POWER_ACTION),
									   &gGPP.user.LidCloseAc);
#else
		if (LoadString(hApplet, IDS_PowerActionNone1+gGPP.user.LidCloseAc.Action, szAction, MAX_PATH))
		{
			SendDlgItemMessage(hwndDlg, IDC_LIDCLOSE,
						 CB_SELECTSTRING,
						 TRUE,
						 (LPARAM)szAction);
		}
#endif
		ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON),
									   g_PowerButton,
									   sizeof(g_PowerButton) / sizeof(POWER_ACTION),
									   &gGPP.user.PowerButtonAc);

#if 0
			/* expiremental code */
		ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_SLEEPBUTTON),
									   g_SleepButton,
									   sizeof(g_SleepButton) / sizeof(POWER_ACTION),
									   &gGPP.user.SleepButtonAc);
#else
		if (LoadString(hApplet, IDS_PowerActionNone1+gGPP.user.SleepButtonAc.Action, szAction, MAX_PATH))
		{
			SendDlgItemMessage(hwndDlg, IDC_SLEEPBUTTON,
						 CB_SELECTSTRING,
						 TRUE,
						 (LPARAM)szAction);
		}
#endif
	}
	else
	{
#if 0

		ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_LIDCLOSE),
									   g_SleepButton,
									   sizeof(g_SleepButton) / sizeof(POWER_ACTION),
									   &gGPP.user.LidCloseDc);

		ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON),
									   g_SleepButton,
									   sizeof(g_SleepButton) / sizeof(POWER_ACTION),
									   &gGPP.user.PowerButtonDc);

		ShowCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_SLEEPBUTTON),
									   g_SleepButton,
									   sizeof(g_SleepButton) / sizeof(POWER_ACTION),
									   &gGPP.user.SleepButtonDc);
#else
		if (LoadString(hApplet, IDS_PowerActionNone1+gGPP.user.LidCloseDc.Action, szAction, MAX_PATH))
		{
			SendDlgItemMessage(hwndDlg, IDC_LIDCLOSE,
						 CB_SELECTSTRING,
						 TRUE,
						 (LPARAM)szAction);
		}
		if (LoadString(hApplet, IDS_PowerActionNone1+gGPP.user.PowerButtonDc.Action, szAction, MAX_PATH))
		{
			SendDlgItemMessage(hwndDlg, IDC_POWERBUTTON,
						 CB_SELECTSTRING,
						 TRUE,
						 (LPARAM)szAction);
		}
		if (LoadString(hApplet, IDS_PowerActionNone1+gGPP.user.SleepButtonDc.Action, szAction, MAX_PATH))
		{
			SendDlgItemMessage(hwndDlg, IDC_SLEEPBUTTON,
						 CB_SELECTSTRING,
						 TRUE,
						 (LPARAM)szAction);
		}
#endif
	}
}

void Adv_InitDialog()
{
	HWND hList1;
	HWND hList2;
	HWND hList3;

	BOOLEAN bSuspend = FALSE;
	BOOLEAN bHibernate;
	BOOLEAN bShutdown;

	SYSTEM_POWER_CAPABILITIES spc;

	CheckDlgButton(hAdv,
		IDC_SYSTRAYBATTERYMETER,
		gGPP.user.GlobalFlags & EnableSysTrayBatteryMeter ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hAdv,
		IDC_PASSWORDLOGON,
		gGPP.user.GlobalFlags & EnablePasswordLogon ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hAdv,
		IDC_VIDEODIMDISPLAY,
		gGPP.user.GlobalFlags & EnableVideoDimDisplay ? BST_CHECKED : BST_UNCHECKED);

	GetPwrCapabilities(&spc);

	if (spc.SystemS1 || spc.SystemS2 || spc.SystemS3)
		bSuspend=TRUE;

	bHibernate = spc.HiberFilePresent;
	bShutdown = spc.SystemS5;

	hList1 = GetDlgItem(hAdv, IDC_LIDCLOSE);
	SendMessage(hList1, CB_RESETCONTENT, 0, 0);

	memset(g_SystemBatteries, 0x0, sizeof(g_SystemBatteries));
	if (spc.SystemBatteriesPresent)
	{
		AddItem(hList1, IDS_PowerActionNone1, (LPARAM)PowerActionNone, g_SystemBatteries);

		if (bSuspend)
		{
			AddItem(hList1, IDS_PowerActionSleep, (LPARAM)PowerActionSleep, g_SystemBatteries);
		}

		if (bHibernate)
		{
			AddItem(hList1, IDS_PowerActionHibernate, (LPARAM)PowerActionHibernate, g_SystemBatteries);
		}
	}
	else
	{
		ShowWindow(GetDlgItem(hAdv, IDC_VIDEODIMDISPLAY), FALSE);
		ShowWindow(GetDlgItem(hAdv, IDC_SLIDCLOSE), FALSE);
		ShowWindow(hList1, FALSE);
	}

	hList2 = GetDlgItem(hAdv, IDC_POWERBUTTON);
	SendMessage(hList2, CB_RESETCONTENT, 0, 0);

	memset(g_PowerButton, 0x0, sizeof(g_PowerButton));
	if (spc.PowerButtonPresent)
	{
		AddItem(hList2, IDS_PowerActionNone1, (LPARAM)PowerActionNone, g_PowerButton);
		AddItem(hList2, IDS_PowerActionWarmEject, (LPARAM)PowerActionWarmEject, g_PowerButton);

		if (bSuspend)
		{
			AddItem(hList2, IDS_PowerActionSleep, (LPARAM)PowerActionSleep, g_PowerButton);
		}

		if (bHibernate)
		{
			AddItem(hList2, IDS_PowerActionHibernate, (LPARAM)PowerActionHibernate, g_PowerButton);

		}
		if (bShutdown)
		{
			AddItem(hList2, IDS_PowerActionShutdown, (LPARAM)PowerActionShutdown, g_PowerButton);
		}
	}
	else
	{
		ShowWindow(GetDlgItem(hAdv, IDC_SPOWERBUTTON), FALSE);
		ShowWindow(hList2, FALSE);
	}

	hList3=GetDlgItem(hAdv, IDC_SLEEPBUTTON);
	SendMessage(hList3, CB_RESETCONTENT, 0, 0);
    memset(g_SleepButton, 0x0, sizeof(g_SleepButton));

	if (spc.SleepButtonPresent)
	{
		AddItem(hList3, IDS_PowerActionNone1, (LPARAM)PowerActionNone, g_SleepButton);
		AddItem(hList3, IDS_PowerActionWarmEject, (LPARAM)PowerActionWarmEject, g_SleepButton);

		if (bSuspend)
		{
			AddItem(hList3, IDS_PowerActionSleep, (LPARAM)PowerActionSleep, g_SleepButton);
		}

		if (bHibernate)
		{
			AddItem(hList3, IDS_PowerActionHibernate, (LPARAM)PowerActionHibernate, g_SleepButton);
		}

		if (bShutdown)
		{
			AddItem(hList3, IDS_PowerActionShutdown, (LPARAM)PowerActionShutdown, g_SleepButton);
		}
	}
	else
	{
		ShowWindow(GetDlgItem(hAdv, IDC_SSLEEPBUTTON), FALSE);
		ShowWindow(hList3, FALSE);
	}

	if (ReadGlobalPwrPolicy(&gGPP))
	{
		ShowCurrentPowerActionPolicies(hAdv);
	}
}


void Adv_SaveData(HWND hwndDlg)
{
	BOOL bSystrayBatteryMeter;
	BOOL bPasswordLogon;
	BOOL bVideoDimDisplay;

	bSystrayBatteryMeter =
		(IsDlgButtonChecked(hwndDlg, IDC_SYSTRAYBATTERYMETER) == BST_CHECKED);

	bPasswordLogon =
		(IsDlgButtonChecked(hwndDlg, IDC_PASSWORDLOGON) == BST_CHECKED);

	bVideoDimDisplay =
		(IsDlgButtonChecked(hwndDlg, IDC_VIDEODIMDISPLAY) == BST_CHECKED);

	if (bSystrayBatteryMeter)
	{
		if (!(gGPP.user.GlobalFlags & EnableSysTrayBatteryMeter))
		{
			gGPP.user.GlobalFlags = gGPP.user.GlobalFlags + EnableSysTrayBatteryMeter;
		}
	}
	else
	{
		if ((gGPP.user.GlobalFlags & EnableSysTrayBatteryMeter))
		{
			gGPP.user.GlobalFlags = gGPP.user.GlobalFlags - EnableSysTrayBatteryMeter;
		}
	}

	if (bPasswordLogon)
	{
		if (!(gGPP.user.GlobalFlags & EnablePasswordLogon))
		{
			gGPP.user.GlobalFlags = gGPP.user.GlobalFlags + EnablePasswordLogon;
		}
	}
	else
	{
		if ((gGPP.user.GlobalFlags & EnablePasswordLogon))
		{
			gGPP.user.GlobalFlags = gGPP.user.GlobalFlags - EnablePasswordLogon;
		}
	}

	if (bVideoDimDisplay)
	{
		if (!(gGPP.user.GlobalFlags & EnableVideoDimDisplay))
		{
			gGPP.user.GlobalFlags = gGPP.user.GlobalFlags + EnableVideoDimDisplay;
		}
	}
	else
	{
		if ((gGPP.user.GlobalFlags & EnableVideoDimDisplay))
		{
			gGPP.user.GlobalFlags = gGPP.user.GlobalFlags - EnableVideoDimDisplay;
		}
	}

	if (!IsBatteryUsed())
	{
		SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON), &gGPP.user.PowerButtonAc);
#if 0
		SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_LIDCLOSE), &gGPP.user.LidCloseAc);
		SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_SLEEPBUTTON), &gGPP.user.SleepButtonAc);
#endif
	}
	else
	{
#if 0
		SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_POWERBUTTON), &gGPP.user.PowerButtonDc);
		SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_LIDCLOSE), &gGPP.user.LidCloseDc);
		SaveCurrentPowerActionPolicy(GetDlgItem(hwndDlg, IDC_SLEEPBUTTON), &gGPP.user.SleepButtonDc);
#endif
	}

	if (!WriteGlobalPwrPolicy(&gGPP))
	{
		MessageBox(hwndDlg, L"WriteGlobalPwrPolicy failed", NULL, MB_OK);
	}

	Adv_InitDialog();
}
