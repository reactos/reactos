#include "cabinet.h"
#include "cabwnd.h"
#include "rcids.h"
#include <desktray.h>
#include <trayp.h>
#include "deskconf.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Support defines and functions for ConfigDesktopDlgProc and friends
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
// Some globals
//
HWND g_hdlgDesktopConfig = NULL;
BOOL g_fShouldShowConfigDesktop = FALSE;
WNDPROC g_DesktopTBProc;


///////////////////////////////////////////////////////////////////////////////
//
// Externs and prototypes we need
//
#ifdef DESKBTN
extern HWND g_hwndDesktopTB;
void Tray_DesktopMenu(DWORD dwPos);
#else
#define g_hwndDesktopTB NULL
#define Tray_DesktopMenu(dwPos) 0
#endif

#define TIMER_DISMISS 1
#define TIMER_POLL    2

#define TIMEOUT_POLL_INTERVAL       50
#define TIMEOUT_DISMISS_INTERVAL    5000

#define BLEND_TRANSPARENT   0
#define BLEND_TRANSLUCENT   128
#define BLEND_OPAQUE        255
#define BLEND_INCREMENT_VAL 10

#define PrivateWS_EX_LAYERED       0x00080000
#define PrivateULW_ALPHA           0x00000002
#if (WINVER >= 0x0500)
// for files in nt5api and w5api dirs, use the definition in sdk include.
// And make sure our private define is in sync with winuser.h.
#if WS_EX_LAYERED != PrivateWS_EX_LAYERED
#error inconsistant WS_EX_LAYERED in winuser.h
#endif
#if ULW_ALPHA != PrivateULW_ALPHA
#error inconsistant ULW_ALPHA in winuser.h
#endif

#else
#define WS_EX_LAYERED   PrivateWS_EX_LAYERED
#define ULW_ALPHA       PrivateULW_ALPHA
#endif // (WINVER >= 0x0500)

typedef BOOL (* PFNUPDATELAYEREDWINDOW)
    (HWND hwnd, 
    HDC hdcDst,
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags);


///////////////////////////////////////////////////////////////////////////////
//
// Implementation
//

// Helper function used to call UpdateLayeredWindow
BOOL BlendLayeredWindow(HWND hwnd, POINT* ppt, LPPOINT pptDst, SIZE* psize, HDC hdc, BYTE bBlendConst)
{
    BOOL bRet = FALSE;
    static PFNUPDATELAYEREDWINDOW pfn = NULL;

    if (NULL == pfn)
    {
        HMODULE hmod = GetModuleHandle(TEXT("USER32"));
        
        if (hmod)
            pfn = (PFNUPDATELAYEREDWINDOW)GetProcAddress(hmod, "UpdateLayeredWindow");
    }

    if (pfn)
    {
        BLENDFUNCTION blend;
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.AlphaFormat = 0;
        blend.SourceConstantAlpha = bBlendConst;

        bRet = pfn(hwnd, NULL, pptDst, psize, hdc, ppt, 0, &blend,
            ULW_ALPHA);
    }

    return bRet;    
}

//
// BOOL BlendMe(HWND hDlg, LPPOINT pptSrc, LPPOINT pptDst, LPRECT prcDlg, BYTE bCurAlpha)
//
// Warning:  This function preserves state information between calls, thus it can only be
// used by one client - the ConfigDesktopDlg code.  Do not call this function with other
// windows!
//
// BlendMe is used to display the window in a alpha blended manner.  Unfortunately in
// order for this to work the system requires us to alpha blend from a bitmap representing
// the window we want to display.  This sucks and makes our work a lot harder.  Currently
// this function caches a bitmap to the window and continually uses it as the blended image
// so if the contents of the window changes they will not be reflected with this function.
// The cached bitmap is freed when BlendMe is called with a NULL hDlg parameter.  BlendMe also
// makes the window "normal" again (strips the WS_EX_LAYERED) bit when the alpha value is
// requested to be at the maximum (BLEND_OPAQUE).
//
BOOL BlendMe(HWND hDlg, LPPOINT pptSrc, LPPOINT pptDst, LPRECT prcDlg, BYTE bCurAlpha)
{
    BOOL fRetVal = FALSE;
    static HBITMAP hbmp = NULL;

    if (!hDlg)
    {
DeleteBmp:
        if (hbmp) {
            DeleteObject(hbmp);
            hbmp = NULL;
        }
        return TRUE;
    }

    if (g_bRunOnNT5)
    {
        HDC hdcDlg, hdcCompat;
        SIZE size = {prcDlg->right - prcDlg->left, prcDlg->bottom - prcDlg->top};
        HBITMAP hbmpOld;
        BOOL fCreatedBmp = FALSE;

        if (bCurAlpha == BLEND_OPAQUE)
        {
            SetWindowLong(hDlg, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
            InvalidateRect(hDlg, NULL, FALSE);
            UpdateWindow(hDlg);
            goto DeleteBmp;
        }

        hdcDlg = GetDC(hDlg);
        hdcCompat = CreateCompatibleDC(hdcDlg);
        if (hdcDlg && hdcCompat)
        {
            if (!hbmp)
            {
                hbmp = CreateCompatibleBitmap(hdcDlg, size.cx, size.cy);
                fCreatedBmp = TRUE;
            }
            if (hbmp)
            {
                hbmpOld = SelectObject(hdcCompat, hbmp);
                if (fCreatedBmp)
                    SendMessage(hDlg, WM_PRINT, (WPARAM)hdcCompat,
                        PRF_CHILDREN | PRF_CLIENT | PRF_ERASEBKGND | PRF_NONCLIENT);
                SetWindowLong(hDlg, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
                ShowWindow(hDlg, SW_SHOWNA);
                BlendLayeredWindow(hDlg, pptSrc, pptDst, &size, hdcCompat, bCurAlpha);
            }
            SelectObject(hdcCompat, hbmpOld);
            DeleteDC(hdcCompat);
            ReleaseDC(hDlg, hdcDlg);
            fRetVal = TRUE;
        }
    }
    return fRetVal;
}

// Helper function used to dismiss the ConfigDesktopDlg button
void KillConfigDesktopDlg()
{
    if (g_hdlgDesktopConfig)
    {
        BlendMe(NULL, NULL, NULL, NULL, 0);
        DestroyWindow(g_hdlgDesktopConfig);
        g_hdlgDesktopConfig = NULL;
    }
}

// Helper function to calculate where to place the ConfigDesktopDlg button so that it is
// adjacent to the "Show Desktop" tray button.
void CalcConfigDesktopDlgPos(/* out */LPPOINT lpptDlg, /* in */LPRECT lprcDlg)
{
    RECT rcButton, rcTray;
    BOOL fDeskBtn = (g_hwndDesktopTB && g_ts.fShowDeskBtn);

    GetWindowRect(v_hwndTray, &rcTray);
    if (fDeskBtn)
    {
        GetWindowRect(g_hwndDesktopTB, &rcButton);
    }
    else
    {
        POINT pt;
        GetCursorPos(&pt);
        SetRect(&rcButton, pt.x, pt.y, pt.x, pt.y);
    }

    if (g_ts.uStuckPlace == STICK_BOTTOM || g_ts.uStuckPlace == STICK_TOP)
    {
        lpptDlg->x = rcButton.left - (fDeskBtn ? (lprcDlg->right - lprcDlg->left - (rcButton.right - rcButton.left)) : 0);
        if (g_ts.uStuckPlace == STICK_BOTTOM)
            lpptDlg->y = rcTray.top - (lprcDlg->bottom - lprcDlg->top);
        else
            lpptDlg->y = rcTray.bottom;
    }
    else
    {
        if (g_ts.uStuckPlace == STICK_LEFT)
            lpptDlg->x = rcTray.right;
        else
            lpptDlg->x = rcTray.left - (lprcDlg->right - lprcDlg->left);
        lpptDlg->y = rcButton.top;
    }
}

#ifdef DESKBTN
// The purpose of this subclass is to show the ConfigDesktopDlg button when the user moves the
// mouse over the "Show Desktop" tray button and the desktop is in the raised state.
LRESULT CALLBACK DesktopTBSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_MOUSEMOVE && !g_hdlgDesktopConfig && g_fDesktopRaised && g_fShouldShowConfigDesktop)
    {
        RECT rc = {0, 0, 40, 20}; // Dummy rectangle
        POINT pt;
        HWND hwnd;

        // Don't show if we find a topmost window (like our config desktop menu) on top of where the
        // ConfigDesktopDlg button would go
        CalcConfigDesktopDlgPos(&pt, &rc);
        hwnd = WindowFromPoint(pt);
        if (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST))
            g_hdlgDesktopConfig = CreateDialog(hinstCabinet, MAKEINTRESOURCE(DLG_CONFIGDESKTOP),
                                    HWND_DESKTOP, (DLGPROC)ConfigDesktopDlgProc);
    }

    return CallWindowProc(g_DesktopTBProc, hwnd, uMsg, wParam, lParam);
}
#endif

//
// BOOL CALLBACK ConfigDesktopDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
// This function implements the "Customize My Desktop..." button displayed adjacent to the "Show Desktop"
// tray button.  If the system is capable the dialog is displayed using fade in/out via alpha blending
// so it isn't too disruptive to the content of the desktop.  The button self dismisses after a timeout
// period expires and can also reshow itself after being dismissed if the mouse moves over the hot spot
// (currently the "Show Desktop" button).  Clicking on the button invokes the Active Desktop Customization
// menu.
//
BOOL CALLBACK ConfigDesktopDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rcDlg, rcButton;
    POINT pt, ptDlg;
    int iAlphaTemp;
    static int iCurAlpha;
    static BOOL fDismissing;

    switch (uMsg) {
        case WM_INITDIALOG:
            fDismissing = FALSE;
            iCurAlpha = BLEND_TRANSPARENT + 1;
            SetWindowLong(hDlg, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
            GetWindowRect(hDlg, &rcDlg);
            CalcConfigDesktopDlgPos(&ptDlg, &rcDlg);
            SetWindowPos(hDlg, HWND_TOPMOST, ptDlg.x, ptDlg.y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE);
            pt.x = pt.y = 0;
            if (!BlendMe(hDlg, &pt, &ptDlg, &rcDlg, (BYTE)iCurAlpha))
                ShowWindow(hDlg, SW_SHOWNA);
            SetTimer(hDlg, TIMER_DISMISS, TIMEOUT_DISMISS_INTERVAL, NULL);
            SetTimer(hDlg, TIMER_POLL, TIMEOUT_POLL_INTERVAL, NULL);
            return FALSE;

        case WM_COMMAND:
            {
                DWORD dwPos;
                BOOL fDeskBtn = (g_hwndDesktopTB && g_ts.fShowDeskBtn);

                GetWindowRect(hDlg, &rcDlg);
                dwPos = MAKELONG(((fDeskBtn && ((g_ts.uStuckPlace == STICK_TOP) || (g_ts.uStuckPlace == STICK_BOTTOM))) ? LOWORD(rcDlg.right) : LOWORD(rcDlg.left)),
                                 (((g_ts.uStuckPlace == STICK_TOP) || ((g_ts.uStuckPlace != STICK_BOTTOM) && !fDeskBtn)) ? LOWORD(rcDlg.top) : LOWORD(rcDlg.bottom)));
                KillConfigDesktopDlg();
                Tray_DesktopMenu(dwPos);
            }
            break;

        case WM_TIMER:
            switch (wParam)
            {
                case TIMER_POLL:
                    // Get positions, rectangles
                    GetWindowRect(hDlg, &rcDlg);
                    if (g_hwndDesktopTB)
                        GetWindowRect(g_hwndDesktopTB, &rcButton);
                    else
                        SetRectEmpty(&rcButton);
                    GetCursorPos(&pt);

                    // Defer dismissing us if the mouse is in areas we're interested in tracking
                    if (PtInRect(&rcDlg, pt) || PtInRect(&rcButton, pt))
                    {
                        fDismissing = FALSE;
                        SetTimer(hDlg, TIMER_DISMISS, TIMEOUT_DISMISS_INTERVAL, NULL);
                    }

                    // Figure out if we need to increase or decrease the alpha for the blend
                    if (PtInRect(&rcDlg, pt) || (!fDismissing && (iCurAlpha < BLEND_TRANSLUCENT)))
                    {
                        iAlphaTemp = iCurAlpha + BLEND_INCREMENT_VAL;
                        if (iAlphaTemp > BLEND_OPAQUE)
                            iAlphaTemp = BLEND_OPAQUE;
                    }
                    else
                    {
                        iAlphaTemp = iCurAlpha - BLEND_INCREMENT_VAL;
                        if (iAlphaTemp < (fDismissing ? BLEND_TRANSPARENT : BLEND_TRANSLUCENT))
                            iAlphaTemp = fDismissing ? BLEND_TRANSPARENT : BLEND_TRANSLUCENT;
                    }

                    // If we have calculated that the alpha has changed then update our image on the screen
                    if (iCurAlpha != iAlphaTemp)
                    {
                        iCurAlpha = iAlphaTemp;
                        GetWindowRect(hDlg, &rcDlg);
                        pt.x = pt.y = 0;
                        BlendMe(hDlg, &pt, (LPPOINT)&rcDlg, &rcDlg, (BYTE)iCurAlpha);
                    }

                    // Finally, if we are in the process of dismissing and our alpha has gone to zero then destroy us.
                    if (fDismissing && (iCurAlpha == BLEND_TRANSPARENT))
                        KillConfigDesktopDlg();
                    break;

                case TIMER_DISMISS:
                    // Start the dismissal process
                    fDismissing = TRUE;
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

BOOL GetDesktopSCFPaths(LPTSTR lpszDeskQL, LPTSTR lpszSystem)
{
    TCHAR szFileName[MAX_PATH];
    if (SHGetSpecialFolderPath(NULL, lpszDeskQL, CSIDL_APPDATA, FALSE))
    {
        GetSystemDirectory(lpszSystem, MAX_PATH);
        LoadString(hinstCabinet, IDS_DESKTOPQUICKLAUNCH, (LPTSTR)(lpszDeskQL + lstrlen(lpszDeskQL)), MAX_PATH - lstrlen(lpszDeskQL));
        LoadString(hinstCabinet, IDS_SHOWDESKSCF, szFileName, ARRAYSIZE(szFileName));
        StrCatN(lpszDeskQL, szFileName, MAX_PATH - lstrlen(lpszDeskQL));
        StrCatN(lpszSystem, szFileName, MAX_PATH - lstrlen(lpszSystem));
        return TRUE;
    }
    return FALSE;
}

void ShowHideDeskBtnOnQuickLaunch(BOOL fShownOnTray)
{
    if (fShownOnTray != BOOLIFY(g_ts.fShowDeskBtn))
    {
        TCHAR szQL[MAX_PATH], szSystem[MAX_PATH];

        if (GetDesktopSCFPaths(szQL, szSystem))
        {
            if (fShownOnTray ? DeleteFile(szQL) : CopyFile(szSystem, szQL, TRUE))
                SHChangeNotify(fShownOnTray ? SHCNE_DELETE : SHCNE_CREATE, SHCNF_PATH, szQL, NULL);
        }
#if 0
        DWORD dwAttribs;
        TCHAR szPath[MAX_PATH];
        if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, FALSE))
        {
            LoadString(hinstCabinet, IDS_DESKTOPQUICKLAUNCH, (LPTSTR)(szPath + lstrlen(szPath)), ARRAYSIZE(szPath) - lstrlen(szPath));
            dwAttribs = GetFileAttributes(szPath);
            if (dwAttribs != -1)
            {
                if (fShownOnTray)
                    dwAttribs |= FILE_ATTRIBUTE_HIDDEN;
                else
                    dwAttribs &= ~FILE_ATTRIBUTE_HIDDEN;
                if (SetFileAttributes(szPath, dwAttribs))
                    SHChangeNotify(SHCNE_ATTRIBUTES, SHCNF_PATH, szPath, NULL);
            }
        }
#endif
    }
}

