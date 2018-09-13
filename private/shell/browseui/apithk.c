#ifdef UNICODE
#error start menu won't run on win95 if this apithk.c is compiled unicode
#endif
//
//  APITHK.C
//
//  This file has API thunks that allow shdocvw to load and run on
//  multiple versions of NT or Win95.  Since this component needs
//  to load on the base-level NT 4.0 and Win95, any calls to system
//  APIs introduced in later OS versions must be done via GetProcAddress.
// 
//  Also, any code that may need to access data structures that are
//  post-4.0 specific can be added here.
//
//  NOTE:  this file does *not* use the standard precompiled header,
//         so it can set _WIN32_WINNT to a later version.
//


#include "priv.h"       // Don't use precompiled header here
#include "menubar.h"  // for the MENUBAR_* defines

HINSTANCE GetComctl32Hinst()
{
    static HINSTANCE s_hinst = NULL;
    if (!s_hinst)
        s_hinst = GetModuleHandle(TEXT("comctl32.dll"));
    return s_hinst;
}

STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes)
{
    if (g_bRunOnNT5 || g_bRunOnMemphis)
    {
        HCURSOR hcur = LoadCursor(NULL, IDC_HAND);  // from USER, system supplied
        if (hcur)
            return hcur;
    }

    return LoadCursor(GetComctl32Hinst(), IDC_HAND_INTERNAL);
}

/*----------------------------------------------------------
Purpose: Use the Microsoft ActiveAccessiblity routines to 
        notify Accessibility programs of events
*/
typedef void (* PFNNOTIFYWINEVENT)(DWORD event, HWND hwnd, LONG idObject, LONG idChild);
#define DONOTHING_WINEVENT (PFNNOTIFYWINEVENT)1
void NT5_NotifyWinEvent(
    IN DWORD event,
    IN HWND hwnd,
    IN LONG idObject,
    IN LONG idChild)
{
    static PFNNOTIFYWINEVENT pfn = NULL;

    if (NULL == pfn)
    {
        HMODULE hmod = GetModuleHandle(TEXT("USER32"));
        
        if (hmod)
            pfn = (PFNNOTIFYWINEVENT)GetProcAddress(hmod, "NotifyWinEvent");
        
        if (!pfn)
            pfn = DONOTHING_WINEVENT;
    }

    if (pfn != DONOTHING_WINEVENT)
        pfn(event, hwnd, idObject, idChild);
}


/*----------------------------------------------------------
Purpose: Use the Microsoft ActiveAccessiblity routines to 
        return an IAccessible object
*/
typedef LRESULT (* PFNLRESULTFROMOBJECT)(REFIID riid, WPARAM wParam, IUnknown* punk);

LRESULT ACCESSIBLE_LresultFromObject(
    IN REFIID riid,
    IN WPARAM wParam,
    OUT IUnknown* punk)
{
    static PFNLRESULTFROMOBJECT pfn = NULL;
    static BOOL fLoadAttempted = FALSE;
    LRESULT lRet = (LRESULT)E_FAIL;

    if (NULL == pfn && !fLoadAttempted)
    {
        // LoadLibrary here because OLEAcc is not loaded in the process
        HMODULE hmod = LoadLibrary(TEXT("OLEACC"));
        
        if (hmod)
            pfn = (PFNLRESULTFROMOBJECT)GetProcAddress(hmod, "LresultFromObject");
        else
            fLoadAttempted = TRUE;
    }

    if (pfn)
        lRet = pfn(riid, wParam, punk);

    return lRet;
}

/*----------------------------------------------------------
Purpose: Thunk for NT 5's AllowSetForegroundWindow

Returns: 
Cond:    --
*/
typedef BOOL (* PFNALLOWSFW)(DWORD dwProcessId);

BOOL
NT5_AllowSetForegroundWindow(
    IN DWORD dwProcessId )
{
    BOOL bRet = FALSE;
    static PFNALLOWSFW pfn = NULL;

    if (g_bRunOnNT5)
    {

        if (NULL == pfn)
        {
            HMODULE hmod = GetModuleHandle(TEXT("USER32"));
        
            if (hmod)
                pfn = (PFNALLOWSFW)GetProcAddress(hmod, "AllowSetForegroundWindow");
        }

        if (pfn)
            bRet = pfn(dwProcessId);
    }

    return bRet;    
}

typedef BOOL (* PFNANIMATEWINDOW)(HWND hwnd, DWORD dwTime, DWORD dwFlags);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's AnimateWindow.

Returns: 
Cond:    --
*/
BOOL
NT5_AnimateWindow(
    IN HWND hwnd,
    IN DWORD dwTime,
    IN DWORD dwFlags)
{
    BOOL bRet = FALSE;
    static PFNANIMATEWINDOW pfn = NULL;

    ASSERT(g_bRunOnMemphis || g_bRunOnNT5);

    if (NULL == pfn)
    {
        HMODULE hmod = GetModuleHandle(TEXT("USER32"));
        
        if (hmod)
            pfn = (PFNANIMATEWINDOW)GetProcAddress(hmod, "AnimateWindow");
    }

    if (pfn)
        bRet = pfn(hwnd, dwTime, dwFlags);

    return bRet;    
}

/*----------------------------------------------------------
Purpose: Position Menubands using NT5's AnimateWindow if available.

*/

void SlideAnimate(HWND hwnd, RECT* prc, UINT uFlags, UINT uSide)
{
    DWORD dwAnimateFlags = AW_CENTER;
    switch(uSide) 
    {
    case MENUBAR_LEFT:      dwAnimateFlags = AW_HOR_NEGATIVE;
        break;
    case MENUBAR_RIGHT:     dwAnimateFlags = AW_HOR_POSITIVE;
        break;
    case MENUBAR_TOP:       dwAnimateFlags = AW_VER_NEGATIVE;
        break;
    case MENUBAR_BOTTOM:    dwAnimateFlags = AW_VER_POSITIVE;
        break;
    }
    NT5_AnimateWindow(hwnd, 120, dwAnimateFlags | AW_SLIDE);
}

void AnimateSetMenuPos(HWND hwnd, RECT* prc, UINT uFlags, UINT uSide, BOOL fNoAnimate)
{
 
    if ((g_bRunOnMemphis || g_bRunOnNT5) && !fNoAnimate)
    {
        BOOL fAnimate = FALSE;
        SystemParametersInfo(SPI_GETMENUANIMATION, 0, &fAnimate, 0);
        if (fAnimate)
        {
            SetWindowPos(hwnd, HWND_TOPMOST, prc->left, prc->top,
                    RECTWIDTH(*prc), RECTHEIGHT(*prc), uFlags);
        
            fAnimate = FALSE;
#ifdef WINNT
            SystemParametersInfo(SPI_GETMENUFADE, 0, &fAnimate, 0);
#endif // WINNT
            if (fAnimate)
            {
                NT5_AnimateWindow(hwnd, 175, AW_BLEND);
            }
            else
            {
                SlideAnimate(hwnd, prc, uFlags, uSide);
            }
        }
        else
            goto UseSetWindowPos;
    }
    else
    {
UseSetWindowPos:
        // Enable the show window so that it gets displayed.
        uFlags |= SWP_SHOWWINDOW;

        SetWindowPos(hwnd, HWND_TOPMOST, prc->left, prc->top, RECTWIDTH(*prc), RECTHEIGHT(*prc), 
                     uFlags);
    }
}


/*----------------------------------------------------------
Purpose: Use the Microsoft ActiveAccessiblity routines to 
        return an IAccessible object
*/
typedef LRESULT (* PFNCREATESTDACCESSIBLEOBJECT)(HWND hwnd, LONG idObject, REFIID riid, void** ppvObj);


LRESULT ACCESSIBLE_CreateStdAccessibleObject(
    IN HWND hwnd,
    IN LONG idObject,
    IN REFIID riid,
    OUT void** ppvObj)
{
    static PFNCREATESTDACCESSIBLEOBJECT pfn = NULL;
    static BOOL fLoadAttempted = FALSE;
    LRESULT lRet = (LRESULT)E_FAIL;

    if (NULL == pfn && !fLoadAttempted)
    {
        // LoadLibrary here because OLEAcc is not loaded in the process
        HMODULE hmod = LoadLibrary(TEXT("OLEACC"));
        
        if (hmod)
            pfn = (PFNCREATESTDACCESSIBLEOBJECT)GetProcAddress(hmod, "CreateStdAccessibleObject");
        else
            fLoadAttempted = TRUE;
    }

    if (pfn)
        lRet = pfn(hwnd, idObject, riid, ppvObj);

    return lRet;
}


#if !defined(LSFW_LOCK) || !defined(LSFW_UNLOCK)
#define LSFW_LOCK       1
#define LSFW_UNLOCK     2
#endif

typedef BOOL (* PFNLOCKSETFOREGROUNDWINDOW)(UINT);

BOOL MyLockSetForegroundWindow(BOOL fLock)
{
    static PFNLOCKSETFOREGROUNDWINDOW pfn = (PFNLOCKSETFOREGROUNDWINDOW)-1;
    BOOL fRet = FALSE;

    if ((PFNLOCKSETFOREGROUNDWINDOW)-1 == pfn)
    {
        HMODULE hmod = GetModuleHandle(TEXT("USER32"));
        if (hmod)
            pfn = (PFNLOCKSETFOREGROUNDWINDOW)GetProcAddress(hmod, "LockSetForegroundWindow");
        else
            pfn = NULL;
    }

    if (pfn)
        fRet = pfn(fLock ? LSFW_LOCK : LSFW_UNLOCK);

    return fRet;
}

typedef UINT (* PFNSHEXTRACTICONSW)(LPCWSTR wszFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags);

STDAPI_(UINT) MyExtractIconsW(LPCWSTR wszFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags)
{
    UINT uiRet = 0;
    static PFNSHEXTRACTICONSW pfn = NULL;

    if (GetUIVersion() >= 5)
    {
        if (NULL == pfn)
        {
            HMODULE hmod = GetModuleHandle(TEXT("SHELL32"));
            if (hmod)
                pfn = (PFNSHEXTRACTICONSW)GetProcAddress(hmod, "SHExtractIconsW");
        }

        if (pfn)
            uiRet = pfn(wszFileName, nIconIndex, cxIcon, cyIcon, phicon, piconid, nIcons, flags);
    }

    return uiRet;    
}


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

BOOL BlendLayeredWindow(HWND hwnd, HDC hdcDest, POINT* ppt, SIZE* psize, HDC hdc, POINT* pptSrc, BYTE bBlendConst)
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

        bRet = pfn(hwnd, hdcDest, ppt, psize, hdc, pptSrc, 0, &blend, ULW_ALPHA);
    }

    return bRet;    
}

BOOL NT5_SetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF crKey,
    BYTE bAlpha,
    DWORD dwFlags)
{
    static BOOL (*pfn)(HWND, COLORREF, BYTE, DWORD) = NULL;

    if (pfn == NULL)
    {
        HMODULE hMod = LoadLibrary(TEXT("user32.dll"));
        if (hMod)
        {
            pfn = (BOOL (*)(HWND, COLORREF, BYTE, DWORD))GetProcAddress(hMod, "SetLayeredWindowAttributes");
        }
    }

    if (pfn)
        return pfn(hwnd, crKey, bAlpha, dwFlags);
    return 0;
}
