#ifndef _INC_PGEXTRAS_H
#define _INC_PGEXTRAS_H

#include "pgbase.h"
#include "Select.h"

class CYesNoPg : public WizardPage
{
public:
	CYesNoPg(LPPROPSHEETPAGE ppsp, int nIdTitle, int nIdSubTitle, int nPageId)
		: WizardPage(ppsp, nIdTitle, nIdSubTitle)
	{
		m_dwPageId = nPageId;
		ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	}
protected:
	virtual BOOL IsOptionSet() = 0;
	virtual void SetOptions(BOOL bOn) = 0;
	virtual int GetSettingsPage() {return 0;} // Default is no next page

	
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		// Make sure the controls exist
		_ASSERTE(GetDlgItem(m_hwnd, IDC_RADIOYES));
		_ASSERTE(GetDlgItem(m_hwnd, IDC_RADIONO));

		// Set Yes/No buttons
		Button_SetCheck(GetDlgItem(m_hwnd, IDC_RADIOYES), IsOptionSet());
		Button_SetCheck(GetDlgItem(m_hwnd, IDC_RADIONO), !IsOptionSet());
		return 1;
	}
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lResult = 1;
		SetOptions(Button_GetCheck(GetDlgItem(m_hwnd, IDC_RADIOYES)));
		g_Options.ApplyPreview();
		return lResult;
	}
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
	{
	 	DWORD dwTemp = GetSettingsPage();
		if(dwTemp)
		{
			if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_RADIOYES)))
				sm_WizPageOrder.AddPages(m_dwPageId, &dwTemp, 1);
			else
				sm_WizPageOrder.RemovePages(&dwTemp, 1);
		}
		return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
	}


};

class CSoundSentryPg : public CYesNoPg
{
public:
    CSoundSentryPg(LPPROPSHEETPAGE ppsp)
		: CYesNoPg(ppsp, IDS_SNDWIZSENTRYTITLE, IDS_SNDWIZSENTRYSUBTITLE, IDD_SNDWIZSENTRYSHOWSOUNDS) {}
protected:
	BOOL IsOptionSet()
	{
		return (g_Options.m_schemePreview.m_SOUNDSENTRY.dwFlags & SSF_SOUNDSENTRYON);
	}
	void SetOptions(BOOL bOn)
	{
		if(bOn)
		{
			g_Options.m_schemePreview.m_SOUNDSENTRY.dwFlags |= SSF_SOUNDSENTRYON;
            g_Options.m_schemePreview.m_SOUNDSENTRY.dwFlags &= SSF_VALID;

			// Hard code to flash 'Window' if we are setting this option
			g_Options.m_schemePreview.m_SOUNDSENTRY.iWindowsEffect = SSWF_WINDOW;

            // For NT: The other options are not supported. a-anilk
			g_Options.m_schemePreview.m_SOUNDSENTRY.iFSTextEffect = 0;
			g_Options.m_schemePreview.m_SOUNDSENTRY.iFSGrafEffect = 0;
		}
		else
			g_Options.m_schemePreview.m_SOUNDSENTRY.dwFlags &= ~SSF_SOUNDSENTRYON;
	}
};

class CShowSoundsPg : public CYesNoPg
{
public:
    CShowSoundsPg(LPPROPSHEETPAGE ppsp)
		: CYesNoPg(ppsp, IDS_SNDWIZSHOWSOUNDSTITLE, IDS_SNDWIZSHOWSOUNDSSUBTITLE, IDD_SNDWIZSENTRYSHOWSOUNDS2) {}
protected:
	BOOL IsOptionSet()
	{
		return (g_Options.m_schemePreview.m_bShowSounds);
	}
	void SetOptions(BOOL bOn)
	{
		g_Options.m_schemePreview.m_bShowSounds = bOn;
	}
};

class CFilterKeysPg : public CYesNoPg
{
public:
    CFilterKeysPg(LPPROPSHEETPAGE ppsp)
		: CYesNoPg(ppsp, IDS_WIZFILTERKEYSTITLE, IDS_WIZFILTERKEYSSUBTITLE, IDD_KBDWIZFILTERKEYS1) {}
protected:
	BOOL IsOptionSet()
	{
		// Only return TRUE if filterkeys is ON and is set to BounceKeys
		return (g_Options.m_schemePreview.m_FILTERKEYS.dwFlags & FKF_FILTERKEYSON
			&&	g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec);
	}
	void SetOptions(BOOL bOn)
	{
		if(bOn)
		{
			g_Options.m_schemePreview.m_FILTERKEYS.dwFlags |= FKF_FILTERKEYSON;

			// NOTE: We are hard coding the bounce rate to .5 seconds when
			// we turn this option ON.
			g_Options.m_schemePreview.m_FILTERKEYS.iWaitMSec = 0;
			g_Options.m_schemePreview.m_FILTERKEYS.iDelayMSec = 0;
			g_Options.m_schemePreview.m_FILTERKEYS.iRepeatMSec = 0;
            // Take this value from access.cpl
			// g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec = 500; a-anilk
		}
		else
			g_Options.m_schemePreview.m_FILTERKEYS.dwFlags &= ~FKF_FILTERKEYSON;
	}
	int GetSettingsPage() {return IDD_KBDWIZFILTERKEYS2;}

};


/***************************************/

//
// Times are in milliseconds
//
#define BOUNCESIZE 5
UINT BounceTable[BOUNCESIZE] = {
    {  500 },
    {  700 },
    { 1000 },
    { 1500 },
    { 2000 }
};

class CFilterKeysSettingsPg : public WizardPage
{
public:
    CFilterKeysSettingsPg(LPPROPSHEETPAGE ppsp)
		: WizardPage(ppsp, IDS_WIZFILTERKEYSETTING, IDS_WIZFILTERKEYSSUBTITLE)
	{
		m_dwPageId = IDD_KBDWIZFILTERKEYS2;
		ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	}

protected:
	void UpdateTime()
	{
		INT_PTR nBounceRate = SendDlgItemMessage(m_hwnd,IDC_BK_BOUNCERATE, TBM_GETPOS, 0,0);
		if(nBounceRate < 1 || nBounceRate > BOUNCESIZE)
			nBounceRate = 1;

		// Look up in table
		nBounceRate = BounceTable[nBounceRate - 1];

		TCHAR buf[10], buf2[20];
		wsprintf(buf,__TEXT("%d.%d"),nBounceRate/1000,	(nBounceRate%1000)/100);
		GetNumberFormat(LOCALE_USER_DEFAULT, 0, buf, NULL, buf2, 20);
		SetDlgItemText(m_hwnd, IDC_BK_TIME, buf2);
	}
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		_ASSERTE(g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec);
		_ASSERTE(GetDlgItem(m_hwnd, IDC_RADIOBEEPYES));
		_ASSERTE(GetDlgItem(m_hwnd, IDC_RADIOBEEPNO));

		Button_SetCheck(GetDlgItem(m_hwnd, IDC_RADIOBEEPYES), (g_Options.m_schemePreview.m_FILTERKEYS.dwFlags & FKF_CLICKON));
		Button_SetCheck(GetDlgItem(m_hwnd, IDC_RADIOBEEPNO), !(g_Options.m_schemePreview.m_FILTERKEYS.dwFlags & FKF_CLICKON));

		// Set slider for bounce rate
		SendDlgItemMessage(m_hwnd,IDC_BK_BOUNCERATE, TBM_SETRANGE,
							 TRUE,MAKELONG(1,BOUNCESIZE));

		// Figure out initial settings
		// Make sure initial slider settings is not SMALLER than current setting
		int nIndex = 0;
		for(int i=BOUNCESIZE - 1;i>=0;i--)
		{
			if(BounceTable[i] >= g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec)
				nIndex = i;
			else
				break;
		}
		SendDlgItemMessage(m_hwnd,IDC_BK_BOUNCERATE, TBM_SETPOS, TRUE, nIndex+1);
		UpdateTime();

		return 1;
	}
	LRESULT HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_HSCROLL:
			{
				UpdateTime();

				if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_RADIOBEEPYES)))
					g_Options.m_schemePreview.m_FILTERKEYS.dwFlags |= FKF_CLICKON;
				else
					g_Options.m_schemePreview.m_FILTERKEYS.dwFlags &= ~FKF_CLICKON;

				// Bounce Keys
				INT_PTR nIndex = SendDlgItemMessage(m_hwnd, IDC_BK_BOUNCERATE, TBM_GETPOS, 0, 0);
				g_Options.m_schemePreview.m_FILTERKEYS.iWaitMSec = 0;
				g_Options.m_schemePreview.m_FILTERKEYS.iDelayMSec = 0;
				g_Options.m_schemePreview.m_FILTERKEYS.iRepeatMSec = 0;
				g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec = BounceTable[nIndex - 1];

				g_Options.ApplyPreview();
			}
			break;
		default:
			break;
		}
		return 0;
	}

    // Handle Next> and make sure you transfer all settings as the user may not 
    // always change bounce repeat rate : a-anilk
    LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lResult = 1;

		WORD wNotifyCode = HIWORD(wParam);
		WORD wCtlID      = LOWORD(wParam);
		HWND hwndCtl     = (HWND)lParam;

		switch(wCtlID)
		{
		case IDC_RADIOBEEPYES:
		case IDC_RADIOBEEPNO:
			if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_RADIOBEEPYES)))
				g_Options.m_schemePreview.m_FILTERKEYS.dwFlags |= FKF_CLICKON;
			else
				g_Options.m_schemePreview.m_FILTERKEYS.dwFlags &= ~FKF_CLICKON;

            g_Options.ApplyPreview();

			lResult = 0;
			break;

		default:
			break;
		}
		return lResult;
	}

};

////////////////////////////////////////////////////////////////////
// Mouse Keys
class CMouseKeysPg : public CYesNoPg
{
public:
    CMouseKeysPg(LPPROPSHEETPAGE ppsp)
		: CYesNoPg(ppsp, IDS_MSEWIZMOUSEKEYSTITLE, IDS_MSEWIZMOUSEKEYSSUBTITLE, IDD_MSEWIZMOUSEKEYS) {}
protected:
	BOOL IsOptionSet()
	{
		return (g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags & MKF_MOUSEKEYSON);
	}
	void SetOptions(BOOL bOn)
	{
		if(bOn)
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags |= MKF_MOUSEKEYSON;
		else
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags &= ~MKF_MOUSEKEYSON;
	}
	int GetSettingsPage() {return IDD_MSEWIZMOUSEKEYS1;}

};

static UINT g_nSpeedTable[] = { 10, 20, 30, 40, 60, 80, 120, 180, 360 };
static UINT g_nAccelTable[] = { 5000, 4500, 4000, 3500, 3000, 2500, 2000, 1500, 1000 };

class CMouseKeysSettingsPg : public WizardPage
{
public:
    CMouseKeysSettingsPg(LPPROPSHEETPAGE ppsp)
		: WizardPage(ppsp, IDS_MSEWIZMOUSEKEYSETTING, IDS_MSEWIZMOUSEKEYSSUBTITLE)
	{
		m_dwPageId = IDD_MSEWIZMOUSEKEYS1;
		ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	}

protected:
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		// ALWAYS use modifiers
//		Button_SetCheck(GetDlgItem(m_hwnd, IDC_MK_USEMODKEYS), g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags & MKF_MODIFIERS);

		if(g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags & MKF_REPLACENUMBERS)
			Button_SetCheck(GetDlgItem(m_hwnd, IDC_MK_NLON), TRUE);
		else
			Button_SetCheck(GetDlgItem(m_hwnd, IDC_MK_NLOFF), TRUE);

		SendDlgItemMessage(m_hwnd,IDC_MK_TOPSPEED, TBM_SETRANGE, TRUE,MAKELONG(0,8));
		SendDlgItemMessage(m_hwnd,IDC_MK_ACCEL, TBM_SETRANGE, TRUE,MAKELONG(0,8));

		int nIndex = 0;
		for(int i=8;i>=0;i--)
		{
			if(g_nSpeedTable[i] >= g_Options.m_schemePreview.m_MOUSEKEYS.iMaxSpeed)
				nIndex = i;
			else
				break;
		}
		SendDlgItemMessage(m_hwnd,IDC_MK_TOPSPEED, TBM_SETPOS, TRUE, nIndex);

		for(i=8;i>=0;i--)
		{
			if(g_nAccelTable[i] <= g_Options.m_schemePreview.m_MOUSEKEYS.iTimeToMaxSpeed)
				nIndex = i;
			else
				break;
		}
		SendDlgItemMessage(m_hwnd,IDC_MK_ACCEL, TBM_SETPOS, TRUE, nIndex);
		return 1;
	}

	void GetSettingsFromControls()
	{
		INT_PTR nIndex;

		g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags |= MKF_MOUSEKEYSON;
		if(TRUE/*Button_GetCheck(GetDlgItem(m_hwnd, IDC_MK_USEMODKEYS))*/) // NOTE: ALWAYS use modifiers
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags |= MKF_MODIFIERS;
		else
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags &= ~MKF_MODIFIERS;

		if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_MK_NLON)))
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags |= MKF_REPLACENUMBERS;
		else
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags &= ~MKF_REPLACENUMBERS;

		nIndex = SendDlgItemMessage(m_hwnd, IDC_MK_TOPSPEED, TBM_GETPOS, 0, 0);
		g_Options.m_schemePreview.m_MOUSEKEYS.iMaxSpeed = g_nSpeedTable[nIndex];
		nIndex = SendDlgItemMessage(m_hwnd, IDC_MK_ACCEL, TBM_GETPOS, 0, 0);
		g_Options.m_schemePreview.m_MOUSEKEYS.iTimeToMaxSpeed = g_nAccelTable[nIndex];

 #pragma message("Handle THis!")
		// 3/15/95 -
		// Always init the control speed to 1/8 of the screen width/
//		g_mk.iCtrlSpeed = GetSystemMetrics(SM_CXSCREEN) / 16;

		g_Options.ApplyPreview();
	}

	LRESULT HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_HSCROLL:
			GetSettingsFromControls();
			break;
		default:
			break;
		}
		return 0;
	}

	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lResult = 1;

		WORD wNotifyCode = HIWORD(wParam);
		WORD wCtlID      = LOWORD(wParam);
		HWND hwndCtl     = (HWND)lParam;

		switch(wCtlID)
		{
		case IDC_MK_NLON:
		case IDC_MK_NLOFF:
			GetSettingsFromControls();
			lResult = 0;
			break;

		default:
			break;
		}
		return lResult;
	}
};

//
////////////////////////////////////////////////////////////////////


class CStickyKeysPg : public CYesNoPg
{
public:
    CStickyKeysPg(LPPROPSHEETPAGE ppsp)
		: CYesNoPg(ppsp, IDS_WIZSTICKYKEYSTITLE, IDS_WIZSTICKYKEYSSUBTITLE, IDD_KBDWIZSTICKYKEYS) {}
protected:
	BOOL IsOptionSet()
	{
		return g_Options.m_schemePreview.m_STICKYKEYS.dwFlags & SKF_STICKYKEYSON;
	}
	void SetOptions(BOOL bOn)
	{
		if(bOn)
		{
			g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_STICKYKEYSON;

			// Turn selected flags on
			g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_TRISTATE;
			g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_TWOKEYSOFF;
			g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_AUDIBLEFEEDBACK;
		}
		else
			g_Options.m_schemePreview.m_STICKYKEYS.dwFlags &= ~SKF_STICKYKEYSON;
	}
};

class CToggleKeysPg : public CYesNoPg
{
public:
    CToggleKeysPg(LPPROPSHEETPAGE ppsp)
		: CYesNoPg(ppsp, IDS_WIZTOGGLEKEYSTITLE, IDS_WIZTOGGLEKEYSSUBTITLE, IDD_KBDWIZTOGGLEKEYS) {}
protected:
	BOOL IsOptionSet()
	{
		return g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags & TKF_TOGGLEKEYSON;
	}
	void SetOptions(BOOL bOn)
	{
		if(bOn)
			g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags |= TKF_TOGGLEKEYSON;
		else
			g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags &= ~TKF_TOGGLEKEYSON;
	}
};

class CShowKeyboardHelpPg : public CYesNoPg
{
public:
    CShowKeyboardHelpPg(LPPROPSHEETPAGE ppsp)
		: CYesNoPg(ppsp, IDS_WIZSHOWEXTRAKEYBOARDHELPTITLE, IDS_WIZSHOWEXTRAKEYBOARDHELPSUBTITLE, IDD_KBDWIZSHOWEXTRAKEYBOARDHELP) {}
protected:
	BOOL IsOptionSet()
	{
		return g_Options.m_schemePreview.m_bShowExtraKeyboardHelp;
	}
	void SetOptions(BOOL bOn)
	{
		g_Options.m_schemePreview.m_bShowExtraKeyboardHelp = bOn;
	}
};


class CMouseSpeedPg : public WizardPage
{
public:
    CMouseSpeedPg(LPPROPSHEETPAGE ppsp)
		: WizardPage(ppsp, IDS_MSEWIZMOUSESPEEDTITLE, IDS_MSEWIZMOUSESPEEDSUBTITLE)
	{
		m_dwPageId = IDD_MSEWIZMOUSESPEED;
		ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	}

protected:
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		// Set slider for mouse speed, (limit 1 to 20), and set its initial value
		SendDlgItemMessage(m_hwnd,IDC_SLIDER1, TBM_SETRANGE, TRUE,MAKELONG(1,20));
		SendDlgItemMessage(m_hwnd,IDC_SLIDER1, TBM_SETPOS, TRUE, min(20, max(1, g_Options.m_schemePreview.m_nMouseSpeed)));
		return 1;
	}
	LRESULT HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_HSCROLL:
			g_Options.m_schemePreview.m_nMouseSpeed = (UINT)SendDlgItemMessage(m_hwnd, IDC_SLIDER1, TBM_GETPOS, 0, 0);
			g_Options.ApplyPreview();
			break;
		default:
			break;
		}
		return 0;
	}
};


class CMouseTrailsPg : public CYesNoPg
{
public:
    CMouseTrailsPg(LPPROPSHEETPAGE ppsp)
		: CYesNoPg(ppsp, IDS_MSEWIZMOUSETRAILSTITLE, IDS_MSEWIZMOUSETRAILSSUBTITLE, IDD_MSEWIZMOUSETRAILS) {}
protected:
	BOOL IsOptionSet()
	{
		return (g_Options.m_schemePreview.m_nMouseTrails > 1);
	}
	void SetOptions(BOOL bOn)
	{
		g_Options.m_schemePreview.m_nMouseTrails = bOn?7:0;
	}
};




#endif // _INC_PGEXTRAS_H
