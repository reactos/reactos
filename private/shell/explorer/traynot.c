#include "cabinet.h"
#include "trayclok.h"
#include "traynot.h"
#include "rcids.h"

#define XXX_RENIM   0   // cache/reload ShellNotifyIcon's (see #if)

//
// Tray Notify Icon area implementation notes / details:
//
// - The icons are held in a toolbar with PTNPRIVICON on each button's lParam
//

#define INFO_TIMER       48
#define KEYBOARD_VERSION 3

#define NISP_SHAREDICONSOURCE 0x10000000 // says this is the source of a shared icon

#define INFO_INFO       0x00000001
#define INFO_WARNING    0x00000002
#define INFO_ERROR      0x00000003
#define INFO_ICON       0x00000003
#define ICON_HEIGHT      16
#define ICON_WIDTH       16
#define MAX_TIP_WIDTH    300

#define MIN_INFO_TIME   10000  // 10 secs is minimum time a balloon can be up
#define MAX_INFO_TIME   60000  // 1 min is the max time it can be up
typedef struct _TNINFOITEM
{
    INT_PTR   nIcon;
    TCHAR     szTitle[64];
    TCHAR     szInfo[256];
    UINT      uTimeout;
    DWORD     dwFlags;
    BOOL      bMinShown; // was this balloon shown for a min time?
} TNINFOITEM;

//
//  For Win64 compat, the icon and hwnd are handed around as DWORDs
//  (so they won't change size as they travel between 32-bit and
//  64-bit processes).
//
#define GetHIcon(pnid)  ((HICON)ULongToPtr(pnid->dwIcon))
#define GetHWnd(pnid)   ((HWND)ULongToPtr(pnid->dwWnd))

typedef struct _TNPRIVDATA
{
    HWND hwndNotify;
    HWND hwndToolbar;
    HWND hwndClock;
    HIMAGELIST himlIcons;
    int nCols;
    INT_PTR iVisCount;
    BITBOOL fKey:1;
    BITBOOL fReturn:1;
    HWND  hwndInfoTip;
    UINT_PTR uInfoTipTimer;
    TNINFOITEM *pinfo; //current balloon being shown
    HDPA       hdpaInfo; // array of balloons waiting in queque
} TNPRIVDATA, *PTNPRIVDATA;

typedef struct _TNPRIVICON
{
    HWND hWnd;
    UINT uID;
    UINT uCallbackMessage;
    DWORD dwState;
    UINT uVersion;
    HICON hIcon;
} TNPRIVICON, *PTNPRIVICON;


LRESULT _TNSize(PTNPRIVDATA ptnd);
LRESULT _TNSendNotify(PTNPRIVICON ptnpi, UINT uMsg);
void Tray_SizeWindows();
void Tray_Unhide();
BOOL AllowSetForegroundWindow(DWORD dwProcessId);
LRESULT _TNMouseEvent(PTNPRIVDATA ptnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fClickDown);

void _TNShowInfoTip(PTNPRIVDATA ptnd, INT_PTR nIcon, BOOL bShow, BOOL bAsync);
       
void SHAllowSetForegroundWindow(HWND hwnd)
{
    if (g_bRunOnNT5) {
        DWORD dwProcessId = 0;
        GetWindowThreadProcessId(hwnd, &dwProcessId);
        AllowSetForegroundWindow(dwProcessId);
    }
}

const TCHAR c_szTrayNotify[] = TEXT("TrayNotifyWnd");

void _TNSetImage(PTNPRIVDATA ptnd, INT_PTR iIndex, int iImage)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.dwMask = TBIF_IMAGE | TBIF_BYINDEX;
    tbbi.iImage = iImage;

    SendMessage(ptnd->hwndToolbar, TB_SETBUTTONINFO, iIndex, (LPARAM)&tbbi);
}

void _TNSetText(PTNPRIVDATA ptnd, INT_PTR iIndex, LPTSTR pszText)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.dwMask = TBIF_TEXT | TBIF_BYINDEX;
    tbbi.pszText = pszText;
    tbbi.cchText = -1;
    SendMessage(ptnd->hwndToolbar, TB_SETBUTTONINFO, iIndex, (LPARAM)&tbbi);
}

int _TNGetImage(PTNPRIVDATA ptnd, INT_PTR iIndex)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.dwMask = TBIF_IMAGE | TBIF_BYINDEX;

    SendMessage(ptnd->hwndToolbar, TB_GETBUTTONINFO, iIndex, (LPARAM)&tbbi);
    return tbbi.iImage;
}

#define _TNGetDataByIndex(ptnd, i) _TNGetData(ptnd, i, TRUE)
PTNPRIVICON _TNGetData(PTNPRIVDATA ptnd, INT_PTR i, BOOL byIndex)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.lParam = 0;
    tbbi.dwMask = TBIF_LPARAM;
    if (byIndex)
        tbbi.dwMask |= TBIF_BYINDEX;
    SendMessage(ptnd->hwndToolbar, TB_GETBUTTONINFO, i, (LPARAM)&tbbi);
    return (PTNPRIVICON)(LPVOID)tbbi.lParam;
}

INT_PTR _TNGetCount(PTNPRIVDATA ptnd)
{
    return SendMessage(ptnd->hwndToolbar, TB_BUTTONCOUNT, 0, 0L);
}

INT_PTR _TNGetVisibleCount(PTNPRIVDATA ptnd)
{
    if (ptnd->iVisCount != -1) {
        return ptnd->iVisCount;
    } else {
        INT_PTR i;
        INT_PTR iRet = 0;
        INT_PTR iCount = _TNGetCount(ptnd);
        TBBUTTONINFO tbbi;
        tbbi.cbSize = SIZEOF(tbbi);
        tbbi.dwMask = TBIF_STATE | TBIF_BYINDEX;

        for (i = 0; i < iCount; i++) {

            SendMessage(ptnd->hwndToolbar, TB_GETBUTTONINFO, i, (LPARAM)&tbbi);
            if (!(tbbi.fsState & TBSTATE_HIDDEN)) {
                iRet++;
            }
        }
        ptnd->iVisCount = iRet;
        return iRet;
    }
}

int _TNFindImageIndex(PTNPRIVDATA ptnd, HICON hIcon, BOOL fSetAsSharedSource)
{
    INT_PTR i;
    INT_PTR iCount = _TNGetCount(ptnd);
    
    for (i = 0; i < iCount; i++) {
        PTNPRIVICON ptnpi = _TNGetData(ptnd, i, TRUE);
        if (ptnpi->hIcon == hIcon) {
            
            // if we're supposed to mark this as a shared icon source and it's not itself a shared icon
            // target, mark it now.  this is to allow us to recognize when the source icon changes and
            // that we can know that we need to find other indicies and update them too.
            if (fSetAsSharedSource && !(ptnpi->dwState & NIS_SHAREDICON))
                ptnpi->dwState |= NISP_SHAREDICONSOURCE;
            return _TNGetImage(ptnd, i);
        }
    }
    return -1;
}

/*
 ** _TNRemoveImage
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

void _TNRemoveImage(PTNPRIVDATA ptnd, UINT uIMLIndex)
{
    INT_PTR nCount;
    INT_PTR i;

    if (uIMLIndex != (UINT)-1) 
    {
        ImageList_Remove(ptnd->himlIcons, uIMLIndex);

        nCount = _TNGetCount(ptnd);
        for (i = nCount - 1; i >= 0; i--) 
        {
            int iImage = _TNGetImage(ptnd, i);
            if (iImage > (int)uIMLIndex)
                _TNSetImage(ptnd, i, iImage - 1);
        }
    }
}

/*
 ** _TNFindNotify
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

INT_PTR _TNFindNotify(PTNPRIVDATA ptnd, PNOTIFYICONDATA32 pnid)
{
    INT_PTR i;
    PTNPRIVICON ptnpi;
    INT_PTR nCount = _TNGetCount(ptnd);

    for (i = nCount - 1; i >= 0; --i)
    {
        ptnpi = _TNGetDataByIndex(ptnd, i);
        ASSERT(ptnpi);
        if (ptnpi->hWnd==GetHWnd(pnid) && ptnpi->uID==pnid->uID)
        {
            break;
        }
    }

    return(i);
}

//---------------------------------------------------------------------------
// Returns TRUE if either the images are OK as they are or they needed
// resizing and the resize process worked. FALSE otherwise.
BOOL _TNCheckAndResizeImages(PTNPRIVDATA ptnd)
{
    HIMAGELIST himlOld, himlNew;
    int cxSmIconNew, cySmIconNew, cxSmIconOld, cySmIconOld;
    int i, cItems;
    COLORREF clBkNew;
    HICON hicon;
    BOOL fOK = TRUE;

    if (!ptnd)
        return 0;

    himlOld = ptnd->himlIcons;

    // Do dimensions match current icons?
    cxSmIconNew = GetSystemMetrics(SM_CXSMICON);
    cySmIconNew = GetSystemMetrics(SM_CYSMICON);
    ImageList_GetIconSize(himlOld, &cxSmIconOld, &cySmIconOld);
    if (cxSmIconNew != cxSmIconOld || cySmIconNew != cySmIconOld)
    {
        UINT flags = ILC_MASK;
        // Nope, we're gonna need a new imagelist.
        if(IS_WINDOW_RTL_MIRRORED(ptnd->hwndToolbar))
        {
            flags |= ILC_MIRROR;
        }        
        himlNew = ImageList_Create(cxSmIconNew, cySmIconNew, flags, 0, 1);
        if (himlNew)
        {
            clBkNew = GetSysColor(COLOR_3DFACE);
            ImageList_SetBkColor(himlNew, clBkNew);

            // Copy the images over to the new image list.
            cItems = ImageList_GetImageCount(himlOld);
            for (i = 0; i < cItems; i++)
            {
                // REVIEW Lame - there's no way to copy images to an empty
                // imagelist, resizing it on the way.
                hicon = ImageList_GetIcon(himlOld, i, ILD_NORMAL);
                if (hicon)
                {
                    if (ImageList_AddIcon(himlNew, hicon) == -1)
                    {
                        // Couldn't copy image so bail.
                        fOK = FALSE;
                    }
                    DestroyIcon(hicon);
                }
                else
                {
                    fOK = FALSE;
                }

                // FU - bail.
                if (!fOK)
                    break;
            }

            // Did everything copy over OK?
            if (fOK)
            {
                // Yep, Set things up to use the new one.
                ptnd->himlIcons = himlNew;
                // Destroy the old icon cache.
                ImageList_Destroy(himlOld);
                SendMessage(ptnd->hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)ptnd->himlIcons);
                SendMessage(ptnd->hwndToolbar, TB_AUTOSIZE, 0, 0);
            }
            else
            {
                // Nope, stick with what we have.
                ImageList_Destroy(himlNew);
            }
        }
    }

    return fOK;
}

void _TNActivateTips(PTNPRIVDATA ptnd, BOOL bActivate)
{
    HWND hwnd;

    hwnd = (HWND)SendMessage(ptnd->hwndToolbar, TB_GETTOOLTIPS, 0, 0);
    if (hwnd)
        SendMessage(hwnd, TTM_ACTIVATE, (WPARAM)bActivate, 0);
}

void _TNInfoTipMouseClick(PTNPRIVDATA ptnd, int x, int y)
{
    if (ptnd->pinfo)
    {
        RECT rect;
        POINT pt;

        pt.x = x;
        pt.y = y;

        GetWindowRect(ptnd->hwndInfoTip, &rect);
        // x & y are mapped to our window so map the rect to our window as well
        MapWindowRect(HWND_DESKTOP, ptnd->hwndNotify, &rect);

        if (PtInRect(&rect, pt))
            _TNShowInfoTip(ptnd, ptnd->pinfo->nIcon, FALSE, FALSE);
    }
}

#define PADDING 1

void _TNPositionInfoTip(PTNPRIVDATA ptnd)
{
    int x = 0, y = 0;
    RECT rc;

    if (!ptnd->pinfo)
        return;
        
    if (SendMessage(ptnd->hwndToolbar, TB_GETITEMRECT, (WPARAM)ptnd->pinfo->nIcon, (LPARAM)&rc))
    {
        MapWindowPoints(ptnd->hwndToolbar, HWND_DESKTOP, (LPPOINT)&rc, 2);
        x = (rc.left + rc.right)/2;
        y = (rc.top  + rc.bottom)/2;
    }
    
    SendMessage(ptnd->hwndInfoTip, TTM_TRACKPOSITION, 0, MAKELONG(x,y));
}

int CALLBACK DeleteDPAPtrCB(void *pItem, void *pData);

BOOL _IsScreenSaverRunning()
{
    BOOL fRunning;
    if (SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &fRunning, 0))
    {
        return fRunning;
    }

    return FALSE;
}

void _TNShowInfoTip(PTNPRIVDATA ptnd, INT_PTR nIcon, BOOL bShow, BOOL bAsync)
{
    TOOLINFO ti={0};

    ti.cbSize = SIZEOF(ti);
    ti.hwnd = ptnd->hwndNotify;
    ti.uId = (INT_PTR)ptnd->hwndNotify;

    // make sure we only show/hide what we intended to show/hide
    if (ptnd->pinfo && ptnd->pinfo->nIcon == nIcon)
    {
        PTNPRIVICON ptnpi = _TNGetDataByIndex(ptnd, nIcon);

        if (!ptnpi || ptnpi->dwState & NIS_HIDDEN)
        {
            // icon is hidden, cannot show it's balloon
            bShow = FALSE; //show the next balloon instead
        }
        
        if (bShow)
        {
            // If there is a fullscreen app or a screen saver running,
            // we don't show anything and we empty the queue
            if (g_ts.fStuckRudeApp || _IsScreenSaverRunning())
            {
                LocalFree(ptnd->pinfo);
                ptnd->pinfo = NULL;
                DPA_EnumCallback(ptnd->hdpaInfo, DeleteDPAPtrCB, NULL);
                DPA_DeleteAllPtrs(ptnd->hdpaInfo);
                return;
            }

            if (bAsync)
                PostMessage(ptnd->hwndNotify, TNM_ASYNCINFOTIP, (WPARAM)nIcon, 0);
            else
            {
                SendMessage(ptnd->hwndInfoTip, TTM_SETTITLE, ptnd->pinfo->dwFlags & INFO_ICON, (LPARAM)ptnd->pinfo->szTitle);
                _TNPositionInfoTip(ptnd);
                ti.lpszText = ptnd->pinfo->szInfo;
                // if tray is in auto hide mode unhide it
                Tray_Unhide();
                g_ts.fBalloonUp = TRUE;
                SendMessage(ptnd->hwndInfoTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
                // disable regular tooltips
                _TNActivateTips(ptnd, FALSE);
                // show the balloon
                SendMessage(ptnd->hwndInfoTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
                ptnd->uInfoTipTimer = SetTimer(ptnd->hwndNotify, INFO_TIMER, MIN_INFO_TIME, NULL);
            }
        }
        else
        {
            LocalFree(ptnd->pinfo);
            ptnd->pinfo = NULL;
            if (ptnd->uInfoTipTimer)
                KillTimer(ptnd->hwndNotify, ptnd->uInfoTipTimer);
            // hide the previous tip if any
            SendMessage(ptnd->hwndInfoTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)0);
            ti.lpszText = NULL;
            SendMessage(ptnd->hwndInfoTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

            // we are hiding the current balloon. are there any waiting? yes, then show the first one
            if (ptnd->hdpaInfo && DPA_GetPtrCount(ptnd->hdpaInfo) > 0)
            {
                TNINFOITEM *pii = (TNINFOITEM *)DPA_DeletePtr(ptnd->hdpaInfo, 0);

                ASSERT(pii);
                ptnd->pinfo = pii;
                _TNShowInfoTip(ptnd, pii->nIcon, TRUE, bAsync);
            }
            else
            {
                g_ts.fBalloonUp = FALSE; // this will take care of hiding the tray if necessary
                _TNActivateTips(ptnd, TRUE);
            }
        }
    }
    else if (ptnd->pinfo && !bShow)
    {
        // we wanted to hide something that wasn't showing up
        // maybe it's in the queue
        if (ptnd->hdpaInfo && DPA_GetPtrCount(ptnd->hdpaInfo) > 0)
        {
            TNINFOITEM *pii;
            int  i, cItems = DPA_GetPtrCount(ptnd->hdpaInfo);
            
            for (i=0; i < cItems; i++)
            {
                pii = (TNINFOITEM *)DPA_GetPtr(ptnd->hdpaInfo, i);

                ASSERT(pii);
                if (pii->nIcon == nIcon)
                {
                    DPA_DeletePtr(ptnd->hdpaInfo, i); // this just removes it from the dpa
                    LocalFree(pii);
                    return; // just remove the first one
                }
            }
        }
    }
}

#define ICON_WIDTH   16
#define ICON_HEIGHT  16
void _TNSetInfoTip(PTNPRIVDATA ptnd, INT_PTR nIcon, PNOTIFYICONDATA32 pnid, BOOL bAsync)
{
    if (!ptnd->hwndInfoTip)
    {
        ptnd->hwndInfoTip = CreateWindow(TOOLTIPS_CLASS, NULL,
                                         WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         ptnd->hwndNotify, NULL, hinstCabinet,
                                         NULL);
        SetWindowPos(ptnd->hwndInfoTip, HWND_TOPMOST,
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        if (ptnd->hwndInfoTip) 
        {
            TOOLINFO ti = {0};
            RECT     rc = {0,-2,0,0};

            ti.cbSize = SIZEOF(ti);
            ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_TRANSPARENT;
            ti.hwnd = ptnd->hwndNotify;
            ti.uId = (UINT_PTR)ptnd->hwndNotify;
            //ti.lpszText = NULL;
            // set the version so we can have non buggy mouse event forwarding
            SendMessage(ptnd->hwndInfoTip, CCM_SETVERSION, COMCTL32_VERSION, 0);
            SendMessage(ptnd->hwndInfoTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
            SendMessage(ptnd->hwndInfoTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)MAX_TIP_WIDTH);
            SendMessage(ptnd->hwndInfoTip, TTM_SETMARGIN, 0, (LPARAM)&rc);
        }

        ASSERT(ptnd->hdpaInfo == NULL);
        ptnd->hdpaInfo = DPA_Create(10);
    }
    
    if (ptnd->hwndInfoTip)
    {
        // show the new one...
        if (pnid && pnid->szInfo[0] != TEXT('\0'))
        {
            TNINFOITEM *pii = LocalAlloc(LPTR, SIZEOF(TNINFOITEM));
            if (pii)
            {
                pii->nIcon = nIcon;
                lstrcpyn(pii->szInfo, pnid->szInfo, ARRAYSIZE(pii->szInfo));
                lstrcpyn(pii->szTitle, pnid->szInfoTitle, ARRAYSIZE(pii->szTitle));
                pii->uTimeout = pnid->uTimeout;
                if (pii->uTimeout < MIN_INFO_TIME)
                    pii->uTimeout = MIN_INFO_TIME;
                else if (pii->uTimeout > MAX_INFO_TIME)
                    pii->uTimeout = MAX_INFO_TIME;
                pii->dwFlags  = pnid->dwInfoFlags;
                // if pinfo != NULL then we have a balloon showing right now
                // if hdpaInfo != NULL then we just add the new balloon to the queue
                // else we show the new one right away -- it hides the old one but that's ok
                // this is just low mem case when we could not alloc hdpa
                // we also show the info tip right away if the current tip has been shown 
                // for at least the min time and there are no tips waiting (queue size == 0)
                if (ptnd->pinfo && ptnd->hdpaInfo && 
                    (!ptnd->pinfo->bMinShown || DPA_GetPtrCount(ptnd->hdpaInfo) > 0))
                {
                    if (DPA_AppendPtr(ptnd->hdpaInfo, pii) == -1)
                    {
                        LocalFree(pii);
                    }
                    return;
                }
                
                if (ptnd->pinfo)
                {
                    LocalFree(ptnd->pinfo);
                }

                ptnd->pinfo = pii;
                
                _TNShowInfoTip(ptnd, nIcon, TRUE, bAsync);
            }
        }
        else
        {
            _TNShowInfoTip(ptnd, nIcon, FALSE, FALSE);
        }
    }   
}

/*
 ** _TNModifyNotify
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

#if XXX_RENIM
typedef enum {
    COP_ADD, COP_DEL,
} CACHEOP;
void CacheNID(CACHEOP op, INT_PTR nIcon, PNOTIFYICONDATA32 pNidMod);
#endif

BOOL _TNModifyNotify(PTNPRIVDATA ptnd, PNOTIFYICONDATA32 pnid, INT_PTR nIcon, BOOL *pbRefresh)
{
    BOOL fResize = FALSE;
    BOOL bHideButton = FALSE;
    PTNPRIVICON ptnpi = _TNGetDataByIndex(ptnd, nIcon);
    if (!ptnpi)
    {
        return(FALSE);
    }

    _TNCheckAndResizeImages(ptnd);

    if (pnid->uFlags & NIF_STATE) 
    {
#define NIS_VALIDMASK (NIS_HIDDEN | NIS_SHAREDICON)
        DWORD dwOldState = ptnpi->dwState;
        // validate mask
        if (pnid->dwStateMask & ~NIS_VALIDMASK)
            return FALSE;

        ptnpi->dwState = (pnid->dwState & pnid->dwStateMask) | (ptnpi->dwState & ~pnid->dwStateMask);
        
        if (pnid->dwStateMask & NIS_HIDDEN) 
        {
            TBBUTTONINFO tbbi;
            tbbi.cbSize = SIZEOF(tbbi);
            tbbi.dwMask = TBIF_STATE | TBIF_BYINDEX;

            SendMessage(ptnd->hwndToolbar, TB_GETBUTTONINFO, nIcon, (LPARAM)&tbbi);
            if (ptnpi->dwState & NIS_HIDDEN)
            {
                tbbi.fsState |= TBSTATE_HIDDEN;
                bHideButton = TRUE;
            }
            else 
                tbbi.fsState &= ~TBSTATE_HIDDEN;
            SendMessage(ptnd->hwndToolbar, TB_SETBUTTONINFO, nIcon, (LPARAM)&tbbi);
            ptnd->iVisCount = -1;
        }
        
        if ((pnid->dwState ^ dwOldState) & NIS_SHAREDICON) 
        {
            if (dwOldState & NIS_SHAREDICON) 
            {
                // if we're going from shared to not shared, 
                // clear the icon
                _TNSetImage(ptnd, nIcon, -1);
                ptnpi->hIcon = NULL;
            }
        }
        fResize |= ((pnid->dwState ^ dwOldState) & NIS_HIDDEN);
    }

    // The icon is the only thing that can fail, so I will do it first
    if (pnid->uFlags & NIF_ICON)
    {
        int iImageNew;
        if (ptnpi->dwState & NIS_SHAREDICON) 
        {
            iImageNew = _TNFindImageIndex(ptnd, GetHIcon(pnid), TRUE);
            if (iImageNew == -1)
                return FALSE;
        } 
        else 
        {
            int iImageOld = _TNGetImage(ptnd, nIcon);
            if (GetHIcon(pnid))
            {
                // Replace icon knows how to handle -1 for add
                iImageNew = ImageList_ReplaceIcon(ptnd->himlIcons, iImageOld,
                    GetHIcon(pnid));
                if (iImageNew < 0)
                {
                    return(FALSE);
                }
            }
            else
            {
                _TNRemoveImage(ptnd, iImageOld);
                iImageNew = -1;
            }
            
            if (ptnpi->dwState & NISP_SHAREDICONSOURCE) 
            {
                INT_PTR i;
                INT_PTR iCount = _TNGetCount(ptnd);
                // if we're the source of shared icons, we need to go update all the other icons that
                // are using our icon
                for (i = 0; i < iCount; i++) 
                {
                    if (_TNGetImage(ptnd, i) == iImageOld) 
                    {
                        PTNPRIVICON ptnpiTemp = _TNGetDataByIndex(ptnd, i);
                        ptnpiTemp->hIcon = GetHIcon(pnid);
                        _TNSetImage(ptnd, i, iImageNew);
                    }
                }
            }
            if (iImageOld == -1 || iImageNew == -1)
                fResize = TRUE;
        }
        ptnpi->hIcon = GetHIcon(pnid);
        _TNSetImage(ptnd, nIcon, iImageNew);
    }

    if (pnid->uFlags & NIF_MESSAGE)
    {
        ptnpi->uCallbackMessage = pnid->uCallbackMessage;
    }
    
    if (pnid->uFlags & NIF_TIP)
    {
        _TNSetText(ptnd, nIcon, pnid->szTip);
        if(!bHideButton && pbRefresh && pnid->uFlags == NIF_TIP)
        {
            *pbRefresh = FALSE;
        }
    }

    if (fResize)
        _TNSize(ptnd);

    // infotip is up and we are hiding the button it corresponds to...
    if (bHideButton && ptnd->pinfo && ptnd->pinfo->nIcon == nIcon)
        _TNShowInfoTip(ptnd, ptnd->pinfo->nIcon, FALSE, TRUE);

    // need to have info stuff done after resize because we need to
    // position the infotip relative to the hwndToolbar
    if (pnid->uFlags & NIF_INFO)
    {
        // if button is hidden we don't show infotip
        if (!(ptnpi->dwState & NIS_HIDDEN))
            _TNSetInfoTip(ptnd, nIcon, pnid, fResize);
    }
    
#if XXX_RENIM
    CacheNID(COP_ADD, nIcon, pnid);
#endif

    return(TRUE);
}

BOOL _TNSetVersionNotify(PTNPRIVDATA ptnd, PNOTIFYICONDATA32 pnid, INT_PTR nIcon)
{
    PTNPRIVICON ptnpi = _TNGetDataByIndex(ptnd, nIcon);
    if (!ptnpi)
        return FALSE;

    if (pnid->uVersion < NOTIFYICON_VERSION) 
    {
        ptnpi->uVersion = 0;
        return TRUE;
    } 
    else if (pnid->uVersion == NOTIFYICON_VERSION) 
    {
        ptnpi->uVersion = NOTIFYICON_VERSION;
        return TRUE;
    } 
    else 
    {
        return FALSE;
    }

}

void _TNFreeNotify(PTNPRIVDATA ptnd, PTNPRIVICON ptnpi, int iImage)
{
    //if it wasn't sharing an icon with another guy, go ahead and delete it
    if (!(ptnpi->dwState & NIS_SHAREDICON)) 
        _TNRemoveImage(ptnd, iImage);

    LocalFree((HLOCAL)ptnpi);
}

/*
 ** _TNDeleteNotify
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

BOOL_PTR _TNDeleteNotify(PTNPRIVDATA ptnd, INT_PTR nIcon)
{
    // delete info tip if showing
    if (ptnd->pinfo && ptnd->pinfo->nIcon == nIcon) 
    {
        KillTimer(ptnd->hwndNotify, ptnd->uInfoTipTimer);
        SendMessage(ptnd->hwndInfoTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)0);
        ptnd->uInfoTipTimer = 0;
    }
    else
        PostMessage(ptnd->hwndNotify, TNM_ASYNCINFOTIP, 0, 0);

    if (ptnd->hdpaInfo && DPA_GetPtrCount(ptnd->hdpaInfo) > 0)
    {
        TNINFOITEM *pii;
        int  i, cItems = DPA_GetPtrCount(ptnd->hdpaInfo);
        
        for (i=cItems-1; i >= 0; i--)
        {
            pii = (TNINFOITEM *)DPA_GetPtr(ptnd->hdpaInfo, i);

            ASSERT(pii);
            if (pii->nIcon == nIcon)
            {
                DPA_DeletePtr(ptnd->hdpaInfo, i); // this just removes it from the dpa
                LocalFree(pii);
            }
            else if (nIcon < pii->nIcon)
            {
                pii->nIcon--;
            }
        }
    }

    if (ptnd->pinfo)
    {
        if (nIcon == ptnd->pinfo->nIcon)
        {
            // frees pinfo and shows the next balloon if any
            _TNShowInfoTip(ptnd, ptnd->pinfo->nIcon, FALSE, TRUE); 
        }
        else if (nIcon < ptnd->pinfo->nIcon)
            ptnd->pinfo->nIcon--;
        else
            PostMessage(ptnd->hwndNotify, TNM_ASYNCINFOTIP, (WPARAM)ptnd->pinfo->nIcon, 0);
    }

#if XXX_RENIM
    CacheNID(COP_DEL, nIcon, NULL);
#endif

    ptnd->iVisCount = -1;
    return SendMessage(ptnd->hwndToolbar, TB_DELETEBUTTON, nIcon, 0);
}


/*
 ** _TNInsertNotify
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

BOOL _TNInsertNotify(PTNPRIVDATA ptnd, PNOTIFYICONDATA32 pnid)
{
    PTNPRIVICON ptnpi;
    TBBUTTON tbb;
    static int s_iNextID = 0;
    
    // First insert a totally "default" icon
    ptnpi = LocalAlloc(LPTR, SIZEOF(TNPRIVICON));
    if (!ptnpi)
    {
        return FALSE;
    }

    ptnpi->hWnd = GetHWnd(pnid);
    ptnpi->uID = pnid->uID;


    tbb.dwData = (DWORD_PTR)ptnpi;
    tbb.iBitmap = -1;
    tbb.idCommand = s_iNextID++;
    tbb.fsStyle = TBSTYLE_BUTTON;
    tbb.fsState = TBSTATE_ENABLED;
    tbb.iString = -1;
    if (SendMessage(ptnd->hwndToolbar, TB_ADDBUTTONS, 1, (LPARAM)&tbb)) {
        INT_PTR insert = _TNGetCount(ptnd) - 1;

        // Then modify this icon with the specified info
        if (!_TNModifyNotify(ptnd, pnid, insert, NULL))
        {
            _TNDeleteNotify(ptnd, insert);
            // Note that we do not go to the LocalFree
            return FALSE;
        }
    }
    return(TRUE);
}

// set the mouse cursor to the cetner of the button.
// do this becaus our tray notifies don't have enough data slots to
// pass through info about the button's position.
void _TNSetCursorPos(PTNPRIVDATA ptnd, INT_PTR i)
{
    RECT rc;
    if (SendMessage(ptnd->hwndToolbar, TB_GETITEMRECT, i, (LPARAM)&rc)) {
        MapWindowPoints(ptnd->hwndToolbar, HWND_DESKTOP, (LPPOINT)&rc, 2);
        SetCursorPos((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
    }
}

LRESULT _TNSendNotify(PTNPRIVICON ptnpi, UINT uMsg)
{
    if (ptnpi->uCallbackMessage && ptnpi->hWnd)
       return SendNotifyMessage(ptnpi->hWnd, ptnpi->uCallbackMessage, ptnpi->uID, uMsg);
    return 0;
}


LRESULT CALLBACK _TNToolbarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    BOOL fClickDown = FALSE;
    PTNPRIVDATA ptnd = (PTNPRIVDATA)dwRefData;

    switch (uMsg) {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        fClickDown = TRUE;
        // Fall through

    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        _TNMouseEvent(ptnd, uMsg, wParam, lParam, fClickDown);
        break;
        
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_RETURN:
            ptnd->fReturn = TRUE;
            break;
            
        case VK_SPACE:
            ptnd->fReturn = FALSE;
            break;
        }
        break;

    case WM_CONTEXTMENU:
    {
        HWND    hwnd;
        INT_PTR i = SendMessage(ptnd->hwndToolbar, TB_GETHOTITEM, 0, 0);
        PTNPRIVICON ptnpi = _TNGetDataByIndex(ptnd, i);
        if (lParam == (LPARAM)-1)
            ptnd->fKey = TRUE;

        if (ptnd->fKey) {
            _TNSetCursorPos(ptnd, i);
        }

        hwnd = (HWND)SendMessage(ptnd->hwndToolbar, TB_GETTOOLTIPS, 0, 0);
        if (hwnd)
            SendMessage(hwnd, TTM_POP, 0, 0);
            
        if (ptnpi) {
            SHAllowSetForegroundWindow(ptnpi->hWnd);
            if (ptnpi->uVersion >= KEYBOARD_VERSION) {
                _TNSendNotify(ptnpi, WM_CONTEXTMENU);
            } else {
                if (ptnd->fKey) {
                    _TNSendNotify(ptnpi, WM_RBUTTONDOWN);
                    _TNSendNotify(ptnpi, WM_RBUTTONUP);
                }
            }
        }
        return 0;
    }
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

/*
 ** _TNCreate
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

#if XXX_RENIM
void RestoreNIDList(PTNPRIVDATA ptnd);
#endif

LRESULT _TNCreate(HWND hWnd)
{
    HWND hwndClock;
    PTNPRIVDATA ptnd;
    HWND hwndTooltips;
    UINT flags = ILC_MASK;

    ptnd = (PTNPRIVDATA)LocalAlloc(LPTR, SIZEOF(TNPRIVDATA));
    if (!ptnd)
    {
        return(-1);
    }

    hwndClock = ClockCtl_Create(hWnd, IDC_CLOCK, hinstCabinet);
    if (!hwndClock)
    {
        LocalFree(ptnd);
        return(-1);
    }


    SetWindowLongPtr(hWnd, 0, (LONG_PTR)ptnd);

    ptnd->iVisCount = -1;
    ptnd->hwndNotify = hWnd;
    ptnd->hwndClock = hwndClock;
    ptnd->hwndToolbar =  CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                                        WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT |
                                        TBSTYLE_TOOLTIPS |
                                        WS_CLIPCHILDREN |
                                        WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN |
                                        CCS_NORESIZE | TBSTYLE_WRAPABLE ,
                                        0, 0, 0, 0, hWnd, 0, hinstCabinet, NULL);

    SendMessage(ptnd->hwndToolbar, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
    hwndTooltips = (HWND)SendMessage(ptnd->hwndToolbar, TB_GETTOOLTIPS, 0, 0);
    SHSetWindowBits(hwndTooltips, GWL_STYLE, TTS_ALWAYSTIP, TTS_ALWAYSTIP);
    SendMessage(ptnd->hwndToolbar, TB_SETPADDING, 0, MAKELONG(2, 2));
    SendMessage(ptnd->hwndToolbar, TB_SETMAXTEXTROWS, 0, 0);
    SendMessage(ptnd->hwndToolbar, CCM_SETVERSION, COMCTL32_VERSION, 0);
    SendMessage(ptnd->hwndToolbar, TB_SETEXTENDEDSTYLE, 
                TBSTYLE_EX_INVERTIBLEIMAGELIST, TBSTYLE_EX_INVERTIBLEIMAGELIST);
    if(IS_WINDOW_RTL_MIRRORED(ptnd->hwndToolbar))
    {
        flags |= ILC_MIRROR;
    }
    ptnd->himlIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                       flags, 0, 1);
    if (!ptnd->himlIcons)
    {
        return(-1);
    }


    SendMessage(ptnd->hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)ptnd->himlIcons);

    ImageList_SetBkColor(ptnd->himlIcons, GetSysColor(COLOR_3DFACE));

    // if this fails, not that big a deal... we'll still show, but won't handle clicks
    SetWindowSubclass(ptnd->hwndToolbar, _TNToolbarWndProc, 0, (DWORD_PTR)ptnd);

#if XXX_RENIM
    RestoreNIDList(ptnd);
#endif

    return(0);
}

int CALLBACK DeleteDPAPtrCB(void *pItem, void *pData)
{
    LocalFree(pItem);
    return TRUE;
}

/*
 ** _TNDestroy
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

LRESULT _TNDestroy(PTNPRIVDATA ptnd)
{
    if (ptnd)
    {
        RemoveWindowSubclass(ptnd->hwndToolbar, _TNToolbarWndProc, 0);
        while (_TNDeleteNotify(ptnd, 0))
        {
            // Continue while there are icondata's to delete
        }

        if (ptnd->himlIcons)
        {
            ImageList_Destroy(ptnd->himlIcons);
        }

        if (ptnd->hwndInfoTip)
        {
            DestroyWindow(ptnd->hwndInfoTip);
            ptnd->hwndInfoTip = NULL;
        }
        if (ptnd->pinfo)
        {
            LocalFree(ptnd->pinfo);
            ptnd->pinfo = NULL;
        }
        if (ptnd->hdpaInfo)
        {
            DPA_DestroyCallback(ptnd->hdpaInfo, DeleteDPAPtrCB, NULL);
        }

        SetWindowLongPtr(ptnd->hwndNotify, 0, 0);
        LocalFree((HLOCAL)ptnd);
    }

    return(0);
}


/*
 ** _TNPaint
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

LRESULT _TNPaint(PTNPRIVDATA ptnd)
{
    HDC hdc;
    PAINTSTRUCT ps;
    HWND hWnd = ptnd->hwndNotify;

    hdc = BeginPaint(hWnd, &ps);

    // Setting to the current color returns immediately, so don't bother checking
    ImageList_SetBkColor(ptnd->himlIcons, GetSysColor(COLOR_3DFACE));

    EndPaint(hWnd, &ps);
    return(0);
}


#define ROWSCOLS(_nTot, _nROrC) ((_nTot+_nROrC-1)/_nROrC)

int _TNMatchIconsHorz(int nMatchHorz, INT_PTR nIcons, POINT *ppt)
{
    int nRows, nCols;
    int xIcon, yIcon;

    if (!nIcons)
    {
        ppt->x = ppt->y = 0;
        return(0);
    }

    xIcon = GetSystemMetrics(SM_CXSMICON) + 2*g_cxBorder;
    yIcon = GetSystemMetrics(SM_CYSMICON) + 2*g_cyBorder;

    // We can put the icons below or to the left of the clock, which
    // one gives a smaller total height?
    nCols = (nMatchHorz-3*g_cxBorder)/xIcon;
    if (nCols < 1)
    {
        nCols = 1;
    }
    nRows = ROWSCOLS((int)nIcons, nCols);

    ppt->x = g_cxEdge + (nCols*xIcon) + 2*g_cxBorder;
    ppt->y = (nRows*yIcon) + 2*g_cyBorder;

    return(nCols);
}


int _TNMatchIconsVert(int nMatchVert, INT_PTR nIcons, POINT *ppt)
{
    int nRows, nCols;
    int xIcon, yIcon;

    if (!nIcons)
    {
        ppt->x = ppt->y = 0;
        return(0);
    }

    xIcon = GetSystemMetrics(SM_CXSMICON) + 2*g_cxBorder;
    yIcon = GetSystemMetrics(SM_CYSMICON) + 2*g_cyBorder;

    // We can put the icons below or to the left of the clock, which
    // one gives a smaller total height?
    nRows = (nMatchVert-3*g_cyBorder)/yIcon;
    if (nRows < 1)
    {
        nRows = 1;
    }
    nCols = ROWSCOLS((int)nIcons, nRows);

    ppt->x = g_cxEdge + (nCols*xIcon) + 2*g_cxBorder;
    ppt->y = (nRows*yIcon) + 2*g_cyBorder;

    return(nCols);
}


/*
 ** _TNCalcRects
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *      This is kind of strange: the RECT for the clock is exact, while the
 *      RECT for the icons is exact on the left and top, but adds an extra
 *      border on the right and bottom.
 *
 *  RETURNS:
 *
 */

UINT _TNCalcRects(PTNPRIVDATA ptnd, int nMaxHorz, int nMaxVert,
    LPRECT prClock, LPRECT prNotifies)
{
    UINT nCols;
    LRESULT lRes;
    POINT ptBelow, ptLeft;
    int nHorzLeft, nVertLeft;
    int nHorzBelow, nVertBelow;
    int nColsBelow, nColsLeft;
    INT_PTR nIcons;

    // Check whether we should try to match the horizontal or vertical
    // size (the smaller is the one to match)
    enum { MATCH_HORZ, MATCH_VERT } eMatch =
        nMaxHorz <= nMaxVert ? MATCH_HORZ : MATCH_VERT;
    enum { SIDE_LEFT, SIDE_BELOW } eSide;

    lRes = SendMessage(ptnd->hwndClock, WM_CALCMINSIZE, 0, 0);
    prClock->left = prClock->top = 0;
    prClock->right = LOWORD(lRes);
    prClock->bottom = HIWORD(lRes);

    nIcons = _TNGetVisibleCount(ptnd);
    if (nIcons == 0)
    {
        SetRectEmpty(prNotifies);
        goto CalcDone;
    }

    if (eMatch == MATCH_HORZ)
    {
        nColsBelow = _TNMatchIconsHorz(nMaxHorz, nIcons, &ptBelow);
        // Add cxBorder because the clock goes right next to the icons
        nColsLeft = _TNMatchIconsHorz(nMaxHorz-prClock->right+g_cxBorder,
            nIcons, &ptLeft);
    }
    else
    {
        nColsBelow = _TNMatchIconsVert(nMaxVert-prClock->bottom,
            nIcons, &ptBelow);
        nColsLeft = _TNMatchIconsVert(nMaxVert, nIcons, &ptLeft);
    }

    nVertBelow = ptBelow.y + prClock->bottom;
    nHorzBelow = ptBelow.x;
    nVertLeft = ptLeft.y;
    nHorzLeft = ptLeft.x + prClock->right;

    eSide = SIDE_LEFT;
    if (eMatch == MATCH_HORZ)
    {
        // If there is no room on the left, or putting it below makes a
        // smaller rectangle
        if (nMaxHorz<nHorzLeft || nVertBelow<nVertLeft)
        {
            eSide = SIDE_BELOW;
        }
    }
    else
    {
        // If there is room below and putting it below makes a
        // smaller rectangle
        if (nMaxVert>nVertBelow && nHorzBelow<nHorzLeft)
        {
            eSide = SIDE_BELOW;
        }
    }

    prNotifies->left = 0;
    if (eSide == SIDE_LEFT)
    {
        prNotifies->right = ptLeft.x + g_cxBorder;
        prNotifies->top = 0;
        prNotifies->bottom = ptLeft.y;

        OffsetRect(prClock, prNotifies->right, 0);

        nCols = nColsLeft;
    }
    else
    {
        if (ptBelow.x<prClock->right && eMatch==MATCH_VERT)
        {
            // Let's recalc using the whole clock width
            nColsBelow = _TNMatchIconsHorz(prClock->right, nIcons, &ptBelow);
        }
        ptBelow.y += 2;
        prNotifies->right = ptBelow.x;
        prNotifies->top = prClock->bottom + g_cyBorder;
        prNotifies->bottom = prNotifies->top + ptBelow.y;

        if (prClock->right && (prClock->right < prNotifies->right))
        {
            // Use the larger value to center properly
            prClock->right = prNotifies->right;
        }

        nCols = nColsBelow;
    }

CalcDone:
    // At least as tall as a gripper
    if (prClock->bottom < g_cySize + g_cyEdge)
        prClock->bottom = g_cySize + g_cyEdge;

    // Never wider than the space we allotted
    if (prClock->right > nMaxHorz - 4 * g_cxBorder)
        prClock->right = nMaxHorz - 4 * g_cxBorder;

    // Add back the border around the whole window
    OffsetRect(prClock, g_cxBorder, g_cyBorder);

    return(nCols);
}


/*
 ** _TNCalcMinSize
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

LRESULT _TNCalcMinSize(PTNPRIVDATA ptnd, int nMaxHorz, int nMaxVert)
{
    RECT rTotal, rClock, rNotifies;

    if (!(GetWindowLong(ptnd->hwndClock, GWL_STYLE) & WS_VISIBLE) &&
        !_TNGetCount(ptnd)) {
        ShowWindow(ptnd->hwndNotify, SW_HIDE);
        return 0L;
    } else {
        if (!IsWindowVisible(ptnd->hwndNotify))
            ShowWindow(ptnd->hwndNotify, SW_SHOW);
    }

    _TNCalcRects(ptnd, nMaxHorz, nMaxVert, &rClock, &rNotifies);

    UnionRect(&rTotal, &rClock, &rNotifies);

    // this can happen if rClock's hidden width is 0;
    // make sure the rTotal height is at least the clock's height.
    // it can be smaller if the clock is hidden and thus has a 0 width
    if ((rTotal.bottom - rTotal.top) < (rClock.bottom - rClock.top))
        rTotal.bottom = rTotal.top + (rClock.bottom - rClock.top);

    // Add on room for borders
    return(MAKELRESULT(rTotal.right+g_cxBorder, rTotal.bottom+g_cyBorder));
}


/*
 ** _TNSize
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

LRESULT _TNSize(PTNPRIVDATA ptnd)
{
    RECT rTotal, rClock;
    RECT rNotifies;
    HWND hWnd = ptnd->hwndNotify;

    // use GetWindowRect because _TNCalcRects includes the borders
    GetWindowRect(hWnd, &rTotal);
    rTotal.right -= rTotal.left;
    rTotal.bottom -= rTotal.top;
    rTotal.left = rTotal.top = 0;

    // Account for borders on the left and right
    ptnd->nCols = _TNCalcRects(ptnd, rTotal.right,
        rTotal.bottom, &rClock, &rNotifies);


    SetWindowPos(ptnd->hwndClock, NULL, rClock.left, rClock.top,
            rClock.right-rClock.left, rClock.bottom-rClock.top, SWP_NOZORDER);

    SetWindowPos(ptnd->hwndToolbar, NULL, rNotifies.left + g_cxEdge, rNotifies.top,
            rNotifies.right-rNotifies.left-g_cxEdge, rNotifies.bottom-rNotifies.top, SWP_NOZORDER);
    return(0);
}


LRESULT _TNTimer(PTNPRIVDATA ptnd, UINT uTimerID)
{
    if (uTimerID == ptnd->uInfoTipTimer)
    {
        KillTimer(ptnd->hwndNotify, ptnd->uInfoTipTimer);
        ptnd->uInfoTipTimer = 0;
        if (ptnd->pinfo)
        {
            if (ptnd->pinfo->bMinShown 
            ||  (ptnd->hdpaInfo && DPA_GetPtrCount(ptnd->hdpaInfo) > 0)
            ||  ptnd->pinfo->uTimeout == MIN_INFO_TIME)
                _TNShowInfoTip(ptnd, ptnd->pinfo->nIcon, FALSE, FALSE); // hide this balloon and show new one
            else
            {
                ptnd->pinfo->bMinShown = TRUE;
                if (ptnd->pinfo->uTimeout > MIN_INFO_TIME)
                    ptnd->uInfoTipTimer = SetTimer(ptnd->hwndNotify, INFO_TIMER, 
                                                   ptnd->pinfo->uTimeout-MIN_INFO_TIME, NULL);
            }
        }
   }

    return 0;
}

// returns BOOL if the lParam specifies a pos over the clock
extern BOOL IsPosInHwnd(LPARAM lParam, HWND hwnd);

#define _IsOverClock(pTNPrivdata, lParam) IsPosInHwnd(lParam, pTNPrivdata->hwndClock)


/*
 ** _TNMouseEvent
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

LRESULT _TNMouseEvent(PTNPRIVDATA ptnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fClickDown)
{
    POINT pt;
    INT_PTR i;
    PTNPRIVICON ptnpi;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    i = SendMessage(ptnd->hwndToolbar, TB_HITTEST, 0, (LPARAM)&pt);
    ptnpi = _TNGetDataByIndex(ptnd, i);
    if (ptnpi) 
    {
        if (IsWindow(ptnpi->hWnd)) 
        {
            if (fClickDown) 
            {
                SHAllowSetForegroundWindow(ptnpi->hWnd);
                if (ptnd->pinfo && i == ptnd->pinfo->nIcon)
                    _TNShowInfoTip(ptnd, ptnd->pinfo->nIcon, FALSE, FALSE);
            }
            _TNSendNotify(ptnpi, uMsg);
        } 
        else 
        {
            _TNDeleteNotify(ptnd, i);
            Tray_SizeWindows();
        }
        return 1;
    }
    return(0);
}

LRESULT _TNOnCDNotify(PTNPRIVDATA ptnd, LPNMTBCUSTOMDRAW pnm)
{
    switch (pnm->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
        
    case CDDS_ITEMPREPAINT:
    {
        LRESULT lRet = TBCDRF_NOOFFSET;

        // notify us for the hot tracked item please
        if (pnm->nmcd.uItemState & CDIS_HOT)
            lRet |= CDRF_NOTIFYPOSTPAINT;

        // we want the buttons to look totally flat all the time
        pnm->nmcd.uItemState = 0;

        return  lRet;
    }

    case CDDS_ITEMPOSTPAINT:
    {
        // draw the hot tracked item as a focus rect, since
        // the tray notify area doesn't behave like a button:
        //   you can SINGLE click or DOUBLE click or RCLICK
        //   (kybd equiv: SPACE, ENTER, SHIFT F10)
        //
        LRESULT lRes = SendMessage(ptnd->hwndNotify, WM_QUERYUISTATE, 0, 0);
        if (!(LOWORD(lRes) & UISF_HIDEFOCUS) && ptnd->hwndToolbar == GetFocus())
        {
            DrawFocusRect(pnm->nmcd.hdc, &pnm->nmcd.rc);
        }
        break;
    }
    }

    return CDRF_DODEFAULT;
}

LRESULT _TNNotify(PTNPRIVDATA ptnd, LPNMHDR pNmhdr)
{
    switch (pNmhdr->code)
    {
    case NM_KEYDOWN:
        ptnd->fKey = TRUE;
        break;

    case TBN_ENDDRAG:
        ptnd->fKey = FALSE;
        break;

    case TBN_DELETINGBUTTON:
    {
        TBNOTIFY* ptbn = (TBNOTIFY*)pNmhdr;
        PTNPRIVICON ptnpi = (PTNPRIVICON)(LPVOID)ptbn->tbButton.dwData;
        _TNFreeNotify(ptnd, ptnpi, ptbn->tbButton.iBitmap);
        break;
    }

    case NM_CUSTOMDRAW:
        return _TNOnCDNotify(ptnd, (LPNMTBCUSTOMDRAW)pNmhdr);

    default:
        break;
    }

    return(0);
}

//---------------------------------------------------------------------------
void _TNSysChange(PTNPRIVDATA ptnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
    if (uMsg == WM_WININICHANGE)
        _TNCheckAndResizeImages(ptnd);

    if (ptnd && ptnd->hwndClock)
        SendMessage(ptnd->hwndClock, uMsg, wParam, lParam);
}

void _TNCommand(PTNPRIVDATA ptnd, UINT id, UINT uCmd)
{
    switch (uCmd) {
    case BN_CLICKED:
    {
        PTNPRIVICON ptnpi = _TNGetData(ptnd, id, FALSE);
        if (ptnpi) {
            if (ptnd->fKey)
                _TNSetCursorPos(ptnd, SendMessage(ptnd->hwndToolbar, TB_COMMANDTOINDEX, id, 0));

            SHAllowSetForegroundWindow(ptnpi->hWnd);
            if (ptnpi->uVersion >= KEYBOARD_VERSION) {
                // if they are a new version that understands the keyboard messages,
                // send the real message to them.
                _TNSendNotify(ptnpi, ptnd->fKey ? NIN_KEYSELECT : NIN_SELECT);
                // Hitting RETURN is like double-clicking (which in the new
                // style means keyselecting twice)
                if (ptnd->fKey && ptnd->fReturn)
                    _TNSendNotify(ptnpi, NIN_KEYSELECT);
            } else {
                // otherwise mock up a mouse event if it was a keyboard select
                // (if it wasn't a keyboard select, we assume they handled it already on
                // the WM_MOUSE message
                if (ptnd->fKey) {
                    _TNSendNotify(ptnpi, WM_LBUTTONDOWN);
                    _TNSendNotify(ptnpi, WM_LBUTTONUP);
                    if (ptnd->fReturn) {
                        _TNSendNotify(ptnpi, WM_LBUTTONDBLCLK);
                        _TNSendNotify(ptnpi, WM_LBUTTONUP);
                    }
                }
            }
        }
        break;
    }
    }
}

//---------------------------------------------------------------------------
LRESULT CALLBACK TrayNotifyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (WM_CREATE == uMsg)
    {
        return _TNCreate(hWnd);
    }
    else
    {
        PTNPRIVDATA ptnd = (PTNPRIVDATA)GetWindowLongPtr(hWnd, 0);

        if (ptnd)
        {
            switch (uMsg)
            {
            case WM_DESTROY:
                return _TNDestroy(ptnd);

            case WM_COMMAND:
                _TNCommand(ptnd, GET_WM_COMMAND_ID(wParam, lParam), GET_WM_COMMAND_CMD(wParam, lParam));
                break;

            case WM_SETFOCUS:
                SetFocus(ptnd->hwndToolbar);
                break;
                
            case WM_PAINT:
                return _TNPaint(ptnd);

            case WM_CALCMINSIZE:
                return _TNCalcMinSize(ptnd, (int)wParam, (int)lParam);

            case WM_TIMECHANGE:
            case WM_WININICHANGE:
            case WM_POWERBROADCAST:
            case WM_POWER:
                _TNSysChange(ptnd, uMsg, wParam, lParam);
                goto DoDefault;

            case WM_NCHITTEST:
                return(_IsOverClock(ptnd, lParam) ? HTTRANSPARENT : HTCLIENT);

            case WM_NOTIFY:
                return(_TNNotify(ptnd, (LPNMHDR)lParam));

            case TNM_GETCLOCK:
                return (LRESULT)ptnd->hwndClock;

            case TNM_TRAYHIDE:
                if (lParam && IsWindowVisible(ptnd->hwndClock))
                    SendMessage(ptnd->hwndClock, TCM_RESET, 0, 0);
                break;

            case TNM_HIDECLOCK:
                ShowWindow(ptnd->hwndClock, lParam ? SW_HIDE : SW_SHOW);
                break;

            case TNM_TRAYPOSCHANGED:
                if (ptnd->pinfo)
                    PostMessage(ptnd->hwndNotify, TNM_ASYNCINFOTIPPOS, (WPARAM)ptnd->pinfo->nIcon, 0);
                break;
            
            case TNM_ASYNCINFOTIPPOS:
                _TNPositionInfoTip(ptnd);
                break;

            case TNM_ASYNCINFOTIP:
                _TNShowInfoTip(ptnd, (INT_PTR)wParam, TRUE, FALSE);
                break;

            case WM_SIZE:
                _TNSize(ptnd);
                break;

            case WM_TIMER:
                _TNTimer(ptnd, (UINT)wParam);
                break;

            case TNM_RUDEAPP:
                // rude app is getting displayed, hide the balloon
                if (wParam && ptnd->pinfo)
                    _TNShowInfoTip(ptnd, ptnd->pinfo->nIcon, FALSE, FALSE);
                break;

            // only button down, mouse move msgs are forwarded down to us from info tip
            //case WM_LBUTTONUP:
            //case WM_MBUTTONUP:
            //case WM_RBUTTONUP:
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
                _TNInfoTipMouseClick(ptnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                break;

            default:
                goto DoDefault;
            }
        }
        else
        {
DoDefault:
            return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        }
    }

    return 0;
}




/*
 ** TrayNotify
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

BOOL TrayNotifyIcon(PTNPRIVDATA ptnd, PTRAYNOTIFYDATA pnid, BOOL *pbRefresh);

LRESULT TrayNotify(HWND hwndNotify, HWND hwndFrom, PCOPYDATASTRUCT pcds, BOOL *pbRefresh)
{
    PTNPRIVDATA ptnd;
    PTRAYNOTIFYDATA pnid;

    if (!hwndNotify || !pcds)
    {
        return(FALSE);
    }

    ptnd = (PTNPRIVDATA)GetWindowLongPtr(hwndNotify, 0);
    AssertMsg((ptnd != NULL), TEXT("Find the goobber that prematurely removed the PRIVDATA pointer."));
    if (ptnd)
    {
        if (pcds->cbData < SIZEOF(TRAYNOTIFYDATA))
        {
            return(FALSE);
        }

        // We'll add a signature just in case
        pnid = (PTRAYNOTIFYDATA)pcds->lpData;
        if (pnid->dwSignature != NI_SIGNATURE)
        {
            return(FALSE);
        }

        return TrayNotifyIcon(ptnd, pnid, pbRefresh);
    }

    return(TRUE);
}

BOOL TrayNotifyIcon(PTNPRIVDATA ptnd, PTRAYNOTIFYDATA pnid, BOOL *pbRefresh)
{
    PNOTIFYICONDATA32 pNID;
    INT_PTR nIcon;

    pNID = &pnid->nid;
    if (pNID->cbSize<SIZEOF(NOTIFYICONDATA32))
    {
        return(FALSE);
    }
    nIcon = _TNFindNotify(ptnd, pNID);
    
    switch (pnid->dwMessage)
    {
    case NIM_SETFOCUS:
        // the notify client is trying to return focus to us
        if (nIcon >= 0) {
            SetForegroundWindow(v_hwndTray);
            SetFocus(ptnd->hwndToolbar);
            SendMessage(ptnd->hwndToolbar, TB_SETHOTITEM, nIcon, 0);
            return TRUE;
        }
        return FALSE;
        
    case NIM_ADD:
        ptnd->iVisCount = -1;
        if (nIcon >= 0)
        {
            // already there
            return(FALSE);
        }

        if (!_TNInsertNotify(ptnd, pNID))
        {
            return(FALSE);
        }
        // if balloon is up we have to move it, but we cannot straight up
        // position it because traynotify will be moved around by tray
        // so we do it async
        if (ptnd->pinfo)
            PostMessage(ptnd->hwndNotify, TNM_ASYNCINFOTIPPOS, (WPARAM)ptnd->pinfo->nIcon, 0);
        break;

    case NIM_MODIFY:
        if (nIcon < 0)
        {
            return(FALSE);
        }

        if (!_TNModifyNotify(ptnd, pNID, nIcon, pbRefresh))
        {
            return(FALSE);
        }
        // see comment above
        if (ptnd->pinfo)
            PostMessage(ptnd->hwndNotify, TNM_ASYNCINFOTIPPOS, (WPARAM)ptnd->pinfo->nIcon, 0);
        break;

    case NIM_DELETE:
        if (nIcon < 0)
        {
            return(FALSE);
        }

        _TNDeleteNotify(ptnd, nIcon);
        _TNSize(ptnd);
        break;

    case NIM_SETVERSION:
        if (nIcon < 0)
        {
            return(FALSE);
        }

        return _TNSetVersionNotify(ptnd, pNID, nIcon);

    default:
        return(FALSE);
    }
    
    return(TRUE);
}

/*
 ** TrayNotifyCreate
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

HWND TrayNotifyCreate(HWND hwndParent, UINT uID, HINSTANCE hInst)
{
    WNDCLASSEX wc;
    DWORD dwExStyle = WS_EX_STATICEDGE;

    ZeroMemory(&wc, SIZEOF(wc));
    wc.cbSize = SIZEOF(WNDCLASSEX);

    if (!GetClassInfoEx(hInst, c_szTrayNotify, &wc))
    {
        wc.lpszClassName = c_szTrayNotify;
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = TrayNotifyWndProc;
        wc.hInstance = hInst;
        //wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
        //wc.lpszMenuName  = NULL;
        //wc.cbClsExtra = 0;
        wc.cbWndExtra = SIZEOF(PTNPRIVDATA);
        //wc.hIconSm = NULL;

        if (!RegisterClassEx(&wc))
        {
            return(NULL);
        }

        if (!ClockCtl_Class(hInst))
        {
            return(NULL);
        }
    }

    return(CreateWindowEx(dwExStyle, c_szTrayNotify,
            NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 0, 0,
            hwndParent, (HMENU)uID, hInst, NULL));
}

#if XXX_RENIM // {
//***   XXX_RENIM --
// DESCRIPTION
//  rather than make every client handle "TaskbarCreated" message, we keep
// track of clients here.  for now we've deemed this extra work 'not worth
// it', but we might change our minds.  here's some mostly-debugged code
// for if that day comes...
// NOTES
//  - need to persist bitmap for icon, not handle (since image list cache
// dies w/ shell).
//  - haven't tested DeleteNID (call & implementation).
//  - SHDeleteKey call is currently deactivated (for debug repro'ability
// and safety until fully debugged).
//  - probably a couple other details...

#define SETWITHMASK(dwOld, dwNew, dwNewMask) \
    (((dwOld) & ~(dwNewMask)) | ((dwNew) & (dwNewMask)))

//***   MergeNID -- merge changes into current state
// DESCRIPTION
//  we need to know what NID to use for a reinit.  to do that we need to
// start w/ the original 'add' and update it w/ any 'modify's.  alternately
// we could use the private struct and convert it back to a NID, but that
// seemed way harder.
void MergeNID(PNOTIFYICONDATA32 pNidCur, PNOTIFYICONDATA32 pNidMod)
{
    ASSERT(pNidMod->cbSize == pNidCur->cbSize);
    ASSERT(pNidMod->cbSize == pNidCur->cbSize);

    // BUGBUG should we check for NIM_ADD?
    if (pNidCur->hWnd == 0)
        pNidCur->hWnd = pNidMod->hWnd;
    if (pNidCur->uID == 0)
        pNidCur->uID = pNidMod->uID;
    pNidCur->uFlags |= pNidMod->uFlags;

    if (pNidMod->uFlags & NIF_MESSAGE)
        pNidCur->uCallbackMessage = pNidMod->uCallbackMessage;
    if (pNidMod->uFlags & NIF_ICON)
        pNidCur->hIcon = pNidMod->hIcon;
    if (pNidMod->uFlags & NIF_TIP)
        lstrcpyn(pNidCur->szTip, pNidMod->szTip, ARRAYSIZE(pNidCur->szTip));
    if (pNidMod->uFlags & NIF_STATE) {
        pNidCur->dwState = SETWITHMASK(pNidCur->dwState, pNidMod->dwState, pNidMod->dwStateMask);
    }
    if (pNidMod->uFlags & NIF_INFO) {
        lstrcpyn(pNidCur->szInfo, pNidMod->szInfo, ARRAYSIZE(pNidCur->szInfo));
        lstrcpyn(pNidCur->szInfoTitle, pNidMod->szInfoTitle, ARRAYSIZE(pNidCur->szInfoTitle));
    }

#define NIS_VALIDMASK (NIS_HIDDEN | NIS_SHAREDICON)
    if (1) {
        pNidCur->dwStateMask = SETWITHMASK(pNidCur->dwStateMask, pNidMod->dwStateMask, NIS_VALIDMASK);
    }

    return;
}

HKEY g_hkNid;

#define CCH_IDNID   (2 + 1)
#define REGSTR_KEY_NIDCACHE TEXT("NIDCache")

//***   GetKeyNID --
//
HKEY GetKeyNID(INT_PTR nIcon, TCHAR *pszIdNid)
{
    HKEY hk, hkSession;

    if (g_hkNid == 0) {
        hkSession = GetSessionKey(MAXIMUM_ALLOWED);
        if (hkSession)
        {
            if (NO_ERROR == RegCreateKeyEx(hkSession, REGSTR_KEY_NIDCACHE, 0, 0, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, NULL))
                g_hkNid = hk;
            RegCloseKey(hkSession);
        }
    }

    wnsprintf(pszIdNid, CCH_IDNID, TEXT("%d"), nIcon);

    return g_hkNid;
}

//***   DeleteKeyNID -- nuke the entire key/ cache
//
void DeleteKeyNID()
{
    if (g_hkNid) {
        RegCloseKey(g_hkNid);
        g_hkNid = 0;
    }
    if (0)  // BUGBUG off until fully debugged
    {
        HKEY hkSession = GetSessionKey(KEY_ALL_ACCESS);
        if (hkSession)
        {
            SHDeleteKey(hkSession, REGSTR_KEY_NIDCACHE); // recursive!
            RegCloseKey(hkSession);
        }
    }

    return;
}

//***   ReadNID, WriteNID, DeleteNID --
// ENTRY/EXIT
//  nIcon       #
//  pNidMod     guy to read (or write).  0'ed on read if not found.
BOOL ReadNID(INT_PTR nIcon, PNOTIFYICONDATA32 pNidMod)
{
    HKEY hk;
    DWORD dwSize, dwType;
    TCHAR szIdNid[CCH_IDNID];

    hk = GetKeyNID(nIcon, szIdNid);
    if (hk) {
        dwSize = SIZEOF(*pNidMod);
        if (ERROR_SUCCESS == RegQueryValueEx(hk, szIdNid, NULL, &dwType, (BYTE *)pNidMod, &dwSize) && dwType == REG_BINARY) {
            return TRUE;
        }
    }

    memset(pNidMod, 0, SIZEOF(*pNidMod));
    pNidMod->cbSize = SIZEOF(*pNidMod);

    return FALSE;
}

BOOL WriteNID(INT_PTR nIcon, PNOTIFYICONDATA32 pNidMod)
{
    HKEY hk;
    TCHAR szIdNid[CCH_IDNID];

    hk = GetKeyNID(nIcon, szIdNid);
    if (hk) {
        if (ERROR_SUCCESS == RegSetValueEx(hk, szIdNid, 0, REG_BINARY, (BYTE *)pNidMod, SIZEOF(*pNidMod))) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL DeleteNID(INT_PTR nIcon)
{
    HKEY hk;
    TCHAR szIdNid[CCH_IDNID];

    ASSERT(0);  // untested!
    hk = GetKeyNID(nIcon, szIdNid);
    if (hk) {
        if (ERROR_SUCCESS == RegDeleteValue(hk, szIdNid))
            return TRUE;
    }
    ASSERT(0);  // 'impossible'
    return FALSE;
}

//***   CacheNID -- update NID cache
//
void CacheNID(CACHEOP op, INT_PTR nIcon, PNOTIFYICONDATA32 pNidMod)
{
    NOTIFYICONDATA nidCur;

    switch (op) {
    case COP_ADD:
        ReadNID(nIcon, &nidCur);
        MergeNID(&nidCur, pNidMod);
        WriteNID(nIcon, &nidCur);
        break;
    case COP_DEL:
        DeleteNID(nIcon);
        break;
    }

    return;
}

//***   RestoreNID -- restore a single NID
// DESCRIPTION
//  we dummy up a request and resubmit it
void RestoreNID(PTNPRIVDATA ptnd, PNOTIFYICONDATA32 pnidCur)
{
    TRAYNOTIFYDATA tnd;

    tnd.dwSignature = NI_SIGNATURE;
    tnd.dwMessage = NIM_ADD;
    tnd.nid = *pnidCur;
    TrayNotifyIcon(ptnd, &tnd, NULL);
    return;
}

//***   RestoreNIDList -- restore all NIDs from cache, and nuke cache
//
int RestoreNIDCallback(void *pElt, void *pData)
{
    PNOTIFYICONDATA32 pnidCur = pElt;
    PTNPRIVDATA ptnd = pData;

    ASSERT(pnidCur->cbSize == SIZEOF(*pnidCur));

    RestoreNID(ptnd, pnidCur);

    return 1;   // keep going
}

void RestoreNIDList(PTNPRIVDATA ptnd)
{
    HKEY hk;
    HDSA hdsa;
    int i, j;
    NOTIFYICONDATA nidCur;
    TCHAR szIdNid[CCH_IDNID];

    hk = GetKeyNID(0, szIdNid);
    if (hk) {
        // generate list (gotta build up a list, then process, o.w.
        // RegEnum will get screwed up)
        hdsa = DSA_Create(SIZEOF(nidCur), 2);
        if (hdsa) {
            DWORD cchVal, dwType;

            for (i = 0;
              cchVal = ARRAYSIZE(szIdNid),
              RegEnumValue(hk, i, szIdNid, &cchVal, 0, &dwType, NULL, NULL) == ERROR_SUCCESS;
              i++) {
                if (dwType == REG_BINARY && EVAL(StrToIntEx(szIdNid, 0, &j))) {
                    if (ReadNID(j, &nidCur)) {
                        // BUGBUG raymondc - need to revalidate the HWND
                        // to make sure it hasn't been recycled
                        DSA_AppendItem(hdsa, &nidCur);
                    }
                }
            }
        }

        // BUGBUG raymondc - if we delete here, it means we don't recover the
        // icons when Explorer crashes a *second* time.
        // nuke cache
        DeleteKeyNID();     // hk no longer valid!

        // restore
        if (hdsa) {
            DSA_EnumCallback(hdsa, RestoreNIDCallback, (void *)ptnd);
            DSA_Destroy(hdsa);
        }
    }

    return;
}
#endif // }
