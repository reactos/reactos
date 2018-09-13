//---------------------------------------------------------------------------
// Helper routines for an owner draw menu showing the contents of a directory.
//---------------------------------------------------------------------------

// BUGBUG (scotth): note this file is #included by SHELL32 and SHDOC401.
//                  We really want the bits in one place, but right now
//                  SHDOC401 needs some changes which SHELL32 on win95
//                  does not provide.
//
//                  The second best solution is to place this code in
//                  a static lib (stocklib).  However, shell32's default
//                  data segment is still shared, and since this file
//                  contains some globals, we'd have problems with that.
//                  If shell32 is fixed, we can add this file to stocklib.
//
//                  Our third best solution is to #include this file.
//                  That's better than maintaining two different source
//                  codes.
//

#include <limits.h>

#ifdef IN_SHDOCVW
extern "C" LPITEMIDLIST IEILCreate(UINT cbSize);
#define _ILCreate   IEILCreate
#endif
STDAPI_(LPITEMIDLIST) SafeILClone(LPCITEMIDLIST pidl);
#define ILClone         SafeILClone

#define CXIMAGEGAP      6
// #define SRCSTENCIL           0x00B8074AL


typedef enum
{
    FMII_DEFAULT =      0x0000,
    FMII_BREAK =        0x0001
} FMIIFLAGS;


#define FMI_NULL            0x00000000
#define FMI_MARKER          0x00000001
#define FMI_FOLDER          0x00000002
#define FMI_EXPAND          0x00000004
#define FMI_EMPTY           0x00000008
#define FMI_SEPARATOR       0x00000010
#define FMI_DISABLED        0x00000020     // Enablingly Challenged ???
#define FMI_ON_MENU         0x00000040
#define FMI_IGNORE_PIDL     0x00000080     // Ignore the pidl as the display string
#define FMI_FILESYSTEM      0x00000100
#define FMI_MARGIN          0x00000200
#define FMI_MAXTIPWIDTH     0x00000400
#define FMI_TABSTOP         0x00000800
#define FMI_DRAWFLAGS       0x00001000
#define FMI_ALTITEM         0x00002000     // Item came from alternate pidl
#define FMI_CXMAX           0x00004000
#define FMI_CYMAX           0x00008000
#define FMI_CYSPACING       0x00010000
#define FMI_ASKEDFORTOOLTIP 0x00020000
#define FMI_USESTRING       0x00040000     // Use psz before pidl as display string


// One of these per file menu.
typedef struct
{
    IShellFolder *  psf;                // Shell Folder.
    IStream *       pstm;               // Optional stream
    HMENU           hmenu;              // Menu.
    LPITEMIDLIST    pidlFolder;         // Pidl for the folder.
    HDPA            hdpa;               // List of items (see below).
    UINT            idCmd;              // Command.
    DWORD           fmf;                // Header flags.
    UINT            fFSFilter;          // file system enum filter
    HBITMAP         hbmp;               // Background bitmap.
    UINT            cxBmp;              // Width of bitmap.
    UINT            cyBmp;              // Height of bitmap.
    UINT            cxBmpGap;           // Gap for bitmap.
    UINT            yBmp;               // Cached Y coord.
    COLORREF        clrBkg;             // Background color.
    UINT            cySel;              // Prefered height of selection.
    PFNFMCALLBACK   pfncb;              // Callback function.
    IShellFolder *  psfAlt;             // Alternate Shell Folder.
    LPITEMIDLIST    pidlAltFolder;      // Pidl for the alternate folder.
    HDPA            hdpaAlt;            // Alternate dpa
    int             cyMenuSizeSinceLastBreak;  // Size of menu (cy)
    UINT            cyMax;              // Max allowable height of entire menu in pixels
    UINT            cxMax;              // Max allowable width in pixels
    UINT            cySpacing;          // Spacing b/t menu items in pixels
    LPTSTR          pszFilterTypes;     // Multi-string list of extensions (e.g., "doc\0xls\0")
} FILEMENUHEADER, *PFILEMENUHEADER;


// One of these for each file menu item.
//
//  !!! Note: the testers have a test utility which grabs
//      the first 7 fields of this structure.  If you change
//      the order or meaning of these fields, make sure they
//      are notified so they can update their automated tests.
//
typedef struct
{
    PFILEMENUHEADER pfmh;               // The header.
    int             iImage;             // Image index to use.
    DWORD           Flags;              // Misc flags above.
    LPITEMIDLIST    pidl;               // IDlist for item.
    LPTSTR          psz;                // Text when not using pidls.
    UINT            cyItem;             // Custom height.
    LPTSTR          pszTooltip;         // Item tooltip.
    RECT            rcMargin;           // Margin around tooltip
    DWORD           dwMaxTipWidth;      // Maximum tooltip width
    DWORD           dwTabstop;
    UINT            uDrawFlags;
    LPARAM          lParam;             // Application data
    int             nOrder;             // Ordinal indicating user preference
    DWORD           dwEffect;           // Acceptable drop effects
} FILEMENUITEM, *PFILEMENUITEM;


#define X_TIPOFFSET         130      // an arbitrary number of pixels

class CFSMenuAgent
{
private:
    DWORD   _dwState;           // MAS_*
    HHOOK   _hhookMsg;

    PFILEMENUITEM _pfmiCur;     // Current item selected
    PFILEMENUITEM _pfmiDrag;    // Item being dragged
    PFILEMENUITEM _pfmiDrop;    // Target of drop

    DWORD   _dwStateSav;        // Saved state once menu goes away

    HWND    _hwndMenu;       
    HDC     _hdc;
    RECT    _rcCur;          // rect of current selection
    RECT    _rcCurScr;       // rect of current selection in screen coords
    RECT    _rcMenu;         // rect of whole menu in screen coords
    int     _yCenter;        // center of item (in screen coords)

    HCURSOR _hcurSav;
    HBRUSH  _hbr;

public:
    CFSMenuAgent(void);

    void    Init(void);
    void    Reset(void);
    void    EndMenu(void);

    void    UpdateInsertionCaret(void);

    void    SetEditMode(BOOL bEdit, DWORD dwEffect);
    void    SetCaretPos(LPPOINT ppt);
    HCURSOR SetCursor(DWORD dwEffect);
    void    SetCurrentRect(HDC hdc, LPRECT prcItem);

    void    SetItem(PFILEMENUITEM pfmi) { _pfmiCur = pfmi; }
    void    SetDragItem(void) { _pfmiDrag = _pfmiCur; }
    void    SetDropItem(void);

    DWORD   GetDragEffect(void);

    BOOL    ProcessCommand(HWND hwnd, HMENU hmenuBar, UINT idMenu, HMENU hmenu, UINT idCmd);

    friend  LRESULT CALLBACK CFSMenuAgent_MsgHook(int nCode, WPARAM wParam, LPARAM lParam);
    friend  LRESULT FileMenu_DrawItem(HWND hwnd, DRAWITEMSTRUCT *pdi);
};


// Menu agent state
#define MAS_EDITMODE            0x00000001
#define MAS_LBUTTONDOWN         0x00000002
#define MAS_LBUTTONUP           0x00000004
#define MAS_INSERTABOVE         0x00000008

// Edit mode states

#define MenuDD_IsButtonDown()   (g_fsmenuagent._dwState & MAS_LBUTTONDOWN)
#define MenuDD_InEditMode()     (g_fsmenuagent._dwState & MAS_EDITMODE)
#define MenuDD_InsertAbove()    (g_fsmenuagent._dwState & MAS_INSERTABOVE)
#define MenuDD_GetBrush()       (g_fsmenuagent._hbr)


//---------------------------------------------------------------------------

CFSMenuAgent g_fsmenuagent;

PFILEMENUITEM g_pfmiLastSel = NULL;
PFILEMENUITEM g_pfmiLastSelNonFolder = NULL;
// This saves us creating DC's all the time for the blits.
HDC g_hdcMem = NULL;
HFONT g_hfont = NULL;
BOOL g_fAbortInitMenu = FALSE;
// Tooltip stuff.
HWND g_hwndTip = NULL;
RECT g_rcItem = {0, 0, 0, 0};
HIMAGELIST g_himlIconsSmall = NULL;
HIMAGELIST g_himlIcons = NULL;


//---------------------------------------------------------------------------
// Validation functions


// IEUNIX -- these functions don't appear to be defined anywhere else, as the
// other def'n would imply
#if defined(DEBUG) || defined(UNIX)

BOOL IsValidPFILEMENUHEADER(PFILEMENUHEADER pfmh)
{
    return (IS_VALID_WRITE_PTR(pfmh, FILEMENUHEADER) &&
            ((NULL == pfmh->psf && 
              NULL == pfmh->pidlFolder) || 
             (IS_VALID_CODE_PTR(pfmh->psf, IShellFolder) && 
              IS_VALID_PIDL(pfmh->pidlFolder))) &&
            IS_VALID_HANDLE(pfmh->hmenu, MENU) &&
            IS_VALID_HANDLE(pfmh->hdpa, DPA) &&
            ((NULL == pfmh->psfAlt && 
              NULL == pfmh->pidlAltFolder) || 
             (IS_VALID_CODE_PTR(pfmh->psfAlt, IShellFolder) &&
              IS_VALID_PIDL(pfmh->pidlAltFolder))) &&
            (NULL == pfmh->hdpaAlt || IS_VALID_HANDLE(pfmh->hdpaAlt, DPA)) &&
            (NULL == pfmh->pszFilterTypes || IS_VALID_STRING_PTR(pfmh->pszFilterTypes, -1)));
}    

BOOL IsValidPFILEMENUITEM(PFILEMENUITEM pfmi)
{
    return (IS_VALID_WRITE_PTR(pfmi, FILEMENUITEM) &&
            IS_VALID_STRUCT_PTR(pfmi->pfmh, FILEMENUHEADER) &&
            (NULL == pfmi->pidl || IS_VALID_PIDL(pfmi->pidl)) &&
            (NULL == pfmi->psz || IS_VALID_STRING_PTR(pfmi->psz, -1)) &&
            (NULL == pfmi->pszTooltip || IS_VALID_STRING_PTR(pfmi->pszTooltip, -1)));
}    
#else

BOOL IsValidPFILEMENUHEADER(PFILEMENUHEADER pfmh);
BOOL IsValidPFILEMENUITEM(PFILEMENUITEM pfmi);

#endif


//---------------------------------------------------------------------------
// Helper functions


void FileList_Reorder(PFILEMENUHEADER pfmh);
BOOL FileMenuHeader_InsertItem(PFILEMENUHEADER pfmh, UINT iItem, FMIIFLAGS fFlags);
BOOL FileMenuItem_Destroy(PFILEMENUITEM pfmi);
BOOL FileMenuItem_Move(HWND hwnd, PFILEMENUITEM pfmiFrom, PFILEMENUHEADER pfmhTo, int iPosTo);
BOOL Tooltip_Create(HWND *phwndTip);


__inline static BOOL LAlloc(UINT cb, PVOID *ppv)
{
    ASSERT(ppv);

    *ppv = (PVOID*)LocalAlloc(LPTR, cb);
    return *ppv ? TRUE : FALSE;
}

__inline static BOOL LFree(PVOID pv)
{
    return LocalFree(pv) ? FALSE : TRUE;
}


/*----------------------------------------------------------
Purpose: Allocate a multi-string (double-null terminated)

Returns:
Cond:    --
*/
BOOL
MultiSz_AllocCopy(
    IN  LPCTSTR  pszSrc,
    OUT LPTSTR * ppszDst)
{
    BOOL fRet = FALSE;
    UINT cch;
    UINT cchMac = 0;
    LPCTSTR psz;
    LPTSTR pszDst;

    ASSERT(pszSrc && ppszDst);

    psz = pszSrc;
    while (*psz)
    {
        cch = lstrlen(psz) + 1;
        cchMac += cch;
        psz += cch;
    }
    cchMac++;       // extra null

    if (LAlloc(CbFromCch(cchMac), (PVOID *)ppszDst))
    {
        psz = pszSrc;
        pszDst = *ppszDst;
        while (*psz)
        {
            lstrcpy(pszDst, psz);
            cch = lstrlen(psz) + 1;
            psz += cch;
            pszDst += cch;
        }
        fRet = TRUE;
    }

    return fRet;
}


/*----------------------------------------------------------
Purpose: Allocate a string

Returns:
Cond:    --
*/
BOOL
Sz_AllocCopyA(
    IN  LPCSTR  pszSrc,
    OUT LPTSTR *ppszDst)
{
    BOOL fRet = FALSE;
    UINT cch;

    ASSERT(pszSrc && ppszDst);

    // NB We allocate an extra char in case we need to add an '&'.
    cch = lstrlenA(pszSrc) + 2;
    if (LAlloc(CbFromCchA(cch), (PVOID *)ppszDst))
    {
#ifdef UNICODE
        MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, *ppszDst, cch);
#else
        lstrcpy(*ppszDst, pszSrc);
#endif
        fRet = TRUE;
    }

    return fRet;
}


/*----------------------------------------------------------
Purpose: Allocate a string

Returns:
Cond:    --
*/
BOOL
Sz_AllocCopyW(
    IN  LPCWSTR pszSrc,
    OUT LPTSTR *ppszDst)
{
    BOOL fRet = FALSE;
    UINT cch;

    ASSERT(pszSrc && ppszDst);

    // NB We allocate an extra char in case we need to add an '&'.
    cch = lstrlenW(pszSrc) + 2;
    if (LAlloc(CbFromCchW(cch), (PVOID *)ppszDst))
    {
#ifdef UNICODE
        lstrcpy(*ppszDst, pszSrc);
#else
        WideCharToMultiByte(CP_ACP, 0, pszSrc, -1, *ppszDst, CbFromCchW(cch), NULL, NULL);
#endif
        fRet = TRUE;
    }

    return fRet;
}

#ifdef UNICODE
#define Sz_AllocCopy    Sz_AllocCopyW
#else
#define Sz_AllocCopy    Sz_AllocCopyA
#endif


HCURSOR LoadMenuCursor(UINT idCur)
{
#ifdef IN_SHDOCVW
    return LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(idCur));
#else
    HINSTANCE hinst = GetModuleHandle(TEXT("shdocvw.dll"));

    if (hinst)
        return LoadCursor(hinst, MAKEINTRESOURCE(idCur));

    return LoadCursor(NULL, IDC_ARROW);
#endif
}    


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


#ifdef DEBUG
static void DumpMsg(MSG * pmsg)
{
    ASSERT(pmsg);

    switch (pmsg->message)
    {
    case WM_LBUTTONDOWN:
        TraceMsg(TF_ALWAYS, "MsgHook: msg = WM_LBUTTONDOWN hwnd = %#08lx  x = %d  y = %d",
                 pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        TraceMsg(TF_ALWAYS, "                              keys = %#04lx  x = %d  y = %d",
                 pmsg->wParam, LOWORD(pmsg->lParam), HIWORD(pmsg->lParam));
        break;

    case WM_LBUTTONUP:
        TraceMsg(TF_ALWAYS, "MsgHook: msg = WM_LBUTTONUP   hwnd = %#08lx  x = %d  y = %d",
                 pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        TraceMsg(TF_ALWAYS, "                              keys = %#04lx  x = %d  y = %d",
                 pmsg->wParam, LOWORD(pmsg->lParam), HIWORD(pmsg->lParam));
        break;

    case WM_MOUSEMOVE:
        break;

    case WM_TIMER:
        TraceMsg(TF_ALWAYS, "MsgHook: msg = WM_TIMER       hwnd = %#08lx  x = %d  y = %d",
                 pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        TraceMsg(TF_ALWAYS, "                              id = %#08lx",
                 pmsg->wParam);
        break;

    case WM_MENUSELECT:
        TraceMsg(TF_ALWAYS, "MsgHook: msg = WM_MENUSELECT  hwnd = %#08lx  x = %d  y = %d",
                 pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        TraceMsg(TF_ALWAYS, "                              uItem = %#04lx  flags = %#04lx  hmenu = %#08lx",
                 GET_WM_MENUSELECT_CMD(pmsg->wParam, pmsg->lParam),
                 GET_WM_MENUSELECT_FLAGS(pmsg->wParam, pmsg->lParam),
                 GET_WM_MENUSELECT_HMENU(pmsg->wParam, pmsg->lParam));
        break;

    default:
        TraceMsg(TF_ALWAYS, "MsgHook: msg = %#04lx        hwnd = %#04lx  x = %d  y = %d",
                 pmsg->message, pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        break;
    }
}    
#endif


/*----------------------------------------------------------
Purpose: Message hook used to track drag and drop within the menu.

*/
LRESULT CALLBACK CFSMenuAgent_MsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0;
    MSG * pmsg = (MSG *)lParam;

    switch (nCode)
    {
    case MSGF_MENU:
#ifdef DEBUG
        if (IsFlagSet(g_dwDumpFlags, DF_HOOK))
            DumpMsg(pmsg);
#endif

        switch (pmsg->message)
        {
        case WM_LBUTTONUP:
            // We record the mouse up IFF it happened in the menu
            // and we had previously recorded the mouse down.
            if (IsFlagSet(g_fsmenuagent._dwState, MAS_EDITMODE | MAS_LBUTTONDOWN))
            {
                POINT pt;
                
                TraceMsg(TF_MENU, "MenuDD: getting mouse up");
                
                pt.x = LOWORD(pmsg->lParam);
                pt.y = HIWORD(pmsg->lParam);

                if (PtInRect(&g_fsmenuagent._rcMenu, pt))
                {
                    SetFlag(g_fsmenuagent._dwState, MAS_LBUTTONUP);
                    g_fsmenuagent.EndMenu();
                }
            }
            ClearFlag(g_fsmenuagent._dwState, MAS_LBUTTONDOWN);
            break;

        case WM_LBUTTONDOWN:
            if (g_fsmenuagent._pfmiCur && 
                (g_fsmenuagent._pfmiCur->dwEffect & DROPEFFECT_MOVE))
            {
                TraceMsg(TF_MENU, "MenuDD: getting mouse down");
                
                SetFlag(g_fsmenuagent._dwState, MAS_LBUTTONDOWN);
                g_fsmenuagent.SetDragItem();
            }
            break;

        case WM_MOUSEMOVE:
            if (g_fsmenuagent._dwState & MAS_EDITMODE)
            {
                POINT pt;
                BOOL bInMenu;

                pt.x = LOWORD(pmsg->lParam);
                pt.y = HIWORD(pmsg->lParam);

                g_fsmenuagent.SetCaretPos(&pt);

                bInMenu = PtInRect(&g_fsmenuagent._rcMenu, pt);
#if 0
                TraceMsg(TF_MENU, "MenuDD: %s (%d,%d) in [%d,%d,%d,%d]", 
                         bInMenu ? TEXT("in menu") : TEXT("not in menu"),
                         pt.x, pt.y, g_fsmenuagent._rcMenu.left, g_fsmenuagent._rcMenu.top,
                         g_fsmenuagent._rcMenu.right, g_fsmenuagent._rcMenu.bottom);
#endif

                // Determine which cursor to show
                if ( !bInMenu )
                    g_fsmenuagent.SetItem(NULL);

                g_fsmenuagent.SetCursor(g_fsmenuagent.GetDragEffect());
            }
            break;

        case WM_MENUSELECT:
            BLOCK
            {
                UINT uItem = GET_WM_MENUSELECT_CMD(pmsg->wParam, pmsg->lParam);
                HMENU hmenu = GET_WM_MENUSELECT_HMENU(pmsg->wParam, pmsg->lParam);

                // Is the menu going away?
                if (0 == uItem && NULL == hmenu)
                {
                    // Yes; release menu drag/drop
                    TraceMsg(TF_MENU, "MenuDD: menu being cancelled");

                    // Since we're in the middle of the hook chain, call
                    // the next hook first, then remove ourselves
                    lRet = CallNextHookEx(g_fsmenuagent._hhookMsg, nCode, wParam, lParam);

                    // Was an item dropped?
                    if (g_fsmenuagent._dwState & MAS_LBUTTONUP)
                    {
                        // Yes; remember it
                        g_fsmenuagent.SetDropItem();
                    }
                    g_fsmenuagent.Reset();
                    return lRet;
                }
            }
            break;
        }
        break;

    default:
        if (0 > nCode)
            return CallNextHookEx(g_fsmenuagent._hhookMsg, nCode, wParam, lParam);
        break;
    }

    // Pass it on to the next hook in the chain
    if (0 == lRet)
        lRet = CallNextHookEx(g_fsmenuagent._hhookMsg, nCode, wParam, lParam);

    return lRet;
}    


CFSMenuAgent::CFSMenuAgent(void)
{
    // This object is global, and not allocated.  We must explicitly
    // initialize the variables.

    _dwState = 0;
    _hhookMsg = 0;
    _dwStateSav = 0;

    _pfmiCur = NULL;
    _pfmiDrag = NULL;
    _pfmiDrop = NULL;

    _hwndMenu = NULL;
    _hdc = NULL;
    _hcurSav = NULL;
    _hbr = NULL;
}    


/*----------------------------------------------------------
Purpose: Initialize the menu drag/drop structure.  This must
         anticipate being called when it is already initialized.
         Also, this will get called whenever a cascaded menu 
         is opened.  Be sure to maintain state across these
         junctures.

*/
void CFSMenuAgent::Init(void)
{
    TraceMsg(TF_MENU, "Initialize menu drag/drop");

    // Do not init _pfmiDrag, since this function is called for every
    // cascaded menu, and _pfmiDrag must be remembered across these
    // menus.

    _pfmiDrop = NULL;
    _dwStateSav = 0;

    if (NULL == _hhookMsg)
    {
        _hhookMsg = SetWindowsHookEx(WH_MSGFILTER, CFSMenuAgent_MsgHook, HINST_THISDLL, 0);
    }

    if (NULL == _hbr)
    {
        // Don't need to release this
        _hbr = GetSysColorBrush(COLOR_3DFACE);
    }
}    


/*----------------------------------------------------------
Purpose: Make the menu go away

*/
void CFSMenuAgent::EndMenu(void)
{
    ASSERT(IsWindow(_hwndMenu));

    SendMessage(_hwndMenu, WM_CANCELMODE, 0, 0);
}    


/*----------------------------------------------------------
Purpose: Decides whether to position the caret above or below the
         menu item, based upon the given point (cursor position).

*/
void CFSMenuAgent::SetCaretPos(LPPOINT ppt)
{
    ASSERT(ppt);

    if (ppt->y < _yCenter)
    {
        // Change the caret position?
        if (IsFlagClear(_dwState, MAS_INSERTABOVE))
        {
            // Yes
            SetFlag(_dwState, MAS_INSERTABOVE);
            UpdateInsertionCaret();
        }
    }
    else
    {
        // Change the caret position?
        if (IsFlagSet(_dwState, MAS_INSERTABOVE))
        {
            // Yes
            ClearFlag(_dwState, MAS_INSERTABOVE);
            UpdateInsertionCaret();
        }
    }
}    


void CFSMenuAgent::UpdateInsertionCaret(void)
{
    if (_dwState & MAS_EDITMODE)    
    {
        InvalidateRect(_hwndMenu, &_rcCur, FALSE);
        UpdateWindow(_hwndMenu);
    }
}    


void CFSMenuAgent::SetDropItem(void) 
{ 
    // Only set the drop item if the drop effect is supported.
    ASSERT(_pfmiDrag);

    if (_pfmiCur && (_pfmiCur->dwEffect & _pfmiDrag->dwEffect))
        _pfmiDrop = _pfmiCur; 
    else
        _pfmiDrop = NULL;
}


/*----------------------------------------------------------
Purpose: Set the cursor based on the given flags

*/
HCURSOR CFSMenuAgent::SetCursor(DWORD dwEffect)
{
    HCURSOR hcur = NULL;

    ASSERT(_dwState & MAS_EDITMODE);

    // Does this item support the requested drop effect?
    if (_pfmiCur && (dwEffect & _pfmiCur->dwEffect))
    {
        // Yes
        UINT idCur;

        if (dwEffect & DROPEFFECT_MOVE)
            idCur = IDC_MENUMOVE;
        else if (dwEffect & DROPEFFECT_COPY)
            idCur = IDC_MENUCOPY;
        else
        {
            ASSERT_MSG(0, "Unknown drop effect!");
            idCur = IDC_MENUDENY;
        }

        hcur = ::SetCursor(LoadMenuCursor(idCur));
    }
    else
    {
        // No
        hcur = ::SetCursor(LoadMenuCursor(IDC_MENUDENY));
    }

    return hcur;
}    


DWORD CFSMenuAgent::GetDragEffect(void)
{
    if (_pfmiDrag)
        return _pfmiDrag->dwEffect;
    else
        return DROPEFFECT_NONE;
}    


void CFSMenuAgent::SetEditMode(BOOL bEdit, DWORD dwEffect)
{
    // Only update if the state has changed
    if (bEdit && IsFlagClear(_dwState, MAS_EDITMODE))
    {
        TraceMsg(TF_MENU, "MenuDD: entering edit mode");

        SetFlag(_dwState, MAS_EDITMODE);

        _hcurSav = SetCursor(dwEffect);
    }
    else if (!bEdit && IsFlagSet(_dwState, MAS_EDITMODE))
    {
        TraceMsg(TF_MENU, "MenuDD: leaving edit mode");

        ClearFlag(_dwState, MAS_EDITMODE);

        ASSERT(_hcurSav);

        if (_hcurSav)
        {
            ::SetCursor(_hcurSav);
            _hcurSav = NULL;
        }
    }
}    


void CFSMenuAgent::SetCurrentRect(HDC hdc, LPRECT prcItem)
{
    HWND hwnd = WindowFromDC(hdc);

    ASSERT(hdc);
    ASSERT(prcItem);

    _hwndMenu = hwnd;
    _hdc = hdc;
    _rcCur = *prcItem;
    _rcCurScr = *prcItem;
    GetWindowRect(hwnd, &_rcMenu);

    MapWindowPoints(hwnd, NULL, (LPPOINT)&_rcCurScr, 2);

    _yCenter = _rcCurScr.top + (_rcCurScr.bottom - _rcCurScr.top) / 2;
}


/*----------------------------------------------------------
Purpose: Reset the menu agent.  This is called when the menu goes
         away.  The ProcessCommand method still needs some state
         information so it knows what action had taken place.
         This info is moved to a post-action field.

*/
void CFSMenuAgent::Reset(void)
{
    TraceMsg(TF_MENU, "MenuDD: releasing edit mode resources");

    // Remember the state for FileMenu_ProcessCommand
    _dwStateSav = _dwState;

    SetEditMode(FALSE, DROPEFFECT_NONE);

    TraceMsg(TF_MENU, "MenuDD: Hook removed for menu drag/drop");

    if (_hhookMsg)
    {
        UnhookWindowsHookEx(_hhookMsg);
        _hhookMsg = NULL;
    }

    // Reset
    _pfmiCur = NULL;
    _hwndMenu = NULL;
    _hdc = NULL;
    _dwState = 0;
    _hbr = NULL;

    ASSERT(NULL == _hcurSav);
}    


/*----------------------------------------------------------
Purpose: Have a whack at the WM_COMMAND, in case it is the result
         of drag and drop within the menu.

Returns: TRUE if this function handled the command
*/
BOOL CFSMenuAgent::ProcessCommand(HWND hwnd, HMENU hmenuBar, UINT idMenu, HMENU hmenu, UINT idCmd)
{
    BOOL bRet = FALSE;

    if (hmenu && _pfmiDrag && (_dwStateSav & MAS_EDITMODE))
    {
        ASSERT(IS_VALID_STRUCT_PTR(_pfmiDrag, FILEMENUITEM));

        // Did the user move an item within the menu?
        if (_pfmiDrop)
        {
            // Yes
            ASSERT(IS_VALID_STRUCT_PTR(_pfmiDrop, FILEMENUITEM));

            int iPosTo = DPA_GetPtrIndex(_pfmiDrop->pfmh->hdpa, _pfmiDrop);

            if (IsFlagClear(_dwStateSav, MAS_INSERTABOVE))
                iPosTo++;

            IEPlaySound(TEXT("MoveMenuItem"), FALSE);

            bRet = FileMenuItem_Move(hwnd, _pfmiDrag, _pfmiDrop->pfmh, iPosTo);

            // Re-order the items
            FileList_Reorder(_pfmiDrop->pfmh);
        }

        _pfmiDrag = NULL;
        _pfmiDrop = NULL;

#if 0
        // Did we successfully handle this?
        if (bRet)
        {
            // Yes; bring the menu back up so the user can continue
            // editting.
            HiliteMenuItem(hwnd, hmenuBar, idMenu, MF_BYCOMMAND | MF_HILITE);
            DrawMenuBar(hwnd);
            
            TrackPopupMenu(hmenu, TPM_LEFTALIGN, _rcMenu.left,
                           _rcMenu.top, 0, hwnd, NULL);

            HiliteMenuItem(hwnd, hmenuBar, idMenu, MF_BYCOMMAND | MF_UNHILITE);
            DrawMenuBar(hwnd);
        }
#endif
        
        // Always return true because we handled it
        bRet = TRUE;        
    }

    return bRet;
}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
void DeleteGlobalMemDCAndFont(void)
{
    if (g_hdcMem)
    {
        DeleteDC(g_hdcMem);
        g_hdcMem = NULL;
    }
    if (g_hfont)
    {
        DeleteObject(g_hfont);
        g_hfont = NULL;
    }
}




DWORD
GetItemTextExtent(
    IN HDC     hdc,
    IN LPCTSTR lpsz)
{
    SIZE sz;

    GetTextExtentPoint(hdc, lpsz, lstrlen(lpsz), &sz);
    // NB This is OK as long as an item's extend doesn't get very big.
    return MAKELONG((WORD)sz.cx, (WORD)sz.cy);
}


/*----------------------------------------------------------
Purpose: Validates pfmitem.  This also initializes pfmitemOut 
         given the mask and flags set in pfmitem.  This helper 
         function is useful for APIs to "cleanse" incoming 
         FMITEM structures.

Returns: TRUE if pfmitem is a valid structure
Cond:    --
*/
BOOL
IsValidFMItem(
    IN  FMITEM const * pfmitem,
    OUT PFMITEM        pfmitemOut)
{
    BOOL bRet = FALSE;

    ASSERT(pfmitem);
    ASSERT(pfmitemOut);

    if (IS_VALID_READ_PTR(pfmitem, FMITEM) &&
        SIZEOF(*pfmitem) == pfmitem->cbSize)
    {   
        ZeroInit(pfmitemOut, SIZEOF(*pfmitemOut));

        pfmitemOut->cbSize = SIZEOF(*pfmitemOut);
        pfmitemOut->dwMask = pfmitem->dwMask;

        if (pfmitemOut->dwMask & FMI_TYPE)
            pfmitemOut->dwType = pfmitem->dwType;

        if (pfmitemOut->dwMask & FMI_ID)
            pfmitemOut->uID = pfmitem->uID;

        if (pfmitemOut->dwMask & FMI_ITEM)
            pfmitemOut->uItem = pfmitem->uItem;

        if (pfmitemOut->dwMask & FMI_IMAGE)
            pfmitemOut->iImage = pfmitem->iImage;
        else
            pfmitemOut->iImage = -1;

        if (pfmitemOut->dwMask & FMI_DATA)
            pfmitemOut->pvData = pfmitem->pvData;

        if (pfmitemOut->dwMask & FMI_HMENU)
            pfmitemOut->hmenuSub = pfmitem->hmenuSub;

        if (pfmitemOut->dwMask & FMI_METRICS)
            pfmitemOut->cyItem = pfmitem->cyItem;

        if (pfmitemOut->dwMask & FMI_LPARAM)
            pfmitemOut->lParam = pfmitem->lParam;

        // The FMIT_STRING and FMIT_SEPARATOR are exclusive
        if (IsFlagSet(pfmitemOut->dwType, FMIT_STRING) &&
            IsFlagSet(pfmitemOut->dwType, FMIT_SEPARATOR))
        {
            bRet = FALSE;
        }
        else
            bRet = TRUE;
    }
    return bRet;
}


void
FileMenuItem_GetDisplayName(
    IN PFILEMENUITEM pfmi,
    IN LPTSTR        pszName,
    IN UINT          cchName)
{
    STRRET str;

    ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));
    ASSERT(IS_VALID_WRITE_BUFFER(pszName, TCHAR, cchName));

    // Is this a special empty item?
    if (pfmi->Flags & FMI_EMPTY)
    {
        // Yep, load the string from a resource.
        LoadString(HINST_THISDLL, IDS_NONE, pszName, cchName);
    }
    else
    {
        // Nope, ask the folder for the name of the item.
        PFILEMENUHEADER pfmh = pfmi->pfmh;
        LPSHELLFOLDER psfTemp;

        ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

        if (pfmi->Flags & FMI_ALTITEM) {
            psfTemp = pfmh->psfAlt;
        } else {
            psfTemp = pfmh->psf;
        }

        // If it's got a pidl use that, else just use the normal menu string.
        if (psfTemp && pfmi->pidl && 
            IsFlagClear(pfmi->Flags, FMI_IGNORE_PIDL))
        {
            if (SUCCEEDED(psfTemp->GetDisplayNameOf(pfmi->pidl, SHGDN_NORMAL, &str)))
            {
                StrRetToStrN(pszName, cchName, &str, pfmi->pidl);
            }
        }
        else if (pfmi->psz)
        {
            lstrcpyn(pszName, pfmi->psz, cchName);
        }
        else
        {
            *pszName = TEXT('\0');
        }
    }
}

#define FileMenuHeader_AllowAbort(pfmh) (!(pfmh->fmf & FMF_NOABORT))


/*----------------------------------------------------------
Purpose: Create a menu item structure to be stored in the hdpa

Returns: TRUE on success
Cond:    --
*/
BOOL
FileMenuItem_Create(
    IN  PFILEMENUHEADER pfmh,
    IN  LPCITEMIDLIST   pidl,       OPTIONAL
    IN  int             iImage,
    IN  DWORD           dwFlags,    // FMI_*
    OUT PFILEMENUITEM * ppfmi)
{
    PFILEMENUITEM pfmi = (PFILEMENUITEM)LocalAlloc(LPTR, SIZEOF(FILEMENUITEM));

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    ASSERT(ppfmi);
    ASSERT(NULL == pidl || IS_VALID_PIDL(pidl));

    if (pfmi)
    {
        DWORD dwAttribs = SFGAO_FOLDER | SFGAO_FILESYSTEM;
        IShellFolder * psfTemp;
        BOOL bUseAlt = IsFlagSet(dwFlags, FMI_ALTITEM);

        pfmi->pfmh = pfmh;
        pfmi->pidl = (LPITEMIDLIST)pidl;
        pfmi->iImage = iImage;
        pfmi->Flags = dwFlags;
        pfmi->nOrder = INT_MAX;     // New items go to the bottom

        if (bUseAlt)
            psfTemp = pfmh->psfAlt;
        else 
            psfTemp = pfmh->psf;

        if (pidl &&
            SUCCEEDED(psfTemp->GetAttributesOf(1, &pidl, &dwAttribs)))
        {
            if (dwAttribs & SFGAO_FOLDER)
                pfmi->Flags |= FMI_FOLDER;

            if (dwAttribs & SFGAO_FILESYSTEM)
                pfmi->Flags |= FMI_FILESYSTEM;
        }
    }

    *ppfmi = pfmi;

    return (NULL != pfmi);
}


/*----------------------------------------------------------
Purpose: Move an item within the same menu or across menus

*/
BOOL FileMenuItem_Move(
    HWND            hwnd,
    PFILEMENUITEM   pfmiFrom, 
    PFILEMENUHEADER pfmhTo, 
    int             iPosTo)
{
    BOOL bRet = FALSE;
    TCHAR szFrom[MAX_PATH + 1];     // +1 for double null

    ASSERT(IS_VALID_STRUCT_PTR(pfmiFrom, FILEMENUITEM));
    ASSERT(IS_VALID_STRUCT_PTR(pfmhTo, FILEMENUHEADER));

    PFILEMENUHEADER pfmhFrom = pfmiFrom->pfmh;
    HDPA hdpaFrom = pfmhFrom->hdpa;
    HDPA hdpaTo = pfmhTo->hdpa;
    BOOL bSameMenu = (pfmhFrom == pfmhTo);

    ASSERT(IsFlagSet(pfmhFrom->fmf, FMF_CANORDER));

    // Is this item being moved within the same menu?
    if (bSameMenu)
    {
        // Yes; simply change the order of the menu below
        bRet = TRUE;
    }
    else
    {
        // No; need to move the actual file to the menu's associated
        // folder.  Also note the placement of the item in the menu.
        TCHAR szTo[MAX_PATH + 1];       // +1 for double null
        IShellFolder * psf = pfmhFrom->psf;
        STRRET str;

        SHGetPathFromIDList(pfmhTo->pidlFolder, szTo);
        szTo[lstrlen(szTo) + 1] = 0;   // double null

        if (SUCCEEDED(psf->GetDisplayNameOf(pfmiFrom->pidl, SHGDN_FORPARSING, &str)))
        {
            StrRetToStrN(szFrom, SIZECHARS(szFrom), &str, pfmiFrom->pidl);
            szFrom[lstrlen(szFrom) + 1] = 0;   // double null

            // WARNING: if you change this code to perform rename on
            // collision, be sure to update the pfmiFrom contents to
            // reflect that name change!

            SHFILEOPSTRUCT shop = {hwnd, FO_MOVE, szFrom, szTo, 0, };
            bRet = (NO_ERROR == SHFileOperation(&shop));

            if (bRet)
            {
                // Flush the notification so the menu is updated immediately.
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSH, szFrom, NULL);
            }

            // The move operation will send a notification to the 
            // window, which will eventually invalidate this menu 
            // to have it rebuilt.  However, before that happens
            // we want to record the position of this dragged item
            // in the destination menu.  So change the order of 
            // the menu anyway.
        }
    }

    if (bRet)
    {
        // Change the order of the menu
        int iPosFrom = DPA_GetPtrIndex(hdpaFrom, pfmiFrom);

        bRet = FALSE;

        // Account for the fact we delete before we insert within the
        // same menu
        if (bSameMenu && iPosTo > iPosFrom)
            iPosTo--;

        DPA_DeletePtr(hdpaFrom, iPosFrom);
        iPosTo = DPA_InsertPtr(hdpaTo, iPosTo, pfmiFrom);
        if (-1 != iPosTo)
        {
            // Update the header of the item
            pfmiFrom->pfmh = pfmhTo;

            // Move the menu items
            MENUITEMINFO mii;

            mii.cbSize = SIZEOF(mii);
            mii.fMask = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;
            
            if (GetMenuItemInfo(pfmhFrom->hmenu, iPosFrom, TRUE, &mii))
            {
                // Remove a submenu first so it doesn't get nuked
                if (GetSubMenu(pfmhFrom->hmenu, iPosFrom))
                    RemoveMenu(pfmhFrom->hmenu, iPosFrom, MF_BYPOSITION);

                DeleteMenu(pfmhFrom->hmenu, iPosFrom, MF_BYPOSITION);
                if ( !InsertMenuItem(pfmhTo->hmenu, iPosTo, TRUE, &mii) )
                {
                    TraceMsg(TF_ERROR, "Failed to move menu item");
                    DPA_DeletePtr(hdpaTo, iPosTo);
                }
                else
                {
                    SetFlag(pfmhFrom->fmf, FMF_DIRTY);
                    SetFlag(pfmhTo->fmf, FMF_DIRTY);
                    bRet = TRUE;
                }
            }
        }
        else
        {
            // Punt
            TraceMsg(TF_ERROR, "Menu: could not insert moved item in the DPA");
        }
    }
    return bRet;
}    


/*----------------------------------------------------------
Purpose: Enumerates the folder and adds the files to the DPA.

Returns: count of items in the list
*/
int
FileList_Build(
    IN PFILEMENUHEADER pfmh,
    IN int             cItems,
    IN BOOL            bUseAlt)
    {
#ifdef DEBUG
    TCHAR szName[MAX_PATH];
#endif
    HDPA hdpaTemp;
    HRESULT hres;
    LPITEMIDLIST pidlSkip = NULL;
    LPITEMIDLIST pidlProgs = NULL;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (FileMenuHeader_AllowAbort(pfmh) && g_fAbortInitMenu)
        return -1;

    if (bUseAlt) {
        hdpaTemp = pfmh->hdpaAlt;
    } else {
        hdpaTemp = pfmh->hdpa;
    }


    if (hdpaTemp && pfmh->psf)
    {
        LPENUMIDLIST penum;
        LPSHELLFOLDER psfTemp;

        // Take care with Programs folder.
        // If this is the parent of the programs folder set pidlSkip to
        // the last bit of the programs pidl.
        if (pfmh->fmf & FMF_NOPROGRAMS)
            {
            pidlProgs = SHCloneSpecialIDList(NULL,
                                            (bUseAlt ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS),
                                             TRUE);

            if (ILIsParent((bUseAlt ? pfmh->pidlAltFolder : pfmh->pidlFolder),
                           pidlProgs, TRUE))
                {
                TraceMsg(TF_MENU, "FileList_Build: Programs parent.");
                pidlSkip = ILFindLastID(pidlProgs);
                }
            }

        // Decide which shell folder to enumerate.

        if (bUseAlt) {
            psfTemp = pfmh->psfAlt;
        } else {
            psfTemp = pfmh->psf;
        }

        // We now need to iterate over the children under this guy...
        hres = psfTemp->EnumObjects(NULL, pfmh->fFSFilter, &penum);
        if (SUCCEEDED(hres))
        {
            ULONG celt;
            LPITEMIDLIST pidl = NULL;

            // The pidl is stored away into the pfmi structure, so
            // don't free it here
            while (penum->Next(1, &pidl, &celt) == S_OK && celt == 1)
            {
                PFILEMENUITEM pfmi;

                // Abort.
                if (FileMenuHeader_AllowAbort(pfmh) && g_fAbortInitMenu)
                    break;

                if (pidlSkip && psfTemp->CompareIDs(0, pidlSkip, pidl) == 0)
                {
                   ILFree(pidl);    // Don't leak this one...
                   TraceMsg(DM_TRACE, "FileList_Build: Skipping Programs.");
                   continue;
                }

                // Is there a list of extensions on which we need to
                // filter?
                if (pfmh->pszFilterTypes)
                {
                    STRRET str;
                    DWORD dwAttribs = SFGAO_FOLDER | SFGAO_FILESYSTEM;

                    psfTemp->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &dwAttribs);

                    // only apply the filter to file system objects

                    if ((dwAttribs & SFGAO_FILESYSTEM) &&
                        SUCCEEDED(psfTemp->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)))
                    {
                        TCHAR szFile[MAX_PATH];
                        StrRetToStrN(szFile, SIZECHARS(szFile), &str, pidl);

                        if (!(dwAttribs & SFGAO_FOLDER))
                        {
                            LPTSTR psz = pfmh->pszFilterTypes;
                            LPTSTR pszExt = PathFindExtension(szFile);

                            if (TEXT('.') == *pszExt)
                                pszExt++;

                            while (*psz)
                            {
                                // Skip this file?
                                if (0 == lstrcmpi(pszExt, psz))
                                    break;          // No

                                psz += lstrlen(psz) + 1;
                            }

                            if ( !*psz )
                            {
                                ILFree(pidl);       // don't leak this
                                continue;
                            }
                        }
                    }
                }

                if (FileMenuItem_Create(pfmh, pidl, -1, bUseAlt ? FMI_ALTITEM : 0, &pfmi))
                {
                    int idpa;

                    if (!bUseAlt)
                    {
                        // Set the allowable drop effects (as a target).
                        // We don't allow common user items to be moved.
                        pfmi->dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY;
                    }

                    idpa = DPA_AppendPtr(hdpaTemp, pfmi);

                    // NB We only callback for non-folders at the moment
                    //
                    // HACK don't callback for non file system things
                    // this callback is used to set hotkeys, and that tries
                    // to load the PIDL passed back as a file, and that doesn't
                    // work for non FS pidls
                    if (pfmh->pfncb && (pfmi->Flags & FMI_FILESYSTEM))
                    {
                        FMCBDATA fmcbdata;

                        fmcbdata.hmenu = pfmh->hmenu;
                        fmcbdata.iPos = idpa;
                        // Don't know the id because it hasn't been 
                        // added to the menu yet
                        fmcbdata.idCmd = (UINT)-1;
                        if (bUseAlt) {
                            fmcbdata.pidlFolder = pfmh->pidlAltFolder;
                        } else {
                            fmcbdata.pidlFolder = pfmh->pidlFolder;
                        }
                        fmcbdata.pidl = pidl;
                        fmcbdata.psf = psfTemp;
                        fmcbdata.pvHeader = pfmh;

                        pfmh->pfncb(FMM_ADD, &fmcbdata, 0);
                    }

#ifdef DEBUG
                    FileMenuItem_GetDisplayName(pfmi, szName, ARRAYSIZE(szName));
                    TraceMsg(TF_MENU, "FileList_Build: Non-folder %s", szName);
#endif

                    cItems++;
                }
            }
            penum->Release();
        }
        else
        {
            TraceMsg(TF_ERROR, "FileList_Build: Enumeration failed - leaving folder empty.");
        }

        ILFree(pidlProgs);
    }

    // Insert a special Empty item (unless the header flag says
    // not to).
    if (!cItems && hdpaTemp && !(pfmh->fmf & FMF_NOEMPTYITEM) && !bUseAlt)
    {
        PFILEMENUITEM pfmi;

        if (FileMenuItem_Create(pfmh, NULL, -1, FMI_EMPTY, &pfmi))
            {
            DPA_SetPtr(hdpaTemp, cItems, pfmi);
            cItems++;
            }
        }
    return cItems;
    }


#define FS_SORTBYNAME       0
#define FS_SORTBYORDINAL    1

//---------------------------------------------------------------------------
// Simplified version of the file info comparison function.
int CALLBACK FileMenuItem_Compare(LPVOID pv1, LPVOID pv2, LPARAM lParam)
{
    PFILEMENUITEM pfmi1 = (PFILEMENUITEM)pv1;
    PFILEMENUITEM pfmi2 = (PFILEMENUITEM)pv2;
    int nRet;
    TCHAR szName1[MAX_PATH];
    TCHAR szName2[MAX_PATH];

    switch (lParam)
    {
    case FS_SORTBYNAME:
        // Directories come first, then files
        if ((pfmi1->Flags & FMI_FOLDER) > (pfmi2->Flags & FMI_FOLDER))
            return -1;
        else if ((pfmi1->Flags & FMI_FOLDER) < (pfmi2->Flags & FMI_FOLDER))
            return 1;

        FileMenuItem_GetDisplayName(pfmi1, szName1, ARRAYSIZE(szName1));
        FileMenuItem_GetDisplayName(pfmi2, szName2, ARRAYSIZE(szName2));
        nRet = lstrcmpi(szName1, szName2);
        break;

    case FS_SORTBYORDINAL:
        if (pfmi1->nOrder == pfmi2->nOrder)
            nRet = 0;
        else
            nRet = (pfmi1->nOrder < pfmi2->nOrder ? -1 : 1);
        break;

    default:
        ASSERT_MSG(0, "Bad lParam passed to FileMenuItem_Compare");
        nRet = 0;
        break;
    }

    return nRet;
}


LPVOID CALLBACK FileMenuItem_Merge(UINT uMsg, LPVOID pvDest, LPVOID pvSrc, LPARAM lParam)
{
    PFILEMENUITEM pfmiDest = (PFILEMENUITEM)pvDest;
    PFILEMENUITEM pfmiSrc = (PFILEMENUITEM)pvSrc;
    LPVOID pvRet = pfmiDest;

    switch (uMsg)
    {
    case DPAMM_MERGE:
        // We just care about the order field
        pfmiDest->nOrder = pfmiSrc->nOrder;
        break;

    case DPAMM_DELETE:
    case DPAMM_INSERT:
        // Don't need to implement this
        ASSERT(0);
        pvRet = NULL;
        break;
    }
    
    return pvRet;
}


// Header for file menu streams
typedef struct tagFMSTREAMHEADER
{
    DWORD cbSize;           // Size of header
    DWORD dwVersion;        // Version of header
} FMSTREAMHEADER;

#define FMSTREAMHEADER_VERSION  1

typedef struct tagFMSTREAMITEM
{
    DWORD cbSize;           // Size including pidl (not for versioning)
    int   nOrder;           // User-specified order
} FMSTREAMITEM;

#define CB_FMSTREAMITEM     (sizeof(FMSTREAMITEM))

HRESULT 
CALLBACK 
FileMenuItem_SaveStream(DPASTREAMINFO * pinfo, IStream * pstm, LPVOID pvData)
{
    // We only write menu items with pidls
    PFILEMENUITEM pfmi = (PFILEMENUITEM)pinfo->pvItem;
    HRESULT hres = S_FALSE;

    if (pfmi->pidl)
    {
        FMSTREAMITEM fmsi;
        ULONG cbWrite;
        ULONG cbWritePidl;

        // Size of header, pidl, and ushort for pidl size.
        fmsi.cbSize = CB_FMSTREAMITEM + pfmi->pidl->mkid.cb + sizeof(USHORT);
        fmsi.nOrder = pfmi->nOrder;

        hres = pstm->Write(&fmsi, CB_FMSTREAMITEM, &cbWrite);
        if (SUCCEEDED(hres))
        {
            hres = pstm->Write(pfmi->pidl, pfmi->pidl->mkid.cb + sizeof(USHORT), &cbWritePidl);
            ASSERT(fmsi.cbSize == cbWrite + cbWritePidl);
        }
    }

    return hres;
}   

 
HRESULT 
CALLBACK 
FileMenuItem_LoadStream(DPASTREAMINFO * pinfo, IStream * pstm, LPVOID pvData)
{
    HRESULT hres;
    FMSTREAMITEM fmsi;
    ULONG cbRead;
    PFILEMENUHEADER pfmh = (PFILEMENUHEADER)pvData;

    ASSERT(pfmh);

    hres = pstm->Read(&fmsi, CB_FMSTREAMITEM, &cbRead);
    if (SUCCEEDED(hres))
    {
        if (CB_FMSTREAMITEM != cbRead)
            hres = E_FAIL;
        else
        {
            ASSERT(CB_FMSTREAMITEM < fmsi.cbSize);
            if (CB_FMSTREAMITEM < fmsi.cbSize)
            {
                UINT cb = fmsi.cbSize - CB_FMSTREAMITEM;
                LPITEMIDLIST pidl = _ILCreate(cb);
                if ( !pidl )
                    hres = E_OUTOFMEMORY;
                else
                {
                    hres = pstm->Read(pidl, cb, &cbRead);
                    if (SUCCEEDED(hres) && cb == cbRead && 
                        IS_VALID_PIDL(pidl))
                    {
                        PFILEMENUITEM pfmi;

                        if (FileMenuItem_Create(pfmh, pidl, -1, 0, &pfmi))
                        {
                            pfmi->nOrder = fmsi.nOrder;
                            pinfo->pvItem = pfmi;
                            hres = S_OK;
                        }
                        else
                            hres = E_OUTOFMEMORY;
                    }
                    else
                        hres = E_FAIL;

                    // Cleanup
                    if (FAILED(hres))
                        ILFree(pidl);
                }
            }
            else
                hres = E_FAIL;
        }
    }

    ASSERT((S_OK == hres && pinfo->pvItem) || FAILED(hres));
    return hres;
}    


int
CALLBACK
FileMenuItem_DestroyCB(LPVOID pv, LPVOID pvData)
{
    return FileMenuItem_Destroy((PFILEMENUITEM)pv);
}    


BOOL 
FileList_Load(
    IN  PFILEMENUHEADER pfmh,
    OUT HDPA *    phdpa,
    IN  IStream * pstm)
{
    HDPA hdpa = NULL;
    FMSTREAMHEADER fmsh;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    ASSERT(phdpa);
    ASSERT(pstm);

    // Read the header for more info
    if (SUCCEEDED(pstm->Read(&fmsh, sizeof(fmsh), NULL)) &&
        sizeof(fmsh) == fmsh.cbSize &&
        FMSTREAMHEADER_VERSION == fmsh.dwVersion)
    {
        // Load the stream.  (Should be ordered by name.)
        DPA_LoadStream(&hdpa, FileMenuItem_LoadStream, pstm, pfmh);
    }

    *phdpa = hdpa;

    return (NULL != hdpa);
}    


HRESULT 
FileList_Save(
    IN  PFILEMENUHEADER pfmh,
    IN  IStream * pstm)
{
    HRESULT hres = E_OUTOFMEMORY;
    FMSTREAMHEADER fmsh;
    HDPA hdpa;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    ASSERT(pstm);

    // Clone the array and sort by name for the purpose of persisting it
    hdpa = DPA_Clone(pfmh->hdpa, NULL);
    if (hdpa)
    {
        DPA_Sort(hdpa, FileMenuItem_Compare, FS_SORTBYNAME);

        // Save the header
        fmsh.cbSize = sizeof(fmsh);
        fmsh.dwVersion = FMSTREAMHEADER_VERSION;

        hres = pstm->Write(&fmsh, sizeof(fmsh), NULL);
        if (SUCCEEDED(hres))
        {
            hres = DPA_SaveStream(hdpa, FileMenuItem_SaveStream, pstm, pfmh);
        }

        DPA_Destroy(hdpa);
    }

    return hres;
}    


void FileList_Reorder(PFILEMENUHEADER pfmh)
{
    int i;
    int cel;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    BOOL bCantOrder = (NULL == pfmh->pstm);

    // Update the order fields.  While we're at it, massage the 
    // dwEffect field so it reflects whether something can be
    // ordered based on the stream (no stream means no reorder).

    cel = DPA_GetPtrCount(pfmh->hdpa);
    for (i = 0; i < cel; i++)
    {
        PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_FastGetPtr(pfmh->hdpa, i);
        pfmi->nOrder = i;

        if (bCantOrder)
            pfmi->dwEffect = DROPEFFECT_NONE;
    }
}    


// Caller should release the stream after using it
BOOL FileList_GetStream(PFILEMENUHEADER pfmh, IStream ** ppstm)
{
    if (NULL == pfmh->pstm)
    {
        if (pfmh->pfncb)
        {
            FMGETSTREAM fmgs = { 0 };
            FMCBDATA fmcbdata;

            fmcbdata.hmenu = pfmh->hmenu;
            fmcbdata.idCmd = pfmh->idCmd;
            fmcbdata.iPos = -1;
            fmcbdata.pidlFolder = pfmh->pidlFolder;
            fmcbdata.pidl = NULL;
            fmcbdata.psf = pfmh->psf;
            fmcbdata.pvHeader = pfmh;

            if (S_OK == pfmh->pfncb(FMM_GETSTREAM, &fmcbdata, (LPARAM)&fmgs) &&
                fmgs.pstm)
            {
                // Cache this stream away
                pfmh->pstm = fmgs.pstm;
            }
        }
    }
    else
    {
        // Reset the seek pointer to beginning
        LARGE_INTEGER dlibMove = { 0 };
        pfmh->pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    }

    if (pfmh->pstm)
        pfmh->pstm->AddRef();
        
    *ppstm = pfmh->pstm;

    return (NULL != *ppstm);
}    


void  
FileList_Sort(
    PFILEMENUHEADER pfmh)
{
    IStream * pstm;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    // First sort by name
    DPA_Sort(pfmh->hdpa, FileMenuItem_Compare, FS_SORTBYNAME);

    // Can this menu be sorted by the user?
    if ((pfmh->fmf & FMF_CANORDER) && FileList_GetStream(pfmh, &pstm))
    {
        // Yes; get the stream and try to load the order info
        HDPA hdpaOrder;

        // Read the order from the stream
        if (FileList_Load(pfmh, &hdpaOrder, pstm))
        {
            // Sort the menu according to this stream's order.

            // The persisted order is by name.  This reduces the number of
            // sorts to two at load-time, and 1 at save-time.  (Persisting
            // by ordinal number means we sort three times at load-time, and
            // none at save-time.  We want to speed up the initial menu
            // creation as much as possible.)

            // (Already sorted by name above)
            DPA_Merge(pfmh->hdpa, hdpaOrder, DPAM_SORTED, FileMenuItem_Compare, 
                      FileMenuItem_Merge, FS_SORTBYNAME);
            DPA_Sort(pfmh->hdpa, FileMenuItem_Compare, FS_SORTBYORDINAL);

            DPA_DestroyCallback(hdpaOrder, FileMenuItem_DestroyCB, NULL);
        }

        pstm->Release();
    }

    FileList_Reorder(pfmh);
}


//---------------------------------------------------------------------------
// Use the text extent of the given item and the size of the image to work
// what the full extent of the item will be.
DWORD
GetItemExtent(
    IN HDC           hdc,
    IN PFILEMENUITEM pfmi)
{
    WORD wHeight;
    WORD wWidth;
    DWORD dwExtent;
    TCHAR szName[MAX_PATH];
    PFILEMENUHEADER pfmh;
    BITMAP bmp;

    ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

    FileMenuItem_GetDisplayName(pfmi, szName, ARRAYSIZE(szName));

    pfmh = pfmi->pfmh;
    ASSERT(pfmh);

    // Limit the width of the text?
    if (0 < pfmh->cxMax)
    {
        // Yes
        PathCompactPath(hdc, szName, pfmh->cxMax);
    }

    dwExtent = GetItemTextExtent(hdc, szName);

    wHeight = HIWORD(dwExtent);

    // If no custom height - calc it.
    if (!pfmi->cyItem)
    {
        if (pfmh->fmf & FMF_LARGEICONS)
            wHeight = max(wHeight, ((WORD)g_cyIcon)) + 2;
        else
            wHeight = max(wHeight, ((WORD)g_cySmIcon)) + pfmh->cySpacing;
    }
    else
    {
        wHeight = max(wHeight, pfmi->cyItem);
    }

    ASSERT(pfmi->pfmh);

    //    string, image, gap on either side of image, popup triangle
    //    and background bitmap if there is one.
    // BUGBUG popup triangle size needs to be real
    wWidth = LOWORD(dwExtent) + GetSystemMetrics(SM_CXMENUCHECK);

    // Keep track of the width and height of the bitmap.
    if (pfmh->hbmp && !pfmh->cxBmp && !pfmh->cyBmp)
    {
        GetObject(pfmh->hbmp, SIZEOF(bmp), &bmp);
        pfmh->cxBmp = bmp.bmWidth;
        pfmh->cyBmp = bmp.bmHeight;
    }

    // Gap for bitmap.
    wWidth += (WORD) pfmh->cxBmpGap;

    // Space for image if there is one.
    // NB We currently always allow room for the image even if there
    // isn't one so that imageless items line up properly.
    if (pfmh->fmf & FMF_LARGEICONS)
        wWidth += g_cxIcon + (2 * CXIMAGEGAP);
    else
        wWidth += g_cxSmIcon + (2 * CXIMAGEGAP);

    return MAKELONG(wWidth, wHeight);
}


/*----------------------------------------------------------
Purpose: Get the PFILEMENUITEM of this menu item

Returns: 
Cond:    --
*/
PFILEMENUITEM  
FileMenu_GetItemData(
    IN HMENU hmenu, 
    IN UINT iItem,
    IN BOOL bByPos)
{
    MENUITEMINFO mii;

    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_DATA | MIIM_STATE;
    mii.cch = 0;     // just in case

    if (GetMenuItemInfo(hmenu, iItem, bByPos, &mii))
        return (PFILEMENUITEM)mii.dwItemData;

    return NULL;
}


PFILEMENUHEADER FileMenu_GetHeader(HMENU hmenu)
{
    PFILEMENUITEM pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);

    if (pfmi && 
        EVAL(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM)) &&
        EVAL(IS_VALID_STRUCT_PTR(pfmi->pfmh, FILEMENUHEADER)))
    {
        return pfmi->pfmh;
    }

    return NULL;
}


/*----------------------------------------------------------
Purpose: Create a file menu header.  This header is to be associated 
         with the given menu handle.

         If the menu handle already has header, simply return the
         existing header.

Returns: pointer to header
         NULL on failure
*/
PFILEMENUHEADER
FileMenuHeader_Create(
    IN HMENU        hmenu,
    IN HBITMAP      hbmp,
    IN int          cxBmpGap,
    IN COLORREF     clrBkg,
    IN int          cySel,
    IN const FMCOMPOSE * pfmc)      OPTIONAL
{
    PFILEMENUITEM pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
    PFILEMENUHEADER pfmh;

    // Does this guy already have a header?
    if (pfmi)
    {
        // Yes; use it
        pfmh = pfmi->pfmh;
        ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    }
    else
    {
        // Nope, create one now.
        pfmh = (PFILEMENUHEADER)LocalAlloc(LPTR, SIZEOF(FILEMENUHEADER));
        if (pfmh)
        {
            // Keep track of the header.
            TraceMsg(TF_MENU, "Creating filemenu header for %#08x (%x)", hmenu, pfmh);

            pfmh->hdpa = DPA_Create(0);
            if (pfmh->hdpa == NULL)
            {
                LocalFree((HLOCAL)pfmh);
                pfmh = NULL;
            }
            else
            {
                pfmh->hmenu = hmenu;
                pfmh->hbmp = hbmp;
                pfmh->cxBmpGap = cxBmpGap;
                pfmh->clrBkg = clrBkg;
                pfmh->cySel = cySel;
                pfmh->cySpacing = 6;        // default for small icons
            }
        }
    }

    if (pfmc && pfmh)
    {
        // Set additional values
        if (IsFlagSet(pfmc->dwMask, FMC_CALLBACK))
            pfmh->pfncb = pfmc->pfnCallback;

        if (IsFlagSet(pfmc->dwMask, FMC_CYMAX))
            pfmh->cyMax = pfmc->cyMax;

        if (IsFlagSet(pfmc->dwMask, FMC_CXMAX))
            pfmh->cxMax = pfmc->cxMax;

        if (IsFlagSet(pfmc->dwMask, FMC_CYSPACING))
            pfmh->cySpacing = pfmc->cySpacing;

        if (IsFlagSet(pfmc->dwMask, FMC_FILTERTYPES))
        {
            // This is a double-null terminated string
            MultiSz_AllocCopy(pfmc->pszFilterTypes, &pfmh->pszFilterTypes);
        }
    }

    return pfmh;
}


/*----------------------------------------------------------
Purpose: Set info specific to a folder.

Returns:
Cond:    --
*/
BOOL
FileMenuHeader_SetFolderInfo(
    IN PFILEMENUHEADER pfmh,
    IN const FMCOMPOSE * pfmc)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    ASSERT(pfmc);

    // Keep track of the header.
    pfmh->idCmd = pfmc->id;

    if (IsFlagSet(pfmc->dwMask, FMC_FILTER))
        pfmh->fFSFilter = pfmc->dwFSFilter;

    if (IsFlagSet(pfmc->dwMask, FMC_CYMAX))
        pfmh->cyMax = pfmc->cyMax;

    if (IsFlagSet(pfmc->dwMask, FMC_CXMAX))
        pfmh->cxMax = pfmc->cxMax;

    if (IsFlagSet(pfmc->dwMask, FMC_CYSPACING))
        pfmh->cySpacing = pfmc->cySpacing;

    if (IsFlagSet(pfmc->dwMask, FMC_FILTERTYPES))
        MultiSz_AllocCopy(pfmc->pszFilterTypes, &pfmh->pszFilterTypes);

    if (pfmc->pidlFolder)
    {
        pfmh->pidlFolder = ILClone(pfmc->pidlFolder);
        if (pfmh->pidlFolder)
        {
            LPSHELLFOLDER psfDesktop;
            if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
            {
                if (SUCCEEDED(psfDesktop->BindToObject(pfmh->pidlFolder, 
                    NULL, IID_IShellFolder, (PVOID *)&pfmh->psf)))
                {
                    return TRUE;
                }
            }
            ILFree(pfmh->pidlFolder);
        }
    }
    return FALSE;
}


/*----------------------------------------------------------
Purpose: Create the tooltip window

Returns:
Cond:    --
*/
BOOL
FileMenuHeader_CreateTooltipWindow(
    IN  PFILEMENUHEADER pfmh)
    {
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    // Check if we need to create the main tooltip window
    if (g_hwndTip)
    {
        if (IsWindow(g_hwndTip))
        {
            TCHAR szClass[MAX_PATH];
            GetClassName(g_hwndTip, szClass, ARRAYSIZE(szClass));
            if (lstrcmpi(szClass, TOOLTIPS_CLASS) != 0)
                g_hwndTip = NULL;
        }
        else
            g_hwndTip = NULL;
    }

    if (!g_hwndTip)
        Tooltip_Create(&g_hwndTip);

    ASSERT(IS_VALID_HANDLE(g_hwndTip, WND));

    return NULL != g_hwndTip;
    }


//---------------------------------------------------------------------------
// Give the submenu a marker item so we can check it's a filemenu item
// at initpopupmenu time.
BOOL FileMenuHeader_InsertMarkerItem(PFILEMENUHEADER pfmh)
{
    PFILEMENUITEM pfmi;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (FileMenuItem_Create(pfmh, NULL, -1, FMI_MARKER | FMI_EXPAND, &pfmi))
    {
        DPA_SetPtr(pfmh->hdpa, 0, pfmi);
        FileMenuHeader_InsertItem(pfmh, 0, FMII_DEFAULT);
        return TRUE;
    }
    TraceMsg(TF_ERROR, "FileMenuHeader_InsertMarkerItem: Can't create marker item.");
    return FALSE;
}


/*----------------------------------------------------------
Purpose: This functions adds the given item (index into DPA)
         into the actual menu.

Returns:
Cond:    --
*/
BOOL
FileMenuHeader_InsertItem(
    IN PFILEMENUHEADER pfmh,
    IN UINT            iItem,
    IN FMIIFLAGS       fFlags)
{
    PFILEMENUITEM pfmi;
    UINT fMenu;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    // Normal item.
    pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, iItem);
    if (!pfmi)
        return FALSE;

    if (pfmi->Flags & FMI_ON_MENU)
        return FALSE;
    else
        pfmi->Flags |= FMI_ON_MENU;

    // The normal stuff.
    fMenu = MF_BYPOSITION|MF_OWNERDRAW;
    // Keep track of where it's going in the menu.

    // The special stuff...
    if (fFlags & FMII_BREAK)
    {
        fMenu |= MF_MENUBARBREAK;
    }

    // Is it a folder (that's not open yet)?
    if (pfmi->Flags & FMI_FOLDER)
    {
        // Yep. Create a submenu item.
        HMENU hmenuSub = CreatePopupMenu();
        if (hmenuSub)
        {
            MENUITEMINFO mii;
            LPITEMIDLIST pidlSub;
            PFILEMENUHEADER pfmhSub;
            FMCOMPOSE fmc;

            // Set the callback now so it can be called when adding items
            fmc.cbSize = SIZEOF(fmc);
            fmc.dwMask = FMC_CALLBACK;
            fmc.pfnCallback = pfmh->pfncb;

            // Insert it into the parent menu.
            fMenu |= MF_POPUP;
            InsertMenu(pfmh->hmenu, iItem, fMenu, (UINT_PTR)hmenuSub, (LPTSTR)pfmi);
            // Set it's ID.
            mii.cbSize = SIZEOF(mii);
            mii.fMask = MIIM_ID;
            mii.wID = pfmh->idCmd;
            SetMenuItemInfo(pfmh->hmenu, iItem, TRUE, &mii);
            pidlSub = ILCombine((pfmi->Flags & FMI_ALTITEM) ? pfmh->pidlAltFolder : pfmh->pidlFolder, pfmi->pidl);
            pfmhSub = FileMenuHeader_Create(hmenuSub, NULL, 0, (COLORREF)-1, 0, &fmc);
            ASSERT(pfmh);
            if (pfmh)
            {
                // Inherit settings from the parent filemenu
                fmc.dwMask     = FMC_PIDL | FMC_FILTER | FMC_CYMAX |
                                 FMC_CXMAX | FMC_CYSPACING;
                fmc.id         = pfmh->idCmd;
                fmc.pidlFolder = pidlSub;
                fmc.dwFSFilter = pfmh->fFSFilter;
                fmc.cyMax      = pfmh->cyMax;
                fmc.cxMax      = pfmh->cxMax;
                fmc.cySpacing  = pfmh->cySpacing;

                if (pfmh->pszFilterTypes)
                {
                    fmc.dwMask |= FMC_FILTERTYPES;
                    fmc.pszFilterTypes = pfmh->pszFilterTypes;
                }

                FileMenuHeader_SetFolderInfo(pfmhSub, &fmc);

                // Magically inherit certain flags
                // BUGBUG (scotth): can we inherit all the bits?
                pfmhSub->fmf = pfmh->fmf & FMF_INHERITMASK;

                // Build it a bit at a time.
                FileMenuHeader_InsertMarkerItem(pfmhSub);
            }
            ILFree(pidlSub);
        }
    }
    else
    {
        // Nope.
        if (pfmi->Flags & FMI_EMPTY)
            fMenu |= MF_DISABLED | MF_GRAYED;

        InsertMenu(pfmh->hmenu, iItem, fMenu, pfmh->idCmd, (LPTSTR)pfmi);
    }

    return TRUE;
}


/*----------------------------------------------------------
Purpose: Remove the rest of the items from the main list starting
         at the given index.

Returns: --
Cond:    --
*/
void
FileList_StripLeftOvers(
    IN PFILEMENUHEADER pfmh,
    IN int             idpaStart,
    IN BOOL            bUseAlt)
{
    int cItems;
    int i;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    cItems = DPA_GetPtrCount(pfmh->hdpa);

    // Do this backwards to stop things moving around as
    // we delete them.
    for (i = cItems - 1; i >= idpaStart; i--)
    {
        PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, i);
        if (pfmi)
        {
            // Tell the callback we're removing this
            if (pfmh->pfncb && pfmi->pidl && 
                IsFlagClear(pfmi->Flags, FMI_IGNORE_PIDL))
            {
                FMCBDATA fmcbdata;

                fmcbdata.hmenu = pfmh->hmenu;
                fmcbdata.iPos = i;
                fmcbdata.idCmd = GetMenuItemID(pfmh->hmenu, i);
                if (bUseAlt)
                {
                    fmcbdata.pidlFolder = pfmh->pidlAltFolder;
                    fmcbdata.psf = pfmh->psfAlt;
                }
                else
                {
                    fmcbdata.pidlFolder = pfmh->pidlFolder;
                    fmcbdata.psf = pfmh->psf;
                }
                fmcbdata.pidl = pfmi->pidl;
                fmcbdata.pvHeader = pfmh;

                pfmh->pfncb(FMM_REMOVE, &fmcbdata, 0);
            }

            // (We don't need to worry about recursively deleting
            // subfolders because their contents haven't been added yet.)

            // Delete the item itself (note there is no menu item
            // to delete)
            FileMenuItem_Destroy(pfmi);
            DPA_DeletePtr(pfmh->hdpa, i);
        }
    }
}


/*----------------------------------------------------------
Purpose: This function adds a "More Items..." menu item
         at the bottom of the menu.  It calls the callback
         to get the string.

Returns: 
Cond:    --
*/
void
FileMenuHeader_AddMoreItemsItem(
    IN PFILEMENUHEADER pfmh,
    IN UINT            iPos)
{
    PFILEMENUITEM pfmi;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (NULL == pfmh->pfncb)
    {
        // BUGBUG (scotth): this shouldn't be required, but we don't
        // have a default resource ID for this right now.

        TraceMsg(TF_ERROR, "Need a callback in order to add a More item.");
        ASSERT(0);
    }
    else if (FileMenuItem_Create(pfmh, NULL, -1, 0, &pfmi))
    {
        FMCBDATA fmcbdata;
        FMMORESTRING fmms = {0};

        // Make the pidl be the whole path to the folder
        pfmi->pidl = ILClone(pfmh->pidlFolder);
        pfmi->Flags |= FMI_IGNORE_PIDL;

        fmcbdata.hmenu = pfmh->hmenu;
        fmcbdata.iPos = -1;
        fmcbdata.idCmd = (UINT)-1;

        // BUGBUG (scotth): we don't ask for string for alternate lists
        fmcbdata.pidlFolder = NULL;
        fmcbdata.pidl = pfmi->pidl;
        fmcbdata.psf = pfmh->psf;

        // Was a string set?
        if (S_OK == pfmh->pfncb(FMM_GETMORESTRING, &fmcbdata, (LPARAM)&fmms))
        {
            Sz_AllocCopy(fmms.szMoreString, &(pfmi->psz));

            if (DPA_SetPtr(pfmh->hdpa, iPos, pfmi))
            {
                MENUITEMINFO mii;

                // Set the command ID
                mii.cbSize = SIZEOF(mii);
                mii.fMask  = MIIM_ID | MIIM_TYPE | MIIM_DATA;
                mii.wID    = fmms.uID;
                mii.fType  = MFT_OWNERDRAW;
                mii.dwItemData = (DWORD_PTR)pfmi;

                EVAL(InsertMenuItem(pfmh->hmenu, iPos, TRUE, &mii));
            }
        }
    }
}


/*----------------------------------------------------------
Purpose: Enumerates the DPA and adds each item into the
         menu.  Inserts vertical breaks if the menu becomes
         too long.

Returns: count of items added to menu
Cond:    --
*/
int
FileList_AddToMenu(
    IN PFILEMENUHEADER pfmh,
    IN BOOL            bUseAlt,
    IN BOOL            bAddSeparatorSpace)
{
    UINT i, cItems;
    int cItemMac;
    PFILEMENUITEM pfmi;
    int cyMenu, cyItem, cyMenuMax;
    HDC hdc;
    HFONT hfont, hfontOld;
    NONCLIENTMETRICS ncm;
    int idpa;
    HDPA hdpaT;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (bUseAlt)
        hdpaT = pfmh->hdpaAlt;
    else
        hdpaT = pfmh->hdpa;

    cItemMac = 0;
    cyItem = 0;
    cyMenu = pfmh->cyMenuSizeSinceLastBreak;

    if (0 < pfmh->cyMax)
        cyMenuMax = pfmh->cyMax;
    else
        cyMenuMax = GetSystemMetrics(SM_CYSCREEN);

    // Get the rough height of an item so we can work out when to break the
    // menu. User should really do this for us but that would be useful.
    hdc = GetDC(NULL);
    if (hdc)
    {
        ncm.cbSize = SIZEOF(NONCLIENTMETRICS);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, SIZEOF(ncm), &ncm, FALSE))
        {
            hfont = CreateFontIndirect(&ncm.lfMenuFont);
            if (hfont)
            {
                hfontOld = SelectFont(hdc, hfont);
                cyItem = HIWORD(GetItemExtent(hdc, (PFILEMENUITEM)DPA_GetPtr(hdpaT, 0)));
                SelectObject(hdc, hfontOld);
                DeleteObject(hfont);
            }
        }
        ReleaseDC(NULL, hdc);
    }

    // If we are appending items to a menu, we need to account
    // for the separator.

    if (bAddSeparatorSpace) {
        cyMenu += cyItem;
    }

    cItems = DPA_GetPtrCount(hdpaT);

    for (i = 0; i < cItems; i++)
    {
        if (bUseAlt) {
            // Move the items from the alternate list to the main
            // list and use the new index.
            pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpaAlt, i);
            if (!pfmi)
                continue;

            idpa = DPA_AppendPtr(pfmh->hdpa, pfmi);

        } else {
            idpa = i;
        }

        // Keep a rough count of the height of the menu.
        cyMenu += cyItem;
        if (cyMenu > cyMenuMax)
        {
            // Add a vertical break?
            if ( !(pfmh->fmf & (FMF_NOBREAK | FMF_RESTRICTHEIGHT)) )
            {
                // Yes
                FileMenuHeader_InsertItem(pfmh, idpa, FMII_BREAK);
                cyMenu = cyItem;
            }
            // Restrict height?
            else if (IsFlagSet(pfmh->fmf, FMF_RESTRICTHEIGHT))
            {
                // Yes; remove the remaining items from the list
                FileList_StripLeftOvers(pfmh, idpa, bUseAlt);

                // (so cyMenuSizeSinceLastBreak is accurate)
                cyMenu -= cyItem;

                // Add a "more..." item at the end?
                if (pfmh->fmf & FMF_MOREITEMS)
                {
                    // Yes
                    FileMenuHeader_AddMoreItemsItem(pfmh, idpa);
                }

                // We won't go any further
                break;
            }
        }
        else
        {
            FileMenuHeader_InsertItem(pfmh, idpa, FMII_DEFAULT);
            cItemMac++;
        }
    }

    // Save the current cy size so we can use this again
    // if more items are appended to this menu.

    pfmh->cyMenuSizeSinceLastBreak = cyMenu;

    return cItemMac;
}


BOOL
FileList_AddImages(
    IN PFILEMENUHEADER pfmh,
    IN BOOL            bUseAlt)
{
    PFILEMENUITEM pfmi;
    int i, cItems;
    HDPA hdpaTemp;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (bUseAlt) {
        hdpaTemp = pfmh->hdpaAlt;
    } else {
        hdpaTemp = pfmh->hdpa;
    }

    cItems = DPA_GetPtrCount(hdpaTemp);
    for (i = 0; i < cItems; i++)
    {
        if (FileMenuHeader_AllowAbort(pfmh) && g_fAbortInitMenu)
        {
            TraceMsg(TF_MENU, "FileList_AddImages: Abort: Defering images till later.");
            break;
        }

        pfmi = (PFILEMENUITEM)DPA_GetPtr(hdpaTemp, i);
        if (pfmi && pfmi->pidl && (pfmi->iImage == -1) &&
            IsFlagClear(pfmi->Flags, FMI_IGNORE_PIDL))
        {
            pfmi->iImage = SHMapPIDLToSystemImageListIndex(
                                (bUseAlt ? pfmh->psfAlt : pfmh->psf),
                                pfmi->pidl, NULL);
        }
    }
    return TRUE;
}


//---------------------------------------------------------------------------
BOOL FileMenuItem_Destroy(PFILEMENUITEM pfmi)
{
    BOOL fRet = FALSE;

    ASSERT(NULL == pfmi || IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

    if (pfmi)
    {
        if (pfmi->pidl)
            ILFree(pfmi->pidl);
        if (pfmi->psz)
            LFree(pfmi->psz);
        if (pfmi->pszTooltip)
            LFree(pfmi->pszTooltip);
        LocalFree(pfmi);
        fRet = TRUE;
    }

    return fRet;
}

//---------------------------------------------------------------------------
// Clean up the items created by FileList_Build;
void FileList_UnBuild(PFILEMENUHEADER pfmh)
{
    int cItems;
    int i;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    cItems = DPA_GetPtrCount(pfmh->hdpa);
    for (i=cItems-1; i>=0; i--)
    {
        PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, i);
        if (FileMenuItem_Destroy(pfmi))
            DPA_DeletePtr(pfmh->hdpa, i);
    }
}


// Flags for FileMenuHeader_AddFiles
#define FMHAF_USEALT            0x0001
#define FMHAF_SEPARATOR         0x0002

/*----------------------------------------------------------
Purpose: Add files to a file menu header. This function goes thru
         the following steps:

         - enumerates the folder and fills the hdpa list with items
           (files and subfolders)
         - sorts the list
         - gets the images for the items in the list
         - adds the items from list into actual menu

         The last step also (optionally) caps the length of the
         menu to the specified height.  Ideally, this should
         happen at the enumeration time, except the required sort
         prevents this from happening.  So we end up adding a
         bunch of items to the list and then removing them if
         there are too many.

Returns: count of items added
         -1 if aborted
Cond:    --
*/
HRESULT
FileMenuHeader_AddFiles(
    IN  PFILEMENUHEADER pfmh,
    IN  int             iPos,
    IN  UINT            uFlags,                 // FMHAF_*
    OUT int *           pcItems)
{
    HRESULT hres;
    BOOL bUseAlt = IsFlagSet(uFlags, FMHAF_USEALT);

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    int cItems = FileList_Build(pfmh, iPos, bUseAlt);

    // If the build was aborted cleanup and early out.
    if (FileMenuHeader_AllowAbort(pfmh) && g_fAbortInitMenu)
    {
        // Cleanup.
        TraceMsg(TF_MENU, "FileList_Build aborted.");
        FileList_UnBuild(pfmh);
        hres = E_ABORT;
        *pcItems = -1;
    }
    else
    {
        *pcItems = cItems;

        if (cItems > 1)
            FileList_Sort(pfmh);

        if (cItems != 0)
        {
            BOOL bSeparator = IsFlagSet(uFlags, FMHAF_SEPARATOR);
            if (bSeparator)
            {
                // insert a line
                FileMenu_AppendItem(pfmh->hmenu, (LPTSTR)FMAI_SEPARATOR, 0, -1, NULL, 0);
            }

            // Add the images *after* adding to the menu, since the menu
            // may be capped to a maximum height, and we can then prevent
            // adding images we won't need.
            *pcItems = FileList_AddToMenu(pfmh, bUseAlt, bSeparator);
            FileList_AddImages(pfmh, bUseAlt);
        }

        hres = (*pcItems < cItems) ? S_FALSE : S_OK;
    }

    if (g_fAbortInitMenu)
        g_fAbortInitMenu = FALSE;

    TraceMsg(TF_MENU, "FileMenuHeader_AddFiles: Added %d filemenu items.", cItems);
    return hres;
}


//----------------------------------------------------------------------------
// Free up a header (you should delete all the items first).
void FileMenuHeader_Destroy(PFILEMENUHEADER pfmh)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    TraceMsg(TF_MENU, "Destroy filemenu for (%x)", pfmh);

    // Clean up the header.
    DPA_Destroy(pfmh->hdpa);
    if (pfmh->pidlFolder)
    {
        ILFree(pfmh->pidlFolder);
        pfmh->pidlFolder = NULL;
    }
    if (pfmh->psf)
    {
        pfmh->psf->Release();
        pfmh->psf = NULL;
    }

    if (pfmh->pstm)
    {
        pfmh->pstm->Release();
        pfmh->pstm = NULL;
    }

    if (pfmh->pidlAltFolder)
    {
        ILFree(pfmh->pidlAltFolder);
        pfmh->pidlAltFolder = NULL;
    }
    if (pfmh->psfAlt)
    {
        pfmh->psfAlt->Release();
        pfmh->psfAlt = NULL;
    }
    if (pfmh->pszFilterTypes)
    {
        LFree(pfmh->pszFilterTypes);
        pfmh->pszFilterTypes = NULL;
    }

    LocalFree((HLOCAL)pfmh);    // needed?
}

//---------------------------------------------------------------------------
// We create subemnu's with one marker item so we can check it's a file menu
// at init popup time but we need to delete it before adding new items.
BOOL FileMenuHeader_DeleteMarkerItem(PFILEMENUHEADER pfmh)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    // It should just be the only one in the menu.
    if (GetMenuItemCount(pfmh->hmenu) == 1)
    {
        // It should have the right id.
        if (GetMenuItemID(pfmh->hmenu, 0) == pfmh->idCmd)
        {
            // With item data and the marker flag set.
            PFILEMENUITEM pfmi = FileMenu_GetItemData(pfmh->hmenu, 0, TRUE);
            if (pfmi && (pfmi->Flags & FMI_MARKER))
            {
                // Delete it.
                ASSERT(pfmh->hdpa);
                ASSERT(DPA_GetPtrCount(pfmh->hdpa) == 1);
                // NB The marker shouldn't have a pidl.
                ASSERT(!pfmi->pidl);

                LocalFree((HLOCAL)pfmi);

                DPA_DeletePtr(pfmh->hdpa, 0);
                DeleteMenu(pfmh->hmenu, 0, MF_BYPOSITION);
                // Cleanup OK.
                return TRUE;
            }
        }
    }

    TraceMsg(TF_MENU, "Can't find marker item.");
    return FALSE;
}


/*----------------------------------------------------------
Purpose: Add files to this menu.

Returns: number of items added
Cond:    --
*/
HRESULT
FileMenu_AddFiles(
    IN     HMENU       hmenu,
    IN     UINT        iPos,
    IN OUT FMCOMPOSE * pfmc)
{
    HRESULT hres = E_OUTOFMEMORY;
    BOOL fMarker = FALSE;
    PFILEMENUHEADER pfmh;

    // NOTE:  this function takes in FMCOMPOSE, which can be A or W
    //        version depending on the platform.  Since the function 
    //        is internal, wrapped by FileMenu_ComposeA/W, it expects 
    //        the pidl to be valid, and will not use the pszFolder field.

    if (IsFlagClear(pfmc->dwMask, FMC_FILTER))
        pfmc->dwFSFilter = 0;

    if (IsFlagClear(pfmc->dwMask, FMC_FLAGS))
        pfmc->dwFlags = 0;

    // (FileMenuHeader_Create might return an existing header)
    pfmh = FileMenuHeader_Create(hmenu, NULL, 0, (COLORREF)-1, 0, pfmc);
    if (pfmh)
    {
        PFILEMENUITEM pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
        if (pfmi)
        {
            // Clean up marker item if there is one.
            if ((pfmi->Flags & FMI_MARKER) && (pfmi->Flags & FMI_EXPAND))
            {
                // Nope, do it now.
                TraceMsg(TF_MENU, "Removing marker item.");
                FileMenuHeader_DeleteMarkerItem(pfmh);
                fMarker = TRUE;
                if (iPos)
                    iPos--;
            }
        }

        // Add the new stuff
        FileMenuHeader_SetFolderInfo(pfmh, pfmc);

        // Tack on more flags
        pfmh->fmf |= pfmc->dwFlags;

        SetFlag(pfmh->fmf, FMF_NOABORT);
        hres = FileMenuHeader_AddFiles(pfmh, iPos, 0, &pfmc->cItems);
        ClearFlag(pfmh->fmf, FMF_NOABORT);

        if ((E_ABORT == hres || 0 == pfmc->cItems) && fMarker)
        {
            // Aborted or no items. Put the marker back (if there used
            // to be one).
            FileMenuHeader_InsertMarkerItem(pfmh);
        }
    }

    return hres;
}


//---------------------------------------------------------------------------
// Returns the number of items added.
STDAPI_(UINT)
FileMenu_AppendFilesForPidl(
    HMENU hmenu,
    LPITEMIDLIST pidl,
    BOOL bInsertSeparator)
{
    int cItems = 0;
    BOOL fMarker = FALSE;
    PFILEMENUHEADER pfmh;
    PFILEMENUITEM pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);

    ASSERT(IS_VALID_HANDLE(hmenu, MENU));
    ASSERT(IS_VALID_PIDL(pidl));

    //
    // Get the filemenu header from the first filemenu item
    //

    if (!pfmi)
        return 0;

    pfmh = pfmi->pfmh;


    if (pfmh)
    {
        // Clean up marker item if there is one.
        if ((pfmi->Flags & FMI_MARKER) && (pfmi->Flags & FMI_EXPAND))
        {
            // Nope, do it now.
            // TraceMsg(DM_TRACE, "t.fm_ii: Removing marker item.");
            FileMenuHeader_DeleteMarkerItem(pfmh);
            fMarker = TRUE;
        }

        // Add the new stuff.
        if (pidl)
        {
            LPSHELLFOLDER psfDesktop;
            if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
            {
                pfmh->pidlAltFolder = ILClone(pidl);

                if (pfmh->pidlAltFolder) {

                    pfmh->hdpaAlt = DPA_Create(0);

                    if (pfmh->hdpaAlt) {

                        if (SUCCEEDED(psfDesktop->BindToObject(pfmh->pidlAltFolder, 
                            NULL, IID_IShellFolder, (LPVOID *)&pfmh->psfAlt)))
                        {
                            UINT uFlags = FMHAF_USEALT;

                            if (bInsertSeparator)
                                uFlags |= FMHAF_SEPARATOR;

                            pfmh->fmf |= FMF_NOABORT;
                            FileMenuHeader_AddFiles(pfmh, 0, uFlags, &cItems);
                            pfmh->fmf = pfmh->fmf & ~FMF_NOABORT;
                        }

                        DPA_Destroy (pfmh->hdpaAlt);
                        pfmh->hdpaAlt = NULL;
                    }
                }
                // we assume this is a static object... which it is.
                // psfDesktop->Release();
            }
        }

        if (cItems <= 0 && fMarker)
        {
            // Aborted or no item  s. Put the marker back (if there used
            // to be one).
            FileMenuHeader_InsertMarkerItem(pfmh);
        }
    }

    return cItems;
}


//---------------------------------------------------------------------------
// Delete all the menu items listed in the given header.
UINT
FileMenuHeader_DeleteAllItems(
    IN PFILEMENUHEADER pfmh)
{
    int i;
    int cItems = 0;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER))
    {
        // Notify.
        if (pfmh->pfncb)
            {
            FMCBDATA fmcbdata;

            fmcbdata.hmenu = pfmh->hmenu;
            fmcbdata.iPos = 0;
            fmcbdata.idCmd = (UINT)-1;
            fmcbdata.pidlFolder = pfmh->pidlFolder;
            fmcbdata.pidl = NULL;
            fmcbdata.psf = pfmh->psf;
            fmcbdata.pvHeader = pfmh;

            pfmh->pfncb(FMM_DELETEALL, &fmcbdata, 0);
            }

        // Clean up the items.
        cItems = DPA_GetPtrCount(pfmh->hdpa);
        // Do this backwards to stop things moving around as
        // we delete them.
        for (i = cItems - 1; i >= 0; i--)
        {
            PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, i);
            if (pfmi)
            {
                // Does this item have a subfolder?
                if (pfmi->Flags & FMI_FOLDER)
                {
                    // Yep.
                    // Get the submenu for this item.
                    // Delete all it's items.
                    FileMenu_DeleteAllItems(GetSubMenu(pfmh->hmenu, i));
                }
                // Delete the item itself.
                DeleteMenu(pfmh->hmenu, i, MF_BYPOSITION);
                FileMenuItem_Destroy(pfmi);
                DPA_DeletePtr(pfmh->hdpa, i);
            }
        }
    }
    return cItems;
}

//---------------------------------------------------------------------------
// NB The creator of the filemenu has to explicitly call FileMenu_DAI to free
// up FileMenu items because USER doesn't send WM_DELETEITEM for ownerdraw
// menu. Great eh?
// Returns the number of items deleted.
UINT  FileMenu_DeleteAllItems(HMENU hmenu)
{
    PFILEMENUHEADER pfmh;

    if (!IsMenu(hmenu))
        return 0;

    pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
        
        // Save the order if necessary
        if (IsFlagSet(pfmh->fmf, FMF_DIRTY | FMF_CANORDER))
        {
            FileMenu_SaveOrder(pfmh->hmenu);
            ClearFlag(pfmh->fmf, FMF_DIRTY);
        }

        UINT cItems = FileMenuHeader_DeleteAllItems(pfmh);
        FileMenuHeader_Destroy(pfmh);
        return cItems;
    }

    return 0;
}

//---------------------------------------------------------------------------
STDAPI_(void)
FileMenu_Destroy(HMENU hmenu)
{
    TraceMsg(TF_MENU, "Destroying filemenu for %#08x", hmenu);

    FileMenu_DeleteAllItems(hmenu);
    DestroyMenu(hmenu);

    // Reset the menu tracking agent
    g_fsmenuagent.Reset();

    //
    // Delete current global g_hdcMem and g_hfont so they'll be
    // refreshed with current font metrics next time the menu size
    // is calculated.  This is needed in case the menu is being destroyed
    // as part of a system metrics change.
    //
    DeleteGlobalMemDCAndFont();
}


//---------------------------------------------------------------------------
// Cause the given filemenu to be rebuilt.
STDAPI_(void)
FileMenu_Invalidate(HMENU hmenu)
{
    ASSERT(IS_VALID_HANDLE(hmenu, MENU));

    // Is this a filemenu?
    // NB First menu item must be a FileMenuItem.
    PFILEMENUITEM pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
    if (pfmi)
    {
        ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

        // Yep, Is there already a marker here?
        if ((pfmi->Flags & FMI_MARKER) && (pfmi->Flags & FMI_EXPAND))
        {
            TraceMsg(TF_MENU, "Menu is already invalid.");
        }
        else if (pfmi->pfmh)
        {
            PFILEMENUHEADER pfmhSave = pfmi->pfmh;

            FileMenuHeader_DeleteAllItems(pfmi->pfmh);

            ASSERT(IS_VALID_STRUCT_PTR(pfmhSave, FILEMENUHEADER));

            // above call freed pfmi
            FileMenuHeader_InsertMarkerItem(pfmhSave);
        }
    }
}


//---------------------------------------------------------------------------
// Cause the given filemenu to be marked invalid but don't delete any items
// yet.
void  FileMenu_DelayedInvalidate(HMENU hmenu)
{
    // Is this a filemenu?
    // NB First menu item must be a FileMenuItem.
    PFILEMENUITEM pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
    if (pfmi && pfmi->pfmh)
        SetFlag(pfmi->pfmh->fmf, FMF_DELAY_INVALID);
}


BOOL  FileMenu_IsDelayedInvalid(HMENU hmenu)
{
    PFILEMENUITEM pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);

    return (pfmi && pfmi->pfmh &&
            IsFlagSet(pfmi->pfmh->fmf, FMF_DELAY_INVALID));
}


/*----------------------------------------------------------
Purpose: Compose a file menu.

         Ansi version

Returns: S_OK if all the files were added
         S_FALSE if some did not get added (reached cyMax)
         error on something bad
Cond:    --
*/
STDAPI
FileMenu_ComposeA(
    IN HMENU        hmenu,
    IN UINT         nMethod,
    IN FMCOMPOSEA * pfmc)
{
    HRESULT hres = E_INVALIDARG;

    if (IS_VALID_WRITE_PTR(pfmc, FMCOMPOSEA) &&
        SIZEOF(*pfmc) == pfmc->cbSize)
    {
        FMCOMPOSEA fmc;

        fmc = *pfmc;

        if (IsFlagSet(fmc.dwMask, FMC_STRING))
        {
            // Convert string to pidl
            TCHAR szFolder[MAX_PATH];

#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, 0, fmc.pszFolder, -1, szFolder,
                                SIZECHARS(szFolder));
#else
            lstrcpy(szFolder, fmc.pszFolder);
#endif
            fmc.pidlFolder = ILCreateFromPath(szFolder);
            if (NULL == fmc.pidlFolder)
            {
                hres = E_OUTOFMEMORY;
                goto Bail;
            }
        }
        else if (IsFlagClear(fmc.dwMask, FMC_PIDL))
        {
            // Either FMC_PIDL or FMC_STRING must be set
            hres = E_INVALIDARG;
            goto Bail;
        }

        switch (nMethod)
        {
        case FMCM_INSERT:
            hres = FileMenu_AddFiles(hmenu, 0, (FMCOMPOSE *)&fmc);
            break;

        case FMCM_APPEND:
            hres = FileMenu_AddFiles(hmenu, GetMenuItemCount(hmenu),
                                     (FMCOMPOSE *)&fmc);
            break;

        case FMCM_REPLACE:
            FileMenu_DeleteAllItems(hmenu);
            hres = FileMenu_AddFiles(hmenu, 0, (FMCOMPOSE *)&fmc);
            break;

        default:
            ASSERT(0);
            goto Bail;
        }

        pfmc->cItems = fmc.cItems;

Bail:
        // Cleanup
        if (IsFlagSet(fmc.dwMask, FMC_STRING) && fmc.pidlFolder)
            ILFree(fmc.pidlFolder);
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Compose a file menu.

         Unicode version

Returns:
Cond:    --
*/
STDAPI
FileMenu_ComposeW(
    IN HMENU        hmenu,
    IN UINT         nMethod,
    IN FMCOMPOSEW * pfmc)
{
    HRESULT hres = E_INVALIDARG;

    if (IS_VALID_WRITE_PTR(pfmc, FMCOMPOSEW) &&
        SIZEOF(*pfmc) == pfmc->cbSize)
    {
        FMCOMPOSEW fmc;

        fmc = *pfmc;

        if (IsFlagSet(fmc.dwMask, FMC_STRING))
        {
            // Convert string to pidl
            TCHAR szFolder[MAX_PATH];

#ifdef UNICODE
            lstrcpy(szFolder, fmc.pszFolder);
#else
            WideCharToMultiByte(CP_ACP, 0, fmc.pszFolder, -1, szFolder,
                                SIZECHARS(szFolder), NULL, NULL);
#endif
            fmc.pidlFolder = ILCreateFromPath(szFolder);
            if (NULL == fmc.pidlFolder)
            {
                hres = E_OUTOFMEMORY;
                goto Bail;
            }
        }
        else if (IsFlagClear(fmc.dwMask, FMC_PIDL))
        {
            // Either FMC_PIDL or FMC_STRING must be set
            hres = E_INVALIDARG;
            goto Bail;
        }

        switch (nMethod)
        {
        case FMCM_INSERT:
            hres = FileMenu_AddFiles(hmenu, 0, (FMCOMPOSE *)&fmc);
            break;

        case FMCM_APPEND:
            hres = FileMenu_AddFiles(hmenu, GetMenuItemCount(hmenu),
                                     (FMCOMPOSE *)&fmc);
            break;

        case FMCM_REPLACE:
            FileMenu_DeleteAllItems(hmenu);
            hres = FileMenu_AddFiles(hmenu, 0, (FMCOMPOSE *)&fmc);
            break;

        default:
            ASSERT(0);
            goto Bail;
        }

        pfmc->cItems = fmc.cItems;

Bail:
        // Cleanup
        if (IsFlagSet(fmc.dwMask, FMC_STRING) && fmc.pidlFolder)
            ILFree(fmc.pidlFolder);
    }

    return hres;
}


LRESULT FileMenu_DrawItem(HWND hwnd, DRAWITEMSTRUCT *pdi)
{
    int y, x;
    TCHAR szName[MAX_PATH];
    DWORD dwExtent;
    int cxIcon, cyIcon;
    RECT rcBkg;
    HBRUSH hbrOld = NULL;
    UINT cyItem, dyItem;
    HIMAGELIST himl;
    RECT rcClip;

    if ((pdi->itemAction & ODA_SELECT) || (pdi->itemAction & ODA_DRAWENTIRE))
    {
        PFILEMENUHEADER pfmh;
        PFILEMENUITEM pfmi = (PFILEMENUITEM)pdi->itemData;
        IShellFolder * psf;

#ifndef UNIX
        ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));
#endif
        
        if (!pfmi)
        {
            TraceMsg(TF_ERROR, "FileMenu_DrawItem: Filemenu is invalid (no item data).");
            return FALSE;
        }

        pfmh = pfmi->pfmh;
        ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

        if (pfmi->Flags & FMI_ALTITEM)
            psf = pfmh->psfAlt;
        else
            psf = pfmh->psf;

        // Adjust for large/small icons.
        if (pfmh->fmf & FMF_LARGEICONS)
        {
            cxIcon = g_cxIcon;
            cyIcon = g_cyIcon;
        }
        else
        {
            cxIcon = g_cxSmIcon;
            cyIcon = g_cxSmIcon;
        }

        // Is the menu just starting to get drawn?
        if (pdi->itemAction & ODA_DRAWENTIRE)
        {
            if (pfmi == DPA_GetPtr(pfmh->hdpa, 0))
            {
                // Yes; reset the last selection item
                g_pfmiLastSelNonFolder = NULL;
                g_pfmiLastSel = NULL;

                // Initialize to handle drag and drop?
                if (pfmh->fmf & FMF_CANORDER)
                {
                    // Yes
                    g_fsmenuagent.Init();
                }
            }
        }


        if (pdi->itemState & ODS_SELECTED)
        {
            if (pfmh->fmf & FMF_CANORDER)
            {
                // Pass on the current hDC and selection rect so the 
                // drag/drop hook can actively draw

                RECT rc = pdi->rcItem;
                
                hbrOld = SelectBrush(pdi->hDC, GetSysColorBrush(COLOR_MENUTEXT));

                // With no background image, the caret goes all the way 
                // across; otherwise it stops in line with the bitmap.
                if (pfmh->hbmp)
                    rc.left += pfmh->cxBmpGap;

                g_fsmenuagent.SetCurrentRect(pdi->hDC, &rc);
                g_fsmenuagent.SetItem(pfmi);

                // Are we in edit mode?
                if (MenuDD_IsButtonDown())
                {
                    // Yes
                    g_fsmenuagent.SetEditMode(TRUE, DROPEFFECT_MOVE);
                }
            }

            // Determine the selection colors
            //
            // Normal menu colors apply until we are in edit mode, in which
            // case the menu item is drawn unselected and an insertion caret 
            // is drawn above or below the current item.  The exception is 
            // if the item is a cascaded menu item, then we draw it 
            // normally, but also show the insertion caret.  (We do this
            // because Office does this, and also, USER draws the arrow
            // in the selected color always, so it looks kind of funny 
            // if we don't select the menu item.)
            //

            // Is the user dragging and dropping and we're not over
            // a cascaded menu item?
            if ((pfmh->fmf & FMF_CANORDER) && MenuDD_InEditMode() &&
                !(pfmi->Flags & FMI_FOLDER))
            {
                // Yes; show the item in the unselected colors
                // (dwRop = SRCAND)
                hbrOld = SelectBrush(pdi->hDC, GetSysColorBrush(COLOR_MENUTEXT));
            }
            else
            {
                // No
                SetBkColor(pdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
                SetTextColor(pdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                hbrOld = SelectBrush(pdi->hDC, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
            }

            // REVIEW HACK NB - keep track of the last selected item.
            // NB The keyboard handler needs to know about all selections
            // but the WM_COMMAND stuff only cares about non-folders.
            g_pfmiLastSel = pfmi;
            if (!(pfmi->Flags & FMI_FOLDER))
                g_pfmiLastSelNonFolder = pfmi;
            // Get the rect of the item in screen coords.
            g_rcItem = pdi->rcItem;
            MapWindowPoints(WindowFromDC(pdi->hDC), NULL, (LPPOINT)&g_rcItem, 2);
        }
        else
        {
            // dwRop = SRCAND;
            hbrOld = SelectBrush(pdi->hDC, GetSysColorBrush(COLOR_MENUTEXT));
        }

        // Initial start pos.
        x = pdi->rcItem.left+CXIMAGEGAP;

        // Draw the background image.
        if (pfmh->hbmp)
        {
            // Draw it the first time the first item paints.
            if (pfmi == DPA_GetPtr(pfmh->hdpa, 0) &&
                (pdi->itemAction & ODA_DRAWENTIRE))
            {
                if (!g_hdcMem)
                {
                    g_hdcMem = CreateCompatibleDC(pdi->hDC);
                    ASSERT(g_hdcMem);
                }
                if (g_hdcMem)
                {
                    HBITMAP hbmOld;

                    if (!pfmh->yBmp)
                    {
                        GetClipBox(pdi->hDC, &rcClip);
                        pfmh->yBmp = rcClip.bottom;
                    }
                    hbmOld = SelectBitmap(g_hdcMem, pfmh->hbmp);
                    BitBlt(pdi->hDC, 0, pfmh->yBmp-pfmh->cyBmp, pfmh->cxBmp, pfmh->cyBmp, g_hdcMem, 0, 0, SRCCOPY);
                    SelectBitmap(g_hdcMem, hbmOld);
                }
            }
            x += pfmh->cxBmpGap;
        }

        // Background color for when the bitmap runs out.
        if ((pfmh->clrBkg != (COLORREF)-1) &&
            (pfmi == DPA_GetPtr(pfmh->hdpa, 0)) &&
            (pdi->itemAction & ODA_DRAWENTIRE))
        {
            HBRUSH hbr;

            if (!pfmh->yBmp)
            {
                GetClipBox(pdi->hDC, &rcClip);
                pfmh->yBmp = rcClip.bottom;
            }
            rcBkg.top = 0;
            rcBkg.left = 0;
            rcBkg.bottom = pfmh->yBmp - pfmh->cyBmp;
            rcBkg.right = max(pfmh->cxBmp, pfmh->cxBmpGap);
            hbr = CreateSolidBrush(pfmh->clrBkg);
            FillRect(pdi->hDC, &rcBkg, hbr);
            DeleteObject(hbr);
        }

        // Special case the separator.
        if (pfmi->Flags & FMI_SEPARATOR)
        {
            // With no background image it goes all the way across otherwise
            // it stops in line with the bitmap.
            if (pfmh->hbmp)
                pdi->rcItem.left += pfmh->cxBmpGap;
            pdi->rcItem.bottom = (pdi->rcItem.top+pdi->rcItem.bottom)/2;
            DrawEdge(pdi->hDC, &pdi->rcItem, EDGE_ETCHED, BF_BOTTOM);
            // Early out.
            goto ExitProc;
        }

        // Have the selection not include the icon to speed up drawing while
        // tracking.
        pdi->rcItem.left += pfmh->cxBmpGap;

        // Get the name.
        FileMenuItem_GetDisplayName(pfmi, szName, ARRAYSIZE(szName));

        // Limit the width of the text?
        if (0 < pfmh->cxMax)
        {
            // Yes
            PathCompactPath(pdi->hDC, szName, pfmh->cxMax);
        }

        // NB Keep a plain copy of the name for testing and accessibility.
        if (!pfmi->psz)
            Sz_AllocCopy(szName, &(pfmi->psz));

        dwExtent = GetItemTextExtent(pdi->hDC, szName);
        y = (pdi->rcItem.bottom+pdi->rcItem.top-HIWORD(dwExtent))/2;
        // Support custom heights for the selection rectangle.
        if (pfmh->cySel)
        {
            cyItem = pdi->rcItem.bottom-pdi->rcItem.top;
            // Is there room?
            if ((cyItem > pfmh->cySel) && (pfmh->cySel > HIWORD(dwExtent)))
            {
                dyItem = (cyItem-pfmh->cySel)/2;
                pdi->rcItem.top += dyItem ;
                pdi->rcItem.bottom -= dyItem;
            }
        }
        else if(!(pfmh->fmf & FMF_LARGEICONS))
        {
            // Shrink the selection rect for small icons a bit.
            pdi->rcItem.top += 1;
            pdi->rcItem.bottom -= 1;
        }


        // Draw the text.

        int fDSFlags;

        if (pfmi->Flags & FMI_IGNORE_PIDL)
        {
            //
            // If the string is not coming from a pidl,
            // we can format the menu text.
            //
            fDSFlags = DST_PREFIXTEXT;
        }
        else if ((pfmi->Flags & FMI_ON_MENU) == 0)
        {
            //
            // Norton Desktop Navigator 95 replaces the Start->&Run
            // menu item with a &Run pidl.  Even though the text is
            // from a pidl, we still want to format the "&R" correctly.
            //
            fDSFlags = DST_PREFIXTEXT;
        }
        else
        {
            //
            // All other strings coming from pidls are displayed
            // as is to preserve any & in their display name.
            //
            fDSFlags = DST_TEXT;
        }

        if ((pfmi->Flags & FMI_EMPTY) || (pfmi->Flags & FMI_DISABLED))
        {
            if (pdi->itemState & ODS_SELECTED)
            {
                if (GetSysColor(COLOR_GRAYTEXT) == GetSysColor(COLOR_HIGHLIGHTTEXT))
                {
                    fDSFlags |= DSS_UNION;
                }
                else
                {
                    SetTextColor(pdi->hDC, GetSysColor(COLOR_GRAYTEXT));
                }
            }
            else
            {
                fDSFlags |= DSS_DISABLED;
            }

            ExtTextOut(pdi->hDC, 0, 0, ETO_OPAQUE, &pdi->rcItem, NULL, 0, NULL);
            DrawState(pdi->hDC, NULL, NULL, (LONG_PTR)szName, lstrlen(szName), x+cxIcon+CXIMAGEGAP,
                y, 0, 0, fDSFlags);
        }
        else
        {
            ExtTextOut(pdi->hDC, x+cxIcon+CXIMAGEGAP, y, ETO_OPAQUE, &pdi->rcItem, NULL,
                0, NULL);
            DrawState(pdi->hDC, NULL, NULL, (LONG_PTR)szName, lstrlen(szName), x+cxIcon+CXIMAGEGAP,
                y, 0, 0, fDSFlags);
        }

        // Get the image if it needs it,
        if ((pfmi->iImage == -1) && pfmi->pidl && psf &&
            IsFlagClear(pfmi->Flags, FMI_IGNORE_PIDL))
        {
            pfmi->iImage = SHMapPIDLToSystemImageListIndex(psf, pfmi->pidl, NULL);
        }

        // Draw the image (if there is one).
        if (pfmi->iImage != -1)
        {
            int nDC = 0;

            // Try to center image.
            y = (pdi->rcItem.bottom+pdi->rcItem.top-cyIcon)/2;

            if (pfmh->fmf & FMF_LARGEICONS)
            {
                himl = g_himlIcons;
                // Handle minor drawing glitches that can occur with large icons.
                if ((pdi->itemState & ODS_SELECTED) && (y < pdi->rcItem.top))
                {
                    nDC = SaveDC(pdi->hDC);
                    IntersectClipRect(pdi->hDC, pdi->rcItem.left, pdi->rcItem.top,
                        pdi->rcItem.right, pdi->rcItem.bottom);
                }
            }
            else
            {
                himl = g_himlIconsSmall;
            }

            ImageList_DrawEx(himl, pfmi->iImage, pdi->hDC, x, y, 0, 0,
                GetBkColor(pdi->hDC), CLR_NONE, ILD_NORMAL);

            // Restore the clip rect if we were doing custom clipping.
            if (nDC)
                RestoreDC(pdi->hDC, nDC);
        }

        // Is the user dragging and dropping onto an item that accepts
        // a drop?
        if ((pfmh->fmf & FMF_CANORDER) && 
            (pdi->itemState & ODS_SELECTED) &&
            MenuDD_InEditMode() && 
            (pfmi->dwEffect & g_fsmenuagent.GetDragEffect()))
        {
            // Yes; draw the insertion caret 
            RECT rc = pdi->rcItem;
            POINT pt;

            // We actively draw the insertion caret on mouse moves.
            // When the cursor moves between menu items, the msg hook
            // does not get a mouse move until after this paint.  But
            // we need to update the caret position correctly, so do
            // it here too.
            GetCursorPos(&pt);
            g_fsmenuagent.SetCaretPos(&pt);

            rc.left += 4;
            rc.right -= 8;

            TraceMsg(TF_MENU, "MenuDD:  showing caret %s", MenuDD_InsertAbove() ? TEXT("above") : TEXT("below"));

            if (MenuDD_InsertAbove())
            {
                // Hide any existing caret
                HBRUSH hbrSav = SelectBrush(pdi->hDC, MenuDD_GetBrush());
                PatBlt(pdi->hDC, rc.left, pdi->rcItem.bottom - 2, (rc.right - rc.left), 2, PATCOPY);
                SelectBrush(pdi->hDC, hbrSav);
                
                // Show caret in new position
                PatBlt(pdi->hDC, rc.left, pdi->rcItem.top, (rc.right - rc.left), 2, BLACKNESS);
            }
            else
            {
                // Hide any existing caret
                HBRUSH hbrSav = SelectBrush(pdi->hDC, MenuDD_GetBrush());
                PatBlt(pdi->hDC, rc.left, pdi->rcItem.top, (rc.right - rc.left), 2, PATCOPY);
                SelectBrush(pdi->hDC, hbrSav);
                
                // Show caret in new position
                PatBlt(pdi->hDC, rc.left, pdi->rcItem.bottom - 2, (rc.right - rc.left), 2, BLACKNESS);
            }
        }
    }

ExitProc:
    // Cleanup.
    if (hbrOld)
        SelectObject(pdi->hDC, hbrOld);

    return TRUE;
}


DWORD FileMenuItem_GetExtent(PFILEMENUITEM pfmi)
{
    DWORD dwExtent = 0;

    if (pfmi)
    {
        if (pfmi->Flags & FMI_SEPARATOR)
        {
            dwExtent = MAKELONG(0, GetSystemMetrics(SM_CYMENUSIZE)/2);
        }
        else
        {
            PFILEMENUHEADER pfmh = pfmi->pfmh;

            ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

            if (!g_hdcMem)
            {
                g_hdcMem = CreateCompatibleDC(NULL);
                ASSERT(g_hdcMem);
            }
            if (g_hdcMem)
            {
                // Get the rough height of an item so we can work out when to break the
                // menu. User should really do this for us but that would be useful.
                if (!g_hfont)
                {
                    NONCLIENTMETRICS ncm;
                    ncm.cbSize = SIZEOF(ncm);
                    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, SIZEOF(ncm), &ncm, FALSE))
                    {
                        g_hfont = CreateFontIndirect(&ncm.lfMenuFont);
                        ASSERT(g_hfont);
                    }
                }

                if (g_hfont)
                {
                    HFONT hfontOld = SelectFont(g_hdcMem, g_hfont);
                    dwExtent = GetItemExtent(g_hdcMem, pfmi);
                    SelectFont(g_hdcMem, hfontOld);
                    // NB We hang on to the font, it'll get stomped by
                    // FM_TPME on the way out.
                }
                // NB We hang on to the DC, it'll get stomped by FM_TPME on the way out.
            }
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "FileMenu_GetExtent: Filemenu is invalid.");
    }

    return dwExtent;
}


LRESULT FileMenu_MeasureItem(HWND hwnd, MEASUREITEMSTRUCT *lpmi)
{
    DWORD dwExtent = FileMenuItem_GetExtent((PFILEMENUITEM)lpmi->itemData);
    lpmi->itemHeight = HIWORD(dwExtent);
    lpmi->itemWidth = LOWORD(dwExtent);

    return TRUE;
}


STDAPI_(DWORD)
FileMenu_GetItemExtent(HMENU hmenu, UINT iItem)
{
    DWORD dwRet = 0;
    PFILEMENUHEADER pfmh = FileMenu_GetHeader(hmenu);

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

    if (pfmh)
        dwRet = FileMenuItem_GetExtent((PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, iItem));

    return dwRet;
}

//----------------------------------------------------------------------------
STDAPI_(HMENU)
FileMenu_FindSubMenuByPidl(HMENU hmenu, LPITEMIDLIST pidlFS)
{
    PFILEMENUHEADER pfmh;
    int i;

    if (!pidlFS)
    {
        ASSERT(0);
        return NULL;
    }
    if (ILIsEmpty(pidlFS))
        return hmenu;

    pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        int cItems = DPA_GetPtrCount(pfmh->hdpa);
        for (i = cItems - 1 ; i >= 0; i--)
        {
            // HACK: We directly call this FS function to compare two pidls.
            // For all items, see if it's the one we're looking for.
            PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, i);

            if (pfmi && pfmi->pidl && IsFlagClear(pfmi->Flags, FMI_IGNORE_PIDL) &&
                0 == pfmh->psf->CompareIDs(0, pidlFS, pfmi->pidl))
            {
                HMENU hmenuSub;

                if ((pfmi->Flags & FMI_FOLDER) &&
                    (NULL != (hmenuSub = GetSubMenu(hmenu, i))))
                {
                    // recurse to find the next sub menu
                    return FileMenu_FindSubMenuByPidl(hmenuSub, (LPITEMIDLIST)ILGetNext(pidlFS));

                }
                else
                {
                    ASSERT(0); // we're screwed.
                    break;
                }
            }
        }
    }
    return NULL;
}


/*----------------------------------------------------------
Purpose: Fills the given filemenu with contents of the appropriate
         directory.

Returns: S_OK if all the files were added
         S_FALSE if some did not get added (reached cyMax)
         error on something bad
Cond:    --
*/
STDAPI
FileMenu_InitMenuPopupEx(
    IN     HMENU   hmenu,
    IN OUT PFMDATA pfmdata)
{
    HRESULT hres = E_INVALIDARG;
    PFILEMENUITEM pfmi;
    PFILEMENUHEADER pfmh;

    ASSERT(IS_VALID_HANDLE(hmenu, MENU));

    if (IS_VALID_WRITE_PTR(pfmdata, FMDATA) &&
        SIZEOF(*pfmdata) == pfmdata->cbSize)
    {
        hres = E_FAIL;      // assume error

        g_fAbortInitMenu = FALSE;

        // Is this a filemenu?
        pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
        if (pfmi)
        {
            ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

            pfmh = pfmi->pfmh;
            if (pfmh)
            {
                // Yes
                if (IsFlagSet(pfmh->fmf, FMF_DELAY_INVALID))
                {
                    FileMenu_Invalidate(hmenu);
                    ClearFlag(pfmh->fmf, FMF_DELAY_INVALID);
                }
                
                // BUGBUG (scotth): this can return S_OK but not
                // set the cItems field if this menu has already
                // been filled out.

                hres = S_OK;

                // Have we already filled this thing out?
                if (IsFlagSet(pfmi->Flags, FMI_MARKER | FMI_EXPAND))
                {
                    // No, do it now.  Get the previously init'ed header.
                    FileMenuHeader_DeleteMarkerItem(pfmh);

                    // Fill it full of stuff.
                    hres = FileMenuHeader_AddFiles(pfmh, 0, 0, &pfmdata->cItems);
                    if (E_ABORT == hres)
                    {
                        // Aborted - put the marker back.
                        FileMenuHeader_InsertMarkerItem(pfmh);
                    }
                    else if (pfmh->pidlAltFolder) 
                    {
                        pfmh->hdpaAlt = DPA_Create(0);

                        if (pfmh->hdpaAlt) 
                        {
                            int cItems;

                            if (E_ABORT == FileMenuHeader_AddFiles(pfmh, 0,
                                           FMHAF_SEPARATOR | FMHAF_USEALT,
                                           &cItems))
                            {
                               // Aborted - put the marker back.
                               FileMenuHeader_InsertMarkerItem(pfmh);
                            }

                            DPA_Destroy (pfmh->hdpaAlt);
                            pfmh->hdpaAlt = NULL;
                        }
                    }
                }
            }
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Fills the given filemenu with contents of the appropriate
         directory.

Returns: FALSE if the given menu isn't a filemenu
Cond:    --
*/
STDAPI_(BOOL)
FileMenu_InitMenuPopup(
    IN HMENU hmenu)
{
    FMDATA fmdata = {SIZEOF(fmdata)};   // zero init everything else

    return SUCCEEDED(FileMenu_InitMenuPopupEx(hmenu, &fmdata));
}


BOOL FileMenu_IsUnexpanded(HMENU hmenu)
{
    BOOL fRet = FALSE;
    PFILEMENUITEM pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);

    ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

    if (pfmi)
    {
        if ((pfmi->Flags & FMI_MARKER) && (pfmi->Flags & FMI_EXPAND))
            fRet = TRUE;
    }

    return fRet;
}


//---------------------------------------------------------------------------
// This sets whether to load all the images while creating the menu or to
// defer it until the menu is actually being drawn.
STDAPI_(void)
FileMenu_AbortInitMenu(void)
{
    g_fAbortInitMenu = TRUE;
}


/*----------------------------------------------------------
Purpose: Returns a clone of the last selected pidl

Returns: 
Cond:    --
*/
STDAPI_(BOOL)
FileMenu_GetLastSelectedItemPidls(
    IN  HMENU          hmenu, 
    OUT LPITEMIDLIST * ppidlFolder,         OPTIONAL
    OUT LPITEMIDLIST * ppidlItem)           OPTIONAL
{
    BOOL bRet    = FALSE;
    LPITEMIDLIST pidlFolder = NULL;
    LPITEMIDLIST pidlItem = NULL;

    // BUGBUG (scotth): this global should be moved into the 
    //  instance data of the header.
    if (g_pfmiLastSelNonFolder)
    {
        // Get to the header.
        PFILEMENUHEADER pfmh = g_pfmiLastSelNonFolder->pfmh;
        if (pfmh)
        {
            ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
            
            bRet = TRUE;

            if (ppidlFolder)
            {
                if (g_pfmiLastSelNonFolder->Flags & FMI_ALTITEM)
                    pidlFolder = ILClone(pfmh->pidlAltFolder);
                else
                    pidlFolder = ILClone(pfmh->pidlFolder);
                bRet = (NULL != pidlFolder);
            }

            if (bRet && ppidlItem)
            {
                if (g_pfmiLastSelNonFolder->pidl)
                {
                    pidlItem = ILClone(g_pfmiLastSelNonFolder->pidl);
                    bRet = (NULL != pidlItem);
                }
                else
                    bRet = FALSE;
            }

            if (!bRet)
            {
                if (pidlFolder)
                {
                    // Failed; free the pidl we just allocated
                    ILFree(pidlFolder);
                    pidlFolder = NULL;
                }
            }
        }
    }

    // Init because callers get lazy and don't pay attention to the return
    // value.
    if (ppidlFolder)
        *ppidlFolder = pidlFolder;
    if (ppidlItem)
        *ppidlItem = pidlItem;
    
    if (!bRet)
        TraceMsg(TF_WARNING, "No previously selected item.");

    return bRet;
}


/*----------------------------------------------------------
Purpose: Returns the command ID and hmenu of the last selected
         menu item.  The given hmenuRoot is the parent hmenu
         that must be a FileMenu.

Returns: S_OK 
         S_FALSE if there was no last selected item
Cond:    --
*/
STDAPI
FileMenu_GetLastSelectedItem(
    IN  HMENU   hmenu, 
    OUT HMENU * phmenu,         OPTIONAL
    OUT UINT *  puItem)         OPTIONAL
{
    HRESULT hres = S_FALSE;

    if (phmenu)
        *phmenu = NULL;
    if (puItem)
        *puItem = 0;

    if (g_pfmiLastSelNonFolder)
    {
        // Get to the header.
        PFILEMENUHEADER pfmh = g_pfmiLastSelNonFolder->pfmh;
        if (pfmh)
        {
            ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

            if (phmenu)
                *phmenu = pfmh->hmenu;

            if (puItem)
            {
                // BUGBUG (scotth): this isn't stored right now
                ASSERT(0);
            }
            hres = S_OK;
        }
    }

    return hres;
}


#define AnsiUpperChar(c) ( (TCHAR)LOWORD((DWORD_PTR)CharUpper((LPTSTR)(DWORD)MAKELONG((DWORD) c, 0))) )


int FileMenuHeader_LastSelIndex(PFILEMENUHEADER pfmh)
{
    int i;
    PFILEMENUITEM pfmi;

    for (i = GetMenuItemCount(pfmh->hmenu)-1;i >= 0; i--)
    {
        pfmi = FileMenu_GetItemData(pfmh->hmenu, i, TRUE);
        if (pfmi && (pfmi == g_pfmiLastSel))
            return i;
    }
    return -1;
}


//---------------------------------------------------------------------------
// If the string contains &ch or begins with ch then return TRUE.
BOOL _MenuCharMatch(LPCTSTR lpsz, TCHAR ch, BOOL fIgnoreAmpersand)
{
    LPTSTR pchAS;

    // Find the first ampersand.
    pchAS = StrChr(lpsz, TEXT('&'));
    if (pchAS && !fIgnoreAmpersand)
    {
        // Yep, is the next char the one we want.
        if (AnsiUpperChar(*CharNext(pchAS)) == AnsiUpperChar(ch))
        {
            // Yep.
            return TRUE;
        }
    }
    else if (AnsiUpperChar(*lpsz) == AnsiUpperChar(ch))
    {
        return TRUE;
    }

    return FALSE;
}


STDAPI_(LRESULT)
FileMenu_HandleMenuChar(HMENU hmenu, TCHAR ch)
{
    UINT iItem, cItems, iStep;
    PFILEMENUITEM pfmi;
    int iFoundOne;
    TCHAR szName[MAX_PATH];
    PFILEMENUHEADER pfmh;

    iFoundOne = -1;
    iStep = 0;
    iItem = 0;
    cItems = GetMenuItemCount(hmenu);

    // Start from the last place we looked from.
    pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        iItem = FileMenuHeader_LastSelIndex(pfmh) + 1;
        if (iItem >= cItems)
            iItem = 0;
    }

    while (iStep < cItems)
    {
        pfmi = FileMenu_GetItemData(hmenu, iItem, TRUE);
        if (pfmi)
        {
            BOOL bIgnoreAmpersand = (pfmi->pidl && IsFlagClear(pfmi->Flags, FMI_IGNORE_PIDL));

            FileMenuItem_GetDisplayName(pfmi, szName, ARRAYSIZE(szName));
            if (_MenuCharMatch(szName, ch, bIgnoreAmpersand))
            {
                // Found (another) match.
                if (iFoundOne != -1)
                {
                    // More than one, select the first.
                    return MAKELRESULT(iFoundOne, MNC_SELECT);
                }
                else
                {
                    // Found at least one.
                    iFoundOne = iItem;
                }
            }

        }
        iItem++;
        iStep++;
        // Wrap.
        if (iItem >= cItems)
            iItem = 0;
    }

    // Did we find one?
    if (iFoundOne != -1)
    {
        // Just in case the user types ahead without the selection being drawn.
        pfmi = FileMenu_GetItemData(hmenu, iFoundOne, TRUE);
        if (!(pfmi->Flags & FMI_FOLDER))
            g_pfmiLastSelNonFolder = pfmi;

        return MAKELRESULT(iFoundOne, MNC_EXECUTE);
    }
    else
    {
        // Didn't find it.
        return MAKELRESULT(0, MNC_IGNORE);
    }
}


/*----------------------------------------------------------
Purpose: Create a filemenu from a given normal menu

Returns:
Cond:    --
*/
STDAPI_(BOOL)
FileMenu_CreateFromMenu(
    IN HMENU    hmenu,
    IN COLORREF clr,
    IN int      cxBmpGap,
    IN HBITMAP  hbmp,
    IN int      cySel,
    IN DWORD    fmf)
{
    BOOL fRet = FALSE;

    if (hmenu)
    {
        PFILEMENUHEADER pfmh = FileMenuHeader_Create(hmenu, hbmp, cxBmpGap, clr, cySel, NULL);

        if (!g_himlIcons || !g_himlIconsSmall)
            Shell_GetImageLists(&g_himlIcons, &g_himlIconsSmall);

        if (pfmh)
        {
            // Default flags.
            pfmh->fmf = fmf;
            if (FileMenuHeader_InsertMarkerItem(pfmh))
                fRet = TRUE;
            else
            {
                // BUGBUG (scotth): FileMenuHeader_Create can return a pointer
                //  that is already stored in a filemenu item, in which case this
                //  destroy will stomp a data structure.
                TraceMsg(TF_ERROR, "Can't create file menu.");
                FileMenuHeader_Destroy(pfmh);
            }
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "Menu is null.");
    }

    return fRet;
}


HMENU  FileMenu_Create(COLORREF clr, int cxBmpGap, HBITMAP hbmp, int cySel, DWORD fmf)
{
    HMENU hmenuRet = NULL;
    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        if (FileMenu_CreateFromMenu(hmenu, clr, cxBmpGap, hbmp, cySel, fmf))
            hmenuRet = hmenu;
        else
            DestroyMenu(hmenu);
    }

    return hmenuRet;
}


/*----------------------------------------------------------
Purpose: Insert a generic item into a filemenu

Returns:
Cond:    --
*/
STDAPI
FileMenu_InsertItemEx(
    IN HMENU          hmenu,
    IN UINT           iPos,
    IN FMITEM const * pfmitem)
{
    HRESULT hres = E_INVALIDARG;
    PFILEMENUITEM pfmi;
    FMITEM fmitem;

    // Is this a filemenu?
    pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);

    if (IsValidFMItem(pfmitem, &fmitem) && pfmi)
    {
        // Yes
        PFILEMENUHEADER pfmh = pfmi->pfmh;

        ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));

        // Have we cleaned up the marker item?
        if ((pfmi->Flags & FMI_MARKER) && (pfmi->Flags & FMI_EXPAND))
        {
            // Nope, do it now.
            FileMenuHeader_DeleteMarkerItem(pfmh);
        }

        hres = E_OUTOFMEMORY;

        // Add the new item.
        if (FileMenuItem_Create(pfmh, NULL, fmitem.iImage, 0, &pfmi))
        {
            if (fmitem.pvData && IsFlagSet(fmitem.dwType, FMIT_STRING))
            {
                if (!Sz_AllocCopy((LPTSTR)fmitem.pvData, &(pfmi->psz)))
                    TraceMsg(TF_ERROR, "Unable to allocate menu item text.");
                pfmi->Flags |= FMI_IGNORE_PIDL;
            }
            pfmi->cyItem = fmitem.cyItem;
            pfmi->lParam = fmitem.lParam;
            DPA_InsertPtr(pfmh->hdpa, iPos, pfmi);

            if (IsFlagSet(fmitem.dwType, FMIT_SEPARATOR))
            {
                // Override the setting made above, since separator and
                // text are mutually exclusive
                pfmi->Flags = FMI_SEPARATOR;
                InsertMenu(hmenu, iPos, MF_BYPOSITION|MF_OWNERDRAW|MF_DISABLED|MF_SEPARATOR, 
                           fmitem.uID, (LPTSTR)pfmi);
            }
            else if (fmitem.hmenuSub)
            {
                MENUITEMINFO mii;

                pfmi->Flags |= FMI_FOLDER;
                if ((iPos == 0xffff) || (iPos == 0xffffffff))
                    iPos = GetMenuItemCount(pfmh->hmenu);

                InsertMenu(pfmh->hmenu, iPos, MF_BYPOSITION|MF_OWNERDRAW|MF_POPUP, 
                           (UINT_PTR)fmitem.hmenuSub, (LPTSTR)pfmi);
                // Set it's ID.
                mii.cbSize = SIZEOF(mii);
                mii.fMask = MIIM_ID;
                // mii.wID = pfmh->idCmd;
                mii.wID = fmitem.uID;
                SetMenuItemInfo(pfmh->hmenu, iPos, TRUE, &mii);
            }
            else
            {
                InsertMenu(hmenu, iPos, MF_BYPOSITION|MF_OWNERDRAW, 
                           fmitem.uID, (LPTSTR)pfmi);
            }

            hres = S_OK;
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Old function to insert a generic item onto a filemenu

Returns:
Cond:    --
*/
STDAPI_(BOOL)
FileMenu_InsertItem(
    IN HMENU  hmenu,
    IN LPTSTR psz,
    IN UINT   id,
    IN int    iImage,
    IN HMENU  hmenuSub,
    IN UINT   cyItem,
    IN UINT   iPos)
{
    FMITEM fmitem;

    fmitem.cbSize = SIZEOF(fmitem);
    fmitem.dwMask = FMI_TYPE | FMI_ID | FMI_IMAGE | FMI_HMENU |
                    FMI_METRICS;

    if ((LPTSTR)FMAI_SEPARATOR == psz)
    {
        fmitem.dwType = FMIT_SEPARATOR;
    }
    else if (NULL == psz)
    {
        fmitem.dwType = 0;
    }
    else
    {
        fmitem.dwType = FMIT_STRING;
#ifdef UNICODE
        fmitem.dwType |= FMIT_UNICODE;
#endif

        fmitem.dwMask |= FMI_DATA;
        fmitem.pvData = psz;
    }

    fmitem.uID = id;
    fmitem.iImage = iImage;
    fmitem.hmenuSub = hmenuSub;
    fmitem.cyItem = cyItem;

    return SUCCEEDED(FileMenu_InsertItemEx(hmenu, iPos, &fmitem));
}


/*----------------------------------------------------------
Purpose: Get info about this filemenu item.

*/
STDAPI
FileMenu_GetItemInfo(
    IN  HMENU   hmenu,
    IN  UINT    uItem,
    IN  BOOL    bByPos,
    OUT PFMITEM pfmitem)
{
    HRESULT hres = E_INVALIDARG;

    if (IS_VALID_WRITE_PTR(pfmitem, FMITEM) &&
        SIZEOF(*pfmitem) == pfmitem->cbSize)
    {
        PFILEMENUITEM pfmi;

        hres = E_FAIL;

        pfmi = FileMenu_GetItemData(hmenu, uItem, bByPos);
        if (pfmi)
        {
            // BUGBUG (scotth): we don't fill in all the fields
            
            if (IsFlagSet(pfmitem->dwMask, FMI_LPARAM))
                pfmitem->lParam = pfmi->lParam;
            
            hres = S_OK;
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Save the order of the menu to the given stream.

*/
STDAPI
FileMenu_SaveOrder(HMENU hmenu)
{
    HRESULT hres = E_FAIL;
    PFILEMENUITEM pfmi;
    IStream * pstm;

    pfmi = FileMenu_GetItemData(hmenu, 0, TRUE);
    if (pfmi && FileList_GetStream(pfmi->pfmh, &pstm))
    {
        hres = FileList_Save(pfmi->pfmh, pstm);
        pstm->Release();
    }

    return hres;
}


STDAPI_(BOOL)
FileMenu_AppendItem(HMENU hmenu, LPTSTR psz, UINT id, int iImage,
    HMENU hmenuSub, UINT cyItem)
{
    return FileMenu_InsertItem(hmenu, psz, id, iImage, hmenuSub, cyItem, 0xffff);
}


STDAPI_(BOOL)
FileMenu_TrackPopupMenuEx(HMENU hmenu, UINT Flags, int x, int y,
    HWND hwndOwner, LPTPMPARAMS lpTpm)
{
    BOOL fRet = TrackPopupMenuEx(hmenu, Flags, x, y, hwndOwner, lpTpm);
    // Cleanup.

    DeleteGlobalMemDCAndFont();

    return fRet;
}


//----------------------------------------------------------------------------
// Like Users only this works on submenu's too.
// NB Returns 0 for seperators.
UINT FileMenu_GetMenuItemID(HMENU hmenu, UINT iItem)
{
    MENUITEMINFO mii;

    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_ID;
    mii.cch = 0;     // just in case

    if (GetMenuItemInfo(hmenu, iItem, TRUE, &mii))
        return mii.wID;

    return 0;
}


PFILEMENUITEM  _FindItemByCmd(PFILEMENUHEADER pfmh, UINT id, int *piPos)
{
    if (pfmh)
    {
        int cItems, i;

        cItems = DPA_GetPtrCount(pfmh->hdpa);
        for (i = 0; i < cItems; i++)
        {
            PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, i);
            if (pfmi)
            {
                // Is this the right item?
                // NB This ignores menu items.
                if (id == GetMenuItemID(pfmh->hmenu, i))
                {
                    // Yep.
                    if (piPos)
                        *piPos = i;
                    return pfmi;
                }
            }
        }
    }
    return NULL;
}


PFILEMENUITEM  _FindMenuOrItemByCmd(PFILEMENUHEADER pfmh, UINT id, int *piPos)
{
    if (pfmh)
    {
        int cItems, i;

        cItems = DPA_GetPtrCount(pfmh->hdpa);
        for (i = 0; i < cItems; i++)
        {
            PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, i);
            if (pfmi)
            {
                // Is this the right item?
                // NB This includes menu items.
                if (id == FileMenu_GetMenuItemID(pfmh->hmenu, i))
                {
                    // Yep.
                    if (piPos)
                        *piPos = i;
                    return pfmi;
                }
            }
        }
    }
    return NULL;
}


//----------------------------------------------------------------------------
// NB This deletes regular items or submenus.
STDAPI_(BOOL)
FileMenu_DeleteItemByCmd(HMENU hmenu, UINT id)
{
    PFILEMENUHEADER pfmh;

    if (!IsMenu(hmenu))
        return FALSE;

    if (!id)
        return FALSE;

    pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        int i;
        PFILEMENUITEM pfmi = _FindMenuOrItemByCmd(pfmh, id, &i);
        if (pfmi)
        {
            // If it's a submenu, delete it's items first.
            HMENU hmenuSub = GetSubMenu(pfmh->hmenu, i);
            if (hmenuSub)
                FileMenu_DeleteAllItems(hmenuSub);
            // Delete the item itself.
            DeleteMenu(pfmh->hmenu, i, MF_BYPOSITION);
            FileMenuItem_Destroy(pfmi);
            DPA_DeletePtr(pfmh->hdpa, i);
            return TRUE;
        }
    }
    return FALSE;
}


STDAPI_(BOOL)
FileMenu_DeleteItemByIndex(HMENU hmenu, UINT iItem)
{
    PFILEMENUHEADER pfmh;

    if (!IsMenu(hmenu))
        return FALSE;

    pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, iItem);
        if (pfmi)
        {
            // Delete the item itself.
            DeleteMenu(pfmh->hmenu, iItem, MF_BYPOSITION);
            FileMenuItem_Destroy(pfmi);
            DPA_DeletePtr(pfmh->hdpa, iItem);
            return TRUE;
        }
    }
    return FALSE;
}


//---------------------------------------------------------------------------
// Search for the first sub menu of the given menu, who's first item's ID
// is id. Returns NULL, if nothing is found.
HMENU _FindMenuItemByFirstID(HMENU hmenu, UINT id, int *pi)
{
    int cMax, c;
    MENUITEMINFO mii;

    ASSERT(hmenu);

    // Search all items.
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_ID;
    mii.cch = 0;     // just in case

    cMax = GetMenuItemCount(hmenu);
    for (c=0; c<cMax; c++)
    {
        // Is this item a submenu?
        HMENU hmenuSub = GetSubMenu(hmenu, c);
        if (hmenuSub && GetMenuItemInfo(hmenuSub, 0, TRUE, &mii))
        {
            if (mii.wID == id)
            {
                // Found it!
                if (pi)
                    *pi = c;
                return hmenuSub;
            }
        }
    }

    return NULL;
}


STDAPI_(BOOL)
FileMenu_DeleteMenuItemByFirstID(HMENU hmenu, UINT id)
{
    int i;
    PFILEMENUITEM pfmi;
    PFILEMENUHEADER pfmh;
    HMENU hmenuSub;

    if (!IsMenu(hmenu))
        return FALSE;

    if (!id)
        return FALSE;

    pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        hmenuSub = _FindMenuItemByFirstID(hmenu, id, &i);
        if (hmenuSub && i)
        {
            // Delete the submenu.
            FileMenu_DeleteAllItems(hmenuSub);
            // Delete the item itself.
            pfmi = FileMenu_GetItemData(hmenu, i, TRUE);
            DeleteMenu(pfmh->hmenu, i, MF_BYPOSITION);
            FileMenuItem_Destroy(pfmi);
            DPA_DeletePtr(pfmh->hdpa, i);
            return TRUE;
        }
    }
    return FALSE;
}


STDAPI_(BOOL)
FileMenu_DeleteSeparator(HMENU hmenu)
{
    int i;
    PFILEMENUHEADER pfmh;

    if (!IsMenu(hmenu))
        return FALSE;

    pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        PFILEMENUITEM pfmi = _FindItemByCmd(pfmh, 0, &i);
        if (pfmi)
        {
            // Yep.
            DeleteMenu(pfmh->hmenu, i, MF_BYPOSITION);
            if (pfmi->pidl)
                ILFree(pfmi->pidl);
            LocalFree((HLOCAL)pfmi);
            DPA_DeletePtr(pfmh->hdpa, i);
            return TRUE;
        }
    }
    return FALSE;
}


STDAPI_(BOOL)
FileMenu_InsertSeparator(HMENU hmenu, UINT iPos)
{
    return FileMenu_InsertItem(hmenu, (LPTSTR)FMAI_SEPARATOR, 0, -1, NULL, 0, iPos);
}


STDAPI_(BOOL)
FileMenu_IsFileMenu(HMENU hmenu)
{
    return FileMenu_GetHeader(hmenu) ? TRUE : FALSE;
}


STDAPI_(BOOL)
FileMenu_EnableItemByCmd(HMENU hmenu, UINT id, BOOL fEnable)
{
    PFILEMENUHEADER pfmh;

    if (!IsMenu(hmenu))
        return FALSE;

    if (!id)
        return FALSE;

    pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        PFILEMENUITEM pfmi = _FindItemByCmd(pfmh, id, NULL);
        if (pfmi)
        {
            if (fEnable)
            {
                pfmi->Flags &= ~FMI_DISABLED;
                EnableMenuItem(pfmh->hmenu, id, MF_BYCOMMAND | MF_ENABLED);
            }
            else
            {
                pfmi->Flags |= FMI_DISABLED;
                EnableMenuItem(pfmh->hmenu, id, MF_BYCOMMAND | MF_GRAYED);
            }
            return TRUE;
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "Menu is not a filemenu.");
    }

    return FALSE;
}


STDAPI_(BOOL)
FileMenu_GetPidl(HMENU hmenu, UINT iPos, LPITEMIDLIST *ppidl)
{
    BOOL fRet = FALSE;
    PFILEMENUHEADER pfmh = FileMenu_GetHeader(hmenu);
    if (pfmh)
    {
        PFILEMENUITEM pfmi = (PFILEMENUITEM)DPA_GetPtr(pfmh->hdpa, iPos);
        if (pfmi)
        {
            if (pfmh->pidlFolder && pfmi->pidl && 
                IsFlagClear(pfmi->Flags, FMI_IGNORE_PIDL))
            {
                *ppidl = ILCombine(pfmh->pidlFolder, pfmi->pidl);
                fRet = TRUE;
            }
        }
    }

    return fRet;
}


BOOL Tooltip_Create(HWND *phwndTip)
{
    BOOL fRet = FALSE;

    *phwndTip = CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, HINST_THISDLL, NULL);
    if (*phwndTip)
    {
        TOOLINFO ti;

        ti.cbSize = SIZEOF(ti);
        ti.uFlags = TTF_TRACK;
        ti.hwnd = NULL;
        ti.uId = 0;
        ti.lpszText = NULL;
        ti.hinst = HINST_THISDLL;
        SetRectEmpty(&ti.rect);
        SendMessage(*phwndTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        fRet = TRUE;
    }

    return fRet;
}


void Tooltip_SetText(HWND hwndTip, LPCTSTR pszText)
{
    if (hwndTip)
    {
        TOOLINFO ti;
        ti.cbSize = SIZEOF(ti);
        ti.uFlags = 0;
        ti.hwnd = NULL;
        ti.uId = 0;
        ti.lpszText = (LPTSTR)pszText;
        ti.hinst = HINST_THISDLL;
        SendMessage(hwndTip, TTM_UPDATETIPTEXT, 0, (LPARAM)(LPTOOLINFO)&ti);
    }
}


void Tooltip_Hide(HWND hwndTip)
{
    if (hwndTip)
    {
        TOOLINFO ti;
        ti.cbSize = SIZEOF(ti);
        ti.hwnd = NULL;
        ti.uId = 0;
        SendMessage(hwndTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
    }
}


void Tooltip_Show(HWND hwndTip)
{
    if (hwndTip)
    {
        TOOLINFO ti;
        ti.cbSize = SIZEOF(ti);
        ti.hwnd = NULL;
        ti.uId = 0;
        SendMessage(hwndTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
        SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    }
}


void Tooltip_SetPos(HWND hwndTip, int x, int y)
{
    ASSERT(IsWindow(hwndTip));

    SendMessage(hwndTip, TTM_TRACKPOSITION, 0, MAKELPARAM(x, y));
}


/*----------------------------------------------------------
Purpose: Ask the callback for a tooltip.

Returns:
Cond:    --
*/
void
FileMenuItem_GetTooltip(
    IN PFILEMENUITEM pfmi)
{
    ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

    PFILEMENUHEADER pfmh = pfmi->pfmh;

    if (pfmh->pfncb)
    {
        FMCBDATA fmcbdata;
        FMTOOLTIP fmtt = {0};

        if (pfmi->pszTooltip)
        {
            // Free the previous tooltip
            LocalFree(pfmi->pszTooltip);
            pfmi->pszTooltip = NULL;
        }

        fmcbdata.hmenu = pfmh->hmenu;
        fmcbdata.iPos = -1;
        fmcbdata.idCmd = (UINT)-1;

        // BUGBUG (scotth): we don't ask for tooltips for alternate lists
        fmcbdata.pidlFolder = pfmh->pidlFolder;
        fmcbdata.pidl = pfmi->pidl;
        fmcbdata.psf = pfmh->psf;

        // Was a tooltip set?
        if (S_OK == pfmh->pfncb(FMM_GETTOOLTIP, &fmcbdata, (LPARAM)&fmtt))
        {
            Sz_AllocCopyW(fmtt.pszTip, &(pfmi->pszTooltip));
            SHFree(fmtt.pszTip);

            if (pfmi->pszTooltip)
            {
                // Set the other settings
                if (IsFlagSet(fmtt.dwMask, FMTT_MARGIN))
                {
                    pfmi->rcMargin = fmtt.rcMargin;
                    SetFlag(pfmi->Flags, FMI_MARGIN);
                }

                if (IsFlagSet(fmtt.dwMask, FMTT_MAXWIDTH))
                {
                    pfmi->dwMaxTipWidth = fmtt.dwMaxWidth;
                    SetFlag(pfmi->Flags, FMI_MAXTIPWIDTH);
                }

                if (IsFlagSet(fmtt.dwMask, FMTT_DRAWFLAGS))
                {
                    pfmi->uDrawFlags = fmtt.uDrawFlags;
                    SetFlag(pfmi->Flags, FMI_DRAWFLAGS);
                }

                if (IsFlagSet(fmtt.dwMask, FMTT_TABSTOP))
                {
                    pfmi->dwTabstop = fmtt.dwTabstop;
                    SetFlag(pfmi->Flags, FMI_TABSTOP);
                }
            }
        }
    }
}


/*----------------------------------------------------------
Purpose: Called on WM_MENUSELECT

*/
STDAPI_(BOOL)
FileMenu_HandleMenuSelect(
    IN HMENU  hmenu,
    IN WPARAM wparam,
    IN LPARAM lparam)
{
    UINT id = LOWORD(wparam);
    BOOL fTip = FALSE;
    BOOL fRet = FALSE;
    PFILEMENUITEM pfmi = g_pfmiLastSelNonFolder;

    if (hmenu && pfmi)
    {
        ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

        PFILEMENUHEADER pfmh = pfmi->pfmh;

        if (pfmh && IsFlagSet(pfmh->fmf, FMF_TOOLTIPS) &&
            FileMenuHeader_CreateTooltipWindow(pfmh))
        {
            // Have we asked for the tooltip?
            if (IsFlagClear(pfmi->Flags, FMI_ASKEDFORTOOLTIP))
            {
                // No; do it now
                FileMenuItem_GetTooltip(pfmi);

                SetFlag(pfmi->Flags, FMI_ASKEDFORTOOLTIP);
            }

            // Does this have a tooltip?
            if (pfmi->pszTooltip && FileMenu_GetHeader(hmenu) == pfmh)
            {
                // Yes
                Tooltip_Hide(g_hwndTip);
                Tooltip_SetPos(g_hwndTip, g_rcItem.left + X_TIPOFFSET, g_rcItem.bottom);
                Tooltip_SetText(g_hwndTip, pfmi->pszTooltip);

                if (IsFlagSet(pfmi->Flags, FMI_MAXTIPWIDTH))
                    SendMessage(g_hwndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)pfmi->dwMaxTipWidth);

                if (IsFlagSet(pfmi->Flags, FMI_MARGIN))
                    SendMessage(g_hwndTip, TTM_SETMARGIN, 0, (LPARAM)&pfmi->rcMargin);

                Tooltip_Show(g_hwndTip);
                fTip = TRUE;
            }
        }
        fRet = TRUE;
    }

    if (!fTip && IsWindow(g_hwndTip))
        Tooltip_Hide(g_hwndTip);

    return fRet;
}


STDAPI_(void)
FileMenu_EditMode(BOOL bEdit)
{
    g_fsmenuagent.SetEditMode(bEdit, DROPEFFECT_MOVE);
}    


/*----------------------------------------------------------
Purpose: 

*/
STDAPI_(BOOL)
FileMenu_ProcessCommand(
    IN HWND   hwnd,
    IN HMENU  hmenuBar,
    IN UINT   idMenu,
    IN HMENU  hmenu,
    IN UINT   idCmd)
{
    return g_fsmenuagent.ProcessCommand(hwnd, hmenuBar, idMenu, hmenu, idCmd);
}


void FileMenuHeader_HandleUpdateImage(PFILEMENUHEADER pfmh, int iImage)
{
    int i;
    PFILEMENUITEM pfmi;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    ASSERT(-1 != iImage);

    // Look for any image indexes that are being changed
    
    for (i = GetMenuItemCount(pfmh->hmenu) - 1; i >= 0; i--)
    {
        pfmi = FileMenu_GetItemData(pfmh->hmenu, i, TRUE);
        if (pfmi)
        {
            ASSERT(IS_VALID_STRUCT_PTR(pfmi, FILEMENUITEM));

            if (pfmi->iImage == iImage)
            {
                // Invalidate this image.  It will be recalculated when
                // the menu item is redrawn.
                pfmi->iImage = -1;
            }

            HMENU hmenuSub = GetSubMenu(pfmh->hmenu, i);
            if (hmenuSub)
            {
                PFILEMENUHEADER pfmhT = FileMenu_GetHeader(hmenuSub);
                if (pfmhT)
                    FileMenuHeader_HandleUpdateImage(pfmhT, iImage);
            }    
        }
    }
}    


BOOL FileMenuHeader_HandleNotify(PFILEMENUHEADER pfmh, LPCITEMIDLIST * ppidl, LONG lEvent)
{
    BOOL bRet;
    int iImage;

    ASSERT(IS_VALID_STRUCT_PTR(pfmh, FILEMENUHEADER));
    
    switch (lEvent)
    {
    case SHCNE_UPDATEIMAGE:
        if (EVAL(ppidl && ppidl[0]))
        {
            iImage = *(int UNALIGNED *)((BYTE *)ppidl[0] + 2);

            if (-1 != iImage)
                FileMenuHeader_HandleUpdateImage(pfmh, iImage);
        }
        bRet = TRUE;
        break;

    default:
        bRet = FALSE;
        break;
    }

    return bRet;
}    


STDAPI_(BOOL)
FileMenu_HandleNotify(HMENU hmenu, LPCITEMIDLIST * ppidl, LONG lEvent)
{
    BOOL bRet = FALSE;
    PFILEMENUHEADER pfmh = FileMenu_GetHeader(hmenu);

    if (hmenu && pfmh)
    {
        bRet = FileMenuHeader_HandleNotify(pfmh, ppidl, lEvent);
    }

    return bRet;
}    
    
