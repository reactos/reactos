/*
 * mouse - Dialog box property sheet for "mouse ui tweaks"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

KL const c_klDelay = { &c_hkCU, c_tszRegPathDesktop, c_tszDelay };

const char CODESEG c_szUser[] = "USER";		/* Must be ANSI */

const static DWORD CODESEG rgdwHelp[] = {
	IDC_SPEEDTEXT,	    IDH_SPEEDHELP,
	IDC_SPEEDFAST,	    IDH_SPEEDHELP,
	IDC_SPEEDSLOW,	    IDH_SPEEDHELP,
	IDC_SPEEDTRACK,	    IDH_SPEEDHELP,
	IDC_SPEEDHELP,	    IDH_SPEEDHELP,

	IDC_SENSGROUP,	    IDH_GROUP,

	IDC_DBLCLKTEXT,	    IDH_DBLCLK,
	IDC_DBLCLK,	    IDH_DBLCLK,
	IDC_DBLCLKUD,	    IDH_DBLCLK,

	IDC_DRAGTEXT,	    IDH_DRAG,
	IDC_DRAG,	    IDH_DRAG,
	IDC_DRAGUD,	    IDH_DRAG,

	IDC_SENSHELP,	    IDH_GROUP,

	IDC_EFFECTGROUP,    IDH_GROUP,
	IDC_BEEP,	    IDH_BEEP,
	IDC_XMOUSE,	    IDH_XMOUSE,

/* BUGBUG -- hover */

	IDC_WHEELGROUP,     IDH_GROUP,
	IDC_WHEELENABLE,    IDH_WHEEL,
	IDC_WHEELPAGE,      IDH_WHEEL,
	IDC_WHEELLINE,      IDH_WHEEL,
	IDC_WHEELLINENO,    IDH_WHEEL,

	IDC_TESTGROUP,	    IDH_TEST,
	IDC_TEST,	    IDH_TEST,

	IDC_TIPS,	    IDH_TIPSTIP,
	IDC_RESET,	    IDH_RESET,

	0,		    0,
};

#pragma END_CONST_DATA

typedef WORD DT, FAR *LPDT;	/* typeof(dtMNDropDown) */

#ifdef WIN32
#define fLpdt (lpdt != &dtScratch)
#else
#define fLpdt (SELECTOROF(lpdt) != SELECTOROF((LPDT)&dtScratch))
#endif

/*
 * Globals
 */
DT dtScratch;		/* Point lpdt here if we are stuck */
DT dtNT;		/* Point lpdt here if we are on NT */

LPDT lpdt;		/* Where to tweak to adjust the actual dt */

/*
 * Instanced.  We're a cpl so have only one instance, but I declare
 * all the instance stuff in one place so it's easy to convert this
 * code to multiple-instance if ever we need to.
 */
typedef struct MDII {		/* Mouse_dialog instance info */
    BOOL fDrag;			/* Potential drag in progress? */
    DT dtOrig;			/* Original dt when we started */
    RECT rcTest;		/* Test area */
    RECT rcDrag;		/* Drag test rectangle */
    RECT rcDblClk;		/* Double click rectangle */
    LONG tmClick;		/* Time of previous lbutton down */
    HCURSOR hcurDrag;		/* What is being dragged? */
    BOOL fFactory;		/* Factory defaults? */
    POINT ptDblClk;		/* Double click values pending */
    POINT ptDrag;		/* Drag values pending */
    int cxAspect;		/* Screen aspect ratio */
    int cyAspect;		/* Screen aspect ratio */
    int idi;			/* Which icon to use? */
} MDII, *PMDII;

MDII mdii;
#define pmdii (&mdii)

#define DestroyCursor(hcur) SafeDestroyIcon((HICON)(hcur))

/*****************************************************************************
 *
 *  Grovelling to find the dropmenu variable.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  dtDefault
 *
 *	Return the default dropmenu time, which is DoubleClickTime * 4 / 5.
 *
 *****************************************************************************/

DT PASCAL
dtDefault(void)
{
    return GetDoubleClickTime() * 4 / 5;
}

/*****************************************************************************
 *
 *  dtCur
 *
 *	Determine what the dropmenu time is, by external means.
 *
 *	It ought to be DoubleClickTime * 4 / 5, or the value in the registry.
 *
 *****************************************************************************/

INLINE DT
dtCur(void)
{
    return GetIntPkl(dtDefault(), &c_klDelay);
}

/*****************************************************************************
 *
 *  GetProcOrd
 *
 *  Win95 does not allow GetProcAddress to work on Kernel32, so we must
 *  implement it by hand.
 *
 *****************************************************************************/

/*
 * winnt.h uses these totally screwed up structure names.
 * Does anybody speak Hungarian over there?
 */
typedef IMAGE_DOS_HEADER IDH, *PIDH;
typedef IMAGE_NT_HEADERS NTH, *PINTH; /* I like how this is "HEADERS" plural */
typedef IMAGE_EXPORT_DIRECTORY EDT, *PEDT;
typedef DWORD EAT, *PEAT;
typedef IMAGE_DATA_DIRECTORY OTE, *POTE;

#define pvAdd(pv, cb) ((LPVOID)((LPSTR)(pv) + (DWORD)(cb)))
#define pvSub(pv1, pv2) (DWORD)((LPSTR)(pv1) - (LPSTR)(pv2))

#define poteExp(pinth) (&(pinth)->OptionalHeader. \
			  DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT])

FARPROC PASCAL
GetProcOrd(LPVOID lpv, UINT ord)
{
    PIDH pidh = lpv;
    if (!IsBadReadPtr(pidh, sizeof(*pidh)) &&
	pidh->e_magic == IMAGE_DOS_SIGNATURE) {
	PINTH pinth = pvAdd(pidh, pidh->e_lfanew);
	if (!IsBadReadPtr(pinth, sizeof(*pinth)) &&
	    pinth->Signature == IMAGE_NT_SIGNATURE) {
	    PEDT pedt = pvAdd(pidh, poteExp(pinth)->VirtualAddress);
	    if (!IsBadReadPtr(pedt, sizeof(*pedt)) &&
		(ord - pedt->Base) < pedt->NumberOfFunctions) {
		PEAT peat = pvAdd(pidh, pedt->AddressOfFunctions);
		FARPROC fp = pvAdd(pidh, peat[ord - pedt->Base]);
		if (pvSub(fp, peat) >= poteExp(pinth)->Size) {
		    return fp;
		} else {		/* Forwarded!? */
		    return 0;
		}
	    } else {
		return 0;
	    }
	} else {
	    return 0;
	}
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  fGrovel
 *
 * Grovel into USER's DS to find the dropmenu time.
 * The problem is that there is no documented way of getting and setting
 * the dropmenu time without rebooting.  So we find it by (trust me)
 * disassembling the SetDoubleClickTime function and knowing that the
 * last instructions are
 *
 *	mov [xxxx], ax ; set drop menu time
 *	pop ds
 *	leave
 *	retf 2
 *
 *  Good news!  On Windows NT, there is a new SPI to do this.
 *
 *****************************************************************************/

typedef HINSTANCE (*LL16)(LPCSTR);
typedef FARPROC (*GPA16)(HINSTANCE, LPCSTR);
typedef BOOL (*FL16)(HINSTANCE);
typedef LPVOID (*MSL)(DWORD);

BOOL PASCAL
fGrovel(void)
{
    OSVERSIONINFO ovi;

    if (SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &dtNT, 0)) {
        lpdt = &dtNT;
        return 1;
    }

    /* Else win95 - must grovel */
    ovi.dwOSVersionInfoSize = sizeof(ovi);
    if (GetVersionEx(&ovi) &&
	ovi.dwMajorVersion == 4 &&
	ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
	HINSTANCE hinstK32 = GetModuleHandle(c_tszKernel32);
	if (hinstK32) {
	    LL16 LoadLibrary16 = (LL16)GetProcOrd(hinstK32, 35);
	    FL16 FreeLibrary16 = (FL16)GetProcOrd(hinstK32, 36);
	    GPA16 GetProcAddress16 = (GPA16)GetProcOrd(hinstK32, 37);
	    MSL MapSL = (MSL)GetProcAddress(hinstK32, c_tszMapSL);
	    if ((DWORD)LoadLibrary16 & (DWORD)FreeLibrary16 &
		(DWORD)GetProcAddress16 & (DWORD)MapSL) {
		HINSTANCE hinst16 = LoadLibrary16(c_szUser);
		if ((UINT)hinst16 > 32) {
		    FARPROC fp = GetProcAddress16(hinst16,
						  MAKEINTRESOURCE(20));
		    if (fp) {
			LPBYTE lpSDCT;
			GetDoubleClickTime(); /* Force segment present */
			lpSDCT = MapSL((DWORD)fp);
			if (!IsBadReadPtr(lpSDCT, 84)) {
			    int i;
			    for (i = 0; i < 80; i++, lpSDCT++) {
				if (*(LPDWORD)lpSDCT == 0x02CAC91F) {
				    lpdt = MapSL(MAKELONG(
					     *(LPWORD)(lpSDCT - 2),
						hinst16));
				    return *lpdt == dtCur();
				}
			    }
			}
		    }
		    FreeLibrary16(hinst16);
		}
	    }
	}
    }
    return 0;
}

/*****************************************************************************
 *
 *  msDt
 *
 *	Get the actual drop time if possible.  Don't all this unless
 *	you know it'll work.
 *
 *****************************************************************************/

UINT PASCAL
msDt(void)
{
    if (lpdt == &dtNT) {
	SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &dtNT, 0);
    }
    return *lpdt;
}

/*****************************************************************************
 *
 *  SetDt
 *
 *	Set the drop time, returning TRUE if we need a reboot.
 *
 *****************************************************************************/

BOOL PASCAL
SetDt(UINT ms, DWORD spif)
{
    if (lpdt == &dtNT) {
	SystemParametersInfo(SPI_SETMENUSHOWDELAY, ms, 0, spif);
	return 0;
    } else {
	if (spif & SPIF_UPDATEINIFILE) {
	    SetIntPkl(pmdii->dtOrig, &c_klDelay);
	}
	if (fLpdt) {
	    *lpdt = ms;
	    return 0;
	} else {
	    return 1;
	}
    }
}

/*****************************************************************************
 *
 *  fXMouse
 *
 *	Determine whether XMouse is enabled.
 *
 *	Returns 0 if disabled, 1 if enabled, or -1 if not supported.
 *
 *      Note that there are *two* ways of getting this information,
 *      depending on which flavor of NT/Win9x we are running.  So
 *      try all of them until one of them works.
 *
 *****************************************************************************/

BOOL PASCAL
fXMouse(void)
{
    BOOL fX;
    if (SystemParametersInfo(SPI_GETUSERPREFERENCE,
			     SPI_UP_ACTIVEWINDOWTRACKING, &fX, 0)) {
        return fX != 0;
    } else if (SystemParametersInfo(SPI_GETACTIVEWINDOWTRACKING,
                                    0, &fX, 0)) {
        return fX != 0;
    } else {
	return -1;
    }
}

/*****************************************************************************
 *
 *  SetXMouse
 *
 *	Set the XMouse feature.
 *
 *****************************************************************************/

INLINE void
SetXMouse(BOOL f)
{
    if (SystemParametersInfo(SPI_SETUSERPREFERENCE,
                             SPI_UP_ACTIVEWINDOWTRACKING, (LPVOID)f,
                             SPIF_UPDATEINIFILE)) {
    } else {
        SystemParametersInfo(SPI_SETACTIVEWINDOWTRACKING,
                             0, (LPVOID)f, SPIF_UPDATEINIFILE);
    }
}

/*****************************************************************************
 *
 *  cxDragCur
 *
 *	Return the current horizontal drag sensitivity.
 *
 *****************************************************************************/

INLINE int
cxDragCur(void)
{
    return GetSystemMetrics(SM_CXDRAG);
}

/*****************************************************************************
 *
 *  SetCxCyDrag
 *
 *	Set the new horizontal and vertical drag tolerances.
 *
 *****************************************************************************/

INLINE void
SetCxCyDrag(int cxDrag, int cyDrag)
{
    SystemParametersInfo(SPI_SETDRAGWIDTH, (WPARAM)cxDrag, 0L,
			 SPIF_UPDATEINIFILE);
    SystemParametersInfo(SPI_SETDRAGHEIGHT, (WPARAM)cyDrag, 0L,
			 SPIF_UPDATEINIFILE);
}

/*****************************************************************************
 *
 *  cxDblClkCur
 *
 *	Return the current horizontal double click sensitivity.
 *	Note that GetSystemMetrics records the total width, so we
 *	need to divide by two to get the half-wit half-width.
 *
 *****************************************************************************/

INLINE int
cxDblClkCur(void)
{
    return GetSystemMetrics(SM_CXDOUBLECLK) / 2;
}

/*****************************************************************************
 *
 *  SetCxCyDblClk
 *
 *	Set the current horizontal double click sensitivity.
 *	Note that GetSystemMetrics records the total width, so we
 *	need to multiply the half-width and half-height by two.
 *
 *****************************************************************************/

INLINE void
SetCxCyDblClk(int cxDblClk, int cyDblClk)
{
    SystemParametersInfo(SPI_SETDOUBLECLKWIDTH, (WPARAM)cxDblClk * 2, 0L,
			 SPIF_UPDATEINIFILE);
    SystemParametersInfo(SPI_SETDOUBLECLKHEIGHT, (WPARAM)cyDblClk * 2, 0L,
			 SPIF_UPDATEINIFILE);
}

/*****************************************************************************
 *
 *  Mouse_ReloadDlgInt
 *
 *	Reload values from an edit control.
 *
 *	hdlg is the dialog box itself.
 *
 *	idc is the edit control identifier.
 *
 *	ppt -> a POINT structure which will contain the current value
 *	       in the x, and an aspect-ratio-corrected value in the y.
 *
 *  We allow the value to exceed the range, in case you're stupid enough
 *  to really want it.
 *
 *
 *****************************************************************************/

void PASCAL
Mouse_ReloadDlgInt(HWND hdlg, UINT idc, PPOINT ppt)
{
    BOOL f;
    LRESULT lr;
    HWND hwnd;
    int x;

    hwnd = GetDlgItem(hdlg, idc+didcUd);
    if (hwnd) {
        lr = SendMessage(hwnd, UDM_GETRANGE, 0, 0L);
        x = (int)GetDlgItemInt(hdlg, idc+didcEdit, &f, 0);
        x = max((UINT)x, HIWORD(lr)); /* Force to lower limit of range */
        ppt->x = x;
        ppt->y = MulDiv(x, pmdii->cyAspect, pmdii->cxAspect);
    }
}

/*****************************************************************************
 *
 *  Mouse_InitDlgInt
 *
 *	Initialize a paired edit control / updown control.
 *
 *	hdlg is the dialog box itself.
 *
 *	idc is the edit control identifier.  It is assumed that idc+didcUd is
 *	the identifier for the updown control.
 *
 *	xMin and xMax are the limits of the control.
 *
 *	x = initial value
 *
 *	ppt -> a POINT structure which will contain the current value
 *	       in the x, and an aspect-ratio-corrected value in the y.
 *
 *
 *****************************************************************************/

void PASCAL
Mouse_InitDlgInt(HWND hdlg, UINT idc, int xMin, int xMax, int xVal, PPOINT ppt)
{
    SendDlgItemMessage(hdlg, idc+didcEdit, EM_LIMITTEXT, 2, 0L);
    SetDlgItemInt(hdlg, idc+didcEdit, xVal, 0);
    
    SendDlgItemMessage(hdlg, idc+didcUd,
		       UDM_SETRANGE, 0, MAKELPARAM(xMax, xMin));

    Mouse_ReloadDlgInt(hdlg, idc, ppt);
}

/*****************************************************************************
 *
 *  The trackbar
 *
 *	The trackbar slider is piecewise linear.  It really should be
 *	exponential, but it's hard to write exp() and log() for integers.
 *
 *	Given two parallel arrays which describe the domain and range,
 *	with
 *
 *	x[N] <= x <= x[N+1]	mapping to	y[N] <= y <= y[N+1],
 *
 *	then
 *
 *	x[N] <= x <= x[N+1] maps to
 *
 *		y = y[N] + (x - x[N]) * (y[N+1] - y[N]) / (x[N+1] - x[N]).
 *
 *****************************************************************************/

/* tbt = trackbar tick */
#define tbtMax 120
#define tbtFreq 15
#define dtMax 65534		    /* Don't use 65535; that's uiErr */

const static UINT CODESEG rgtbt[] =
	{ 0, tbtMax/2, tbtMax*3/4, tbtMax*7/8, tbtMax };
const static UINT CODESEG rgdt[] =
	{ 0,      500,       2000,       5000,  dtMax };

/*****************************************************************************
 *
 *  Mouse_Interpolate
 *
 *	Perform piecewise linear interpolation.  See the formulas above.
 *
 *****************************************************************************/

UINT PASCAL
Mouse_Interpolate(UINT x, const UINT CODESEG *px, const UINT CODESEG *py)
{
    while (x > px[1]) px++, py++;
    return py[0] + MulDiv(x - px[0], py[1] - py[0], px[1] - px[0]);
}

/*****************************************************************************
 *
 *  Mouse_GetDt
 *
 *	Get the setting that the user has selected.
 *
 *	hdlg = dialog handle
 *
 *	dtMax maps to dtInfinite.
 *
 *****************************************************************************/

DT PASCAL
Mouse_GetDt(HWND hdlg)
{
    return Mouse_Interpolate(
		(UINT)SendDlgItemMessage(hdlg, IDC_SPEEDTRACK,
				         TBM_GETPOS, 0, 0L), rgtbt, rgdt);
}

/*****************************************************************************
 *
 *  Mouse_SetDt
 *
 *	Set the setting into the trackbar.
 *
 *	hdlg = dialog handle
 *
 *****************************************************************************/

void PASCAL
Mouse_SetDt(HWND hdlg, DT dt)
{
    SendDlgItemMessage(hdlg, IDC_SPEEDTRACK, TBM_SETPOS, 1,
		       Mouse_Interpolate(dt, rgdt, rgtbt));
}

/*****************************************************************************
 *
 *  Mouse_SetDirty
 *
 *	Make a control dirty.
 *
 *****************************************************************************/

void NEAR PASCAL
Mouse_SetDirty(HWND hdlg)
{
    pmdii->fFactory = 0;
    PropSheet_Changed(GetParent(hdlg), hdlg);
}

/*****************************************************************************
 *
 *  Mouse_UpdateWheel
 *
 *	Update all the wheel control controls.
 *
 *	If "Use wheel" is unchecked, then disable all the insides.
 *
 *****************************************************************************/

void PASCAL
Mouse_UpdateWheel(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_WHEELENABLE);
    if (hwnd) {
	UINT idc;
	BOOL f = IsWindowEnabled(hwnd) &&
		 IsDlgButtonChecked(hdlg, IDC_WHEELENABLE);
	for (idc = IDC_WHEELPAGE; idc <= IDC_WHEELLAST; idc++) {
	    EnableWindow(GetDlgItem(hdlg, idc), f);
	}
    }
}

/*****************************************************************************
 *
 *  Mouse_Reset
 *
 *	Reset all controls to initial values.  This also marks
 *	the control as clean.
 *
 *****************************************************************************/

BOOL PASCAL
Mouse_Reset(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_SPEEDTRACK);
    BOOL f;
    SendMessage(hwnd, TBM_SETRANGE, 0, MAKELPARAM(0, tbtMax));
    SendMessage(hwnd, TBM_SETTICFREQ, tbtFreq, 0L);

    if (fLpdt) {
	pmdii->dtOrig = msDt();		/* Save for revert */
	if (pmdii->dtOrig > dtMax) {	/* Max out here */
	    pmdii->dtOrig = dtMax;
	}
    } else {
	/* just use what seems to be the current setting */
	pmdii->dtOrig = dtCur();
    }
    Mouse_SetDt(hdlg, pmdii->dtOrig);

    f = fXMouse();
    if (f >= 0) {
	CheckDlgButton(hdlg, IDC_XMOUSE, f);
    }

    Mouse_UpdateWheel(hdlg);

    Mouse_InitDlgInt(hdlg, IDC_DBLCLK, 1, 32, cxDblClkCur(), &pmdii->ptDblClk);
    Mouse_InitDlgInt(hdlg, IDC_DRAG, 1, 32, cxDragCur(), &pmdii->ptDrag);

    return 1;
}

/*****************************************************************************
 *
 *  Mouse_Apply
 *
 *	Write the changes to the registry and into USER's DS.
 *
 *****************************************************************************/

BOOL NEAR PASCAL
Mouse_Apply(HWND hdlg)
{
    BOOL fSendWinIniChange = 0;
    BOOL f;
    BOOL fNow;
    DWORD dwNow;
    DT dt;

    dt = Mouse_GetDt(hdlg);
    if (dt != pmdii->dtOrig) {
	pmdii->dtOrig = dt;
	if (pmdii->fFactory) {
	    /* DelPkl(&c_klDelay); */
	    dt = dtDefault();
	}
	if (SetDt(pmdii->dtOrig, SPIF_UPDATEINIFILE)) {
	    Common_NeedLogoff(hdlg);
	}
	fSendWinIniChange = 1;
    }

    if (cxDragCur() != pmdii->ptDrag.x) {
	SetCxCyDrag(pmdii->ptDrag.x, pmdii->ptDrag.y);
	fSendWinIniChange = 1;
    }

    if (cxDblClkCur() != pmdii->ptDblClk.x) {
	SetCxCyDblClk(pmdii->ptDblClk.x, pmdii->ptDblClk.y);
	fSendWinIniChange = 1;
    }

    fNow = fXMouse();
    if (fNow >= 0) {
	f = IsDlgButtonChecked(hdlg, IDC_XMOUSE);
	if (fNow != f) {
	    SetXMouse(f);
	    fSendWinIniChange = 1;
	}
    }

    if (SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &dwNow, 0)) {
	if (IsWindowEnabled(GetDlgItem(hdlg, IDC_WHEELENABLE))) {
	    DWORD dw;

	    if (!IsDlgButtonChecked(hdlg, IDC_WHEELENABLE)) {
		dw = 0;
	    } else if (IsDlgButtonChecked(hdlg, IDC_WHEELPAGE)) {
		dw = WHEEL_PAGESCROLL;
	    } else {
		dw = GetDlgItemInt(hdlg, IDC_WHEELLINENO, &f, 0);
	    }

	    if (dw != dwNow) {
		SystemParametersInfo(SPI_SETWHEELSCROLLLINES, dw, 0, 0);
		fSendWinIniChange = 1;
	    }
	}
    }

    if (fSendWinIniChange) {
	SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
		          (LPARAM)(LPCTSTR)c_tszWindows);
    }

    return Mouse_Reset(hdlg);
}

/*****************************************************************************
 *
 *  Mouse_ReloadUpdowns
 *
 *	Reload the values from the updown controls and update our
 *	internals accordingly.
 *
 *****************************************************************************/

BOOL PASCAL
Mouse_ReloadUpdowns(HWND hdlg)
{
    Mouse_ReloadDlgInt(hdlg, IDC_DBLCLK, &pmdii->ptDblClk);
    Mouse_ReloadDlgInt(hdlg, IDC_DRAG, &pmdii->ptDrag);
    Mouse_SetDirty(hdlg);
    return 1;
}

/*****************************************************************************
 *
 *  Mouse_FactoryReset
 *
 *	Restore to original factory settings.
 *
 *	Droptime = DoubleClickTime * 4 / 5.
 *	Animation = !((GetSystemMetrics(SM_SLOWMACHINE) & 0x0004) &&
 *		      (GetSystemMetrics(SM_SLOWMACHINE) & 0x0001))
 *	cxDrag = 2
 *	cxDblClk = 2
 *
 *****************************************************************************/

BOOL PASCAL
Mouse_FactoryReset(HWND hdlg)
{
    Mouse_SetDirty(hdlg);

    Mouse_SetDt(hdlg, dtDefault());

    if (fXMouse() >= 0) {
	CheckDlgButton(hdlg, IDC_XMOUSE, 0);
    }

    SetDlgItemInt(hdlg, IDC_DRAG, 2, 0);
    SetDlgItemInt(hdlg, IDC_DBLCLK, 2, 0);

    Mouse_ReloadUpdowns(hdlg);

    if (GetDlgItem(hdlg, IDC_WHEELENABLE)) {
	CheckDlgButton(hdlg, IDC_WHEELENABLE, TRUE);
	CheckRadioButton(hdlg, IDC_WHEELPAGE, IDC_WHEELLINE, IDC_WHEELLINE);
	SetDlgItemInt(hdlg, IDC_WHEELLINENO, 3, 0);
    }

    pmdii->fFactory = 1;
    return 1;
}

/*****************************************************************************
 *
 *  Mouse_OnTips
 *
 *****************************************************************************/

void PASCAL
Mouse_OnTips(HWND hdlg)
{
    WinHelp(hdlg, c_tszMyHelp, HELP_FINDER, 0);
}

#ifdef IDC_BUGREPORT
/*****************************************************************************
 *
 *  Mouse_OnBugReport
 *
 *****************************************************************************/

void PASCAL
Mouse_OnBugReport(HWND hdlg)
{
    ShellExecute(hdlg, "open", "http://abject/tweakui/", "", "",
                 SW_NORMAL);
}
#endif

/*****************************************************************************
 *
 *  Mouse_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
Mouse_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_RESET:	/* Reset to factory default */
	if (codeNotify == BN_CLICKED) return Mouse_FactoryReset(hdlg);
	break;

    case IDC_TIPS:	/* Call up help */
	if (codeNotify == BN_CLICKED) Mouse_OnTips(hdlg);
	break;

#ifdef IDC_BUGREPORT
    case IDC_BUGREPORT:
        if (codeNotify == BN_CLICKED) Mouse_OnBugReport(hdlg);
        break;
#endif

    case IDC_XMOUSE:
    case IDC_WHEELPAGE:
    case IDC_WHEELLINE:
	if (codeNotify == BN_CLICKED) Mouse_SetDirty(hdlg);
	break;

    case IDC_DRAG:
    case IDC_DBLCLK:
    case IDC_WHEELLINENO:
	if (codeNotify == EN_CHANGE) {
	    Mouse_ReloadUpdowns(hdlg);
	    Mouse_SetDirty(hdlg);
	}
	break;

    case IDC_WHEELENABLE:
	if (codeNotify == BN_CLICKED) {
	    Mouse_UpdateWheel(hdlg);
	    Mouse_SetDirty(hdlg);
	}
	break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  Mouse_SetTestIcon
 *
 *	Set a new test icon, returning the previous one.
 *
 *****************************************************************************/

HCURSOR PASCAL
Mouse_SetTestIcon(HWND hdlg, UINT idi)
{
    return (HCURSOR)
	SendDlgItemMessage(hdlg, IDC_TEST, STM_SETICON,
			(WPARAM)LoadIconId(idi), 0L);
}

/*****************************************************************************
 *
 *  Mouse_StopDrag
 *
 *	Stop any drag operation in progress.
 *
 *	We must release the capture unconditionally, or a double-click
 *	will result in the mouse capture being stuck!
 *
 *****************************************************************************/

void PASCAL
Mouse_StopDrag(HWND hdlg)
{
    ReleaseCapture();	/* Always do this! */
    if (pmdii->hcurDrag) {
	SetCursor(0);	/* We're about to destroy the current cursor */
	DestroyCursor(pmdii->hcurDrag);
	pmdii->hcurDrag = 0;
	DestroyCursor(Mouse_SetTestIcon(hdlg, pmdii->idi));
    }
    pmdii->fDrag = 0;	/* not dragging */
}

/*****************************************************************************
 *
 *  Mouse_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
Mouse_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	Mouse_Apply(hdlg);
	break;

    /*
     * If we are dragging, then ESC cancels the drag, not the prsht.
     * Note that we must set the message result *last*, because
     * ReleaseCapture will recursively call the dialog procedure,
     * smashing whatever used to be in the message result.
     */
    case PSN_QUERYCANCEL:
	if (pmdii->fDrag) {
	    Mouse_StopDrag(hdlg);
	    SetDlgMsgResult(hdlg, WM_NOTIFY, 1);
	}
	return 1;
    }
    return 0;
}

/*****************************************************************************
 *
 *  Mouse_OnInitDialog
 *
 *	Initialize the controls.
 *
 *****************************************************************************/

BOOL NEAR PASCAL
Mouse_OnInitDialog(HWND hdlg)
{
    UINT idc;
    DWORD dw;
    POINT pt;			/* Dialog origin */

    pmdii->idi = IDI_GEAR1;	/* Start with the first gear */

    pmdii->fDrag = 0;		/* not dragging */

    /* Make sure first click isn't counted as a double */
    pmdii->tmClick = 0;

    pmdii->fFactory = 0;
    pmdii->hcurDrag = 0;

    pt.x = pt.y = 0;
    ClientToScreen(hdlg, &pt);	/* pt = our dialog box origin */

    /*
     *	Get client coordinates by getting window coordinates, then
     *	removing bias.
     */
    GetWindowRect(GetDlgItem(hdlg, IDC_TEST), &pmdii->rcTest);
    OffsetRect(&pmdii->rcTest, -pt.x, -pt.y);

    {
	HDC hdc = GetDC(0);
	if (hdc) {
	    pmdii->cxAspect = GetDeviceCaps(hdc, ASPECTX);
	    pmdii->cyAspect = GetDeviceCaps(hdc, ASPECTY);
	    ReleaseDC(0, hdc);
	    if (pmdii->cxAspect == 0) {	/* Buggy display driver */
		goto Fallback;
	    }
	} else {		/* Assume 1:1 aspect ratio */
	    Fallback:
	    pmdii->cxAspect = pmdii->cyAspect = 1;
	}
    }

    SendDlgItemMessage(hdlg, IDC_WHEELLINENO, EM_LIMITTEXT, 3, 0L);
    SetDlgItemInt(hdlg, IDC_WHEELLINENO, 3, 0);
    SendDlgItemMessage(hdlg, IDC_WHEELLINEUD,
		       UDM_SETRANGE, 0, MAKELPARAM(999, 1));
    if (SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &dw, 0)) {
	if (GetSystemMetrics(SM_MOUSEWHEELPRESENT)) {
	    CheckDlgButton(hdlg, IDC_WHEELENABLE, dw != 0);
	    if (dw == WHEEL_PAGESCROLL) {
		CheckDlgButton(hdlg, IDC_WHEELPAGE, TRUE);
	    } else {
		CheckDlgButton(hdlg, IDC_WHEELLINE, TRUE);
		if (dw) {
		    SetDlgItemInt(hdlg, IDC_WHEELLINENO, dw, 0);
		}
	    }
	} else {
	    EnableWindow(GetDlgItem(hdlg, IDC_WHEELENABLE), 0);
	}
    } else {
	for (idc = IDC_WHEELFIRST; idc <= IDC_WHEELLAST; idc++) {
	    DestroyWindow(GetDlgItem(hdlg, idc));
	}
    }

    if (fXMouse() < 0) {
	DestroyWindow(GetDlgItem(hdlg, IDC_XMOUSE));
    }

    if (fGrovel()) {
	Mouse_Reset(hdlg);
	return 1;		/* Allow focus to travel normally */
    } else {
	lpdt = &dtScratch;
	*lpdt = dtCur();	/* Gotta give it something */
	Mouse_Reset(hdlg);
	ShowWindow(GetDlgItem(hdlg, IDC_SPEEDHELP), SW_HIDE);
	return 0;
    }
}

/*****************************************************************************
 *
 *  Mouse_OnLButtonDown
 *
 *	If the left button went down in the test area, begin capturing.
 *	Also record the time the button went down, so we can do double-click
 *	fuzz testing.
 *
 *****************************************************************************/

BOOL PASCAL
Mouse_OnLButtonDown(HWND hdlg, int x, int y)
{
    POINT pt = { x, y };
    LONG tm = GetMessageTime();

    if (PtInRect(&pmdii->rcTest, pt)) {
	/*
	 *  Is this a double-click?
	 */
	if (pmdii->tmClick &&
	    (DWORD)(tm - pmdii->tmClick) < GetDoubleClickTime() &&
	    PtInRect(&pmdii->rcDblClk, pt)) {
	    pmdii->idi ^= IDI_GEAR1 ^ IDI_GEAR2;
	    DestroyCursor(Mouse_SetTestIcon(hdlg, pmdii->idi));
	    tm = 0;
	}

	SetRectPoint(&pmdii->rcDrag, pt);
	SetRectPoint(&pmdii->rcDblClk, pt);
	InflateRect(&pmdii->rcDrag, pmdii->ptDrag.x, pmdii->ptDrag.y);
	InflateRect(&pmdii->rcDblClk, pmdii->ptDblClk.x, pmdii->ptDblClk.y);

	pmdii->fDrag = 1;	/* Drag in progress */
	SetCapture(hdlg);
    }
    pmdii->tmClick = tm;
    return 1;
}

/*****************************************************************************
 *
 *  Mouse_OnMouseMove
 *
 *	If we are captured, see if we've moved far enough to act as
 *	if a drag is in progress.
 *
 *****************************************************************************/

BOOL PASCAL
Mouse_OnMouseMove(HWND hdlg, int x, int y)
{
    if (pmdii->fDrag && !pmdii->hcurDrag) {
	POINT pt = { x, y };
	if (!PtInRect(&pmdii->rcDrag, pt)) {
	    pmdii->hcurDrag = Mouse_SetTestIcon(hdlg, IDI_BLANK);
	    SetCursor(pmdii->hcurDrag);
	}
    }
    return 0;
}

/*****************************************************************************
 *
 *  Mouse_OnRButtonUp
 *
 *	If the button went up in the menu test area, track a menu.
 *
 *****************************************************************************/

BOOL PASCAL
Mouse_OnRButtonUp(HWND hdlg, int x, int y)
{
    POINT pt = { x, y };
    if (PtInRect(&pmdii->rcTest, pt) && fLpdt) {
	DT dt;
	int id;

	dt = msDt();			/* Save for revert */
	SetDt(Mouse_GetDt(hdlg), 0);

	ClientToScreen(hdlg, &pt);	/* Make it screen coordinates */
	id = TrackPopupMenuEx(GetSubMenu(pcdii->hmenu, 0),
			      TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_VERTICAL |
			      TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y,
			      hdlg, 0);

	SetDt(dt, 0);
	return 1;
    } else {
        return 0;			/* Do the default thing */
    }
}

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

/*
 * The HANDLE_WM_* macros weren't designed to be used from a dialog
 * proc, so we need to handle the messages manually.  (But carefully.)
 */

BOOL EXPORT
Mouse_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: return Mouse_OnInitDialog(hdlg);

    /* We have only one trackbar, so we don't need to check */
    case WM_HSCROLL: Mouse_SetDirty(hdlg); return 1;

    /* We have two updowns, but reloading is cheap, so we just reload both */
    case WM_VSCROLL:
	if (GET_WM_VSCROLL_CODE(wParam, lParam) == SB_THUMBPOSITION) {
	    return Mouse_ReloadUpdowns(hdlg);
	}
	break;

    case WM_COMMAND:
	return Mouse_OnCommand(hdlg,
			       (int)GET_WM_COMMAND_ID(wParam, lParam),
			       (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
    case WM_NOTIFY:
	return Mouse_OnNotify(hdlg, (NMHDR FAR *)lParam);

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
	return Mouse_OnLButtonDown(hdlg, LOWORD(lParam), HIWORD(lParam));

    case WM_ACTIVATE:
	if (GET_WM_ACTIVATE_STATE(wParam, lParam) == WA_INACTIVE) {
	    Mouse_StopDrag(hdlg);
	}
	break;

    case WM_LBUTTONUP:
	Mouse_StopDrag(hdlg);
	break;

    case WM_RBUTTONUP:
	return Mouse_OnRButtonUp(hdlg, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP: Common_OnHelp(lParam, &rgdwHelp[0]); break;

    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelp[0]); break;

    case WM_MOUSEMOVE:
	return Mouse_OnMouseMove(hdlg, LOWORD(lParam), HIWORD(lParam));

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}
