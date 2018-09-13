#include "ctlspriv.h"

#define TF_TT 0x10

//#define TTDEBUG

#define ACTIVE          0x10
#define BUTTONISDOWN    0x20
#define BUBBLEUP        0x40
#define VIRTUALBUBBLEUP 0x80  // this is for dead areas so we won't
                                //wait after moving through dead areas
#define TRACKMODE       0x01

#define MAXTIPSIZE       128
#define INITIALTIPSIZE    80
#define XTEXTOFFSET        2
#define YTEXTOFFSET        1
#define XBALLOONOFFSET    10
#define YBALLOONOFFSET     8
#define BALLOON_X_CORNER  13
#define BALLOON_Y_CORNER  13
#define STEMOFFSET        16
#define STEMHEIGHT        20
#define STEMWIDTH         14
#define MINBALLOONWIDTH   30 // min width for stem to show up

#define TTT_INITIAL        1
#define TTT_RESHOW         2
#define TTT_POP            3
#define TTT_AUTOPOP        4

#define TIMEBETWEENANIMATE  2000        // 2 Seconds between animates

#define MAX_TIP_CHARACTERS 100
#define TITLEICON_WIDTH   16
#define TITLEICON_HEIGHT  16
#define TITLEICON_DIST    8     // Distance from Icon to Title
#define TITLE_INFO_DIST   6     // Distance from the Title to the Tip Text
#define MAX_TIP_WIDTH     300   // Seems kind of arbitrary. Width of the tip.


typedef struct tagWIN95TOOLINFO {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT uId;
    RECT rect;
    HINSTANCE hinst;
    LPSTR lpszText;
} WIN95TTTOOLINFO;


/* tooltips.c */

typedef struct {
    CONTROLINFO ci;
    //HWND hwnd;       // in ci
    int iNumTools;
    int iDelayTime;
    int iReshowTime;
    int iAutoPopTime;
    PTOOLINFO tools;
    PTOOLINFO pCurTool;
    BOOL fMyFont;
    HFONT hFont;
    //UINT uiCodePage; // in ci
    DWORD dwFlags;
    //DWORD dwStyle;   // in ci

    // Timer info;
    UINT_PTR idTimer;
    POINT pt;

    UINT_PTR idtAutoPop;

    // Tip buffer
    LPTSTR lpTipText;
    UINT   cchTipText;

    LPTSTR lpTipTitle;
    UINT   cchTipTitle; 
    UINT   uTitleBitmap;
    int    iTitleHeight;
    HIMAGELIST himlTitleBitmaps;

    POINT ptTrack; // the saved track point from TTM_TRACKPOSITION

    BOOL fBkColorSet :1;
    BOOL fTextColorSet :1;
    BOOL fUnderStem : 1;        // true if stem is under the balloon
    BOOL fInWindowFromPoint:1;  // handling a TTM_WINDOWFROMPOINT message
    BOOL fEverShown:1;          // Have we ever been shown before?
    COLORREF clrTipBk;          // This is joeb's idea...he wants it
    COLORREF clrTipText;        // to be able to _blend_ more, so...
    
    int  iMaxTipWidth;          // the maximum tip width
    RECT rcMargin;              // margin offset b/t border and text
    int  iStemHeight;           // balloon mode stem/wedge height
    DWORD dwLastDisplayTime;    // The tick count taken at the last display. Used for animate puroposes.
} CToolTipsMgr, NEAR *PToolTipsMgr;

#define TTToolHwnd(pTool)  ((pTool->uFlags & TTF_IDISHWND) ? (HWND)pTool->uId : pTool->hwnd)
#define IsTextPtr(lpszText)  (((lpszText) != LPSTR_TEXTCALLBACK) && (!IS_INTRESOURCE(lpszText)))

//
// Function prototypes
//
LRESULT WINAPI ToolTipsWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL TTSetDelayTime(PToolTipsMgr pTtm, WPARAM wParam, LPARAM lParam);
int NEAR PASCAL TTGetDelayTime(PToolTipsMgr pTtm, WPARAM wParam);

#ifdef UNICODE
BOOL ThunkToolInfoAtoW (LPTOOLINFOA lpTiA, LPTOOLINFOW lpTiW, BOOL bThunkText, UINT uiCodePage);
BOOL ThunkToolInfoWtoA (LPTOOLINFOW lpTiW, LPTOOLINFOA lpTiA, UINT uiCodePage);
BOOL ThunkToolTipTextAtoW (LPTOOLTIPTEXTA lpTttA, LPTOOLTIPTEXTW lpTttW, UINT uiCodePage);
#endif

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitToolTipsClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    // See if we must register a window class
    if (!GetClassInfo(hInstance, c_szSToolTipsClass, &wc)) {
#ifndef WIN32
    extern LRESULT CALLBACK _ToolTipsWndProc(HWND, UINT, WPARAM, LPARAM);
    wc.lpfnWndProc = _ToolTipsWndProc;
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

void NEAR PASCAL _GetHcursorPdy3(int *pdxRight, int *pdyBottom)
{
    int i;
    int iXOR = 0;
    int dy, dx;
    CURMASK CurMask[16*8];
    ICONINFO iconinfo;
    BITMAP bm;
    HCURSOR hCursor = GetCursor();

    *pdyBottom = 16; //best guess
    *pdxRight = 16;  //best guess
    if (!GetIconInfo(hCursor, &iconinfo))
        return;
    if (!GetObject(iconinfo.hbmMask, sizeof(bm), (LPSTR)&bm))
        return;
    if (!GetBitmapBits(iconinfo.hbmMask, sizeof(CurMask), CurMask))
        return;
    i = (int)(bm.bmWidth * bm.bmHeight / _BitSizeOf(CURMASK) );
    
    if (!iconinfo.hbmColor) 
    {
        // if no color bitmap, then the hbmMask is a double height bitmap
        // with the cursor and the mask stacked.
        iXOR = i - 1;
        i /= 2;    
    } 
    
    if ( i >= sizeof(CurMask)) i = sizeof(CurMask) -1;
    if (iXOR >= sizeof(CurMask)) iXOR = 0;
    
    for (i--; i >= 0; i--)
    {
        if (CurMask[i] != 0xFFFF || (iXOR && (CurMask[iXOR--] != 0)))
            break;
    }
    
    if (iconinfo.hbmColor) DeleteObject(iconinfo.hbmColor);
    if (iconinfo.hbmMask) DeleteObject(iconinfo.hbmMask);

    // Compute the pointer height
    dy = (i + 1) * _BitSizeOf(CURMASK) / (int)bm.bmWidth;
    dx = (i + 1) * _BitSizeOf(CURMASK) / (int)bm.bmHeight;

    // Compute the distance between the pointer's lowest, left, rightmost
    //  pixel and the HotSpotspot
    *pdyBottom = dy - (int)iconinfo.yHotspot;
#if defined(UNIX)
    if ((int)iconinfo.yHotspot > dy)
    {
        *pdyBottom = 16; //best guess
    }
#endif
    *pdxRight  = dx - (int)iconinfo.xHotspot;
}

// this returns the values in work area coordinates because
// that's what set window placement uses
void NEAR PASCAL _GetCursorLowerLeft(int *piLeft, int *piTop, int *piWidth, int *piHeight)
{
    DWORD dwPos;
    
    dwPos = GetMessagePos();
    _GetHcursorPdy3(piWidth, piHeight);
    *piLeft = GET_X_LPARAM(dwPos);
    *piTop  = GET_Y_LPARAM(dwPos) + *piHeight;
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
        hFont = CCCreateStatusFont();
        pTtm->fMyFont = TRUE;
        
        if (!hFont) {
            hFont = g_hfontSystem;
            pTtm->fMyFont = FALSE;
        }
    }

    pTtm->hFont = hFont;
    pTtm->ci.uiCodePage = GetCodePageForFont(hFont);
}

BOOL NEAR PASCAL ChildOfActiveWindow(HWND hwndChild)
{
    HWND hwnd = hwndChild;
    HWND hwndActive = GetForegroundWindow();

    while (hwnd)    {
        if (hwnd == hwndActive)
            return TRUE;
        else
            hwnd = GetParent(hwnd);
    }
    return FALSE;
}

void NEAR PASCAL PopBubble(PToolTipsMgr pTtm)
{
    // we're at least waiting to show;
    DebugMsg(TF_TT, TEXT("PopBubble (killing timer)"));
    if(pTtm->idTimer) {
        KillTimer(pTtm->ci.hwnd, pTtm->idTimer);
        pTtm->idTimer = 0;
    }

    if (pTtm->idtAutoPop) {
        KillTimer(pTtm->ci.hwnd, pTtm->idtAutoPop);
        pTtm->idtAutoPop = 0;
    }


    if (IsWindowVisible(pTtm->ci.hwnd) && pTtm->pCurTool) {
        NMHDR nmhdr;
        nmhdr.hwndFrom = pTtm->ci.hwnd;
        nmhdr.idFrom = pTtm->pCurTool->uId;
        nmhdr.code = TTN_POP;

        SendNotifyEx(pTtm->pCurTool->hwnd, (HWND)-1,
                     TTN_POP, &nmhdr,
                     (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0);
    }

    KillTimer(pTtm->ci.hwnd, TTT_POP);
    ShowWindow(pTtm->ci.hwnd, SW_HIDE);
    pTtm->dwFlags &= ~(BUBBLEUP|VIRTUALBUBBLEUP);
    pTtm->pCurTool = NULL;

}

PToolTipsMgr NEAR PASCAL ToolTipsMgrCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
    PToolTipsMgr pTtm = (PToolTipsMgr)LocalAlloc(LPTR, sizeof(CToolTipsMgr));
    if (pTtm) {

        CIInitialize(&pTtm->ci, hwnd, lpCreateStruct);

        // LPTR zeros the rest of the struct for us
        TTSetDelayTime(pTtm, TTDT_AUTOMATIC, (LPARAM)-1);
        pTtm->dwFlags = ACTIVE;
        pTtm->iMaxTipWidth = -1;
        
        // These are the defaults (straight from cutils.c), 
        // but you can always change them...
        pTtm->clrTipBk = g_clrInfoBk;
        pTtm->clrTipText = g_clrInfoText;

        // Setup the default tooltip text buffer
        pTtm->lpTipText = LocalAlloc (LPTR, INITIALTIPSIZE * sizeof(TCHAR));

        if (pTtm->lpTipText) {
            pTtm->cchTipText = INITIALTIPSIZE;

        } else {
            LocalFree (pTtm);
            pTtm = NULL;
        }
    }
    return pTtm;
}

void NEAR PASCAL TTSetTimer(PToolTipsMgr pTtm, int id)
{
    int iDelayTime = 0;

    if(pTtm->idTimer) {
        KillTimer(pTtm->ci.hwnd, pTtm->idTimer);
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
        pTtm->idtAutoPop = SetTimer(pTtm->ci.hwnd, id, iDelayTime, NULL);
        return;
    }

    
    DebugMsg(TF_TT, TEXT("TTSetTimer %d for %d ms"), id, iDelayTime);
    
    if (SetTimer(pTtm->ci.hwnd, id, iDelayTime, NULL) &&
        (id != TTT_POP)) {
        pTtm->idTimer = id;
        GetCursorPos(&pTtm->pt);
    }
}

//
//  Double-hack to solve blinky-tooltips problems.
//
//  fInWindowFromPoint makes us temporarily transparent.
//
//  Clear the WS_DISABLED flag to trick USER into hit-testing against us.
//  USER by default skips disabled windows.  Restore the flag afterwards.
//  VB in particular likes to run around disabling all top-level windows
//  owned by his process.
//
//  We must use SetWindowBits() instead of EnableWindow() because
//  EnableWindow() will mess with the capture and focus.
//
HWND TTWindowFromPoint(PToolTipsMgr pTtm, LPPOINT ppt)
{
    HWND hwnd;
    DWORD dwStyle;
    dwStyle = SetWindowBits(pTtm->ci.hwnd, GWL_STYLE, WS_DISABLED, 0);
    pTtm->fInWindowFromPoint = TRUE;
    hwnd = (HWND)SendMessage(pTtm->ci.hwnd, TTM_WINDOWFROMPOINT, 0, (LPARAM)ppt);
    pTtm->fInWindowFromPoint = FALSE;
    SetWindowBits(pTtm->ci.hwnd, GWL_STYLE, WS_DISABLED, dwStyle);
    return hwnd;
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
        ((pTtm->ci.style & TTS_ALWAYSTIP) || ChildOfActiveWindow(hwnd)) &&
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
    
    
#ifdef WINDOWS_ME
    if(!(pTtm && lpToolInfo))
    {
    DebugMsg(TF_ALWAYS, TEXT("FindTool passed invalid argumnet. Exiting..."));
    return NULL;
    }
#endif //WINDOWS_ME
    // BUGBUG: in win95, this was NOT validated... by doing so now, we may
    // cause some compat problems... if so, we need to assume for 4.0 marked
    // guys that cbSize == &(0->lParam)
    if (lpToolInfo->cbSize > sizeof(TOOLINFO))
        return NULL;
        
    // you can pass in an index or a toolinfo descriptor
    if (IS_INTRESOURCE(lpToolInfo)) {
        i = PtrToUlong(lpToolInfo);
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


LRESULT WINAPI TTSubclassProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, ULONG_PTR dwRefData);

void NEAR PASCAL TTUnsubclassHwnd(HWND hwnd, HWND hwndTT, BOOL fForce)
{
    ULONG_PTR dwRefs;
    
    if (IsWindow(hwnd) &&
        GetWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT, (PULONG_PTR) &dwRefs))
    {
        if (!fForce && (dwRefs > 1))
            SetWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT, dwRefs - 1);
        else
            RemoveWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT);
    }
}

LRESULT WINAPI TTSubclassProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, ULONG_PTR dwRefData)
{
    if (((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST)) ||
        (message == WM_NCMOUSEMOVE))
    {
        RelayToToolTips((HWND)uIdSubclass, hwnd, message, wParam, lParam);
    }
    else if (message == WM_NCDESTROY)
    {
        TTUnsubclassHwnd(hwnd, (HWND)uIdSubclass, TRUE);
    }

    return DefSubclassProc(hwnd, message, wParam, lParam);
}

void NEAR PASCAL TTSubclassHwnd(PTOOLINFO pTool, HWND hwndTT)
{
    HWND hwnd;
    
    if (IsWindow(hwnd = TTToolHwnd(pTool)))
    {
        ULONG_PTR dwRefs;

        GetWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT, &dwRefs);
        SetWindowSubclass(hwnd, TTSubclassProc, (UINT_PTR)hwndTT, dwRefs + 1);
    }
}
    
    
void NEAR PASCAL TTSetTipText(LPTOOLINFO pTool, LPTSTR lpszText)
{
    // if it wasn't alloc'ed before, set it to NULL now so we'll alloc it
    // otherwise, don't touch it and it will be realloced
    if (!IsTextPtr(pTool->lpszText)) {
        pTool->lpszText = NULL;
    }
    
    if (IsTextPtr(lpszText)) {
        DebugMsg(TF_TT, TEXT("TTSetTipText %s"), lpszText);
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
    PTOOLINFO ptoolsNew;
    LRESULT lResult;

    // bail for right now;
    
    if (lpToolInfo->cbSize > sizeof(TOOLINFO)) {
        ASSERT(0);
        return 0L;
    }

    // on failure to alloc do nothing.
    ptoolsNew = CCLocalReAlloc(pTtm->tools,
                               sizeof(TOOLINFO)*(pTtm->iNumTools+1));
    if ( !ptoolsNew )
            return 0L;
    
    if(pTtm->tools) {
        // realloc could have moved stuff around.  repoint pCurTool
        if (pTtm->pCurTool) {
            pTtm->pCurTool = ((PTOOLINFO)ptoolsNew) + (pTtm->pCurTool - pTtm->tools);
        }
    }
    
    pTtm->tools = ptoolsNew;
    

    pTool = &pTtm->tools[pTtm->iNumTools];
    pTtm->iNumTools++;
    hmemcpy(pTool, lpToolInfo, lpToolInfo->cbSize); 
    pTool->lpszText = NULL;

    //
    // If the tooltip will be displayed within a RTL mirrored window, then
    // simulate mirroring the tooltip. [samera]
    // 
    //
    if (IS_WINDOW_RTL_MIRRORED(lpToolInfo->hwnd) &&
        (!(pTtm->ci.dwExStyle & RTL_MIRRORED_WINDOW)))
    {
        // toggle (mirror) the flags
        pTool->uFlags ^= (TTF_RTLREADING | TTF_RIGHT);
    }

    TTSetTipText(pTool, lpToolInfo->lpszText);
    if (pTool->uFlags & TTF_SUBCLASS) {
        TTSubclassHwnd(pTool, pTtm->ci.hwnd);
    }

    if (!lpToolInfo->hwnd || !IsWindow(lpToolInfo->hwnd)) {
#ifdef UNICODE
        lResult = NFR_UNICODE;
#else
        lResult = NFR_ANSI;
#endif
    } else if (pTool->uFlags & TTF_UNICODE) {
        lResult = NFR_UNICODE;
    } else {
        lResult = SendMessage (pTool->hwnd, WM_NOTIFYFORMAT,
                               (WPARAM)pTtm->ci.hwnd, NF_QUERY);
    }

    if (lResult == NFR_UNICODE) {
        pTool->uFlags |= TTF_UNICODE;
    }

#ifdef TTDEBUG
    DebugMsg(TF_TT, TEXT("Tool Added: ptr=%d, uFlags=%d, wid=%d, hwnd=%d"),
             pTool, pTool->uFlags, pTool->uId, pTool->hwnd);
#endif

    return 1L;
}

void NEAR PASCAL TTBeforeFreeTool(PToolTipsMgr pTtm, LPTOOLINFO pTool)
{
    if (pTool->uFlags & TTF_SUBCLASS) 
        TTUnsubclassHwnd(TTToolHwnd(pTool), pTtm->ci.hwnd, FALSE);

    // clean up
    TTSetTipText(pTool, NULL);
}

void NEAR PASCAL DeleteTool(PToolTipsMgr pTtm, LPTOOLINFO lpToolInfo)
{
    PTOOLINFO pTool;

    // bail for right now;
    if (lpToolInfo->cbSize > sizeof(TOOLINFO)) {
        ASSERT(0);
        return;
    }

    pTool = FindTool(pTtm, lpToolInfo);
    if(pTool) {
        if (pTtm->pCurTool == pTool)
            PopBubble(pTtm);

        TTBeforeFreeTool(pTtm, pTool);

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
    if (!(pTtm->ci.style & TTS_NOPREFIX)) {
        StripAccelerators(pTtm->lpTipText, pTtm->lpTipText, FALSE);
    }
}


//
//  The way we detect if a window is a toolbar or not is by asking it
//  for its MSAA class ID.  We cannot use GetClassWord(GCL_ATOM) because
//  Microsoft LiquidMotion **superclasses** the toolbar, so the classname
//  won't match.
//
#define IsToolbarWindow(hwnd) \
    (SendMessage(hwnd, WM_GETOBJECT, 0, OBJID_QUERYCLASSNAMEIDX) == MSAA_CLASSNAMEIDX_TOOLBAR)

LPTSTR NEAR PASCAL GetToolText(PToolTipsMgr pTtm, PTOOLINFO pTool)
{
    int id;
    HINSTANCE hinst;
    DWORD dwStrLen;
    TOOLTIPTEXT ttt;
    
    if (!pTool)
        return NULL;

#ifdef TTDEBUG
    DebugMsg(TF_TT, TEXT("        **Enter GetToolText: ptr=%d, wFlags=%d, wid=%d, hwnd=%d"),
             pTool, pTool->uFlags, pTool->uId, pTool->hwnd);
#endif

    if (pTtm->lpTipText) {
        *pTtm->lpTipText = TEXT('\0');
    } else {
        pTtm->lpTipText = LocalAlloc (LPTR, INITIALTIPSIZE * sizeof(TCHAR));
        pTtm->cchTipText = INITIALTIPSIZE;
    }

    if (pTool->lpszText == LPSTR_TEXTCALLBACK) {

        ttt.hdr.idFrom = pTool->uId;
        ttt.hdr.code = TTN_NEEDTEXT;
        ttt.hdr.hwndFrom = pTtm->ci.hwnd;

        ttt.szText[0] = TEXT('\0');
        ttt.lpszText = ttt.szText;
        ttt.uFlags = pTool->uFlags;
        ttt.lParam = pTool->lParam;
        ttt.hinst = NULL;

        SendNotifyEx(pTool->hwnd, (HWND) -1,
                     0, (NMHDR FAR *)&ttt,
                     (pTool->uFlags & TTF_UNICODE) ? 1 : 0);

        // APPHACK for Elcom Advanced Disk Catalog and for Microsoft
        // LiquidMotion:
        // they subclass toolbar & expect this notification to
        // be ANSI.  So if the UNICODE notification failed,
        // and our parent is a toolbar, then try again in ANSI.

        if (ttt.lpszText == ttt.szText && ttt.szText[0] == TEXT('\0') &&
            (pTool->uFlags & TTF_UNICODE) && pTtm->ci.iVersion < 5 &&
            IsToolbarWindow(pTool->hwnd)) {
            SendNotifyEx(pTool->hwnd, (HWND) -1,
                     0, (NMHDR FAR *)&ttt,
                     FALSE);
        }

        if (ttt.uFlags & TTF_DI_SETITEM) {
            if (IS_INTRESOURCE(ttt.lpszText)) {
                pTool->lpszText = ttt.lpszText;
                pTool->hinst = ttt.hinst;
            } else if (ttt.lpszText != LPSTR_TEXTCALLBACK) {
                TTSetTipText(pTool, ttt.lpszText);
            }
        }
        
        if (IsFlagPtr(ttt.lpszText))
            return NULL;

#if defined(WINDOWS_ME)
        //
        // we allow the RtlReading flag ONLY to be changed here.
        //
    if (ttt.uFlags & TTF_RTLREADING)
            pTool->uFlags |= TTF_RTLREADING;
    else
            pTool->uFlags &= ~TTF_RTLREADING;
#endif

        if (IS_INTRESOURCE(ttt.lpszText)) {
            id = PtrToUlong(ttt.lpszText);
            hinst = ttt.hinst;
            ttt.lpszText = ttt.szText;
            goto LoadFromResource;
        }
        
        if (*ttt.lpszText == TEXT('\0'))
            return NULL;


        dwStrLen = lstrlen(ttt.lpszText) + 1;
        if (pTtm->cchTipText < dwStrLen)
        {
            LPTSTR psz = LocalReAlloc (pTtm->lpTipText,
                                       dwStrLen * sizeof(TCHAR),
                                       LMEM_MOVEABLE);
            if (psz)
            {
                pTtm->lpTipText = psz;
                pTtm->cchTipText = dwStrLen;
            }
        }

        if (pTtm->lpTipText)
        {
            lstrcpyn(pTtm->lpTipText, ttt.lpszText, pTtm->cchTipText);
        }

#ifdef UNICODE
        //
        //  if ttt.lpszText != ttt.szText and the ttt.uFlags has TTF_MEMALLOCED, then
        //  the ANSI thunk allocated the buffer for us, so free it.
        //

        if ((ttt.lpszText != ttt.szText) && (ttt.uFlags & TTF_MEMALLOCED)) {
            LocalFree (ttt.lpszText);
        }
#endif

        StripAccels(pTtm);

    } else if (pTool->lpszText && IS_INTRESOURCE(pTool->lpszText)) {
        id = PtrToLong(pTool->lpszText);
        hinst = pTool->hinst;

LoadFromResource:

        if (pTtm->lpTipText) {
            if (!LoadString(hinst, id, pTtm->lpTipText, pTtm->cchTipText))
                return NULL;

            StripAccels(pTtm);
        }

    } else  {
        // supplied at creation time.
#ifdef TTDEBUG
        DebugMsg(TF_TT, TEXT("GetToolText returns %s"), pTool->lpszText);
#endif

        if (pTool->lpszText && *pTool->lpszText) {

            dwStrLen = lstrlen(pTool->lpszText) + 1;
            if (pTtm->cchTipText < dwStrLen)
            {
                LPTSTR psz = LocalReAlloc (pTtm->lpTipText,
                                           dwStrLen * sizeof(TCHAR),
                                           LMEM_MOVEABLE);
                if (psz)
                {
                    pTtm->lpTipText = psz;
                    pTtm->cchTipText = dwStrLen;
                }
            }

            if (pTtm->lpTipText) {
                lstrcpyn(pTtm->lpTipText, pTool->lpszText, pTtm->cchTipText);
                StripAccels(pTtm);
            }
        }
    }

#ifdef TTDEBUG
    DebugMsg(TF_TT, TEXT("        **GetToolText returns %s"), pTtm->lpTipText ? pTtm->lpTipText : TEXT("NULL"));
#endif
    return pTtm->lpTipText;
}

LPTSTR NEAR PASCAL GetCurToolText(PToolTipsMgr pTtm)
{
    LPTSTR psz = NULL;
    if (pTtm->pCurTool)
        psz = GetToolText(pTtm, pTtm->pCurTool);

    // this could have changed during the WM_NOTIFY back
    if (!pTtm->pCurTool)
        psz = NULL;
    
    return psz;
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
    // We never care if the point is in a track tool or we're using
    // a hit-test.
    if (pTool->uFlags & (TTF_TRACK | TTF_USEHITTEST))
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
        DebugMsg(TF_TT, TEXT("                DumpTool: (%d) hwnd = %d %d, %d %d %d %d"),pTool,
                 pTool->hwnd,
                 (UINT)pTool->uFlags,
                 pTool->rect.left, pTool->rect.top,
                 pTool->rect.right, pTool->rect.bottom);
    } else {
        DebugMsg(TF_TT, TEXT("                DumpTool: (NULL)"));
    }
}
#else
#define DebugDumpTool(p)
#endif

#define HittestInTool(pTool, hwnd, ht) \
    ((pTool->uFlags & TTF_USEHITTEST) && pTool->hwnd == hwnd && ht == pTool->rect.left)

PTOOLINFO NEAR PASCAL GetToolAtPoint(PToolTipsMgr pTtm, HWND hwnd, int x, int y, 
        int ht, BOOL fCheckText)
{
    PTOOLINFO pToolReturn = NULL;
    PTOOLINFO pTool;

    // short cut..  if we're in the same too, and the bubble is up (not just virtual)
    // return it.  this prevents us from having to poll all the time and
    // prevents us from switching to another tool when this one is good
    if ((pTtm->dwFlags & BUBBLEUP) && pTtm->pCurTool != NULL &&
        (HittestInTool(pTtm->pCurTool, hwnd, ht) ||
         PointInTool(pTtm->pCurTool, hwnd, x, y)))
    {
        return pTtm->pCurTool;
    }

#ifdef TTDEBUG
    DebugMsg(TF_TT, TEXT("******Entering GetToolAtPoint"));
#endif
    if(pTtm->iNumTools) {
        for(pTool = &pTtm->tools[pTtm->iNumTools-1];
            pTool >= pTtm->tools;
            pTool--) {

#ifdef TTDEBUG
            //DebugMsg(TF_TT, TEXT("    Point in Tool Check"));
            //DebugDumpTool(pTool);
#endif

        if(HittestInTool(pTool, hwnd, ht) || PointInTool(pTool, hwnd, x, y)) {
#ifdef TTDEBUG
                //DebugMsg(TF_TT, TEXT("        yes"));
#endif

                // if this tool has text, return it.
                // otherwise, save it away as a dead area tool,
                // and keep looking
                if (fCheckText) {
                    if (GetToolText(pTtm, pTool)) {
#ifdef TTDEBUG
                        //DebugMsg(TF_TT, TEXT("            Return! case it Has text"));
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
                    //DebugMsg(TF_TT, TEXT("            Return! No text check"));
                    //DebugDumpTool(pTool);
#endif
                    return pTool;
                }
            }
    }
    }
#ifdef TTDEBUG
    DebugMsg(TF_TT, TEXT("            Return! no text but returning anyways"));
    DebugDumpTool(pToolReturn);
#endif
    return pToolReturn;
}

void NEAR PASCAL ShowVirtualBubble(PToolTipsMgr pTtm)
{
    PTOOLINFO pTool = pTtm->pCurTool;

    DebugMsg(TF_TT, TEXT("Entering ShowVirtualBubble so popping bubble"));
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


void NEAR PASCAL TTGetTipPosition(PToolTipsMgr pTtm, LPRECT lprc, int cxText, int cyText, int *pxStem, int *pyStem)
{
    RECT rcWorkArea;
    // ADJUSTRECT!  Keep TTAdjustRect and TTM_GETBUBBLESIZE in sync.
    int cxMargin = pTtm->rcMargin.left + pTtm->rcMargin.right;
    int cyMargin = pTtm->rcMargin.top + pTtm->rcMargin.bottom;
    int iBubbleWidth =  2*XTEXTOFFSET * g_cxBorder + cxText + cxMargin;
    int iBubbleHeight = 2*YTEXTOFFSET * g_cyBorder + cyText + cyMargin;
    UINT uSide = (UINT)-1;
    RECT rcTool;
    MONITORINFO mi;
    HMONITOR    hMonitor;
    POINT pt;
    BOOL bBalloon = pTtm->ci.style & TTS_BALLOON;
    int  xStem, yStem;
    int iCursorHeight=0;
    int iCursorWidth=0;
        
    if (bBalloon  || pTtm->cchTipTitle)
    {
        // ADJUSTRECT!  Keep TTAdjustRect and TTM_GETBUBBLESIZE in sync.
        iBubbleWidth += 2*XBALLOONOFFSET;
        iBubbleHeight += 2*YBALLOONOFFSET;

        if (bBalloon)
        {
            if (iBubbleWidth < MINBALLOONWIDTH)
                pTtm->iStemHeight = 0;
            else
            {
                pTtm->iStemHeight = STEMHEIGHT;
                if (pTtm->iStemHeight > iBubbleHeight/3)
                    pTtm->iStemHeight = iBubbleHeight/3; // don't let the stem be longer than the bubble -- looks ugly
            }
        }
    }
    
    GetToolRect(pTtm->pCurTool, &rcTool);
    
    if (pTtm->pCurTool->uFlags & TTF_TRACK) {

        lprc->left = pTtm->ptTrack.x;
        lprc->top = pTtm->ptTrack.y;
        if (bBalloon)
        {
            // adjust the desired left hand side
            xStem = pTtm->ptTrack.x;
            yStem = pTtm->ptTrack.y;
        }

        // BUGBUG: should we not do this in case of TTS_BALLOON?
        if (pTtm->pCurTool->uFlags & TTF_CENTERTIP) {
            // center the bubble around the ptTrack
            lprc->left -= (iBubbleWidth / 2);
            if (!bBalloon)
                lprc->top -=  (iBubbleHeight / 2);
        }
        
        if (pTtm->pCurTool->uFlags & TTF_ABSOLUTE)
        {
            // with goto bellow we'll skip adjusting
            // bubble height -- so do it here
            if (bBalloon)
                iBubbleHeight += pTtm->iStemHeight;
            goto CompleteRect;
        }

        // in balloon style the positioning depends on the position
        // of the stem and we don't try to position the tooltip
        // next to the tool rect
        if (!bBalloon)
        {
            // now align it so that the tip sits beside the rect.
            if (pTtm->ptTrack.y > rcTool.bottom) 
            {
                uSide = TRACK_BOTTOM;
                if (lprc->top < rcTool.bottom)
                    lprc->top = rcTool.bottom;    
            }
            else if (pTtm->ptTrack.x < rcTool.left) 
            {
                uSide = TRACK_LEFT;
                if (lprc->left + iBubbleWidth > rcTool.left)
                    lprc->left = rcTool.left - iBubbleWidth;
            } 
            else if (pTtm->ptTrack.y < rcTool.top) 
            {    
                uSide = TRACK_TOP;
                if (lprc->top + iBubbleHeight > rcTool.top) 
                    lprc->top = rcTool.top - iBubbleHeight;    
            } 
            else 
            {    
                uSide = TRACK_RIGHT;
                if (lprc->left < rcTool.right)
                    lprc->left = rcTool.right;
            }
        }        
    } 
    else if (pTtm->pCurTool->uFlags & TTF_CENTERTIP) 
    {
        lprc->left = (rcTool.right + rcTool.left - iBubbleWidth)/2;
        lprc->top = rcTool.bottom;
        if (bBalloon)
        {
            xStem = (rcTool.left + rcTool.right)/2;
            yStem = rcTool.bottom;
        }
    } 
    else 
    {
        // now set it
        _GetCursorLowerLeft((LPINT)&lprc->left, (LPINT)&lprc->top, &iCursorWidth, &iCursorHeight);
        if (bBalloon)
        {
            HMONITOR  hMon1, hMon2;
            POINT     pt;
            BOOL      bOnSameMonitor = FALSE;
            int iTop = lprc->top - (iCursorHeight + iBubbleHeight + pTtm->iStemHeight);

            xStem = lprc->left;
            yStem = lprc->top;

            pt.x = xStem;
            pt.y = lprc->top;
            hMon1 = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
            pt.y = iTop;
            hMon2 = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

            if (hMon1 == hMon2)
            {
                // the hmons are the same but maybe iTop is off any monitor and we just defaulted 
                // to the nearest one -- check if it's really on the monitor
                mi.cbSize = sizeof(mi);
                GetMonitorInfo(hMon1, &mi);

                if (PtInRect(&mi.rcMonitor, pt))
                {
                    // we'd like to show balloon above the cursor so that wedge/stem points
                    // to tip of the cursor not its bottom left corner
                    yStem -= iCursorHeight;
                    lprc->top = iTop;
                    bOnSameMonitor = TRUE;
                }   
            }

            if (!bOnSameMonitor)
            {
                xStem += iCursorWidth/2;
                iCursorHeight = iCursorWidth = 0;
            }
        }
    }

    //
    //  At this point, (lprc->left, lprc->top) is the position
    //  at which we would prefer that the tooltip appear.
    //
    if (bBalloon)
    {
        // adjust the left point now that all calculations are done
        // but only if we're not in the center tip mode
        // note we use height as width so we can have 45 degree angle that looks nice
        if (!(pTtm->pCurTool->uFlags & TTF_CENTERTIP) && iBubbleWidth > STEMOFFSET + pTtm->iStemHeight)
            lprc->left -= STEMOFFSET;
        // adjust the height to include stem
        iBubbleHeight += pTtm->iStemHeight;
    }

    pt.x = lprc->left;
    pt.y = lprc->top;
    hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    
    if (GetWindowLong(pTtm->ci.hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
    {
        CopyRect(&rcWorkArea, &mi.rcMonitor);
    } else {
        CopyRect(&rcWorkArea, &mi.rcWork);
    }

    //
    //  At this point, rcWorkArea is the rectangle within which
    //  the tooltip should finally appear.
    //
    //  Now fiddle with the coordinates to try to find a sane location
    //  for the tip.
    //


    // move it up if it's at the bottom of the screen
    if ((lprc->top + iBubbleHeight) >= (rcWorkArea.bottom)) {
        if (uSide == TRACK_BOTTOM) 
            lprc->top = rcTool.top - iBubbleHeight;     // flip to top
        else 
        {
            //
            //  We can't "stick to bottom" because that would cause
            //  our tooltip to lie under the mouse cursor, causing it
            //  to pop immediately!  So go just above the mouse cursor.
            //
            // cannot do that in the track mode -- tooltip randomly on the 
            // screen, not even near the button
            //
            // BUGBUG raymondc v6: This screws up Lotus SmartCenter.
            // Need to be smarter about when it is safe to flip up.
            // Perhaps by checking if the upflip would put the tip too
            // far away from the mouse.
            if (pTtm->pCurTool->uFlags & TTF_TRACK)
                lprc->top = pTtm->ptTrack.y - iBubbleHeight;
            else
            {            
                int y = GET_Y_LPARAM(GetMessagePos());
                lprc->top = y - iBubbleHeight;
                if (bBalloon)
                    yStem = y;
            }
        }
    }
    
    // If above the top of the screen...
    if (lprc->top < rcWorkArea.top) 
    {
        if (uSide == TRACK_TOP) 
            lprc->top = rcTool.bottom;      // flip to bottom
        else 
            lprc->top = rcWorkArea.top;     // stick to top
    }

    // move it over if it extends past the right.
    if ((lprc->left + iBubbleWidth) >= (rcWorkArea.right)) 
    {
        // flipping is not the right thing to do with balloon style
        // because the wedge/stem can stick out of the window and 
        // would therefore be clipped so
        if (bBalloon)
        {
            // move it to the left so that stem appears on the right side of the balloon
            // again we use height as width so we can have 45 degree angle
            if (iBubbleWidth >= MINBALLOONWIDTH)
                lprc->left = xStem + min(STEMOFFSET, (iBubbleWidth-pTtm->iStemHeight)/2) - iBubbleWidth;
            // are we still out?
            if (lprc->left + iBubbleWidth >= rcWorkArea.right)
                lprc->left = rcWorkArea.right - iBubbleWidth - 1;
        }
        else if (uSide == TRACK_RIGHT) 
            lprc->left = rcTool.left - iBubbleWidth;    // flip to left
        else 
            // not in right tracking mode, just scoot it over
            lprc->left = rcWorkArea.right - iBubbleWidth - 1; // stick to right
    }

    // if too far left...
    if (lprc->left < rcWorkArea.left) 
    {
        if (uSide == TRACK_LEFT)
        {
            // flipping is not the right thing to do with balloon style
            // because the wedge/stem can stick out of the window and 
            // would therefore be clipped so
            if (bBalloon)
                lprc->left = rcWorkArea.left; //pTtm->ptTrack.x;
            else
                lprc->left = rcTool.right;          // flip to right
        }
        else 
            lprc->left = rcWorkArea.left;       // stick to left
    }
    
CompleteRect:
    lprc->right = lprc->left + iBubbleWidth;
    lprc->bottom = lprc->top + iBubbleHeight;
    if (bBalloon && pxStem && pyStem)
    {
        *pxStem = xStem;
        *pyStem = yStem;
    }
}

BOOL TTCreateTitleBitmaps(PToolTipsMgr pTtm)
{
    if (pTtm->himlTitleBitmaps)
        return TRUE;

    pTtm->himlTitleBitmaps = ImageList_Create(TITLEICON_WIDTH, TITLEICON_HEIGHT, ILC_COLOR24 | ILC_MASK, 3, 1);
    if (pTtm->himlTitleBitmaps)
    {
        HICON hicon;
        
        hicon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_TITLE_INFO), IMAGE_ICON, 
                  TITLEICON_WIDTH, TITLEICON_HEIGHT, LR_DEFAULTCOLOR);
        ImageList_AddIcon(pTtm->himlTitleBitmaps, hicon);
        DestroyIcon(hicon);
        hicon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_TITLE_WARNING), IMAGE_ICON, 
                  TITLEICON_WIDTH, TITLEICON_HEIGHT, LR_DEFAULTCOLOR);
        ImageList_AddIcon(pTtm->himlTitleBitmaps, hicon);
        DestroyIcon(hicon);
        hicon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_TITLE_ERROR), IMAGE_ICON, 
                  TITLEICON_WIDTH, TITLEICON_HEIGHT, LR_DEFAULTCOLOR);
        ImageList_AddIcon(pTtm->himlTitleBitmaps, hicon);
        DestroyIcon(hicon);
        return TRUE;
    }

    return FALSE;
}

// Called when caclulating the size of a "titled tool tip" or actually drawing
// based on the boolean value bCalcRect.
BOOL TTRenderTitledTip(PToolTipsMgr pTtm, HDC hdc, BOOL bCalcRect, RECT* prc, UINT uDrawFlags)
{
    RECT rc;
    int lWidth=0, lHeight=0;
    HFONT hfont;
    COLORREF crOldTextColor;
    int iOldBKMode;

    // If we don't have a title, we don't need to be here.
    if (pTtm->cchTipTitle == 0)
        return FALSE;

    CopyRect(&rc, prc);
    if (pTtm->uTitleBitmap != TTI_NONE)
    {
        lWidth    = TITLEICON_WIDTH + TITLEICON_DIST;
        lHeight  += TITLEICON_HEIGHT;
        if (!bCalcRect && pTtm->himlTitleBitmaps)
        {
            ImageList_Draw(pTtm->himlTitleBitmaps, pTtm->uTitleBitmap - 1, hdc, rc.left, rc.top, ILD_TRANSPARENT);
        }
        rc.left  += lWidth;
    }

    if (!bCalcRect)
    {
        crOldTextColor = SetTextColor(hdc, pTtm->clrTipText);
        iOldBKMode = SetBkMode(hdc, TRANSPARENT);
    }
    
    if (pTtm->lpTipTitle[0] != TEXT('\0'))
    {
        LOGFONT lf;
        HFONT   hfTitle;
        UINT    uFlags = uDrawFlags | DT_SINGLELINE; // title should be on one line only

        hfont = GetCurrentObject(hdc, OBJ_FONT);
        GetObject(hfont, sizeof(lf), &lf);
        lf.lfWeight = FW_BOLD;
        hfTitle = CreateFontIndirect(&lf);
        // hfont should already be set to this
        hfont = SelectObject(hdc, hfTitle);

        // drawtext does not calculate the height if these are specified
        if (!bCalcRect)
            uFlags |= DT_BOTTOM;

        // we need to calc title height -- either we did it before or we'll do it now
        ASSERT(pTtm->iTitleHeight != 0 || uFlags & DT_CALCRECT);

        // adjust the rect so we can stick the title to the bottom of it
        rc.bottom = rc.top + max(pTtm->iTitleHeight, TITLEICON_HEIGHT);
        // problems in DrawText if margins make rc.right < rc.left
        // even though we are asking for calculation of the rect nothing happens, so ...
        if (bCalcRect)
            rc.right = rc.left + MAX_TIP_WIDTH;

        DrawText(hdc, pTtm->lpTipTitle, lstrlen(pTtm->lpTipTitle), &rc, uFlags);

        if (pTtm->iTitleHeight == 0)
            pTtm->iTitleHeight = RECTHEIGHT(rc);    // Use rc instead of lfHeight, because it can be Negative.

        lHeight  = max(lHeight, pTtm->iTitleHeight) + TITLE_INFO_DIST;
        lWidth  += RECTWIDTH(rc);
        
        SelectObject(hdc, hfont);
        DeleteObject(hfTitle);
    }

    // adjust the rect for the info text
    CopyRect(&rc, prc);
    rc.top += lHeight;

    // we want multi line text -- tooltip will give us single line if we did not set MAXWIDTH
    uDrawFlags &= ~DT_SINGLELINE;
    DrawText(hdc, pTtm->lpTipText, lstrlen(pTtm->lpTipText), &rc, uDrawFlags);
    lHeight += RECTHEIGHT(rc);
    lWidth   = max(lWidth, RECTWIDTH(rc));

    if (bCalcRect)
    {
        prc->right = prc->left + lWidth;
        prc->bottom = prc->top + lHeight;
    }
    else
    {
        SetTextColor(hdc, crOldTextColor);
        SetBkMode(hdc, iOldBKMode);
    }

    return TRUE;
}

void NEAR PASCAL TTGetTipSize(PToolTipsMgr pTtm, PTOOLINFO pTool,LPTSTR lpstr, LPINT pcxText, LPINT pcyText)
{

    // get the size it will be
    HDC hdc  = GetDC(pTtm->ci.hwnd);
    HFONT hOldFont;
    
    if(pTtm->hFont) hOldFont = SelectObject(hdc, pTtm->hFont);

    /* If need to fire off the pre-DrawText notify then do so, otherwise use the
       original implementation that just called MGetTextExtent */


    {
        NMTTCUSTOMDRAW nm;
        DWORD dwCustom;
        UINT  uDefDrawFlags = 0;

        nm.nmcd.hdr.hwndFrom = pTtm->ci.hwnd;
        nm.nmcd.hdr.idFrom = pTool->uId;
        nm.nmcd.hdr.code = NM_CUSTOMDRAW;
        nm.nmcd.hdc = hdc;
        // TTGetTipSize must use CDDS_PREPAINT so the client can tell
        // whether we are measuring or painting
        nm.nmcd.dwDrawStage = CDDS_PREPAINT;
        nm.nmcd.rc.left = nm.nmcd.rc.top = 0;

        if (pTtm->ci.style & TTS_NOPREFIX)
            uDefDrawFlags = DT_NOPREFIX;

        if (pTtm->iMaxTipWidth == -1) 
        {
            uDefDrawFlags |= DT_CALCRECT|DT_SINGLELINE |DT_LEFT;
            MGetTextExtent(hdc, lpstr, -1, pcxText, pcyText);
            nm.nmcd.rc.right = *pcxText;
            nm.nmcd.rc.bottom = *pcyText;
            
        }
        else 
        {    
            uDefDrawFlags |= DT_CALCRECT | DT_LEFT | DT_WORDBREAK | DT_EXPANDTABS | DT_EXTERNALLEADING;
            nm.nmcd.rc.right = pTtm->iMaxTipWidth;
            nm.nmcd.rc.bottom = 0;
            DrawText( hdc, lpstr, lstrlen(lpstr), &nm.nmcd.rc, uDefDrawFlags );
            *pcxText = nm.nmcd.rc.right;
            *pcyText = nm.nmcd.rc.bottom;
        }

#if defined(WINDOWS_ME)
        if ( (pTtm->pCurTool->uFlags & TTF_RTLREADING) || (pTtm->ci.dwExStyle & WS_EX_RTLREADING) )
            uDefDrawFlags |= DT_RTLREADING;
#endif
        //
        // Make it right aligned, if requested. [samera]
        //
        if (pTool->uFlags & TTF_RIGHT)
            uDefDrawFlags |= DT_RIGHT;

        nm.uDrawFlags = uDefDrawFlags;

        dwCustom = (DWORD)SendNotifyEx(pTool->hwnd, (HWND) -1,
                     0, (NMHDR*) &nm,
                     (pTool->uFlags & TTF_UNICODE) ? 1 : 0);

        if (TTRenderTitledTip(pTtm, hdc, TRUE, &nm.nmcd.rc, uDefDrawFlags))
        {
            *pcxText = nm.nmcd.rc.right - nm.nmcd.rc.left;
            *pcyText = nm.nmcd.rc.bottom - nm.nmcd.rc.top;
        }
        else if ((dwCustom & CDRF_NEWFONT) || nm.uDrawFlags != uDefDrawFlags)
        {
            DrawText( hdc, lpstr, lstrlen(lpstr), &nm.nmcd.rc, nm.uDrawFlags );

            *pcxText = nm.nmcd.rc.right - nm.nmcd.rc.left;
            *pcyText = nm.nmcd.rc.bottom - nm.nmcd.rc.top;
        }
        // did the owner specify the size?
        else if (pTtm->ci.iVersion >= 5 && (nm.nmcd.rc.right - nm.nmcd.rc.left != *pcxText || 
                                            nm.nmcd.rc.bottom - nm.nmcd.rc.top != *pcyText))
        {
            *pcxText = nm.nmcd.rc.right - nm.nmcd.rc.left;
            *pcyText = nm.nmcd.rc.bottom - nm.nmcd.rc.top;
        }

        // notify parent afterwards if they want us to
        if (!(dwCustom & CDRF_SKIPDEFAULT) &&
            dwCustom & CDRF_NOTIFYPOSTPAINT) {
            nm.nmcd.dwDrawStage = CDDS_POSTPAINT;
            SendNotifyEx(pTool->hwnd, (HWND) -1,
                         0, (NMHDR*) &nm,
                         (pTool->uFlags & TTF_UNICODE) ? 1 : 0);
        }

    }


    if(pTtm->hFont) SelectObject(hdc, hOldFont);
    ReleaseDC(pTtm->ci.hwnd, hdc);

    // after the calc rect, add a little space on the right
    *pcxText += g_cxEdge;
    *pcyText += g_cyEdge;
}

//
//  Given an inner rectangle, return the coordinates of the outer,
//  or vice versa.
//
//  "outer rectangle" = window rectangle.
//  "inner rectangle" = the area where we draw the text.
//
//  This allows people like listview and treeview to position
//  the tooltip so the inner rectangle exactly coincides with
//  their existing text.
//
//  All the places we do rectangle adjusting are marked with
//  the comment
//
//      // ADJUSTRECT!  Keep TTAdjustRect in sync.
//
LRESULT TTAdjustRect(PToolTipsMgr pTtm, BOOL fLarger, LPRECT prc)
{
    RECT rc;

    if (!prc)
        return 0;

    //
    //  Do all the work on our private little rectangle on the
    //  assumption that everything is getting bigger.  At the end,
    //  we'll flip all the numbers around if in fact we're getting
    //  smaller.
    //
    rc.top = rc.left = rc.bottom = rc.right = 0;

    // TTRender adjustments -
    rc.left   -= XTEXTOFFSET*g_cxBorder + pTtm->rcMargin.left;
    rc.right  += XTEXTOFFSET*g_cxBorder + pTtm->rcMargin.right;
    rc.top    -= YTEXTOFFSET*g_cyBorder + pTtm->rcMargin.top;
    rc.bottom += YTEXTOFFSET*g_cyBorder + pTtm->rcMargin.bottom;

    // Compensate for the hack in TTRender that futzes all the rectangles
    // by one pixel.  Look for "Account for off-by-one."
    rc.bottom--;
    rc.right--;

    if (pTtm->ci.style & TTS_BALLOON || pTtm->cchTipTitle)
    {
        InflateRect(&rc, XBALLOONOFFSET, YBALLOONOFFSET);
    }

    //
    //  Ask Windows how much adjusting he will do to us.
    //
    //  Since we don't track WM_STYLECHANGED/GWL_EXSTYLE, we have to ask USER
    //  for our style information, since the app may have changed it.
    //
    AdjustWindowRectEx(&rc,
                       pTtm->ci.style,
                       BOOLFROMPTR(GetMenu(pTtm->ci.hwnd)),
                       GetWindowLong(pTtm->ci.hwnd, GWL_EXSTYLE));

    //
    //  Now adjust our caller's rectangle.
    //
    if (fLarger)
    {
        prc->left   += rc.left;
        prc->right  += rc.right;
        prc->top    += rc.top;
        prc->bottom += rc.bottom;
    }
    else
    {
        prc->left   -= rc.left;
        prc->right  -= rc.right;
        prc->top    -= rc.top;
        prc->bottom -= rc.bottom;
    }

    return TRUE;
}

#define CSTEMPOINTS 3
// bMirrored does not mean a mirrored tooltip.
// It means simulating the behavior or a mirrored tooltip for a tooltip created with a mirrored parent.
HRGN CreateBalloonRgn(int xStem, int yStem, int iWidth, int iHeight, int iStemHeight, BOOL bUnderStem, BOOL bMirrored)
{
    int  y = 0, yHeight = iHeight;
    HRGN rgn;

    if (bUnderStem)
        yHeight -= iStemHeight;
    else
        y = iStemHeight;
        
    rgn = CreateRoundRectRgn(0, y, iWidth, yHeight, BALLOON_X_CORNER, BALLOON_Y_CORNER);
    if (rgn)
    {
        // create wedge/stem rgn
        if (iWidth >= MINBALLOONWIDTH)
        {
            HRGN rgnStem;
            POINT aptStemRgn[CSTEMPOINTS];
            POINT *ppt = aptStemRgn;
            POINT pt;
            BOOL  bCentered;
            int   iStemWidth = iStemHeight+1; // for a 45 degree angle

            // we center the stem if we have TTF_CENTERTIP or the width
            // of the balloon is not big enough to offset the stem by 
            // STEMOFFSET
            // can't quite center the tip on TTF_CENTERTIP because it may be
            // moved left or right it did not fit on the screen: just check
            // if xStem is in the middle
            bCentered = (xStem == iWidth/2) || (iWidth < 2*STEMOFFSET + iStemWidth);

            if (bCentered)
                pt.x = (iWidth - iStemWidth)/2;
            else if (xStem > iWidth/2)
            {
                if(bMirrored)
                {
                    pt.x = STEMOFFSET + iStemWidth;
                }
                else
                {
                    pt.x = iWidth - STEMOFFSET - iStemWidth;
                }    
            }    
            else
            {
                if(bMirrored)
                {
                    pt.x = iWidth - STEMOFFSET;
                }
                else
                {
                    pt.x = STEMOFFSET;
                }    
            }    

            if (bMirrored && (ABS(pt.x - (iWidth - xStem)) <= 2))
            {
                pt.x = iWidth - xStem; // avoid rough edges, have a straight line
                
            }
            else if (!bMirrored && (ABS(pt.x - xStem) <= 2))
            {
                pt.x = xStem; // avoid rough edges, have a straight line
            }    
            if (bUnderStem)
                pt.y = iHeight - iStemHeight - 2;
            else
                pt.y = iStemHeight + 2;
            *ppt++ = pt;
            if(bMirrored)
            {
                pt.x -= iStemWidth;            
            }
            else
            {
                pt.x += iStemWidth;
            }    
            if (bMirrored && (ABS(pt.x - (iWidth - xStem)) <= 2))
            {
                pt.x = iWidth - xStem; // avoid rough edges, have a straight line
                
            }
            else if (!bMirrored && (ABS(pt.x - xStem) <= 2))
            {
                pt.x = xStem; // avoid rough edges, have a straight line
            }    
            *ppt++ = pt;
            if(bMirrored)
            {
                pt.x = iWidth - xStem;
            }
            else
            {
                pt.x = xStem;                
            }
            pt.y = yStem;
            *ppt = pt;

            rgnStem = CreatePolygonRgn(aptStemRgn, CSTEMPOINTS, ALTERNATE);
            if (rgnStem)
            {
                CombineRgn(rgn, rgn, rgnStem, RGN_OR);
                DeleteObject(rgnStem);
            }
        }
    }
    return rgn;
}

void NEAR PASCAL DoShowBubble(PToolTipsMgr pTtm)
{
    HFONT hFontPrev;
    RECT rc;
    int cxText, cyText;
    int xStem, yStem;
    LPTSTR lpstr;
    NMTTSHOWINFO si;
    
    DebugMsg(TF_TT, TEXT("Entering DoShowBubble"));
    
    lpstr = GetCurToolText(pTtm);

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
    

    do {
        // get the size it will be
        TTGetTipSize(pTtm, pTtm->pCurTool, lpstr, &cxText, &cyText);
        TTGetTipPosition(pTtm, &rc, cxText, cyText, &xStem, &yStem);

#ifdef MAINWIN
        // IEUNIX : Mainwin Z-ordering problems.
        SetWindowPos(pTtm->ci.hwnd, HWND_TOPMOST, rc.left, rc.top,
                     rc.right-rc.left, rc.bottom-rc.top,
                     SWP_NOACTIVATE);
#else
        {
            UINT uFlags = SWP_NOACTIVATE | SWP_NOZORDER;

            if (pTtm->ci.style & TTS_BALLOON)
                uFlags |= SWP_HIDEWINDOW;
            SetWindowPos(pTtm->ci.hwnd, NULL, rc.left, rc.top,
                         rc.right-rc.left, rc.bottom-rc.top, uFlags);
        }
#endif

        // BUGBUG: chicago id was busted.  I *hope* no one relied on it...
        // bzzzz...  folks did.  we're stuck with it
        si.hdr.hwndFrom = pTtm->ci.hwnd;
        si.hdr.idFrom = pTtm->pCurTool->uId;
        si.hdr.code = TTN_SHOW;
        si.dwStyle = pTtm->ci.style;

        hFontPrev = pTtm->hFont;
        if (!SendNotifyEx(pTtm->pCurTool->hwnd, (HWND)-1,
                          TTN_SHOW, &si.hdr,
                          (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0)) {

            // Bring to top only if we are an unowned tooltip, since we
            // may have sunken below our tool in the Z-order.  Do this
            // only if unowned; if we are owned, then USER will make sure
            // we are above our owner.
            //
            // We must scrupulously avoid messing with our Z-order in the
            // owned case, because Office curiously creates a tooltip
            // owned by toplevel window 1, but attached to a tool on
            // toplevel window 2.  When you hover over window 2, the
            // tooltip from window 1 wants to appear.  If we brought
            // ourselves to the top, this would also bring window 1
            // to the top (because USER raises and lowers owned/owner
            // windows as a group).  Result:  Window 1 covers window 2.

            UINT uFlags = SWP_NOACTIVATE | SWP_NOSIZE;
            if (GetWindow(pTtm->ci.hwnd, GW_OWNER))
                uFlags |= SWP_NOZORDER;

            SetWindowPos(pTtm->ci.hwnd, HWND_TOP, rc.left, rc.top,
                         0, 0, uFlags);
        }
    
    } while (hFontPrev != pTtm->hFont);

    // create the balloon region if necessary
    // Note: Don't use si.dwStyle here, since other parts of comctl32
    // look at pTtm->ci.style to decide what to do
    if (pTtm->ci.style & TTS_BALLOON)
    {
        HRGN rgn;
        BOOL bMirrored = FALSE;
        if(pTtm->pCurTool)
        {
            bMirrored = (IS_WINDOW_RTL_MIRRORED(pTtm->pCurTool->hwnd) && (!(pTtm->ci.dwExStyle & RTL_MIRRORED_WINDOW)));
        }
        pTtm->fUnderStem = yStem >= rc.bottom-1;
        rgn = CreateBalloonRgn(bMirrored ? (rc.right - xStem) : (xStem - rc.left), yStem-rc.top, rc.right-rc.left, rc.bottom-rc.top, 
                               pTtm->iStemHeight, pTtm->fUnderStem, bMirrored);

        if (rgn && !SetWindowRgn(pTtm->ci.hwnd, rgn, FALSE))
            DeleteObject(rgn);
         // AnimateWindow does not support regions so we must do SetWindowPos    
        SetWindowPos(pTtm->ci.hwnd,HWND_TOP,0,0,0,0,SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE);
    }
    else
    {
        BOOL fAllowFade = !(si.dwStyle & TTS_NOFADE);
        BOOL fAllowAnimate = !(si.dwStyle & TTS_NOANIMATE);
        DWORD dwCurrentTime = (pTtm->dwLastDisplayTime == 0)? TIMEBETWEENANIMATE : GetTickCount();
        DWORD dwDelta = dwCurrentTime - pTtm->dwLastDisplayTime;


        // If we're under the minimum time between animates, then we don't animate
        if (dwDelta < TIMEBETWEENANIMATE)
            fAllowFade = fAllowAnimate = FALSE;

        CoolTooltipBubble(pTtm->ci.hwnd, &rc, fAllowFade, fAllowAnimate);

        pTtm->dwLastDisplayTime = GetTickCount();

        //
        //  HACK! for MetaStock 6.5.  They superclass the tooltips class and install
        //  their own class which takes over WM_PAINT completely.  Animation causes
        //  them to freak out because that causes us to receive a WM_PRINTCLIENT,
        //  which causes TTRender to send a TTN_NEEDTEXT, and they never expected
        //  to receive that notification at that time.
        //
        //  We used to show ourselves with an empty window region, then see if the
        //  WM_PAINT ever reached us.  Unfortunately, that roached Outlook.  So we
        //  just look at the flag afterwards.  This means that MetaStock's first
        //  tooltip will look bad, but the rest will be okay.
        //
        if (pTtm->ci.iVersion < 4 && !pTtm->fEverShown &&
            (si.dwStyle & (TTS_NOFADE | TTS_NOANIMATE)) == 0) {
            // Force a WM_PAINT message so we can check if we got it
            InvalidateRect(pTtm->ci.hwnd, NULL, TRUE);
            UpdateWindow(pTtm->ci.hwnd);
            if (!pTtm->fEverShown) {
                // Detected a hacky app.  Turn off animation.
                SetWindowBits(pTtm->ci.hwnd, GWL_STYLE, TTS_NOFADE | TTS_NOANIMATE,
                                                        TTS_NOFADE | TTS_NOANIMATE);
                pTtm->fEverShown = TRUE;        // don't make this check again
            }
        }
    }

    pTtm->dwFlags |= BUBBLEUP;
    RedrawWindow(pTtm->ci.hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
}

void NEAR PASCAL ShowBubbleForTool(PToolTipsMgr pTtm, PTOOLINFO pTool)
{
    DebugMsg(TF_TT, TEXT("ShowBubbleForTool"));
    // if there's a bubble up for a different tool, pop it.
    if ((pTool != pTtm->pCurTool) && (pTtm->dwFlags & BUBBLEUP)) {
        PopBubble(pTtm);
    }

    // if the bubble was for a different tool, or no bubble, show it
    if ((pTool != pTtm->pCurTool) || !(pTtm->dwFlags & (VIRTUALBUBBLEUP|BUBBLEUP))) {
        
        pTtm->pCurTool = pTool;
        DoShowBubble(pTtm);

    } else {
        DebugMsg(TF_TT, TEXT("ShowBubbleForTool not showinb bubble"));
    }
}

void NEAR PASCAL HandleRelayedMessage(PToolTipsMgr pTtm, HWND hwnd, 
        UINT message, WPARAM wParam, LPARAM lParam)
{
    int ht = HTERROR;

    if (pTtm->dwFlags & TRACKMODE) {
        // punt all messages if we're in track mode
        return;
    }
    
    if (pTtm->dwFlags & BUTTONISDOWN) {
        // verify that the button is down
        // this can happen if the tool didn't set capture so it didn't get the up message
        if (GetKeyState(VK_LBUTTON) >= 0 &&
            GetKeyState(VK_RBUTTON) >= 0 &&
            GetKeyState(VK_MBUTTON) >= 0)
            pTtm->dwFlags &= ~BUTTONISDOWN;
    }
    
    switch(message) {
    case WM_NCLBUTTONUP:
    case WM_NCRBUTTONUP:
    case WM_NCMBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONUP:
        pTtm->dwFlags &= ~BUTTONISDOWN;
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
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
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(hwnd, &pt);
        lParam = MAKELONG(pt.x, pt.y);
        ht = (int) wParam;

        // Fall thru...
    }
    case WM_MOUSEMOVE: {

        PTOOLINFO pTool;
        // to prevent us from popping up when some
        // other app is active
        if(((!(pTtm->ci.style & TTS_ALWAYSTIP)) && !(ChildOfActiveWindow(hwnd))) ||
           !(pTtm->dwFlags & ACTIVE) ||
           (pTtm->dwFlags & BUTTONISDOWN))
        {
            break;
        }

        pTool = GetToolAtPoint(pTtm, hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), ht, FALSE);
        if(pTool) {
            int id;
            // show only if another is showing
            if (pTtm->dwFlags & (VIRTUALBUBBLEUP | BUBBLEUP)) {
                // call show if bubble is up to make sure we're showing
                // for the right tool
                if (pTool != pTtm->pCurTool) {

                    DebugMsg(TF_TT, TEXT("showing virtual bubble"));
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

            DebugMsg(TF_TT, TEXT("MouseMove over pTool id = %d"), id);
            if (id)
                TTSetTimer(pTtm, id);

        } else {
            
            DebugMsg(TF_TT, TEXT("MouseMove over non-tool"));
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
        if (pTtm->dwFlags & TRACKMODE) {
            // if track mode is in effect and active, then
            // redisplay the bubble.
            if (pTtm->pCurTool)
                DoShowBubble(pTtm);
        } else
        if (lpTool == pTtm->pCurTool) {

            // set the current position to our saved position.
            // ToolHasMoved will return false for us if those this point
            // is no longer within pCurTool's area
            GetCursorPos(&pTtm->pt);
            if (!ToolHasMoved(pTtm)) {
                if (pTtm->dwFlags & ( VIRTUALBUBBLEUP | BUBBLEUP)) 
                    DoShowBubble(pTtm);
            } else {
                
                DebugMsg(TF_TT, TEXT("TTUpdateTipText popping bubble"));
                PopBubble(pTtm);
            }
        }
    }
}

void NEAR PASCAL TTSetFont(PToolTipsMgr pTtm, HFONT hFont, BOOL fInval)
{
    ToolTips_NewFont(pTtm, hFont);
    if (fInval)
    {
        // is a balloon up and is it in the track mode?
        if ((pTtm->dwFlags & ACTIVE) && pTtm->pCurTool && (pTtm->pCurTool->uFlags & TTF_TRACK))
        {
            PTOOLINFO pCurTool = pTtm->pCurTool;
            
            PopBubble(pTtm); // sets pTtm->pCurTool to NULL
            ShowBubbleForTool(pTtm, pCurTool);
        }
        else
            InvalidateRect(pTtm->ci.hwnd, NULL, FALSE);
    }
}

void NEAR PASCAL TTSetDelayTime(PToolTipsMgr pTtm, WPARAM wParam, LPARAM lParam)
{
    int iDelayTime = GET_X_LPARAM(lParam);

    switch (wParam) 
    {
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
BOOL NEAR PASCAL CopyToolInfoA(PTOOLINFO pToolSrc, PTOOLINFOA lpTool, UINT uiCodePage)
{
    if (pToolSrc && lpTool) {
        if (lpTool->cbSize >= sizeof(TOOLINFOA) - sizeof(LPARAM)) {
            lpTool->uFlags = pToolSrc->uFlags;
            lpTool->hwnd = pToolSrc->hwnd;
            lpTool->uId = pToolSrc->uId;
            lpTool->rect = pToolSrc->rect;
            lpTool->hinst = pToolSrc->hinst;
            if ((pToolSrc->lpszText != LPSTR_TEXTCALLBACK) &&
                !IS_INTRESOURCE(pToolSrc->lpszText)) {

                if (lpTool->lpszText) {
                    WideCharToMultiByte (uiCodePage, 0,
                                                 pToolSrc->lpszText,
                                                 -1,
                                                 lpTool->lpszText,
                                                 80, NULL, NULL);
                }
            } 
            else 
                lpTool->lpszText = (LPSTR)pToolSrc->lpszText;
        }

        if (lpTool->cbSize > FIELD_OFFSET(TOOLINFOA, lParam))
            lpTool->lParam = pToolSrc->lParam;
        
        if (lpTool->cbSize > sizeof(TOOLINFOA))
            return FALSE;
            
        return TRUE;
    } 
    else
        return FALSE;
}
#endif

BOOL NEAR PASCAL CopyToolInfo(PTOOLINFO pToolSrc, PTOOLINFO lpTool)
{
    if (pToolSrc && lpTool && lpTool->cbSize <= sizeof(TOOLINFO)) {
        if (lpTool->cbSize >= sizeof(TOOLINFO) - sizeof(LPARAM)) {
            lpTool->uFlags = pToolSrc->uFlags;
            lpTool->hwnd = pToolSrc->hwnd;
            lpTool->uId = pToolSrc->uId;
            lpTool->rect = pToolSrc->rect;
            lpTool->hinst = pToolSrc->hinst;
            if ((pToolSrc->lpszText != LPSTR_TEXTCALLBACK) && !IS_INTRESOURCE(pToolSrc->lpszText)) 
            {
                if (lpTool->lpszText) 
                    lstrcpy(lpTool->lpszText, pToolSrc->lpszText);
            }
            else 
                lpTool->lpszText = pToolSrc->lpszText;
        }
        if (lpTool->cbSize > FIELD_OFFSET(TOOLINFO, lParam))
            lpTool->lParam = pToolSrc->lParam;
        
        if (lpTool->cbSize > sizeof(TOOLINFO))
            return FALSE;
    
        return TRUE;
    }
    else
        return FALSE;
}

PTOOLINFO TTToolAtMessagePos(PToolTipsMgr pTtm)
{
    PTOOLINFO pTool;
    HWND hwndPt;
    POINT pt;
    DWORD dwPos = GetMessagePos();
    //int ht;

    pt.x = GET_X_LPARAM(dwPos);
    pt.y = GET_Y_LPARAM(dwPos);
    hwndPt = TTWindowFromPoint(pTtm, &pt);
    //ht = SendMessage(hwndPt, WM_NCHITTEST, 0, MAKELONG(pt.x, pt.y));
    ScreenToClient(hwndPt, &pt);
    pTool = GetToolAtPoint(pTtm, hwndPt, pt.x, pt.y, HTERROR, TRUE);

    return pTool;
}

void TTCheckCursorPos(PToolTipsMgr pTtm)
{
    PTOOLINFO pTool;

    pTool = TTToolAtMessagePos(pTtm);
    if ((pTtm->pCurTool != pTool) || 
        ToolHasMoved(pTtm)) {
        PopBubble(pTtm);

        DebugMsg(TF_TT, TEXT("TTCheckCursorPos popping bubble"));
    }
}

void NEAR PASCAL TTHandleTimer(PToolTipsMgr pTtm, UINT_PTR id)
{
    PTOOLINFO pTool;
    
    // punt all timers in track mode
    if (pTtm->dwFlags & TRACKMODE)
        return;

    switch (id) {

    case TTT_AUTOPOP:
        TTCheckCursorPos(pTtm); 
        if (pTtm->pCurTool) {
            DebugMsg(TF_TT, TEXT("ToolTips: Auto popping"));
            ShowVirtualBubble(pTtm);
        }
        break;

    case TTT_POP:

        // this could be started up again by a slight mouse touch
        if (pTtm->dwFlags & VIRTUALBUBBLEUP) {
            KillTimer(pTtm->ci.hwnd, TTT_POP);
        }

        TTCheckCursorPos(pTtm); 
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

        pTool = TTToolAtMessagePos(pTtm);
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

BOOL TTRender(PToolTipsMgr pTtm, HDC hdc)
{
    BOOL bRet = FALSE;
    RECT rc;
    LPTSTR lpszStr;

    if (pTtm->pCurTool &&
        (lpszStr = GetCurToolText(pTtm)) &&
        *lpszStr) {
        UINT uFlags;
        NMTTCUSTOMDRAW nm;
        UINT uDefDrawFlags = 0;
        BOOL bUseDrawText;
        LPRECT prcMargin = &pTtm->rcMargin;

        HBRUSH hbr;
        DWORD  dwCustomDraw;

        uFlags = 0;

#if defined(WINDOWS_ME)
        if ( (pTtm->pCurTool->uFlags & TTF_RTLREADING) || (pTtm->ci.dwExStyle & WS_EX_RTLREADING) )
            uFlags |= ETO_RTLREADING;
#endif

        SelectObject(hdc, pTtm->hFont);
        GetClientRect(pTtm->ci.hwnd, &rc);
        SetTextColor(hdc, pTtm->clrTipText);

        /* If we support pre-Draw text then call the client allowing them to modify
         /  the item, and then render.  Otherwise just use ExTextOut */
        nm.nmcd.hdr.hwndFrom = pTtm->ci.hwnd;
        nm.nmcd.hdr.idFrom = pTtm->pCurTool->uId;
        nm.nmcd.hdr.code = NM_CUSTOMDRAW;
        nm.nmcd.hdc = hdc;
        nm.nmcd.dwDrawStage = CDDS_PREPAINT;

        // ADJUSTRECT!  Keep TTAdjustRect and TTGetTipPosition in sync.
        nm.nmcd.rc.left   = rc.left   + XTEXTOFFSET*g_cxBorder + prcMargin->left;
        nm.nmcd.rc.right  = rc.right  - XTEXTOFFSET*g_cxBorder - prcMargin->right;
        nm.nmcd.rc.top    = rc.top    + YTEXTOFFSET*g_cyBorder + prcMargin->top;
        nm.nmcd.rc.bottom = rc.bottom - YTEXTOFFSET*g_cyBorder - prcMargin->bottom;

        if (pTtm->ci.style & TTS_BALLOON)
        {
            InflateRect(&(nm.nmcd.rc), -XBALLOONOFFSET, -YBALLOONOFFSET);
            if (!pTtm->fUnderStem)
                OffsetRect(&(nm.nmcd.rc), 0, pTtm->iStemHeight);
        }

        if (pTtm->iMaxTipWidth == -1) 
            uDefDrawFlags = DT_SINGLELINE |DT_LEFT;
        else 
            uDefDrawFlags = DT_LEFT | DT_WORDBREAK | DT_EXPANDTABS | DT_EXTERNALLEADING;

        if (pTtm->ci.style & TTS_NOPREFIX)
            uDefDrawFlags |= DT_NOPREFIX;

#if defined(WINDOWS_ME)
        if ( (pTtm->pCurTool->uFlags & TTF_RTLREADING) || (pTtm->ci.dwExStyle & WS_EX_RTLREADING) )
            uDefDrawFlags |= DT_RTLREADING;
#endif
        //
        // Make it right aligned, if requested. [samera]
        //
        if (pTtm->pCurTool->uFlags & TTF_RIGHT)
            uDefDrawFlags |= DT_RIGHT;
 
        nm.uDrawFlags = uDefDrawFlags;

        dwCustomDraw = (DWORD)SendNotifyEx(pTtm->pCurTool->hwnd, (HWND) -1,
                     0, (NMHDR*) &nm,
                     (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0);
        // did the owner do custom draw? yes, we're done
        if (pTtm->ci.iVersion >= 5 && dwCustomDraw == CDRF_SKIPDEFAULT)
            return TRUE;

        bUseDrawText = (nm.uDrawFlags != uDefDrawFlags ||
                        !(uDefDrawFlags & DT_SINGLELINE) ||
                        (uDefDrawFlags & (DT_RTLREADING|DT_RIGHT)) ||
                        (pTtm->cchTipTitle != 0));

        if (pTtm->clrTipBk != GetNearestColor(hdc, pTtm->clrTipBk) ||
            bUseDrawText) 
        {
            // if this fails, it may be the a dither...
            // in which case, we can't set the bk color
            hbr = CreateSolidBrush(pTtm->clrTipBk);
            FillRect(hdc, &rc, hbr);
            DeleteObject(hbr);

            SetBkMode(hdc, TRANSPARENT);
            uFlags |= ETO_CLIPPED;
        } 
        else 
        {
            uFlags |= ETO_OPAQUE;
            SetBkColor(hdc, pTtm->clrTipBk);
        }

        if (bUseDrawText) 
        {
            // Account for off-by-one.  Something wierd about DrawText
            // clips the bottom-most pixelrow, so increase one more
            // into the margin space.

            // ADJUSTRECT!  Keep TTAdjustRect in sync.
            nm.nmcd.rc.bottom++;
            nm.nmcd.rc.right++;
            // if in balloon style the text is already indented so no need for inflate..
            if (pTtm->cchTipTitle > 0 && !(pTtm->ci.style & TTS_BALLOON))
                InflateRect(&nm.nmcd.rc, -XBALLOONOFFSET, -YBALLOONOFFSET);

            if (!TTRenderTitledTip(pTtm, hdc, FALSE, &nm.nmcd.rc, uDefDrawFlags))
                DrawText(hdc, lpszStr, lstrlen(lpszStr), &nm.nmcd.rc, nm.uDrawFlags);
        }
        else
        {
            // ADJUSTRECT!  Keep TTAdjustRect and TTGetTipPosition in sync.
            int x = XTEXTOFFSET*g_cxBorder + prcMargin->left;
            int y = YTEXTOFFSET*g_cyBorder + prcMargin->top;

            if (pTtm->ci.style & TTS_BALLOON)
            {
                HRGN rgn;
                
                x += XBALLOONOFFSET;
                y += YBALLOONOFFSET;
                InflateRect(&rc, -XBALLOONOFFSET, -YBALLOONOFFSET);
                if (!pTtm->fUnderStem)
                {
                    y += pTtm->iStemHeight;
                    OffsetRect(&rc, 0, pTtm->iStemHeight);
                }
                
                rgn = CreateRectRgn(1,1,2,2);
                if (rgn)
                {
                    int iRet = GetWindowRgn(pTtm->ci.hwnd, rgn);
                    if (iRet != ERROR)
                    {
                        // ExtTextOut only fills the rect specified and that
                        // only if uFlags & ETO_OPAQUE
                        HBRUSH hbr = CreateSolidBrush(pTtm->clrTipBk);
                        FillRgn(hdc, rgn, hbr);
                        DeleteObject(hbr);
                    }
                    DeleteObject(rgn);
                }
            }
            else if (pTtm->cchTipTitle > 0)
            {
                InflateRect(&rc, -XBALLOONOFFSET, -YBALLOONOFFSET);
            }

            if (!TTRenderTitledTip(pTtm, hdc, FALSE, &rc, uDefDrawFlags))
                ExtTextOut(hdc, x, y, uFlags, &rc, lpszStr, lstrlen(lpszStr), NULL);
        }

        if (pTtm->ci.style & TTS_BALLOON)
        {
            HRGN rgn = CreateRectRgn(1,1,2,2);

            if (rgn)
            {
                int iRet = GetWindowRgn(pTtm->ci.hwnd, rgn);
                if (iRet != ERROR)
                {
                    HBRUSH hbr = CreateSolidBrush(pTtm->clrTipText);
                    FrameRgn(hdc, rgn, hbr, 1, 1);
                    DeleteObject(hbr);
                }
                DeleteObject(rgn);
            }
        }

        // notify parent afterwards if they want us to
        if (!(dwCustomDraw & CDRF_SKIPDEFAULT) &&
            dwCustomDraw & CDRF_NOTIFYPOSTPAINT) {
            // Convert PREPAINT to POSTPAINT and ITEMPREPAINT to ITEMPOSTPAINT
            COMPILETIME_ASSERT(CDDS_POSTPAINT - CDDS_PREPAINT ==
                               CDDS_ITEMPOSTPAINT - CDDS_ITEMPREPAINT);
            nm.nmcd.dwDrawStage += CDDS_POSTPAINT - CDDS_PREPAINT;
            SendNotifyEx(pTtm->pCurTool->hwnd, (HWND) -1,
                         0, (NMHDR*) &nm,
                         (pTtm->pCurTool->uFlags & TTF_UNICODE) ? 1 : 0);
        }

        bRet = TRUE;
    }

    return bRet;
}

void TTOnPaint(PToolTipsMgr pTtm)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(pTtm->ci.hwnd, &ps);

    if (!TTRender(pTtm, hdc)) {
        DebugMsg(TF_TT, TEXT("TTOnPaint render failed popping bubble"));
        PopBubble(pTtm);
    }

    EndPaint(pTtm->ci.hwnd, &ps);
    pTtm->fEverShown = TRUE;                // See TTOnFirstShow
}

LRESULT WINAPI ToolTipsWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PTOOLINFO pTool;
    PTOOLINFO pToolSrc;
    PToolTipsMgr pTtm = GetWindowPtr(hwnd, 0);
    
    if (!pTtm && uMsg != WM_CREATE)
        goto DoDefault;

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
        {
        LRESULT res;
        TOOLINFOW ti;

        if (!lParam) {
            return FALSE;
        }

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, TRUE, pTtm->ci.uiCodePage)) {
            return FALSE;
        }

        res = AddTool(pTtm, &ti);

        if ((ti.uFlags & TTF_MEMALLOCED) && (ti.lpszText != LPSTR_TEXTCALLBACK)) {
            LocalFree (ti.lpszText);
        }

        return res;
        }
#endif

    case TTM_ADDTOOL:
        if (!lParam)
            return FALSE;

        return AddTool(pTtm, (LPTOOLINFO)lParam);

#ifdef UNICODE
    case TTM_DELTOOLA:
        {
        TOOLINFOW ti;

        if (!lParam) {
            return FALSE;
        }

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, FALSE, pTtm->ci.uiCodePage)) {
            break;
        }
        DeleteTool(pTtm, &ti);

        break;
        }
#endif
        
    case TTM_DELTOOL:
        if (!lParam)
            return FALSE;

        DeleteTool(pTtm, (LPTOOLINFO)lParam);
        break;

#ifdef UNICODE
    case TTM_NEWTOOLRECTA:
        {
        TOOLINFOW ti;

        if (!lParam) {
            return FALSE;
        }

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, FALSE, pTtm->ci.uiCodePage)) {
            break;
        }

        pTool = FindTool(pTtm, &ti);
        if(pTool) {
            pTool->rect = ((LPTOOLINFOA)lParam)->rect;
        }

        break;
        }
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
        {
        TOOLINFOW ti;

        if (!lParam) {
            return FALSE;
        }

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, FALSE, pTtm->ci.uiCodePage)) {
            return FALSE;
        }

        pToolSrc = FindTool(pTtm, &ti);

        return (LRESULT)(UINT)CopyToolInfoA(pToolSrc, (LPTOOLINFOA)lParam, pTtm->ci.uiCodePage);
        }

    case TTM_GETCURRENTTOOLA:
        if (lParam) 
            return (LRESULT)(UINT)CopyToolInfoA(pTtm->pCurTool, (LPTOOLINFOA)lParam, pTtm->ci.uiCodePage);
        else
            return BOOLFROMPTR(pTtm->pCurTool);

    case TTM_ENUMTOOLSA:
    {
        if (wParam >= 0 && wParam < (UINT)pTtm->iNumTools) {
            pToolSrc = &pTtm->tools[wParam];
            return (LRESULT)(UINT)CopyToolInfoA(pToolSrc, (LPTOOLINFOA)lParam, pTtm->ci.uiCodePage);
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
        if (lParam)
            return (LRESULT)(UINT)CopyToolInfo(pTtm->pCurTool, (LPTOOLINFO)lParam);
        else 
            return BOOLFROMPTR(pTtm->pCurTool);

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
        {
        TOOLINFOW ti;

        if (!lParam) {
            return FALSE;
        }

        if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, TRUE, pTtm->ci.uiCodePage)) {
            return FALSE;
        }

        pTool = FindTool(pTtm, &ti);
        if (pTool) {
            TTSetTipText(pTool, NULL);
            hmemcpy(pTool, &ti, ti.cbSize);
            pTool->lpszText = NULL;
            TTSetTipText(pTool, ti.lpszText);

            if (pTool == pTtm->pCurTool) {
                DoShowBubble(pTtm);
            }
        }


        if ((ti.uFlags & TTF_MEMALLOCED) && (ti.lpszText != LPSTR_TEXTCALLBACK)) {
            LocalFree (ti.lpszText);
        }

        break;
        }
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
        pTool = GetToolAtPoint(pTtm, lphitinfoA->hwnd, lphitinfoA->pt.x, lphitinfoA->pt.y, HTERROR, TRUE);
        if (pTool) {
            ThunkToolInfoWtoA(pTool, (LPTOOLINFOA)(&(lphitinfoA->ti)), pTtm->ci.uiCodePage);
            return TRUE;
        }
        return FALSE;
#endif

    case TTM_HITTEST:
#define lphitinfo ((LPHITTESTINFO)lParam)
        if (!lParam)
            return FALSE;
        pTool = GetToolAtPoint(pTtm, lphitinfo->hwnd, lphitinfo->pt.x, lphitinfo->pt.y, HTERROR, TRUE);
        if (pTool) {
            
            // for back compat...  if thesize isn't set right, we only give
            // them the win95 amount.
            if (lphitinfo->ti.cbSize != sizeof(TTTOOLINFO)) {
                *((WIN95TTTOOLINFO*)&lphitinfo->ti) = *(WIN95TTTOOLINFO*)pTool;
            } else {
                lphitinfo->ti = *pTool;
            }
            return TRUE;
        }
        return FALSE;

#ifdef UNICODE
    case TTM_GETTEXTA: {
        LPWSTR lpszTemp;
        TOOLINFOW ti;

        if (!lParam || !((LPTOOLINFOA)lParam)->lpszText)
            return FALSE;

        if (!ThunkToolInfoAtoW((LPTOOLINFOA)lParam, &ti, FALSE, pTtm->ci.uiCodePage))
            break;
                       
        ((LPTOOLINFOA)lParam)->lpszText[0] = 0;
        pTool = FindTool(pTtm, &ti);
        lpszTemp = GetToolText(pTtm, pTool);
        if (lpszTemp) 
            WideCharToMultiByte (pTtm->ci.uiCodePage,
                                 0,
                                 lpszTemp,
                                 -1,
                                 (((LPTOOLINFOA)lParam)->lpszText),
                                 80, NULL, NULL);
        
        break;
    }
#endif

    case TTM_GETTEXT: {
        LPTSTR lpszTemp;
        if (!lParam || !pTtm || !((LPTOOLINFO)lParam)->lpszText)
            return FALSE;

        ((LPTOOLINFO)lParam)->lpszText[0] = 0;
        pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
        lpszTemp = GetToolText(pTtm, pTool);
        if (lpszTemp) 
            lstrcpy((((LPTOOLINFO)lParam)->lpszText), lpszTemp);
    }
        break;


    case WM_GETTEXTLENGTH:
    case WM_GETTEXT:
    {
        LPTSTR lpszStr;
#ifdef UNICODE_WIN9x
        char *pszDest = uMsg == WM_GETTEXT ? (char *)lParam : NULL;
#else
        TCHAR *pszDest = uMsg == WM_GETTEXT ? (TCHAR *)lParam : NULL;
#endif
        LRESULT lres;

        // Pre-terminate the string just in case
        if (pszDest && wParam)
            pszDest[0] = 0;

        if (pTtm && (lpszStr = GetCurToolText(pTtm))) {
#ifdef UNICODE_WIN9x
            LPSTR pStringA = ProduceAFromW(pTtm->ci.uiCodePage, lpszStr);
            if (pStringA) {
                if (pszDest && wParam) {
                    lstrcpynA(pszDest, pStringA, (int) wParam);
                    lres = lstrlenA(pszDest);
                } else {
                    lres = lstrlenA(pStringA);
                }
                FreeProducedString(pStringA);
            } else {                // out of memory
                lres = 0;
            }
#else
            if (pszDest && wParam) {
                StrCpyN(pszDest, lpszStr, (int) wParam);
                lres = lstrlen(pszDest);
            } else {
                lres = lstrlen(lpszStr);
            }
#endif
        } else {                    // No current tool
            lres = 0;
        }
        return lres;
    }

    case TTM_RELAYEVENT:
#define lpmsg ((LPMSG)lParam)
        if (!lParam)
            return FALSE;
        HandleRelayedMessage(pTtm, lpmsg->hwnd, lpmsg->message, lpmsg->wParam,
                lpmsg->lParam);
#undef lpmsg
        break;

        // this is here for people to subclass and fake out what we
        // think the window from point is.  this facilitates "transparent" windows
        case TTM_WINDOWFROMPOINT: {
            HWND hwndPt = WindowFromPoint(*((POINT FAR *)lParam));
            DebugMsg(TF_TT, TEXT("TTM_WINDOWFROMPOINT %x"), hwndPt);
            return (LRESULT)hwndPt;
        }

#ifdef UNICODE
        case TTM_UPDATETIPTEXTA:
            {
            TOOLINFOW ti;

            if (lParam) {
                if (!ThunkToolInfoAtoW ((LPTOOLINFOA)lParam, &ti, TRUE, pTtm->ci.uiCodePage)) {
                    break;
                }
                TTUpdateTipText(pTtm, &ti);

                if ((ti.uFlags & TTF_MEMALLOCED) && (ti.lpszText != LPSTR_TEXTCALLBACK)) {
                    LocalFree (ti.lpszText);
                }
            }
            break;
            }
#endif

    case TTM_UPDATETIPTEXT:
        if (lParam)
            TTUpdateTipText(pTtm, (LPTOOLINFO)lParam);
        break;

    /* Pop the current tooltip if there is one displayed, ensuring that the virtual
    /  bubble is also discarded. */

    case TTM_POP:
    {
        if ( pTtm ->dwFlags & BUBBLEUP )
            PopBubble( pTtm );

        pTtm ->dwFlags &= ~VIRTUALBUBBLEUP;

        break;
    }


    case TTM_TRACKPOSITION:
        if ((GET_X_LPARAM(lParam) != pTtm->ptTrack.x) || 
            (GET_Y_LPARAM(lParam) != pTtm->ptTrack.y)) 
        {
            pTtm->ptTrack.x = GET_X_LPARAM(lParam); 
            pTtm->ptTrack.y = GET_Y_LPARAM(lParam);
        
            // if track mode is in effect, update the position
            if ((pTtm->dwFlags & TRACKMODE) && 
                pTtm->pCurTool) {
                DoShowBubble(pTtm);
            }
        }
        break;
        
    case TTM_UPDATE:
        if (!lParam ||
            lParam == (LPARAM)pTtm->pCurTool) {
            DoShowBubble(pTtm);
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
        if (pTtm->clrTipBk != (COLORREF)wParam) {
            pTtm->clrTipBk = (COLORREF)wParam;
            InvalidateRgn(pTtm->ci.hwnd,NULL,TRUE);
        }
        pTtm->fBkColorSet = TRUE;
        break;
        
    case TTM_GETTIPBKCOLOR:
        return (LRESULT)(UINT)pTtm->clrTipBk;
        
    case TTM_SETTIPTEXTCOLOR:
        if (pTtm->clrTipText != (COLORREF)wParam) {
            InvalidateRgn(pTtm->ci.hwnd,NULL,TRUE);
            pTtm->clrTipText = (COLORREF)wParam;
        }
        pTtm->fTextColorSet = TRUE;
        break;
        
    case TTM_GETTIPTEXTCOLOR:
        return (LRESULT)(UINT)pTtm->clrTipText;
        
    case TTM_SETMAXTIPWIDTH:
    {
        int iOld = pTtm->iMaxTipWidth;
        pTtm->iMaxTipWidth = (int)lParam;
        return iOld;
    }
        
    case TTM_GETMAXTIPWIDTH:
        return pTtm->iMaxTipWidth;
        
    case TTM_SETMARGIN:
        if (lParam)
            pTtm->rcMargin = *(LPRECT)lParam;
        break;

    case TTM_GETMARGIN:
        if (lParam)
            *(LPRECT)lParam = pTtm->rcMargin;
        break;

    case TTM_GETBUBBLESIZE:
        if (lParam)
        {
            pTool = FindTool(pTtm, (LPTOOLINFO)lParam);
            if (pTool)
            {
                LPTSTR lpstr = GetToolText(pTtm, pTool);
                int    cxText, cyText, cxMargin, cyMargin, iBubbleWidth, iBubbleHeight;

                TTGetTipSize(pTtm, pTool, lpstr, &cxText, &cyText);

                cxMargin = pTtm->rcMargin.left + pTtm->rcMargin.right;
                cyMargin = pTtm->rcMargin.top + pTtm->rcMargin.bottom;
                iBubbleWidth =  2*XTEXTOFFSET * g_cxBorder + cxText + cxMargin;
                iBubbleHeight = 2*YTEXTOFFSET * g_cyBorder + cyText + cyMargin;

                if (pTtm->ci.style & TTS_BALLOON)
                {
                    iBubbleWidth += 2*XBALLOONOFFSET;
                    iBubbleHeight += 2*YBALLOONOFFSET;
                }   
                return MAKELONG(iBubbleWidth, iBubbleHeight);
            }
        }
        break;

    case TTM_ADJUSTRECT:
        return TTAdjustRect(pTtm, BOOLFROMPTR(wParam), (LPRECT)lParam);

#ifdef UNICODE
    case TTM_SETTITLEA:
        {
            TCHAR szTitle[MAX_TIP_CHARACTERS];
            pTtm->uTitleBitmap = (UINT)wParam;
            Str_Set(&pTtm->lpTipTitle, NULL);
            pTtm->iTitleHeight = 0;

            TTCreateTitleBitmaps(pTtm);

            if (lParam)
            {
                pTtm->cchTipTitle = lstrlenA((LPCSTR)lParam);
                if (pTtm->cchTipTitle < ARRAYSIZE(szTitle))
                {
                    ConvertAToWN(pTtm->ci.uiCodePage, szTitle, ARRAYSIZE(szTitle),
                        (LPCSTR)lParam, -1);
                    Str_Set(&pTtm->lpTipTitle, szTitle);
                    return TRUE;
                }
            }
            pTtm->cchTipTitle = 0;
            return FALSE;
        }
        break;
#endif
    case TTM_SETTITLE:
        {
            pTtm->uTitleBitmap = (UINT)wParam;
            Str_Set(&pTtm->lpTipTitle, NULL);
            pTtm->iTitleHeight = 0;

            TTCreateTitleBitmaps(pTtm);

            if (lParam)
            {
                pTtm->cchTipTitle = lstrlen((LPCTSTR)lParam);
                if (pTtm->cchTipTitle < MAX_TIP_CHARACTERS)
                {
                    Str_Set(&pTtm->lpTipTitle, (LPCTSTR)lParam);
                    return TRUE;
                }
            }
            pTtm->cchTipTitle = 0;
            return FALSE;
        }
        break;


        /* uMsgs that REALLY came for me. */
    case WM_CREATE:
        {
            DWORD dwBits, dwValue;
            
            pTtm = ToolTipsMgrCreate(hwnd, (LPCREATESTRUCT)lParam);
            if (!pTtm)
                return -1;
            
            SetWindowPtr(hwnd, 0, pTtm);
            SetWindowBits(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW, WS_EX_TOOLWINDOW);

            dwBits = WS_CHILD | WS_POPUP | WS_BORDER | WS_DLGFRAME;
            dwValue = WS_POPUP | WS_BORDER;
            // we don't want border for balloon style
            if (pTtm->ci.style & TTS_BALLOON)
                dwValue &= ~WS_BORDER;
            SetWindowBits(hwnd, GWL_STYLE, dwBits, dwValue);
            
            TTSetFont(pTtm, 0, FALSE);
            break;
        }

    case WM_TIMER:  
        TTHandleTimer(pTtm, wParam);
        break;

        
    case WM_NCHITTEST:
        // we should not return HTTRANSPARENT here because then we don't receive the mouse events
        // and we cannot forward them down to our parent. but because of the backcompat we keep doing
        // it unless we are using comctl32 v5 or greater
        //
        // If we are inside TTWindowFromPoint, then respect transparency
        // even on v5 clients.
        //
        // Otherwise, your tooltips flicker because the tip appears,
        // then WM_NCHITTEST says "not over the tool any more" (because
        // it's over the tooltip), so the bubble pops, and then the tip
        // reappears, etc.
        if (pTtm && (pTtm->ci.iVersion < 5 || pTtm->fInWindowFromPoint) &&
            pTtm->pCurTool && (pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
        {
            return HTTRANSPARENT;
        } 
        goto DoDefault;
        
    case WM_MOUSEMOVE:
        // the cursor moved onto the tips window.
        if (!(pTtm->dwFlags & TRACKMODE) && pTtm->pCurTool && !(pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
            PopBubble(pTtm);
        // fall through

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (pTtm->pCurTool && (pTtm->pCurTool->uFlags & TTF_TRANSPARENT))
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            
            MapWindowPoints(pTtm->ci.hwnd, pTtm->pCurTool->hwnd, &pt, 1);
            SendMessage(pTtm->pCurTool->hwnd, uMsg, wParam, MAKELPARAM(pt.x, pt.y));
        }
        break;

    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        if (pTtm) {
            if (!pTtm->fBkColorSet)
                pTtm->clrTipBk = g_clrInfoBk;
            if (!pTtm->fTextColorSet)
                pTtm->clrTipText = g_clrInfoText;
        }
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        if (pTtm->fMyFont)
            TTSetFont(pTtm, 0, FALSE);
        break;

    case WM_PAINT: 
        TTOnPaint(pTtm);
        break;

    case WM_SETFONT:
        TTSetFont(pTtm, (HFONT)wParam, (BOOL)lParam);
        return(TRUE);

    case WM_GETFONT:
        if (pTtm) {
           return((LRESULT)pTtm->hFont);
        }
        break;

    case WM_NOTIFYFORMAT:
        if (lParam == NF_QUERY) {
#ifdef UNICODE
            return NFR_UNICODE;
#else
            return NFR_ANSI;
#endif
        } else if (lParam == NF_REQUERY) {
            int i;

            for(i = 0 ; i < pTtm->iNumTools; i++) {
                pTool = &pTtm->tools[i];

                if (SendMessage (pTool->hwnd, WM_NOTIFYFORMAT,
                                 (WPARAM)hwnd, NF_QUERY) == NFR_UNICODE) {
                    pTool->uFlags |= TTF_UNICODE;
                } else {
                    pTool->uFlags &= ~TTF_UNICODE;
                }
            }

            return CIHandleNotifyFormat(&pTtm->ci, lParam);
        }
        return 0;

    case WM_ERASEBKGND:
        break;
        
    case WM_STYLECHANGED:
        if ((wParam == GWL_STYLE) && pTtm) 
        {
            DWORD dwNewStyle = ((LPSTYLESTRUCT)lParam)->styleNew;
            if ( pTtm->ci.style & TTS_BALLOON &&    // If the old style was a balloon,
                !(dwNewStyle & TTS_BALLOON))        // And the new style is not a balloon,
            {
                // Then we need to unset the region.
                SetWindowRgn(pTtm->ci.hwnd, NULL, FALSE);
            }

            pTtm->ci.style = ((LPSTYLESTRUCT)lParam)->styleNew;
        }
        break;
        
    case WM_DESTROY: 
        {
            if (pTtm->tools) 
            {
                int i;
            
                // free the tools
                for(i = 0 ; i < pTtm->iNumTools; i++) 
                {
                    TTBeforeFreeTool(pTtm, &pTtm->tools[i]);
                }
            
                LocalFree((HANDLE)pTtm->tools);
            }
        
            TTSetFont(pTtm, (HFONT)1, FALSE); // delete font if we made one.

            Str_Set(&pTtm->lpTipText, NULL);
            Str_Set(&pTtm->lpTipTitle, NULL);

            if (pTtm->himlTitleBitmaps)
                ImageList_Destroy(pTtm->himlTitleBitmaps);
        
            LocalFree((HANDLE)pTtm);
            SetWindowPtr(hwnd, 0, 0);
        }
        break;

    case WM_PRINTCLIENT:
        TTRender(pTtm, (HDC)wParam);
        break;

    case WM_GETOBJECT:
        if( lParam == OBJID_QUERYCLASSNAMEIDX )
            return MSAA_CLASSNAMEIDX_TOOLTIPS;
        goto DoDefault;

    default:
    {
        LRESULT lres;
        if (CCWndProc(&pTtm->ci, uMsg, wParam, lParam, &lres))
            return lres;
    }
DoDefault:
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

BOOL ThunkToolInfoAtoW (LPTOOLINFOA lpTiA, LPTOOLINFOW lpTiW, BOOL bThunkText, UINT uiCodePage)
{

    //
    // Copy the constants to the new structure.
    //

    lpTiW->uFlags      = lpTiA->uFlags;
    lpTiW->hwnd        = lpTiA->hwnd;
    lpTiW->uId         = lpTiA->uId;

    lpTiW->rect.left   = lpTiA->rect.left;
    lpTiW->rect.top    = lpTiA->rect.top;
    lpTiW->rect.right  = lpTiA->rect.right;
    lpTiW->rect.bottom = lpTiA->rect.bottom;

    lpTiW->hinst       = lpTiA->hinst;

    //
    //  Set the size properly and optionally copy the new fields if the
    //  structure is large enough.
    //
    if (lpTiA->cbSize <= TTTOOLINFOA_V1_SIZE) {
        lpTiW->cbSize  = TTTOOLINFOW_V1_SIZE;
    } else {
        lpTiW->cbSize  = sizeof(TOOLINFOW);
        lpTiW->lParam  = lpTiA->lParam;
    }

    if (bThunkText) {
        //
        // Thunk the string to the new structure.
        // Special case LPSTR_TEXTCALLBACK.
        //

        if (lpTiA->lpszText == LPSTR_TEXTCALLBACKA) {
            lpTiW->lpszText = LPSTR_TEXTCALLBACKW;

        } else if (!IS_INTRESOURCE(lpTiA->lpszText)) {

            DWORD dwBufSize;
            int iResult;

            dwBufSize = lstrlenA(lpTiA->lpszText) + 1;
            lpTiW->lpszText = LocalAlloc (LPTR, dwBufSize * sizeof(WCHAR));

            if (!lpTiW->lpszText) {
                return FALSE;
            }

            iResult = MultiByteToWideChar (uiCodePage, 0, lpTiA->lpszText, -1,
                                           lpTiW->lpszText, dwBufSize);

            //
            // If iResult is 0, and GetLastError returns an error code,
            // then MultiByteToWideChar failed.
            //

            if (!iResult) {
                if (GetLastError()) {
                    return FALSE;
                }
            }

            lpTiW->uFlags |= TTF_MEMALLOCED;

        } else {
            lpTiW->lpszText = (LPWSTR)lpTiA->lpszText;
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

BOOL ThunkToolInfoWtoA (LPTOOLINFOW lpTiW, LPTOOLINFOA lpTiA, UINT uiCodePage)
{
    int iResult = 1;

    //
    // Copy the constants to the new structure.
    //

    lpTiA->uFlags      = lpTiW->uFlags;
    lpTiA->hwnd        = lpTiW->hwnd;
    lpTiA->uId         = lpTiW->uId;

    lpTiA->rect.left   = lpTiW->rect.left;
    lpTiA->rect.top    = lpTiW->rect.top;
    lpTiA->rect.right  = lpTiW->rect.right;
    lpTiA->rect.bottom = lpTiW->rect.bottom;

    lpTiA->hinst       = lpTiW->hinst;

    //
    //  Set the size properly and optionally copy the new fields if the
    //  structure is large enough.
    //
    if (lpTiW->cbSize <= TTTOOLINFOW_V1_SIZE) {
        lpTiA->cbSize  = TTTOOLINFOA_V1_SIZE;
    } else {
        lpTiA->cbSize  = sizeof(TOOLINFOA);
        lpTiA->lParam  = lpTiA->lParam;
    }

    //
    // Thunk the string to the new structure.
    // Special case LPSTR_TEXTCALLBACK.
    //

    if (lpTiW->lpszText == LPSTR_TEXTCALLBACKW) {
        lpTiA->lpszText = LPSTR_TEXTCALLBACKA;

    } else if (!IS_INTRESOURCE(lpTiW->lpszText)) {

        //
        // It is assumed that lpTiA->lpszText is already setup to
        // a valid buffer, and that buffer is 80 characters.
        // 80 characters is defined in the TOOLTIPTEXT structure.
        //

        iResult = WideCharToMultiByte (uiCodePage, 0, lpTiW->lpszText, -1,
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

BOOL ThunkToolTipTextAtoW (LPTOOLTIPTEXTA lpTttA, LPTOOLTIPTEXTW lpTttW, UINT uiCodePage)
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

    lpTttW->hinst  = lpTttA->hinst;
    lpTttW->uFlags = lpTttA->uFlags;
    lpTttW->lParam = lpTttA->lParam;

    //
    // Thunk the string to the new structure.
    // Special case LPSTR_TEXTCALLBACK.
    //

    if (lpTttA->lpszText == LPSTR_TEXTCALLBACKA) {
        lpTttW->lpszText = LPSTR_TEXTCALLBACKW;

    } else if (!IS_INTRESOURCE(lpTttA->lpszText)) {

        //
        //  Transfer the lpszText into the lpTttW...
        //
        //  First see if it fits into the buffer, and optimistically assume
        //  it will.
        //
        lpTttW->lpszText = lpTttW->szText;
        iResult = MultiByteToWideChar (uiCodePage, 0, lpTttA->lpszText, -1,
                                       lpTttW->szText, ARRAYSIZE(lpTttW->szText));
        if (!iResult) {
            //
            //  Didn't fit into the small buffer; must alloc our own.
            //
            lpTttW->lpszText = ProduceWFromA(uiCodePage, lpTttA->lpszText);
            lpTttW->uFlags |= TTF_MEMALLOCED;
        }

    } else {
        lpTttW->lpszText = (LPWSTR)lpTttA->lpszText;
    }

    return TRUE;
}

#endif
