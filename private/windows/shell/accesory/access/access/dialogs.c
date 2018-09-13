/****************************************************************************

    DIALOGS.C

    This file handles the parts related to the dialogs for adjustment

****************************************************************************/
#include "windows.h"

#include "Dialogs.h"
#include "Menu.h"
#include "skeys.h"
#include "resource.h"
#include "access.h"

// debug only in theory
void AssertBool( LPSTR sz, LPBOOL lpf );
void OkMsgBox( LPSTR szCaption, LPSTR szFormat,... );

#include <stdio.h>
#include <io.h>

/****************************************************************************

    Declaration of externs       

****************************************************************************/

						    /* from ACCESS.C */
extern BOOL    OK_to_Save;
extern HANDLE  hInst;
extern int     iCN;
extern int     iBR;
extern int     iCCN;
extern int     iCBR;
extern BOOL    bCO;
extern char    lpReturn;
extern int     monitor_type;

int fQuestion_Save = ID_FLAG_1;

extern  void InitSerialKeys();

#ifdef NOTUSED
extern COMMVARS passthecomvars;
extern COMMVARS *lpa_passthecomvars;
#endif

HWND FilterhWnd;
HWND StickyhWnd;
HWND MousehWnd;
HWND TogglehWnd;
HWND SerialhWnd;
HWND TimeOuthWnd;
HWND ShowSoundshWnd;
HWND SoundSentryhWnd;

BOOL SerialKeysEnabled = FALSE;
/****************************************************************************

    Declaration of variables

****************************************************************************/

FILTERKEYS    FilterKeysParam;
STICKYKEYS    StickyKeysParam;
MOUSEKEYS     MouseKeysParam;
TOGGLEKEYS    ToggleKeysParam;
ACCESSTIMEOUT TimeOutParam;
SOUNDSENTRY   SoundSentryParam;

#ifdef MYSK
MYSERIALKEYS  MySerialKeysParam;
#else
SERIALKEYS    SerialKeysParam;
#endif

CHAR          SK_ActivePort[MAX_PATH];
INT           fShowSoundsOn;

/*
 * Sound Sentry support
 */
HWND hSSListBox;
typedef struct tagSSInfo {
    UINT idResource;
    UINT iEffectFlag;
    UINT idDialog;
    TCHAR szEffect[128];
} SSINFO, *PSSINFO;
SSINFO aSSWin[4] = {{IDS_NONE, WF_NONE, 0},
		    {IDS_SS_WINCAPTION, WF_TITLE, 0},
		    {IDS_SS_WINWINDOW, WF_WINDOW, 0},
		    {IDS_SS_WINDESKTOP, WF_DISPLAY, 0}};
#ifdef BORDERFLASH
SSINFO aSSText[3] = {{IDS_NONE, TF_NONE, 0},
		     {IDS_SS_FSBORDER, TF_BORDER, 0},
		     {IDS_SS_FSSCREEN, TF_DISPLAY, 0}};
#else
SSINFO aSSText[2] = {{IDS_NONE, TF_NONE, 0},
		     {IDS_SS_FSSCREEN, TF_DISPLAY, 0}};
#endif

#ifdef GRAPHICSMODE
SSINFO aSSGraphics[2] = {{IDS_NONE, GF_NONE, 0},
			 {IDS_SS_GRAPHICSSCREEN, GF_DISPLAY, 0}};

UINT cGraphicEffects = (sizeof(aSSGraphics) / sizeof(SSINFO));
#endif

UINT cWindowsEffects = (sizeof(aSSWin) / sizeof(SSINFO));
UINT cTextEffects = (sizeof(aSSText) / sizeof(SSINFO));

void SerialKeysRemove(HWND hDlg);
void SerialKeysInstall(HWND hDlg);
void SetDialogItems(HWND hDlg);
void SKeysError(HWND hDlg, int Error);


int  iSK_ComName;
int  iSK_BaudRate;
BOOL bSK_CommOpen;

BOOL userpainthidden;
HBRUSH hBrush =0;

//  int        OnOffTable[2] = {
//    { FALSE    },
//    { TRUE    },
//};

BYTE OnOffTable[2] = {
    { FALSE },
    { TRUE },
};


/***************************************/

//
// Times are in milliseconds
//
UINT DelayTable[4] = {
    {  700 },
    { 1000 },
    { 1500 },
    { 2000 },
};

int DelayTableConv[4] = {
    { 1 },
    { 2 },
    { 3 },
    { 4 },
};

/***************************************/

//
// Times are in milliseconds
//
UINT RateTable[7] = {
    {    0 },
    {  300 },
    {  500 },
    {  700 },
    { 1000 },
    { 1500 },
    { 2000 },
};

int RateTableConv[7] = {
    { 1 },
    { 2 },
    { 3 },
    { 4 },
    { 5 },
    { 6 },
    { 7 },
};

/***************************************/

//
// Times are in milliseconds
//
UINT BounceTable[6] = {
    {    0 },
    {  500 },
    {  700 },
    { 1000 },
    { 1500 },
    { 2000 },
};


int BounceTableConv[6] = {
    { 1 },
    { 2 },
    { 3 },
    { 4 },
    { 5 },
    { 6 },
};


/***************************************/

//
// Times are in milliseconds
//
UINT AcceptTable[7] = {
    {    0 },
    {  300 },
    {  500 },
    {  700 },
    { 1000 },
    { 1400 },
    { 2000 },
};

int AcceptTableConv[7] = {
    { 1 },
    { 2 },
    { 3 },
    { 4 },
    { 5 },
    { 6 },
    { 7 },
};

/***************************************/

//
// Times are in milliseconds
//
UINT TimeTable[9] = {
    { 5000 },
    { 4500 },
    { 4000 },
    { 3500 },
    { 3000 },
    { 2500 },
    { 2000 },
    { 1500 },
    { 1000 },
};

int TimeTableConv[9] = {
    { 1 },
    { 2 },
    { 3 },
    { 4 },
    { 5 },
    { 6 },
    { 7 },
    { 8 },
    { 9 },
};


//
// Pixels per second
//
UINT SpeedTable[9] = {
    { 10  },
    { 20  },
    { 30  },
    { 40  },
    { 60  },
    { 80  },
    { 120 },
    { 180 },
    { 360 },
};

int SpeedTableConv[9] = {
    { 1 },
    { 2 },
    { 3 },
    { 4 },
    { 5 },
    { 6 },
    { 7 },
    { 8 },
    { 9 },
};


//
// Times are in milliseconds
//
DWORD TimeOutTable[4] = {
      {  300000 },   //  5 minutes
      {  600000 },   // 10 minutes
      {  900000 },   // 15 minutes
      { 1800000 },   // 30 minutes
};

/****************************************************************************/

struct AcceptText {
    char *acceptstrings;
}
AcceptTableText[]= {

{"OFF"},
{"0.3"},
{"0.5"},
{"0.7"},
{"1.0"},
{"1.4"},
{"2.0"},
};

struct    DelayText {
    char *delaystrings;
}
DelayTableText[]= {

{"0.7"},
{"1.0"},
{"1.5"},
{"2.0"},
};

struct    RateText {
    char *ratestrings;
}
RateTableText[]= {

{"OFF"},
{"0.3"},
{"0.5"},
{"0.7"},
{"1.0"},
{"1.5"},
{"2.0"},
};

struct    BounceText {
    char *bouncestrings;
}
BounceTableText[]= {

{"OFF"},
{"0.5"},
{"0.7"},
{"1.0"},
{"1.5"},
{"2.0"},
};

struct    TableText {
    char *timestrings;
}
TimeTableText[]= {

{"5.0"},
{"4.5"},
{"4.0"},
{"3.5"},
{"3.0"},
{"2.5"},
{"2.0"},
{"1.5"},
{"1.0"},
};

/****************************************************************************/
/* DEBUGGING  */
//
//void OkMsgBox (HWND hWnd, char *szCaption, char *szFormat)
//    {
//    char szBuffer[256];
//    char *pArguments;
//
//    pArguments = (char *) &szFormat + sizeof szFormat;
//    vsprintf (szBuffer, szFormat, pArguments);
//    MessageBox (hWnd, szBuffer, szCaption, MB_OK);
//    }
//
/****************************************************************************/
void OkFiltersMessage (HWND hWnd, WORD wnumber)

{
    char sreadbuf[255];
    char    sbuf[45];
    {

    LoadString (hInst,wnumber,(LPSTR)sreadbuf,245);
    LoadString (hInst,IDS_TITLE,(LPSTR)sbuf,35);
    MessageBox (hWnd, (LPSTR)sreadbuf, (LPSTR)sbuf, MB_OK|MB_ICONHAND);
    }
return;
}

/****************************************************************************

    FUNCTION:    AdjustFilterKeys(HWND, unsigned, WORD, LONG)

    PURPOSE:

    COMMENTS:


****************************************************************************/

LRESULT APIENTRY AdjustFilterKeys(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    int    i;
    DWORD  colorcheck,colortest;
    static int  iaccepttemp,idelaytemp,iratetemp,ibouncetemp;
    static int  iaccept,idelay,irate,ibounce;
    static BOOL userpainton,userpaintoff,userpaintoverlap;
    static BOOL fMyQuestion_Save;
    POINT point;
    HWND hFCtrl,hFCtrlID,hCtrlAcceptance,hCtrlDelay,hCtrlRate,hCtrlBounce,hCtrlKybTest;
    char *lpaccepttext,*lpdelaytext,*lpratetext,*lpbouncetext;
    FILTERKEYS *lp_FilterKeys_Param;
    static FILTERKEYS SavedFilterKeysParam;    // local copy for restore on Cancel

    lp_FilterKeys_Param = &FilterKeysParam;

    switch (message) {
    case WM_COMMAND:
	switch (wParam) {

	case ID_OFF:
	case ID_ON:
	    CheckRadioButton(hDlg, ID_OFF, ID_ON, (DWORD)wParam);
	    SetFlag(FilterKeysParam.dwFlags,FKF_FILTERKEYSON,OnOffTable[wParam-ID_OFF]);

		if( TestFlag(FilterKeysParam.dwFlags,FKF_FILTERKEYSON) )
		    {
			if ((iaccepttemp !=1) && (ibouncetemp !=1))
			    {
			    OkFiltersMessage (hDlg,IDS_FILTERS_1);
			    ibouncetemp = 1;
			    SetScrollPos(GetDlgItem(hDlg,ID_KB_BOUNCE_3),SB_CTL,ibouncetemp,TRUE);
			    for (i=0;i<6;i++) {
				if (BounceTableConv[i] == ibouncetemp) break;
				}
			    FilterKeysParam.iBounceMSec = BounceTable[i];
			    lpbouncetext = BounceTableText[i].bouncestrings;
			    SetDlgItemText(hDlg,ID_KB_BOUNCE_TEXT,(LPSTR)lpbouncetext);

			    }
		    }

	    FilterKeysParam.cbSize = sizeof(FilterKeysParam);
	    SystemParametersInfo(
		SPI_SETFILTERKEYS,
		0, // sizeof(FilterKeysParam),
		lp_FilterKeys_Param,
		0);
	    fMyQuestion_Save = 1;
	    break;

	case ID_ON_OFF_FEEDBACK:
	    SetFlag(FilterKeysParam.dwFlags,FKF_HOTKEYSOUND,IsDlgButtonChecked(hDlg, ID_ON_OFF_FEEDBACK) );
	    fMyQuestion_Save = 1;
	    break;

	case ID_ON_OFF_CLICK:
	    SetFlag(FilterKeysParam.dwFlags,FKF_CLICKON,IsDlgButtonChecked(hDlg, ID_ON_OFF_CLICK) );
	    fMyQuestion_Save = 1;
	    break;

	case ID_HOTKEY_ACTIVATION:
	    SetFlag(FilterKeysParam.dwFlags,FKF_HOTKEYACTIVE,IsDlgButtonChecked(hDlg, ID_HOTKEY_ACTIVATION) );
	    fMyQuestion_Save = 1;
	    break;

	case IDOK:
	    FilterKeysParam.cbSize = sizeof(FilterKeysParam);
	    SystemParametersInfo(
		SPI_SETFILTERKEYS,
		0, // sizeof(FilterKeysParam),
		lp_FilterKeys_Param,
		0);
	    FilterhWnd = NULL;
	    if( fMyQuestion_Save ) fQuestion_Save = 1;
	    EndDialog(hDlg, IDOK);
	    break;

	case IDCANCEL:

	    /* Let the caller know the user cancelled */

	    // if ((iaccept != iaccepttemp)||(idelay != idelaytemp)||(irate != iratetemp)||(ibounce != ibouncetemp))
	    //     {
	    //         for (i=0;i<7;i++) {
	    //             if (AcceptTableConv[i] == iaccept) break;
	    //             }
	    //             FilterKeysParam.iWaitMSec = AcceptTable[i];
	    //
	    //         for (i=0;i<4;i++) {
	    //             if (DelayTableConv[i] == idelay) break;
	    //             }
	    //             FilterKeysParam.iDelayMSec = DelayTable[i];
	    //
	    //         for (i=0;i<7;i++) {
	    //             if (RateTableConv[i] == irate) break;
	    //             }
	    //             FilterKeysParam.iRepeatMSec = RateTable[i];
	    //
	    //         for (i=0;i<6;i++) {
	    //             if (BounceTableConv[i] == ibounce) break;
	    //             }
	    //             FilterKeysParam.iBounceMSec = BounceTable[i];
	    //     SystemParametersInfo(
	    //         SPI_SETFILTERKEYS,
	    //         sizeof(FilterKeysParam),
	    //         lp_FilterKeys_Param,
	    //         0);
	    //     }

	    SavedFilterKeysParam.cbSize = sizeof(SavedFilterKeysParam);
	    SystemParametersInfo(               /* restore original state */
		SPI_SETFILTERKEYS,
		0, // sizeof(FilterKeysParam),
		&SavedFilterKeysParam,
		0);
	    FilterKeysParam.cbSize = sizeof(FilterKeysParam);
	    SystemParametersInfo(               /* synch up internal structure */
		SPI_GETFILTERKEYS,
		0, // sizeof(FilterKeysParam),
		&FilterKeysParam,
		0);
	    FilterhWnd = NULL;
	    EndDialog(hDlg, IDCANCEL);
	    break;

	default:
	    return (FALSE);
	}
	return(TRUE);
	break;

/***************************************/

	case WM_ACTIVATEAPP:

	    if (!IsWindowEnabled(GetDlgItem(hDlg,ID_KB_DELAY_3)))
		{
		userpaintoverlap = TRUE;
		return(FALSE);
		break;
		}

/***************************************/

	case WM_CTLCOLORSCROLLBAR:

	     if (userpainton)
		    {
		    userpainton = FALSE;
		    userpaintoverlap = FALSE;

		    colorcheck = GetSysColor(COLOR_SCROLLBAR);
		    colortest = GetBkColor((HDC)wParam);
		    if ((colorcheck == colortest) && (colorcheck == RGB(255,255,255)))                // if background color == scrollbar color
												    // and both are white
			hBrush =CreateSolidBrush(RGB(80,80,80));         //solid gray brush
		    else
			hBrush =CreateSolidBrush(RGB(255,255,255));        //solid white brush

		    point.x = point.y = 0;
		    ClientToScreen (hDlg,&point);
		    UnrealizeObject (hBrush);
		    SetBrushOrgEx((HDC)wParam,point.x,point.y,(LPPOINT)NULL);
		    return((WORD) hBrush);
		    break;
		    }
	    else if (userpaintoff)
		    {
		    userpaintoff = FALSE;
		    userpaintoverlap = FALSE;
		    point.x = point.y = 0;
		    DeleteObject(hBrush);
		    ClientToScreen (hDlg,&point);
		    UnrealizeObject (hBrush);
		    SetBrushOrgEx((HDC)wParam,point.x,point.y,(LPPOINT)NULL);
		    return((WORD) CreateSolidBrush (GetSysColor(COLOR_SCROLLBAR)));
		    break;
		    }
	    else if (userpaintoverlap)
		    {
		    if (lParam == (LPARAM)GetDlgItem(
                                             hDlg,ID_KB_DELAY_3))
			{
			point.x = point.y = 0;
			ClientToScreen (GetDlgItem(hDlg,ID_KB_DELAY_3),&point);
			UnrealizeObject (hBrush);
			SetBrushOrgEx((HDC)wParam,point.x,point.y,(LPPOINT)NULL);
			userpaintoverlap = FALSE;
			return((WORD) hBrush);
			break;
			}
		    else
			{
			return(FALSE);
			break;
			}
		    }
	    else
		    {
		    return(FALSE);
		    break;
		    }

/***************************************/

	case WM_KEYDOWN:

	    switch (wParam)
		{
		case VK_PRIOR:
		    SendMessage (hDlg,WM_HSCROLL,SB_PAGEUP,0L);
		    break;

		case VK_NEXT:
		    SendMessage (hDlg,WM_HSCROLL,SB_PAGEDOWN,0L);
		    break;

		case VK_UP:
		    SendMessage (hDlg,WM_HSCROLL,SB_LINEUP,0L);
		    break;

		case VK_DOWN:
		    SendMessage (hDlg,WM_HSCROLL,SB_LINEDOWN,0L);
		    break;

		case VK_LEFT:
		    SendMessage (hDlg,WM_HSCROLL,SB_PAGEUP,0L);
		    break;

		case VK_RIGHT:
		    SendMessage (hDlg,WM_HSCROLL,SB_PAGEDOWN,0L);
		    break;

		default:
		    return (FALSE);
		    break;

		}
	    return (TRUE);
	    break;


	case WM_HSCROLL:

	    if (userpainthidden)
		hFCtrlID = (HWND)ID_KB_RATE_2;

	    else
		{
		hFCtrl = (HWND)lParam;
		hFCtrlID = (HWND)GetWindowLong (hFCtrl, GWL_ID);
		}

	    if (ID_KB_ACCEPT_3 == (LPARAM)hFCtrlID)
		{
		switch (LOWORD(wParam))
		    {
		case SB_LINEUP:
		    iaccepttemp = max (1,iaccepttemp -1);
		    break;

		case SB_LINEDOWN:
		    iaccepttemp =min (7,iaccepttemp +1);
		    break;

		case SB_PAGEUP:
		    iaccepttemp = max (1,iaccepttemp -2);
		    break;

		case SB_PAGEDOWN:
		    iaccepttemp = min (7,iaccepttemp +2);
		    break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		    iaccepttemp = HIWORD (wParam);
		    break;

		default:
		    return (FALSE);
		  }


		if( TestFlag(FilterKeysParam.dwFlags,FKF_FILTERKEYSON) )
		    {
			if ((iaccepttemp !=1) && (ibouncetemp !=1))
			    {
			    OkFiltersMessage (hDlg,IDS_FILTERS_2);
			    iaccepttemp = 1;
			    }
		    }

		SetScrollPos(hFCtrl,SB_CTL,iaccepttemp,TRUE);

		for (i=0;i<7;i++) {
		    if (AcceptTableConv[i] == iaccepttemp) break;
			}

		    FilterKeysParam.iWaitMSec = AcceptTable[i];
		    lpaccepttext = AcceptTableText[i].acceptstrings;

		SetDlgItemText(hDlg,ID_KB_ACCEPT_TEXT,(LPSTR)lpaccepttext);

		FilterKeysParam.cbSize = sizeof(FilterKeysParam);
		SystemParametersInfo(
		    SPI_SETFILTERKEYS,
		    0, // sizeof(FilterKeysParam),
		    lp_FilterKeys_Param,
		    0);
		fMyQuestion_Save = 1;
		return(TRUE);
		break;
		}

/************************/

	      if (ID_KB_DELAY_3 == (LPARAM)hFCtrlID)
		{
		switch (LOWORD(wParam))
		    {
		case SB_PAGEUP:
		    idelaytemp = max (1,idelaytemp-1);
		    break;

		case SB_LINEUP:
		    idelaytemp = max (1,idelaytemp-1);
		    break;

		case SB_PAGEDOWN:
		    idelaytemp = min (4,idelaytemp+1);
		    break;

		case SB_LINEDOWN:
		    idelaytemp = min (4,idelaytemp+1);
		    break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		    idelaytemp = HIWORD (wParam);
		    break;
		default:
		    return (FALSE);
		  }

		SetScrollPos(hFCtrl,SB_CTL,idelaytemp,TRUE);

		    for (i=0;i<4;i++) {
			if (DelayTableConv[i] == idelaytemp) break;
			}

		    FilterKeysParam.iDelayMSec = DelayTable[i];
		    lpdelaytext = DelayTableText[i].delaystrings;

		SetDlgItemText(hDlg,ID_KB_DELAY_TEXT,(LPSTR)lpdelaytext);
		FilterKeysParam.cbSize = sizeof(FilterKeysParam);
		SystemParametersInfo(
		    SPI_SETFILTERKEYS,
		    0, // sizeof(FilterKeysParam),
		    lp_FilterKeys_Param,
		    0);
		fMyQuestion_Save = 1;
		return(TRUE);
		break;
		}

/************************/

	      if (ID_KB_RATE_2 == (LPARAM)hFCtrlID)
		{
		switch (LOWORD(wParam))
		    {
		case SB_PAGEUP:
		    iratetemp = max (1,iratetemp-2);
		    break;

		case SB_LINEUP:
		    iratetemp = max (1,iratetemp-1);
		    break;

		case SB_PAGEDOWN:
		    iratetemp = min (7,iratetemp+2);
		    break;

		case SB_LINEDOWN:
		    iratetemp = min (7,iratetemp+1);
		    break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		    iratetemp = HIWORD ( wParam );
		    break;
		default:
		    return (FALSE);
		  }


		SetScrollPos(hFCtrl,SB_CTL,iratetemp,TRUE);

                // note the check for SM_THUMBTRACK fixes the bug where
                // we got screwed up if the user drags scroll bar to the
                // off position and we popped up a msgbox before mouse
                // was released!  -- GCL

                if ((iratetemp ==1) &&
                   (LOWORD(wParam) != SB_THUMBTRACK ) &&
                   (IsWindowEnabled (GetDlgItem(hDlg,ID_KB_DELAY_3))))
		    {
		    EnableWindow(GetDlgItem(hDlg,ID_KB_DELAY_3),FALSE);
		    OkFiltersMessage (hDlg,IDS_FILTERS_3);
		    InvalidateRect(GetDlgItem(hDlg,ID_KB_DELAY_3),NULL,FALSE);
		    userpainton = TRUE;
		    userpainthidden = FALSE;
		    ShowWindow(GetDlgItem(hDlg,ID_KB_DELAY_1),SW_HIDE);
		    ShowWindow(GetDlgItem(hDlg,ID_KB_DELAY_TEXT),SW_HIDE);
		    ShowWindow(GetDlgItem(hDlg,ID_KB_DELAY_4),SW_HIDE);
		    ShowWindow(GetDlgItem(hDlg,ID_KB_DELAY_5),SW_HIDE);
		    }
		else if ((iratetemp ==1) && (!IsWindowEnabled (GetDlgItem(hDlg,ID_KB_DELAY_3))))
		    {
		    return (FALSE);
		    break;
		    }
		else if ((iratetemp !=1) && (!IsWindowEnabled (GetDlgItem(hDlg,ID_KB_DELAY_3))))
		    {
		    EnableWindow(GetDlgItem(hDlg,ID_KB_DELAY_3),TRUE);
		    InvalidateRect(GetDlgItem(hDlg,ID_KB_DELAY_3),NULL,FALSE);
		    userpaintoff = TRUE;
		    ShowWindow(GetDlgItem(hDlg,ID_KB_DELAY_1),SW_SHOWNORMAL);
		    ShowWindow(GetDlgItem(hDlg,ID_KB_DELAY_TEXT),SW_SHOWNORMAL);
		    ShowWindow(GetDlgItem(hDlg,ID_KB_DELAY_4),SW_SHOWNORMAL);
		    ShowWindow(GetDlgItem(hDlg,ID_KB_DELAY_5),SW_SHOWNORMAL);
		    }

		for (i=0;i<7;i++)
		    {
		    if (iratetemp == RateTableConv[i] ) break;
		    }
		FilterKeysParam.iRepeatMSec = RateTable[i];
		lpratetext = RateTableText[i].ratestrings;

		SetDlgItemText(hDlg,ID_KB_RATE_TEXT,(LPSTR)lpratetext);

		FilterKeysParam.cbSize = sizeof(FilterKeysParam);
		SystemParametersInfo(
		    SPI_SETFILTERKEYS,
		    0, // sizeof(FilterKeysParam),
		    lp_FilterKeys_Param,
		    0);
		fMyQuestion_Save = 1;
		return(TRUE);
		break;
		}

/************************/

	      if (ID_KB_BOUNCE_3 == (LPARAM)hFCtrlID)
		{
		switch (LOWORD(wParam))
		    {
		case SB_PAGEUP:
		    ibouncetemp = max (1,ibouncetemp-2);
		    break;

		case SB_LINEUP:
		    ibouncetemp = max (1,ibouncetemp-1);
		    break;

		case SB_PAGEDOWN:
		    ibouncetemp = min (6,ibouncetemp+2);
		    break;

		case SB_LINEDOWN:
		    ibouncetemp = min (6,ibouncetemp+1);
		    break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		    ibouncetemp = HIWORD (wParam);
		    break;
		default:
		    return (FALSE);
		  }


		if( TestFlag(FilterKeysParam.dwFlags,FKF_FILTERKEYSON) )
		    {
			if ((iaccepttemp !=1) && (ibouncetemp !=1))
			    {
			    OkFiltersMessage (hDlg,IDS_FILTERS_2);
			    ibouncetemp = 1;
			    }
		    }

		SetScrollPos(hFCtrl,SB_CTL,ibouncetemp,TRUE);

		    for (i=0;i<6;i++) {
			if (BounceTableConv[i] == ibouncetemp) break;
			}

		    FilterKeysParam.iBounceMSec = BounceTable[i];
		    lpbouncetext = BounceTableText[i].bouncestrings;

		SetDlgItemText(hDlg,ID_KB_BOUNCE_TEXT,(LPSTR)lpbouncetext);
		FilterKeysParam.cbSize = sizeof(FilterKeysParam);
		SystemParametersInfo(
		    SPI_SETFILTERKEYS,
		    0, // sizeof(FilterKeysParam),
		    lp_FilterKeys_Param,
		    0);
		fMyQuestion_Save = 1;
		return(TRUE);
		break;
		}

/***************************************/

    case WM_INITDIALOG:                            /* Request to initalize        */

	FilterhWnd = hDlg;
	fMyQuestion_Save = 0;
	userpainton = userpaintoff = userpainthidden = userpaintoverlap = FALSE;

	FilterKeysParam.cbSize = sizeof(FilterKeysParam);
	SystemParametersInfo(
	    SPI_GETFILTERKEYS,
	    0, // sizeof(FilterKeysParam),
	    lp_FilterKeys_Param,
	    0);

	SavedFilterKeysParam.cbSize = sizeof(SavedFilterKeysParam);
	SystemParametersInfo(
	    SPI_GETFILTERKEYS,
	    0, // sizeof(FilterKeysParam),
	    &SavedFilterKeysParam,
	    0);

	hCtrlAcceptance = GetDlgItem(hDlg,ID_KB_ACCEPT_3);
	hCtrlDelay = GetDlgItem(hDlg,ID_KB_DELAY_3);
	hCtrlRate = GetDlgItem(hDlg,ID_KB_RATE_2);
	hCtrlBounce = GetDlgItem(hDlg,ID_KB_BOUNCE_3);
	hCtrlKybTest = GetDlgItem(hDlg,ID_KB_TEST_1);

	// for (i=0;i<2;i++) {
	//     if (OnOffTable[i] == TestFlag(FilterKeysParam.dwFlags,FKF_FILTERKEYSON) break;
	// }
	//
	// CheckRadioButton(hDlg, ID_OFF, ID_ON, ID_OFF + i );
	CheckRadioButton(hDlg, ID_OFF, ID_ON, TestFlag(FilterKeysParam.dwFlags,FKF_FILTERKEYSON) ? ID_ON : ID_OFF );

	CheckDlgButton(hDlg, ID_ON_OFF_FEEDBACK, TestFlag(FilterKeysParam.dwFlags,FKF_HOTKEYSOUND) );
	CheckDlgButton(hDlg, ID_ON_OFF_CLICK, TestFlag(FilterKeysParam.dwFlags,FKF_CLICKON) );
	CheckDlgButton(hDlg, ID_HOTKEY_ACTIVATION, TestFlag(FilterKeysParam.dwFlags,FKF_HOTKEYACTIVE) );


	for (i=0;i<7;i++) {
	    if (AcceptTable[i] == FilterKeysParam.iWaitMSec) break;
	}

	iaccepttemp = AcceptTableConv[i];
	iaccept = iaccepttemp;
	lpaccepttext = AcceptTableText[i].acceptstrings;

	for (i=0;i<4;i++) {
	    if (DelayTable[i] == FilterKeysParam.iDelayMSec) break;
	}

	idelaytemp = DelayTableConv[i];
	idelay = idelaytemp;
	lpdelaytext = DelayTableText[i].delaystrings;


	for (i=0;i<7;i++) {
	    if (RateTable[i] == FilterKeysParam.iRepeatMSec) break;
	}

	iratetemp = RateTableConv[i];
	irate = iratetemp;
	lpratetext = RateTableText[i].ratestrings;

	for (i=0;i<6;i++) {
	    if (BounceTable[i] == FilterKeysParam.iBounceMSec) break;
	}

	ibouncetemp = BounceTableConv[i];
	ibounce = ibouncetemp;
	lpbouncetext = BounceTableText[i].bouncestrings;


	SetScrollRange (hCtrlAcceptance,SB_CTL,1,7,FALSE);
	SetScrollRange (hCtrlDelay,SB_CTL,1,4,FALSE);
	SetScrollRange (hCtrlRate,SB_CTL,1,7,FALSE);
	SetScrollRange (hCtrlBounce,SB_CTL,1,6,FALSE);

	SetScrollPos(hCtrlAcceptance,SB_CTL,iaccepttemp,TRUE);
	SetScrollPos(hCtrlDelay,SB_CTL,idelaytemp,TRUE);
	SetScrollPos(hCtrlRate,SB_CTL,iratetemp,TRUE);
	SetScrollPos(hCtrlBounce,SB_CTL,ibouncetemp,TRUE);

	SetDlgItemText(hDlg,ID_KB_ACCEPT_TEXT,(LPSTR)lpaccepttext);
	SetDlgItemText(hDlg,ID_KB_DELAY_TEXT,(LPSTR)lpdelaytext);
	SetDlgItemText(hDlg,ID_KB_RATE_TEXT,(LPSTR)lpratetext);
	SetDlgItemText(hDlg,ID_KB_BOUNCE_TEXT,(LPSTR)lpbouncetext);

	if (iratetemp == 1)
	    {
		userpainthidden=TRUE;
	    }

	return(TRUE);
	break;

    default:
	break;
    }
return (FALSE);
}

/****************************************************************************

    FUNCTION:    AdjustSticKeys(HWND, unsigned, WORD, LONG)

    PURPOSE:

    COMMENTS:


****************************************************************************/

LRESULT APIENTRY AdjustSticKeys(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    STICKYKEYS        *lp_SticKeys_Param;
    static BOOL fMyQuestion_Save;

    lp_SticKeys_Param = &StickyKeysParam;

    switch (message) {
    case WM_COMMAND:
	switch (wParam) {

	case ID_OFF:
	case ID_ON:
	    CheckRadioButton(hDlg, ID_OFF, ID_ON, (DWORD)wParam);
	    SetFlag(StickyKeysParam.dwFlags,SKF_STICKYKEYSON,OnOffTable[wParam-ID_OFF]);
	    fMyQuestion_Save = 1;
	    break;

	case ID_TRISTATE:
	    SetFlag(StickyKeysParam.dwFlags,SKF_TRISTATE,IsDlgButtonChecked(hDlg, ID_TRISTATE));
	    fMyQuestion_Save = 1;
	    break;

	case ID_ON_OFF_FEEDBACK:
	    SetFlag(StickyKeysParam.dwFlags,SKF_HOTKEYSOUND,IsDlgButtonChecked(hDlg, ID_ON_OFF_FEEDBACK));
	    fMyQuestion_Save = 1;
	    break;

	case ID_HOTKEY_ACTIVATION:
	    SetFlag(StickyKeysParam.dwFlags,SKF_HOTKEYACTIVE,IsDlgButtonChecked(hDlg, ID_HOTKEY_ACTIVATION));
	    fMyQuestion_Save = 1;
	    break;

	case ID_TWOKEYS_TURNOFF:
	    SetFlag(StickyKeysParam.dwFlags,SKF_TWOKEYSOFF,IsDlgButtonChecked(hDlg, ID_TWOKEYS_TURNOFF));
	    fMyQuestion_Save = 1;
	    break;

	case ID_STATE_FEEDBACK:
	    SetFlag(StickyKeysParam.dwFlags,SKF_AUDIBLEFEEDBACK,IsDlgButtonChecked(hDlg, ID_STATE_FEEDBACK));
	    fMyQuestion_Save = 1;
	    break;

	case IDOK:
	    StickyKeysParam.cbSize = sizeof(StickyKeysParam);
	    SystemParametersInfo(
		SPI_SETSTICKYKEYS,
		0, // sizeof(StickyKeysParam),
		lp_SticKeys_Param,
		0);
	    StickyhWnd = NULL;
	    if( fMyQuestion_Save ) fQuestion_Save = 1;
	    EndDialog(hDlg, IDOK);
	    break;

	case IDCANCEL:

	    /* Let the caller know the user cancelled */
	    StickyhWnd = NULL;
	    EndDialog(hDlg, IDCANCEL);
	    break;
	default:
	    return (FALSE);
	}
	return(TRUE);
	break;

    case WM_INITDIALOG:                            /* Request to initalize        */

	StickyhWnd = hDlg;
	fMyQuestion_Save = 0;

	StickyKeysParam.cbSize = sizeof(StickyKeysParam);
	SystemParametersInfo(
	    SPI_GETSTICKYKEYS,
	    0, // sizeof(StickyKeysParam),
	    lp_SticKeys_Param,
	    0);

	// for (i=0;i<2;i++) {
	//     if (OnOffTable[i] == TestFlag(StickyKeysParam.dwFlags,SKF_STICKYKEYSON) ) break;
	// }
	// CheckRadioButton(hDlg, ID_OFF, ID_ON, ID_OFF + i);
	CheckRadioButton(hDlg, ID_OFF, ID_ON, TestFlag(StickyKeysParam.dwFlags,SKF_STICKYKEYSON) ? ID_ON : ID_OFF );

	CheckDlgButton(hDlg, ID_HOTKEY_ACTIVATION, TestFlag(StickyKeysParam.dwFlags,SKF_HOTKEYACTIVE) );
	CheckDlgButton(hDlg, ID_ON_OFF_FEEDBACK, TestFlag(StickyKeysParam.dwFlags,SKF_HOTKEYSOUND) );
	CheckDlgButton(hDlg, ID_STATE_FEEDBACK, TestFlag(StickyKeysParam.dwFlags,SKF_AUDIBLEFEEDBACK) );
	CheckDlgButton(hDlg, ID_TRISTATE, TestFlag(StickyKeysParam.dwFlags,SKF_TRISTATE) );
	CheckDlgButton(hDlg, ID_TWOKEYS_TURNOFF, TestFlag(StickyKeysParam.dwFlags,SKF_TWOKEYSOFF) );
	return(TRUE);
	break;

    default:
	break;
    }
    return (FALSE);
}

#ifdef CALCULATEDINTERNALLY
/****************************************************************************

    FUNCTION:    CalculateMouseTable()

    PURPOSE:    To set mouse table based on time to max speed and max speed.

    COMMENTS:


****************************************************************************/
void    CalculateMouseTable()
{
    long    Total_Distance;         /* in 1000th of pixel */

    long    Accel_Per_Tick;         /* in 1000th of pixel/tick */
    long    Current_Speed;          /* in 1000th of pixel/tick */
    long    Max_Speed;              /* in 1000th of pixel/tick */
    long    Real_Total_Distance;    /* in pixels */
    long    Real_Delta_Distance;    /* in pixels */
    int     i;
    int     Num_Constant_Table,Num_Accel_Table;

    Max_Speed = MouseKeysParam.iMaxSpeed;
    Max_Speed *= 1000 / 20;

    Accel_Per_Tick = Max_Speed * 1000 / (MouseKeysParam.iTimeToMaxSpeed * 20);
    Current_Speed = 0;
    Total_Distance = 0;
    Real_Total_Distance = 0;
    Num_Constant_Table = 0;
    Num_Accel_Table = 0;

    for(i=0; i<= 255; i++) {
	Current_Speed = Current_Speed + Accel_Per_Tick;
	if (Current_Speed > Max_Speed) {
	    Current_Speed = Max_Speed;
	}
	Total_Distance += Current_Speed;

	//
	// Calculate how many pixels to move on this tick
	//
	Real_Delta_Distance = ((Total_Distance - (Real_Total_Distance * 1000)) + 500) / 1000 ;
	//
	// Calculate total distance moved up to this point
	//
	Real_Total_Distance = Real_Total_Distance + Real_Delta_Distance;

	if ((Current_Speed < Max_Speed) && (Num_Accel_Table < 128)) {
	    MouseKeysParam.bAccelTable[Num_Accel_Table++] = (BYTE)Real_Delta_Distance;
	}

	if ((Current_Speed == Max_Speed) && (Num_Constant_Table < 128)) {
	    MouseKeysParam.bConstantTable[Num_Constant_Table++] = (BYTE)Real_Delta_Distance;
	}

    }
    MouseKeysParam.bAccelTableLen = (BYTE)Num_Accel_Table;
    MouseKeysParam.bConstantTableLen = (BYTE)Num_Constant_Table;
}
#endif

/****************************************************************************

    FUNCTION:    AdjustMouseKeys(HWND, unsigned, WORD, LONG)

    PURPOSE:

    COMMENTS:


****************************************************************************/

LRESULT APIENTRY AdjustMouseKeys(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    char *lptimetext;
    int        i;
    static int     ispeedtemp,itimetemp;                  /* these valuse MUST be static for scroll bars to work !!! */
    static int     ispeed,itime;                      /* these valuse MUST be static for scroll bars to work !!! */
    static BOOL fMyQuestion_Save;
    HWND    hCtrlTime,hCtrlSpeed,hMCtrl,hMCtrlID;  /* ,hCtrlText; */
    MOUSEKEYS    *lp_MouseKeys_Param;
    static MOUSEKEYS   SavedMouseKeysParam;    // saved copy for restore on Cancel

    lp_MouseKeys_Param = &MouseKeysParam;

    switch (message) {
    case WM_COMMAND:
	switch (wParam) {
	case ID_OFF:
	case ID_ON:
	    CheckRadioButton(hDlg, ID_OFF, ID_ON, (DWORD)wParam);
	    SetFlag(MouseKeysParam.dwFlags,MKF_MOUSEKEYSON,OnOffTable[wParam-ID_OFF]);
	    fMyQuestion_Save = 1;
	    MouseKeysParam.cbSize = sizeof(MouseKeysParam);
	    SystemParametersInfo(
		SPI_SETMOUSEKEYS,
		0, // sizeof(MouseKeysParam),
		lp_MouseKeys_Param,
		0);
	    break;

	case ID_ON_OFF_FEEDBACK:
	    SetFlag(MouseKeysParam.dwFlags,MKF_HOTKEYSOUND,IsDlgButtonChecked(hDlg, ID_ON_OFF_FEEDBACK));
	    fMyQuestion_Save = 1;
	    break;

	case ID_HOTKEY_ACTIVATION:
	    SetFlag(MouseKeysParam.dwFlags,MKF_HOTKEYACTIVE,IsDlgButtonChecked(hDlg, ID_HOTKEY_ACTIVATION));
	    fMyQuestion_Save = 1;
	    break;

	case IDOK:
	    MouseKeysParam.cbSize = sizeof(MouseKeysParam);
	    SystemParametersInfo(
		SPI_SETMOUSEKEYS,
		0, // sizeof(MouseKeysParam),
		lp_MouseKeys_Param,
		0);
	    MousehWnd = NULL;
	    if( fMyQuestion_Save ) fQuestion_Save = 1;
	    EndDialog(hDlg,IDOK);
	    break;

	case IDCANCEL:

/* Let the caller know the user cancelled */
/* set speed and time parameters back if they have been changed since we do on the FLY now */

	    // MousehWnd = NULL;
	    // if (ispeed != ispeedtemp)
	    // {
	    //     for (i=0;i<9;i++) {
	    //         if (SpeedTableConv[i] == ispeed) break;
	    //                 }
	    //         MouseKeysParam.iMaxSpeed = SpeedTable[i];
	    //         SystemParametersInfo(
	    //             SPI_SETMOUSEKEYS,
	    //             sizeof(MouseKeysParam),
	    //             lp_MouseKeys_Param,
	    //             0);
	    //
	    // }
	    //
	    // if (itime != itimetemp)
	    // {
	    //     for (i=0;i<9;i++) {
	    //         if (TimeTableConv[i] == itime) break;
	    //                 }
	    //         MouseKeysParam.iTimeToMaxSpeed = TimeTable[i];
	    //         SystemParametersInfo(
	    //             SPI_SETMOUSEKEYS,
	    //             sizeof(MouseKeysParam),
	    //             lp_MouseKeys_Param,
	    //             0);
	    //
	    // }
	    SavedMouseKeysParam.cbSize = sizeof(SavedMouseKeysParam);
	    i = SystemParametersInfo(               /* restore saved settings */
		SPI_SETMOUSEKEYS,
		0, // sizeof(SavedMouseKeysParam),
		&SavedMouseKeysParam,
		0);
#ifdef DEBUG
	    if( i != TRUE )
		{
		OkMsgBox( "ERROR", "Return from SET MK = %i", i );
		}
#endif
	    MouseKeysParam.cbSize = sizeof(MouseKeysParam);
	    i = SystemParametersInfo(               /* synch up our internal data struct */
		SPI_GETMOUSEKEYS,
		0, // sizeof(MouseKeysParam),
		&MouseKeysParam,
		0);
#ifdef DEBUG
	    if( i != TRUE )
		{
		OkMsgBox( "ERROR", "Return from GET MK = %i", i );
		}
#endif
	    EndDialog(hDlg,IDCANCEL);
	    break;

	default:
		return (FALSE);
	    }
    return(TRUE);
    break;


	case WM_KEYDOWN:

	    switch (wParam)
		{
		case VK_PRIOR:
		    SendMessage (hDlg,WM_HSCROLL,SB_PAGEUP,0L);
		    break;

		case VK_NEXT:
		    SendMessage (hDlg,WM_HSCROLL,SB_PAGEDOWN,0L);
		    break;

		case VK_UP:
		    SendMessage (hDlg,WM_HSCROLL,SB_LINEUP,0L);
		    break;

		case VK_DOWN:
		    SendMessage (hDlg,WM_HSCROLL,SB_LINEDOWN,0L);
		    break;

		case VK_LEFT:
		    SendMessage (hDlg,WM_HSCROLL,SB_PAGEUP,0L);
		    break;

		case VK_RIGHT:
		    SendMessage (hDlg,WM_HSCROLL,SB_PAGEDOWN,0L);
		    break;

		default:
		    return(FALSE);
		    break;

		}
	return (TRUE);
    break;


	case WM_HSCROLL:

	    hMCtrl = (HWND)lParam;
	    hMCtrlID = (HWND)GetWindowLong (hMCtrl, GWL_ID);

	    if (ID_MK_SPEED_1 == (LPARAM)hMCtrlID)
		{
		switch (LOWORD(wParam))
		    {
		case SB_LINEUP:
		    ispeedtemp = max (1,ispeedtemp -1);
		    break;

		case SB_LINEDOWN:
		    ispeedtemp =min (9,ispeedtemp +1);
		    break;

		case SB_PAGEUP:
		    ispeedtemp = max (1,ispeedtemp -3);
		    break;

		case SB_PAGEDOWN:
		    ispeedtemp = min (9,ispeedtemp +3);
		    break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		    ispeedtemp = HIWORD (wParam);
		    break;

		default:
		    return (FALSE);
		  }

		for (i=0;i<9;i++) {
		    if (SpeedTableConv[i] == ispeedtemp) break;
			    }
		    MouseKeysParam.iMaxSpeed = SpeedTable[i];

		MouseKeysParam.cbSize = sizeof(MouseKeysParam);
		SystemParametersInfo(
		    SPI_SETMOUSEKEYS,
		    0, // sizeof(MouseKeysParam),
		    lp_MouseKeys_Param,
		    0);
		SetScrollPos(hMCtrl,SB_CTL,ispeedtemp,TRUE);
		SetDlgItemInt(hDlg,ID_MK_SPEED_TEXT,MouseKeysParam.iMaxSpeed,FALSE);

/* DEBUGGING  */

/*        OkMsgBox (NULL,"MouseKeys Speed set to",
		    "iMaxSpeed = %04i",
		    ispeedtemp);
*/

		fMyQuestion_Save = 1;
		return(TRUE);
		break;
		}

	    else
		{
		switch (LOWORD(wParam))
		    {
		case SB_PAGEUP:
		    itimetemp = max (1,itimetemp-3);
		    break;

		case SB_LINEUP:
		    itimetemp = max (1,itimetemp-1);
		    break;

		case SB_PAGEDOWN:
		    itimetemp = min (9,itimetemp+3);
		    break;

		case SB_LINEDOWN:
		    itimetemp = min (9,itimetemp+1);
		    break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		    itimetemp = HIWORD (wParam);
		    break;
		default:
		    return (FALSE);
		  }

		for (i=0;i<9;i++) {
		    if (TimeTableConv[i] == itimetemp) break;
			}
		    MouseKeysParam.iTimeToMaxSpeed = TimeTable[i];

		MouseKeysParam.cbSize = sizeof(MouseKeysParam);
		SystemParametersInfo(
		    SPI_SETMOUSEKEYS,
		    0, // sizeof(MouseKeysParam),
		    lp_MouseKeys_Param,
		    0);

		SetScrollPos(hMCtrl,SB_CTL,itimetemp,TRUE);
		lptimetext = TimeTableText[i].timestrings;
		SetDlgItemText(hDlg,ID_MK_TIME_TEXT,(LPSTR)lptimetext);

/* DEBUGGING  */

/*        OkMsgBox (NULL,"MouseKeys Time Set to",
		    "iTimeToMaxSpeed = %04i",
		    itimetemp);
*/

		fMyQuestion_Save = 1;
		return(TRUE);
		break;
		}

    case WM_INITDIALOG:                            /* Request to initalize        */

	MousehWnd = hDlg;
	fMyQuestion_Save = 0;
	ispeedtemp = ispeed = itimetemp = itime = 0;

	MouseKeysParam.cbSize = sizeof(MouseKeysParam);
	i = SystemParametersInfo(
	    SPI_GETMOUSEKEYS,
	    0, // sizeof(MouseKeysParam),
	    lp_MouseKeys_Param,
	    0);
#ifdef DEBUG
	if( i != TRUE )
	    {
	    OkMsgBox( "ERROR", "Return from GET MK init = %i", i );
	    }
#endif

	SavedMouseKeysParam.cbSize = sizeof(SavedMouseKeysParam);
	i = SystemParametersInfo(
	    SPI_GETMOUSEKEYS,
	    0, // sizeof(SavedMouseKeysParam),
	    &SavedMouseKeysParam,
	    0);
#ifdef DEBUG
	if( i != TRUE )
	    {
	    OkMsgBox( "ERROR", "Return from GET MK save = %i", i );
	    }
#endif

	hCtrlTime = GetDlgItem (hDlg,ID_MK_TIME_1);
	hCtrlSpeed = GetDlgItem (hDlg,ID_MK_SPEED_1);

	// for (i=0;i<2;i++) {
	//     if (OnOffTable[i] == TestFlag(MouseKeysParam.dwFlags,MKF_MOUSEKEYSON) ) break;
	// }
	// CheckRadioButton(hDlg, ID_OFF, ID_ON, ID_OFF + i);
	CheckRadioButton(hDlg, ID_OFF, ID_ON, TestFlag(MouseKeysParam.dwFlags,MKF_MOUSEKEYSON) ? ID_ON : ID_OFF );

	CheckDlgButton(hDlg, ID_HOTKEY_ACTIVATION, TestFlag(MouseKeysParam.dwFlags,MKF_HOTKEYACTIVE) );
	CheckDlgButton(hDlg, ID_ON_OFF_FEEDBACK, TestFlag(MouseKeysParam.dwFlags,MKF_HOTKEYSOUND) );

	for (i=0;i<9;i++) {
	    if (SpeedTable[i] == MouseKeysParam.iMaxSpeed) break;
	}
	    ispeedtemp = SpeedTableConv[i];
	    ispeed = ispeedtemp;


	for (i=0;i<9;i++) {
	    if (TimeTable[i] == MouseKeysParam.iTimeToMaxSpeed) break;
	}
	    itimetemp = TimeTableConv[i];
	    itime = itimetemp;
	    lptimetext = TimeTableText[i].timestrings;

	SetScrollRange (hCtrlSpeed,SB_CTL,1,9,FALSE);
	SetScrollRange (hCtrlTime,SB_CTL,1,9,FALSE);


	SetScrollPos(hCtrlSpeed,SB_CTL,ispeedtemp,TRUE);
	SetScrollPos(hCtrlTime,SB_CTL,itimetemp,TRUE);

	SetDlgItemInt(hDlg,ID_MK_SPEED_TEXT,MouseKeysParam.iMaxSpeed,FALSE);
	SetDlgItemText(hDlg,ID_MK_TIME_TEXT,(LPSTR)lptimetext);


/* DEBUGGING  */

/*        OkMsgBox (NULL,"MouseKeys Initalized at",
		    "iMaxSpeed = %04i, iTimeToMaxSpeed = %04i",
		    ispeedtemp,itimetemp);
*/
	return (TRUE);
	break;

    }
    return(FALSE);
}

/****************************************************************************

    FUNCTION:    AdjustToggleKeys(HWND, unsigned, WORD, LONG)

    PURPOSE:

    COMMENTS:


****************************************************************************/

LRESULT APIENTRY AdjustToggleKeys(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    TOGGLEKEYS    *lp_ToggleKeys_Param;
    static BOOL fMyQuestion_Save;

    lp_ToggleKeys_Param = &ToggleKeysParam;

    switch (message) {
    case WM_COMMAND:
	switch (wParam) {

	case ID_OFF:
	case ID_ON:
	    CheckRadioButton(hDlg, ID_OFF, ID_ON, (DWORD)wParam);
	    SetFlag(ToggleKeysParam.dwFlags,TKF_TOGGLEKEYSON,OnOffTable[wParam-ID_OFF]);
	    fMyQuestion_Save = 1;
	    break;

	case IDOK:
	    ToggleKeysParam.cbSize = sizeof(ToggleKeysParam);
	    SystemParametersInfo(
		SPI_SETTOGGLEKEYS,
		0, // sizeof(ToggleKeysParam),
		lp_ToggleKeys_Param,
		0);
	    TogglehWnd = NULL;
	    if( fMyQuestion_Save ) fQuestion_Save = 1;
	    EndDialog(hDlg, IDOK);
	    break;

	case ID_HOTKEY_ACTIVATION:
	    SetFlag(ToggleKeysParam.dwFlags,TKF_HOTKEYACTIVE,IsDlgButtonChecked(hDlg, ID_HOTKEY_ACTIVATION));
	    fMyQuestion_Save = 1;
	    break;

	case ID_ON_OFF_FEEDBACK:
	    SetFlag(ToggleKeysParam.dwFlags,TKF_HOTKEYSOUND,IsDlgButtonChecked(hDlg, ID_ON_OFF_FEEDBACK));
	    fMyQuestion_Save = 1;
	    break;

	case IDCANCEL:

	    /* Let the caller know the user cancelled */
	    TogglehWnd = NULL;
	    EndDialog(hDlg, IDCANCEL);
	    break;

	default:
	    return (FALSE);
	    break;
	}
	return (TRUE);
	break;

    case WM_INITDIALOG:                        /* Request to initalize        */

	TogglehWnd = hDlg;
	fMyQuestion_Save = 0;

	ToggleKeysParam.cbSize = sizeof(ToggleKeysParam);
	SystemParametersInfo(
	    SPI_GETTOGGLEKEYS,
	    0, // sizeof(ToggleKeysParam),
	    lp_ToggleKeys_Param,
	    0);

	// for (i=0;i<2;i++) {
	//     if (OnOffTable[i] == TestFlag(ToggleKeysParam.dwFlags,TKF_TOGGLEKEYSON) ) break;
	// }
	// CheckRadioButton(hDlg, ID_OFF, ID_ON, ID_OFF + i);
	CheckRadioButton(hDlg, ID_OFF, ID_ON, TestFlag(ToggleKeysParam.dwFlags,TKF_TOGGLEKEYSON) ? ID_ON : ID_OFF );

	CheckDlgButton(hDlg, ID_HOTKEY_ACTIVATION, TestFlag(ToggleKeysParam.dwFlags,TKF_HOTKEYACTIVE) );
	CheckDlgButton(hDlg, ID_ON_OFF_FEEDBACK, TestFlag(ToggleKeysParam.dwFlags,TKF_HOTKEYSOUND) );

	return(TRUE);
	break;

    default:
	break;
    }
    return (FALSE);
}

/****************************************************************************

    FUNCTION:    AdjustTimeOut(HWND, unsigned, WORD, LONG)

    PURPOSE:

    COMMENTS:


****************************************************************************/

LRESULT APIENTRY AdjustTimeOut(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    ACCESSTIMEOUT        *lp_TimeOut_Param;
    int        i;
    static BOOL fMyQuestion_Save;

    lp_TimeOut_Param = &TimeOutParam;

    switch (message) {
    case WM_COMMAND:
	switch (wParam) {

	case ID_OFF:
	case ID_ON:
	    CheckRadioButton(hDlg, ID_OFF, ID_ON, (int)wParam);
	    SetFlag(TimeOutParam.dwFlags,ATF_TIMEOUTON,OnOffTable[wParam-ID_OFF]);
	    fMyQuestion_Save = 1;
	    break;

	case ID_ON_OFF_FEEDBACK:
	    SetFlag(TimeOutParam.dwFlags,ATF_ONOFFFEEDBACK,IsDlgButtonChecked(hDlg, ID_ON_OFF_FEEDBACK));
	    fMyQuestion_Save = 1;
	    break;

	case ID_TIMEOUT_1:
	case ID_TIMEOUT_2:
	case ID_TIMEOUT_3:
	case ID_TIMEOUT_4:
	    CheckRadioButton(hDlg, ID_TIMEOUT_1, ID_TIMEOUT_4, (int)wParam);
	    TimeOutParam.iTimeOutMSec = TimeOutTable[wParam-ID_TIMEOUT_1];
	    fMyQuestion_Save = 1;
	    break;

	case IDOK:
	    TimeOutParam.cbSize = sizeof(TimeOutParam);
	    SystemParametersInfo(
		SPI_SETACCESSTIMEOUT,
		0, // sizeof(TimeOutParam),
		lp_TimeOut_Param,
		0);
	    TimeOuthWnd = NULL;
	    if( fMyQuestion_Save ) fQuestion_Save = 1;
	    EndDialog(hDlg, IDOK);
	    break;

	case IDCANCEL:

	    /* Let the caller know the user cancelled */
	    TimeOuthWnd = NULL;
	    EndDialog(hDlg, IDCANCEL);
	    break;
	default:
	    return (FALSE);
	}
	return(TRUE);
	break;

    case WM_INITDIALOG:                            /* Request to initalize        */

	TimeOuthWnd = hDlg;
	fMyQuestion_Save = 0;

	TimeOutParam.cbSize = sizeof(TimeOutParam);
	SystemParametersInfo(
	    SPI_GETACCESSTIMEOUT,
	    0, // sizeof(TimeOutParam),
	    lp_TimeOut_Param,
	    0);

	// for (i=0;i<2;i++) {
	//     if (OnOffTable[i] == TestFlag(TimeOutParam.dwFlags,ATF_TIMEOUTON) ) break;
	// }
	// CheckRadioButton(hDlg, ID_OFF, ID_ON, ID_OFF + i);
	CheckRadioButton(hDlg, ID_OFF, ID_ON, TestFlag(TimeOutParam.dwFlags,ATF_TIMEOUTON) ? ID_ON : ID_OFF );

	CheckDlgButton(hDlg, ID_ON_OFF_FEEDBACK, TestFlag(TimeOutParam.dwFlags,ATF_ONOFFFEEDBACK) );

	for (i=0;i<4;i++) {
	    if (TimeOutTable[i] == TimeOutParam.iTimeOutMSec) break;
	}
	CheckRadioButton(hDlg, ID_TIMEOUT_1, ID_TIMEOUT_4, ID_TIMEOUT_1 + i);

	return(TRUE);
	break;

    default:
	break;
    }
    return (FALSE);
}

/****************************************************************************

    FUNCTION:    AdjustSerialKeys(HWND, unsigned, WORD, LONG)

    PURPOSE:

    COMMENTS:


****************************************************************************/

LRESULT APIENTRY AdjustSerialKeys(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{

	int    iPortButton;
	int    iBaudButton;
   static BOOL fMyQuestion_Save;
	char   sbuf1[255];

#ifdef MYSK
    MYSERIALKEYS    *lp_serialkeys;
    lp_serialkeys    = &MySerialKeysParam;
#endif

	switch (message)
	{
		//-------------------------------------------------------------
		case WM_COMMAND:
			switch (wParam) 
			{

				//------------------------------------------------------
				case IDC_INST_REMOVE:
					if (!SerialKeysEnabled)
						SerialKeysInstall(hDlg);
					else
						SerialKeysRemove(hDlg);

					SetDialogItems(hDlg);
					break;

				//---------------------------------------------------------
				case ID_OFF:
				case ID_ON:
					CheckRadioButton(hDlg, ID_OFF, ID_ON, (int)wParam);
					bSK_CommOpen = (BOOL)wParam;
		   fMyQuestion_Save = 1;
					break;

				//---------------------------------------------------------
				case ID_BAUD_300:
				case ID_BAUD_600:
				case ID_BAUD_1200:
				case ID_BAUD_2400:
				case ID_BAUD_4800:
				case ID_BAUD_9600:
				case ID_BAUD_19200:
					CheckRadioButton(hDlg, ID_BAUD_300, ID_BAUD_19200,  
                                     (int)wParam);
					iSK_BaudRate = (int)wParam;
		   fMyQuestion_Save = 1;
					break;

				//---------------------------------------------------------
				case ID_COM1:
				case ID_COM2:
				case ID_COM3:
				case ID_COM4:
					CheckRadioButton(hDlg, ID_COM1, ID_COM4, (int)wParam);
					iSK_ComName = (int)wParam;
		   fMyQuestion_Save = 1;
					break;

				//---------------------------------------------------------
				case IDOK:

					if (!SerialKeysEnabled)         // Is Serial Keys Installed?
					{
						EndDialog(hDlg, IDOK);          // No - Remove Dialog
						break;
					}

#ifdef MYSK
					MySerialKeysParam.fSerialKeysOn = bSK_CommOpen;
					MySerialKeysParam.iBaudRate     = iSK_BaudRate - ID_BAUD_300;
					MySerialKeysParam.iComName      = iSK_ComName - ID_COM1;
		   SystemParametersInfo(SPI_SETSERIALKEYS,sizeof(MySerialKeysParam),lp_serialkeys,0);

#else
					switch( iSK_ComName )
					{
						case ID_COM1:
							strcpy( SK_ActivePort, "COM1" );
							break;
						case ID_COM2:
							strcpy( SK_ActivePort, "COM2" );
							break;
						case ID_COM3:
							strcpy( SK_ActivePort, "COM3" );
							break;
						case ID_COM4:
							strcpy( SK_ActivePort, "COM4" );
							break;
						default:
                                                     // OkMsgBox( "Access Utility", "Bad SerialKeys Port Name" );
                                                        OkFiltersMessage( hDlg, IDS_BAD_SK_PORT );
							break;
					}

					switch( iSK_BaudRate )
					{
						case ID_BAUD_300:
							SerialKeysParam.iBaudRate = CBR_300;
							break;
						case ID_BAUD_600:
							SerialKeysParam.iBaudRate = CBR_600;
							break;
						case ID_BAUD_1200:
							SerialKeysParam.iBaudRate = CBR_1200;
							break;
						case ID_BAUD_2400:
							SerialKeysParam.iBaudRate = CBR_2400;
							break;
						case ID_BAUD_4800:
							SerialKeysParam.iBaudRate = CBR_4800;
							break;
						case ID_BAUD_9600:
							SerialKeysParam.iBaudRate = CBR_9600;
							break;
						case ID_BAUD_19200:
							SerialKeysParam.iBaudRate = CBR_19200;
							break;
						default:
                                                     // OkMsgBox( "Access Utility", "Bad SerialKeys Baud Rate" );
                                                        OkFiltersMessage( hDlg, IDS_BAD_SK_BAUD );
							break;
					}

					SerialKeysParam.dwFlags = SERKF_AVAILABLE;
					if (bSK_CommOpen == ID_ON)
						SerialKeysParam.dwFlags += SERKF_SERIALKEYSON;

					SKEY_SystemParametersInfo
					(
						SPI_SETSERIALKEYS,
						sizeof(SerialKeysParam),
						&SerialKeysParam,0
					);

					bSK_CommOpen = (SerialKeysParam.dwFlags & SERKF_SERIALKEYSON) ? ID_ON : ID_OFF;
#endif

					if ( fMyQuestion_Save ) 
						fQuestion_Save = 1;

			    EndDialog(hDlg, IDOK);
		break;
				
				//---------------------------------------------------------
				case IDCANCEL:
				
					/* Let the caller know the user cancelled */
					SerialhWnd = NULL;
					EndDialog(hDlg, IDCANCEL);
					break;

				//---------------------------------------------------------
			default:
					return (FALSE);
			}
			return (TRUE);

		//-------------------------------------------------------------
		case WM_INITDIALOG:                        /* Request to initalize        */

			SerialhWnd = hDlg;
			fMyQuestion_Save = 0;

			// serialkeys params already filled in earlier....or were they?
			// before we save, check to see if AAC or framing errors changed baudrate
			// and we havenot caught it for display or saving purposes as yet 
			// we don't do this now, should we?  doesn't vxd fill in
			// current values for us automatically?

#ifdef MYSK
			iPortButton = MySerialKeysParam.iComName + ID_COM1;
			if( MySerialKeysParam.iComName < 0 || MySerialKeysParam.iComName > 3 )
			   {
			//  OkMsgBox( "Access Utility", "Bad SerialKeys port number %i",
			//      MySerialKeysParam.iComName );
			   MySerialKeysParam.iComName = 0;
			   }
			if( MySerialKeysParam.iBaudRate < 0 || MySerialKeysParam.iBaudRate > 5 )
			   {
			//  OkMsgBox( "Access Utility", "Bad SerialKeys baud rate option %i",
			//      MySerialKeysParam.iBaudRate );
			   MySerialKeysParam.iBaudRate = 0;
			   }
			CheckRadioButton(hDlg, ID_COM1, ID_COM4, ID_COM1 + MySerialKeysParam.iComName );
			CheckRadioButton(hDlg, ID_BAUD_300, ID_BAUD_9600, ID_BAUD_300 + MySerialKeysParam.iBaudRate );
			if( ! MySerialKeysParam.fSerialKeysOn )
			   CheckRadioButton(hDlg, ID_OFF, ID_ON, ID_OFF);
			else
			   CheckRadioButton(hDlg, ID_OFF, ID_ON, ID_ON);
#else

			//
			// Check if the Serial Keys Service is installed
			SerialKeysEnabled = SKEY_SystemParametersInfo   // Get Current Values
				(
					SPI_GETSERIALKEYS,sizeof(SerialKeysParam),
					&SerialKeysParam,0
				);

			if (SerialKeysEnabled)
			{
			    LoadString (hInst,IDS_SK_REMOVE,(LPSTR)sbuf1,245);
	       if( lstrlen( SK_ActivePort ) == 0 )
					lstrcpy( SK_ActivePort, "COM1" );
				
				iPortButton = ID_COM1 + (SK_ActivePort[3] - '1');
			}
			else
			{
			    LoadString (hInst,IDS_SK_INSTALL,(LPSTR)sbuf1,245);
				iPortButton = ID_COM1;                                  // Set Default Com Port
				SerialKeysParam.iBaudRate = CBR_300;    // Set Default
				SerialKeysParam.dwFlags = 0;
			}               

			SetDlgItemText(hDlg, IDC_INST_REMOVE,sbuf1);
			SetDialogItems(hDlg);

			switch( SerialKeysParam.iBaudRate )
			{
		    case CBR_300:
		  iBaudButton = ID_BAUD_300;
		       break;

		    case CBR_600:
		       iBaudButton = ID_BAUD_600;
		       break;

		    case CBR_1200:
		       iBaudButton = ID_BAUD_1200;
		       break;

		    case CBR_2400:
		       iBaudButton = ID_BAUD_2400;
		       break;

		    case CBR_4800:
		       iBaudButton = ID_BAUD_4800;
		       break;

		    case CBR_9600:
		       iBaudButton = ID_BAUD_9600;
		       break;

		    case CBR_19200:
		       iBaudButton = ID_BAUD_19200;
		       break;

		    case 0:
					iBaudButton = ID_BAUD_300;
					break;

		    default:
		       OkMsgBox( "Access Utility", "Bad SerialKeys baud rate %i",
			   SerialKeysParam.iBaudRate );
		       iBaudButton = ID_BAUD_300;
		       break;
			}

			// Set Default Values
			iSK_BaudRate    = iBaudButton;
			iSK_ComName             = iPortButton;
			bSK_CommOpen    = (SerialKeysParam.dwFlags & SERKF_SERIALKEYSON) ? ID_ON : ID_OFF;

			CheckRadioButton(hDlg, ID_OFF, ID_ON, bSK_CommOpen);
			CheckRadioButton(hDlg, ID_COM1,ID_COM4, iPortButton);
			CheckRadioButton(hDlg, ID_BAUD_300, ID_BAUD_19200, iBaudButton);
#endif
			return(TRUE);

		//-------------------------------------------------------------
		default:
			break;
    }
    return (FALSE);
}


/****************************************************************************/
void SerialKeysInstall(HWND hDlg)
{
   char sbuf1[255];

	SERVICE_STATUS  ssStatus;
	DWORD   dwOldCheckPoint;
	DWORD   LastError;
	int             ErrCode;
	char    FileName[255];

	LPCTSTR lpszBinaryPathName = TEXT("%SystemRoot%\\system32\\skeys.exe");

	SC_HANDLE   schService = NULL;
	SC_HANDLE   schSCManager = NULL;

	GetWindowsDirectory(FileName,sizeof(FileName));
	strcat(FileName,"\\system32\\skeys.exe");
	//lpszBinaryPathName = 

	if (GetFileAttributes(FileName) == 0xFFFFFFFF)        // Is Service File installed?
	{       
		SKeysError(hDlg, IDS_SK_ERR_FAILFIND);  // No - Display Error & Exit
		return;
	}

	schSCManager = OpenSCManager            // Open Service Manager
		(         
		NULL,                           // machine (NULL == local)
	    NULL,                       // database (NULL == default)
		SC_MANAGER_ALL_ACCESS           // access required
	);

	if (schSCManager == NULL)                                       // Did Open Service Fail?
	{
		LastError = GetLastError();                             // Yes - Get Error Code

		if (LastError == ERROR_ACCESS_DENIED)           // Did we have correct privledge?
			SKeysError(hDlg, IDS_SK_ERR_ACCESS);    // No - Disp error
		else
			SKeysError(hDlg, IDS_SK_ERR_FAILSCM);   // Yes - Misc Error

		return;
	}


	schService = CreateService(
	    schSCManager,               // SCManager database
	    TEXT("SerialKeys"),                 // name of service
	    TEXT("SerialKeys"),                 // name to display (new parameter after october beta)
	    SERVICE_ALL_ACCESS,         // desired access
	    SERVICE_WIN32_OWN_PROCESS,  // service type
	    SERVICE_AUTO_START,             // start type
	    SERVICE_ERROR_NORMAL,       // error control type
	    FileName,                           // service's binary
	    NULL,                       // no load ordering group
	    NULL,                       // no tag identifier
	    NULL,                       // no dependencies
	    NULL,                       // LocalSystem account
	    NULL);                      // no password

	ErrCode = IDS_SK_ERR_CREATE;
	if (schService == NULL)                 // Was Service Created?
		goto ErrExit;                           // No - Exit 

	CloseServiceHandle(schService);
	schService = OpenService
		(
			schSCManager ,
			"SerialKeys",
			SERVICE_ALL_ACCESS
		);

	ErrCode = IDS_SK_ERR_START;
	if (!StartService(schService,0,NULL))
		goto ErrExit;
		
	QueryServiceStatus(schService,&ssStatus);

	while (ssStatus.dwCurrentState != SERVICE_RUNNING)
	{
		dwOldCheckPoint = ssStatus.dwCheckPoint;
		Sleep(ssStatus.dwWaitHint);
		if (!QueryServiceStatus(schService,&ssStatus))
			break;

		if (dwOldCheckPoint >= ssStatus.dwCheckPoint)
			break;
	}

	if (ssStatus.dwCurrentState == SERVICE_RUNNING)
	{
		SerialKeysEnabled = TRUE;
		LoadString (hInst,IDS_SK_REMOVE,(LPSTR)sbuf1,245);
		SetDlgItemText(hDlg, IDC_INST_REMOVE,sbuf1);
		goto Exit;
	}

ErrExit:
	DeleteService(schService);
	SKeysError(hDlg, ErrCode);

Exit:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return;
}

/****************************************************************************/
void SerialKeysRemove(HWND hDlg)
{
   char sbuf1[255];

	SC_HANDLE   schService = NULL;
	SC_HANDLE   schSCManager = NULL;
	SERVICE_STATUS  ssStatus;
	DWORD   dwOldCheckPoint;
	DWORD   LastError;
	
	// Open Service Control Manager
	schSCManager = OpenSCManager
		(
		NULL,                   // machine (NULL == local)
	    NULL,                   // database (NULL == default)
		SC_MANAGER_ALL_ACCESS   // access required
	);

	if (schSCManager == NULL)               // Was Open successful?
	{
		LastError = GetLastError();                             // Yes - Get Error Code

		if (LastError == ERROR_ACCESS_DENIED)           // Did we have correct privledge?
			SKeysError(hDlg, IDS_SK_ERR_REMOVE);    // No - Disp error
		else
			SKeysError(hDlg, IDS_SK_ERR_FAILSCM);   // Yes - Misc Error

		return;                                         // No - Exit
	}

	schService = OpenService                // Open Service
		(
		schSCManager, 
		"SerialKeys", 
		SERVICE_ALL_ACCESS
	);

	if (schService == NULL)                 // Did Open Fail?
	{
		SKeysError(hDlg, IDS_SK_ERR_OPEN);
		goto Exit;
	}

	ControlService(schService,SERVICE_CONTROL_STOP,&ssStatus);      // Stop Service
	QueryServiceStatus(schService,&ssStatus);                                       // Get Status

	//
	// Wait until the service is stopped.
	while (ssStatus.dwCurrentState != SERVICE_STOPPED)                      // Wait until Service Stops
	{
		dwOldCheckPoint = ssStatus.dwCheckPoint;
		Sleep(ssStatus.dwWaitHint);
		if (!QueryServiceStatus(schService,&ssStatus))
			break;

		if (dwOldCheckPoint >= ssStatus.dwCheckPoint)
			break;
	}

	//
	// Stop the Service
	if (ssStatus.dwCurrentState != SERVICE_STOPPED)
	{
		SKeysError(hDlg, IDS_SK_ERR_STOP);
		goto Exit;
	}

	//
	// Delete the Service
	if (!DeleteService(schService))
	{
		SKeysError(hDlg, IDS_SK_ERR_DELETE);
		goto Exit;
	} 

	SerialKeysEnabled = FALSE;
	LoadString (hInst,IDS_SK_INSTALL,(LPSTR)sbuf1,245);
	SetDlgItemText(hDlg, IDC_INST_REMOVE,sbuf1);

Exit:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return;
}       

/****************************************************************************/
void SKeysError(HWND hDlg, int Error)
{
   char sbuf1[255];
   char sbuf2[255];

	LoadString (hInst,IDS_SK_TITLE,(LPSTR)sbuf1,245);
	LoadString (hInst,Error,(LPSTR)sbuf2,245);
	MessageBox (hDlg, (LPSTR)sbuf2,(LPSTR)sbuf1, MB_OK|MB_ICONHAND);
}


/****************************************************************************/
// Enable or Disable Dialog Controlls
void SetDialogItems(HWND hDlg)
{
	EnableWindow(GetDlgItem(hDlg,ID_OFF),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_ON ),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_BAUD_300),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_BAUD_600),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_BAUD_1200),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_BAUD_2400),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_BAUD_4800),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_BAUD_9600),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_BAUD_19200),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_COM1),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_COM2),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_COM3),SerialKeysEnabled);
	EnableWindow(GetDlgItem(hDlg,ID_COM4),SerialKeysEnabled);

}


/****************************************************************************/
BOOL LoadLBStrings(hwndLB, pSSInfo, cEffects)
HWND hwndLB;
PSSINFO pSSInfo;
UINT cEffects;
{
    UINT i, index;

    for (i = 0; i < cEffects; i++) {
	LoadString(hInst,
		   pSSInfo[i].idResource,
		   (LPTSTR)pSSInfo[i].szEffect,
		   MAXEFFECTSTRING
		   );
    index = (UINT)SendMessage(hwndLB,
                              CB_ADDSTRING,
                              0,
                              (LPARAM)(LPTSTR)pSSInfo[i].szEffect
                             );
	if (index == CB_ERR || index == CB_ERRSPACE) {
	    return FALSE;
	}
	pSSInfo[i].idDialog = index;
    }
    return TRUE;
}

/****************************************************************************

    FUNCTION:    AdjustSoundSentry(HWND, unsigned, WORD, LONG)

    PURPOSE:

    COMMENTS:


****************************************************************************/

LRESULT APIENTRY AdjustSoundSentry(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    SOUNDSENTRY *lp_SoundSentry_Param;
    char   *lpReturnType;
    LRESULT i;
    static BOOL fMyQuestion_Save;

    lp_SoundSentry_Param = &SoundSentryParam;
    lpReturnType = &lpReturn;
    switch (message) {
    case WM_COMMAND:
	switch (LOWORD(wParam)) {

	case ID_OFF:
	case ID_ON:
	    CheckRadioButton(hDlg, ID_OFF, ID_ON, LOWORD(wParam));
	    SetFlag(SoundSentryParam.dwFlags,SSF_SOUNDSENTRYON,OnOffTable[wParam-ID_OFF]);
	    fMyQuestion_Save = 1;
	    break;
	case ID_SS_WINDOWS:
	    if (HIWORD(wParam) == CBN_SELCHANGE) {
		i = SendDlgItemMessage(hDlg, ID_SS_WINDOWS, CB_GETCURSEL, 0, 0);
		SoundSentryParam.iWindowsEffect = aSSWin[i].iEffectFlag;
		fMyQuestion_Save =1;
	    }
	    return FALSE;
	    break;

	//
	// Full screen text and graphics modes are only available on the
	// x86 platform.
	//
	case ID_SS_TEXTMODE:
	    if (HIWORD((DWORD)wParam) == CBN_SELCHANGE) {
		i = SendDlgItemMessage(hDlg, ID_SS_TEXTMODE, CB_GETCURSEL, 0, 0);
		SoundSentryParam.iFSTextEffect = aSSText[i].iEffectFlag;
		fMyQuestion_Save =1;
	    }
	    return FALSE;
	    break;

	case ID_SS_GRAPHICSMODE:
	    if (HIWORD((DWORD)wParam) == CBN_SELCHANGE) {
		i = SendDlgItemMessage(hDlg, ID_SS_GRAPHICSMODE, CB_GETCURSEL, 0, 0);
		SoundSentryParam.iFSGrafEffect = aSSGraphics[i].iEffectFlag;
		fMyQuestion_Save =1;
	    }
	    return FALSE;
	    break;

	case IDOK:
	    SoundSentryParam.cbSize = sizeof(SoundSentryParam);
	    SystemParametersInfo(
		SPI_SETSOUNDSENTRY,
		0, // sizeof(SoundSentryParam),
		lp_SoundSentry_Param,
		0);
	    SoundSentryhWnd = NULL;
	    if( fMyQuestion_Save ) fQuestion_Save = 1;
	    EndDialog(hDlg, IDCANCEL);
	    break;

	case IDCANCEL:
	    SoundSentryhWnd = NULL;
	    EndDialog(hDlg, IDCANCEL);
	    break;
	default:
	    return (FALSE);
	}
	return(TRUE);
	break;

    case WM_INITDIALOG:                            /* Request to initalize        */

	SoundSentryhWnd = hDlg;
	fMyQuestion_Save = 0;

	SoundSentryParam.cbSize = sizeof(SoundSentryParam);
	SystemParametersInfo(
	    SPI_GETSOUNDSENTRY,
	    0, // sizeof(SoundSentryParam),
	    lp_SoundSentry_Param,
	    0);

	// for (i=0;i<2;i++) {
	//     if (OnOffTable[i] == TestFlag(SoundSentryParam.dwFlags,SSF_SOUNDSENTRYON) ) break;
	// }
	// CheckRadioButton(hDlg, ID_OFF, ID_ON, ID_OFF + i);
	CheckRadioButton(hDlg, ID_OFF, ID_ON, TestFlag(SoundSentryParam.dwFlags,SSF_SOUNDSENTRYON) ? ID_ON : ID_OFF );

	hSSListBox = GetDlgItem(hDlg, ID_SS_WINDOWS);
	if (!LoadLBStrings(hSSListBox, &aSSWin[0], cWindowsEffects)) {
	    EndDialog(hDlg, IDCANCEL);
	    return FALSE;
	}
	for (i = 0; i < cWindowsEffects; i++) {
	    if (aSSWin[i].iEffectFlag == SoundSentryParam.iWindowsEffect) {
		SendMessage(hSSListBox, CB_SETCURSEL, aSSWin[i].idDialog, 0);
		break;
	    }
	}

	//
	// Full screen text and graphics modes are only available on the
	// x86 platform.
	//
	hSSListBox = GetDlgItem(hDlg, ID_SS_TEXTMODE);
	if (!LoadLBStrings(hSSListBox, &aSSText[0], cTextEffects)) {
	    EndDialog(hDlg, IDCANCEL);
	    return FALSE;
	}
	for (i = 0; i < cTextEffects; i++) {
	    if (aSSText[i].iEffectFlag == SoundSentryParam.iFSTextEffect) {
		SendMessage(hSSListBox, CB_SETCURSEL, aSSText[i].idDialog, 0);
		break;
	    }
	}

	hSSListBox = GetDlgItem(hDlg, ID_SS_GRAPHICSMODE);
	if (!LoadLBStrings(hSSListBox, &aSSGraphics[0], cGraphicEffects)) {
	    EndDialog(hDlg, IDCANCEL);
	    return FALSE;
	}
	for (i = 0; i < cGraphicEffects; i++) {
	    if (aSSGraphics[i].iEffectFlag == SoundSentryParam.iFSGrafEffect) {
		SendMessage(hSSListBox, CB_SETCURSEL, aSSGraphics[i].idDialog, 0);
		break;
	    }
	}

	return(TRUE);
	    break;


    default:
	break;

    }
    return (FALSE);
}

/*-------------------------------------------------------------------------*/
/*                                                                         */
/*    OkMsgBox( szCaption, szFormat )                                      */
/*                                                                         */
/*-------------------------------------------------------------------------*/

void OkMsgBox( LPSTR szCaption, LPSTR szFormat,... )
    {
    char szBuffer[256];
    va_list vaList;


    //
    // Use va_start, va_end to manage vaList
    //
    va_start(vaList, szFormat);
    wvsprintf( szBuffer, szFormat, vaList );
    va_end(vaList);

    MessageBox( NULL, (LPSTR)szBuffer, (LPSTR)szCaption, MB_OK );
    }

void AssertBool( LPSTR sz, LPBOOL lpf )
    {
    if( *lpf != 0 && *lpf != 1 )
	{
// #ifdef DEBUG
	OkMsgBox( "Access Utility",
		  "Assertion Failed: boolean variable '%s' has value %X",
		  sz, *lpf );
// #endif
	*lpf = 1;
	}
    }
