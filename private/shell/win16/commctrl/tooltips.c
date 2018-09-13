#include "ctlspriv.h"

#define TEXT(x) x
#define LPTSTR LPSTR
#define LPCTSTR LPCSTR
#define TCHAR char

#define TTDEBUG
extern const TCHAR FAR c_szTTSubclass[];

#define ACTIVE          0x10
#define BUTTONISDOWN    0x20
#define BUBBLEUP        0x40
#define VIRTUALBUBBLEUP 0x80  // this is for dead areas so we won't
                                //wait after moving through dead areas
#define TRACKMODE       0x01

#define MAXTIPSIZE 128
#define XTEXTOFFSET 2
#define YTEXTOFFSET 1

#define TTT_INITIAL  1
#define TTT_RESHOW   2
#define TTT_POP      3
#define TTT_AUTOPOP  4

#define CH_PREFIX '&'

/* tooltips.c */

typedef struct {
    HWND hwnd;
    int iNumTools;
    int iDelayTime;
    int iReshowTime;
    int iAutoPopTime;
    PTOOLINFO tools;
    PTOOLINFO pCurTool;
    BOOL fMyFont;
    HFONT hFont;
    DWORD dwFlags;
    DWORD dwStyle;

    // Timer info;
    UINT idTimer;
    POINT pt;

    UINT idtAutoPop;
    TOOLTIPTEXT ttt;

    POINT ptTrack; // the saved track point from TTM_TRACKPOSITION

    COLORREF clrTipBk;          // This is joeb's idea...he wants it
    COLORREF clrTipText;        // to be able to _blend_ more, so...
} CToolTipsMgr, NEAR *PToolTipsMgr;


#define TTWindowFromPoint(pTtm, ppt) (HWND)SendMessage(pTtm->hwnd, TTM_WINDOWFROMPOINT, 0, (LPARAM)(LPPOINT)ppt)
#define TTToolHwnd(pTool)  ((pTool->uFlags & TTF_IDISHWND) ? (HWND)pTool->uId : pTool->hwnd)
#define IsTextPtr(lpszText)  (((lpszText) != LPSTR_TEXTCALLBACK) && (HIWORD(lpszText)))

//
// Function prototypes
//
LRESULT WINAPI ToolTipsWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL TTSetDelayTime(PToolTipsMgr pTtm, WPARAM wParam, LPARAM lParam);
int NEAR PASCAL TTGetDelayTime(PToolTipsMgr pTtm, WPARAM wParam);

#ifdef UNICODE
BOOL ThunkToolInfoAtoW (LPTOOLINFOA lpTiA, LPTOOLINFOW lpTiW, BOOL bThunkText);
BOOL ThunkToolInfoWtoA (LPTOOLINFOW lpTiW, LPTOOLINFOA lpTiA);
BOOL ThunkToolTipTextAtoW (LPTOOLTIPTEXTA lpTttA, LPTOOLTIPTEXTW lpTttW);
#endif

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitToolTipsClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    // See if we must register a window class
    if (!GetClassInfo(hInstance, c_szSToolTipsClass, &wc)) {
#ifndef WIN32
#ifndef IEWIN31
    extern LRESULT CALLBACK _ToolTipsWndProc(HWND, UINT, WPARAM, LPARAM);
    wc.lpfnWndProc = _ToolTipsWndProc;
#else
    wc.lpfnWndProc = (WNDPROC)ToolTipsWndProc;
#endif
#else
    wc.lpfnWndProc = (WNDPROC)ToolTipsWndProc;
#endif

    wc.lpszClassName = c_szSToolTipsClass;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = NULL;
    wc.lpszMenuName = NULL;
    wc.hbrBackground = (HBRUSH)(NULL);
    wc.hInstance = hInstance;
    wc.style = CS_DBLCLKS | CS_GLOBALCLASS | CS_SAVEBITS;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PToolTipsMgr);

    return RegisterClass(&wc);
    }
    return TRUE;
}
#pragma code_seg()


/* _  G E T  H C U R S O R  P D Y 3 */
/*-------------------------------------------------------------------------
 %%Function: _GetHcursorPdy3
 %%Contact: migueldc

 With the new mouse drivers that allow you to customize the mouse
 pointer size, GetSystemMetrics returns useless values regarding
 that pointer size.

 Assumptions:
 1. The pointer's width is equal to its height. We compute
 its height and infer its width.
 2. The pointer's leftmost pixel is located in the 0th column
 of the bitmap describing it.
 3. The pointer's topmost pixel is located in the 0th row
 of the bitmap describing it.

 This function looks at the mouse pointer bitmap,
 to find out the height of the mouse pointer (not returned),
 the vertical distance between the cursor's hot spot and
 the cursor's lowest visible pixel (pdyBottom),
 the horizontal distance between the hot spot and the pointer's
 left edge (pdxLeft) annd the horizontal distance between the
 hot spot and the pointer's right edge (pdxRight).
 -------------------------------------------------------------------------*/
typedef WORD CURMASK;
#define _BitSizeOf(x) (sizeof(x)*8)

void NEAR PASCAL _GetHcursorPdy3(int FAR *pdyBottom)
{
#ifdef IEWIN31_25
//    *pdyBottom = 16;  //best guess
    *pdyBottom = GetSystemMetrics(SM_CYCURSOR)/2;
    return;
#else
    int i;
    int iXOR = 0;
    int dy;
    CURMASK CurMask[16*8];
    ICONINFO iconinfo;
    BITMAP bm;
    HCURSOR hCursor = GetCursor();

    if (!GetIconInfo(hCursor, &iconinfo)) {
        *pdyBottom = 16;  //best guess
        return;
    }
    if (!GetObject(iconinfo.hbmMask, sizeof(bm), (LPSTR)&bm)) {
        *pdyBottom = 16;  //best guess
        return;
    }
    if (!GetBitmapBits(iconinfo.hbmMask, sizeof(CurMask), CurMask)) {
        *pdyBottom = 16;  //best guess
        return;
    }
    i = (int)(bm.bmWidth * bm.bmHeight / _BitSizeOf(CURMASK) );

    if (!iconinfo.hbmColor) {
        // if no color bitmap, then the hbmMask is a double height bitmap
        // with the cursor and the mask stacked.
        iXOR = i - 1;
        i /= 2;

    }

    if ( i >= sizeof(CurMask)) i = sizeof(CurMask) -1;
    if (iXOR >= sizeof(CurMask)) iXOR = 0;
    for (i--; i >= 0; i--)   {
    if (CurMask[i] != 0xFFFF || (iXOR && (CurMask[iXOR--] != 0)))
        break;
    }
    if (iconinfo.hbmColor) DeleteObject(iconinfo.hbmColor);
    if (iconinfo.hbmMask) DeleteObject(iconinfo.hbmMask);

    // Compute the pointer height
    dy = (i + 1) * _BitSizeOf(CURMASK) / (int)bm.bmWidth;

    // Compute the distance between the pointer's lowest, left, rightmost
    //  pixel and the HotSpotspot
    *pdyBottom = dy - (int)iconinfo.yHotspot;
#endif

}

// this returns the values in work area coordinates because
// that's what set window placement uses
void NEAR PASCAL _GetCursorLowerLeft(int FAR *piLeft,
                     int FAR *piBottom)
{
    DWORD dwPos;
    int dy;
    dwPos = GetMessagePos();
    _GetHcursorPdy3(&dy);
    *piLeft = LOWORD(dwPos);
    *piBottom = HIWORD(dwPos)+dy;
}

void NEAR PASCAL ToolTips_NewFont(PToolTipsMgr pTtm, HFONT hFont)
{
    if (pTtm->fMyFont && pTtm->hFont)
    {
        DeleteObject(pTtm->hFont);
        pTtm->fMyFont = FALSE;
    }

    if ( !hFont )
    {

#ifdef IEWIN31_25
        // SPI_GETNONCLIENTMETRICS is not supported on win31
        LOGFONT lf;
        if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
        {
            hFont = CreateFontIndirect(&lf);
        }
#else   // !IEWIN31_25

#ifndef WIN32
        LOGFONT lf;
#endif
        NONCLIENTMETRICS ncm;

        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

#ifdef WIN32
        hFont = CreateFontIndirect(&ncm.lfStatusFont);
#else
        lf.lfHeight = (int)ncm.lfStatusFont.lfHeight;
        lf.lfWidth = (int)ncm.lfStatusFont.lfWidth;
        lf.lfEscapement = (int)ncm.lfStatusFont.lfEscapement;
        lf.lfOrientation = (int)ncm.lfStatusFont.lfOrientation;
        lf.lfWeight = (int)ncm.lfStatusFont.lfWeight;
        hmemcpy(&lf.lfItalic, &ncm.lfStatusFont.lfCommon, sizeof(COMMONFONT));

        hFont = CreateFontIndirect(&lf);
#endif
#endif //!IEWIN31_25
        pTtm->fMyFont = TRUE;

        if (!hFont) {
            hFont = g_hfontSystem;
            pTtm->fMyFont = FALSE;
        }
    }

    pTtm->hFont = hFont;
}

BOOL NEAR PASCAL ChildOfActiveWindow(HWND hwnd)
{
#ifndef WIN31
    HWND hwndActive = GetForegroundWindow();
#else
    HWND hwndActive = GetActiveWindow();
#endif
    return hwndActive && (hwnd == hwndActive || IsChild(hwndActive, hwnd));
}

void NEAR PASCAL PopBubble(PToolTipsMgr pTtm)
{
    // we're at least waiting to show;
    DebugMsg(DM_TRACE, TEXT("PopBubble"));
    if(pTtm->idTimer) {
        KillTimer(pTtm->hwnd, pTtm->idTimer);
        pTtm->idTimer = 0;
    }

    if (pTtm->idtAutoPop) {
        KillTimer(pTtm->hwnd, pTtm->idtAutoPop);
        pTtm->idtAutoPop = 0;
    }


    if (IsWindowVisible(pTtm->hwnd) && pTtm->pCurTool) {

        NMHDR nmhdr;
        nmhdr.hwndFrom = pTtm->hwnd;
        nmhdr.idFrom = pTtm->pCurTool->uId;
        nmhdr.code = TTN_POP;
#ifdef IEWIN31_25
//causes crash!
//        SendNotifyEx(pTtm->pCurTool->hwnd, (HWND)-1,
//                     TTN_POP, &nmhdr,
//                     (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0);
#else
        SendMessage(pTtm->pCurTool->hwnd, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
#endif

    }

    KillTimer(pTtm->hwnd, TTT_POP);
    ShowWindow(pTtm->hwnd, SW_HIDE);
    pTtm->dwFlags &= ~(BUBBLEUP|VIRTUALBUBBLEUP);
    pTtm->pCurTool = NULL;

}

PToolTipsMgr NEAR PASCAL ToolTipsMgrCreate(CREATESTRUCT FAR* lpCreateStruct)
{
    PToolTipsMgr pTtm = (PToolTipsMgr)LocalAlloc(LPTR, sizeof(CToolTipsMgr));
    if (pTtm) {

        // LPTR zeros the rest of the struct for us
        TTSetDelayTime(pTtm, TTDT_AUTOMATIC, (LPARAM)-1);
        pTtm->dwFlags = ACTIVE;
        pTtm->ttt.hdr.hwndFrom = pTtm->hwnd;
        pTtm->dwStyle = lpCreateStruct->style;

        // These are the defaults (straight from cutils.c),
        // but you can always change them...
#ifdef IEWIN31_25
        pTtm->clrTipBk = RGB(255, 255, 128);
        pTtm->clrTipText = RGB(0, 0, 0);
#else
        pTtm->clrTipBk = GetSysColor(COLOR_INFOBK);
        pTtm->clrTipText = GetSysColor(COLOR_INFOTEXT);
#endif

    }
    return pTtm;
}

void NEAR PASCAL TTSetTimer(PToolTipsMgr pTtm, int id)
{
    int iDelayTime = 0;

    if(pTtm->idTimer) {
    KillTimer(pTtm->hwnd, pTtm->idTimer);
    }

    switch (id) {
        case TTT_POP:
        case TTT_RESHOW:
            iDelayTime = pTtm->iReshowTime;
            if (iDelayTime < 0)
                iDelayTime = GetDoubleClickTime() / 5;
            break;

        case TTT_INITIAL:
            iDelayTime = pTtm->iDelayTime;
            if (iDelayTime < 0)
                iDelayTime = GetDoubleClickTime();
            break;

    case TTT_AUTOPOP:
        iDelayTime = pTtm->iAutoPopTime;
        if (iDelayTime < 0)
            iDelayTime = GetDoubleClickTime() * 10;
        pTtm->idtAutoPop = SetTimer(pTtm->hwnd, id, iDelayTime, NULL);
        return;
    }

    if (SetTimer(pTtm->hwnd, id, iDelayTime, NULL) &&
        (id != TTT_POP)) {
        pTtm->idTimer = id;
        GetCursorPos(&pTtm->pt);
    }
}

BOOL NEAR PASCAL ToolHasMoved(PToolTipsMgr pTtm)
{
    // this is in case Raymond pulls something sneaky like moving
    // the tool out from underneath the cursor.

    HWND hwnd;
    RECT rc;
    PTOOLINFO pTool = pTtm->pCurTool;

    if (!pTool)
        return TRUE;

    hwnd = TTToolHwnd(pTool);

    // if the window is no longer visible, or is no long a child
    // of the active (without the always tip flag)
    // also check window at point to ensure that the window isn't covered
    if (IsWindowVisible(hwnd) &&
        ((pTtm->dwStyle & TTS_ALWAYSTIP) || ChildOfActiveWindow(hwnd)) &&
        (hwnd == TTWindowFromPoint(pTtm, &pTtm->pt))) {

        GetWindowRect(hwnd, &rc);
        if(PtInRect(&rc, pTtm->pt) )
            return FALSE;
    }

    return TRUE;
}

BOOL NEAR PASCAL MouseHasMoved(PToolTipsMgr pTtm)
{
    POINT pt;
    GetCursorPos(&pt);
    return ( (pt.x != pTtm->pt.x) || (pt.y != pTtm->pt.y) );
}

PTOOLINFO NEAR PASCAL FindTool(PToolTipsMgr pTtm, LPTOOLINFO lpToolInfo)
{
    int i;
    PTOOLINFO pTool;


    // BUGBUG: in win95, this was NOT validated... by doing so now, we may
    // cause some compat problems... if so, we need to assume for 4.0 marked
    // guys that cbSize == &(0->lParam)
    if (lpToolInfo->cbSize > sizeof(TOOLINFO))
        return NULL;

    // you can pass in an index or a toolinfo descriptor
    if (!HIWORD(lpToolInfo)) {
        i = (int)LOWORD(lpToolInfo);
        if (i < pTtm->iNumTools) {
            return &pTtm->tools[i];
        } else
            return NULL;

    }

    for(i = 0 ; i < pTtm->iNumTools; i++) {
    pTool = &pTtm->tools[i];
    if((pTool->hwnd == lpToolInfo->hwnd) &&
       (pTool->uId == lpToolInfo->uId))
        return pTool;
    }
    return NULL;
}

#ifdef WIN32
#define SetPropEx SetProp
#define GetPropEx GetProp
#define RemovePropEx RemoveProp
#else
#ifdef IEWIN31_25
// Emulate properties that handle DWORDs
BOOL WINAPI SetPropEx(HWND hWnd, LPCSTR lpString, DWORD dwData)
{
    DWORD* pdwData = (DWORD*)LocalAlloc(LPTR, sizeof(DWORD));
    if (!pdwData)
        return FALSE;

    *pdwData = dwData;

    if (!SetProp(hWnd, lpString, pdwData))
    {
        LocalFree(pdwData);
        return FALSE;
    }
    return TRUE;
}
DWORD WINAPI GetPropEx(HWND hWnd, LPCSTR lpString)
{
    DWORD* pdw = (DWORD*)GetProp(hWnd, lpString);
    if (!pdw)
        return NULL;
    return *pdw;
}
DWORD WINAPI RemovePropEx(HWND hWnd, LPCSTR lpString)
{
    DWORD* pdw = (DWORD*)GetProp(hWnd, lpString);
    if (!pdw)
        LocalFree(pdw);
    return RemoveProp(hWnd, lpString);
}
#endif //IEWIN31_25
#endif

typedef struct _ttsubclass {
    WNDPROC pfnWndProc;
    int cRef;
    HWND hwndTT;
} TTSUBCLASS, FAR *LPTTSUBCLASS;

void NEAR PASCAL TTUnsubclassHwnd(HWND hwnd)
{
//#ifndef WIN31
#if !defined(WIN31) || defined(IEWIN31_25)
    LPTTSUBCLASS lpttsc;
    int cRef = 0;

    lpttsc = (LPTTSUBCLASS)GetPropEx(hwnd, c_szTTSubclass);
    if (lpttsc && lpttsc->cRef > 0) {
        lpttsc->cRef--;

        if (!lpttsc->cRef) {

            // nothing left.. bail!
            // not subclassed yet, do it now.
            SetWindowLong(hwnd, GWL_WNDPROC, (LONG)lpttsc->pfnWndProc);
            RemovePropEx(hwnd, c_szTTSubclass);
            GlobalFreePtr(lpttsc);
        }
    }
#endif
}

//#ifndef WIN31
#if !defined(WIN31) || defined(IEWIN31_25)
LRESULT WINAPI TTSubClassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPTTSUBCLASS lpttsc = (LPTTSUBCLASS)GetPropEx(hwnd, c_szTTSubclass);

    if (lpttsc) {
        WNDPROC pfnWndProc = lpttsc->pfnWndProc;
        // save this away in case we get nuked before we call it

        switch (message) {
        case WM_DESTROY:
            lpttsc->cRef = 1;
            TTUnsubclassHwnd(hwnd);
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_NCMOUSEMOVE:
        case WM_MOUSEMOVE:
            RelayToToolTips(lpttsc->hwndTT, hwnd, message, wParam, lParam);
            break;
        }

        return CallWindowProc(pfnWndProc, hwnd, message, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

}
#endif

void NEAR PASCAL TTSubclassHwnd(PTOOLINFO pTool, HWND hwndTT)
{
//#ifndef WIN31
#if !defined(WIN31) || defined(IEWIN31_25)
    HWND hwnd;
    LPTTSUBCLASS lpttsc;
    int cRef = 0;

    hwnd = TTToolHwnd(pTool);
    lpttsc = (LPTTSUBCLASS)GetPropEx(hwnd, c_szTTSubclass);
    if (!lpttsc) {

        lpttsc = (LPTTSUBCLASS)GlobalAllocPtr(GPTR, sizeof(TTSUBCLASS));
        if (lpttsc) {

            // not subclassed yet, do it now.
#ifdef WIN32
            if (SetPropEx(hwnd, c_szTTSubclass, (HANDLE)lpttsc)) {
#else
            if (SetPropEx(hwnd, c_szTTSubclass, (DWORD)lpttsc)) {
#endif

                lpttsc->pfnWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
                lpttsc->cRef = 1;
                lpttsc->hwndTT = hwndTT;
                SetWindowLong(hwnd, GWL_WNDPROC, (LPARAM)(WNDPROC)TTSubClassWndProc);
            }
        }
    } else {
        lpttsc->cRef++;
    }
#endif
}


void NEAR PASCAL TTSetTipText(LPTOOLINFO pTool, LPTSTR lpszText)
{
    // if it wasn't alloc'ed before, set it to NULL now so we'll alloc it
    // otherwise, don't touch it and it will be realloced
    if (!IsTextPtr(pTool->lpszText)) {
        pTool->lpszText = NULL;
    }

    if (IsTextPtr(lpszText)) {
        Str_Set(&pTool->lpszText, lpszText);
    } else {
        // if it was alloc'ed before free it now.
        Str_Set(&pTool->lpszText, NULL);
        pTool->lpszText = lpszText;
    }
}

LRESULT NEAR PASCAL AddTool(PToolTipsMgr pTtm, LPTOOLINFO lpToolInfo)
{
    PTOOLINFO pTool;

    // bail for right now;

    if (lpToolInfo->cbSize > sizeof(TOOLINFO)) {
        Assert(0);
        return 0L;
    }

    // on failure to alloc do nothing.
    if(pTtm->tools) {
    HLOCAL h = LocalReAlloc((HANDLE)pTtm->tools,
                sizeof(TOOLINFO)*(pTtm->iNumTools+1),
                LMEM_MOVEABLE | LMEM_ZEROINIT);
    if(h) {

            // realloc could have moved stuff around.  repoint pCurTool
            if (pTtm->pCurTool) {
                pTtm->pCurTool = ((PTOOLINFO)h) + (pTtm->pCurTool - pTtm->tools);
            }
        pTtm->tools = (PTOOLINFO)h;

        } else
        return 0L;
    } else {
    pTtm->tools = (PTOOLINFO)LocalAlloc(LPTR, sizeof(TOOLINFO));
    if ( !pTtm->tools )
        return 0L;
    }

    pTool = &pTtm->tools[pTtm->iNumTools];
    pTtm->iNumTools++;
    hmemcpy(pTool, lpToolInfo, lpToolInfo->cbSize);
    pTool->lpszText = NULL;
    TTSetTipText(pTool, lpToolInfo->lpszText);
    if (pTool->uFlags & TTF_SUBCLASS) {
        TTSubclassHwnd(pTool, pTtm->hwnd);
    }

    if (SendMessage (pTool->hwnd, WM_NOTIFYFORMAT, (WPARAM)pTtm->hwnd,
                                  NF_QUERY) == NFR_UNICODE) {
        pTool->uFlags |= TTF_UNICODE;
    }

#ifdef TTDEBUG
    DebugMsg(DM_TRACE, TEXT("Tool Added: ptr=%d, uFlags=%d, wid=%d, hwnd=%d"),
             pTool, pTool->uFlags, pTool->uId, pTool->hwnd);
#endif

    return 1L;
}

void NEAR PASCAL TTBeforeFreeTool(LPTOOLINFO pTool)
{
    if (pTool->uFlags & TTF_SUBCLASS)
        TTUnsubclassHwnd(TTToolHwnd(pTool));

    // clean up
    TTSetTipText(pTool, NULL);
}

void NEAR PASCAL DeleteTool(PToolTipsMgr pTtm, LPTOOLINFO lpToolInfo)
{
    PTOOLINFO pTool;

    // bail for right now;
    if (lpToolInfo->cbSize > sizeof(TOOLINFO)) {
        Assert(0);
        return;
    }

    pTool = FindTool(pTtm, lpToolInfo);
    if(pTool) {
    if (pTtm->pCurTool == pTool)
        PopBubble(pTtm);

        TTBeforeFreeTool(pTool);

    // replace it with the last one.. no need to waste cycles in realloc
    pTtm->iNumTools--;
    *pTool = pTtm->tools[pTtm->iNumTools]; // struct copy

    //cleanup if we moved the current tool
    if(pTtm->pCurTool == &pTtm->tools[pTtm->iNumTools])
        pTtm->pCurTool = pTool;
    }
}

// this strips out & markers so that people can use menu text strings
void NEAR PASCAL StripAccels(PToolTipsMgr pTtm)
{
    if (!(pTtm->dwStyle & TTS_NOPREFIX)) {

        if (pTtm->ttt.lpszText != pTtm->ttt.szText) {
            lstrcpyn(pTtm->ttt.szText, pTtm->ttt.lpszText, ARRAYSIZE(pTtm->ttt.szText));
            pTtm->ttt.lpszText = pTtm->ttt.szText;
        }
        StripAccelerators(pTtm->ttt.szText, pTtm->ttt.szText);
    }
}

LPTSTR NEAR PASCAL GetToolText(PToolTipsMgr pTtm, PTOOLINFO pTool)
{
    int id;
    HINSTANCE hinst;

    pTtm->ttt.szText[0] = TEXT('\0');
    pTtm->ttt.lpszText = pTtm->ttt.szText;
#ifdef TTDEBUG
    DebugMsg(DM_TRACE, TEXT("        **Enter GetToolText: ptr=%d, wFlags=%d, wid=%d, hwnd=%d"),
             pTool, pTool->uFlags, pTool->uId, pTool->hwnd);
#endif
    if (pTool->lpszText == LPSTR_TEXTCALLBACK) {

    pTtm->ttt.hdr.idFrom = pTool->uId;
    pTtm->ttt.hdr.code = TTN_NEEDTEXT;
        pTtm->ttt.hdr.hwndFrom = pTtm->hwnd;
        pTtm->ttt.uFlags = pTool->uFlags;
        pTtm->ttt.lParam = pTool->lParam;
    SendMessage(pTool->hwnd, WM_NOTIFY, pTool->uId, (LPARAM)(LPTOOLTIPTEXT)&pTtm->ttt);

        if (pTtm->ttt.uFlags & TTF_DI_SETITEM) {
            if (!HIWORD(pTtm->ttt.lpszText)) {
                pTool->lpszText = pTtm->ttt.lpszText;
                pTool->hinst = pTool->hinst;
            } else if (pTtm->ttt.lpszText != LPSTR_TEXTCALLBACK) {
                TTSetTipText(pTool, pTool->lpszText);
            }
        }

        if (!pTtm->ttt.lpszText)
            return NULL;

#if defined(WINDOWS_ME)
        //
        // we allow the RtlReading flag ONLY to be changed here.
        //
        if (pTtm->ttt.uFlags & TTF_RTLREADING)
            pTool->uFlags |= TTF_RTLREADING;
        else
            pTool->uFlags &= ~TTF_RTLREADING;
#endif
        if (!HIWORD(pTtm->ttt.lpszText)) {
            id = (UINT)(DWORD)pTtm->ttt.lpszText;
            hinst = pTtm->ttt.hinst;
            pTtm->ttt.lpszText = pTtm->ttt.szText;
            goto LoadFromResource;
        }

        StripAccels(pTtm);

    } else if (pTool->lpszText && !HIWORD(pTool->lpszText)) {
        id = (UINT)(DWORD)pTool->lpszText;
        hinst = pTool->hinst;
        pTtm->ttt.lpszText = pTtm->ttt.szText;

LoadFromResource:
        if (!LoadString(hinst, id, pTtm->ttt.szText, ARRAYSIZE(pTtm->ttt.szText)))
            return NULL;

        StripAccels(pTtm);

    } else  {
    // supplied at creation time.
#ifdef TTDEBUG
        DebugMsg(DM_TRACE, TEXT("GetToolText returns %s"), pTool->lpszText);
#endif
    return pTool->lpszText;
    }
#ifdef TTDEBUG
    DebugMsg(DM_TRACE, TEXT("        **GetToolText returns %s"), pTtm->ttt.lpszText ? pTtm->ttt.lpszText : TEXT("NULL"));
#endif
    return pTtm->ttt.lpszText;
}

void NEAR PASCAL GetToolRect(PTOOLINFO pTool, LPRECT lprc)
{
    if (pTool->uFlags & TTF_IDISHWND) {
        GetWindowRect((HWND)pTool->uId, lprc);
    } else {
        *lprc = pTool->rect;
        MapWindowPoints(pTool->hwnd, HWND_DESKTOP, (LPPOINT)lprc, 2);
    }
}

BOOL NEAR PASCAL PointInTool(PTOOLINFO pTool, HWND hwnd, int x, int y)
{

    // we never care if the point is in a track tool.
    if (pTool->uFlags & TTF_TRACK)
        return FALSE;


    if (pTool->uFlags & TTF_IDISHWND) {
        if (hwnd == (HWND)pTool->uId) {
            return TRUE;
        }
    } else if(hwnd == pTool->hwnd) {
    POINT pt;
    pt.x = x;
    pt.y = y;
    if (PtInRect(&pTool->rect, pt)) {
        return TRUE;
    }
    }
    return FALSE;
}

#ifdef TTDEBUG
void NEAR PASCAL DebugDumpTool(PTOOLINFO pTool)
{
    if (pTool) {
        DebugMsg(DM_TRACE, TEXT("                DumpTool: (%d) hwnd = %d %d, %d %d %d %d"),pTool,
                 pTool->hwnd,
                 (UINT)pTool->uFlags,
                 pTool->rect.left, pTool->rect.top,
                 pTool->rect.right, pTool->rect.bottom);
    } else {
        DebugMsg(DM_TRACE, TEXT("                DumpTool: (NULL)"));
    }
}
#else
#define DebugDumpTool(p)
#endif

PTOOLINFO NEAR PASCAL GetToolAtPoint(PToolTipsMgr pTtm, HWND hwnd, int x, int y, BOOL fCheckText)
{
    PTOOLINFO pToolReturn = NULL;
    PTOOLINFO pTool;

    // short cut..  if we're in the same too, and the bubble is up (not just virtual)
    // return it.  this prevents us from having to poll all the time and
    // prevents us from switching to another tool when this one is good
    if ((pTtm->dwFlags & BUBBLEUP) && PointInTool(pTtm->pCurTool, hwnd, x, y))
        return pTtm->pCurTool;

#ifdef TTDEBUG
    DebugMsg(DM_TRACE, TEXT("******Entering GetToolAtPoint"));
#endif
    if(pTtm->iNumTools) {
    for(pTool = &pTtm->tools[pTtm->iNumTools-1];
        pTool >= pTtm->tools;
        pTool--) {

#ifdef TTDEBUG
            //DebugMsg(DM_TRACE, TEXT("    Point in Tool Check"));
            //DebugDumpTool(pTool);
#endif

        if( PointInTool(pTool, hwnd, x, y) ) {
#ifdef TTDEBUG
                //DebugMsg(DM_TRACE, TEXT("        yes"));
#endif

                // if this tool has text, return it.
                // otherwise, save it away as a dead area tool,
                // and keep looking
                if (fCheckText) {
                    if (GetToolText(pTtm, pTool)) {
#ifdef TTDEBUG
                        //DebugMsg(DM_TRACE, TEXT("            Return! case it Has text"));
                        //DebugDumpTool(pTool);
#endif
                        return pTool;
                    } else if (pTtm->dwFlags & (BUBBLEUP|VIRTUALBUBBLEUP)) {
                        // only return this (only allow a virutal tool
                        // if there was previously a tool up.
                        // IE, we can't start things off with a virutal tool
                        pToolReturn = pTool;
                    }
                } else {
#ifdef TTDEBUG
                    //DebugMsg(DM_TRACE, TEXT("            Return! No text check"));
                    //DebugDumpTool(pTool);
#endif
                    return pTool;
                }
            }
    }
    }
#ifdef TTDEBUG
    DebugMsg(DM_TRACE, TEXT("            Return! no text but returning anyways"));
    DebugDumpTool(pToolReturn);
#endif
    return pToolReturn;
}

void NEAR PASCAL ShowVirtualBubble(PToolTipsMgr pTtm)
{
    PTOOLINFO pTool = pTtm->pCurTool;

    DebugMsg(DM_TRACE, TEXT("Entering ShowVirtualBubble"));
    PopBubble(pTtm);

    // Set this back in so that while we're in this tool's area,
    // we won't keep querying for info
    pTtm->pCurTool = pTool;
    pTtm->dwFlags |= VIRTUALBUBBLEUP;
}

#define TRACK_TOP    0
#define TRACK_LEFT   1
#define TRACK_BOTTOM 2
#define TRACK_RIGHT  3


void NEAR PASCAL TTGetTipPosition(PToolTipsMgr pTtm, LPRECT lprc, int cxText, int cyText)
{
    int iX; // cursor pos
    RECT rcWorkArea;
    int iBubbleWidth =  2*XTEXTOFFSET * g_cyEdge + cxText;
    int iBubbleHeight = 2*YTEXTOFFSET * g_cyEdge + cyText;
    UINT uSide = (UINT)-1;
    RECT rcTool;
#if WINVER >= 0x040A
    HMONITOR hMonitor;
    MONITORINFO mi;
#endif

    GetToolRect(pTtm->pCurTool, &rcTool);

    if (pTtm->pCurTool->uFlags & TTF_TRACK) {

        lprc->left = pTtm->ptTrack.x;
        lprc->top = pTtm->ptTrack.y;

        if (pTtm->pCurTool->uFlags & TTF_CENTERTIP) {
            // center the bubble around the ptTrack
            lprc->left -= (iBubbleWidth / 2);
            lprc->top -=  (iBubbleHeight / 2);
        }

        if (pTtm->pCurTool->uFlags & TTF_ABSOLUTE)
            goto CompleteRect;

        // now align it so that the tip sits beside the rect.
        if (pTtm->ptTrack.y > rcTool.bottom) {

            uSide = TRACK_BOTTOM;
            if (lprc->top < rcTool.bottom)
                lprc->top = rcTool.bottom;

        } else if (pTtm->ptTrack.x < rcTool.left) {

            uSide = TRACK_LEFT;
            if (lprc->left + iBubbleWidth > rcTool.left)
                lprc->left = rcTool.left - iBubbleWidth;

        } else if (pTtm->ptTrack.y < rcTool.top) {

            uSide = TRACK_TOP;
            if (lprc->top + iBubbleHeight > rcTool.top)
                lprc->top = rcTool.top - iBubbleHeight;

        } else {

            uSide = TRACK_RIGHT;
            if (lprc->left < rcTool.right)
                lprc->left = rcTool.right;

        }

    } else if (pTtm->pCurTool->uFlags & TTF_CENTERTIP) {
        lprc->left = (rcTool.right + rcTool.left - cxText)/2;
        lprc->top = rcTool.bottom;
    } else {
        // now set it
        _GetCursorLowerLeft(&lprc->left, &lprc->top);
    }

    // validate the position we got
#if WINVER >= 0x040A
    hMonitor = MonitorFromPoint(*((LPPOINT)lprc), TRUE);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
#endif

#ifndef IEWIN31_25  // There is no tool tray on win 3.1 so use screen limits
    if (GetWindowLong(pTtm->hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
#endif  //IEWIN31_25
    {
        // if we're topmost, our limits are the monitor screen limits
#if WINVER >= 0x040A
        CopyRect(&rcWorkArea, &mi.rcMonitor);
#else
        rcWorkArea.left = 0;
        rcWorkArea.top = 0;
        rcWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
        rcWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
#endif
    }

#ifndef IEWIN31_25  // There is no tool tray on win 3.1
    else
    {
        // otherwise it's the limits of the monitor workarea
#if WINVER >= 0x040A
        CopyRect(&rcWorkArea, &mi.rcWork);
#else
        SystemParametersInfo(SPI_GETWORKAREA, FALSE, &rcWorkArea, 0);
#endif
    }
#endif  //IEWIN31_25

    // move it up if it's at the bottom of the screen
    if ((lprc->top + iBubbleHeight) >= (rcWorkArea.bottom)) {
        if ((uSide == (UINT)-1) || (uSide == TRACK_BOTTOM)) {
            lprc->top =  rcTool.top - iBubbleHeight;
        } else {
            lprc->top = rcWorkArea.bottom - iBubbleHeight;
        }
    }

    if (lprc->top < rcWorkArea.top) {
        if (uSide == TRACK_TOP) {
            lprc->top = rcTool.bottom;
        } else {
            lprc->top = rcWorkArea.top;
        }
    }

    // move it over if it extends past the right.
    if ((lprc->left + iBubbleWidth) >= (rcWorkArea.right)) {
        if (uSide == TRACK_RIGHT) {
            lprc->left = rcTool.left - iBubbleWidth;
        } else {
            // not in right tracking mode, just scoot it over
            lprc->left = rcWorkArea.right - iBubbleWidth - 1;
        }
    }


    iX = lprc->left - rcWorkArea.left;
    if (lprc->left < rcWorkArea.left) {
        if (uSide == TRACK_LEFT) {
            lprc->left = rcTool.right;
        } else {
            lprc->left = rcWorkArea.left;
        }
    }

CompleteRect:
    lprc->right = lprc->left + iBubbleWidth;
    lprc->bottom = lprc->top + iBubbleHeight;
}

void NEAR PASCAL TTGetTipSize(PToolTipsMgr pTtm, LPTSTR lpstr, LPINT pcxText, LPINT pcyText)
{
    HDC hdc  = GetDC(pTtm->hwnd);
    HFONT hOldFont;

    if(pTtm->hFont) hOldFont = SelectObject(hdc, pTtm->hFont);
    MGetTextExtent(hdc, lpstr, lstrlen(lpstr), pcxText, pcyText);
    if(pTtm->hFont) SelectObject(hdc, hOldFont);
    ReleaseDC(pTtm->hwnd, hdc);
}

void NEAR PASCAL DoShowBubble(PToolTipsMgr pTtm)
{
    RECT rc;
    int cxText, cyText;
    LPTSTR lpstr;
    NMHDR nmhdr;

    DebugMsg(DM_TRACE, TEXT("Entering DoShowBubble"));

    lpstr = GetToolText(pTtm, pTtm->pCurTool);

    if (pTtm->dwFlags & TRACKMODE) {

        if (!lpstr || !*lpstr) {
            PopBubble(pTtm);
            pTtm->dwFlags &= ~TRACKMODE;
            return;
        }

    } else {

        TTSetTimer(pTtm, TTT_POP);
        if( !lpstr || !*lpstr ) {

            ShowVirtualBubble(pTtm);
            return;
        }
        TTSetTimer(pTtm, TTT_AUTOPOP);
    }

    // get the size it will be
    TTGetTipSize(pTtm, lpstr, &cxText, &cyText);
    TTGetTipPosition(pTtm, &rc, cxText, cyText);

    SetWindowPos(pTtm->hwnd, NULL, 0, 0,
                 rc.right-rc.left, rc.bottom-rc.top,
                 SWP_NOACTIVATE | SWP_NOMOVE |SWP_NOZORDER);

    // BUGBUG: chicago id was busted.  I *hope* no one relied on it...


    nmhdr.hwndFrom = pTtm->hwnd;
    nmhdr.idFrom = pTtm->pCurTool->uId;
    nmhdr.code = TTN_SHOW;

#ifdef IEWIN31_25
//BUGBUG: this notification is corrupting rc!????
//    if (!SendNotifyEx(pTtm->pCurTool->hwnd, (HWND)-1,
//                      TTN_SHOW, &nmhdr,
//                      (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0))
#else
    if (!SendMessage(pTtm->pCurTool->hwnd, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr))
#endif
    {
        SetWindowPos(pTtm->hwnd, NULL, rc.left, rc.top,
                     0, 0,
                     SWP_NOACTIVATE | SWP_NOSIZE |SWP_NOZORDER);
    }

    SetWindowPos(pTtm->hwnd, NULL, 0,0,0,0,
                 SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);

    pTtm->dwFlags |= BUBBLEUP;
    RedrawWindow(pTtm->hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);

}

void NEAR PASCAL ShowBubbleForTool(PToolTipsMgr pTtm, PTOOLINFO pTool)
{
    // if there's a bubble up for a different tool, pop it.
    if ((pTool != pTtm->pCurTool) && (pTtm->dwFlags & BUBBLEUP)) {
    PopBubble(pTtm);
    }

    // if the bubble was for a different tool, or no bubble, show it
    if ((pTool != pTtm->pCurTool) || !(pTtm->dwFlags & (VIRTUALBUBBLEUP|BUBBLEUP))) {
    pTtm->pCurTool = pTool;
    DoShowBubble(pTtm);
    }
}

void NEAR PASCAL HandleRelayedMessage(PToolTipsMgr pTtm, HWND hwnd, UINT message, LONG lParam)
{
    if (pTtm->dwFlags & TRACKMODE) {
        // punt all messages if we're in track mode
        return;
    }

    switch(message) {
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONUP:
        pTtm->dwFlags &= ~BUTTONISDOWN;
        break;

        // relayed messages
        case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
        pTtm->dwFlags |= BUTTONISDOWN;
        ShowVirtualBubble(pTtm);
        break;

    case WM_NCMOUSEMOVE:
    {
        // convert to client coords
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ScreenToClient(hwnd, &pt);
        lParam = MAKELONG(pt.x, pt.y);
    }
    case WM_MOUSEMOVE: {

            PTOOLINFO pTool;
        // to prevent us from popping up when some
        // other app is active
        if(((!(pTtm->dwStyle & TTS_ALWAYSTIP)) && !(ChildOfActiveWindow(hwnd))) ||
           !(pTtm->dwFlags & ACTIVE) ||
           (pTtm->dwFlags & BUTTONISDOWN))
        break;

            pTool = GetToolAtPoint(pTtm, hwnd, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), FALSE);
        if(pTool) {
                int id;
        // show only if another is showing
        if (pTtm->dwFlags & (VIRTUALBUBBLEUP | BUBBLEUP)) {
            // call show if bubble is up to make sure we're showing
            // for the right tool
                    if (pTool != pTtm->pCurTool) {
                        PopBubble(pTtm);
                        pTtm->pCurTool = pTool;
                        ShowVirtualBubble(pTtm);
                        id = TTT_RESHOW;
                    } else {
                        if (pTtm->idTimer == TTT_RESHOW) {
                            // if the timer is currently waiting to reshow,
                            // don't reset the timer on mouse moves
                            id = 0;
                        } else {
                            // if we're looking to pop the bubble,
                            // any mouse move within the same window
                            // should reset our timer.
                            id = TTT_POP;
                        }
                    }

                    if (pTtm->idtAutoPop)
                        TTSetTimer(pTtm, TTT_AUTOPOP);

        } else {
            pTtm->pCurTool = pTool;
                    id = TTT_INITIAL;
                }

                if (id)
                    TTSetTimer(pTtm, id);

        } else {
        PopBubble(pTtm);
        }
        break;
    }
    }
}

void NEAR PASCAL TTUpdateTipText(PToolTipsMgr pTtm, LPTOOLINFO lpti)
{
    LPTOOLINFO lpTool;

    lpTool = FindTool(pTtm, lpti);
    if (lpTool) {
        lpTool->hinst = lpti->hinst;
        TTSetTipText(lpTool, lpti->lpszText);
        if (lpTool == pTtm->pCurTool) {

            // set the current position to our saved position.
            // ToolHasMoved will return false for us if those this point
            // is no longer within pCurTool's area
            GetCursorPos(&pTtm->pt);
            if (!ToolHasMoved(pTtm)) {
                DoShowBubble(pTtm);
            } else {
                PopBubble(pTtm);
            }
        }
    }
}

void NEAR PASCAL TTSetFont(PToolTipsMgr pTtm, HFONT hFont, BOOL fInval)
{
    ToolTips_NewFont(pTtm, hFont);
    if (fInval)
        InvalidateRect(pTtm->hwnd, NULL, FALSE);
}

void NEAR PASCAL TTSetDelayTime(PToolTipsMgr pTtm, WPARAM wParam, LPARAM lParam)
{
    int iDelayTime = (int)(SHORT)LOWORD(lParam);

    switch (wParam) {

    case TTDT_INITIAL:
        pTtm->iDelayTime = iDelayTime;
        break;

    case TTDT_AUTOPOP:
        pTtm->iAutoPopTime = iDelayTime;
        break;

    case TTDT_RESHOW:
        pTtm->iReshowTime = iDelayTime;
        break;

    case TTDT_AUTOMATIC:
        if (iDelayTime > 0)
        {
            pTtm->iDelayTime = iDelayTime;
            pTtm->iReshowTime = pTtm->iDelayTime / 5;
            pTtm->iAutoPopTime = pTtm->iDelayTime * 10;
        }
        else
        {
            pTtm->iDelayTime = -1;
            pTtm->iReshowTime = -1;
            pTtm->iAutoPopTime = -1;
        }
        break;
    }
}

int NEAR PASCAL TTGetDelayTime(PToolTipsMgr pTtm, WPARAM wParam)
{
    switch (wParam) {

    case TTDT_AUTOMATIC:
    case TTDT_INITIAL:
        return (pTtm->iDelayTime < 0 ? GetDoubleClickTime() : pTtm->iDelayTime);

    case TTDT_AUTOPOP:
        return (pTtm->iAutoPopTime < 0 ? GetDoubleClickTime()*10 : pTtm->iAutoPopTime);

    case TTDT_RESHOW:
        return (pTtm->iReshowTime < 0 ? GetDoubleClickTime()/5 : pTtm->iReshowTime);

    default:
        return -1;
    }
}

#ifdef UNICODE
BOOL NEAR PASCAL CopyToolInfoA(PTOOLINFO pToolSrc, PTOOLINFOA lpTool)
{
    if (pToolSrc && lpTool) {
        if (lpTool->cbSize >= sizeof(TOOLINFOA) - sizeof(LPARAM)) {
            lpTool->uFlags = pToolSrc->uFlags;
            lpTool->hwnd = pToolSrc->hwnd;
            lpTool->uId = pToolSrc->uId;
            lpTool->rect = pToolSrc->rect;
            lpTool->hinst = pToolSrc->hinst;
            if ((pToolSrc->lpszText != LPSTR_TEXTCALLBACK) &&
                HIWORD(pToolSrc->lpszText)) {

                if (lpTool->lpszText) {
                    WideCharToMultiByte (CP_ACP, 0,
                                                 pToolSrc->lpszText,
                                                 -1,
                                                 lpTool->lpszText,
                                                 80, NULL, NULL);
                }
            }
            else
                lpTool->lpszText = (LPSTR)pToolSrc->lpszText;
        }

        if (lpTool->cbSize > (UINT)(&((LPTOOLINFOA)0)->lParam))
            lpTool->lParam = pToolSrc->lParam;

        if (lpTool->cbSize > sizeof(TOOLINFOA))
            return FALSE;

        return TRUE;
    }
    else
        return FALSE;
}
#endif

BOOL NEAR PASCAL CopyToolInfo(PTOOLINFO pToolSrc, LPTOOLINFO lpTool)
{
    if (pToolSrc && lpTool && lpTool->cbSize <= sizeof(TOOLINFO)) {
        if (lpTool->cbSize >= sizeof(TOOLINFO) - sizeof(LPARAM)) {
            lpTool->uFlags = pToolSrc->uFlags;
            lpTool->hwnd = pToolSrc->hwnd;
            lpTool->uId = pToolSrc->uId;
            lpTool->rect = pToolSrc->rect;
            lpTool->hinst = pToolSrc->hinst;
            if ((pToolSrc->lpszText != LPSTR_TEXTCALLBACK) && HIWORD(pToolSrc->lpszText))
            {
                if (lpTool->lpszText)
                    lstrcpy(lpTool->lpszText, pToolSrc->lpszText);
            }
            else
                lpTool->lpszText = pToolSrc->lpszText;
        }
        if (lpTool->cbSize > (UINT)(&((LPTOOLINFO)0)->lParam))
            lpTool->lParam = pToolSrc->lParam;

        if (lpTool->cbSize > sizeof(TOOLINFO))
            return FALSE;

        return TRUE;
    }
    else
        return FALSE;
}

void NEAR PASCAL TTHandleTimer(PToolTipsMgr pTtm, UINT id)
{
    PTOOLINFO pTool;
    HWND hwndPt;
    POINT pt;
    DWORD dwPos;

    // punt all timers in track mode
    if (pTtm->dwFlags & TRACKMODE)
        return;


    switch (id) {

    case TTT_AUTOPOP:
        DebugMsg(DM_TRACE, TEXT("ToolTips: Auto popping"));
        ShowVirtualBubble(pTtm);
        break;

    case TTT_POP:

        // this could be started up again by a slight mouse touch
        if (pTtm->dwFlags & VIRTUALBUBBLEUP) {
            KillTimer(pTtm->hwnd, TTT_POP);
        }

        dwPos = GetMessagePos();
        pt.x = LOWORD(dwPos);
        pt.y = HIWORD(dwPos);
        hwndPt = TTWindowFromPoint(pTtm, &pt);
        ScreenToClient(hwndPt, &pt);
        pTool = GetToolAtPoint(pTtm, hwndPt, pt.x, pt.y, TRUE);
        if ((pTtm->pCurTool != pTool) ||
            ToolHasMoved(pTtm)) {
            PopBubble(pTtm);
        }
        break;

    case TTT_INITIAL:
        if(ToolHasMoved(pTtm)) {
            // this means the timer went off
            // without us getting a mouse move
            // which means they left our tools.
            PopBubble(pTtm);
            break;
        }

        // else fall through

    case TTT_RESHOW:
        dwPos = GetMessagePos();
        pt.x = LOWORD(dwPos);
        pt.y = HIWORD(dwPos);
        hwndPt = TTWindowFromPoint(pTtm, &pt);
        ScreenToClient(hwndPt, &pt);
        pTool = GetToolAtPoint(pTtm, hwndPt, pt.x, pt.y, TRUE);
        if (!pTool) {
            if (pTtm->pCurTool)
                PopBubble(pTtm);
        } else if (pTtm->dwFlags & ACTIVE) {
            if (id == TTT_RESHOW) {
                // this will force a re-show
                pTtm->dwFlags &= ~(BUBBLEUP|VIRTUALBUBBLEUP);
            }
            ShowBubbleForTool(pTtm, pTool);
        }
        break;
    }
}

LRESULT WINAPI ToolTipsWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PTOOLINFO pTool;
    PTOOLINFO pToolSrc;
    LONG lStyle;
    PToolTipsMgr pTtm = (PToolTipsMgr)GetWindowInt(hwnd, 0);
#ifdef UNICODE
    WCHAR     szTextBuffer[80];
    TOOLINFOW ti;

    ti.lpszText = szTextBuffer;
#endif

    switch(uMsg)
    {
    case TTM_ACTIVATE:
        if (wParam) {
        pTtm->dwFlags |= ACTIVE;
        } else {
        PopBubble(pTtm);
        pTtm->dwFlags &= ~(ACTIVE | TRACKMODE);
        }
        break;

    case TTM_SETDELAYTIME:
        TTSetDelayTime(pTtm, wParam, lParam);
        break;

    case TTM_GETDELAYTIME:
        return (LRESULT)(UINT)TTGetDelayTime(pTtm, wParam);

#ifdef UNICODE
    case TTM_ADDTOOLA:
        if (!lParam)
            return FALSE;

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, TRUE)) {
            return 0;
        }
        return AddTool(pTtm, &ti);
#endif

    case TTM_ADDTOOL:
        if (!lParam)
            return FALSE;
        return AddTool(pTtm, (LPTOOLINFO)lParam);

#ifdef UNICODE
        case TTM_DELTOOLA:
            if (!lParam)
                return FALSE;

            if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, FALSE)) {
                break;
            }
            DeleteTool(pTtm, &ti);
            break;
#endif

    case TTM_DELTOOL:
        if (!lParam)
            return FALSE;
        DeleteTool(pTtm, (LPTOOLINFO)lParam);
        break;

#ifdef UNICODE
    case TTM_NEWTOOLRECTA:
        if (!lParam)
            return FALSE;

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, FALSE)) {
            break;
        }
        pTool = FindTool(pTtm, &ti);
        if(pTool) {
            pTool->rect = ((LPTOOLINFOA)lParam)->rect;
        }
        break;
#endif

    case TTM_NEWTOOLRECT:
        if (!lParam)
            return FALSE;

        pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
        if(pTool) {
            pTool->rect = ((LPTOOLINFO)lParam)->rect;
        }
        break;

    case TTM_GETTOOLCOUNT:
        return pTtm->iNumTools;

#ifdef UNICODE
    case TTM_GETTOOLINFOA:
        if (!lParam)
            return FALSE;

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, FALSE)) {
            return FALSE;
        }
        pToolSrc = FindTool(pTtm, &ti);
        return (LRESULT)(UINT)CopyToolInfoA(pToolSrc, (LPTOOLINFOA)lParam);

    case TTM_GETCURRENTTOOLA:
        return (LRESULT)(UINT)CopyToolInfoA(pTtm->pCurTool, (LPTOOLINFOA)lParam);

    case TTM_ENUMTOOLSA:
    {
        if (wParam >= 0 && wParam < (UINT)pTtm->iNumTools) {
            pToolSrc = &pTtm->tools[wParam];
            return (LRESULT)(UINT)CopyToolInfoA(pToolSrc, (LPTOOLINFOA)lParam);
        }
        return FALSE;
    }
#endif

    case TTM_GETTOOLINFO:
        if (!lParam)
            return FALSE;
        pToolSrc = FindTool(pTtm, (LPTOOLINFO)lParam);
        return (LRESULT)(UINT)CopyToolInfo(pToolSrc, (LPTOOLINFO)lParam);

    case TTM_GETCURRENTTOOL:
        return (LRESULT)(UINT)CopyToolInfo(pTtm->pCurTool, (LPTOOLINFO)lParam);

    case TTM_ENUMTOOLS:
    {
        if (wParam >= 0 && wParam < (UINT)pTtm->iNumTools) {
            pToolSrc = &pTtm->tools[wParam];
            return (LRESULT)(UINT)CopyToolInfo(pToolSrc, (LPTOOLINFO)lParam);
        }
        return FALSE;
    }

#ifdef UNICODE
    case TTM_SETTOOLINFOA:
        if (!lParam)
            return FALSE;

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, TRUE))
            return FALSE;

        pTool = FindTool(pTtm, (LPTOOLINFO)&ti);
        if (pTool) {
            TTSetTipText(pTool, NULL);
            hmemcpy(pTool, &ti, ti.cbSize);
            pTool->lpszText = NULL;
            TTSetTipText(pTool, ti.lpszText);
        }

        break;
#endif

    case TTM_SETTOOLINFO:
        if (!lParam)
            return FALSE;
        pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
        if (pTool) {
            TTSetTipText(pTool, NULL);
            hmemcpy(pTool,(LPTOOLINFO)lParam, ((LPTOOLINFO)lParam)->cbSize);
            pTool->lpszText = NULL;
            TTSetTipText(pTool, ((LPTOOLINFO)lParam)->lpszText);

            if (pTool == pTtm->pCurTool) {
                DoShowBubble(pTtm);
            }
        }
        break;

#ifdef UNICODE
    case TTM_HITTESTA:
#define lphitinfoA ((LPHITTESTINFOA)lParam)
        if (!lParam)
            return FALSE;
        pTool = GetToolAtPoint(pTtm, lphitinfoA->hwnd, lphitinfoA->pt.x, lphitinfoA->pt.y, TRUE);
        if (pTool) {
            ThunkToolInfoWtoA(pTool, (LPTOOLINFOA)(&(lphitinfoA->ti)));
            return TRUE;
        }
        return FALSE;
#endif

    case TTM_HITTEST:
#define lphitinfo ((LPHITTESTINFO)lParam)
        if (!lParam)
            return FALSE;
        pTool = GetToolAtPoint(pTtm, lphitinfo->hwnd, lphitinfo->pt.x, lphitinfo->pt.y, TRUE);
        if (pTool) {
            lphitinfo->ti = *pTool;
            return TRUE;
        }
        return FALSE;

#ifdef UNICODE
    case TTM_GETTEXTA: {
        LPWSTR lpszTemp;

        if (!lParam)
            return FALSE;
        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, FALSE)) {
            break;
        }
        pTool = FindTool(pTtm, &ti);
        lpszTemp = GetToolText(pTtm, pTool);
        if (((LPTOOLINFOA)lParam)->lpszText) {
            WideCharToMultiByte (CP_ACP,
                                 0,
                                 lpszTemp,
                                 -1,
                                 (((LPTOOLINFOA)lParam)->lpszText),
                                 80, NULL, NULL);
        }
    }
        break;
#endif

    case TTM_GETTEXT: {
        LPTSTR lpszTemp;
        if (!lParam)
            return FALSE;
        pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
        lpszTemp = GetToolText(pTtm, pTool);
        if (((LPTOOLINFO)lParam)->lpszText) {
            lstrcpy((((LPTOOLINFO)lParam)->lpszText), lpszTemp);
        }
    }
        break;


    case WM_GETTEXTLENGTH:
    case WM_GETTEXT:
    {
        LPTSTR lpszStr;
        if (pTtm->pCurTool &&
            (lpszStr = GetToolText(pTtm, pTtm->pCurTool))) {
            if (lParam)
                lstrcpyn((LPTSTR)lParam, lpszStr, wParam);
            return lstrlen(lpszStr);
        } else {
            return 0;
        }
    }

    case TTM_RELAYEVENT:
#define lpmsg ((LPMSG)lParam)
        if (!lParam)
            return FALSE;
        HandleRelayedMessage(pTtm, lpmsg->hwnd, lpmsg->message, lpmsg->lParam);
#undef lpmsg
        break;

        // this is here for people to subclass and fake out what we
        // think the window from point is.  this facilitates "transparent" windows
        case TTM_WINDOWFROMPOINT: {
            HWND hwndPt = WindowFromPoint(*((POINT FAR *)lParam));
            DebugMsg(DM_TRACE, TEXT("TTM_WINDOWFROMPOINT %x"), hwndPt);
            return (LRESULT)(UINT)hwndPt;
        }

#ifdef UNICODE
        case TTM_UPDATETIPTEXTA:
            if (lParam) {
                if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, TRUE)) {
                    break;
                }
                TTUpdateTipText(pTtm, &ti);
            }
            break;
#endif

    case TTM_UPDATETIPTEXT:
        if (lParam)
            TTUpdateTipText(pTtm, (LPTOOLINFO)lParam);
        break;

    case TTM_TRACKPOSITION:
        if (((int)LOWORD(lParam) != pTtm->ptTrack.x) ||
            ((int)HIWORD(lParam) != pTtm->ptTrack.y))
        {
            pTtm->ptTrack.x = LOWORD(lParam);
            pTtm->ptTrack.y = HIWORD(lParam);

            // if track mode is in effect, update the position
            if ((pTtm->dwFlags & TRACKMODE) &&
                pTtm->pCurTool) {
                DoShowBubble(pTtm);
            }
        }
        break;

    case TTM_TRACKACTIVATE:
        if (pTtm->dwFlags & ACTIVE) {
            if (wParam && lParam)
                wParam = TRACKMODE;
            else
                wParam = 0;

            if ((wParam ^ pTtm->dwFlags) & TRACKMODE) {
                // if the trackmode changes by this..
                PopBubble(pTtm);

                pTtm->dwFlags ^= TRACKMODE;
                if (wParam) {

                    // turning on track mode
                    pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
                    if (pTool) {
                        // only if the tool is found
                        ShowBubbleForTool(pTtm, pTool);
                    }
                }
            }
        }
        return TRUE;

    case TTM_SETTIPBKCOLOR:
        pTtm->clrTipBk = (COLORREF)wParam;
        InvalidateRgn(pTtm->hwnd,NULL,TRUE);
        break;

    case TTM_GETTIPBKCOLOR:
        return (LRESULT)(UINT)pTtm->clrTipBk;

    case TTM_SETTIPTEXTCOLOR:
        InvalidateRgn(pTtm->hwnd,NULL,TRUE);
        pTtm->clrTipText = (COLORREF)wParam;
        break;

    case TTM_GETTIPTEXTCOLOR:
        return (LRESULT)(UINT)pTtm->clrTipText;

        /* uMsgs that REALLY came for me. */
    case WM_CREATE:
        // bugbug, this doesn't belong here, but we don't currently
        // have a way of ensuring that the values are always correct
        InitGlobalMetrics(0);

        lStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        lStyle |= WS_EX_TOOLWINDOW;
        SetWindowLong(hwnd, GWL_EXSTYLE, lStyle);
        SetWindowLong(hwnd, GWL_STYLE,WS_POPUP | WS_BORDER);
        pTtm = ToolTipsMgrCreate((LPCREATESTRUCT)lParam);
        if (!pTtm)
            return -1;
        pTtm->hwnd = hwnd;
        SetWindowInt(hwnd, 0, (int)pTtm);
        TTSetFont(pTtm, 0, FALSE);
        break;

    case WM_TIMER:
        TTHandleTimer(pTtm, wParam);
        break;


    case WM_NCHITTEST:
        if (pTtm && pTtm->pCurTool && (pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
        {
            return HTTRANSPARENT;
        } else {
            goto DoDefault;
        }

    case WM_MOUSEMOVE:
        // the cursor moved onto the tips window.
        if (!(pTtm->dwFlags & TRACKMODE) && pTtm->pCurTool && !(pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
            PopBubble(pTtm);

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (pTtm->pCurTool && (pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            MapWindowPoints(pTtm->hwnd, pTtm->pCurTool->hwnd, &pt, 1);
            SendMessage(pTtm->pCurTool->hwnd, uMsg, wParam, lParam);
        }
        break;

    case WM_SYSCOLORCHANGE:
        ReInitGlobalColors();
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        if (pTtm->fMyFont)
            TTSetFont(pTtm, 0, FALSE);
        break;

    case WM_PAINT: {

        PAINTSTRUCT ps;
        RECT rc;
        LPTSTR lpszStr;

        HDC hdc = BeginPaint(hwnd, &ps);

        if (pTtm->pCurTool &&
            (lpszStr = GetToolText(pTtm, pTtm->pCurTool)) &&
            *lpszStr) {
            HBRUSH hbr;

            SelectObject(hdc, pTtm->hFont);
            GetClientRect(hwnd, &rc);
            SetTextColor(hdc, pTtm->clrTipText);

            hbr = CreateSolidBrush(pTtm->clrTipBk);
            FillRect(hdc, &rc, hbr);
            DeleteObject(hbr);

            SetBkMode(hdc, TRANSPARENT);
            ExtTextOut(hdc,
                       XTEXTOFFSET*g_cxEdge,
                       YTEXTOFFSET*g_cyEdge,
#if defined(WINDOWS_ME)
                       ((pTtm->pCurTool->uFlags & TTF_RTLREADING)
                        ?ETO_RTLREADING :0)|
#endif
                       ETO_CLIPPED, &rc, lpszStr,
                       lstrlen(lpszStr), NULL);
//            DrawEdge(hdc, &rc, BDR_RAISEDOUTER, BF_RECT);

        } else
            PopBubble(pTtm);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_SETFONT:
        TTSetFont(pTtm, (HFONT)wParam, (BOOL)lParam);
        return(TRUE);

    case WM_GETFONT:
        return((LRESULT)(UINT)pTtm->hFont);

    case WM_NOTIFYFORMAT:
        if (lParam == NF_QUERY) {
#ifdef UNICODE
            return NFR_UNICODE;
#else
            return NFR_ANSI;
#endif
        } else if (lParam == NF_REQUERY) {
            int i;

            pTool = &pTtm->tools[0];

            for(i = 0 ; i < pTtm->iNumTools; i++) {
                pTool = &pTtm->tools[i];

                if (SendMessage (pTool->hwnd, WM_NOTIFYFORMAT,
                                 (WPARAM)hwnd, NF_QUERY)) {
                    pTool->uFlags |= TTF_UNICODE;
                }
            }

            return ((pTool->uFlags & TTF_UNICODE) ? NFR_UNICODE : NFR_ANSI);
        }
        return 0;

    case WM_DESTROY: {
        if(pTtm->tools) {
            int i;

            // free the tools
            for(i = 0 ; i < pTtm->iNumTools; i++) {
                TTBeforeFreeTool(&pTtm->tools[i]);
            }

            LocalFree((HANDLE)pTtm->tools);
        }

        TTSetFont(pTtm, (HFONT)1, FALSE); // delete font if we made one.
        LocalFree((HANDLE)pTtm);
        break;
    }

DoDefault:
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

#ifdef UNICODE
//========================================================================
//
// Ansi <=> Unicode Thunk Routines
//
//========================================================================


//*************************************************************
//
//  ThunkToolInfoAtoW()
//
//  Purpose:    Thunks a TOOLINFOA structure to a TOOLINFOW
//              structure.
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//*************************************************************

BOOL ThunkToolInfoAtoW (LPTOOLINFOA lpTiA, LPTOOLINFOW lpTiW, BOOL bThunkText)
{
    int iResult = 1;

    //
    // Copy the constants to the new structure.
    //

    lpTiW->cbSize      = sizeof (TOOLINFOW);
    lpTiW->uFlags      = lpTiA->uFlags;
    lpTiW->hwnd        = lpTiA->hwnd;
    lpTiW->uId         = lpTiA->uId;

    lpTiW->rect.left   = lpTiA->rect.left;
    lpTiW->rect.top    = lpTiA->rect.top;
    lpTiW->rect.right  = lpTiA->rect.right;
    lpTiW->rect.bottom = lpTiA->rect.bottom;

    lpTiW->hinst       = lpTiA->hinst;

    if (bThunkText) {
        //
        // Thunk the string to the new structure.
        // Special case LPSTR_TEXTCALLBACK.
        //

        if (lpTiA->lpszText == LPSTR_TEXTCALLBACKA) {
            lpTiW->lpszText = LPSTR_TEXTCALLBACKW;

        } else if (HIWORD(lpTiA->lpszText)) {

            //
            // It is assumed that lpTiW->lpszText is already setup to
            // a valid buffer, and that buffer is 80 characters.
            // 80 characters is defined in the TOOLTIPTEXT structure.
            //

            iResult = MultiByteToWideChar (CP_ACP, 0, lpTiA->lpszText, -1,
                                           lpTiW->lpszText, 80);
        } else {
            lpTiW->lpszText = (LPWSTR)lpTiA->lpszText;
        }

        //
        // If iResult is 0, and GetLastError returns an error code,
        // then MultiByteToWideChar failed.
        //

        if (!iResult) {
            if (GetLastError()) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

//*************************************************************
//
//  ThunkToolInfoWtoA()
//
//  Purpose:    Thunks a TOOLINFOW structure to a TOOLINFOA
//              structure.
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//*************************************************************

BOOL ThunkToolInfoWtoA (LPTOOLINFOW lpTiW, LPTOOLINFOA lpTiA)
{
    int iResult = 1;

    //
    // Copy the constants to the new structure.
    //

    lpTiA->cbSize      = sizeof (TOOLINFOA);
    lpTiA->uFlags      = lpTiW->uFlags;
    lpTiA->hwnd        = lpTiW->hwnd;
    lpTiA->uId         = lpTiW->uId;

    lpTiA->rect.left   = lpTiW->rect.left;
    lpTiA->rect.top    = lpTiW->rect.top;
    lpTiA->rect.right  = lpTiW->rect.right;
    lpTiA->rect.bottom = lpTiW->rect.bottom;

    lpTiA->hinst       = lpTiW->hinst;

    //
    // Thunk the string to the new structure.
    // Special case LPSTR_TEXTCALLBACK.
    //

    if (lpTiW->lpszText == LPSTR_TEXTCALLBACKW) {
        lpTiA->lpszText = LPSTR_TEXTCALLBACKA;

    } else if (HIWORD(lpTiW->lpszText)) {

        //
        // It is assumed that lpTiA->lpszText is already setup to
        // a valid buffer, and that buffer is 80 characters.
        // 80 characters is defined in the TOOLTIPTEXT structure.
        //

        iResult = WideCharToMultiByte (CP_ACP, 0, lpTiW->lpszText, -1,
                                       lpTiA->lpszText, 80, NULL, NULL);
    } else {
        lpTiA->lpszText = (LPSTR)lpTiW->lpszText;
    }

    //
    // If iResult is 0, and GetLastError returns an error code,
    // then WideCharToMultiByte failed.
    //

    if (!iResult) {
        if (GetLastError()) {
            return FALSE;
        }
    }

    return TRUE;
}


//*************************************************************
//
//  ThunkToolTipTextAtoW()
//
//  Purpose:    Thunks a TOOLTIPTEXTA structure to a TOOLTIPTEXTW
//              structure.
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//*************************************************************

BOOL ThunkToolTipTextAtoW (LPTOOLTIPTEXTA lpTttA, LPTOOLTIPTEXTW lpTttW)
{
    int iResult;

    if (!lpTttA || !lpTttW)
        return FALSE;

    //
    // Thunk the NMHDR structure.
    //

    lpTttW->hdr.hwndFrom = lpTttA->hdr.hwndFrom;
    lpTttW->hdr.idFrom   = lpTttA->hdr.idFrom;
    lpTttW->hdr.code     = TTN_NEEDTEXTW;


    iResult = MultiByteToWideChar (CP_ACP, 0, lpTttA->szText, -1,
                                   lpTttW->szText, 80);
    if (!iResult) {
        if (GetLastError()) {
            return FALSE;
        }
    }

    //
    // Thunk the string to the new structure.
    // Special case LPSTR_TEXTCALLBACK.
    //

    if (lpTttA->lpszText == LPSTR_TEXTCALLBACKA) {
        lpTttW->lpszText = LPSTR_TEXTCALLBACKW;

    } else if (HIWORD(lpTttA->lpszText)) {

        //
        // If lpszText isn't pointing to the
        // szText buffer, then thunk it.
        //

        if (lpTttA->lpszText != lpTttA->szText) {

            //
            // It is assumed that lpTttW->lpszText is already setup to
            // a valid buffer, and that buffer is 80 characters.
            // 80 characters is defined in the TOOLTIPTEXT structure.
            //

            iResult = MultiByteToWideChar (CP_ACP, 0, lpTttA->lpszText, -1,
                                           lpTttW->lpszText, 80);
            if (!iResult) {
                if (GetLastError()) {
                    return FALSE;
                }
            }
        }

    } else {
        lpTttW->lpszText = (LPWSTR)lpTttA->lpszText;
    }

    //
    // Thunk the remaining part of the TOOLTIPTEXTA structure.
    //

    lpTttW->hinst  = lpTttA->hinst;
    lpTttW->uFlags = lpTttA->uFlags;

    return TRUE;
}

//*************************************************************
//
//  ThunkToolTipTextWtoA()
//
//  Purpose:    Thunks a TOOLTIPTEXTW structure to a TOOLTIPTEXTA
//              structure.
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//*************************************************************

BOOL ThunkToolTipTextWtoA (LPTOOLTIPTEXTW lpTttW, LPTOOLTIPTEXTA lpTttA)
{
    int iResult;

    if (!lpTttA || !lpTttW)
        return FALSE;

    //
    // Thunk the NMHDR structure.
    //

    lpTttA->hdr.hwndFrom = lpTttW->hdr.hwndFrom;
    lpTttA->hdr.idFrom   = lpTttW->hdr.idFrom;
    lpTttA->hdr.code     = TTN_NEEDTEXTA;


    iResult = WideCharToMultiByte (CP_ACP, 0, lpTttW->szText, -1,
                                   lpTttA->szText, 80, NULL, NULL);
    if (!iResult) {
        if (GetLastError()) {
            return FALSE;
        }
    }

    //
    // Thunk the string to the new structure.
    // Special case LPSTR_TEXTCALLBACK.
    //

    if (lpTttW->lpszText == LPSTR_TEXTCALLBACKW) {
        lpTttA->lpszText = LPSTR_TEXTCALLBACKA;

    } else if (HIWORD(lpTttW->lpszText)) {

        //
        // If lpszText isn't pointing to the
        // szText buffer, then thunk it.
        //

        if (lpTttW->lpszText != lpTttW->szText) {
            //
            // It is assumed that lpTttA->lpszText is already setup to
            // a valid buffer, and that buffer is 80 characters.
            // 80 characters is defined in the TOOLTIPTEXT structure.
            //

            iResult = WideCharToMultiByte (CP_ACP, 0, lpTttW->lpszText, -1,
                                           lpTttA->lpszText, 80, NULL, NULL);
            if (!iResult) {
                if (GetLastError()) {
                    return FALSE;
                }
            }
        }

    } else {
        lpTttA->lpszText = (LPSTR)lpTttW->lpszText;
    }

    //
    // Thunk the remaining part of the TOOLTIPTEXT structure.
    //

    lpTttA->hinst  = lpTttW->hinst;
    lpTttA->uFlags = lpTttW->uFlags;

    return TRUE;
}


#endif

