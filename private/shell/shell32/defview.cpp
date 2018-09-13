#include "shellprv.h"
extern "C" {
#include <regstr.h>
#include <shellp.h>
#include <htmlhelp.h>
#include "ole2dup.h"
#include "ids.h"
#include "defview.h"
#include "lvutil.h"
#include "idlcomm.h"
#include "filetbl.h"
#include "undo.h"
#include "vdate.h"
} ;
#include "cnctnpt.h"
#include "mmhelper.h"
#include "ovrlaymn.h"
#include "sfvext.h"
#include "_security.h"
#include "unicpp\dutil.h"
#include "uemapp.h"
#include "unicpp\deskhtm.h"
#include "unicpp\dcomp.h"
#include "datautil.h"
#include "defvphst.h"
#include <shdispid.h>
#include <limits.h>
#include "prop.h"
#include <mshtmcid.h>

// docfindx.cpp
UINT GetControlCharWidth(HWND hwnd);

// An old version of Web View had the notion of customizing the listview
// background image and icon text colors, but we're not using that any
// more. We still allow that to be changed via desktop.ini, but there's
// no global inherited version of this support. The code is under ifdef:
//
//#define CUSTOM_BACKGROUND

#define TF_FOCUS    TF_WARNING

#define DEFAULT_NUMCHARS 20

void DisableActiveDesktop();
extern "C" HMENU CDesktop_GetActiveDesktopMenu(void);

STDAPI_(void) DPA_FreeIDArray(HDPA hdpa);

BOOL StringFromCustomViewData(SFVVIEWSDATA *pItem, LPTSTR pszString, UINT cb, UINT idString);
BOOL ColorFromCustomViewData(SFVVIEWSDATA *pItem, COLORREF * pcr, UINT idCR);
BOOL ShowInfoTip();

//
// define MAX_ICON_WAIT to be the most (in ms) we will ever wait for a
// icon to be extracted.
// BUGBUG we should read this from the registry
//
// define MIN_ICON_WAIT to be amount of time that has to go by
// before we start waiting again.
// BUGBUG we should read this from the registry
//
// define TF_ICON to be the trace flags you want all the icon
// related trace out in this file to use.
//
#define MAX_ICON_WAIT       500
#define MIN_ICON_WAIT       2500

#define TF_ICON TF_DEFVIEW
//#define TF_ICON TF_ALWAYS

#include <sfview.h>
#include "sfviewp.h"
#include "shellp.h"

// PRIORITIES for tasks added to the DefView background task scheduler
#define TASK_PRIORITY_BKGRND_FILL   ITSAT_DEFAULT_PRIORITY
#define TASK_PRIORITY_GET_ICON      ITSAT_DEFAULT_PRIORITY

#define DEFVIEW_THREAD_IDLE_TIMEOUT     (1000 * 60 * 2)

BOOL DefView_IdleDoStuff(CDefView *pdsv, LPRUNNABLETASK pTask, REFTASKOWNERID rTID, DWORD_PTR lParam, DWORD dwPriority );

// Whenever we enter an "update pending" state, set this timer
// instead. Let child DefViewOCs have a window to get our
// listview without us invalidating it's state
//
#define DV_IDTIMER_UPDATEPENDING 2
#define UPDATEPENDINGTIME (5*1000)

#define DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE  4
#define NOTIFY_AUTOMATION_SELCHANGE_TIMEOUT     (GetDoubleClickTime())

// Uncomment following line to turn on timing of view enumeration
// #define TIMING 1
//
#ifdef TIMING
    DWORD   dwFinish, dwStart;
#endif

STDAPI SHGetIconFromPIDL(IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, int *piImage);

#ifndef SIF_ALL
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS)
#endif

#define ID_LISTVIEW     1
#define ID_STATIC       2

#define WM_DVI_FILLSTUFF        (WM_USER + 0x100)
#define WM_DVI_ENDIDLE          (WM_USER + 0x101)
#define WM_DVI_GETICON          (WM_USER + 0x102)

#define INVALID_THREAD_ID ((DWORD)-1)

#define DV_CDB_IsCommonDialog(_pdsv) \
        (_pdsv->_pcdb)

#define DV_CDB_OnDefaultCommand(_pdsv) \
        (_pdsv->_pcdb ? _pdsv->_pcdb->OnDefaultCommand( \
        _pdsv->_psvOuter ? _pdsv->_psvOuter : _pdsv) : E_NOTIMPL)

#define DV_CDB_OnStateChange(_pdsv, _code) \
        (_pdsv->_pcdb ? _pdsv->_pcdb->OnStateChange( \
        _pdsv->_psvOuter ? _pdsv->_psvOuter : _pdsv, _code) : E_NOTIMPL)

#define DV_CDB_IncludeObject(_pdsv, _pidl) \
        (_pdsv->_pcdb ? _pdsv->_pcdb->IncludeObject( \
        _pdsv->_psvOuter ? _pdsv->_psvOuter : _pdsv, _pidl) : S_OK)

extern BOOL g_fDraggingOverSource;

UINT g_msgMSWheel = 0;

extern "C" const TCHAR c_szDefViewClass[] = TEXT("SHELLDLL_DefView");

#define COMBINED_CX 244
#define COMBINED_CY 0

#define IsDefaultState(_dvHead) ((_dvHead).dvState.lParamSort == 0 && \
                                 (_dvHead).dvState.iDirection == 1 && \
                                 (_dvHead).dvState.iLastColumnClick == -1 && \
                                 (_dvHead).ptScroll.x == 0 && (_dvHead).ptScroll.y == 0)

typedef struct
{
    POINT pt;
    ITEMIDLIST idl;
} DVITEM;
//
// The following are the standard colors supported on desktop
//
const COLORREF  g_VgaColorTable[] = {
                                        0x000000,   // Black
                                        0x000080,
                                        0x0000FF,
                                        0x008000,
                                        0x008080,
                                        0x00FF00,   // Green
                                        0x00FFFF,   // Yellow
                                        0x800000,
                                        0x800080,
                                        0x808000,
                                        0x808080,
                                        0xF0CAA6,
                                        0xF0FBFF,
                                        0xFF0000,   // Blue
                                        0xFF00FF,   // Magenta
                                        0xFFFF00,   // cobalt
                                        0xFFFFFF    // White
                                    };


//
// Note that it returns NULL, if iItem is -1.
//

HRESULT DefView_ExplorerCommand(CDefView *pdsv, UINT idFCIDM);
void DefView_DismissEdit(CDefView *pdsv);
void DefView_OnInitMenu(CDefView *pdsv);
LRESULT DefView_OnMenuSelect(CDefView *pdsv, UINT id, UINT mf, HMENU hmenu);
void DV_GetMenuHelpText(CDefView *pdsv, UINT_PTR id, LPTSTR pszText, UINT cchText);
void DV_GetToolTipText(CDefView *pdsv, UINT_PTR id, LPTSTR pszText, UINT cchText);
void DV_DoDefaultStatusBar(CDefView *pdsv, BOOL fInitialize);

void DefView_MoveSelectedItems(CDefView *pdsv, int dx, int dy, BOOL fAll);
BOOL DefView_GetDropPoint(CDefView *pdv, POINT *ppt);
BOOL DefView_GetDragPoint(CDefView *pdv, POINT *ppt);
DWORD LVStyleFromView(CDefView *lpdv);
HRESULT DefView_GetItemObjects(CDefView *pdsv, LPCITEMIDLIST **ppidl, UINT uItem, UINT *pcItems);

HRESULT DV_AllocRestOfStream(IStream *pstm, void **ppData, UINT *puLen);

#define SHOWICONS 1
#ifdef SHOWICONS
#define DV_SHOWICONS(pdsv) (!(pdsv->_fCombinedView) || !(pdsv->_fs.fFlags & FWF_NOICONS))
#else
#define DV_SHOWICONS(pdsv) (TRUE)
#endif

// determine if color is light or dark
#define COLORISLIGHT(clr) ((5*GetGValue((clr)) + 2*GetRValue((clr)) + GetBValue((clr))) > 8*128)

//----------------------------------------------------------------------------
#define MINVIEWWIDTH    170
#define MINVIEWHEIGHT   132

// REVIEW UNDONE - calculate these, don't guess!
#define PARENTGAPWIDTH  (40 - 8)    // 40 is win95, -8 is nGapWidth (below)
#define PARENTGAPHEIGHT (32 - 93)   // 32 is win95, -93 is nGepHeight (below)

#define EXTVIEWEXTRAWIDTH  100
#define EXTVIEWEXTRAHEIGHT 110


//----------------------------------------------------------------------------
void EnableCombinedView(CDefView *pdsv, BOOL fEnable);


#ifdef DEBUG

void DefView_StartNotify(CDefView *pdsv, UINT code)
{
    if (pdsv == NULL)
        return;

    TIMESTART(pdsv->_WMNotify);

    switch (code) {
        case LVN_ITEMCHANGING:
            TIMESTART(pdsv->_LVChanging);
            break;

        case LVN_ITEMCHANGED:
            TIMESTART(pdsv->_LVChanged);
            break;

        case LVN_DELETEITEM:
            TIMESTART(pdsv->_LVDelete);
            break;

        case LVN_GETDISPINFO:
            TIMESTART(pdsv->_LVGetDispInfo);
            break;
    }
}

void DefView_StopNotify(CDefView *pdsv, UINT code)
{
    if (pdsv == NULL)
        return;

    TIMESTOP(pdsv->_WMNotify);

    switch (code) {
        case LVN_ITEMCHANGING:
            TIMESTOP(pdsv->_LVChanging);
            break;

        case LVN_ITEMCHANGED:
            TIMESTOP(pdsv->_LVChanged);
            break;

        case LVN_DELETEITEM:
            TIMESTOP(pdsv->_LVDelete);
            break;

        case LVN_GETDISPINFO:
            TIMESTOP(pdsv->_LVGetDispInfo);
            break;
    }
}

HRESULT DV_Next(CDefView *pdsv, IEnumIDList *peunk, int cnt, LPITEMIDLIST *ppidl, ULONG *pcelt)
{
    HRESULT hres;

    TIMESTART(pdsv->_EnumNext);
    hres = peunk->Next(cnt, ppidl, pcelt);
    TIMESTOP(pdsv->_EnumNext);

    return hres;
}

TCHAR *DV_Name(CDefView *pdsv)
{
    static TCHAR ach[128];
    if (pdsv->_hwndMain)
    {
        GetWindowText(pdsv->_hwndMain, ach, ARRAYSIZE(ach));
    }
    else
    {
        lstrcpy(ach, TEXT("<NULL _hwndMain>"));
    }
    return ach;
}

#else
#define DefView_StartNotify(pdsv, code)
#define DefView_StopNotify(pdsv, code)
#define DV_Next(pdsv,peunk,cnt,ppidl,pcelt) peunk->Next(cnt, ppidl, pcelt)
#define DV_Name(pdsv) NULL
#endif


HRESULT CDefView::CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return m_cCallback.CallCB(uMsg, wParam, lParam);
}

void CDefView::RegisterSFVEvents(IUnknown * pTarget, BOOL fConnect)
{
    ConnectToConnectionPoint(SAFECAST(this, IShellView2*), 
        DIID_DShellFolderViewEvents, fConnect, pTarget, &m_dwConnectionCookie, NULL);
}

//  There is almost always an automation client.  Usually the Address Bar
//  and Explorer Band are watching us.  Sometimes MSHTML is watching, too.

HRESULT CDefView::NotifyAutomation(DISPID dispid)
{
    return IUnknown_CPContainerInvokeParam(m_pauto, DIID_DShellFolderViewEvents,
                                           dispid, NULL, 0);
}

//----------------------------------------------------------------------------
//
//  CDefView::CheckIfSelectedAndNotifyAutomation
//
//  This happens rarely, so it doesn't matter that we go through a lot of
//  work, only to find that nobody is listening.
//
void CDefView::CheckIfSelectedAndNotifyAutomation(LPCITEMIDLIST pidl, int iItem)
{
    // Don't bother notifying if there is a pending selection change notify as this will
    // also update the user...
    if (_fSelectionChangePending)
        return;

    if (m_pauto)
    {
        // See if we need to get the Index number for the item
        if (iItem < 0)
            iItem = _FindItem(pidl, NULL, FALSE);
        if (iItem >= 0)
        {
            // Ok we have an index see if the item is selected or not...
            if (ListView_GetItemState(_hwndListview, iItem, LVIS_SELECTED) & LVIS_SELECTED)
            {
                // Ok we will tell them about it, for now don't pass the pidl...
                // Trying to keep perf OK, don't do invoke directly but instead post the
                // same message we do for selection changed.
                _PostSelChangedMessage();
            }
        }
    }
}


//----------------------------------------------------------------------------
BOOL DV_IsDropOnSource(CDefView *pdsv, IDropTarget *pdtgt)
{
    // context menu paste (_bMouseMenu shows context menu, cut stuff shows source)
    if (pdsv->_bMouseMenu && pdsv->_bHaveCutStuff) {
        int iItem = ListView_GetNextItem(pdsv->_hwndListview, -1, LVNI_SELECTED);
        if (iItem == -1) {
            return TRUE;
        }
    }
    //
    // If pdtgt is specified, it should match.
    //
    if (pdtgt && (pdtgt != pdsv->_pdtgtBack))
    {
        return FALSE;
    }

    if (pdsv->_itemCur != -1 || !pdsv->_bDragSource)
    {
        // We did not drag onto the background of the source
        return FALSE;
    }

    return TRUE;
}


void CDefView::_SameViewMoveIcons()
{
    POINT ptDrop = _ptDrop;

    ASSERT(DV_ISANYICONMODE(_fs.ViewMode));

    LVUtil_ClientToLV(_hwndListview, &ptDrop);

    DefView_MoveSelectedItems(this, ptDrop.x - _ptDragAnchor.x, ptDrop.y - _ptDragAnchor.y, FALSE);
}

#ifdef WINNT

BOOL _DoesRegkeyExist(HKEY hkRoot, LPCTSTR pszSubkey)
{
    LONG l = 0;
    return RegQueryValue(hkRoot, pszSubkey, NULL, &l) == ERROR_SUCCESS;
}

#endif


//
// This function checks if the current HTML wallpaper is the default
// wallpaper and returns TRUE if so. If the wallpaper is the default wallpaper,
// it reads the colors from the registry. If the colors are missing, then it
// supplies the default colors.
//
BOOL GetColorsFromHTMLdoc(CDefView *pdsv, COLORREF *clrTextBk, COLORREF *clrHotlight)
{
    int i;

    // If the HTML document hasn't reached ready-state interactive, then
    // the background color is NOT yet set!
    if(pdsv->m_cFrame.m_bgColor == CLR_INVALID)
        return FALSE;

//  98/11/19 #250276 vtan: Use the previously private but now
//  public function _SetDesktopListViewIconTextColors to
//  update the public member variable m_bgColor from Trident.

    (HRESULT)pdsv->m_cFrame._SetDesktopListViewIconTextColors(FALSE);       // no update required - prevents recursion
    
    *clrTextBk = pdsv->m_cFrame.m_bgColor;

    // Check if the given background color is a standard color. If not, use
    // the system background color itself.

    BOOL fStandardColor = FALSE;
    for(i = 0; i < ARRAYSIZE(g_VgaColorTable); i++)
    {
        if(g_VgaColorTable[i] == *clrTextBk)
        {
            fStandardColor = TRUE;
            break;
        }
    }

    if(!fStandardColor)
        *clrTextBk = GetSysColor(COLOR_BACKGROUND);

    if(COLORISLIGHT(*clrTextBk))
        *clrHotlight = 0x000000;    //Black as hightlight color!
    else
        *clrHotlight = 0xFFFFFF;    //White as highlight color!

    return(TRUE);
}

// Set the colors for the folder - taking care if it's the desktop.
void DSV_SetFolderColors(CDefView *pdsv)
{
    COLORREF clrText, clrTextBk, clrWindow;

    // Is this view for the desktop?
    if (pdsv->_IsDesktop())
    {
        TCHAR szWallpaper[128];
        TCHAR szPattern[128];
        HKEY hkey;
        UINT cb;
        DWORD dwPaintVersion;
        COLORREF clrHotlight;

        Shell_SysColorChange();


        //If we show HTML wallpaper, then get the appropriate colors too!
        if((pdsv->_fCombinedView) && GetColorsFromHTMLdoc(pdsv, &clrTextBk, &clrHotlight))
        {
            //Set the Hotlight color!
            ListView_SetHotlightColor(pdsv->_hwndListview, clrHotlight);
        }
        else
        {
            // Yep.
            // Clear the background color of the desktop to make it
            // properly handle transparency.
            clrTextBk = GetSysColor(COLOR_BACKGROUND);

            //Reset the Hotlight color sothat the system color can be used.
            ListView_SetHotlightColor(pdsv->_hwndListview, CLR_DEFAULT);
        }
        // set a text color that will show up over desktop color
        if (COLORISLIGHT(clrTextBk))
            clrText = 0x000000; // black
        else
            clrText = 0xFFFFFF; // white

        clrWindow = CLR_NONE; // Assume transparent

        //
        //  if there is no wallpaper or pattern we can use
        //  a solid color for the ListView. otherwise we
        //  need to use a transparent ListView, this is much
        //  slower so dont do it unless we need to.
        //
        //  Don't do this optimization if USER is going to paint
        //  some magic text on the desktop, such as
        //
        //      "FailSafe" (SM_CLEANBOOT)
        //      "Debug" (SM_DEBUG)
        //      "Build ####" (REGSTR_PATH_DESKTOP\PaintDesktopVersion)
        //      "Evaluation Version"
        //
        //  too bad there is no SPI_GETWALLPAPER, we need to read
        //  from WIN.INI.
        //
        //  BUGBUG we assume the string for none starts with a '('
        //  BUGBUG we dont know if a random app has subclassed
        //  BUGBUG ..the desktop, we should check this case too.
        //

        szWallpaper[0] = 0;
        szPattern[0] = 0;
        dwPaintVersion = 0;

        if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_DESKTOP, &hkey) == 0)
        {
            cb = SIZEOF(szWallpaper);
            SHQueryValueEx(hkey, TEXT("Wallpaper"), NULL, NULL, (LPBYTE)szWallpaper, (ULONG*)&cb);
            cb = SIZEOF(szPattern);
            SHQueryValueEx(hkey, TEXT("Pattern"), NULL, NULL, (LPBYTE)szPattern, (ULONG*)&cb);
            cb = SIZEOF(dwPaintVersion);
            SHQueryValueEx(hkey, TEXT("PaintDesktopVersion"), NULL, NULL, (LPBYTE)&dwPaintVersion, (ULONG*)&cb);
            RegCloseKey(hkey);

#ifdef WINNT
            // Other external criteria for painting the version
            //
            //  -   This is a beta version (has an expiration date)
            //  -   A test certificate is installed
            //
            if (dwPaintVersion == 0 && IsOS(OS_WIN2000))
            {
#define REGSTR_PATH_LM_ROOTCERTIFICATES \
        TEXT("SOFTWARE\\Microsoft\\SystemCertificates\\Root\\Certificates")
#define REGSTR_PATH_GPO_ROOTCERTIFICATES \
        TEXT("SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\Root\\Certificates")
#define REGSTR_KEY_TESTCERTIFICATE \
        TEXT("2BD63D28D7BCD0E251195AEB519243C13142EBC3")

                dwPaintVersion = (0 != USER_SHARED_DATA->SystemExpirationDate.QuadPart) ||
                                 _DoesRegkeyExist(HKEY_LOCAL_MACHINE, REGSTR_PATH_LM_ROOTCERTIFICATES TEXT("\\") REGSTR_KEY_TESTCERTIFICATE) ||
                                 _DoesRegkeyExist(HKEY_LOCAL_MACHINE, REGSTR_PATH_GPO_ROOTCERTIFICATES TEXT("\\") REGSTR_KEY_TESTCERTIFICATE);
            }
#endif
        }

#ifndef CRAZY_CODE
        if (pdsv->_fCombinedView ||
#else
        if (
#endif
            (GetSystemMetrics(SM_CLEANBOOT) == 0 &&
             GetSystemMetrics(SM_DEBUG) == 0 &&
             !dwPaintVersion &&
             (!pdsv->_fHasDeskWallPaper) &&
             (szWallpaper[0] == 0 || szWallpaper[0] == TEXT('(')) &&
             (szPattern[0] == 0 || szPattern[0] == TEXT('('))))
        {
           clrWindow = GetSysColor(COLOR_BACKGROUND);
        }
    }
    else
    {
        // Nope.
        clrWindow = GetSysColor(COLOR_WINDOW);
        clrTextBk = clrWindow;
        clrText = GetSysColor(COLOR_WINDOWTEXT);

        if ((pdsv->_fs.fFlags & FWF_TRANSPARENT) && !pdsv->_fCombinedView)
        {
            clrWindow = CLR_NONE;
            clrTextBk = CLR_NONE;
        }
    }

    if (!pdsv->_fClassic && ISVALIDCOLOR(pdsv->_crCustomColors[CRID_CUSTOMTEXTBACKGROUND]))
        clrTextBk = pdsv->_crCustomColors[CRID_CUSTOMTEXTBACKGROUND];

    if (!pdsv->_fClassic && ISVALIDCOLOR(pdsv->_crCustomColors[CRID_CUSTOMTEXT]))
        clrText = pdsv->_crCustomColors[CRID_CUSTOMTEXT];

    // if its a thumbvw, do the thumbvw thing, always do standard listvw thing.
    if (pdsv->m_cFrame.IsSFVExtension())
    {
        IShellFolderView * pSFV = pdsv->m_cFrame.GetExtendedISFV();

        if (pSFV)
        {
            HRESULT hres;
            IDefViewExtInit2 * pShellView = NULL;

            hres = pSFV->QueryInterface(IID_IDefViewExtInit2, (void **)&pShellView);
            if (SUCCEEDED(hres))
            {
                pShellView->SetViewWindowColors(clrText, clrTextBk, clrWindow); 
                ATOMICRELEASE(pShellView);
            }
        }

    }
    ListView_SetBkColor(pdsv->_hwndListview, clrWindow);
    ListView_SetTextBkColor(pdsv->_hwndListview, clrTextBk);
    ListView_SetTextColor(pdsv->_hwndListview, clrText);
}

//----------------------------------------------------------------------------
DWORD LVStyleFromView(CDefView *pdsv)
{
    DWORD dwStyle;

    if (pdsv->_IsDesktop())
    {
        dwStyle = LVS_ICON | LVS_NOSCROLL | LVS_ALIGNLEFT;
    }
    else
    {
        switch (pdsv->_fs.ViewMode) 
        {
        case FVM_LIST:
            dwStyle = LVS_LIST;
            break;
        case FVM_DETAILS:
            dwStyle = LVS_REPORT;
            break;
        case FVM_SMALLICON:
            dwStyle = LVS_SMALLICON;
            break;
        default:
            TraceMsg(TF_WARNING, "Unknown ViewMode value");

            // fall through...
        case FVM_ICON:
            dwStyle = LVS_ICON;
            break;
        }
        dwStyle |= LVS_SHOWSELALWAYS;   // make sure selection is visible
    }

    if (pdsv->_fs.fFlags & FWF_AUTOARRANGE)
        dwStyle |= LVS_AUTOARRANGE;

    if (pdsv->_fs.fFlags & FWF_SINGLESEL)
        dwStyle |= LVS_SINGLESEL;

    if (pdsv->_fs.fFlags & FWF_ALIGNLEFT)
        dwStyle |= LVS_ALIGNLEFT;

    if (pdsv->_fs.fFlags & FWF_NOSCROLL)
        dwStyle |= LVS_NOSCROLL;

    return dwStyle;
}

void CDefView::_GetSortDefaults(DVSAVESTATE *pSaveState)
{
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SORTCOLUMNS, FALSE);
    pSaveState->lParamSort = ss.lParamSort;
    pSaveState->iDirection = ss.iSortDirection ? ss.iSortDirection : 1;
    pSaveState->iLastColumnClick = -1;
    CallCB(SFVM_GETSORTDEFAULTS, (LPARAM)&(pSaveState->iDirection), (WPARAM)&(pSaveState->lParamSort));

    TraceMsg(TF_DEFVIEW, "_GetSortDefaults returning lParamSort==%d, iDir==%d", pSaveState->lParamSort, pSaveState->iDirection);
}

HRESULT CDefView::_GetDetailsHelper(int i, DETAILSINFO *pdi)
{
    HRESULT hres = E_NOTIMPL;

    if (_pshf2)
    {
        hres = _pshf2->GetDetailsOf(pdi->pidl, i, (SHELLDETAILS *)&pdi->fmt);
    }

    if (FAILED(hres))   // Don't make NSEs impl all of IShellFolder2
    {
        if (_psd)
        {
            // HACK: pdi->fmt is the same layout as SHELLDETAILS
            hres = _psd->GetDetailsOf(pdi->pidl, i, (SHELLDETAILS *)&pdi->fmt);
        }
        else if (HasCB())
        {
            hres = CallCB(SFVM_GETDETAILSOF, i, (LPARAM)pdi);
        }
        else
        {
            TraceMsg(TF_ERROR, "cdv._gdh: neither ISD or callback found");
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Determine if the given defview state struct has valid
         state info.  If is doesn't, this function massages the
         values so it does.
*/
void ValidateDVState(
                     IN OUT PDVSAVESTATE pdvState)
{
    ASSERT(IS_VALID_WRITE_PTR(pdvState, DVSAVESTATE));
    
    if (0 == pdvState->iDirection)
        pdvState->iDirection = 1;
}


#define CCOLSHEADER_SIGNATURE 0xfddfdffd
#define CCOLSHEADER_VERSION_IE4 0x0C
#define CCOLSHEADER_VERSION_IE5 0x0E

// Use this for actualy dumping to the stream
typedef struct
{
    DWORD  dwSignature;
    USHORT uVersion; // 0x0c == IE4, 0x0e == IE5
    USHORT uCols;
    USHORT uOffsetWidths;
    USHORT uOffsetColOrder;
    USHORT uOffsetColStates;
} COLSFILEHEADER, *LPCOLSFILEHEADER;

class CColumnPointer
{
public:
    // Note: we assume that pdsv is used _only_ for calling IVC.  If this changes, fix callers
    CColumnPointer(BYTE *pCols, CDefView *pdsv)
    {
        LPCOLSFILEHEADER lpcfh = (LPCOLSFILEHEADER) pCols;
        DWORD *pColList = NULL;
        int u;

        // set these up, as they shouldn't change
        m_cfh.dwSignature = CCOLSHEADER_SIGNATURE;
        m_cfh.uVersion = CCOLSHEADER_VERSION_IE5;

        m_dsaWidths = DSA_Create(sizeof(USHORT), 5); // Standard defview has 5 columns
        m_dsaColOrder = DSA_Create(sizeof(INT), 5);


        // Here we need to loop through pCols (which points to a ColsFileHeader,
        //   followed by the two arrays), and set up the DPAs, and our m_cfh

        // if we think there are zero columns, then bail...
        if (lpcfh && lpcfh->dwSignature == CCOLSHEADER_SIGNATURE && lpcfh->uCols)
        {

            for (u = 0; u < lpcfh->uCols; u++)
            {
                TraceMsg(TF_DEFVIEW, "CColumns: %d initialized to %d wide", u, *((USHORT*)(pCols + lpcfh->uOffsetWidths + u * sizeof(USHORT))));
                DSA_AppendItem(m_dsaColOrder, (pCols + lpcfh->uOffsetColOrder + u * sizeof(INT)) );
                DSA_AppendItem(m_dsaWidths,   (pCols + lpcfh->uOffsetWidths + u * sizeof(USHORT)) );
            }

            if (lpcfh->uVersion >= CCOLSHEADER_VERSION_IE5)
            {
                // the col list is at the very end
                pColList = (DWORD*) (pCols + lpcfh->uOffsetColStates);
                TraceMsg(TF_DEFVIEW, "Initializing with column info, pCols=%x");
            }
        }
        else
        {
            if (pCols != NULL)
                TraceMsg(TF_ERROR, "CColumnPointer Not able to set column info (w95 upgrade???) (pCols = 0x%X)",pCols);
            else
                TraceMsg(TF_DEFVIEW, "CColumnPointer ctor: not passed any data");
        }
        if (pdsv)
            pdsv->InitializeVariableColumns(pColList);
        // Set any defaults
    }


    ~CColumnPointer()
    {
        if (m_dsaWidths)
            DSA_Destroy(m_dsaWidths);
        if (m_dsaColOrder)
            DSA_Destroy(m_dsaColOrder);
    }

    // uOrder should normally be uCol
    BOOL AppendColumn(UINT uCol, USHORT uWidth, INT uOrder)
    {
        UINT *p;
        // Slide every index above this one up
        for(INT u=0;u<DSA_GetItemCount(m_dsaColOrder);u++)
        {
            p = (UINT *) DSA_GetItemPtr(m_dsaColOrder, u);
            if (!p)
                break; // safety...
            if (*p >= uCol)
                (*p)++;
        }

        DSA_AppendItem(m_dsaWidths, &uWidth);
        DSA_AppendItem(m_dsaColOrder, &uOrder);
        // maybe we should store column ordering as absolute numbers
        return TRUE;
    }

    BOOL RemoveColumn(UINT uCol)
    {
        UINT *p;

        if ((int)uCol >= DSA_GetItemCount(m_dsaWidths))
            return FALSE;
        // Slide every index above this one down
        for(INT u=0;u<DSA_GetItemCount(m_dsaColOrder);u++)
        {
            p = (UINT *) DSA_GetItemPtr(m_dsaColOrder, u);
            if (!p)
                break; // safety...
            if (*p > uCol)
                (*p)--;
        }

        DSA_DeleteItem(m_dsaWidths, uCol);
        DSA_DeleteItem(m_dsaColOrder, uCol);
        return TRUE;
    }

    UINT RestoreColumnWidth(UINT uCol, UINT uDefWid)
    {
        USHORT uWidth = 0;
        if (uCol < (UINT) DSA_GetItemCount(m_dsaWidths))
        {
            DSA_GetItem(m_dsaWidths, uCol, &uWidth);
        }
        return(uWidth ? uWidth : uDefWid);        // disallow zero width columns
    }

    // return true if default widths
    BOOL SaveColumnWidths(HWND hwndList)
    {
        UINT cCols;
        USHORT us;
        LV_COLUMN lvc;
        BOOL bOk = TRUE;
        HDSA dsaNewWidths;
        HWND hwndHead = ListView_GetHeader(hwndList);

        if (!hwndHead || !(cCols = Header_GetItemCount(hwndHead)))
            return TRUE;

        dsaNewWidths = DSA_Create(sizeof(USHORT), cCols);
        if (!dsaNewWidths)
            return TRUE;

        TraceMsg(TF_DEFVIEW, "SaveColumnWidths storing %d columns", cCols);
        for (UINT u=0; u<cCols && bOk; ++u)
        {
            lvc.mask = LVCF_WIDTH;
            bOk = ListView_GetColumn(hwndList, u, &lvc);
            us = (USHORT) lvc.cx;    // make sure its a short
            DSA_AppendItem(dsaNewWidths, &us);
            // TraceMsg(TF_DEFVIEW, "  saving col %d width of %d", u, us);
        }
        if (bOk)
        {
            if (m_dsaWidths)
                DSA_Destroy(m_dsaWidths);
            m_dsaWidths = dsaNewWidths;
        }
        else
            DSA_Destroy(dsaNewWidths);
        return !bOk;
    }

    void RestoreColumnOrder(HWND hwndList)
    {
        UINT cCols;
        UINT *pCols;
        HWND hwndHead = ListView_GetHeader(hwndList);

        if (!hwndHead)
            return;

        cCols = Header_GetItemCount(hwndHead);

        if (cCols != (UINT) DSA_GetItemCount(m_dsaColOrder))
        {
            // this is a normal case if a folder is opened and there is no saved state. no need to spew.
            if (DSA_GetItemCount(m_dsaColOrder))
                TraceMsg(TF_DEFVIEW, "RestoreColumnOrder: cCols (%d) mismatched DPA count (%d), NOT RESTORING",cCols, DSA_GetItemCount(m_dsaColOrder));
            return;
        }

        pCols  = (LPUINT) LocalAlloc(LPTR, cCols * sizeof(UINT));
        for(UINT u=0; u<cCols; u++)
            DSA_GetItem(m_dsaColOrder, u, pCols + u);

        ListView_SetColumnOrderArray(hwndList, cCols, pCols);
        LocalFree(pCols);
    }

    // return TRUE if default order
    BOOL SaveColumnOrder(HWND hwndList)
    {
        BOOL bDefaultOrder = TRUE;
        UINT cCols;
        HWND hwndHead = ListView_GetHeader(hwndList);

        if (!hwndHead)
        {
            TraceMsg(TF_DEFVIEW, "returning prev col order, not reading from listview");
            return TRUE;
        }

        cCols = Header_GetItemCount(hwndHead);

        if (!cCols)
        {
            TraceMsg(TF_DEFVIEW, "returning prev col order, not reading from listview");
            return TRUE;
        }

        UINT *pCols = (LPUINT) LocalAlloc(LPTR, cCols * sizeof(UINT));

        ListView_GetColumnOrderArray(hwndList, cCols, pCols);

        DSA_DeleteAllItems(m_dsaColOrder);
        for (UINT u=0; u<cCols; ++u)
        {
            DSA_AppendItem(m_dsaColOrder, &pCols[u]);
            if (pCols[u] != u)
                bDefaultOrder = FALSE;
        }

        LocalFree(pCols);
        return(bDefaultOrder);
    }

    HRESULT Write(IStream* pstm, CDefView * pvc)
    {
        DWORD i;

        if (!pstm) {
            return (E_INVALIDARG);
        }

        ASSERT(DSA_GetItemCount(m_dsaColOrder) == DSA_GetItemCount(m_dsaWidths));

        // dwSignature and cbSize were set above
        m_cfh.uCols = (UINT) DSA_GetItemCount(m_dsaColOrder);
        m_cfh.uOffsetColOrder = sizeof(COLSFILEHEADER);
        m_cfh.uOffsetWidths = m_cfh.uOffsetColOrder + sizeof(UINT) * m_cfh.uCols;
        m_cfh.uOffsetColStates = m_cfh.uOffsetWidths + sizeof(USHORT) * m_cfh.uCols;

        // No point in persisting, if there aren't any columns around.
        // this is true for folders that are just opened and closed
        if (!m_cfh.uCols)
            return E_FAIL;

        // Note- dependent on DSA storing data internally as byte-packed.
        pstm->Write(&m_cfh, sizeof(m_cfh), NULL);
        pstm->Write(DSA_GetItemPtr(m_dsaColOrder, 0), sizeof(UINT) * DSA_GetItemCount(m_dsaColOrder), NULL);
        pstm->Write(DSA_GetItemPtr(m_dsaWidths, 0), sizeof(USHORT) * DSA_GetItemCount(m_dsaWidths), NULL);

        for(i=0;i<pvc->GetMaxColumns();i++)
        {
            if (pvc->IsColumnOn(i))
                pstm->Write(&i, sizeof(DWORD), NULL);
        }
        i=0xFFFFFFFF;
        pstm->Write(&i, sizeof(DWORD), NULL);

        TraceMsg(TF_DEFVIEW, "CColumnPointer persisting %d columns", m_cfh.uCols);
        return S_OK; // duhhh, pass down errors from stream writes...
    }

private:
    COLSFILEHEADER m_cfh; // old header
    HDSA m_dsaWidths;
    HDSA m_dsaColOrder;
};

void CDefView::AddColumns()
{
    int i;
    UINT iVisible;
    LPBYTE pColHdr;
    LV_COLUMN col;
    UINT uLen;

    // so we do this once
    ASSERT(!_bLoadedColumns);
    _bLoadedColumns = TRUE;

    // I also use this as a flag for whether to free pColHdr
    IStream *pstmCols = NULL;

    //
    // Calculate a reasonable size to initialize the column width to.

    _cxChar = GetControlCharWidth(_hwndListview);

    // Check whether there is any column enumerator (ShellDetails or callback)
    if (!_psd && !_pshf2 && !this->HasCB())
    {
        goto Error1;
    }

    pColHdr = NULL;
    if (SUCCEEDED(CallCB(SFVM_GETCOLSAVESTREAM, STGM_READ,
            (LPARAM)&pstmCols)) && pstmCols)
    {
        uLen = 0;

        HRESULT hres = DV_AllocRestOfStream(pstmCols, (void **)&pColHdr, &uLen);

        ATOMICRELEASE(pstmCols);

        if (FAILED(hres))
        {
            // Make sure we do not try to Free below
            pstmCols = NULL;
            uLen = 0;
        }
    }
    else if (_pSaveHeader)
    {
        uLen = _pSaveHeader->GetColumnsInfo(&pColHdr);
    }
    else
    {
        uLen = 0;
    }

    TraceMsg(TF_DEFVIEW, "AddColumns: about to create CColumnPointer with uLen == %d, pColHdr = %x",uLen, pColHdr);
    if (_pcp)
        delete _pcp;
    _pcp = new CColumnPointer(pColHdr, this);

    for (i=0; i< (int) GetMaxColumns(); ++i)
    {
        if (this->IsColumnOn(i))
        {
            MapRealToVisibleColumn((UINT) i, &iVisible);

            col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            col.fmt = m_pColItems[i].fmt;
            col.cx = _pcp->RestoreColumnWidth(iVisible, m_pColItems[i].cChars * _cxChar);  // Note iVisible
            col.pszText = m_pColItems[i].szName;
            col.cchTextMax = MAX_COLUMN_NAME_LEN;
            col.iSubItem = i;

            if (col.fmt & LVCFMT_COL_HAS_IMAGES)
            {
                ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_SUBITEMIMAGES, LVS_EX_SUBITEMIMAGES);
                col.fmt &= ~LVCFMT_COL_HAS_IMAGES;
            }

            ListView_InsertColumn(_hwndListview, iVisible, &col);
        }

    }
    // i is now the number of columns

    MapRealToVisibleColumn((UINT) i, &iVisible);
    iVisible++; // don't want zero based -- should make GetVisibleColumnCount?
    TraceMsg(TF_DEFVIEW, "AddColumns: settting CDefView's m_uCols = %d",iVisible);
    m_uCols = iVisible;

    // Set the header control to have zero margin around bitmaps, for the sort arrows
    Header_SetBitmapMargin( ListView_GetHeader(_hwndListview), 0);

    ListView_SetExtendedListViewStyleEx(_hwndListview,
        LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP,
        LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP);

    _pcp->RestoreColumnOrder(_hwndListview);

    if (pstmCols)
    {
        LocalFree((HLOCAL)pColHdr);
    }

Error1:;

       // use real numbers, not visible
    if (_dvState.iLastColumnClick >= i) {
        _GetSortDefaults(&_dvState);

        if (_dvState.iLastColumnClick >= i ||
            _dvState.lParamSort >= i) {

            // our defaults won't work on this view....
            // hard code these defaults
            _dvState.lParamSort = 0;
            _dvState.iDirection = 1;
            _dvState.iLastColumnClick = -1;

        }
    }
}

void CDefView::InitSelectionMode()
{
    SHELLSTATE ss;

    SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW, FALSE);

    _dwSelectionMode = 0;

    if (!_fClassic || (_fs.fFlags & FWF_SINGLECLICKACTIVATE))
    {
        if (!ss.fDoubleClickInWebView)
            _dwSelectionMode = LVS_EX_TRACKSELECT|LVS_EX_ONECLICKACTIVATE;
    }
}

void CDefView::UpdateSelectionMode()
{
    this->InitSelectionMode();
    ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE, _dwSelectionMode);
}

void UpdateUnderlines(CDefView *pdsv)
{
    DWORD cb;
    DWORD dwUnderline = ICON_IE;
    DWORD dwExStyle;
    
    //
    // Read the icon underline settings.
    //
    cb = SIZEOF(dwUnderline);
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                    TEXT("IconUnderline"),
                    NULL,
                    &dwUnderline,
                    &cb,
                    FALSE,
                    &dwUnderline,
                    cb);

    //
    // If it says to use the IE link settings, read them in.
    //
    if (dwUnderline == ICON_IE)
    {
        dwUnderline = ICON_YES;

        TCHAR szUnderline[8];
        cb = SIZEOF(szUnderline);
        SHRegGetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                        TEXT("Anchor Underline"),
                        NULL,
                        szUnderline,
                        &cb,
                        FALSE,
                        szUnderline,
                        cb);

        //
        // Convert the string to an ICON_ value.
        //
        if (!lstrcmpi(szUnderline, TEXT("hover")))
            dwUnderline = ICON_HOVER;
        else if (!lstrcmpi(szUnderline, TEXT("no")))
            dwUnderline = ICON_NO;
        else
            dwUnderline = ICON_YES;
    }

    //
    // Convert the ICON_ value into an LVS_EX value.
    //
    switch (dwUnderline)
    {
    case ICON_NO:
        dwExStyle = 0;
        break;

    case ICON_HOVER:
        dwExStyle = LVS_EX_UNDERLINEHOT;
        break;

    case ICON_YES:
        dwExStyle = LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD;
        break;
    }

    //
    // Set the new LVS_EX_UNDERLINE flags.
    //
    ListView_SetExtendedListViewStyleEx(pdsv->_hwndListview,
                                        LVS_EX_UNDERLINEHOT |
                                        LVS_EX_UNDERLINECOLD,
                                        dwExStyle);
}

LRESULT CDefView::_OnCreate(HWND hWnd)
{
    DWORD dwStyle;
    DWORD dwExStyle;
    HIMAGELIST himlLarge, himlSmall;
    ULONG rgfAttr;

    SetWindowLongPtr(hWnd, 0, (LONG_PTR)this);
    this->AddRef(); // hwnd -> this
    _hwndView = hWnd;
    _hmenuCur = NULL;
    _uState = SVUIA_DEACTIVATE;
    _hAccel = LoadAccelerators(HINST_THISDLL, MAKEINTRESOURCE(ACCEL_DEFVIEW));

    // Note that we are going to get a WM_SIZE message soon, which will
    // place this window correctly
    //

    // Map the ViewMode to the proper listview style
    dwStyle = LVStyleFromView(this);
    dwExStyle = 0;

    // If the parent window is mirrored then the treeview window will inheret the mirroring flag
    // And we need the reading order to be Left to right, which is the right to left in the mirrored mode.

    if (IS_WINDOW_RTL_MIRRORED(hWnd)) 
    {
        // This means left to right reading order because this window will be mirrored.
        dwExStyle |= WS_EX_RTLREADING;
    }

    rgfAttr = SFGAO_CANRENAME;
    if (SUCCEEDED(_pshf->GetAttributesOf(0, NULL, &rgfAttr))
     && (rgfAttr & SFGAO_CANRENAME))
    {
        dwStyle |= LVS_EDITLABELS;
    }

    if (!_IsDesktop() && !(_fs.fFlags & FWF_NOCLIENTEDGE))
    {
        dwExStyle |= WS_EX_CLIENTEDGE;
    }

    if (_IsOwnerData())
        dwStyle |= LVS_OWNERDATA;

    _hwndListview = CreateWindowEx(dwExStyle, WC_LISTVIEW, NULL,
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | dwStyle | LVS_SHAREIMAGELISTS,
            0, 0, 0, 0, hWnd, (HMENU)ID_LISTVIEW, HINST_THISDLL, NULL);
    if (!_hwndListview)
    {
        TraceMsg(TF_ERROR, "Failed to create view window");
        return -1;     // failure
    }

    DWORD dwLVExStyle = LVS_EX_INFOTIP | LVS_EX_LABELTIP;
    if (_IsDesktop() && (GetNumberOfMonitors() > 1))
        dwLVExStyle |= LVS_EX_MULTIWORKAREAS;

    // turn on infotips -- window was just created, so all LVS_EX bits are off
    ListView_SetExtendedListViewStyle(_hwndListview, dwLVExStyle);

    _fmt = 0;
    // Be sure that the OS is supporting the flags DATE_LTRREADING and DATE_RTLREADING
    if (g_bBiDiPlatform)
    {
        // Get the date format reading order
        LCID locale = GetUserDefaultLCID();
        if (   (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC)
            || (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))
        {
            //Get the real list view windows ExStyle.
            // [msadek]; we shouldn't check for either WS_EX_RTLREADING OR RTL_MIRRORED_WINDOW
            // on localized builds we have both of them to display dirve letters,..etc correctly
            // on enabled builds we have none of them. let's check on RTL_MIRRORED_WINDOW only
            
            dwLVExStyle = GetWindowLong(_hwndListview, GWL_EXSTYLE);
            if (dwLVExStyle & RTL_MIRRORED_WINDOW)
                _fmt = LVCFMT_RIGHT_TO_LEFT;
            else
                _fmt = LVCFMT_LEFT_TO_RIGHT;
        }
    }

    // set the TTS_TOPMOST style bit for the tooltip
    HWND hwndInfoTip = ListView_GetToolTips(_hwndListview);
    if (EVAL(hwndInfoTip))
    {

        //make the tooltip window  to be topmost window
        SetWindowPos(hwndInfoTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        // set the AutoPopTime (the duration of showing the tooltip) to a large value
        SendMessage(hwndInfoTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)MAXSHORT);

        // increase the ShowTime (the delay before we show the tooltip) to 2 times the default value
        LRESULT uiShowTime = SendMessage(hwndInfoTip, TTM_GETDELAYTIME, TTDT_INITIAL, 0);
        SendMessage(hwndInfoTip, TTM_SETDELAYTIME, TTDT_INITIAL, uiShowTime * 2);
    }

    UpdateUnderlines(this);

    Shell_GetImageLists(&himlLarge, &himlSmall);
    ListView_SetImageList(_hwndListview, himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(_hwndListview, himlSmall, LVSIL_SMALL);

    ASSERT(_psd == NULL);  // this should not be set yet...

    // IShellDetails for old callers, new guys use IShellFolder2
    _pshf->CreateViewObject(_hwndMain, IID_IShellDetails, (void **)&_psd);

    if ( (_fs.ViewMode == FVM_DETAILS) ||
         (SHGetAppCompatFlags(ACF_LOADCOLUMNHANDLER) & ACF_LOADCOLUMNHANDLER))
    {
        if (!_bLoadedColumns)
            this->AddColumns();
    }

    DSV_SetFolderColors(this);

    if (!g_msgMSWheel)
        g_msgMSWheel = RegisterWindowMessage(TEXT("MSWHEEL_ROLLMSG"));

    return 0;   // success
}

// "Auto" AutoArrange means re-position if we are in a positioned view
// and the listview is not in auto-arrange mode. we do this to re-layout
// the icons in cases where that makes sense

STDMETHODIMP CDefView::AutoAutoArrange(DWORD dwReserved)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.SFVAutoAutoArrange( dwReserved );
    }

    if (!_bItemsMoved && DV_ISANYICONMODE(_fs.ViewMode) &&
        !(GetWindowStyle(_hwndListview) & LVS_AUTOARRANGE))
    {
        ListView_Arrange(_hwndListview, LVA_DEFAULT);
    }
    return NOERROR;
}

LRESULT CDefView::WndSize(HWND hWnd)
{
    RECT rc;

    // We need to dismiss "name edit" mode, if we are in.
    DefView_DismissEdit(this);

    // Get the client size.
    GetClientRect(hWnd, &rc);

    // Set the Static to be the Client size.
    if (_hwndStatic)
    {
        MoveWindow(_hwndStatic, rc.left, rc.top,
            rc.right-rc.left, rc.bottom-rc.top, TRUE);
        RedrawWindow(_hwndStatic, NULL, NULL, RDW_ERASE |RDW_INVALIDATE);
    }

    //
    // Set all windows to their new rectangles.
    //
    // BUGBUG: if this is an SFV extension and _fGetWindowLV has happened,
    // we'll resize it incorrectly.  Should be an impossible situation to
    // be in, but we should be more careful...
    //
    m_cFrame.SetRect(&rc);

    // Don't resize _hwndListview if a DefViewOC is using it.
    //
    // If we're waiting for a Web View (!_fCanActivateNow), then it
    // doesn't make sense to resize the _hwndListview -- just extra
    // work, right?  But in the non-WebView case, it's this first
    // resize which sets the listview size, and then there are no
    // more.  Unfortunately, the first resize comes in when the
    // _hwndListview is created, which is *before* _fCanActivateNow
    // can possibly be set.
    //

    if (!_fGetWindowLV)
    {
        SetWindowPos(_hwndListview, NULL, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
        AutoAutoArrange(0); // defview OC does this in the other case
    }

    CallCB(SFVM_SIZE, 0, 0);
    return 1;
}


UINT _DSV_GetMenuIDFromViewMode(UINT ViewMode)
{
    switch (ViewMode) {
    case FVM_SMALLICON:
        return SFVIDM_VIEW_SMALLICON;
    case FVM_LIST:
        return SFVIDM_VIEW_LIST;
    case FVM_DETAILS:
        return SFVIDM_VIEW_DETAILS;
    case FVM_ICON:
    default:
        return SFVIDM_VIEW_ICON;
    }
}

void CDefView::CheckToolbar()
{
    int idCmd;
    int idCmdCurView = m_cFrame.CurExtViewId();

    if (idCmdCurView < 0)
    {
        idCmdCurView = _DSV_GetMenuIDFromViewMode(_fs.ViewMode);
    }

    if (SHGetAppCompatFlags(ACF_WIN95DEFVIEW) & ACF_WIN95DEFVIEW) {
        // preserve win95 behavior for dumb corel apps
        for (idCmd = SFVIDM_VIEW_ICON; idCmd <= SFVIDM_VIEW_DETAILS; idCmd++)
        {
            _psb->SendControlMsg(
                FCW_TOOLBAR, TB_CHECKBUTTON, idCmd, (LPARAM)(idCmd == idCmdCurView), NULL);
        }

        for (idCmd = SFVIDM_VIEW_EXTFIRST; idCmd <= SFVIDM_VIEW_EXTLAST; idCmd++)
        {
            _psb->SendControlMsg(
                FCW_TOOLBAR, TB_CHECKBUTTON, idCmd, (LPARAM)(idCmd == idCmdCurView), NULL);
        }
    } else if (_pbtn) {

        // loop through our button array and check/uncheck any SFVIDM_VIEW_EXT
        // buttons.  normally, we won't have any of these.

        for (int i = 0; i < _cButtons; i++) {
            if (InRange(_pbtn[i].idCommand, SFVIDM_VIEW_EXTFIRST, SFVIDM_VIEW_EXTLAST)) {
                _psb->SendControlMsg(FCW_TOOLBAR, TB_CHECKBUTTON, _pbtn[i].idCommand, 
                        (LPARAM)(_pbtn[i].idCommand == idCmdCurView), NULL);
            }
        }
    }
}

void CDefView::OnListViewDelete(int iItem, LPITEMIDLIST pidl)
{
    LPITEMIDLIST pidlReal = pidl;

    if (!pidlReal && _IsOwnerData())
    {
        CallCB(SFVM_GETITEMIDLIST, iItem, (LPARAM)&pidlReal);
    }

    if (pidlReal)
    {
        CallCB(SFVM_DELETEITEM, 0, (LPARAM)pidlReal);
    }
    
    if (pidl)
        ILFree(pidl);
}

// NOTE: many keys are handled as accelerators

void CDefView::HandleKeyDown(LV_KEYDOWN *lpnmhdr)
{
    // REVIEW: these are things not handled by accelerators, see if we can
    // make them all based on accelerators

    switch (lpnmhdr->wVKey) {
    case VK_ESCAPE:
        if (_bHaveCutStuff)
            OleSetClipboard(NULL);
        break;
    }
}

//
// DV_GetPidl
//
// This function checks to see if we are in virtual mode or not.  If we are in
// virtual mode, we always need to ask our folder we are viewing for the item and
// not the listview.
//
// BUGBUG: this really should return a const poitner since clients
// are not supposed to free this.
//
LPITEMIDLIST CDefView::_GetPIDL(int i)
{
    if (_IsOwnerData())
    {
        LPITEMIDLIST pidl = NULL;
        CallCB(SFVM_GETITEMIDLIST, i, (LPARAM)&pidl);
        return pidl;
    }

    return (LPITEMIDLIST)LVUtil_GetLParam(_hwndListview, i);
}

LPCITEMIDLIST DV_GetPIDLParam(CDefView *pdsv, LPARAM lParam, int i)
{
    if (lParam)
        return (LPCITEMIDLIST)lParam;

    return (LPCITEMIDLIST)pdsv->_GetPIDL(i);
}

//
// DefView_GetItemPIDLS
//
//  This function returns an array of LPCITEMIDLIST for "selected" objects in the
// listview. It always returns the number of selected objects. Typically, the
// client (1) calls this function with cItemMax==0 to know the required size for the
// array, (2) allocates a block of memory for the array, and (3) calls this function
// again to get the list of LPCITEMIDLIST.
//
// Notes: Note that this function returns LP*C*ITEMIDLIST. The caller is not
//  supposed alter or delete them. Their lifetime are very short (until the
//  list view is modified).
//
UINT DefView_GetItemPIDLS(CDefView *pdsv, LPCITEMIDLIST apidl[], UINT cItemMax,
        UINT uItem)
{
    // REVIEW: We should put the focused one at the top of the list.
    int iItem = -1;
    int iItemFocus = -1;
    UINT cItem = 0;
    UINT uType;

    switch (uItem)
    {
    case SVGIO_SELECTION:
        // special case for faster search
        if (!cItemMax) {
            return ListView_GetSelectedCount(pdsv->_hwndListview);
        }
        iItemFocus = ListView_GetNextItem(pdsv->_hwndListview, -1, LVNI_FOCUSED);
        uType = LVNI_SELECTED;
        break;

    case SVGIO_ALLVIEW:
        // special case for faster search
        if (!cItemMax)
            return ListView_GetItemCount(pdsv->_hwndListview);
        uType = LVNI_ALL;
        break;
    }

    while((iItem = ListView_GetNextItem(pdsv->_hwndListview, iItem, uType))!=-1)
    {
        if (cItem < cItemMax)
        {
            // Check if the item is the focused one or not.
            if (iItem == iItemFocus)
            {
                // Yes, put it at the top.
                apidl[cItem] = apidl[0];
                apidl[0] = pdsv->_GetPIDL(iItem);
            }
            else
            {
                // No, put it at the end of the list.
                apidl[cItem] = pdsv->_GetPIDL(iItem);
            }
        }
        cItem++;
    }

    return cItem;
}


//
//  This function get the array of IDList from the selection and calls
// IShellFolder::GetUIObjectOf member to get the specified UI object
// interface.
//
HRESULT DefView_GetUIObjectFromItem(CDefView *pdsv, REFIID riid, void **ppv, UINT uItem)
{
    LPCITEMIDLIST *apidl=NULL;
    UINT cItems;
    HRESULT hres;

    if (pdsv->m_cFrame.IsSFVExtension()) // IShellView extension
    {
        hres = E_INVALIDARG;

        if (SVGIO_SELECTION == uItem)
        {
            // delegate to the IShellView2 extension
            hres = pdsv->m_cFrame.GetExtendedISFV()->GetSelectedObjects(&apidl, &cItems);
        }
#if 0 // THIS IS UNTESTED: it's here in case someone needs it later. make sure it works
        else if (SVGIO_ALLVIEW == uItem)
        {
            hres = pdsv->m_cFrame.GetExtendedISFV()->GetObjectCount(&cItems);
            if (SUCCEEDED(hres))
            {
                apidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, SIZEOF(LPCITEMIDLIST)*cItems);
                if (apidl)
                {
                    UINT count = 0;
                    
                    for (UINT i=0; i < cItems; i++)
                    {
                        if (SUCCEEDED(pdsv->m_cFrame.GetExtendedISFV()->GetObject((LPITEMIDLIST*)&apidl[count], count)))
                            count++;
                    }
                    cItems = count;
                    if (0 == cItems)
                    {
                        LocalFree(apidl);
                        hres = E_UNEXPECTED;
                    }
                }
            }
        }
#endif
        
        if (SUCCEEDED(hres))
        {
            if (cItems)
            {
                hres = pdsv->_pshf->GetUIObjectOf(pdsv->_hwndMain, cItems, apidl, riid, 0, ppv);
                ASSERT(apidl);
                LocalFree((HLOCAL)apidl);
            }
            else
                hres = E_INVALIDARG;
        }
    }
    else
    {
        hres = DefView_GetItemObjects(pdsv, &apidl, uItem, &cItems);
        if (SUCCEEDED(hres))
        {
            if (cItems)
            {
                hres = pdsv->_pshf->GetUIObjectOf(pdsv->_hwndMain, cItems, apidl, riid, 0, ppv);
            }
            else
            {
                hres = E_INVALIDARG;
            }

            if (apidl)
                LocalFree(apidl);
        }
    }

    return hres;
}

//
//  This function creates the cached context menu for the current selection
// and update the cache, if it is not created yet. Then, returns a copied
// pointer to it. The caller should Release() it.
//
//  NOTE: if we're in an extended view, don't cache the context menu
// because the top defview isn't notified of selection changes, so the
// cache will get out of date.
//
IContextMenu *CDefView::_GetContextMenuFromSelection()
{
    IContextMenu *pcm = NULL;

    if (_HasNormalView())
    {
        if (m_cFrame.IsSFVExtension()) // IShellView Extended View
        {
            m_cFrame.GetExtendedISV()->GetItemObject(SVGIO_SELECTION, IID_IContextMenu, (void **) &pcm);
        }
        else    // normal large, small, list, details . . .
        {
            if (_pcmSel == NULL)
                DefView_GetUIObjectFromItem(this, IID_IContextMenu, (void **)&_pcmSel, SVGIO_SELECTION);

            if (_pcmSel)
            {
                pcm = _pcmSel;
                pcm->AddRef();
            }
        }
    }
    else // DocObject extended view
    {
        TraceMsg(TF_ERROR, "Extended View: GetContextMenuFromSelection has no active site!");
    }

    return pcm;
}

// If the browser has a Tree then we want to use explore.
UINT CDefView::_GetExplorerFlag()
{
    HWND hwnd;
    if (_psb && SUCCEEDED(_psb->GetControlWindow(FCW_TREE, &hwnd)) && hwnd)
        return CMF_EXPLORE;
    return 0;
}

#define DEFAULT_ATTRIBUTES  (DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET)
DWORD DefView_GetAttributesFromSelection(CDefView *pdsv, DWORD dwAttrMask)
{
    if (pdsv->_HasNormalView()) // normal views are non docobj extended views.
    {
        if (pdsv->m_cFrame.IsSFVExtension()) // IShellView extension
        {
            LPCITEMIDLIST *apidl;
            HRESULT hres;
            UINT cItems;

            // delegate to the IShellView2 extension
            hres = pdsv->m_cFrame.GetExtendedISFV()->GetSelectedObjects( &apidl, &cItems );
            if (FAILED(hres) || cItems == 0)
            {
                // BUGBUG REVIEW: we may want to mimic the BUGBUG else case above
                dwAttrMask = 0;
            }
            else
            {
                if (FAILED(pdsv->_pshf->GetAttributesOf(cItems, apidl, &dwAttrMask))) {
                    dwAttrMask = 0;
                }
                LocalFree((HLOCAL)apidl);
            }

            return dwAttrMask;
        }
        else    // large, small, list, details . . .
        {

            DWORD dwAttrQuery = DEFAULT_ATTRIBUTES | dwAttrMask;

            if ((pdsv->_dwAttrSel == (DWORD)-1) || (dwAttrQuery != DEFAULT_ATTRIBUTES))
            {
                LPCITEMIDLIST *apidl;
                UINT cItems;

                HRESULT hres = DefView_GetItemObjects(pdsv, &apidl, SVGIO_SELECTION, &cItems);

                //
                // this cache was written right before RC1 if you hit this after fix it
                //
                ASSERT(dwAttrQuery == DEFAULT_ATTRIBUTES);

                if (SUCCEEDED(hres))
                {
                    if (cItems)
                    {
                        if (SUCCEEDED(pdsv->_pshf->GetAttributesOf(
                            cItems, apidl, &dwAttrQuery)))
                        {
                            pdsv->_dwAttrSel = dwAttrQuery;
                        }

                        LocalFree((HLOCAL)apidl);
                    }
                    else
                    {
                        // mask out attrib bits here... if there's no selection, we can't
                        //  rename, delete, link, prop...

                        // BUGBUG: maybe this should be set to 0, but I'm afraid of what
                        // yanking SFGAO_FILESYS and other random stuff will do to us..
                        pdsv->_dwAttrSel &= ~(DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET);
                    }
                }
            }
            return (pdsv->_dwAttrSel & dwAttrMask);
        }
    }
    else // Extended View
    {
        TraceMsg(TF_WARNING, "Extended View: GetAttributesFromSelection has no active site!");
        return 0;
    }
}

//  DV_FlushCachedMenu flags
#define DVFCMF_SEL          0x00000001
#define DVFCMF_BACKGROUND   0x00000002
#define DVFCMF_ALL          0xFFFFFFFF

void DV_FlushCachedMenu(CDefView *pdsv, ULONG dwFlushFlags = DVFCMF_ALL)
{
    if (dwFlushFlags & DVFCMF_SEL)
    {
        // THIS FAULTS if you create two context menus in a row
        //IUnknown_SetSite(SAFECAST(pdsv->_pcmSel,IContextMenu*),NULL);
        ATOMICRELEASE(pdsv->_pcmSel);
    }

    if (pdsv->_uState != SVUIA_ACTIVATE_NOFOCUS)
    {
        if (dwFlushFlags & DVFCMF_BACKGROUND)
        {
            // We only need to collapse when the background is selected.
            // Collapse any menus before flushing the cache. This is for NT Bug#303716
            SendMessage(pdsv->_hwndView, WM_CANCELMODE, 0, 0);

            // THIS FAULTS if you create two context menus in a row
            //IUnknown_SetSite(SAFECAST(pdsv->_pcmBackground,IContextMenu*),NULL);
            ATOMICRELEASE(pdsv->_pcmBackground);
        }
    }
}


void CDefView::ContextMenu(DWORD dwPos)
{
    int iItem;
    int idCmd;
    int idDefault = -1;
    int nInsert;
    HMENU hmContext;
    UINT fFlags = 0;
    POINT pt;
    UINT iIDStart = SFVIDM_CONTEXT_FIRST;
    UINT iIDLast = SFVIDM_CONTEXT_LAST;

    if (SHRestricted(REST_NOVIEWCONTEXTMENU)) {
        return;
    }

    // if shell32's global copy of the stopwatch mode is not init'd yet, init it now.
    if(g_dwStopWatchMode == 0xffffffff)
        g_dwStopWatchMode = StopWatchMode();
    if(g_dwStopWatchMode)
    {
        StopWatch_Start(SWID_MENU, TEXT("Defview ContextMenu Start"), SPMODE_SHELL | SPMODE_DEBUGOUT);
    }

    if (IsWindowVisible(_hwndListview) && (IsChildOrSelf(_hwndListview, GetFocus()) == S_OK)) {
        // Find the selected item
        iItem = ListView_GetNextItem(_hwndListview, -1, LVNI_SELECTED);
    } else {
        iItem = -1;
    }

    if (dwPos == (DWORD) -1)
    {
        if (iItem != -1)
        {
            RECT rc;
            int iItemFocus = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED|LVNI_SELECTED);
            if (iItemFocus == -1)
                iItemFocus = iItem;

            //
            // Note that LV_GetItemRect returns it in client coordinate!
            //
            ListView_GetItemRect(_hwndListview, iItemFocus, &rc, LVIR_ICON);
            pt.x = (rc.left+rc.right)/2;
            pt.y = (rc.top+rc.bottom)/2;
        }
        else
        {
            pt.x = pt.y = 0;
        }
        MapWindowPoints(_hwndListview, HWND_DESKTOP, &pt, 1);
    }
    else
    {
        pt.x = GET_X_LPARAM(dwPos);
        pt.y = GET_Y_LPARAM(dwPos);
    }

    hmContext = CreatePopupMenu();
    if (!hmContext)
    {
        if(g_dwStopWatchMode)
            StopWatch_Stop(SWID_MENU, TEXT("Defview ContextMenu Stop (ERROR)"), SPMODE_SHELL | SPMODE_DEBUGOUT);
        // BUGBUG: There should be an error message here
        return;
    }

    LPARAM i;
    if (iItem == -1)
    {
        DECLAREWAITCURSOR;
        SetWaitCursor();

        // use the background context menu wrapper
        HRESULT hr = this->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu, (void **)&_pcmBackground);

        ResetWaitCursor();

        // set the max range for these, so that they are unaffected...
        iIDStart = 0;
        iIDLast = 0xffff;
        i = _IsDesktop() ? UIBL_CTXTDESKBKGND : UIBL_CTXTDEFBKGND;
    }
    else
    {
        fFlags |= CMF_CANRENAME;

        nInsert = 0;

        // One or more items are selected, let the folder add menuitems.
        _pcmBackground = _GetContextMenuFromSelection();
        i = _IsDesktop() ? UIBL_CTXTDESKITEM : UIBL_CTXTDEFITEM;
    }

    UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_UICONTEXT, i);

    if (IsSafeToDefaultVerb())
    {
        ICommDlgBrowser2 *pcdb2;

        // Get the ICommDlgBrowser2 interface.
        _psb->QueryInterface(IID_ICommDlgBrowser2, (void **)&pcdb2);

        if (_pcmBackground)
        {
            fFlags |= _GetExplorerFlag();

            IUnknown_SetSite(_pcmBackground, SAFECAST(this,IShellView2*));

            if (0 > GetKeyState(VK_SHIFT))
                fFlags |= CMF_EXTENDEDVERBS;
                
            _pcmBackground->QueryContextMenu(hmContext, nInsert, iIDStart, iIDLast, fFlags);

            // If this is the common dialog browser, we need to make the
            // default command "Select" so that double-clicking (which is
            // open in common dialog) makes sense.
            if (DV_CDB_IsCommonDialog(this))
            {
                // make sure this is an item
                if (iItem != -1)
                {
                    HMENU hmSelect = SHLoadPopupMenu(HINST_THISDLL, POPUP_COMMDLG_POPUPMERGE);

                    // If we have a pointer to the ICommDlgBrowser2 interface
                    // query if this interface wants to change the text of the
                    // default verb.  This interface is needed in the common print
                    // dialog to change the default text from 'Select' to 'Print'.
                    if (pcdb2)
                    {
                        WCHAR szTextW[MAX_PATH] = {0};

                        if (pcdb2->GetDefaultMenuText(this, szTextW, ARRAYSIZE(szTextW)) == S_OK)
                        {
                            INT iRetval = TRUE;
                            LPTSTR pszText = NULL;
#ifdef UNICODE
                            pszText = szTextW;
#else
                            CHAR szText[MAX_PATH] = {0};
                            iRetval = SHUnicodeToAnsi( szTextW, szText, ARRAYSIZE(szText));
                            pszText = szText;
#endif
                            if( iRetval )
                            {
                                MENUITEMINFO mi = {0};
                                mi.cbSize       = sizeof(mi);
                                mi.fMask        = MIIM_TYPE;
                                mi.fType        = MFT_STRING;
                                mi.dwTypeData   = pszText;
                                SetMenuItemInfo(hmSelect, 0, MF_BYPOSITION, &mi);
                            }
                        }
                    }

                    // NOTE: Since commdlg always eats the default command,
                    // we don't care what id we assign hmSelect, as long as it
                    // doesn't conflict with any other context menu id.
                    // SFVIDM_CONTEXT_FIRST-1 won't conflict with anyone.
                    Shell_MergeMenus(hmContext, hmSelect, 0,
                                    (UINT)(SFVIDM_BACK_CONTEXT_FIRST-1), (UINT)-1,
                                    MM_ADDSEPARATOR);

                    SetMenuDefaultItem(hmContext, 0, MF_BYPOSITION);
                    DestroyMenu(hmSelect);
                }
            }

            idDefault = GetMenuDefaultItem(hmContext, MF_BYCOMMAND, 0);
        }

        _SHPrettyMenu(hmContext);


        // If this is the common dialog browser 2, we need inform it
        // the context menu is has started.  This notifiction is use in
        // the common print dialog on NT which hosts the printers folder.
        // Common dialog want to relselect the printer object if the user
        // selected the context menu from the background.
        if (pcdb2)
        {
            pcdb2->Notify(this, CDB2N_CONTEXTMENU_START);
        }

        // Keep this code just before the TPM call
        if(g_dwStopWatchMode)
            StopWatch_Stop(SWID_MENU, TEXT("Defview ContextMenu Stop"), SPMODE_SHELL | SPMODE_DEBUGOUT);

        idCmd = TrackPopupMenu(hmContext,
            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
            pt.x, pt.y, 0, _hwndView, NULL);
        INSTRUMENT_TRACKPOPUPMENU(SHCNFI_DEFVIEWX_TPM, _hwndView, idCmd);

        if ((idCmd == idDefault) &&
            DV_CDB_OnDefaultCommand(this) == S_OK)
        {
            // commdlg browser ate the default command
        }
        else if (idCmd == 0)
        {
            // No item selected
        }
        else if ( iItem == -1 )
        {
            // we are using the background context menu wrapper, so let it do the
            // right thing by calling Invoke command

            if (_pcmBackground)
            {
                CMINVOKECOMMANDINFO rgCmd;

                ZeroMemory( &rgCmd, sizeof( rgCmd ));
                rgCmd.cbSize = sizeof( rgCmd );
                rgCmd.lpVerb = (LPCSTR)UIntToPtr( idCmd );

                _pcmBackground->InvokeCommand( & rgCmd );
            }
        }
        else
        {
            // A default menu item was selected; let everybody
            // process it normally
            // Note that this must be called before clearing out _pcmSel,
            // so we cannot do this with a PostMessage
            this->Command(_pcmBackground, GET_WM_COMMAND_MPS(idCmd, 0, 0));
        }

        // If this is the common dialog browser 2, we need inform it
        // the context menu is done.  This notifiction is use in
        // the common print dialog on NT which hosts the printers folder.
        // Common dialog want to relselect the printer object if the user
        // selected the context menu from the background.
        if (pcdb2)
        {
            pcdb2->Notify(this, CDB2N_CONTEXTMENU_DONE);
            pcdb2->Release();
        }

        IUnknown_SetSite(_pcmBackground, NULL);
    }
    else
    {
        // not IsSafeToDefaultVerb(), so we didn't show the cm, so we didn't stop the timer
        if (g_dwStopWatchMode)
            StopWatch_Stop(SWID_MENU, TEXT("Defview ContextMenu Stop (!SafeToDefaultVerb)"), SPMODE_SHELL | SPMODE_DEBUGOUT);
    }

    ATOMICRELEASE(_pcmBackground);

    MSG msg;
    if (PeekMessage(&msg, _hwndMain, WM_DSV_MENUTERM, WM_DSV_MENUTERM, PM_REMOVE))
    {
        // _OnMenuTermination no longer calls DV_FlushCachedMenu so
        // call it manually
        DV_FlushCachedMenu(this);
        _OnMenuTermination();
    }

    DestroyMenu(hmContext);
}

void CALLBACK DefView_GetDataPoint(LPCITEMIDLIST pidl, LPPOINT ppt, LPARAM lParam)
{
    CDefView *pdsv = (CDefView *)lParam;

    if (pidl)
        pdsv->_GetItemPosition(pidl, ppt);
    else
        DefView_GetDragPoint(pdsv, ppt);
}

void DefView_SetViewMode(CDefView *pdsv, UINT fvmNew, DWORD dwStyle)
{
    pdsv->_fs.ViewMode = fvmNew;
    pdsv->_dvState.iDirection = 1;
    SetWindowBits(pdsv->_hwndListview, GWL_STYLE, LVS_TYPEMASK, dwStyle);
}

BOOL DV_GetItemSpacing(CDefView *pdsv, LPITEMSPACING lpis)
{
    DWORD dwSize;

    dwSize = ListView_GetItemSpacing(pdsv->_hwndListview, TRUE);
    lpis->cxSmall = GET_X_LPARAM(dwSize);
    lpis->cySmall = GET_Y_LPARAM(dwSize);
    dwSize = ListView_GetItemSpacing(pdsv->_hwndListview, FALSE);
    lpis->cxLarge = GET_X_LPARAM(dwSize);
    lpis->cyLarge = GET_Y_LPARAM(dwSize);

    return (pdsv->_fs.ViewMode != FVM_ICON);
}

void DefView_SetPoints(CDefView *pdsv, IDataObject *pdtobj)
{
    SCALEINFO si;

    // convert coordinates to large icon view spacing
    if (pdsv->_fs.ViewMode == FVM_ICON) {
        si.xMul = si.yMul = si.xDiv = si.yDiv = 1;
    } else {
        ITEMSPACING is;

        DV_GetItemSpacing(pdsv, &is);

        si.xDiv = is.cxSmall;
        si.yDiv = is.cySmall;
        si.xMul = is.cxLarge;
        si.yMul = is.cyLarge;
    }

    // Assuming this is a CIDLData thing, poke the icon
    // locations into it
    DataObj_SetPoints(pdtobj, DefView_GetDataPoint, (LPARAM)pdsv, &si);
}

BOOL _DidDropOnRecycleBin(IDataObject *pdtobj)
{
    CLSID clsid;
    return SUCCEEDED(DataObj_GetDropTarget(pdtobj, &clsid)) &&
           IsEqualCLSID(clsid, CLSID_RecycleBin);
}

//
// REVIEW: Currently, we are not doing any serialization assuming that
//  only one GUI thread can come here at a time.
//
LRESULT CDefView::_OnBeginDrag(NM_LISTVIEW * pnm)
{
    POINT ptOffset = pnm->ptAction;             // hwndLV client coords

    // This DefView is used as a drag source so we need to see if it's
    // is hosted by something that can disguise the action.
    if (S_OK != _ZoneCheck(PUAF_NOUI, URLACTION_SHELL_WEBVIEW_VERB))
    {
        // This DefView is hosted in HTML, so we need to turn off the
        // ability of this defview from being a drag source.
        return 0;
    }

    if (_bAutoSelChangeTimerSet)
    {   // Hurry up the sel change notification to the automation object
        KillTimer(_hwndView, DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE);
        _bAutoSelChangeTimerSet = FALSE;
        NotifyAutomation(DISPID_SELECTIONCHANGED);
    }
    NotifyAutomation(DISPID_VERBINVOKED);

    //
    // Get the dwEffect from the selection.
    //
    DWORD dwEffect = DefView_GetAttributesFromSelection(this, SFGAO_CANDELETE | DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY);
    // Turn on DROPEFFECT_MOVE for any deleteable item
    // (this is so the item can be dragged to the recycle bin)
    if (SFGAO_CANDELETE & dwEffect)
    {
        dwEffect |= DROPEFFECT_MOVE;
    }
    // Mask out all attribute bits that aren't also DROPEFFECT bits:
    dwEffect &= (DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY);

    // Somebody began dragging in our window, so store that fact
    _bDragSource = TRUE;

    // save away the anchor point;
    _ptDragAnchor = pnm->ptAction;
    LVUtil_ClientToLV(_hwndListview, &_ptDragAnchor);

    ClientToScreen(_hwndListview, &ptOffset);     // now in screen

    IDataObject *pdtobj;
    if (SUCCEEDED(DefView_GetUIObjectFromItem(this, IID_IDataObject, (void **)&pdtobj, SVGIO_SELECTION)))
    {
        // Give the source a chance to alter the drop effect.
        CallCB(SFVM_ALTERDROPEFFECT, (WPARAM)&dwEffect, (LPARAM)pdtobj);
    
        if (DAD_SetDragImageFromWindow(_hwndListview, &ptOffset, pdtobj))
        {
            DefView_SetPoints(this, pdtobj);

            if (DRAGDROP_S_DROP == SHDoDragDrop(_hwndMain, pdtobj, NULL, dwEffect, &dwEffect))
            {
                if (S_OK != CallCB(SFVM_DIDDRAGDROP, (WPARAM)dwEffect, (LPARAM)pdtobj))
                {
                    // the return of DROPEFFECT_MOVE tells us we need to delete the data
                    // see if we need to do that now...

                    // NOTE: we can't trust the dwEffect return result from DoDragDrop() because
                    // some apps (adobe photoshop) return this when you drag a file on them that
                    // they intend to open. so we invented the "PreformedEffect" as a way to
                    // know what the real value is, that is why we test both of these.

                    if ((DROPEFFECT_MOVE == dwEffect) &&
                        (DROPEFFECT_MOVE == DataObj_GetDWORD(pdtobj, g_cfPerformedDropEffect, DROPEFFECT_NONE)))
                    {
                        // enable UI for the recycle bin case (the data will be lost
                        // as the recycle bin really can't recycle stuff that is not files)

                        UINT uFlags = _DidDropOnRecycleBin(pdtobj) ? 0 : CMIC_MASK_FLAG_NO_UI;
                        InvokeVerbOnDataObj(_hwndMain, c_szDelete, uFlags, pdtobj);
                    }
                }
            }

            //
            // We need to clear the dragged image only if we still have the drag context.
            //
            DAD_SetDragImage((HIMAGELIST)-1, NULL);
        }
        pdtobj->Release();
    }
    _bDragSource = FALSE;  // All done dragging
    return 0;
}

void CDefView::_FocusOnSomething(void)
{
    int iFocus = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED);
    if (iFocus == -1) 
    {
        if (ListView_GetItemCount(_hwndListview) > 0)
        {
            // set the focus on the first item.
            ListView_SetItemState(_hwndListview, 0, LVIS_FOCUSED, LVIS_FOCUSED);
        }
    }
}

#define DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE  4
#define NOTIFY_AUTOMATION_SELCHANGE_TIMEOUT     (GetDoubleClickTime())

HRESULT CDefView::_InvokeCommand(IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici)
{
    TCHAR szWorkingDir[MAX_PATH];
#ifdef UNICODE
    CHAR szWorkingDirAnsi[MAX_PATH];
#endif

    if (SUCCEEDED(CallCB(SFVM_GETWORKINGDIR, ARRAYSIZE(szWorkingDir), (LPARAM)szWorkingDir)))
    {
#ifdef UNICODE
        // Fill in both the ansi working dir and the unicode one
        // since we don't know who's gonna be processing this thing.
        SHUnicodeToAnsi(szWorkingDir, szWorkingDirAnsi, ARRAYSIZE(szWorkingDirAnsi));
        pici->lpDirectory  = szWorkingDirAnsi;
        pici->lpDirectoryW = szWorkingDir;
        pici->fMask |= CMIC_MASK_UNICODE;
#else
        pici->lpDirectory = szWorkingDir;
#endif
    }

    // In case the ptInvoke field was not already set for us, guess where
    // that could be. BUGBUG: (dli) maybe should let the caller set all points
    if (!(pici->fMask & CMIC_MASK_PTINVOKE))
    {
        POINT ptFocus;
        if (GetCursorPos(&ptFocus))
        {
            pici->ptInvoke = ptFocus;
            pici->fMask |= CMIC_MASK_PTINVOKE;
        }
    }

    if (_bAutoSelChangeTimerSet)
    {   // Hurry up the sel change notification to the automation object
        KillTimer(_hwndView, DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE);
        _bAutoSelChangeTimerSet = FALSE;
        NotifyAutomation(DISPID_SELECTIONCHANGED);
    }
    NotifyAutomation(DISPID_VERBINVOKED);
    return pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)pici);
}

DWORD CDefView::_GetNeededSecurityAction(void)
{
    DWORD dwUrlAction = 0;
    IUnknown * punk;

    ASSERT(_psb);

    // If we are hosted by Trident, Zone Check Action.
    if (SUCCEEDED(_psb->QueryInterface(IID_IIsWebBrowserSB, (void **)&punk)))
    {
        dwUrlAction = URLACTION_SHELL_VERB;
        ATOMICRELEASE(punk);
    }
    else if (_fGetWindowLV)
    {
        // If we are using WebView, Zone Check Action.
        dwUrlAction = URLACTION_SHELL_WEBVIEW_VERB;
    }

    return dwUrlAction;
}

HRESULT CDefView::_ZoneCheck(DWORD dwFlags, DWORD dwAllowAction)
{
    HRESULT hr = S_OK;
    DWORD dwUrlAction = _GetNeededSecurityAction();

    ASSERT(_psb);

    if (dwUrlAction && (dwUrlAction != dwAllowAction))
    {
        IInternetHostSecurityManager * pihsm;

        // First check if our parent wants to generate our context (Zone/URL).
        hr = IUnknown_QueryService(_psb, IID_IInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pihsm);

        // Did that fail?
        if (FAILED(hr) && m_cFrame.m_pDocView)
        {
            if (!_fGetWindowLV)
                TraceMsg(TF_DEFVIEW, "We will fall back to Zone Checking the PIDL instead of the zone our host generates.");  // We should only get here in the plain WebView case.

            // Yes, so if we are in WebView mode, check the instance of Trident that is
            // displaying the WebView content, because that content could discuise the DefView
            // and make the user unknowingly do something bad.
            hr = IUnknown_QueryService(m_cFrame.m_pDocView, IID_IInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pihsm);
        }

        // Were we able to get an IInternetHostSecurityManager interface?
        if (EVAL(SUCCEEDED(hr)))
        {
            // This is the prefered way to do the zone check.

            hr = ZoneCheckHost(pihsm, dwUrlAction, dwFlags | PUAF_FORCEUI_FOREGROUND);
            ATOMICRELEASE(pihsm);
        }
        else
        {
            TCHAR szPathSource[MAX_PATH];
            // No, we were not able to get the interface.  So fall back to zone checking the
            // URL that comes from the pidl we are at.

            if (_GetPath(szPathSource))
            {
                IInternetSecurityMgrSite * pisms;

                // Try to get a IInternetSecurityMgrSite so our UI will be modal.
                if (SUCCEEDED(IUnknown_QueryService(_psb, SID_STopLevelBrowser, IID_IInternetSecurityMgrSite, (void**)&pisms)))
                {
                    // TODO: Have this object support IInternetSecurityMgrSite in case our parent doesn't provide one.
                    //       Make that code support ::GetWindow() and ::EnableModless() or we won't get the modal behavior
                    //       needed for VB and AOL.

                    hr = ZoneCheckUrl(szPathSource, dwUrlAction, dwFlags | PUAF_ISFILE | PUAF_FORCEUI_FOREGROUND, pisms);
                    pisms->Release();
                }
            }
        }
    }

    return hr;
}


BOOL CDefView::IsSafeToDefaultVerb(void)
{
    ASSERT(_psb);
    return S_OK == _ZoneCheck(PUAF_WARN_IF_DENIED, 0);
}

//----------------------------------------------------------------------------
#define CMD_ID_FIRST    1
#define CMD_ID_LAST     0x7fff

void CDefView::_ProcessDblClick(LPNMITEMACTIVATE pnmia)
{
    // Use the cached context menu object if there is one, else
    // make it.
    IContextMenu *pcmSel;
    // SHIFT invokes the "alternate" command (usually Print)
    int iShowCmd = SW_NORMAL;
    DECLAREWAITCURSOR;

    if (_IsDesktop())
        UEMFireEvent(&UEMIID_SHELL, UEME_UISCUT, UEMF_XEVENT, -1, (LPARAM)-1);

    if (DV_CDB_OnDefaultCommand(this) == S_OK)
    {
        return;         /* commdlg browser ate the message */
    }
    SetWaitCursor();
    pcmSel = _GetContextMenuFromSelection();

    if (pcmSel)
    {
        CMINVOKECOMMANDINFOEX ici;

        ZeroMemory(&ici, SIZEOF(ici));
        ici.cbSize = SIZEOF(ici);
        ici.hwnd = _hwndMain;
        ici.nShow = iShowCmd;

        // Get the point where the double click is invoked.
        GetMsgPos(&ici.ptInvoke);
        ici.fMask |= CMIC_MASK_PTINVOKE;

        // record if shift or control was being held down
        SetICIKeyModifiers(&ici.fMask);

        // Security note: we assume "properties" is a safe action.
        if (pnmia->uKeyFlags & LVKF_ALT)
        {
#ifdef UNICODE
            // Fill in both the ansi verb and the unicode verb since we
            // don't know who is going to be processing this thing.
            ici.lpVerb = "properties";
            ici.lpVerbW = c_szProperties;
            ici.fMask |= CMIC_MASK_UNICODE;
#else
            ici.lpVerb = c_szProperties;
#endif
            // If ALT double click, accelerator for "Properties..."

            // need to reset it so that user won't blow off the app starting  cursor
            // also so that if we won't leave the wait cursor up when we're not waiting
            // (like in a prop sheet or something that has a message loop
            ResetWaitCursor();
            hcursor_wait_cursor_save = NULL;

            INSTRUMENT_STATECHANGE(SHCNFI_STATE_DEFVIEWX_ALT_DBLCLK);

            _InvokeCommand(pcmSel, &ici);
        }
        else if (IsSafeToDefaultVerb())
        {
            HMENU hmenu = CreatePopupMenu();
            if (hmenu)
            {
                UINT idCmd;
                UINT fFlags = CMF_DEFAULTONLY;

                fFlags |= _GetExplorerFlag();

                //
                //  SHIFT+dblclick does a Explore by default
                //
                if (pnmia->uKeyFlags & LVKF_SHIFT)
                {
                    fFlags |= CMF_EXPLORE;
                }

                pcmSel->QueryContextMenu(hmenu, 0, CMD_ID_FIRST, CMD_ID_LAST,
                                                 fFlags);

                idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, GMDI_GOINTOPOPUPS);

                // need to reset it so that user won't blow off the app starting  cursor
                // also so that if we won't leave the wait cursor up when we're not waiting
                // (like in a prop sheet or something that has a message loop
                ResetWaitCursor();
                hcursor_wait_cursor_save = NULL;
                if (idCmd != -1)
                {
                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CMD_ID_FIRST);
                    _InvokeCommand(pcmSel, &ici);
                }
                DestroyMenu(hmenu);
            }
        }
        // Release our use of the context menu
        if (pcmSel == _pcmSel)
        {
            DV_FlushCachedMenu(this);
        }
        pcmSel->Release();
    }

    if (hcursor_wait_cursor_save)
        ResetWaitCursor();
}

void CDefView::_UpdateColData(CBackgroundColInfo *pbgci)
{
    UINT iItem = _FindItem(pbgci->GetPIDL(), NULL, FALSE);
    if (iItem != -1)
    {
        UINT    uiCol = pbgci->GetColumn();

        if (IsColumnOn(uiCol))
        {
            UINT iVisCol;
            MapRealToVisibleColumn(uiCol, &iVisCol);

            ListView_SetItemText(_hwndListview, iItem, iVisCol, const_cast<TCHAR*>(pbgci->GetText()));
        }
    }

    delete pbgci;
}

void CDefView::_UpdateIcon(CDVGetIconTask * pTask)
{
    ASSERT(pTask);
    ASSERT(pTask->_pidl);

    if (!pTask->_fUsed)
    {
        int i = _FindItem(pTask->_pidl, NULL, FALSE);

        InterlockedDecrement(&_AsyncIconCount);

        DebugMsg(TF_ICON, TEXT("async icon done: pidl=%08X i=%d icon=%d count=%d"), pTask->_pidl, i, pTask->_iIcon, _AsyncIconCount);

        if (i >= 0)
        {
            LV_ITEM item;

            item.mask = LVIF_IMAGE;
            item.iItem = i;
            item.iImage = pTask->_iIcon;
            item.iSubItem = 0;

            ListView_SetItem(_hwndListview, &item);
        }
    }
    ATOMICRELEASE(pTask);
}

void CDefView::_UpdateOverlay(int iList, int iOverlay)
{
    ASSERT (iList >= 0);
    
    if (_IsOwnerData())
    {
        // In the ownerdata case, tell the owner that the overlay changed
        CallCB(SFVM_SETICONOVERLAY, iList, iOverlay);
        ListView_RedrawItems(_hwndListview, iList, iList);
    }
    else
        ListView_SetItemState(_hwndListview, iList, INDEXTOOVERLAYMASK(iOverlay), LVIS_OVERLAYMASK);
}

HRESULT CDefView::_GetIconAsync(LPCITEMIDLIST pidl, int *piIcon, BOOL fCanWait)
{
    HRESULT hres;

#ifdef MAX_ICON_WAIT
    //
    // get the time here so we include the time it took to load handlers etc.
    //
    LONG time = GetTickCount();
#endif

    //
    // if we are not an owner-data view then try to extract asynchronously
    //
    UINT flags = (_IsOwnerData() ? 0 : GIL_ASYNC);

again:
    hres = SHGetIconFromPIDL(_pshf, _psi, pidl, flags, piIcon);

    if (SUCCEEDED(hres))
        return S_OK;        // indicate that we got the real icon

    if (hres == E_PENDING && (flags & GIL_ASYNC))
    {
        hres = S_FALSE;     // the icon index we have is a placeholder

        DebugMsg(TF_ICON, TEXT("SHGetIconFromPIDL returns E_PENDING for pidl=%08X count=%d."), pidl, _AsyncIconCount);

#ifdef MAX_ICON_WAIT
        LONG wait;

        //
        // if this is the first async icon, setup to wait for the
        // icon to finish extracting.
        //
        if (fCanWait && (_AsyncIconCount == 0))
        {
            if ((time - _AsyncIconTime) >= MIN_ICON_WAIT)
            {
                _AsyncIconTime = time;
            }

            //
            // query the time again in case SHGetIconFromPIDL went out to lunch
            // in an icon handler.
            //
            time = GetTickCount();

            wait = MAX_ICON_WAIT - (time - _AsyncIconTime);

            if (_AsyncIconEvent == 0)
            {
                _AsyncIconEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

                if (_AsyncIconEvent == 0)
                    wait = 0;
            }
            else if (wait > 0)
            {
                ResetEvent(_AsyncIconEvent);
            }
        }
        else
        {
            wait = 0;
        }
#endif

        IRunnableTask * pTask = NULL;
        CDVGetIconTask * pIconTask;
        hres = CDVGetIconTask_CreateInstance( this, pidl, &pTask, &pIconTask );
        if ( SUCCEEDED( hres ))
        {
            InterlockedIncrement(&_AsyncIconCount);

            BOOL fRes = DefView_IdleDoStuff(this, pTask,
                                            TOID_DVIconExtract,
                                            0, TASK_PRIORITY_GET_ICON);
            if ( !fRes )
            {
                InterlockedDecrement(&_AsyncIconCount);
                ATOMICRELEASE( pTask );
                flags &= ~GIL_ASYNC;
                goto again;
            }
        }
        else
        {
            DebugMsg(TF_ICON, TEXT("Failed to create CDVGetIconTask!"));

            flags &= ~GIL_ASYNC;
            goto again;
        }

        // currently we think we are Async...
        hres = S_FALSE;
        
#ifdef MAX_ICON_WAIT
        if (wait > 0)
        {
            DebugMsg(TF_ICON, TEXT("Waiting for icon... base=%d time=%d wait=%d"), _AsyncIconTime, time, wait);

            DWORD err = WaitForSingleObject(_AsyncIconEvent, wait);

            if (err == WAIT_TIMEOUT)
            {
                DebugMsg(TF_ICON, TEXT("**** timeout waiting for icon."));
            }
            else if (err == WAIT_OBJECT_0)
            {
                // The data in "paid" is valid, and we have a
                // WM_DSV_UPDATEICON message in our queue.
                //
                // We can't remove the message from our queue,
                // because that would allow SendNotifyEvents through
                // and potentially destroy the very item we are
                // processing.  Instead we mark the "paid" strucure
                // as "used", so when we get the message we can ignore it.

                pIconTask->_fUsed = TRUE;
                InterlockedDecrement(&_AsyncIconCount);
                *piIcon = pIconTask->_iIcon;

                hres = S_OK;
            }
            else
            {
                DebugMsg(TF_ICON, TEXT("error %d waiting for icon."), err);
            }
        }
#endif
        ASSERT( pTask );
        ATOMICRELEASE( pTask );
    }

    return hres;
}

HRESULT CDefView::_GetOverlayIndexAsync(LPCITEMIDLIST pidl, int iList)
{
    IRunnableTask * pTask = NULL;

    HRESULT hres = CDVIconOverlayTask_CreateInstance( this, pidl, iList, &pTask );
    if ( SUCCEEDED( hres ))
    {
        BOOL fRes = DefView_IdleDoStuff(this, pTask,
                                        TOID_DVIconOverlay,
                                        0, TASK_PRIORITY_GET_ICON);
        ATOMICRELEASE( pTask );
    }

    return hres;
}

//
// Returns: if the cursor is over a listview item, its index; otherwise, -1.
//
int DV_HitTest(CDefView *pdsv, const POINT *ppt)
{
    LV_HITTESTINFO info;

    if (!DV_SHOWICONS(pdsv))
        return -1;

    info.pt = *ppt;
    return ListView_HitTest(pdsv->_hwndListview, &info);
}

typedef void (*PFNGETINFOTIP)(LPVOID, LPTSTR, int);

void _AppendInfoTip(PFNGETINFOTIP pfn, LPVOID pv, NMLVGETINFOTIP *plvn)
{
    BOOL fSuccess = FALSE;

    // If the string is unfolded, then we display just the tip
    // If the string is folded, append the tip to the name
    int cchName = (plvn->dwFlags & LVGIT_UNFOLDED) ? 0 : lstrlen(plvn->pszText);

    if (cchName)
    {
        lstrcatn(plvn->pszText, TEXT("\r\n"), plvn->cchTextMax);
        cchName = lstrlen(plvn->pszText);
    }

    // If there is room in the buffer after we added CRLF, append the
    // infotip text.  We succeeded if there was nontrivial infotip text.

    if (cchName < plvn->cchTextMax)
    {
        pfn(pv, plvn->pszText + cchName, plvn->cchTextMax - cchName);
        fSuccess = plvn->pszText[cchName];
    }

    // If we didn't get an infotip, then null out the buffer so listview
    // will fall back to the default "inplace tooltip" method.
    if (!fSuccess)
        plvn->pszText[0] = TEXT('\0');
}

void _AppendInfoTipFromString(LPVOID pv, LPTSTR pszBuf, int cch)
{
    LPWSTR pwsz = (LPWSTR)pv;
    SHUnicodeToTChar(pwsz, pszBuf, cch);
}

void _AppendInfoTipFromVariant(LPVOID pv, LPTSTR pszBuf, int cch)
{
    VARIANT *pvar = (VARIANT *)pv;
    VariantToStr(pvar, pszBuf, cch);
}

void CDefView::_OnGetInfoTip(NMLVGETINFOTIP *plvn)
{
    if (!ShowInfoTip())
        return;

    LPITEMIDLIST pidl = _GetPIDL(plvn->iItem);
    if (pidl)
    {
        IQueryInfo *pqi;
        LPCITEMIDLIST apidl[1] = { pidl };

        if (SUCCEEDED(_pshf->GetUIObjectOf(NULL, 1, apidl, IID_IQueryInfo, NULL, (void**)&pqi)))
        {
            WCHAR *pwszTip;

            if (SUCCEEDED(pqi->GetInfoTip(0, &pwszTip)) && pwszTip)
            {
                _AppendInfoTip(_AppendInfoTipFromString, pwszTip, plvn);
                SHFree(pwszTip);
            }
            pqi->Release();
        }
        else if (_pshf2)
        {
            VARIANT var;
            VariantInit(&var);
            if (SUCCEEDED(_pshf2->GetDetailsEx(pidl, &SCID_Comment, &var)))
            {
                _AppendInfoTip(_AppendInfoTipFromVariant, &var, plvn);
                VariantClear(&var);
            }
        }
    }
}

HRESULT CDefView::_OnViewWindowActive()
{
    IShellView *psv = _psvOuter ? _psvOuter : SAFECAST(this, IShellView*);

    return _psb->OnViewWindowActive(psv);
}

#ifdef WINNT

// CLR_NONE is a special value that never matches a valid RGB
COLORREF g_crAltColor = CLR_NONE;   // uninitialized magic value

DWORD GetAltColor()
{
    // Fetch the alternate color (for compression) if supplied.
    if (g_crAltColor == CLR_NONE)   // initialized yet?
    {
        DWORD cbData = sizeof(COLORREF);
        DWORD dwType;
        HKEY hkey;

        hkey= SHGetExplorerHkey(HKEY_CURRENT_USER,TRUE);
        if( hkey ) {
            if (SHQueryValueEx(hkey,  TEXT("AltColor"), NULL, &dwType, (LPBYTE)&g_crAltColor, &cbData) != ERROR_SUCCESS)
                g_crAltColor = RGB(0, 0, 255);  // default value
        RegCloseKey(hkey);
        }
    }
    return g_crAltColor;
}
#endif

LRESULT CDefView::_OnLVNotify(NM_LISTVIEW *plvn)
{
    switch (plvn->hdr.code) {
    case NM_KILLFOCUS:
        // force update on inactive to not ruin save bits
        DV_CDB_OnStateChange(this, CDBOSC_KILLFOCUS);
        if (GetForegroundWindow() != _hwndMain)
            UpdateWindow(_hwndListview);
        _fHasListViewFocus = FALSE;
        _EnableDisableTBButtons();
        break;

    case NM_SETFOCUS:
    {
        if (m_cFrame.IsWebView())   // Do OLE stuff
        {
            UIActivate(SVUIA_ACTIVATE_FOCUS);
        }
        else
        {
            //  We should call IShellBrowser::OnViewWindowActive() before
            // calling its InsertMenus().
            _OnViewWindowActive();
            DV_CDB_OnStateChange(this, CDBOSC_SETFOCUS);
            OnActivate(SVUIA_ACTIVATE_FOCUS);
            _FocusOnSomething();
            DV_UpdateStatusBar(this, FALSE);
        }
        _fHasListViewFocus = TRUE;
        _EnableDisableTBButtons();
        break;
    }

    case NM_RCLICK:
    {
        // on the shift+right-click case we want to deselect everything and select just our item if it is 
        // not already selected. if we dont do this, then listview gets confused (because he thinks
        // shift means extend selection, but in the right click case it dosent!) and will bring up the
        // context menu for whatever is currently selected instead of what the user just right clicked on.
        if ((GetKeyState(VK_SHIFT) < 0) &&
            (plvn->iItem >= 0)          &&
            !(ListView_GetItemState(_hwndListview, plvn->iItem, LVIS_SELECTED) & LVIS_SELECTED))
        {
            // clear any currently slected items
            ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);

            // select the guy that was just right-clicked on
            ListView_SetItemState(_hwndListview, plvn->iItem, LVIS_SELECTED, LVIS_SELECTED);
        }
        break;
    }


    case LVN_GETINFOTIP:
        _OnGetInfoTip((NMLVGETINFOTIP *)plvn);
        break;

    case LVN_ITEMACTIVATE:
        if (!_fDisabled)
        {
            //in win95 if user left clicks on one click activate icon and then right
            //clicks on it (within double click time interval), the icon is launched
            //and context menu appears on top of it -- it does not disappear.
            //furthermore the context menu cannot be destroyed but stays on top of
            //any window and items on it are not accessible. to avoid this
            //send cancel mode to itself to destroy context before the icon is
            //launched
            if (_hwndView)
                SendMessage(_hwndView, WM_CANCELMODE, 0, 0);
            _ProcessDblClick((LPNMITEMACTIVATE)plvn);
        }
        break;

#ifdef WINNT
    case NM_CUSTOMDRAW:
        {
        LPNMLVCUSTOMDRAW lpCD = (LPNMLVCUSTOMDRAW)plvn;

        switch (lpCD->nmcd.dwDrawStage) {

        case CDDS_PREPAINT:
            if (_fShowCompColor) {
                return CDRF_NOTIFYITEMDRAW;
            } else {
                return CDRF_DODEFAULT;
            }
            break;

        case CDDS_ITEMPREPAINT:
            {
                LPCITEMIDLIST pidl = DV_GetPIDLParam(this, lpCD->nmcd.lItemlParam, (int)lpCD->nmcd.dwItemSpec);
                if (pidl)
                {
                    DWORD uFlags = SFGAO_COMPRESSED;
                    HRESULT hres = _pshf->GetAttributesOf(1, &pidl, &uFlags);

                    if (SUCCEEDED(hres) && (uFlags & SFGAO_COMPRESSED))
                    {
                        lpCD->clrText = GetAltColor();
                    }
                }

                return CDRF_DODEFAULT;
            }
        }
        }
        return CDRF_DODEFAULT;
#endif

    case LVN_BEGINDRAG:
    case LVN_BEGINRDRAG:
        if (_fDisabled)
            return FALSE;   /* commdlg doesn't want user dragging */
        return _OnBeginDrag(plvn);

    case LVN_ITEMCHANGING:
        if (_fDisabled)
            return TRUE;
        break;

    // Something changed in the listview.  Delete any data that
    // we might have cached away.
    //
    case LVN_ITEMCHANGED:
        // We only care about STATECHANGE messages
        if (!(plvn->uChanged & LVIF_STATE))
        {
            // If the text is changed, we need to flush the cached
            // selected item context menu.  We'll leave the background menu
            // around only if it is currently being displayed.
            if (plvn->uChanged & LVIF_TEXT)
            {
                ULONG dwFlush = DVFCMF_SEL ;  // always flush selitem cm.

                // conditional fixes bug 157347; flushing
                // _pcmBackground if the menu was being displayed would
                // hose up submenus and create other side effects.
                if( !_bContextMenuMode )
                    dwFlush |= DVFCMF_BACKGROUND ;
                    
                DV_FlushCachedMenu( this, dwFlush );
            }
            break;
        }

        // The rest only cares about SELCHANGE messages (avoid LVIS_DRAGSELECT, etc)
        if ((plvn->uNewState ^ plvn->uOldState) & (LVIS_SELECTED | LVIS_FOCUSED))
        {
            //if we are the drag source then dont send selection change message 
            if (!_bDragSource)
            {
                DV_CDB_OnStateChange(this, CDBOSC_SELCHANGE);
            }

            if( (plvn->uNewState ^ plvn->uOldState) & LVIS_SELECTED )
            {
                this->OnLVSelectionChange(plvn);
            }
        }        
        break;

        
    // owner data state changed: e.g. search results
    case LVN_ODSTATECHANGED:
        {
            NM_ODSTATECHANGE *pnm = (NM_ODSTATECHANGE *)plvn;

            // for now handle only selection changes
            if ((pnm->uOldState ^ pnm->uNewState) & LVIS_SELECTED)
            {
                // create NM_LISTVIEW so we can reuse OnLVSelectionChange
                NM_LISTVIEW  lvn = {0};

                lvn.hdr = pnm->hdr;
                lvn.iItem = -1;   // say the change affected all items
                lvn.uNewState = pnm->uNewState;
                lvn.uOldState = pnm->uOldState;
                this->OnLVSelectionChange(&lvn);
            }
        }
        break;
    
    case LVN_DELETEITEM:
        this->OnListViewDelete(plvn->iItem, (LPITEMIDLIST)plvn->lParam);
        DV_FlushCachedMenu(this);
        break;

    case LVN_COLUMNCLICK:
        // allow clicking on columns to set the sort order
        if (_pshf2 || _psd || this->HasCB())
        {
            UINT iRealColumn;
            LPARAM lParamSort       = _dvState.lParamSort;
            LONG iLastColumnClick   = _dvState.iLastColumnClick,
                 iLastSortDirection = _dvState.iDirection;  // push sort state

            // Folder doesn't know which columns are on or off, so communication with folder uses real col #s
            this->MapVisibleToRealColumn(plvn->iSubItem, &iRealColumn);

            // toggle the direction of the sort if on the same column
            if (_dvState.iLastColumnClick == (int) iRealColumn)
                _dvState.iDirection = -_dvState.iDirection;
            else
                _dvState.iDirection = 1;

            // seeral ways to do this... each can defer to the
            // ultimate default that is defview calling itself.
            HRESULT hr = S_FALSE;
            if (_psd)
                hr = _psd->ColumnClick(iRealColumn);

            if (hr != S_OK)
                hr = CallCB(SFVM_COLUMNCLICK, iRealColumn, 0);

            if (hr != S_OK)
                hr = this->Rearrange(iRealColumn);

            // Allows iLastColumnClick to stay valid during the above calls
            if (SUCCEEDED(hr))
                _dvState.iLastColumnClick = iRealColumn;
            else
            {
                //  We failed somewhere so pop the sort state.
                _dvState.iDirection = iLastSortDirection;
                _dvState.iLastColumnClick = (int)_dvState.lParamSort;
                _dvState.lParamSort = lParamSort ;
                _SetSortArrows() ;
                _dvState.iLastColumnClick = iLastColumnClick;
            }
        }
        break;


    case LVN_KEYDOWN:
        this->HandleKeyDown(((LV_KEYDOWN *)plvn));
        break;

#define plvdi ((LV_DISPINFO *)plvn)

    case LVN_BEGINLABELEDIT:
        {
            LPCITEMIDLIST pidl = DV_GetPIDLParam(this, plvdi->item.lParam, plvdi->item.iItem);
            ULONG rgfAttr = SFGAO_CANRENAME;

            if (!pidl || FAILED(_pshf->GetAttributesOf(1, &pidl, &rgfAttr)) ||
                !(rgfAttr & SFGAO_CANRENAME))
            {
                MessageBeep(0);
                return TRUE;        // Don't allow label edit
            }

            _fInLabelEdit = TRUE;

            HWND hwndEdit = ListView_GetEditControl(_hwndListview);
            if (hwndEdit)
            {
                int cchMax = 0;

                CallCB(SFVM_GETCCHMAX, (WPARAM)pidl, (LPARAM)&cchMax);

                if (cchMax)
                {
                    ASSERT(cchMax < 1024);
                    SendMessage(hwndEdit, EM_LIMITTEXT, cchMax, 0);
                }

                TCHAR szName[MAX_PATH];
                STRRET str;
                if (SUCCEEDED(_pshf->GetDisplayNameOf(pidl, SHGDN_INFOLDER | SHGDN_FOREDITING, &str)) &&
                    SUCCEEDED(StrRetToBuf(&str, pidl, szName, ARRAYSIZE(szName))))
                {
                    SetWindowText(hwndEdit, szName);
                }
            }
        }
        break;

    case LVN_ENDLABELEDIT:

        _fInLabelEdit = FALSE;
        if (plvdi->item.pszText)
        {
            LPCITEMIDLIST pidl = DV_GetPIDLParam(this, plvdi->item.lParam, plvdi->item.iItem);
            if (pidl)
            {
                WCHAR wszName[MAX_PATH];

                SHTCharToUnicode(plvdi->item.pszText, wszName, ARRAYSIZE(wszName));

                // _pshf may need to talk back to us or use us to EnableModless for UI.
                // FTP Needs this.
                IUnknown_SetSite(_pshf, SAFECAST(this, IOleCommandTarget *));
                if (SUCCEEDED(_pshf->SetNameOf(_hwndMain, pidl, wszName, SHGDN_INFOLDER, NULL)))
                {
                    SHChangeNotifyHandleEvents();
                    DV_CDB_OnStateChange(this, CDBOSC_RENAME);
                }
                else
                    SendMessage(_hwndListview, LVM_EDITLABEL, plvdi->item.iItem, (LPARAM)plvdi->item.pszText);

                IUnknown_SetSite(_pshf, NULL);
            }
        }
        else
        {
            // The user canceled. so return TRUE to let things like the mouse
            // click be processed.
            return TRUE;
        }
        break;

    case LVN_GETDISPINFO:
    {
        LV_ITEM item;
        TCHAR szIconFile[MAX_PATH];
        UINT uReal;

        if (!(plvdi->item.mask & (LVIF_TEXT | LVIF_IMAGE)))
            break;

        LPCITEMIDLIST pidl = DV_GetPIDLParam(this, plvdi->item.lParam, plvdi->item.iItem);
        if (!pidl)
            break;

        ASSERT(IsValidPIDL(pidl));

        if (plvdi->item.iSubItem != 0)
        {
            ASSERT(_fs.ViewMode == FVM_DETAILS);
            this->MapVisibleToRealColumn(plvdi->item.iSubItem, &uReal);
        }
        else
            uReal = 0;

        item.mask = plvdi->item.mask & (LVIF_TEXT | LVIF_IMAGE);
        item.iItem = plvdi->item.iItem;
        item.iSubItem = uReal;           // We don't store the "visible" col#, but no-one needs it
        item.iImage = plvdi->item.iImage = -1; // for iSubItem!=0 case

        if (item.iSubItem == 0 && (item.mask & LVIF_IMAGE))
        {
            if (!_psio)
                //try again
                _pshf->QueryInterface(IID_IShellIconOverlay, (void **)&_psio);

            // If the location supports the IShellIconOverlay than only need to ask for ghosted, else
            // we need to do the old stuff...
            DWORD uFlags = SFGAO_GHOSTED;
            if (!_psio)
                uFlags |= SFGAO_LINK | SFGAO_SHARE;

            if (FAILED(_pshf->GetAttributesOf(1, &pidl, &uFlags))) {
                uFlags = 0;
            }

            // set the mask
            item.mask |= LVIF_STATE;
            plvdi->item.mask |= LVIF_STATE;
            item.stateMask = LVIS_OVERLAYMASK;

            // Pick the right overlay icon. The order is significant.
            item.state = 0;
            if (_psio)
            {
                int iOverlayIndex = SFV_ICONOVERLAY_UNSET;
                if (_IsOwnerData())
                {
                    // Note: we are passing SFV_ICONOVERLAY_DEFAULT here because
                    // some owners do not respond to SFVM_GETICONOVERLAY might return
                    // iOverlayIndex unchanged and it will get 
                    iOverlayIndex = SFV_ICONOVERLAY_DEFAULT;
                    CallCB(SFVM_GETICONOVERLAY, plvdi->item.iItem, (LPARAM)&iOverlayIndex);
                    if (iOverlayIndex > 0)
                    {
                        item.stateMask |= LVIS_OVERLAYMASK;
                        item.state |= INDEXTOOVERLAYMASK(iOverlayIndex);
                    }
                }

                if (iOverlayIndex == SFV_ICONOVERLAY_UNSET)
                {
                    iOverlayIndex = OI_ASYNC;
                    HRESULT hres = _psio->GetOverlayIndex(pidl, &iOverlayIndex);
                    if (hres == E_PENDING)
                        _GetOverlayIndexAsync(pidl, item.iItem);
                    else if (SUCCEEDED(hres))
                    {
                        ASSERT(iOverlayIndex >= 0);
                        ASSERT(iOverlayIndex < MAX_OVERLAY_IMAGES);

                        // In the owner data case, tell the owner we got an Overlay index
                        if (_IsOwnerData())
                            CallCB(SFVM_SETICONOVERLAY, item.iItem, iOverlayIndex);
                        
                        item.state = INDEXTOOVERLAYMASK(iOverlayIndex);
                    }
                }
            } else {
                if (uFlags & SFGAO_LINK) {
                    item.state = INDEXTOOVERLAYMASK(II_LINK - II_OVERLAYFIRST + 1);
                } else if (uFlags & SFGAO_SHARE) {
                    item.state = INDEXTOOVERLAYMASK(II_SHARE - II_OVERLAYFIRST + 1);
                }
            }

            if (uFlags & SFGAO_GHOSTED) {
                item.stateMask |= LVIS_CUT;
                item.state |= LVIS_CUT;
            } else {
                item.stateMask |= LVIS_CUT;
                item.state &= ~LVIS_CUT;
            }
            
            plvdi->item.stateMask = item.stateMask;
            plvdi->item.state = item.state;

            // Get the image
            TIMESTART(_GetIcon);

            if (_IsOwnerData())
            {
                CallCB(SFVM_GETITEMICONINDEX, plvdi->item.iItem, (LPARAM)&item.iImage);
            }

            if (item.iImage == -1)
                _GetIconAsync(pidl, &item.iImage, TRUE);
            TIMESTOP(_GetIcon);

            plvdi->item.iImage = item.iImage;
        }

        // Note that TEXT must come after IMAGE, since we reuse
        // szIconFile
        if (item.mask & LVIF_TEXT)
        {
            DETAILSINFO di;

            if (plvdi->item.cchTextMax)
                *plvdi->item.pszText = TEXT('\0');

            di.str.uType = STRRET_CSTR;
            di.str.cStr[0] = 0;
            di.iImage = -1;     // Assume for now no image...

            // Note that we do something different for index 0 = NAME
            if (item.iSubItem == 0)
            {
                item.pszText = szIconFile;
                item.cchTextMax = ARRAYSIZE(szIconFile);

                TIMESTART(_GetName);
                if (SUCCEEDED(_pshf->GetDisplayNameOf(
                    pidl, SHGDN_INFOLDER, &di.str)))
                {
#ifndef UNICODE         // We can't do this on unicode (listview is expecting a ptr to a unicode string)
                    if (di.str.uType == STRRET_OFFSET)
                    {
                        // If an offset, we just pass the name back
                        // but don't store it to save on space

                        // BUGBUG (DavePl) Note that this is someone's notify
                        // message, and we are placing a text pointer in it...
                        // a unicode text pointer out of a pidl, which is therefore
                        // not even aligned!

                        plvdi->item.pszText = (LPTSTR)(((LPBYTE)pidl) + di.str.uOffset);
                        item.mask &= ~LVIF_TEXT;
                    }
                    else
                    {
#endif
                        StrRetToBuf(&di.str, pidl, item.pszText, item.cchTextMax);
                        lstrcpyn(plvdi->item.pszText, item.pszText, plvdi->item.cchTextMax);
#ifndef UNICODE
                    }
#endif
                }
                TIMESTOP(_GetName);
            }
            else
            {
                DWORD dwState;

                if (_pshf2 && SUCCEEDED(_pshf2->GetDefaultColumnState(item.iSubItem, &dwState)))
                {
                    if (dwState & SHCOLSTATE_SLOW)
                    {
                        if (_bInSortCallBack)
                            plvdi->item.mask |= LVIF_DI_SETITEM;
                        else
                        {
                            IRunnableTask *pTask;

                            if (SUCCEEDED(CDVExtendedColumnTask_CreateInstance(this, pidl, _fmt, item.iSubItem, &pTask)))
                            {
                                if (DefView_IdleDoStuff(this, pTask, TOID_DVBackgroundEnum, 0, TASK_PRIORITY_BKGRND_FILL))
                                {

                                    // 99/03/26 vtan: If the task was successfully created
                                    // and scheduled then tell the list to set the text to
                                    // nothing. This will prevent another call to
                                    // ListView_GetItemText() from invoking us with
                                    // LVN_GETDISPINFO while the the task is waiting to be
                                    // completed in another thread.

                                    ListView_SetItemText(_hwndListview, plvdi->item.iItem, plvdi->item.iSubItem, NULL);
                                    ATOMICRELEASE(pTask);
                                    // we set the plvdi->item.pszText to empty str already
                                    break;
                                }
                                else
                                    TraceMsg(TF_WARNING, "dv_onlvn: couldn't add extended column task");
                                // else we couldn't add the task, so fall through
                            }
                            else
                                TraceMsg(TF_WARNING, "dv_onlvn: couldn't create extended column task, falling through");
                        }
                    }
                }

                di.pidl = pidl;
                di.fmt  = _fmt;

                if (SUCCEEDED(_GetDetailsHelper(item.iSubItem, &di)))
                {
                    StrRetToBuf(&di.str, pidl, plvdi->item.pszText, plvdi->item.cchTextMax);

                    if ((di.iImage != -1) && (plvdi->item.mask & LVIF_IMAGE))
                    {
                        plvdi->item.iImage = di.iImage;
                    }
                }
            }
        }

        // We only store the name; all other info comes on demand
        if (item.iSubItem == 0)
        {
            plvdi->item.mask |= LVIF_DI_SETITEM;
        }

        break;
    } // LVN_GETDISPINFO

    case LVN_ODFINDITEM:
        // We are owner data so we need to find the item for the user...
        {
            int iItem = -1;
            if (SUCCEEDED(CallCB(SFVM_ODFINDITEM, (WPARAM)&iItem, (LPARAM)plvn)))
                return iItem;
            return -1;  // Not Found
        }
    case LVN_ODCACHEHINT:
        // Just a hint we don't care about return values
        CallCB(SFVM_ODCACHEHINT, 0, (LPARAM)plvn);
        break;

    case LVN_GETEMPTYTEXT:
        if (this->HasCB())
        {
            if ((plvdi->item.mask & LVIF_TEXT) &&
                SUCCEEDED(CallCB(SFVM_GETEMPTYTEXT, (WPARAM)(plvdi->item.cchTextMax), (LPARAM)(plvdi->item.pszText))))
                return TRUE;
        }
        break;

    }
#undef lpdi
#undef plvdi
    return 0;
}

void CDefView::_PostEnumDoneMessage()
{
    _fFileListEnumDone = TRUE;
    if (m_cFrame.m_fReadyStateComplete)
    {
        PostMessage(_hwndView, WM_DSV_FILELISTENUMDONE, 0, 0);
    }
}

void CDefView::_PostSelChangedMessage()
{
    if (!_fSelectionChangePending)
    {
        PostMessage(_hwndView, WM_DSV_SENDSELECTIONCHANGED, 0, 0);
        _fSelectionChangePending = TRUE;
    }
}

// BUGBUG: Todo -- implement enabling/disabling of other toolbar buttons.  We can enable/disable
// based on the current selection, but the problem is that some of the buttons work
// for other guys when defview doesn't have focus.  Specifically, cut/copy/paste work
// for the folders pane.  If we're going to enable/disable these buttons based on the
// selection, then we'll need to have a mechanism that lets the active band (such as
// folders) also have a say about the button state.  That is too much work right now.

static const UINT c_BtnCmds[] =
{
    SFVIDM_EDIT_COPYTO,
    SFVIDM_EDIT_MOVETO,
#ifdef ENABLEDISABLEBUTTONS
    SFVIDM_EDIT_COPY,
    SFVIDM_EDIT_CUT,
#endif
};

static const DWORD c_BtnAttr[] =
{
    SFGAO_CANCOPY,
    SFGAO_CANMOVE,
#ifdef ENABLEDISABLEBUTTONS
    SFGAO_CANCOPY,
    SFGAO_CANMOVE,
#endif
};

#define SFGAO_RELEVANT      (SFGAO_CANCOPY | SFGAO_CANMOVE)

BOOL CDefView::_ShouldEnableButton(UINT uiCmd, DWORD dwAttr, int iIndex)
{
    COMPILETIME_ASSERT(SIZEOF(c_BtnCmds) == SIZEOF(c_BtnAttr));

    if (m_cFrame.IsSFVExtension())
    {
        // for sfv extensions we don't receive NM_SETFOCUS/NM_KILLFOCUS so
        // we cannot correctly enable/disable buttons
        // for now we just always leave them enabled
        return TRUE;
    }

    DWORD dwBtnAttr;

    if (iIndex != -1)
    {
        // Caller was nice and figured out dest index for us
        dwBtnAttr = c_BtnAttr[iIndex];
    }
    else
    {
        // Look for the command ourselves
        dwBtnAttr = SHSearchMapInt((int*)c_BtnCmds, (int*)c_BtnAttr, ARRAYSIZE(c_BtnCmds), uiCmd);
        if (dwBtnAttr == -1)
        {
            // We don't care about this button, just enable it.
            return TRUE;
        }
    }

    if (!_fHasListViewFocus)
    {
        // Disable any button we care about while listview is inactive.
        return FALSE;
    }

    return BOOLIFY(dwAttr & dwBtnAttr);
}

/*
    _GetCachedToolbarSelectionAttrs

    As a perf enhancement, we cache the attributes of the currently selected 
    files/folders in a FS view only.  This is to avoid n^2 traversals of the
    selected items as we select/unselect them.  These cached attributes 
    should not be used for anything other than determining toolbar button
    states and should be revisited if we add toolbar buttons that care about
    much more than the attributes used by Move to & Copy to.
*/
BOOL CDefView::_GetCachedToolbarSelectionAttrs(ULONG *pdwAttr)
{
    BOOL fResult = FALSE;
    CLSID clsid;
    HRESULT hres = IUnknown_GetClassID(_pshf, &clsid);
    if (SUCCEEDED(hres))
    {
        UINT iCount;
        if (IsEqualGUID(CLSID_ShellFSFolder, clsid) && SUCCEEDED(GetSelectedCount(&iCount)))
        {
            if (iCount > 0 && _uCachedSelCount > 0)
            {
                *pdwAttr = _uCachedSelAttrs;
                fResult = TRUE;
            }
        }
    }
    return fResult;
}

void CDefView::_SetCachedToolbarSelectionAttrs(ULONG dwAttrs)
{
    if (SUCCEEDED(GetSelectedCount(&_uCachedSelCount)))
        _uCachedSelAttrs = dwAttrs;
    else
        _uCachedSelCount = 0;
}

void CDefView::_EnableDisableTBButtons()
{
    if (!IsEqualGUID(_clsid, GUID_NULL))
    {
        IExplorerToolbar *piet;
        if (SUCCEEDED(IUnknown_QueryService(_psb, SID_SExplorerToolbar, IID_IExplorerToolbar, (void **)&piet)))
        {
            ULONG dwAttr;

            if (!_GetCachedToolbarSelectionAttrs(&dwAttr))
                dwAttr = DefView_GetAttributesFromSelection(this, SFGAO_RELEVANT);

            for (int i = 0; i < ARRAYSIZE(c_BtnCmds); i++)
            {
                UINT  uiState;
                
                if (SUCCEEDED(piet->GetState(&_clsid, c_BtnCmds[i], &uiState)))
                {
                    if (_ShouldEnableButton(c_BtnCmds[i], dwAttr, i))
                        uiState |= TBSTATE_ENABLED;
                    else
                        uiState &= ~TBSTATE_ENABLED;

                    piet->SetState(&_clsid, c_BtnCmds[i], uiState);
                }
            }

            _SetCachedToolbarSelectionAttrs(dwAttr);
            
            piet->Release();
        }
    }
}

// WARNING: don't add any code here that is expensive in anyway!
// we get many many of these notifies and if we slow this routine down
// we screw select all and large selection perf.
//
// you can add expensive code to the WM_DSV_SENDSELECTIONCHANGED handler, 
// that happens after all of the sel change notifies go through.

void CDefView::OnLVSelectionChange(NM_LISTVIEW *plvn)
{
    // toss the cached context menu
    DV_FlushCachedMenu(this, DVFCMF_SEL);

    // Notify the dispach that the focus changed..
    _PostSelChangedMessage();

    // throw away cached attribute bits
    _dwAttrSel = (DWORD)-1;

    // Tell the defview client that the selection may have changed
    SFVM_SELCHANGE_DATA dvsci;

    dvsci.uNewState = plvn->uNewState;
    dvsci.uOldState = plvn->uOldState;
    dvsci.lParamItem = plvn->lParam;

    CallCB(SFVM_SELCHANGE, MAKEWPARAM(SFVIDM_CLIENT_FIRST, plvn->iItem), (LPARAM)&dvsci);
}

#define IN_VIEW_BMP     0x8000
#define EXT_VIEW_GOES_HERE 0x4000
#define PRIVATE_TB_FLAGS (IN_VIEW_BMP | EXT_VIEW_GOES_HERE)
#define IN_STD_BMP      0x0000


LRESULT CDefView::_OnNotify(NMHDR *pnm)
{
    switch (pnm->idFrom) {

    case ID_LISTVIEW:
        return _OnLVNotify((NM_LISTVIEW *)pnm);

    case FCIDM_TOOLBAR:
        return _TBNotify(pnm);

    default:

        switch (pnm->code) 
        {
        case TTN_NEEDTEXT:
            #define ptt ((LPTOOLTIPTEXT)pnm)
            DV_GetToolTipText(this, ptt->hdr.idFrom, ptt->szText, ARRAYSIZE(ptt->szText));
            #undef ptt
            break;

        case NM_RCLICK:
            if (GetParent(pnm->hwndFrom) == _hwndListview)
            {
                POINT p;
                GetMsgPos(&p);
                _DoColumnsMenu(p.x, p.y);
                return 1;                           // To keep normal context menu from appearing
            }
        }
    }

    return 0;
}


HRESULT CDefView::InitializeVariableColumns(DWORD *pdwColList)
{
#define MI_INITIAL_SIZE 6
    DETAILSINFO di;
    TCHAR szText[MAX_PATH];
    UINT iReal;
#ifdef DEBUG
    DWORD *pdwColOrig = pdwColList;
#endif

    m_cColItems = 0;

    if (!m_pColItems)
        m_pColItems = (COL_INFO *)LocalAlloc(LPTR, MI_INITIAL_SIZE * sizeof(*m_pColItems));

    if (!m_pColItems)
        return E_OUTOFMEMORY;

    for (iReal = 0; ; iReal++)
    {
        di.fmt  = LVCFMT_LEFT;
        di.cxChar = DEFAULT_NUMCHARS;
        di.str.uType = (UINT)-1;
        di.pidl = NULL;

        if (FAILED(_GetDetailsHelper(iReal, &di)))
            break;

        ASSERT(iReal < 100);

        StrRetToBuf(&di.str, NULL, szText, ARRAYSIZE(szText));

        if (iReal >= MI_INITIAL_SIZE)
        {
            COL_INFO *pmi = (COL_INFO *)LocalReAlloc(m_pColItems, (iReal + 1) * SIZEOF(m_pColItems[0]), LMEM_MOVEABLE | LMEM_ZEROINIT);
            ASSERT(pmi);
            if (!pmi)
            {
                // Realloc failed. We need to bail out.
                // Note: m_pColItems will be freed in the CDefView's destructor!
                m_cColItems = iReal;
                return E_OUTOFMEMORY;  //No mem! Bail out!
            }
            m_pColItems = pmi;
        }
        lstrcpyn(m_pColItems[iReal].szName, szText, ARRAYSIZE(m_pColItems[iReal].szName));
        m_pColItems[iReal].fmt = di.fmt;
        m_pColItems[iReal].cChars = di.cxChar;
        m_pColItems[iReal].csFlags = SHCOLSTATE_ONBYDEFAULT;
    }
    m_cColItems = iReal;
    ASSERT(m_cColItems);

    // first init default state from folder
    if (_pshf2)
    {
        for (iReal = 0; iReal < m_cColItems; iReal++)
        {
            if (FAILED(_pshf2->GetDefaultColumnState(iReal, &m_pColItems[iReal].csFlags)))
                m_pColItems[iReal].csFlags = SHCOLSTATE_ONBYDEFAULT;
        }
    }
    // Set up saved column state only if the saved state
    // contains information other than "nothing".
    if ((pdwColList != NULL) && (*pdwColList != 0xFFFFFFFF))
    {

        // 99/02/05 vtan: If there is a saved column state then
        // clear all the column "on" states to "off" and only
        // display what columns are specified. Start at 1 so
        // that name is always on.

        for (iReal = 1; iReal < m_cColItems; iReal++)
            m_pColItems[iReal].csFlags &= ~SHCOLSTATE_ONBYDEFAULT;
        while ((*pdwColList != 0xFFFFFFFF) &&
               SUCCEEDED(this->SetColumnState(*pdwColList, SHCOLSTATE_ONBYDEFAULT, SHCOLSTATE_ONBYDEFAULT)))
        {
            pdwColList++;
        }
    }

    return S_OK;
}

BOOL CDefView::IsColumnHidden(UINT uCol)
{
    ASSERT(_bLoadedColumns);
    BOOL bRet = FALSE;
    if (m_pColItems)
    {
        bRet = (uCol < m_cColItems) && (m_pColItems[uCol].csFlags & SHCOLSTATE_HIDDEN) ? TRUE : FALSE;
    }

    return bRet;
}

BOOL CDefView::IsColumnOn(UINT uCol)
{
    ASSERT(_bLoadedColumns);
    BOOL bRet = FALSE;
    if (m_pColItems)
    {
        bRet = (uCol < m_cColItems) && (m_pColItems[uCol].csFlags & SHCOLSTATE_ONBYDEFAULT) ? TRUE : FALSE;
    }

    return bRet;
}

#define COL_CM_MAXITEMS     10    // how many item show up in context menu before more ... is inserted

HRESULT CDefView::AddColumnsToMenu(HMENU hm, DWORD dwBase)
{
    BOOL bNeedMoreMenu = FALSE;
    HRESULT hres = E_FAIL;

    if (m_pColItems)
    {
        AppendMenu(hm, MF_STRING | MF_CHECKED | MF_GRAYED, dwBase, m_pColItems[0].szName);
        for(UINT i = 1; i < min(COL_CM_MAXITEMS, m_cColItems); i++)
        {
            if (!(m_pColItems[i].csFlags & SHCOLSTATE_HIDDEN))
            {
                if (m_pColItems[i].csFlags & SHCOLSTATE_SECONDARYUI)
                    bNeedMoreMenu = TRUE;
                else
                    AppendMenu(hm, MF_STRING | (m_pColItems[i].csFlags & SHCOLSTATE_ONBYDEFAULT) ? MF_CHECKED : 0, dwBase+i, m_pColItems[i].szName);
            }
        }

        if (bNeedMoreMenu || (m_cColItems > COL_CM_MAXITEMS))
        {
            TCHAR szMore[MAX_PATH];
            LoadString(HINST_THISDLL, IDS_COL_CM_MORE, szMore, SIZEOF(szMore));
            AppendMenu(hm, MF_SEPARATOR, 0, NULL);
            AppendMenu(hm, MF_STRING, SFVIDM_VIEW_COLSETTINGS, szMore);
        }
        hres = S_OK;
    }

    return hres;
}

HRESULT CDefView::SetColumnState(UINT uCol, DWORD dwMask, DWORD dwNewBits)
{
    HRESULT hres = E_FAIL;
    if (uCol >= m_cColItems)
        return E_INVALIDARG;

    if (m_pColItems)
    {
        m_pColItems[uCol].csFlags = (m_pColItems[uCol].csFlags & ~dwMask) | (dwNewBits & dwMask);
        hres = S_OK;
    }

    return hres;
}

HRESULT CDefView::MapRealToVisibleColumn(UINT uRealCol, UINT *puVisCol)
{
    ASSERT(_bLoadedColumns);
    HRESULT hres = E_FAIL;
    *puVisCol = 0;
    if (m_pColItems)
    {
        if (m_cColItems > 0)
        {
            if (uRealCol >= m_cColItems)
                uRealCol = m_cColItems - 1;

            for (UINT i = 0;i <= uRealCol; i++)
            {
                if (m_pColItems[i].csFlags & SHCOLSTATE_ONBYDEFAULT)
                    (*puVisCol)++;
            }

            if (*puVisCol > 0)
                (*puVisCol)--; // 0 based
        }
        hres = S_OK;
        // should probably return S_FALSE if the real column is not currently visible (we return next greater visible)
    }

    return hres;
}

HRESULT CDefView::MapVisibleToRealColumn(UINT uVisCol, UINT *puReal)
{
    ASSERT(_bLoadedColumns);
    HRESULT hres = E_FAIL;
    *puReal=0;
    if (m_pColItems)
    {
        for (UINT i=0;;)
        {
            if (*puReal >= m_cColItems)
                return E_FAIL;
            if (m_pColItems[*puReal].csFlags & SHCOLSTATE_ONBYDEFAULT)
                i++;
            if (i>uVisCol)
                break;
            (*puReal)++;
        }

        hres = S_OK;
    }

    return hres;
}

BOOL CDefView::_IsExtendedColumn(INT_PTR iReal, DWORD *pdwState)
{
    DWORD dwState = 0;
    BOOL bRet = (_pshf2 &&
                 SUCCEEDED(_pshf2->GetDefaultColumnState((int)iReal, &dwState)) &&
                 (dwState & SHCOLSTATE_EXTENDED));
    if (pdwState)
        *pdwState = dwState;
    return bRet;
}

UINT CDefView::GetMaxColumns()
{
    return m_cColItems;
}

// uCol is a real column number, not visible column number
BOOL CDefView::_HandleColumnToggle(UINT uCol, BOOL bRefresh)
{
    UINT uColVis, uColVisOld;
    BOOL fWasOn = IsColumnOn(uCol); // if its off now, we are adding it

    MapRealToVisibleColumn(uCol, &uColVisOld);

    SetColumnState(uCol, SHCOLSTATE_ONBYDEFAULT, fWasOn ? 0 : SHCOLSTATE_ONBYDEFAULT);

    MapRealToVisibleColumn(uCol, &uColVis);
    if (!fWasOn)
    {
        LV_COLUMN col;

        // Adding a column

        col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        col.fmt = m_pColItems[uCol].fmt;
        col.cx = m_pColItems[uCol].cChars * _cxChar;  // Use default width
        col.pszText = m_pColItems[uCol].szName;
        col.cchTextMax = MAX_COLUMN_NAME_LEN;
        col.iSubItem = uCol;                // not vis

        // This is all odd... Find Files uses this, but i think it should be LVCFMT_COL_IMAGE
        if (col.fmt & LVCFMT_COL_HAS_IMAGES)
        {
            ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_SUBITEMIMAGES, LVS_EX_SUBITEMIMAGES);
            col.fmt &= ~LVCFMT_COL_HAS_IMAGES;
        }

        ListView_InsertColumn(_hwndListview, uColVis, &col);

        // now add it to our DSA
        _pcp->AppendColumn(uColVis, (USHORT) col.cx, uColVis);

        m_uCols++;
    }
    else
    {
        _pcp->RemoveColumn(uColVisOld);
        ListView_DeleteColumn(_hwndListview, uColVisOld);
        m_uCols--;

        if (_dvState.lParamSort == (int) uCol)
        {
            UINT iNewVis;
            MapVisibleToRealColumn(uColVisOld, &iNewVis);
            Rearrange(iNewVis);
        }
    }

    if (bRefresh)
    {
        ListView_RedrawItems(_hwndListview, 0, 0x7fff);
        InvalidateRect(_hwndListview, NULL, TRUE);
        UpdateWindow(_hwndListview);
    }
    return TRUE;
}

void CDefView::_SetSortArrows(void)
{
    int iCol;
    int iColLast;
    HWND hwndHead = ListView_GetHeader(_hwndListview);

    // 99/06/09 #340624 vtan: Only put the sort arrows for listviews
    // which are not owner data.

    if (!hwndHead || _IsOwnerData())
        return;

    MapRealToVisibleColumn((UINT) _dvState.lParamSort, (UINT *) &iCol);
    MapRealToVisibleColumn(_dvState.iLastColumnClick, (UINT *) &iColLast);


    HBITMAP hbm =   (HBITMAP) LoadImage(HINST_THISDLL, MAKEINTRESOURCE((_dvState.iDirection>0)?IDB_SORT_UP:IDB_SORT_DN),
                        IMAGE_BITMAP, 0, 0,  LR_LOADMAP3DCOLORS );
    HDITEM hdi1 = {HDI_BITMAP | HDI_FORMAT, 0, NULL, NULL, 0, 0, 0, 0, 0};
    HDITEM hdi2 = {HDI_BITMAP | HDI_FORMAT, 0, NULL, hbm, 0, HDF_BITMAP | HDF_BITMAP_ON_RIGHT, 0, 0, 0};

    // 99/04/16 #326158 vtan: Dump each HBITMAP that already exists in the
    // header. This prevents the GDI Handle leak as the Header control does
    // not take care of this for us.

    Header_GetItem(hwndHead, iCol, &hdi1);
    hdi2.fmt |= hdi1.fmt;
    if (hdi1.hbm != NULL)
        TBOOL(DeleteObject(hdi1.hbm));
    Header_SetItem(hwndHead, iCol, &hdi2);

    if (iColLast != iCol && iColLast != -1)
    {
        HDITEM hdi1 = {HDI_BITMAP | HDI_FORMAT, 0, NULL, NULL, 0, 0, 0, 0, 0};
        Header_GetItem(hwndHead, iColLast, &hdi1);
        hdi1.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
        if (hdi1.hbm != NULL)
        {
            TBOOL(DeleteObject(hdi1.hbm));
            hdi1.hbm = NULL;
        }
        Header_SetItem(hwndHead, iColLast, &hdi1);
    }
}

PFNDPACOMPARE CDefView::_GetCompareFunction(void)

{
    if (_pshf2 == NULL)
        return(_Compare);
    else
        return(_CompareExact);
}

// 99/05/13 vtan: Only use CDefView::_CompareExact if you know that
// IShellFolder2 is implemented. SHCIDS_ALLFIELDS is IShellFolder2
// specific. Use CDefView::_GetCompareFunction() to get the function
// to pass to DPA_Sort() if you don't want to make this determination.

// p1 and p2 are pointers to the lv_item's LPARAM, which is currently the pidl
int CALLBACK CDefView::_CompareExact(void *p1, void *p2, LPARAM lParam)
{
    CDefView *thisObject = (CDefView *)lParam;
    HRESULT hres = thisObject->_pshf2->CompareIDs(thisObject->_dvState.lParamSort | SHCIDS_ALLFIELDS, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);

    // 99/05/18 #341468 vtan: If the first comparison fails it may be because
    // lParamSort is not understood by IShellFolder::CompareIDs (perhaps it's
    // an extended column?) In this case get the default comparison method
    // and use that. If that fails use 0 which should hopefully not fail. If
    // the 0 case fails we are toast with an assert.

    if (FAILED(hres))
    {
        DVSAVESTATE     saveState;

        thisObject->_GetSortDefaults(&saveState);
        hres = thisObject->_pshf2->CompareIDs(saveState.lParamSort | SHCIDS_ALLFIELDS, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
        if (FAILED(hres))
        {
            hres = thisObject->_pshf2->CompareIDs(SHCIDS_ALLFIELDS, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
        }
    }

    ASSERT(SUCCEEDED(hres))
    ASSERT(thisObject->_dvState.iDirection != 0);
    return (short)HRESULT_CODE(hres) * thisObject->_dvState.iDirection;
}

// p1 and p2 are pointers to the lv_item's LPARAM, which is currently the pidl
int CALLBACK CDefView::_Compare(void *p1, void *p2, LPARAM lParam)
{
    CDefView *thisObject = (CDefView *)lParam;
    HRESULT hres = thisObject->_pshf->CompareIDs(thisObject->_dvState.lParamSort, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
    if (FAILED(hres))
    {
        DVSAVESTATE     saveState;

        thisObject->_GetSortDefaults(&saveState);
        hres = thisObject->_pshf->CompareIDs(saveState.lParamSort, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
        if (FAILED(hres))
        {
            hres = thisObject->_pshf->CompareIDs(0, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
        }
    }

    ASSERT(SUCCEEDED(hres))
    ASSERT(thisObject->_dvState.iDirection != 0);
    return (short)HRESULT_CODE(hres) * thisObject->_dvState.iDirection;
}

// dw1 and dw2 are the actual item index.  thisObject index is only valid during the sort
int CALLBACK CDefView::_CompareExtended(LPARAM dw1, LPARAM dw2, LPARAM lParam)
{
    CDefView *thisObject = (CDefView *)lParam;

    // First, match filesystem and put folders in front of files:
    //
    LPCITEMIDLIST pidl1 = thisObject->_GetPIDL((int) dw1);
    LPCITEMIDLIST pidl2 = thisObject->_GetPIDL((int) dw2);

    // 99/03/25 #295631 vtan: Do an IShellFolder2::CompareIDs.
    // Make sure it's IShellFolder2 and not IShellFolder.

    if (thisObject->_pshf2 != NULL)
    {
        HRESULT hres;

        hres = thisObject->_pshf2->CompareIDs(thisObject->_dvState.lParamSort, pidl1, pidl2);
        if (SUCCEEDED(hres))
            return(static_cast<short>(HRESULT_CODE(hres)) * thisObject->_dvState.iDirection);
    }

    DWORD uFlags1 = SFGAO_FOLDER;
    DWORD uFlags2 = SFGAO_FOLDER;
    thisObject->_pshf->GetAttributesOf(1, &pidl1, &uFlags1);
    thisObject->_pshf->GetAttributesOf(1, &pidl2, &uFlags2);
    // if they are different
    if ((uFlags1 & SFGAO_FOLDER) && !(uFlags2 & SFGAO_FOLDER))
        return thisObject->_dvState.iDirection;
    else if (!(uFlags1 & SFGAO_FOLDER) && (uFlags2 & SFGAO_FOLDER))
        return -(thisObject->_dvState.iDirection);

    // Now that we're dealing with the same thing (folder-folder or file-file),
    // we can check what the extended column has to say about our order:
    //
    TCHAR szText1[MAX_PATH], szText2[MAX_PATH];
    UINT iVisCol;

    thisObject->MapRealToVisibleColumn((UINT) thisObject->_dvState.lParamSort, &iVisCol);

    ListView_GetItemText(thisObject->_hwndListview, dw1, iVisCol, szText1, ARRAYSIZE(szText1));
    ListView_GetItemText(thisObject->_hwndListview, dw2, iVisCol, szText2, ARRAYSIZE(szText2));

    ASSERT(thisObject->_dvState.iDirection != 0);
    if (thisObject->_LastSortColType == SHCOLSTATE_TYPE_STR)
        return lstrcmpi(szText1, szText2) * thisObject->_dvState.iDirection;
    else
    {
        int i1 = StrToInt(szText1);
        int i2 = StrToInt(szText2);

        return (i1-i2) * thisObject->_dvState.iDirection;
    }
}

BOOL CDefView::_InternalRearrange(void)
{
    DWORD dwState = 0;
    BOOL bRet = FALSE ;

    TIMEVAR(_DV_ReArrange);
    TIMEIN(_DV_ReArrange);

    TIMESTART(_DV_ReArrange);

    // This is semi-bogus for defview to care whether the column is extended or not.
    // We could have modified the ISF::CompareIDs() to handle extended columns, but
    // then it would only have the pidls, and would have to re-extract any data, so
    // its much faster if we separate out the extended columns, and take advantage
    // of listview's caching abilities.

    if (_IsExtendedColumn(_dvState.lParamSort, &dwState))
    {
        if( _GetBackgroundTaskCount( TOID_DVBackgroundEnum ) > 0 )
            MessageBeep( MB_OK ) ;
        else
        {

            // 99/04/09 #287528 vtan: If browse in same window is on then it's
            // possible that the same window is being reused and the sort by will
            // be on an extended column but the column will not be loaded. To
            // prevent this use the default state to do the sort for the initial
            // time only.

            if (!_bLoadedColumns)
            {
                int         iOldLastColumnClick, iOldDirection;
                LPARAM      lOldParamSort;
                SHELLSTATE  ss;

                lOldParamSort = _dvState.lParamSort;
                iOldLastColumnClick = _dvState.iLastColumnClick;
                iOldDirection = _dvState.iDirection;
                SHGetSetSettings(&ss, SSF_SORTCOLUMNS, FALSE); // get default sort column.
                _dvState.lParamSort = 
                _dvState.iLastColumnClick = (int)ss.lParamSort;
                _dvState.iDirection = ss.iSortDirection? ss.iSortDirection : 1 ;
                bRet = ListView_SortItems(_hwndListview, _Compare, (LPARAM)this);
                _dvState.lParamSort = lOldParamSort;
                _dvState.iLastColumnClick = iOldLastColumnClick;
                _dvState.iDirection = iOldDirection;
            }
            else
            {
                //  Sort on the extended column
                _LastSortColType = dwState & SHCOLSTATE_TYPEMASK;
                // The _bInSortCallBack flag tells the LVN_GETDISPINFO routine to return the data
                // (as opposed to kicking off a thread), and tell listview to store it so it won't ask again.
                _bInSortCallBack = TRUE;
                bRet = ListView_SortItemsEx(_hwndListview, _CompareExtended, (LPARAM)this);
                _bInSortCallBack = FALSE;
            }
        }
    }
    else 
    {
        // dont send a LVM_SORTITEMS to an ownerdraw or comctl32 will rip
        if (!_IsOwnerData())
        {
            bRet = ListView_SortItems(_hwndListview, _Compare, (LPARAM)this);
        }
    }

    // reset to the state that no items have been moved if currently in a positioned mode
    // so auto-arraning works.

    if (DV_ISANYICONMODE(_fs.ViewMode))
        _bItemsMoved = FALSE;

    TIMESTOP(_DV_ReArrange);

    TIMEOUT(_DV_ReArrange);

    return bRet ;
}

int CALLBACK CDefView::_DVITEM_Compare(void *p1, void *p2, LPARAM lParam)
{
    CDefView    *thisObject = reinterpret_cast<CDefView*>(lParam);

    UNALIGNED DVITEM *pdvi1 = (UNALIGNED DVITEM *)p1;
    UNALIGNED DVITEM *pdvi2 = (UNALIGNED DVITEM *)p2;
    LPITEMIDLIST pFakeEnd1, pFakeEnd2;
    USHORT uSave1, uSave2;
    int nCmp;

    // BUGBUG: Note that all this would be unnecessary if
    // IShellFolder::CompareIDs took a bRecurse flag
    pFakeEnd1 = _ILNext(&pdvi1->idl);
    uSave1 = pFakeEnd1->mkid.cb;
    pFakeEnd1->mkid.cb = 0;

    pFakeEnd2 = _ILNext(&pdvi2->idl);
    uSave2 = pFakeEnd2->mkid.cb;
    pFakeEnd2->mkid.cb = 0;

    nCmp = _Compare(&pdvi1->idl, &pdvi2->idl, reinterpret_cast<LPARAM>(thisObject));

    pFakeEnd2->mkid.cb = uSave2;
    pFakeEnd1->mkid.cb = uSave1;

    return(nCmp);
}


//
// This returns TRUE if everything went OK, FALSE otherwise
// Side effect: the listview items are always arranged after this call
//
BOOL CDefView::_RestorePos(PDVSAVEHEADER pSaveHeader, UINT uLen)
{
    UNALIGNED DVITEM *pDVItem, *pDVEnd;
    HDPA dpaItems;
    UNALIGNED DVITEM * UNALIGNED * ppDVItem, * UNALIGNED * ppEndDVItems;
    BOOL bOK = FALSE;
    int iCount, i;
    DWORD dwStyle = GetWindowStyle(_hwndListview);

    // Do the specified sorting for both the ListView and the DPA,
    // so we can traverse them both in order (like the merge step of a
    // merge sort), which should be pretty quick

    // 99/04/07 #287528 vtan: unless it's an extended column which by
    // definition is slow. The text for this is gathered using a DefView
    // task. There was a check for this here which is now removed.

    _SetSortArrows() ;
    _InternalRearrange();

#if 0
    // could hook this up if you think it is worth skipping this expensive code when its not necessary
    // If you do, investigate how long pSaveHeader stays around for
    if (((dwStyle & LVS_TYPEMASK) == LVS_REPORT) ||
        ((dwStyle & LVS_TYPEMASK) == LVS_LIST) )
    {
        // no point in arranging everything...
        // we need to set a flag to do this later..
        return TRUE;
    }
#endif
    pDVItem = (UNALIGNED DVITEM *)(((LPBYTE)pSaveHeader) + pSaveHeader->cbPosOffset);

    // BUGBUG more runtime size checking, should be init in case you don't get
    // here the day you happen to break its validity (DavePl)
    ASSERT(SIZEOF(DVSAVEHEADER) >= SIZEOF(DVITEM));

    pDVEnd = (UNALIGNED DVITEM *)(((LPBYTE)pSaveHeader) + uLen - SIZEOF(DVITEM));

    // Grow every 16 items
    dpaItems = DPA_Create(16);
    if (!dpaItems)
    {
        return bOK;
    }

    for ( ; ; pDVItem=(UNALIGNED DVITEM *)_ILNext(&pDVItem->idl))
    {
        if (pDVItem > pDVEnd)
        {
            // Invalid list
            break;
        }

        // End normally when we reach a NULL IDList
        if (pDVItem->idl.mkid.cb == 0)
        {
            break;
        }

        if (DPA_AppendPtr(dpaItems, pDVItem) < 0)
        {
            break;
        }
    }

    if (!DPA_Sort(dpaItems, _DVITEM_Compare, (LPARAM)this))
    {
        goto Error1;
    }

    ppDVItem = (UNALIGNED DVITEM * UNALIGNED *)DPA_GetPtrPtr(dpaItems);
    ppEndDVItems = ppDVItem + DPA_GetPtrCount(dpaItems);

    // Turn off auto-arrange if it's on at the mo.
    if (dwStyle & LVS_AUTOARRANGE)
        SetWindowLong(_hwndListview, GWL_STYLE, dwStyle & ~LVS_AUTOARRANGE);

    iCount = ListView_GetItemCount(_hwndListview);
    for (i=0; i<iCount; ++i)
    {
        LPITEMIDLIST pidl = _GetPIDL(i);

        // need to check for pidl because this could be on a background
        // thread and an fsnotify could be coming through to blow it away
        for ( ; pidl ; )
        {
            int nCmp;
            LPITEMIDLIST pFakeEnd;
            USHORT uSave;

            if (ppDVItem < ppEndDVItems)
            {
                // We terminate the IDList manually after saving
                // the needed information.  Note we will not GP fault
                // since we added sizeof(ITEMIDLIST) onto the Alloc
                pFakeEnd = _ILNext(&(*ppDVItem)->idl);
                uSave = pFakeEnd->mkid.cb;
                pFakeEnd->mkid.cb = 0;

                nCmp = _Compare(&((*ppDVItem)->idl), pidl, (LPARAM)this);

                pFakeEnd->mkid.cb = uSave;
            }
            else
            {
                // do this by default.  this prevents overlap of icons
                //
                // i.e.  if we've run out of saved positions information,
                // we need to just loop through and set all remaining items
                // to position 0x7FFFFFFFF so that when it's really shown,
                // the listview will pick a new (unoccupied) spot.
                // breaking out now would leave it were the _InternalRearrange
                // put it, but another item with saved state info could
                // have come and be placed on top of it.
                nCmp = 1;
            }


            if (nCmp > 0)
            {
                // We did not find the item
                // reset it's position to be recomputed
                ListView_SetItemPosition32(_hwndListview, i,
                                           0x7FFFFFFF, 0x7FFFFFFF);
                break;
            }
            else if (nCmp == 0)
            {
                UNALIGNED DVITEM * pDVItem = *ppDVItem;

                // They are equal
                ListView_SetItemPosition32(_hwndListview,
                                        i, pDVItem->pt.x, pDVItem->pt.y);
                // Don't check this one again
                ++ppDVItem;
                break;
            }

            // It's less than the current item, so try the next one
            ++ppDVItem;
        }
    }

    // Turn auto-arrange back on if needed...
    if (dwStyle & LVS_AUTOARRANGE)
        SetWindowLong(_hwndListview, GWL_STYLE, dwStyle);

    bOK = TRUE;

    if (DPA_GetPtrCount(dpaItems) > 0)
    {
        // If we read in any icon positions, we should save them later
        // unless the user does something to cause us to go back to
        // the default state
        _bItemsMoved = TRUE;
    }

Error1:;
    DPA_Destroy(dpaItems);
    return(bOK);
}

//
// Save (and check) column header information
// Returns TRUE if the columns are the default width, FALSE otherwise
// Side effect: the stream pointer is left right after the last column
//              EVEN when on default width!
//
BOOL CDefView::SaveCols(IStream *pstm)
{
    BOOL bDefaultCols = FALSE;
    IStream *pstmCols = NULL;

    if (!_psd && !_pshf2 && !this->HasCB())
        return TRUE;

    if (SUCCEEDED(CallCB(SFVM_GETCOLSAVESTREAM, STGM_WRITE, (LPARAM)&pstmCols)))
    {
        // note: this case typically happens on non-FS folders
        TraceMsg(TF_DEFVIEW, "dv::SaveCols got view-specific save stream");
        // kinda hard to get the columns in a non-default state without loading them :)
        if (!_bLoadedColumns)
        {
            pstmCols->Release();
            return TRUE;
        }
        pstm = pstmCols;
    }
    else
    {
        if (!pstm)
            return TRUE;    // Bail out say default...
    }

    // should we set _bLoadedColumns?
    if (!_pcp && _pSaveHeader)
    {
        LPBYTE pColHdr;
        _pSaveHeader->GetColumnsInfo(&pColHdr);

        _pcp = new CColumnPointer(pColHdr, NULL); // pass NULL instead of this, to avoid calling IVC
    }

    if (!_pcp)
    {
        if (pstmCols)
            pstmCols->Release();
        return TRUE;
    }

    // Make sure we have stored the current widths
    if (!_pcp->SaveColumnWidths(_hwndListview))
        bDefaultCols = FALSE;

    if (!_pcp->SaveColumnOrder(_hwndListview))
    {
        // Make sure to save state if the column order changed
        bDefaultCols = FALSE;
    }

    if (FAILED(_pcp->Write(pstm, this)))
    {
        // There is some problem, so just assume
        // default column widths
        bDefaultCols = TRUE;
    }

    if (pstmCols)
    {
        ATOMICRELEASE(pstmCols);

        // Always pretend we got default columns
        return TRUE;
    }

    TraceMsg(TF_DEFVIEW, "dv::SaveCols  returning %s",bDefaultCols ? TEXT("TRUE") : TEXT("FALSE"));

    return bDefaultCols;
}


//
// Save (and check) icon position information
// Returns S_OK if the positions are saved, S_FALSE if we don't need to save,
// E_FAIL on error.
// Side effect: the stream pointer is left right after the last icon
//
HRESULT CDefView::SavePos(IStream *pstm)
{
    int iCount, i;
    DVITEM dvitem;
    HRESULT hres;

    if (!_bItemsMoved || !DV_ISANYICONMODE(_fs.ViewMode) || !_HasNormalView())
        return S_FALSE;

    iCount = ListView_GetItemCount(_hwndListview);

    for (i = 0; ; i++)
    {
        if (i >= iCount)
        {
            hres = S_OK;
            break;
        }

        ListView_GetItemPosition(_hwndListview, i, &dvitem.pt);

        hres = pstm->Write(&dvitem.pt, SIZEOF(dvitem.pt), NULL);
        if (FAILED(hres))
            break;

        LPITEMIDLIST pidl = _GetPIDL(i);
        if (pidl)
            hres = pstm->Write(pidl, pidl->mkid.cb, NULL);
        else
            hres = E_FAIL;

        if (FAILED(hres))
            break;
    }

    if (SUCCEEDED(hres))
    {
        // Terminate the list with a NULL IDList
        dvitem.idl.mkid.cb = 0;
        hres = pstm->Write(&dvitem, SIZEOF(dvitem), NULL);
    }

    return hres;
}

// this should NOT check for whether the item is already in the listview
// if it does, we'll have some serious performance problems
int DefView_AddObject(CDefView *pdsv, LPITEMIDLIST pidl, BOOL bCopy=FALSE)
{
    int i;
    LV_ITEM item;

    TIMESTART(pdsv->_AddObject);

    // Check the commdlg hook to see if we should include this
    // object.
    if (DV_CDB_IncludeObject(pdsv, pidl) != S_OK)
    {
        TIMESTOP(pdsv->_AddObject);
        return -1;
    }

    if (S_FALSE == pdsv->CallCB(SFVM_INSERTITEM, 0, (LPARAM)pidl))
    {
        // Don't add this object
        TIMESTOP(pdsv->_AddObject);
        return -1;
    }

    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    item.iItem = INT_MAX;     // add at end
    item.iSubItem = 0;

    item.iImage = I_IMAGECALLBACK;
    item.pszText = LPSTR_TEXTCALLBACK;
    if (bCopy)
    {
        pidl = ILClone(pidl);
        if (!pidl)
        {
            TIMESTOP(pdsv->_AddObject);
            return -1;
        }
    }

    item.lParam = (LPARAM)pidl;

    i = ListView_InsertItem(pdsv->_hwndListview, &item);

    if (bCopy && i<0)
    {
        ILFree(pidl);
    }

    // If this is the first item added, notify automation
    if (i == 0)
        pdsv->_PostSelChangedMessage();

    TIMESTOP(pdsv->_AddObject);
    return i;
}

// Find the relative pidl, if it exists
//
int CDefView::_FindItem(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlFound, BOOL fSamePtr)
{
    int iItem;
    int cItems;
    int cCounter;

    // this f-n can be called during drag, to get the points for the items being dragged
    // (for drag image).  in the case of docfind folder the pidls packed in dataobject 
    // are full pidls because the source of the items in not necessarily one folder.
    // in that case all the calls bellow are on docfind folder which knows how to handle
    // full pidls, so in ownerdata cases the pidls may not be relative
    RIP(_IsOwnerData() || ILFindLastID(pidl) == pidl);

    cItems = ListView_GetItemCount(_hwndListview);

    if (_iLastFind >= cItems)
        _iLastFind = 0;

    iItem = _iLastFind;
    if (SUCCEEDED(CallCB(SFVM_INDEXOFITEMIDLIST, (WPARAM)&iItem, (LPARAM)pidl)))
    {
        if (ppidlFound)
            *ppidlFound = _GetPIDL(iItem);
        return iItem;
    }

    for (cCounter = 0, iItem = _iLastFind; cCounter < cItems; iItem = (iItem + 1) % cItems, cCounter++) 
    {
        HRESULT hres = ResultFromShort(-1);
        LPITEMIDLIST pidlT = _GetPIDL(iItem);
        if (!pidlT)
            return -1;

        if (pidlT == pidl) 
        {
            hres = ResultFromShort(0);
        } 
        else if (!fSamePtr) 
        {
            // if we don't insist on being the same pointer, do the ole style compare
            // BUGBUG: this passes 0 for the lParam
            hres = _pshf->CompareIDs(0, pidl, pidlT);
        }

        ASSERT(SUCCEEDED(hres));
        if (FAILED(hres))
            return -1;

        if (ShortFromResult(hres) == 0)
        {
            if (ppidlFound)
                *ppidlFound = pidlT;
            _iLastFind = iItem;
#ifdef FINDCACHE_DEBUG
            TraceMsg(TF_DEFVIEW, "####FIND CACHE RESULT --- %s by %d", cCounter < iItem ? TEXT("WIN") : TEXT("LOSE"), iItem - cCounter);
#endif
            return iItem;
        }
    }

    _iLastFind = 0;
    return -1;  // not found
}


//
// Function to process the SFVM_REMOVEOBJECT message, by searching
// through the list for a match of the pidl.  If a match is found, the
// item is removed from the list and the index number is returned, else
// -1 is returned.

int CDefView::_RemoveObject(LPCITEMIDLIST pidl, BOOL fSamePtr)
{
    int i;

    // Docfind will pass in a null pointer to tell us that it wants
    // to refresh the window by deleting all of the items from it.
    if (pidl == NULL)
    {
        // notify the iSHellFolder
        CallCB(SFVM_DELETEITEM, 0, 0);
        ListView_DeleteAllItems(_hwndListview);
        _dwAttrSel = (DWORD)-1;        // Throw away cached information about sel attributes.
        _PostSelChangedMessage();           // and make sure a selection changed notify goes out.
        return 0;
    }

    // Non null go look for item.
    i = _FindItem(pidl, NULL, fSamePtr);
    if (i >= 0)
    {
        RECT rc;
        UINT uState = ListView_GetItemState(_hwndListview, i, LVIS_ALL);
        UINT uCount = 0;

        if (uState & LVIS_FOCUSED)
            ListView_GetItemRect(_hwndListview, i, &rc, LVIR_ICON);

        // do the actual delete
        ListView_DeleteItem(_hwndListview, i);

        // we deleted the focused item.. replace the focus to the nearest item.
        if (uState & LVIS_FOCUSED) 
        {
            int iFocus = i;
            if (DV_ISANYICONMODE(_fs.ViewMode)) 
            {
                LV_FINDINFO lvfi;

                lvfi.flags = LVFI_NEARESTXY;
                lvfi.pt.x = rc.left;
                lvfi.pt.y = rc.top;
                lvfi.vkDirection = 0;
                iFocus = ListView_FindItem(_hwndListview, -1, &lvfi);
            } 
            else  
            {
                if (ListView_GetItemCount(_hwndListview) >= iFocus)
                    iFocus--;
            }

            if (iFocus != -1) 
            {
                ListView_SetItemState(_hwndListview, iFocus, LVIS_FOCUSED, LVIS_FOCUSED);
                ListView_EnsureVisible(_hwndListview, iFocus, FALSE);
            }
        }

        // Notify automation if the listview is now empty
        GetObjectCount(&uCount);
        if (!uCount)
            _PostSelChangedMessage();
    }
    return i;
}

//
// Function to process the SFVM_UPDATEOBJECT message, by searching
// through the list for a match of the first pidl.  If a match is found,
// the item is updated to the second pidl...
//
// Win95's defview supported SFVM_UPDATEOBJECT by swapping the old pidl
// for the new pidl.  It did *not* release the old pidl (causing a leak)
// and it did *not* release the new pidl (until the view went away).
// So buggy callers could still reference their pidl, even though they
// technically handed the pidl off.  IE4 fixed the memory leaks by treating
// everything correctly, including releasing the passed in pidl right away.
// This exposed at least one caller (scheduled tasks, see nt5 bug 123780)
// which referenced their pidl after passing it in.  We could fix this in
// NT5 via an apphack, but since IE4 already shipped, I don't see
// much reason to.
//
// Note: bCopy controls copying only of ppidl[1].  ppidl[0] is never copied.
//
int CDefView::_UpdateObject(LPITEMIDLIST *ppidl, BOOL bCopy /* = FALSE */)
{
    LPITEMIDLIST pidlOld;
    int i = _FindItem(ppidl[0], &pidlOld, FALSE);

    if (i >= 0)
    {
        LPITEMIDLIST pidlNew;
        if (bCopy)
        {
            pidlNew = ILClone(ppidl[1]);
            if (!pidlNew)
                return -1;
        }
        else
            pidlNew = ppidl[1];  // update the second item.

        if (_IsOwnerData())
        {
            if (SUCCEEDED(CallCB(SFVM_SETITEMIDLIST, i, (LPARAM)pidlNew)))
            {
                ListView_Update(_hwndListview, i);
                if (bCopy)
                    ILFree(pidlNew);
            }
            else
            {
                // we failed, try to cleanup and bail.
                if (bCopy)
                    ILFree(pidlNew);
                return -1;
            }
        }
        else
        {
            LV_ITEM item;
            BOOL    bSelected;
            SFVM_SELCHANGE_DATA dvsci;

            //
            // We found the item so lets now update it in the
            // the view.
            //
            item.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
            item.iItem = i;
            item.pszText = LPSTR_TEXTCALLBACK;
            item.iImage = I_IMAGECALLBACK;
            item.iSubItem = 0;  // REVIEW: bug in listview?
            item.lParam = (LPARAM)pidlNew;

            //since directory is cashing the size of both the selected and all
            //items we need to let it know that one/both changed
            bSelected = (ListView_GetItemState(_hwndListview, i, LVIS_SELECTED) & LVIS_SELECTED);

            // if selected, deselect it
            if (bSelected)
            {
                dvsci.uNewState = 0;
                dvsci.uOldState = LVIS_SELECTED;
                dvsci.lParamItem = (LPARAM)pidlOld;

                CallCB(SFVM_SELCHANGE,
                             MAKEWPARAM(SFVIDM_CLIENT_FIRST, i),
                             (LPARAM)&dvsci);
            }
            // now remove it (to force its old size to be subtracted from the cached size)
            CallCB(SFVM_DELETEITEM, 0, (LPARAM)pidlOld);
            // now insert it with a new pidl
            CallCB(SFVM_INSERTITEM, 0, (LPARAM)pidlNew);
            // if it was selected, select it again
            if (bSelected)
            {
                dvsci.uNewState = LVIS_SELECTED;
                dvsci.uOldState = 0;
                dvsci.lParamItem = (LPARAM)pidlNew;

                CallCB(SFVM_SELCHANGE,
                             MAKEWPARAM(SFVIDM_CLIENT_FIRST, i),
                             (LPARAM)&dvsci);
            }

            ListView_SetItem(_hwndListview, &item);

            //  Force update of all remaining columns.
            HWND hwndHeader = ListView_GetHeader(_hwndListview);
            int  cCols      = Header_GetItemCount(hwndHeader);
            for( item.iSubItem++; item.iSubItem < cCols; item.iSubItem++ )
            {
                ListView_SetItemText( _hwndListview, item.iItem, item.iSubItem, 
                                      LPSTR_TEXTCALLBACK );
            }
        }

        // Free the old pidl after we've added the new one
        // Don't do this if owner data as we don't know how they allocated this info...
        if (!_IsOwnerData())
            ILFree(pidlOld);

        // Automation may want to know about the change
        CheckIfSelectedAndNotifyAutomation(ppidl[0], i);
    }
    return i;
}

//
//  invalidates all items with the given image index.
//
//  or update all items if iImage == -1
//
void DefView_UpdateImage(CDefView *pdsv, int iImage)
{
    LV_ITEM item;
    int cItems;

    TraceMsg(TF_DEFVIEW, "DefView_UpdateImage: %d", iImage);

    //
    //  -1 means update all
    //  reset the imagelists incase the size has changed, and do
    //  a full update.
    //
    if (iImage == -1)
    {
        HIMAGELIST himlLarge, himlSmall;

        Shell_GetImageLists(&himlLarge, &himlSmall);

        ListView_SetImageList(pdsv->_hwndListview, himlLarge, LVSIL_NORMAL);
        ListView_SetImageList(pdsv->_hwndListview, himlSmall, LVSIL_SMALL);

        pdsv->ReloadContent();
        return;
    }

    //
    // get a dc so we can optimize for visible/not visible cases
    //
    HDC hdcLV = GetDC(pdsv->_hwndListview);

    //
    // scan the listview updating any items which match
    //
    item.iSubItem = 0;
    cItems = ListView_GetItemCount(pdsv->_hwndListview);
    for (item.iItem = 0; item.iItem < cItems; item.iItem++)
    {
        int iImageOld;

        item.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_NORECOMPUTE;

        ListView_GetItem(pdsv->_hwndListview, &item);
        iImageOld = item.iImage;

        if (item.iImage == iImage)  // this filters I_IMAGECALLBACK for us
        {
            RECT rc;
            LPCITEMIDLIST pidl = DV_GetPIDLParam(pdsv, item.lParam, item.iItem);

            Icon_FSEvent(SHCNE_UPDATEITEM, pidl, NULL);

            //
            // if the item is visible then we don't want to flicker so just
            // kick off an async extract.  if the item is not visible then
            // leave it for later by slamming in I_IMAGECALLBACK.
            //
            item.iImage = I_IMAGECALLBACK;

            if (ListView_GetItemRect(pdsv->_hwndListview, item.iItem, &rc, LVIR_ICON) &&
                RectVisible(hdcLV, &rc))
            {
                int iImageNew;
                HRESULT hres = pdsv->_GetIconAsync(pidl, &iImageNew, FALSE);

                if (hres == S_FALSE)
                    continue;

                if (SUCCEEDED(hres))
                {
                    if (iImageNew == iImageOld)
                    {
                        ListView_RedrawItems( pdsv->_hwndListview, item.iItem, item.iItem );
                        continue;
                    }

                    item.iImage = iImageNew;
                }
            }

            item.mask = LVIF_IMAGE;
            item.iSubItem = 0;
            ListView_SetItem(pdsv->_hwndListview, &item);
        }
    }

    ReleaseDC(pdsv->_hwndListview, hdcLV);
}

// Function to process the SFVM_REFRESHOBJECT message, by searching
// through the list for a match of the first pidl.  If a match is found,
// the item is redrawn.

int CDefView::_RefreshObject(LPITEMIDLIST *ppidl)
{
    // BUGBUG: should support refreshing a range of pidls
    int i = _FindItem(ppidl[0], NULL, FALSE);
    if (i >= 0)
        ListView_RedrawItems(_hwndListview, i, i);
    return i;
}


//
// Function to process the SFVM_GETSELECTEDOBJECTS message
//

HRESULT DefView_GetItemObjects(CDefView *pdsv, LPCITEMIDLIST **ppidl, UINT uItem, UINT *pcItems)
{
    UINT cItems = DefView_GetItemPIDLS(pdsv, NULL, 0, uItem);
    *pcItems = cItems;
    if (ppidl)
    {
        *ppidl = NULL;

        if (cItems == 0)
            return S_OK;  // nothing allocated...

        *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, SIZEOF(LPITEMIDLIST) * cItems);
        if (!*ppidl)
            return E_OUTOFMEMORY;

        cItems = DefView_GetItemPIDLS(pdsv, *ppidl, cItems, uItem);
    }
    *pcItems = cItems;
    return S_OK;
}


void DefView_SetItemPos(CDefView *pdsv, LPSFV_SETITEMPOS psip)
{
    int i = pdsv->_FindItem(psip->pidl, NULL, FALSE);
    if (i >= 0)
    {
        ListView_SetItemPosition32(pdsv->_hwndListview, i, psip->pt.x, psip->pt.y);

        pdsv->_bItemsMoved = TRUE;
        pdsv->_bClearItemPos = FALSE;
    }
}

#define DV_IDTIMER              1
#define ENUMTIMEOUT             3000    // 3 seconds
#define SHORTENUMTIMEOUT        500     // 1/2 second


HRESULT DV_AllocRestOfStream(IStream *pstm, void **ppData, UINT *puLen)
{
    UINT uLen;
    ULARGE_INTEGER libCurPos;
    ULARGE_INTEGER libEndPos;

    pstm->Seek(g_li0, STREAM_SEEK_CUR, &libCurPos);
    pstm->Seek(g_li0, STREAM_SEEK_END, &libEndPos);

    uLen = libEndPos.LowPart - libCurPos.LowPart;
    // Note that we add room for an extra ITEMIDLIST so we don't GP fault
    // when we manually terminate the last ID list
    if (uLen == 0)
    {
        return(E_UNEXPECTED);
    }

    // Allow the caller to have some extra room
    uLen += *puLen;
    if ((*ppData = (void*)LocalAlloc(LPTR, uLen)) == NULL)
    {
        return(E_OUTOFMEMORY);
    }
    *puLen = uLen;

    pstm->Seek(*(LARGE_INTEGER *)&libCurPos, STREAM_SEEK_SET, NULL);
    // This really should not fail
    pstm->Read(*ppData, uLen, NULL);

    return S_OK;
}


HRESULT DV_AllocNewStream(IStream *pstm, void **ppData, UINT *puLen)
{
    struct {
        DVSAVEHEADER dvSaveHeader;
        DVSAVEHEADEREX dvSaveHeaderEx;
    } dv;
    ULARGE_INTEGER libStartPos;
    LARGE_INTEGER dlibMove;
    ULONG cbRead;
    int uLen;
    HRESULT hres;

    // assume failure
    *ppData = NULL;

    // remember the starting point in the stream
    dlibMove.LowPart = dlibMove.HighPart = 0;
    hres = pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libStartPos);
    if (FAILED(hres))
    {
        return hres;
    }

    // read the headers
    hres = pstm->Read(&dv, SIZEOF(dv), &cbRead);
    if (FAILED(hres))
    {
        return hres;
    }

    // if it's our extended header, we're golden
    if ( SIZEOF(dv)==cbRead &&
        dv.dvSaveHeader.cbSize == SIZEOF(DVSAVEHEADER) &&
        dv.dvSaveHeader.cbColOffset == 0 &&
        dv.dvSaveHeaderEx.dwSignature == DVSAVEHEADEREX_SIGNATURE &&
        dv.dvSaveHeaderEx.cbSize >= SIZEOF(DVSAVEHEADEREX))
    {
        if (dv.dvSaveHeaderEx.wVersion < DVSAVEHEADEREX_VERSION)
        {
            // We used to store szExtended in here -- not any more
            dv.dvSaveHeaderEx.dwUnused = 0;
        }

        // Allocate a buffer for the entire stream
        uLen = dv.dvSaveHeaderEx.cbStreamSize;
        *ppData = (PDVSAVEHEADER)LocalAlloc(LPTR, uLen + *puLen);
        if (NULL != *ppData)
        {
            // patch up cbColOffset
            dv.dvSaveHeader.cbColOffset = dv.dvSaveHeaderEx.cbColOffset;

            // copy what we've already read
            ASSERT(dv.dvSaveHeaderEx.cbStreamSize>=SIZEOF(dv));
            memcpy(*ppData, &dv, SIZEOF(dv));

            // read the rest of the data
            hres = pstm->Read((LPBYTE)*ppData + SIZEOF(dv), uLen - SIZEOF(dv), &cbRead);
            if (FAILED(hres) || cbRead != uLen - SIZEOF(dv))
            {
                LocalFree(*ppData);
                *ppData = NULL;
            }
        }
    }

    if (NULL != *ppData)
    {
        *puLen += uLen;
        hres = S_OK;
    }
    else
    {
        // we failed, make sure we don't change the stream position
        dlibMove.LowPart = libStartPos.LowPart;
        pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

        hres = E_FAIL;
    }

    return hres;
}

UINT CDefView::_GetSaveHeader(PDVSAVEHEADER *ppSaveHeader)
{
    IStream *pstm;
    UINT uLen = SIZEOF(ITEMIDLIST);

    *ppSaveHeader = NULL;

//  99/02/05 #226140 vtan: Try to get the view state stream
//  from ShellBrowser. If that fails then look for a global
//  view state stream that is stored when the user clicks on
//  the "Like Current Folder" in the View tab of folder settings.

//  IShellBrowser::GetViewStateStream() match the dwDefRevCount
//  of the cabinet state to make sure that it's valid.

    if (FAILED(_psb->GetViewStateStream(STGM_READ, &pstm)) &&
        FAILED(_LoadGlobalViewState(&pstm)))
        return 0;

    // first try new stream format
    if (FAILED(DV_AllocNewStream(pstm, (void **)ppSaveHeader, &uLen)))
    {
        ASSERT(uLen == SIZEOF(ITEMIDLIST));

        // then try old format
        if (FAILED(DV_AllocRestOfStream(pstm, (void **)ppSaveHeader, &uLen)))
        {
            uLen = 0;
        }
        else
        {
            if (uLen<SIZEOF(DVSAVEHEADER)+SIZEOF(ITEMIDLIST)
                || (*ppSaveHeader)->cbSize != SIZEOF(DVSAVEHEADER))
            {
                LocalFree(*ppSaveHeader);
                *ppSaveHeader = 0;
                uLen = 0;
            }
            else
            {
                // ppSaveHeader points to a valied pre-IE4 DVSAVEHEADER.
                // Put upgrade code here.
                //
            }
// BUGBUG: dan, is this what you wanted?
//            else
//                (*ppSaveHeader)->dvState = _dvState;
        }
    }

    // Massage values if necessary
    if (*ppSaveHeader)
        ValidateDVState( &(*ppSaveHeader)->dvState );

    ATOMICRELEASE(pstm);

    return uLen;
}

#if 0 // we currently don't store anything we care about
PDVSAVEHEADEREX DefView_GetSaveHeaderEx(PDVSAVEHEADER pSaveHeader)
{
    // Do we have room for an extended header?
    if (pSaveHeader && pSaveHeader->cbPosOffset >= SIZEOF(DVSAVEHEADER) + SIZEOF(DVSAVEHEADEREX))
    {
        PDVSAVEHEADEREX pSaveHeaderEx = (PDVSAVEHEADEREX)(pSaveHeader+1);
        // Verify the extended header
        if (pSaveHeaderEx->dwSignature == DVSAVEHEADEREX_SIGNATURE &&
            pSaveHeaderEx->cbSize >= SIZEOF(DVSAVEHEADEREX))
        {
            // A random place to verify this, but we should do it somewhere
            ASSERT(pSaveHeader->cbColOffset == 0 || pSaveHeader->cbColOffset >= SIZEOF(DVSAVEHEADER) + SIZEOF(DVSAVEHEADEREX));

            return(pSaveHeaderEx);
        }
    }

    return(NULL);
}
#endif

// restore the window state
//
//    icon positions
//    window scroll position
//

void CDefView::_RestoreState(PDVSAVEHEADER pInSaveHeader, UINT uLen)
{
    PDVSAVEHEADER pSaveHeader;

    if (pInSaveHeader)
    {
        pSaveHeader = pInSaveHeader;
    }
    else
    {
        uLen = _GetSaveHeader(&pSaveHeader);
        if (uLen == 0)
        {
            _InternalRearrange();
            return;
        }
    }

    _dvState = pSaveHeader->dvState;

    // Columns get restored during window creation

    // make sure the view modes of the saved state match what we have now

    if (pSaveHeader->ViewMode == _fs.ViewMode)
    {
        // If we restored all the icon positions restore the scroll position too
        if (_RestorePos(pSaveHeader, uLen))
            ListView_Scroll(_hwndListview, pSaveHeader->ptScroll.x, pSaveHeader->ptScroll.y);
    }
    else
    {
        TraceMsg(TF_WARNING, "restore state view modes don't match (%d != %d)", pSaveHeader->ViewMode, _fs.ViewMode);
    }

    if (!pInSaveHeader)
        LocalFree((HLOCAL)pSaveHeader);
}

//
//  This function can be called only when we are filling listview items
// (from within ::FillObjects. It is very important to pass consistent
// dwFlags to GetDisplayNameOf and SetNameOf.
//
void DefView_UpdateGlobalFlags(CDefView *pdsv)
{
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS | SSF_SHOWCOMPCOLOR, FALSE);
    pdsv->_fShowAllObjects = ss.fShowAllObjects;

#ifdef WINNT
    // Don't allow compression coloring on the desktop proper
    pdsv->_fShowCompColor  = pdsv->_IsDesktop() ? FALSE : ss.fShowCompColor;
#endif
}

//
//  We refreshed the view.  Take the old pidls and new pidls and compare
//  them, doing a DefView_AddObject for all the new pidls, DefView_RemoveObject
//  for the deleted pidls, and _UpdateObject for the inplace modifies.
//
void CDefView::_FilterDPAs(HDPA hdpa, HDPA hdpaOld)
{
    int i, j;
    LPITEMIDLIST pidl, pidlOld;
    LPARAM lSupportsIdentity = 0;

    // See if the folder supports comparisons on column -1 (test for
    // pidl complete comparisions.  If so, then we can support inplace
    // pidl modification as well as just add and remove.

    if (HasCB() && SUCCEEDED(CallCB(SFVM_SUPPORTSIDENTITY, 0, 0)))
        lSupportsIdentity = SHCIDS_ALLFIELDS;

#ifdef DPA_FILTER_TEST
    for (i = 0; i < DPA_GetPtrCount(hdpaOld); i++) 
    {
        pidl = DPA_FastGetPtr(hdpaOld, i);
        TraceMsg(TF_DEFVIEW, "pidl = %x, %x, %x", pidl, *(DWORD*)pidl, i);
    }
#endif

    // do the compares
    for (;;) 
    {
        int iCompare;
        i = DPA_GetPtrCount(hdpaOld);
        j = DPA_GetPtrCount(hdpa);

        if (!i && !j)
            break;

        if (!i) {
            // only new ones left.  Insert all of them.
            iCompare = -1;
            pidl = (LPITEMIDLIST)DPA_FastGetPtr(hdpa, 0);

        } 
        else if (!j) 
        {
            // only old ones left.  remove them all.
            iCompare = 1;
            pidlOld = (LPITEMIDLIST)DPA_FastGetPtr(hdpaOld, 0);
        } 
        else 
        {
            HRESULT hres;

            pidlOld = (LPITEMIDLIST)DPA_FastGetPtr(hdpaOld, 0);
            pidl = (LPITEMIDLIST)DPA_FastGetPtr(hdpa, 0);

            LPARAM lParam = (LPARAM) (((ULONG_PTR)(_dvState.lParamSort)) | lSupportsIdentity);
            hres = _pshf->CompareIDs(lParam, pidl, pidlOld);
            if (FAILED(hres))
            {
                DVSAVESTATE     saveState;

                _GetSortDefaults(&saveState);
                hres = _pshf->CompareIDs(saveState.lParamSort | lSupportsIdentity, pidl, pidlOld);
                if (FAILED(hres))
                {
                    hres = _pshf->CompareIDs(lSupportsIdentity, pidl, pidlOld);
                }
            }

            ASSERT(SUCCEEDED(hres));
            ASSERT(_dvState.iDirection != 0);
            iCompare = (short)HRESULT_CODE(hres) * _dvState.iDirection;
        }

        if (iCompare == 0) 
        {
            // they're the same ,remove one of each.
            ILFree(pidl);
            DPA_DeletePtr(hdpa, 0);
            DPA_DeletePtr(hdpaOld, 0);
        } 
        else 
        {
            // Not identical.  See if it's just a modify.
            if (lSupportsIdentity && i && j)
            {
                HRESULT hres = _pshf->CompareIDs(_dvState.lParamSort, pidl, pidlOld);
                if (FAILED(hres))
                {
                    DVSAVESTATE     saveState;

                    _GetSortDefaults(&saveState);
                    hres = _pshf->CompareIDs(saveState.lParamSort, pidl, pidlOld);
                    if (FAILED(hres))
                    {
                        hres = _pshf->CompareIDs(0, pidl, pidlOld);
                    }
                }
                ASSERT(SUCCEEDED(hres));
                iCompare = (short)HRESULT_CODE(hres) * _dvState.iDirection;
            }
            if (iCompare == 0) 
            {
                LPITEMIDLIST pp[2] = { pidlOld, pidl };
                if (_UpdateObject(pp) < 0)
                    ILFree(pidl);
                DPA_DeletePtr(hdpa, 0);
                DPA_DeletePtr(hdpaOld, 0);
            } 
            else if (iCompare < 0) 
            {
                // the new item!
                if (DefView_AddObject(this, pidl) == -1)
                    ILFree(pidl);
                DPA_DeletePtr(hdpa, 0);
            } 
            else 
            {
#ifdef DPA_FILTER_TEST
                // old item, delete it.
                TraceMsg(TF_DEFVIEW, "remove pidl = %x, %x", pidlOld, *(DWORD*)pidlOld);
#endif
                _RemoveObject(pidlOld, TRUE);
                DPA_DeletePtr(hdpaOld, 0);
            }
        }
    }

}

// this is only called from within SHCNE_*  don't put up ui on the enum error.
void DefView_Update(CDefView *pdsv)
{
    if (pdsv->_bBkFilling)
    {

        // 99/05/11 #301779 vtan: If there is a background fill and another update
        // is requested then the background fill should dump what's it done and
        // DefView should start all over again. This manifests in the following
        // scenario.
        //  1. 500+ items in a folder.
        //  2. select all items.
        //  3. type "delete" key.
        //  4. 10 SHCNE_UPDATEITEM items get sent.
        //  5. one of these causes a DefView_Update(this).
        //  6. there are still 500+ items so a background thread gets fired to
        //      enumerate the directory contents.
        //  7. the final SHCNE_UPDATEDIR gets sent.
        //  8. DefView_Update(this) is invoked in the default: case.
        //  9. this->_bBkFilling is true so a new enumeration object is not
        //      created and only the current enumeration object is enumerated
        //      to completion which isn't accurate to the current state.

        THR(pdsv->_pScheduler->RemoveTasks(TOID_DVBackgroundEnum, ITSAT_DEFAULT_LPARAM, FALSE));
        pdsv->_bBkFilling = FALSE;
    }
    pdsv->FillObjectsShowHide(FALSE, NULL, 0, FALSE);
}

#define HM_OK 0
#define HM_ABORT 1
#define HM_DESTROY 2

#ifdef DEADCODE
UINT HandleMessages(HWND hwnd, HWND _hwndMain)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {

        // intercept these messages
        if ((msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYDOWN) && msg.wParam == VK_ESCAPE)
            return HM_ABORT;

        if (((msg.hwnd == hwnd) || (msg.hwnd == _hwndMain)) &&
             ( ((msg.message == WM_SYSCOMMAND) && (msg.wParam == SC_CLOSE)) ||
              (msg.message == WM_DESTROY) || (msg.message == WM_CLOSE) ||
              (msg.message == WM_QUIT)))
        {
            PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
            TraceMsg(TF_DEFVIEW, "Got WM_SYSCOMMAND SC_CLOSE!");
            return HM_DESTROY;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }
    return HM_OK;
}
#endif

void CDefView::_ShowControl(UINT idControl, int idCmd)
{
    IBrowserService *pbs;

    if (SUCCEEDED(_psb->QueryInterface(IID_IBrowserService, (void **)&pbs))) {
        pbs->ShowControlWindow(idControl, idCmd);
        ATOMICRELEASE(pbs);
    }
}

bool    IsSingleWindowBrowsing (void)

{
    CABINETSTATE    cabinetState;

    TBOOL(ReadCabinetState(&cabinetState, sizeof(cabinetState)));
    return(!BOOLIFY(cabinetState.fNewWindowMode));
}

//----------------------------------------------------------------------------
// Alter the size of the parent to best fit around the items we have.
void ViewWindow_BestFit(CDefView *pdsv, BOOL bTimeout)
{
    const int cxMin = MINVIEWWIDTH, cyMin = MINVIEWHEIGHT;
    const int cxSpacing = GetSystemMetrics(SM_CXICONSPACING);
    const int cySpacing = GetSystemMetrics(SM_CYICONSPACING);
    DWORD dwStyle = GetWindowStyle(pdsv->_hwndListview);
    RECT rc, rcWork;
    WINDOWPLACEMENT wp;
    int cItems, cxScreen, cyScreen;
    int cxMax, cyMax;
    int cxWorkArea;
    int cyWorkArea;
    int iAdjustFactor;

    // have we already best fit this window? don't do it twice

    if (!(pdsv->_fs.fFlags & FWF_BESTFITWINDOW))
        return;

    // Don't try to do it again.
    pdsv->_fs.fFlags &= ~FWF_BESTFITWINDOW;


    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
    cxWorkArea = (rcWork.right - rcWork.left);
    cyWorkArea = (rcWork.bottom - rcWork.top);

//  99/02/06 #286723 vtan: Adjust best fit size to 4/5 if using
//  less than 1024x768.

    iAdjustFactor = 3;
    if ((cxWorkArea < 1000) && (cyWorkArea < 700))
        ++iAdjustFactor;
    cxMax = (cxWorkArea * iAdjustFactor) / 5;
    cyMax = (cyWorkArea * iAdjustFactor) / 5;

    int iITbar = SBSC_HIDE;
    int iStdBar = SBSC_HIDE;

    switch (pdsv->_uDefToolbar) {
    case HIWORD(TBIF_INTERNETBAR):
        iITbar = SBSC_SHOW;
        goto ShowToolbar;

    case HIWORD(TBIF_STANDARDTOOLBAR):
        iStdBar = SBSC_SHOW;
        goto ShowToolbar;

    case HIWORD(TBIF_NOTOOLBAR):
ShowToolbar:
        pdsv->_ShowControl(FCW_INTERNETBAR, iITbar);
        pdsv->_ShowControl(FCW_TOOLBAR, iStdBar);
        break;
    }
    // Normal views use the same code as before

    // Find the largest square.
    // 4000 should be close enough to infinite
    if (bTimeout)
    {
        cItems = 4000;
        // Give Control Panel a chance to tell us it will
        // probably only have a few items, even though it timed
        // out.
        pdsv->CallCB(SFVM_DEFITEMCOUNT, 0, (LPARAM)&cItems);
    }
    else
    {
        cItems = ListView_GetItemCount(pdsv->_hwndListview);
    }

    // Give docfind a chance to tell us it will eventually
    // have a lot of items, even if it doesn't have any now...
    pdsv->CallCB(SFVM_OVERRIDEITEMCOUNT, 0, (LPARAM)&cItems);


    rc.left = rc.top = rc.right = rc.bottom = 0;
    if (cItems)
    {
        int cy, i1, i2, i3;

        i1 = 1;
        i2 = 0;
        i3 = cItems;
        while (i3 > 0)
        {
            i3 -= i1;
            i1 += 2;
            i2++;
        }

        // Convert this into an equivalent rect for the effective
        // client area.
        // The width.
        rc.right = (i2 * cxSpacing);
        if (rc.right > cxMax)
        {
            rc.right = cxMax;
            // Now recalculate the number of rows
            i2 = rc.right / cxSpacing;
        }

        if (!i2)
            i2 = 1; // Don't divide by zero below...
        // The height.
        cy = (cItems + i2 - 1) / i2;
        rc.bottom = (cy * cySpacing);
    }

    // Make sure we are in the "default" view
    if ((dwStyle & LVS_TYPEMASK) == LVS_ICON)
    {
        // If it's going to be too big then flip into listview.
        pdsv->_fs.ViewMode = (rc.bottom > (cyMax * 3)) ? FVM_LIST : FVM_ICON;

        // Give Briefcase a chance to tell us it likes details
        pdsv->CallCB(SFVM_DEFVIEWMODE, 0, (LPARAM)&pdsv->_fs.ViewMode);

        SetWindowBits(pdsv->_hwndListview, GWL_STYLE, LVS_TYPEMASK, LVStyleFromView(pdsv));

        if (pdsv->_fs.ViewMode == FVM_DETAILS)
        {
            if (!pdsv->_bLoadedColumns)
            {
                pdsv->AddColumns();
            }
            // Need to special-case details because it could be any width
            // I need to add an item in case there were none to start with

            LV_ITEM item;
            int i;

            item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            item.iItem = INT_MAX;     // add at end
            item.iSubItem = 0;

            item.iImage = 0;
            item.pszText = (LPTSTR)c_szNULL;
            item.lParam = 0;

            i = ListView_InsertItem(pdsv->_hwndListview, &item);

            if (i >= 0)
            {
                ListView_GetItemRect(pdsv->_hwndListview, i, &rc, LVIR_BOUNDS);
                ListView_DeleteItem(pdsv->_hwndListview, i);
            }
        }
    }

    // Make sure things aren't too small, or too big
    rc.left = rc.top = 0;
    rc.right  = max(min(rc.right, cxMax), cxMin);
    rc.bottom = max(min(rc.bottom, cyMax), cyMin);

    // 99/02/09 #286723 vtan: Requested by gsierra to change best fit
    // algorithm to use the 3/5 or 4/5 algorithm if browse in same
    // or webview on.

    // 99/03/18 #307124 vtan: Add IsExplorerWindow() so that initial
    // explorer will be sized so that the list view is visible.

    if (IsSingleWindowBrowsing() || pdsv->m_cFrame.IsWebView() || IsExplorerWindow(pdsv->_hwndMain))
    {
        // Extended views or normal view with the web pane on just
        // get most of the screen space
        rc.right = cxMax;
        rc.bottom = cyMax;
    }

    // Allow room for the parent, toolbars, deskbands, etc.
    RECT rcParent, rcListview;
    GetWindowRect(pdsv->_hwndMain, &rcParent);
    GetWindowRect(pdsv->_hwndListview, &rcListview);
    int nGapHeight= (rcParent.bottom - rcParent.top) - (rcListview.bottom - rcListview.top);
    int nGapWidth = (rcParent.right - rcParent.left) - (rcListview.right - rcListview.left);
    // Also allow room for some text.
    InflateRect(&rc, nGapWidth + PARENTGAPWIDTH, nGapHeight + PARENTGAPHEIGHT + (cySpacing / 2));

    // we have to set the size of the listview now because
    // in nashvile, we're not the active shellview, we're the psvPending
    // so sizing hwndmain won't size us.
    //
    // BUGBUG: the above comment implies that if we're thumbnail view, it's size
    // will be wrong.  I don't believe that, so I suspect this code is not needed.
    // And besides, if we're WebView, this is the wrong size anyway!
    //
    SetWindowPos(pdsv->_hwndListview, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOMOVE | SWP_NOZORDER);

    // Make sure it will still fit on the screen.
    wp.length = SIZEOF(wp);
    GetWindowPlacement(pdsv->_hwndMain, &wp);

    // Make sure the window fits in the work area.
    cxScreen = cxWorkArea - GetSystemMetrics(SM_CXFRAME);
    cyScreen = cyWorkArea - GetSystemMetrics(SM_CYFRAME);
    if (wp.rcNormalPosition.left + rc.right > cxScreen)
        rc.right = cxScreen - wp.rcNormalPosition.left;
    if (wp.rcNormalPosition.top + rc.bottom > cyScreen)
        rc.bottom = cyScreen - wp.rcNormalPosition.top;

    // Resize the parent.
    SetWindowPos(pdsv->_hwndMain, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE | SWP_NOZORDER);

    // If we're still in icon mode but AutoArrange is off -
    // things will look weird so fix it now.
    //
    if (pdsv->_HasNormalView())
    {
        ASSERT(!pdsv->_bItemsMoved);   // no items should be positioned in this case
        pdsv->AutoAutoArrange(0);
    }
}

//----------------------------------------------------------------------------
BOOL CDefView::EnumerationTimeout(BOOL bRefresh)
{
    TraceMsg(TF_DEFVIEW, "Enumeration is taking too long.");

    // The static window could already exist during a refresh
    if (!_hwndStatic && bRefresh)
    {
        RECT rc;

        // Note that new windows go to the bottom of the Z order
        _hwndStatic = CreateWindowEx(WS_EX_CLIENTEDGE, ANIMATE_CLASS, c_szNULL,
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | ACS_TRANSPARENT | ACS_AUTOPLAY | ACS_CENTER,
                0, 0, 0, 0, _hwndView, (HMENU)ID_STATIC, HINST_THISDLL, NULL);
        if (!_hwndStatic)
        {
            // We need this window to exist so that we do not get strange
            // painting and clicking results
            return(FALSE);
        }

        GetClientRect(_hwndView, &rc);
        // Move this window to the top so the user sees the "looking" icon
        // We are in a "normal" view.  We need to do this always or the
        // Flashlight doesn't appear.  It tested safe with WebView on.
        SetWindowPos(_hwndStatic, HWND_TOP, 0, 0, rc.right, rc.bottom, 0);
        _OnMoveWindowToTop(_hwndStatic);

        // BUGBUG: _hwndListview is created hidden, this is probably not needed these days.
        ShowWindow(_hwndListview, SW_HIDE);
    }

    ViewWindow_BestFit(this, TRUE);

    return TRUE;
}


void DefView_CheckForFillDoneOnDestroy(HWND _hwndView)
{
    MSG msg;

    if (PeekMessage(&msg, _hwndView, WM_DSV_DESTROYSTATIC, WM_DSV_DESTROYSTATIC, PM_NOREMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == _hwndView)
        {
            TraceMsg(TF_DEFVIEW, "DefView: Fill_Completed after WM_DESTROY!!!");
            DPA_FreeIDArray((HDPA)msg.lParam);
        }
    }

    while (PeekMessage(&msg, _hwndView, WM_DSV_UPDATEICON, WM_DSV_UPDATEICON, PM_REMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == _hwndView)
        {
            TraceMsg(TF_DEFVIEW, "DefView: WM_DSV_UPDATEICON after WM_DESTROY!!!");
            CDVGetIconTask * pTask = (CDVGetIconTask *) msg.lParam;
            ATOMICRELEASE(pTask);
        }
    }

    while (PeekMessage(&msg, _hwndView, WM_DSV_UPDATECOLDATA, WM_DSV_UPDATECOLDATA, PM_REMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == _hwndView)
        {
            TraceMsg(TF_DEFVIEW, "DefView: WM_DSV_UPDATECOLDATA after WM_DESTROY!!!");
            delete (CBackgroundColInfo*)msg.lParam;
        }
    }
}

void CDefView::_ShowListviewIcons()
{
    // NOTE: this is where most of the flicker bugs come from -- showing the
    // listview too early.  This is touchy code, so be careful when you change it.
    // And plese document all changes for future generations.  Thanks.
    //
    // If our view hasn't been UIActivate()d yet, then we are waiting until
    // the IShellBrowser selects us as the active view.
    //
    if (_uState != SVUIA_DEACTIVATE)
    {
        // Gotta show only under the correct conditions:
        //
        ///////
        if ( // We're supposed to show icons
             ((!m_cFrame.IsWebView() || _IsDesktop()) && DV_SHOWICONS(this))
             // and ISFV extensions manage their own icon display
             && !m_cFrame.IsSFVExtension())
        {
            // Bring this to the top while showing it to avoid a second paint when
            // _hwndStatic is destroyed (listview has optimizations when hidden,
            // and it will repaint when once shown even if though it may be obscured)
            //
            SetWindowPos(_hwndListview, HWND_TOP, 0, 0, 0, 0,
                SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
            _OnMoveWindowToTop(_hwndListview);
        }
    }
    else
    {
        // Since all _ShowListviewIcons calls may be done before we go UIActive,
        // we need to keep track of this show and do it later.
        //
        _fShowListviewIconsOnActivate = TRUE;
    }
}

void CDefView::FillDone(HDPA hdpaNew,
        PDVSAVEHEADER pSaveHeader, UINT uLen, BOOL bRefresh, BOOL fInteractive)
{
    HDPA hdpaOld;
    int i, j;

    SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)FALSE, 0);
    //
    // Make it sure that we have this around while in this function.
    //
    this->AddRef();

    hdpaOld = DPA_Create(16);
    if (!hdpaOld)
    {
        goto Error1;
    }

    // now get the dpa of what's currently being viewed.
    i = ListView_GetItemCount(_hwndListview);

    for (j = 0; j < i ; j++)
    {
        LPITEMIDLIST pidl = _GetPIDL(j);
        if (pidl)
        {
            ASSERT(IsValidPIDL(pidl));
            DPA_AppendPtr(hdpaOld, pidl);
        }
    }

    // sort the two for easy comparisions
    // Note the new list should already be sorted
    // I wish we could do this sort in the background, but we just can't
    DPA_Sort(hdpaOld, _GetCompareFunction(), (LPARAM)this);

    _FilterDPAs(hdpaNew, hdpaOld);

    DPA_Destroy(hdpaOld);

Error1:;
    DPA_Destroy(hdpaNew);

    if (bRefresh)
    {
        TIMESTART(_RestoreState);
        _RestoreState(pSaveHeader, uLen);
        TIMESTOP(_RestoreState);
    }

    _ShowListviewIcons();

    if (_hwndStatic)
    {
        DestroyWindow(_hwndStatic);
        _hwndStatic = NULL;

        //
        //  Listview quirk:  If the following conditions are met..
        //
        //      1. WM_SETREDRAW(FALSE) or ShowWindow(SW_HIDE)
        //      2. Listview has never painted yet
        //      3. LVS_LIST
        //
        //  then ListView_LGetRects doesn't work.  And consequently,
        //  everything that relies on known item rectangles (e.g.,
        //  LVM_ENSUREVISIBLE) doesn't work.
        //
        //  (1) _ShowListviewIcons does a ShowWindow(SW_SHOW), but
        //  at the top of this function, we did a WM_SETREDRAW(FALSE),
        //  so condition (1) is met.
        //
        //  (2) Condition (2) is met because this function is called
        //  precisely to prepare the listview for its first paint.
        //
        //  But wait, there's also a listview bug where SetWindowPos
        //  doesn't trigger it into thinking that the window is visible.
        //  So you have to send a manual WM_SHOWWINDOW, too.
        //
        //  So if we detect that condition (3) is also met, we temporarily
        //  enable redraw (thereby cancelling condition 1), tell listview
        //  "No really, you're visible" -- this tickles it into computing
        //  column stuff -- then turn redraw back off.
        //

        if (_fItemsDeferred() &&
            (GetWindowStyle(_hwndListview) & LVS_TYPEMASK) == LVS_LIST)
        {
            // Evil hack (fix comctl32.dll v6.0 someday)
            SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)TRUE, 0);
            SendMessage(_hwndListview, WM_SHOWWINDOW, TRUE, 0);
            SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)FALSE, 0);
        }

        // End of listview hack workaround

        this->SelectSelectedItems();

    }

    // set the focus on the first item.
    _FocusOnSomething();

    // Tell the defview client that this window has been refreshed
    CallCB(SFVM_REFRESH, FALSE, 0);

    DV_UpdateStatusBar(this, TRUE);

    if (_bUpdatePending)
    {
        this->FillObjectsShowHide(FALSE, NULL, 0, fInteractive);
    }
    //
    // Decrement the reference count
    //
    this->Release();
    SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)TRUE, 0);

#ifdef TIMING
    dwFinish = GetTickCount();
    {
        TCHAR sz[40];
        wsprintf(sz, TEXT("%d\n"), dwFinish - dwStart);
        OutputDebugString(sz);
    }

    TIMESTOP(_Fill);
    TIMEOUT(_Fill);
    TIMEOUT(_EnumNext);
    TIMEOUT(_AddObject);
    TIMEOUT(_GetIcon);
    TIMEOUT(_GetName);
    TIMEOUT(_RestoreState);

    TIMEOUT(_FSNotify);
    TIMEOUT(this->-WMNotify);
    TIMEOUT(_LVChanging);
    TIMEOUT(_LVChanged);
    TIMEOUT(_LVDelete);
    TIMEOUT(_LVGetDispInfo);

    TraceMsg(TF_DEFVIEW, "::FillObjects(%s) done! **********************", DV_Name(this));

#endif

}


void ChangeRefForIdle(CDefView *pdsv, BOOL bAdd)
{
    if (bAdd)
    {
        if (InterlockedIncrement(&pdsv->_cRefForIdle) == 0)
        {
            pdsv->AddRef();
        }
    }
    else
    {
#ifdef DEBUG
ENTERCRITICAL;
        if (pdsv->_cRefForIdle < 0)
        {
            TraceMsg(TF_ERROR, "ChangeRefForIdle released too many times!");
            ASSERT(0);
        }
LEAVECRITICAL;
#endif
        InterlockedDecrement(&pdsv->_cRefForIdle);

        // win95 doesn't return the value from the above call...
        if (pdsv->_cRefForIdle == -1)
        {
            pdsv->Release();
        }
    }
}


//
// Returns: TRUE, if we successfully create the idle thread.
//
// On success, *phdpaNew will be NULL.  On failure it may or may not be NULL.
BOOL DV_StartIdle(CDefView *pdsv, IEnumIDList *peunk, HDPA *phdpaNew, BOOL bRefresh)
{
    if (pdsv->EnumerationTimeout(bRefresh))
    {
        IRunnableTask * pTask;

        HRESULT hr = CDVBkgrndEnumTask_CreateInstance( pdsv, peunk, *phdpaNew, bRefresh, & pTask );
        if ( SUCCEEDED( hr ))
        {
            if ( pdsv->_bBkFilling )
            {
                ASSERT( pdsv->_pScheduler );

                // make sure there are no other enum tasks going on...
                hr = pdsv->_pScheduler->RemoveTasks( TOID_DVBackgroundEnum,
                                                     ITSAT_DEFAULT_LPARAM,
                                                     TRUE );
            }

            if (!DefView_IdleDoStuff(pdsv, pTask, TOID_DVBackgroundEnum, 0,
                                             TASK_PRIORITY_BKGRND_FILL))
            {
                pdsv->_bBkFilling = FALSE;
                hr = E_FAIL;
            }

            ATOMICRELEASE(pTask);
            *phdpaNew = NULL;     // This has been freed by pTask->Release()
        }

        if ( SUCCEEDED( hr ))
        {
            pdsv->_bBkFilling = TRUE;
            SetTimer(pdsv->_hwndView, DV_IDTIMER, ENUMTIMEOUT, NULL);
        }
    }

    return pdsv->_bBkFilling;
}

HRESULT CDefView::EmptyBkgrndThread( BOOL fTerminate )
{
    HRESULT hr = S_FALSE;

    if ( _pScheduler )
    {
        if ( fTerminate )
        {
            // set the thread DIE timeout to 60 seconds (previously 4).
            // The thread might be stuck in the middle of an RPC call,
            // so our timeout needs to be at least as long as the network
            // timeout.  (An RPC call?  That's right, because CDrives needs
            // to call WNetGetWkstaInfo, which uses RPC.)
            //
            // Spinning up a drive can take a long time, too.  Screw it.
            // We now just wait forever.
            //
//          _pScheduler->Status( ITSSFLAG_THREAD_TERMINATE_TIMEOUT, 60*1000 );

            // this blocks until the scheduler is dead.
            ATOMICRELEASE( _pScheduler );

// CDTURNER
// as we don't terminate the thread anymore, changing the RefForIdle count would causes
// us to have an incorrect count and fault (there may be some objects left in the message
// queue waiting to be retrieved......

//            if (_cRefForIdle != -1)
//            {
//                ASSERT(0); // this is a wierd case that we
                           // should check out when it happens.
                // Note that we will err on the side of not
                // releasing enough rather than too often,
                // which should never happen.
                // There is only a tiny window when the forgotme
                // flag is not set and the release has happened,
                // so the chance of a memory leak is nearly 0.
//                _cRefForIdle = 0;
//                ChangeRefForIdle(this, FALSE);
//            }

            hr = NOERROR;
        }
        else
        {
            // tell it to wait 10 seconds while emptying....
//            _pScheduler->Status( ITSSFLAG_THREAD_TERMINATE_TIMEOUT, 10*1000 );

            if ( _bEmptyingScheduler )
            {
                // we are already in the removetasks call and got caught with an interthread
                // sendmessage...
                return S_FALSE;
            }
            _bEmptyingScheduler = TRUE;
            
            // empty the queue and wait until it is empty.....
            hr = _pScheduler->RemoveTasks( TOID_NULL, ITSAT_DEFAULT_LPARAM, TRUE );
            
            _bEmptyingScheduler = FALSE;

            // NOTE: as we are not terminating this thread, we will
            // NOTE: assume that anything left in the queue after 10 seconds is
            // NOTE: pretty harmless...
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// Enumeration loop for the main GUI thread.
//
HRESULT CDefView::FillObjects(BOOL bRefresh, PDVSAVEHEADER pSaveHeader, UINT uLen, BOOL fInteractive)
{
    IEnumIDList *peunk;
    LPITEMIDLIST pidl;
    ULONG celt;
    DWORD dwTime;
    BOOL fTimeOut;
    HRESULT hres;
    DWORD dwEnumFlags;
    DWORD dwTimeout = _fs.fFlags & FWF_BESTFITWINDOW ? ENUMTIMEOUT : SHORTENUMTIMEOUT;
    HDPA hdpaNew;
    DECLAREWAITCURSOR;

#ifdef TIMING
    dwStart = GetTickCount();
#endif

    _fFileListEnumDone = FALSE;
    
    // This is a potentially long operation
    SetWaitCursor();

    // make sure we are not doing anything in the background anymore...
    EmptyBkgrndThread( FALSE );

    // We should not save the selection if doing refresh.
    _ClearSelectList();

    if (bRefresh)
    {
        DefView_UpdateGlobalFlags(this);
        _RemoveObject(NULL, FALSE);
    }

    // Setup the enum flags.
    dwEnumFlags = SHCONTF_NONFOLDERS;
    if (_fShowAllObjects)
    {
        dwEnumFlags |= SHCONTF_INCLUDEHIDDEN ;
    }

    //Is this View in Common Dialog
    if (DV_CDB_IsCommonDialog(this) && !(dwEnumFlags & SHCONTF_INCLUDEHIDDEN))
    {
        //Ask Common dialog if its wants to show all files
        ICommDlgBrowser2 *pcdb2;
        if (SUCCEEDED(_psb->QueryInterface(IID_ICommDlgBrowser2, (void **)&pcdb2)))
        {
            DWORD dwFlags = 0;
            pcdb2->GetViewFlags(&dwFlags);
            if (dwFlags & CDB2GVF_SHOWALLFILES)
                dwEnumFlags |= SHCONTF_INCLUDEHIDDEN;
            pcdb2->Release();
        }
    }

    if (!(_fs.fFlags & FWF_NOSUBFOLDERS))
        dwEnumFlags |= SHCONTF_FOLDERS;

    peunk = NULL;   // in case callers dont do this
    hres = _pshf->EnumObjects(fInteractive ? _hwndMain : NULL, dwEnumFlags, &peunk);
    if (hres != S_OK)
    {
        // S_FALSE is success but empty (for doc-find)
        if (hres == S_FALSE)
        {
            CallCB(SFVM_REFRESH, FALSE, 0);
            _ShowListviewIcons();
        }
        if (peunk == NULL)
            goto Done;
    }

    IUnknown_SetSite(peunk, SAFECAST(this, IOleCommandTarget *));      // give enum a ref to us

    hdpaNew = DPA_Create(16);
    if (!hdpaNew)
        goto Error1;

    // 99/05/11 #301779 vtan: IShellFolder::EnumObjects() returns an
    // IEnumIDList for the shell folder. To do this it uses FindFirstFile
    // on Win9x and FindFirstFileEx on WinNT. Either way for a removable
    // disk at the root directory, the "." and ".." FindFirstFile results
    // do not exist and the HANDLE returned is invalid. The file system
    // IShellFolder accounts for this and returns S_FALSE meaning the
    // call didn't result in an error but really contains no valid
    // enumeration. In this case use an empty HDPA (created above) as the
    // PIDL list that's current and invoke CDefView::FillDone() on this.

    if ((hres == S_FALSE) && (peunk != NULL))
    {
        goto EmptyFillDone;
    }

    TIMEIN(_Fill);
    TIMEIN(_AddObject);
    TIMEIN(_GetIcon);
    TIMEIN(_GetName);
    TIMEIN(_FSNotify);
    TIMEIN(_EnumNext);
    TIMEIN(_RestoreState);

    TIMEIN(_WMNotify);
    TIMEIN(_LVChanging);
    TIMEIN(_LVChanged);
    TIMEIN(_LVDelete);
    TIMEIN(_LVGetDispInfo);

    TIMESTART(_Fill);
    TraceMsg(TF_DEFVIEW, "::FillObjects(%s) *****************************************", DV_Name(this));

    //
    //  If the callback returns S_OK to SFVM_BACKGROUNDENUM, start the idle
    // thread immediately.
    //
    if ( CallCB(SFVM_BACKGROUNDENUM, 0, 0) == S_OK )
    {
        TraceMsg(TF_DEFVIEW, "::FillObjects : Start idle thread immediately");
        if (DV_StartIdle(this, peunk, &hdpaNew, bRefresh)) {
            goto Done;
        }
        if (!hdpaNew)
            hdpaNew = DPA_Create(16);
        if (!hdpaNew)
            goto Error1;

        fTimeOut = TRUE;    // Failed to start, don't try it again.
    }
    else
    {
        fTimeOut = FALSE;
    }

    dwTime = GetTickCount();
    while (DV_Next(this, peunk, 1, &pidl, &celt) == S_OK)
    {
        if (DPA_AppendPtr(hdpaNew, pidl) == -1)
        {
            SHFree(pidl);
        }
        pidl = NULL;

        // Are we taking too long?
        if (!fTimeOut && ((GetTickCount() - dwTime) > dwTimeout))
        {
            fTimeOut = TRUE;

            if (!_IsDesktop())
            {
                if (DV_StartIdle(this, peunk, &hdpaNew, bRefresh)) {
                    goto Done;
                }
                if (!hdpaNew)
                    hdpaNew = DPA_Create(16);
                if (!hdpaNew)
                    goto Error1;
            }
        }
    }

    DPA_Sort(hdpaNew, _GetCompareFunction(), (LPARAM)this);

EmptyFillDone:
    this->FillDone(hdpaNew, pSaveHeader, uLen, bRefresh, fInteractive);

Error1:;
    IUnknown_SetSite(peunk, NULL);      // Break the site back pointer.
Done:
    ATOMICRELEASE(peunk);
    ResetWaitCursor();

    return hres;
}

HRESULT CDefView::FillObjectsShowHide(BOOL bRefresh,
        PDVSAVEHEADER pSaveHeader, UINT uLen, BOOL fInteractive)
{
    HRESULT hres;

    _bUpdatePending = FALSE;
    _bUpdatePendingPending = FALSE; // just in case we have a WM_TIMER message around
    hres = this->FillObjects(bRefresh, pSaveHeader, uLen, fInteractive);
    if (!_hwndListview) {
        return hres;
    }

#ifdef DEBUG
    // cache this for error reporting
    _hres = hres;
#endif

    if (SUCCEEDED(hres))
    {
        //
        // If previous enumeration failed, we need to make it visible.
        //
        if (_fEnumFailed)
        {
            _fEnumFailed = FALSE;
            _ShowListviewIcons();
        }
    }
    else
    {
        //
        // The fill objects failed for some reason, we should
        // display an appropriate error message.
        //
        if (!_fEnumFailed)
        {
            ShowWindow(_hwndListview, SW_HIDE);
        }

        TraceMsg(TF_WARNING, "::FillObjects failed (%x)", hres);

        _fEnumFailed = TRUE;
    }

    return hres;
}

void DefView_MoveSelectedItems(CDefView *pdsv, int dx, int dy, BOOL fAll)
{
    LVUtil_MoveSelectedItems(pdsv->_hwndListview, dx, dy, fAll);
    pdsv->_bItemsMoved = TRUE;
    pdsv->_bClearItemPos = FALSE;
}


TCHAR const c_szDelete[] = TEXT("delete");
TCHAR const c_szCut[] = TEXT("cut");
TCHAR const c_szCopy[] = TEXT("copy");
TCHAR const c_szLink[] = TEXT("link");
TCHAR const c_szProperties[] = TEXT("properties");
TCHAR const c_szPaste[] = TEXT("paste");
TCHAR const c_szPasteLink[] = TEXT("pastelink");
// char const c_szPasteSpecial[] = "pastespecial";
TCHAR const c_szRename[] = TEXT("rename");

//
// This function processes command from explorer menu (FCIDM_*)
//
// HACK ALERT:
//  This implementation uses following assumptions.
//  (1) The IShellFolder uses CDefFolderMenu.
//  (2) The CDefFolderMenu always add the folder at the top.
//

#define EC_SELECTION  0
#define EC_BACKGROUND 1
#define EC_EITHER     3

HRESULT DefView_ExplorerCommand(CDefView *pdsv, UINT idFCIDM)
{
    HRESULT hres = E_FAIL;

    //
    // s_idMap[i][0] = Defview menuitem ID
    // s_idMap[i][1] = FALSE, context menu; TRUE; background menu
    // s_idMap[i][2] = Folder menuitem ID
    //
    static struct {
        UINT    idmFC;
        UINT    f_Background;
        LPCTSTR pszVerb;
    } const c_idMap[] = {
        { SFVIDM_FILE_RENAME,      EC_SELECTION, c_szRename },
        { SFVIDM_FILE_DELETE,      EC_SELECTION, c_szDelete },
        { SFVIDM_FILE_PROPERTIES,  EC_EITHER, c_szProperties },
        { SFVIDM_EDIT_COPY,        EC_SELECTION, c_szCopy },
        { SFVIDM_EDIT_CUT,         EC_SELECTION, c_szCut },
        { SFVIDM_FILE_LINK,        EC_SELECTION, c_szLink },
        { SFVIDM_EDIT_PASTE,       EC_BACKGROUND,  c_szPaste },
        { SFVIDM_EDIT_PASTELINK,   EC_BACKGROUND,  c_szPasteLink },
        // { SFVIDM_EDIT_PASTESPECIAL,TRUE,  c_szPasteSpecial },
    };
    int i;

    for (i = 0; i < ARRAYSIZE(c_idMap); i++)
    {
        if (c_idMap[i].idmFC == idFCIDM)
        {
            IContextMenu *pcm = NULL;

            if (c_idMap[i].f_Background == EC_BACKGROUND)
            {
TryBackground:
                pdsv->_pshf->CreateViewObject(pdsv->_hwndMain,
                    IID_IContextMenu, (void **)&pcm);
            }
            else
            {
                DECLAREWAITCURSOR;
                SetWaitCursor();
                pcm = pdsv->_GetContextMenuFromSelection();
                ResetWaitCursor();

                if (!pcm && c_idMap[i].f_Background == EC_EITHER &&
                    !ListView_GetSelectedCount(pdsv->_hwndListview)) {
                    goto TryBackground;
                }
            }

            if (pcm)
            {
                CMINVOKECOMMANDINFOEX ici;

                ZeroMemory(&ici, SIZEOF(ici));
                ici.cbSize = SIZEOF(ici);
                ici.hwnd = pdsv->_hwndMain;
                ici.nShow = SW_NORMAL;

                // record if shift or control was being held down
                SetICIKeyModifiers(&ici.fMask);

                //
                //  We need to call QueryContextMenu() so that CDefFolderMenu
                // can initialize its dispatch table correctly.
                //
                HMENU hmenu = CreatePopupMenu();
#ifdef UNICODE
                // Fill in both the ansi verb and the unicode verb since we
                // don't know who is going to be processing this thing.
                CHAR szVerbAnsi[40];
                SHUnicodeToAnsi(c_idMap[i].pszVerb, szVerbAnsi, ARRAYSIZE(szVerbAnsi));
                ici.lpVerb = szVerbAnsi;
                ici.lpVerbW = c_idMap[i].pszVerb;
                ici.fMask |= CMIC_MASK_UNICODE;
#else
                ici.lpVerb = c_idMap[i].pszVerb;
#endif

                if (hmenu)
                {
                    IUnknown_SetSite(pcm, SAFECAST(pdsv, IOleCommandTarget *));
                    pcm->QueryContextMenu(hmenu, 0,
                                SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST, 0);
                    pdsv->_bContextMenuMode = TRUE;
                    hres = pdsv->_InvokeCommand(pcm, &ici);
                    pdsv->_bContextMenuMode = FALSE;
                    DestroyMenu(hmenu);
                    IUnknown_SetSite(pcm, NULL);
                }

                ATOMICRELEASE(pcm);
                if (pdsv->_pcmSel == pcm)
                {
                    DV_FlushCachedMenu(pdsv);
                }
            }
            else
            {
                //
                //  We should beep if when one of those object keys are
                // pressed when there is no selection.
                //
                MessageBeep(0);
            }

            break;
        }
    }

    ASSERT(i<ARRAYSIZE(c_idMap));

    return hres;
}


STDAPI_(BOOL) Def_IsPasteAvailable(IDropTarget *pdtgt, DWORD *pdwEffect);

BOOL DefView_AllowCommand(CDefView *pdsv, UINT uID, WPARAM wParam, LPARAM lParam)
{
    DWORD dwAttribsIn;
    DWORD dwEffect;

    switch (uID)
    {
    case SFVIDM_EDIT_PASTE:
        return Def_IsPasteAvailable(pdsv->_pdtgtBack, &dwEffect);

    case SFVIDM_EDIT_PASTELINK:
        Def_IsPasteAvailable(pdsv->_pdtgtBack, &dwEffect);
        return dwEffect & DROPEFFECT_LINK;

    case SFVIDM_EDIT_COPY:
        dwAttribsIn = SFGAO_CANCOPY;
        break;

    case SFVIDM_EDIT_CUT:
        dwAttribsIn = SFGAO_CANMOVE;
        break;

    case SFVIDM_FILE_DELETE:
        dwAttribsIn = SFGAO_CANDELETE;
        break;

    case SFVIDM_FILE_LINK:
        dwAttribsIn = SFGAO_CANLINK;
        break;

    case SFVIDM_FILE_PROPERTIES:
        dwAttribsIn = SFGAO_HASPROPSHEET;
        break;

    default:
        ASSERT(FALSE);
        return FALSE;
    }
    return (DefView_GetAttributesFromSelection(pdsv, dwAttribsIn) & dwAttribsIn);
}


// return copy of pidl of folder we're viewing
LPITEMIDLIST CDefView::_GetViewPidl()
{
    LPITEMIDLIST pidl;
    if (SHGetIDListFromUnk(_pshf, &pidl) != S_OK)    // S_FALSE is success by empty
    {
        if (SUCCEEDED(CallCB(SFVM_THISIDLIST, 0, (LPARAM)&pidl)))
        {
            ASSERT(pidl);
        }
        else if (_pidlMonitor)
        {
            pidl = ILClone(_pidlMonitor);
        }
    }
    return pidl;
}

BOOL CDefView::_IsViewDesktop()
{
    BOOL bDesktop = FALSE;
    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        bDesktop = ILIsEmpty(pidl);
        ILFree(pidl);
    }
    return bDesktop;
}

// access to the current views name ala IShellFolder::GetDisplayNameOf()

HRESULT CDefView::_GetNameAndFlags(UINT gdnFlags, LPTSTR pszPath, UINT cch, DWORD *pdwFlags)
{
    *pszPath = 0;

    HRESULT hr;
    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        hr = SHGetNameAndFlags(pidl, gdnFlags, pszPath, cch, pdwFlags);
        ILFree(pidl);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// returns TRUE if the current view is a file system folder, returns the path

BOOL CDefView::_GetPath(LPTSTR pszPath)
{
    *pszPath = 0;

    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        SHGetPathFromIDList(pidl, pszPath);
        ILFree(pidl);
    }
    return *pszPath != 0;
}


EXTERN_C TCHAR const c_szWindowsHlp[] = TEXT("windows.hlp");
EXTERN_C TCHAR const c_szHtmlWindowsHlp[] = TEXT("windows.chm");


#ifdef CUSTOM_BACKGROUND
// A helper function for grovelling the registry
//
BOOL GetRegThing(HKEY ahk[], int nhk, LPCTSTR pszValue, LPBYTE lpThing, DWORD dwThingType, DWORD cbBytes, void *lpDefault)
{
    int i;

    for (i=0 ; i<nhk ; i++)
    {
        DWORD dwType;

        ASSERT(NULL != ahk[i]);

        if (ERROR_SUCCESS ==
            SHQueryValueEx(ahk[i], pszValue, NULL, &dwType, lpThing, &cbBytes) &&
            dwType == dwThingType)
        {
            return TRUE;
        }
    }

    if (lpDefault)
    {
        switch (dwThingType)
        {
        case REG_SZ:
            StrCpyN((LPTSTR)lpThing, (LPTSTR)lpDefault, cbBytes);
            return TRUE;

        case REG_BINARY:
        case REG_DWORD:
            MoveMemory(lpThing, lpDefault, cbBytes);
            return TRUE;

        default:
            ASSERT(0);
            return FALSE;
        }
    }

    return FALSE;
}
#endif // CUSTOM_BACKGROUND


// web view background colors, click mode, etc have changed
//
void CDefView::_UpdateListviewColors(BOOL fClassic)
{
    LVBKIMAGE lvbki;

    _fClassic = fClassic;

    // some common stuff up front
    //
    ZeroMemory(&lvbki, SIZEOF(lvbki));

    //
    // First read the registry/desktop.ini
    //
    TCHAR szImage[INTERNET_MAX_URL_LENGTH];
    int i;

    szImage[0] = 0;
    for (i = 0; i < ARRAYSIZE(_crCustomColors); i++)
        _crCustomColors[i] = CLR_MYINVALID;


    
    if (!DV_CDB_IsCommonDialog(this) && !_IsDesktop())
    {
        // BUGBUG kenwic 052599 #340912 Background needs to change even in classic mode  FIXED kenwic 052599
        // get the background bitmap
        if (!m_cFrame._StringFromViewID(&VID_FolderState, szImage, ARRAYSIZE(szImage), ID_EXTVIEWICONAREAIMAGE))
        {
            m_cFrame._StringFromView(m_cFrame.m_uView, szImage, ARRAYSIZE(szImage), ID_EXTVIEWICONAREAIMAGE);
        }


        // Set up the listview image, if any
        if (szImage[0])
        {
            lvbki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
            lvbki.pszImage = szImage;
        }
    }

    // change the differing stuff
    //
    if (!_fClassic && !DV_CDB_IsCommonDialog(this) && !_IsDesktop())
    {
        // VID_FolderState is what the customize wizard uses to set background
        // image and text colors. These should override any view defined settings
        // so check it first.

        // if there was an image specified but no custom text background,
        // set to CLR_NONE so the listview text is transparent
        // get combined view custom colors
        for (i = 0; i < ARRAYSIZE(_crCustomColors); i++)
        {
            COLORREF cr;
            if (m_cFrame._ColorFromViewID(&VID_FolderState, &cr, i) || m_cFrame._ColorFromView(m_cFrame.m_uView, &cr, i))
            {
                _crCustomColors[i] = PALETTERGB(0, 0, 0) | cr;
            }
        }

        if (!ISVALIDCOLOR(_crCustomColors[CRID_CUSTOMTEXTBACKGROUND]) && szImage[0])
        {
            _crCustomColors[CRID_CUSTOMTEXTBACKGROUND] = CLR_NONE;
        }


#ifdef FLAT_LISTVIEW
        // Make the listview flat...
        SetWindowBits(_hwndListview, GWL_EXSTYLE, WS_EX_CLIENTEDGE, 0);
#endif

#ifdef FLAT_SCROLLBAR
        // ...and its scrollbar too.
        ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_FLATSB, LVS_EX_FLATSB);
        FlatSB_SetScrollProp(_hwndListview, WSB_PROP_HSTYLE, FSB_FLAT_MODE, TRUE);
        FlatSB_SetScrollProp(_hwndListview, WSB_PROP_VSTYLE, FSB_FLAT_MODE, TRUE);
#endif

    }
    else
    {
#ifdef FLAT_LISTVIEW
        // Make the listview 3d
        SetWindowBits(_hwndListview, GWL_EXSTYLE, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
#endif

#ifdef FLAT_SCROLLBAR
        // ...and its scrollbar too.
        LONG lExStyle = ListView_GetExtendedListViewStyle(_hwndListview);
        if (lExStyle & LVS_EX_FLATSB)
        {
            // NOTE: the below comment is no longer true
            // there's no way to remove the FLATSB style, so simulate normal mode
            FlatSB_SetScrollProp(_hwndListview, WSB_PROP_HSTYLE, FSB_REGULAR_MODE, TRUE);
            FlatSB_SetScrollProp(_hwndListview, WSB_PROP_VSTYLE, FSB_REGULAR_MODE, TRUE);
        }
#endif
    }

    // wrap up the common stuff
    //
    // if its a thumbvw, do the thumbvw thing, always do standard listvw thing.
    if (m_cFrame.IsSFVExtension())
    {
        IShellFolderView * pSFV = m_cFrame.GetExtendedISFV();

        if (pSFV)
        {
            HRESULT hres;
            IDefViewExtInit2 * pShellView = NULL;

            hres = pSFV->QueryInterface(IID_IDefViewExtInit2, (void **)&pShellView);
            if (SUCCEEDED(hres))
            {
                WCHAR wszImage[INTERNET_MAX_URL_LENGTH];
                SHTCharToUnicode(szImage, wszImage, ARRAYSIZE(wszImage));
                pShellView->SetViewWindowBkImage(wszImage); 
                ATOMICRELEASE(pShellView);
            }
        }

    }
    ListView_SetBkImage(_hwndListview, &lvbki);
    DSV_SetFolderColors(this);
    this->UpdateSelectionMode();
}

BOOL CDefView::_IsReportView()
{
    return !m_cFrame.IsSFVExtension() && ((LVStyleFromView(this) & LVS_TYPEMASK) == LVS_REPORT);
}

class   CCurrentSelectionTransfer
{
    public:
                        CCurrentSelectionTransfer (void);
                        ~CCurrentSelectionTransfer (void);

        HRESULT         GetSelection (IShellFolderView *pISFV);
        HRESULT         SetSelection (IShellFolderView *pISFV, IShellView *pISV);
    private:
        UINT            m_uiItemCount;
        LPITEMIDLIST    *m_PIDLs;
};

CCurrentSelectionTransfer::CCurrentSelectionTransfer (void) :
    m_uiItemCount(0),
    m_PIDLs(NULL)

{
}

CCurrentSelectionTransfer::~CCurrentSelectionTransfer (void)

{
    if (m_PIDLs != NULL)
    {
        UINT    i;

        for (i = 0; i < m_uiItemCount; ++i)
            ILFree(m_PIDLs[i]);
        SHFree(m_PIDLs);
        m_PIDLs = NULL;
        m_uiItemCount = 0;
    }
}

HRESULT CCurrentSelectionTransfer::GetSelection (IShellFolderView *pISFV)

{
    HRESULT         hres;
    LPCITEMIDLIST   *pidls;

    hres = pISFV->GetSelectedObjects(&pidls, &m_uiItemCount);
    if (SUCCEEDED(hres))
    {
        if (m_uiItemCount > 0)
        {
            m_PIDLs = reinterpret_cast<LPITEMIDLIST*>(SHAlloc(m_uiItemCount * sizeof(LPCITEMIDLIST)));
            if (m_PIDLs != NULL)
            {
                UINT    i;

                for (i = 0; i < m_uiItemCount; ++i)
                    m_PIDLs[i] = ILClone(pidls[i]);
            }
            else
            {

                // 99/08/16 vtan #386924: STRESS: m_uiItemCount can be
                // > 0 but m_PIDLs failed to get allocated. SetSelection()
                // will blindly dereference this assuming that count
                // > 0 means successfully allocated.

                m_uiItemCount = 0;
                hres = E_OUTOFMEMORY;
            }
        }
        if (pidls != NULL)
            LocalFree(pidls);
    }
    return(hres);
}

HRESULT CCurrentSelectionTransfer::SetSelection (IShellFolderView *pISFV, IShellView *pISV)

{
    HRESULT     hres = S_OK;

    if ((m_PIDLs != NULL) && (m_uiItemCount > 0))
    {
        UINT    i, uiFlags;

        THR(pISFV->SetRedraw(FALSE));
        uiFlags = SVSI_SELECT | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE | SVSI_FOCUSED;
        for (i = 0; SUCCEEDED(hres) && (i < m_uiItemCount); ++i)
        {
            LPCITEMIDLIST   pidl;

            pidl = m_PIDLs[i];
            if (pidl != NULL)
            {
                hres = pISV->SelectItem(pidl, uiFlags);
            }
            uiFlags &= ~(SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE | SVSI_FOCUSED);
        }
        THR(pISFV->SetRedraw(TRUE));
    }
    return(hres);
}

BOOL CDefView::HasCurrentViewWindowFocus()
{
    BOOL fRet = false;
    HWND hwndCurrentFocus = GetFocus();
    if (hwndCurrentFocus)
    {
        HWND hwndView;
        if (m_cFrame.IsSFVExtension())
        {
            hwndView = m_cFrame.GetExtendedViewWindow();
        }
        else
        {
            hwndView = _hwndListview;
        }
        fRet = (SHIsChildOrSelf(hwndView, hwndCurrentFocus) == S_OK);
    }
    return fRet;
}

HWND CDefView::ViewWindowSetFocus()
{
    HWND hwndView;
    if (m_cFrame.IsSFVExtension())
    {
        hwndView = m_cFrame.GetExtendedViewWindow();
    }
    else
    {
        hwndView = _hwndListview;
    }
    SetFocus(hwndView);
    if(!_IsDesktop())
    {
        m_cFrame.m_uState = SVUIA_ACTIVATE_FOCUS;
    }
    return hwndView;
}

// we are switching from the current view to extended view uID
// =>switch to webview type view from webview typ view or non webview type view.
HRESULT CDefView::_SwitchToViewIDPVID(UINT uID, SHELLVIEWID const *pvid, BOOL bForce)
{
    HRESULT hres;

    UINT fvmOld;
    DWORD dwStyleOld;
    SHELLSTATE ss;
    BOOL  fCombinedViewOld = (BOOL)_fCombinedView;
    BOOL bSetFocusRequired = HasCurrentViewWindowFocus();

    CCurrentSelectionTransfer   selectionTransfer;

    selectionTransfer.GetSelection(this);

    // VID_FolderState is what's used to specify background images
    // on the listview -- it's not a real view.
    if (pvid && IsEqualIID(*pvid, VID_FolderState))
    {
        ASSERT(!IsEqualIID(*pvid, VID_FolderState));
        return E_FAIL;
    }

    // we shouln't be coming here if for details
    ASSERT(_bLoadedColumns || !(pvid && IsEqualIID(*pvid, VID_Details)));

    // remember current view just in case the new view fails
    //
    fvmOld = _fs.ViewMode;
    dwStyleOld = GetWindowStyle(_hwndListview) & LVS_TYPEMASK;

    // For now, the desktop is always a combined view...
    if (_IsDesktop()) {
        SHGetSetSettings(&ss, SSF_HIDEICONS | SSF_DESKTOPHTML, FALSE);
        // Does the user want desktop in HyperText view?
        if (ss.fDesktopHTML)
            _fCombinedView = TRUE;
        if (ss.fHideIcons)
            _fs.fFlags |= FWF_NOICONS;
        else
            _fs.fFlags &= ~FWF_NOICONS;
    }

    if (_fCombinedView && !fCombinedViewOld)
    {
        EnableCombinedView(this, TRUE);
        ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_REGIONAL, LVS_EX_REGIONAL);
        DSV_SetFolderColors(this);
    }

    // We don't need to OnDeactivate();OnActivate() to
    // remerge the menus because that is really dealing
    // with focus changes

    // Show the extended view
    if (pvid)
    {
        hres = m_cFrame.ShowExtView(pvid, bForce);
    }
    else
    {
        hres = m_cFrame.ShowExtView(uID, bForce);
    }

    if (SUCCEEDED(hres))
    {
        RECT rcClient;

        // Make sure the new view is the correct size
        GetClientRect(_hwndView, &rcClient);
        m_cFrame.SetRect(&rcClient);

        if (_fCombinedView && DV_SHOWICONS(this)) 
        {
            _ShowListviewIcons();
        } 
        else 
        {
            ShowWindow(_hwndListview, SW_HIDE);

            // We want to process FSNotify messages to keep the
            // listview up to date even though it may not be
            // shown for a while -- there may be a DefViewOC
            // on this page which will grab the listview and use it.
            //
            _bUpdatePendingPending = TRUE;
            SetTimer(_hwndView, DV_IDTIMER_UPDATEPENDING, UPDATEPENDINGTIME, NULL);
        }
    }
    else
    {
        // If the previous view was an extended view, the above failed
        // ShowExtView just nuked the view and nothing is showing.
        // Go back to a listview view.
        //
        TraceMsg(TF_DEFVIEW, "_SwitchToViewIDPVID failed ShowExtView");
        _SwitchToViewFVM(fvmOld, dwStyleOld, TRUE);
    }

    selectionTransfer.SetSelection(this, this);
    // If we had focus, make sure we continue to have the focus, especially if we switched to extended views
    if (bSetFocusRequired)
    {
        CallCB(SFVM_SETFOCUS, 0, 0);
        ViewWindowSetFocus();
    }

    // if we're switching from details to thumbnail view we'll have Choose Column item
    // in the view menu that we don't want -- so remove it
    if (_hmenuCur && !_IsReportView())
        DeleteMenu(_hmenuCur, SFVIDM_VIEW_COLSETTINGS, MF_BYCOMMAND);

    CheckToolbar();
#if 0
    // update menus, i.e. add Choose Columns to the view menu if Details view is selected
    // or remove it otherwise
    UINT uState = _uState;
    _SetUpMenus(uState); // calls OnDeactivate which sets _uState to SVUIA_DEACTIVATE
    _uState = uState;
#endif
    _EnableDisableTBButtons();    
    
    // make sure that the listview settings get refreshed anyway (back image)
    _UpdateListviewColors(_fClassic);

    return hres;
}

// we are switching from the current view to a normal listview view
// => in wv or nonwv mode
HRESULT CDefView::_SwitchToViewFVM(UINT fvmNew, DWORD dwStyle, BOOL fForce)
{
    HWND                        hwndCurrentFocus;
    BOOL                        bSetFocusRequired, bHasNormalView;
    CCurrentSelectionTransfer   selectionTransfer;

    BOOL fPrevViewExtended = m_cFrame.IsWebView();

    ASSERT(_hwndListview);

    hwndCurrentFocus = GetFocus();
    bSetFocusRequired = HasCurrentViewWindowFocus();
    selectionTransfer.GetSelection(this);
    bHasNormalView = _HasNormalView();

    // if we haven't loaded the columns yet, do that now
    if (fvmNew == FVM_DETAILS && !_bLoadedColumns)
    {
        // It's okay if (NULL == _pSaveHeader) because that case will be handled
        // in CColumnPointer::CColumnPointer(NULL, ...), which is called in AddColumns().
        this->AddColumns();
        _SetSortArrows();
    }

    if (fPrevViewExtended)     // webview mode?
    {
        // If the DefViewOC is up, then the listview selection is correct,
        // so we don't need to clear the selection state.
        if (!_fGetWindowLV)
        {
            // I don't want to deal with finding the current selection
            // of the extended view, so deselect everything after
            // switching away from one. This is good enough.
            ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);
        }

    }

    // If we were combined, then get the listview out of region mode and
    // reset the color scheme.  Also, turn off the combined bit.
    if (_fCombinedView) {
        _fCombinedView = FALSE;
        ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_REGIONAL, 0);
        DSV_SetFolderColors(this);
    }

    // extended shell view (thumbview) hosted in OC and not killing the OC,
    // so must be swapping to listview with currently active OC.  swap and kill
    // extended shell view.
    if (m_cFrame.IsSFVExtension())
    {
        RECT rcClient;

        if (m_cFrame.IsWebView() && !fForce && _pocWinMan) // oc hosting remains but swap holsted view??
        {
            _pocWinMan->SwapWindow(_hwndListview, &_pocWinMan);
            _fGetWindowLV = TRUE;
        }
        // kill TV either if is extended view  ad not killing it, or not extended view.
        if (!m_cFrame.IsWebView() || !fForce)
        {
            m_cFrame.KillActiveSFV();
            // Make sure the new view is the correct size
            GetClientRect(_hwndView, &rcClient);
            m_cFrame.SetRect(&rcClient);
        }
    }
    // change the listview's view before we hide the extended view
    // on sfv, let wfv handle this.
    DefView_SetViewMode(this, fvmNew, dwStyle);
    if (bHasNormalView)
    {
        ShowWindow(_hwndListview, SW_SHOW);
    }
    if (fForce)
    {
        // now remove the extended view
        if (m_cFrame.IsSFVExtension())
            m_cFrame.SetViewWindowStyle(WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
        m_cFrame.ShowExtView(CSFVFrame::HIDEEXTVIEW, FALSE);
        HWND hwndXV = m_cFrame.GetExtendedViewWindow();
        if (hwndXV)
            SetWindowPos(hwndXV, NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW | SWP_NOZORDER);
    }
    // make sure we're showing valid content
    if (_bUpdatePending)
        _ReloadListviewContent();

    AutoAutoArrange(0);

    selectionTransfer.SetSelection(this, this);

    if (bSetFocusRequired)
    {   // _hwndListview is the current view window. Let's set focus to it.
        CallCB(SFVM_SETFOCUS, 0, 0);
        ViewWindowSetFocus();
    }
    else
    {
        SetFocus(hwndCurrentFocus);
    }

    CheckToolbar();
    // update menus, i.e. add Choose Columns to the view menu if Details view is selected
    // or remove it otherwise
    UINT uState = _uState;
    _SetUpMenus(uState); // calls OnDeactivate which sets _uState to SVUIA_DEACTIVATE
    _uState = uState;

    _EnableDisableTBButtons();

    return S_OK;
}


int CDefView::CheckCurrentViewMenuItem(HMENU hmenu)
{
    int iCurViewMenuItem = m_cFrame.CurExtViewId();

    // If an extended view is showing our listview, then make the extended
    // view selection range a normal CHECKBOX that can be turned off and
    // make the large/small/list/details a selectable mode.
    //
    CheckMenuRadioItem(hmenu, SFVIDM_VIEW_EXTENDEDFIRST, SFVIDM_VIEW_EXTENDEDLAST,
        iCurViewMenuItem, MF_BYCOMMAND | MF_CHECKED);
    if (m_cFrame.IsWebView())
    {
        MENUITEMINFO mii;
        TCHAR szMenuName[50];

        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_TYPE;
        mii.dwTypeData = szMenuName;
        mii.cch = ARRAYSIZE(szMenuName);
        if (GetMenuItemInfo(hmenu, iCurViewMenuItem, MF_BYCOMMAND, &mii))
        {
            mii.fType &= ~MFT_RADIOCHECK;
            SetMenuItemInfo(hmenu, iCurViewMenuItem, MF_BYCOMMAND, &mii);
        }
    }
    iCurViewMenuItem = (m_cFrame.m_uView == CSFVFrame::NOEXTVIEW)
            ? _DSV_GetMenuIDFromViewMode(_fs.ViewMode)
            : m_cFrame.CmdIdFromUid(m_cFrame.m_uView);

    CheckMenuRadioItem(hmenu, SFVIDM_VIEW_FIRSTVIEW
        , SFVIDM_VIEW_SVEXTLAST, iCurViewMenuItem, MF_BYCOMMAND | MF_CHECKED);


    return iCurViewMenuItem;
}

int CDefView::_CheckIfCustomizable()
{
    int iCustomizable;
    TCHAR szPath[MAX_PATH];
    if (!SHRestricted(REST_NOCUSTOMIZEWEBVIEW) && _GetPath(szPath))
    {
        PathAppend(szPath, TEXT("Desktop.ini"));

        DWORD dwAttributes;
        BOOL bFileExistsAlready = PathFileExistsAndAttributes(szPath, &dwAttributes);
        if (bFileExistsAlready)
        {
            //If it is a UNC path, then the attributes will be -1.
            if (dwAttributes == -1)
                iCustomizable = DONTKNOW_IF_CUSTOMIZABLE;
            else
            {
                //It is a local path; We know the attributes for sure!
                if (dwAttributes & FILE_ATTRIBUTE_READONLY)
                   iCustomizable = NOT_CUSTOMIZABLE;
                else
                   iCustomizable = YES_CUSTOMIZABLE;
            }
        }
        else
            iCustomizable = DONTKNOW_IF_CUSTOMIZABLE;

        //If we still don't know...
        if (iCustomizable == DONTKNOW_IF_CUSTOMIZABLE)
        {
            // The file desktop.ini doesn't exist or it exists in a UNC path.
            // So, try the hard method to see if the media is writeable.

            //CreateFile is a costly process that is why we do it only rarely.
            HANDLE hFile = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                   (bFileExistsAlready ? OPEN_EXISTING : CREATE_NEW),
                                   FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                iCustomizable = YES_CUSTOMIZABLE;
                CloseHandle(hFile);

                //Delete only if it didn't exist already
                if (!bFileExistsAlready)
                    DeleteFile(szPath);    //Delete the file that we just created.
            }
            else
                iCustomizable = NOT_CUSTOMIZABLE;
        }
    }
    else
        iCustomizable = NOT_CUSTOMIZABLE;

    return iCustomizable;
}

BOOL CDefView::_InvokeCustomWizard()
{
    BOOL fRet = FALSE;
    TCHAR szPath[MAX_PATH];  // To hold Current directory path

    //Check if we already know if this folder is customizable!
    if ((m_iCustomizable == DONTKNOW_IF_CUSTOMIZABLE) ||
        (m_iCustomizable == MAYBE_CUSTOMIZABLE))
    {
        //Ok! This is a filesystem folder. See if it is customizable
        //and remember that in this view.
        m_iCustomizable = _CheckIfCustomizable();
    }

    if (m_iCustomizable == NOT_CUSTOMIZABLE)
    {
        //If not customizable, put up this error message!
        ShellMessageBox(HINST_THISDLL, _hwndMain, MAKEINTRESOURCE(IDS_NOTCUSTOMIZABLE), NULL,
                       MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
        return FALSE;  // ...and bail out!
    }

    //Save the view state first.
    SaveViewState();

    //BUGBUG! We may need to read this name from the registry. Check with ChristoB!
    if (_GetPath(szPath))
    {
        SHELLEXECUTEINFO ExecInfo = {0};
        TCHAR szParam[MAX_PATH + 20];   // To hold "<dirname> <(int)Folder HWND>"
        // The Command line is of the form <dirname> <(int)Folder HWND>

#ifdef _WIN64
        wnsprintf(szParam, sizeof(szParam), TEXT("%s;%d"), szPath, (INT_PTR)_hwndMain);
#else
        wnsprintf(szParam, sizeof(szParam), TEXT("%s;%d"), szPath, (int)_hwndMain);
#endif    

        //We need to set SEE_MASK_NOCLOSEPROCESS to getback the process handle.
        //FillExecInfo(ExecInfo, _hwndMain, NULL, TEXT("IESHWIZ.EXE"), szParam, NULL, SW_SHOWNORMAL);

        ExecInfo.hwnd            = _hwndMain;
        ExecInfo.lpVerb          = NULL;
        ExecInfo.lpFile          = TEXT("IESHWIZ.EXE");
        ExecInfo.lpParameters    = szParam;
        ExecInfo.lpDirectory     = szPath;
        ExecInfo.nShow           = SW_SHOWNORMAL;
        ExecInfo.fMask           = SEE_MASK_NOCLOSEPROCESS;
        ExecInfo.cbSize          = SIZEOF(SHELLEXECUTEINFO);

        //The wizard makes _hwndMain the owner of the property sheet window. So, we don't have to disable
        // this here.

        fRet = ShellExecuteEx(&ExecInfo);

        BOOL  fNeedToRefresh = FALSE;

        //Let's wait for the wizard to be terminated.
        while(fRet)
        {
            DWORD dwRet = MsgWaitForMultipleObjects(1, &ExecInfo.hProcess, FALSE, INFINITE, QS_ALLINPUT);

            // MsgWaitForMultipleObjects can fail with -1 being returned!
            if ((dwRet == WAIT_OBJECT_0) || (dwRet == -1))
            {
                fNeedToRefresh = TRUE;
                break;
            }
            else
            {
                MSG msg;

                //Get and process the paint messages!
                while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    }
    return fRet;
}

struct {
    UINT uiSfvidm;
    DWORD dwOlecmdid;
} const c_CmdTable[] = {
    { SFVIDM_EDIT_CUT,          OLECMDID_CUT        },
    { SFVIDM_EDIT_COPY,         OLECMDID_COPY       },
    { SFVIDM_EDIT_PASTE,        OLECMDID_PASTE      },
    { SFVIDM_FILE_DELETE,       OLECMDID_DELETE     },
    { SFVIDM_FILE_PROPERTIES,   OLECMDID_PROPERTIES },
};

DWORD OlecmdidFromSfvidm(UINT uiSfvidm)
{
    DWORD dwOlecmdid = 0;

    for (int i=0; i<ARRAYSIZE(c_CmdTable); i++)
    {
        if (c_CmdTable[i].uiSfvidm == uiSfvidm)
        {
            dwOlecmdid = c_CmdTable[i].dwOlecmdid;
            break;
        }
    }

    return dwOlecmdid;
}

LRESULT CDefView::SwitchToHyperText(UINT uID, BOOL fForce)
{
    LRESULT lRes = 0;

    // if current togglable extended view on, turn it off.
    if (!fForce && (uID == (UINT)m_cFrame.CurExtViewId()))
        lRes = SUCCEEDED(_SwitchToViewFVM(_fs.ViewMode, GetWindowStyle(_hwndListview) & LVS_TYPEMASK, TRUE));
    else
    {
        lRes = SUCCEEDED(_SwitchToViewID(m_cFrame.UidFromCmdId(uID), fForce));
        if (lRes)
        {
            // free the cached contextmenu of the current selection
            // so that it can be used by the WM_INITMENUPOPUP for viewextensions
            ATOMICRELEASE(_pcmSel);
        }
    }
    return lRes;
}

void CDefView::_OnMenuTermination()
{
    //we no longer flush the context menu here because we use it in both file and edit menus
    //so if we flush it after we're done with file menu (and edit menu gets selected)
    //since this message is posted it actually flushes the newly created context menu
    //file and edit now flush before they create their versions and the left over
    //context menu is flushed when the window is destroyed -- longer life time but it works
    //
    //DV_FlushCachedMenu(this);
    CallCB(SFVM_EXITMENULOOP, 0, 0);
}

LRESULT CDefView::_SwitchDesktopHTML(BOOL fOn, BOOL fForce)
{
    LRESULT lRes;
    if (fOn)
    {
        int iView;
        UINT uID;

        //Reload the Templates even if we are not in extended view.
        m_cFrame.GetExtViews(TRUE);  //Reload the template names from registry again.

        //We want to switch to HyperText View.
        if ((iView = m_cFrame.GetViewIdFromGUID(&VID_WebView)) < 0)
            uID = 0;
        else
            uID = (UINT)iView;
        uID = m_cFrame.CmdIdFromUid(uID); // only works for docobj extended view.
        //BUGBUG: Can CmdIdFromUid() fail here (return -1) ?
        lRes = SwitchToHyperText(uID, fForce);

        HWND hwndChannelBar;
        //Check if the channel bar is currently running. If so, turn it off!
        if ((hwndChannelBar = FindWindowEx(GetShellWindow(), NULL, TEXT("BaseBar"), TEXT("ChanApp"))) ||
            (hwndChannelBar = FindWindowEx(NULL, NULL, TEXT("BaseBar"), TEXT("ChanApp")))) // can be a toplevel window
        {
            //Close the channel bar.
            PostMessage(hwndChannelBar, WM_CLOSE, (WPARAM)0L, (LPARAM)0L);
        }
    }
    else
    {
        //Switch to LargeIconView.
        _bClearItemPos = FALSE;
        _SwitchToViewFVM(FVM_ICON, LVS_ICON, _fGetWindowLV ? FALSE : TRUE);
        CoFreeUnusedLibraries();
        lRes = TRUE;
    }
    return lRes;
}


class CColumnDlg
{
public:
    HRESULT ShowDialog();
    CColumnDlg(CDefView *pdsv);
    ~CColumnDlg();

private:
    void OnInit(HWND hwndLVAll);
    BOOL SaveState();
    void MoveItem(int iDelta);
    void UpdateDlgButtons(NMLISTVIEW *pnmlv, HWND hwndDlg);

private:
    CDefView *_pdsv;
    HWND _hwndLVAll;
    UINT *_pdwOrder;
    int *_pWidths;
    BOOL _bChanged;
    BOOL _bLoaded;
    BOOL _bUpdating;    // used to block notification processing while we're updating

    static BOOL_PTR CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

CColumnDlg::CColumnDlg(CDefView *pdsv)
{
    _pdsv = pdsv;
    _pdwOrder=NULL;
    _pWidths=FALSE;
    _bLoaded=FALSE;
    _bUpdating = FALSE;
}

CColumnDlg::~CColumnDlg()
{
    if (_pdwOrder)
        LocalFree(_pdwOrder);
    if (_pWidths)
        LocalFree(_pWidths);
}

HRESULT CColumnDlg::ShowDialog()
{
    ASSERT(_pdsv);

    _bChanged = FALSE;      // We are on the stack, so no zero allocator

    _pdwOrder = (UINT*) LocalAlloc(NONZEROLPTR, sizeof(UINT) * _pdsv->m_cColItems); // total columns
    _pWidths = (int *) LocalAlloc(NONZEROLPTR, sizeof(int) * _pdsv->m_cColItems); // total columns
    if (!_pdwOrder || !_pWidths)
        return E_OUTOFMEMORY;

    DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_COLUMN_SETTINGS), _pdsv->_hwndMain, DlgProc, (LPARAM)this);
    return S_OK;
}

// Remember, each column is identified in 3 ways...
//   1. A 'real' column number, the ordinal out of all possible columns
//   2. A 'visible' column number, the index to this column in the listview
//   3. A 'column order #', the position in the header's columnorderarray

void CColumnDlg::OnInit(HWND hwndLVAll)
{
    UINT i, iItem, iVisible;
    UINT *pOrderInverse;

    LV_ITEM lvi;
    LV_COLUMN lvc;

    // Fill in order array with visible columns, and set up inverse table
    _pdsv->MapRealToVisibleColumn(-1, &iVisible);
    iVisible++;
    
    ListView_GetColumnOrderArray(_pdsv->_hwndListview, iVisible, _pdwOrder);
    pOrderInverse = (UINT *)LocalAlloc(NONZEROLPTR, sizeof(DWORD)*iVisible);
    if (!pOrderInverse)
        return;
    for(i=0;i<iVisible;i++)
        pOrderInverse[_pdwOrder[i]] = i;

    _hwndLVAll = hwndLVAll;
    ListView_SetExtendedListViewStyle(hwndLVAll, LVS_EX_CHECKBOXES);

    lvc.mask = (LVCF_FMT | LVCF_SUBITEM);
    lvc.fmt = LVCFMT_LEFT;

    lvc.iSubItem = 0;
    ListView_InsertColumn(hwndLVAll, 0, &lvc);

    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 0;
    for(i=0;i < (int)_pdsv->m_cColItems;i++)
    {
        if (!_pdsv->IsColumnHidden(i))  // Don't put in entries for hidden columns
        {
            lvi.iItem = i;
            lvi.pszText = LPSTR_TEXTCALLBACK;
            ListView_InsertItem(hwndLVAll, &lvi);
        }
    }

    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    // set the visible columns
    for (i=0;i< (int) iVisible;i++)
    {
        UINT iReal;

        _pdsv->MapVisibleToRealColumn(i, &iReal);
        lvi.iItem = pOrderInverse[i];
        lvi.pszText = _pdsv->m_pColItems[iReal].szName;
        lvi.lParam = iReal;                         // store the real col index in the lParam
        lvi.state = INDEXTOSTATEIMAGEMASK(2);
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        ListView_SetItem(hwndLVAll, &lvi);

        // Get the column width from the view's listview
        _pWidths[iReal] = ListView_GetColumnWidth(_pdsv->_hwndListview, i);
    }

    iItem = iVisible;
    for(i=0;i < (int)_pdsv->m_cColItems;i++)
    {
        if (!_pdsv->IsColumnOn(i) && !_pdsv->IsColumnHidden(i))
        {
            lvi.pszText = _pdsv->m_pColItems[i].szName;
            lvi.state = INDEXTOSTATEIMAGEMASK(1);
            lvi.stateMask = LVIS_STATEIMAGEMASK;
            lvi.lParam = i;
            lvi.iItem = iItem;
            ListView_SetItem(hwndLVAll, &lvi);

            iItem++;

            // get the default width we've got saved away
            _pWidths[i] = _pdsv->m_pColItems[i].cChars * _pdsv->_cxChar;
        }
    }

    // set the size properly
    ListView_SetColumnWidth(hwndLVAll, 0, LVSCW_AUTOSIZE);

    LocalFree(pOrderInverse);
    _bLoaded = TRUE;
    ListView_SetItemState(hwndLVAll, 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
}

#define SWAP(x,y) {(x) ^= (y); (y) ^= (x); (x) ^= (y);}

void CColumnDlg::MoveItem(int iDelta)
{
    int i, iNew;

    i = ListView_GetSelectionMark(_hwndLVAll);
    if (i != -1)
    {
        iNew = i + iDelta;
        if (iNew >= 0  && iNew <= (ListView_GetItemCount(_hwndLVAll) -1) )
        {
            LV_ITEM lvi, lvi2;
            TCHAR szTmp1[MAX_COLUMN_NAME_LEN], szTmp2[MAX_COLUMN_NAME_LEN];

            _bChanged = TRUE;
            _bUpdating = TRUE;

            lvi.iItem = i;
            lvi.iSubItem =0;
            lvi.pszText = szTmp1;
            lvi.cchTextMax = ARRAYSIZE(szTmp1);
            lvi.stateMask = LVIS_STATEIMAGEMASK;
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            
            lvi2.iItem = iNew;
            lvi2.iSubItem =0;
            lvi2.pszText = szTmp2;
            lvi2.cchTextMax = ARRAYSIZE(szTmp2);
            lvi2.stateMask = LVIS_STATEIMAGEMASK;
            lvi2.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;

            ListView_GetItem(_hwndLVAll, &lvi);
            ListView_GetItem(_hwndLVAll, &lvi2);

            SWAP(lvi.iItem, lvi2.iItem);

            ListView_SetItem(_hwndLVAll, &lvi);
            ListView_SetItem(_hwndLVAll, &lvi2);

            _bUpdating=FALSE;

            // update selection
            ListView_SetSelectionMark(_hwndLVAll, iNew);
            ListView_SetItemState(_hwndLVAll, iNew , LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
            // HACK: SetItemState sends notifications for i, iNew, then i again.
            // we need to call it twice in a row, so UpdateDlgButtons will get the right item
            ListView_SetItemState(_hwndLVAll, iNew , LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);

            return;
        }
    }
    TraceMsg(TF_WARNING, "ccd.mi couldn't move %d to %d",i, i+iDelta);
    MessageBeep(MB_ICONEXCLAMATION);
    return;
}

BOOL CColumnDlg::SaveState()
{
    int i, cItems = ListView_GetItemCount(_hwndLVAll);
    LV_ITEM lvi = {0};
    int iOrderIndex;

    // Check order
    if (!_bChanged)
        return FALSE;   

    iOrderIndex = 0;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.mask = LVIF_PARAM | LVIF_STATE;

    for(i=0;i < cItems; i++)
    {
        lvi.iItem = i;
        ListView_GetItem(_hwndLVAll, &lvi);

        // toggle it, if the state in the dialog doesn't match the listview state
//        if (lvi.state != (UINT)INDEXTOSTATEIMAGEMASK(_pdsv->IsColumnOn(lvi.lParam)?2:1))  // BUGBUG?  lvi.state is coming back as zero always
        if (BOOLIFY(ListView_GetCheckState(_hwndLVAll, i)) != BOOLIFY(_pdsv->IsColumnOn((UINT)lvi.lParam)))
        {
            _pdsv->_HandleColumnToggle((UINT)lvi.lParam, FALSE);
        }

        if (_pdsv->IsColumnOn((UINT)lvi.lParam))
            _pdwOrder[iOrderIndex++] = (UINT)lvi.lParam; // incorrectly store real (not vis) col #, fix up below
    }

    // must be in a separate loop. (can't map real to visible, if we aren't done setting visible)
    for(i=0;i<iOrderIndex; i++)
    {
        UINT iReal = _pdwOrder[i];
        _pdsv->MapRealToVisibleColumn(iReal, &_pdwOrder[i]);

        if (_pWidths[iReal] < 0) // negative width means they edited it
            ListView_SetColumnWidth(_pdsv->_hwndListview, _pdwOrder[i], -_pWidths[iReal]);

    }
    ListView_SetColumnOrderArray(_pdsv->_hwndListview, iOrderIndex, _pdwOrder);

    // kick the listview into repainting everything
    InvalidateRect(_pdsv->_hwndListview, NULL, TRUE);

    _bChanged = FALSE;
    return TRUE;
}

BOOL EnableDlgItem(HWND hdlg, UINT idc, BOOL f)
{
    return EnableWindow(GetDlgItem(hdlg, idc), f);
}

void CColumnDlg::UpdateDlgButtons(NMLISTVIEW *pnmlv, HWND hwndDlg)
{
    BOOL bChecked, bOldUpdateState=_bUpdating;
    int iWidth;
    int iItem = ListView_GetSelectionMark(_hwndLVAll);

    // to disable checking
    _bUpdating = TRUE;
    if (pnmlv->uNewState & LVIS_STATEIMAGEMASK)
        bChecked = (pnmlv->uNewState & LVIS_STATEIMAGEMASK) == (UINT)INDEXTOSTATEIMAGEMASK(2);
    else 
        bChecked = ListView_GetCheckState(_hwndLVAll, pnmlv->iItem);

    EnableDlgItem(hwndDlg, IDC_COL_UP, pnmlv->iItem > 0);
    EnableDlgItem(hwndDlg, IDC_COL_DOWN, pnmlv->iItem < (int)_pdsv->m_cColItems-1);
    EnableDlgItem(hwndDlg, IDC_COL_SHOW, !bChecked && (pnmlv->lParam != 0));
    EnableDlgItem(hwndDlg, IDC_COL_HIDE, bChecked && (pnmlv->lParam != 0));

    // update the width edit box
    iWidth = _pWidths[pnmlv->lParam];
    if (iWidth < 0) iWidth = -iWidth;   // we store negative values to track if it changed or not
    SetDlgItemInt(hwndDlg, IDC_COL_WIDTH, iWidth, TRUE);

    _bUpdating = bOldUpdateState;
}


BOOL_PTR CALLBACK CColumnDlg::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CColumnDlg *pcd;

    if (uMsg == WM_INITDIALOG)
    {
        pcd = (CColumnDlg *) lParam;
        if (!pcd)
            EndDialog(hwndDlg, FALSE);
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pcd);
    }
    else
        pcd = (CColumnDlg*) GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pcd->OnInit(GetDlgItem(hwndDlg, IDC_COL_LVALL));
            SendDlgItemMessage(hwndDlg, IDC_COL_WIDTH, EM_LIMITTEXT, 3, 0); // 3 digits
            break;
        case WM_DESTROY:
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_COL_UP:
                    pcd->MoveItem(- 1);
                    goto Common;
                case IDC_COL_DOWN:
                    pcd->MoveItem(+ 1);
                    goto Common;
Common:
                    SetFocus(pcd->_hwndLVAll);
                    break;

                case IDC_COL_SHOW:
                case IDC_COL_HIDE:
                {
                    UINT iItem = ListView_GetSelectionMark(pcd->_hwndLVAll);
                    ListView_SetCheckState(pcd->_hwndLVAll, iItem, LOWORD(wParam) == IDC_COL_SHOW);
                    SetFocus(pcd->_hwndLVAll);
                    break;
                }
                case IDC_COL_WIDTH:
                    if (HIWORD(wParam) == EN_CHANGE && !pcd->_bUpdating)
                    {
                        LV_ITEM lvi;
                        lvi.iItem = ListView_GetSelectionMark(pcd->_hwndLVAll);
                        lvi.iSubItem = 0;
                        lvi.mask = LVIF_PARAM;
                        ListView_GetItem(pcd->_hwndLVAll, &lvi);

                        pcd->_pWidths[lvi.lParam] = - (int)GetDlgItemInt(hwndDlg, IDC_COL_WIDTH, NULL, FALSE);
                        pcd->_bChanged = TRUE;
                    }
                    break;

                case IDOK:
                    pcd->SaveState(); // fall through
                case IDCANCEL:
                    return EndDialog(hwndDlg, TRUE);
            }
            break;

        case WM_NOTIFY:
            if (pcd->_bLoaded && !pcd->_bUpdating)
            {
                NMLISTVIEW * pnmlv = (NMLISTVIEW*)lParam;
                switch (((LPNMHDR)lParam)->code)
                {
                    case LVN_ITEMCHANGING:

                        // fix up the buttons & such here
                        if (pnmlv->uChanged & LVIF_STATE)
                            pcd->UpdateDlgButtons(pnmlv, hwndDlg);

                        // We want to reject turning off the name column
                        // it both doesn't make sense to have no name column, and defview assumes there will be one
                        if (pnmlv->lParam == 0 &&
                            (pnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(1))
                        {
                            MessageBeep(MB_ICONEXCLAMATION);
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                            return TRUE;
                        }
                        else
                        {
                            // if something besides focus changed
                            if ((pnmlv->uChanged & ~LVIF_STATE) ||
                                ((pnmlv->uNewState & LVIS_STATEIMAGEMASK) != (pnmlv->uOldState & LVIS_STATEIMAGEMASK)))
                            pcd->_bChanged = TRUE;
                        }
                        break;

                    case NM_DBLCLK:
                        {
                            BOOL bCheck = ListView_GetCheckState(pcd->_hwndLVAll, pnmlv->iItem);
                            ListView_SetCheckState(pcd->_hwndLVAll, pnmlv->iItem, !bCheck);
                        }
                        break;
                }
            }

            break;

        case WM_SYSCOLORCHANGE:
            SendMessage(pcd->_hwndLVAll, uMsg, wParam, lParam);
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

LRESULT ToggleComponent(UINT uID)
{
    IActiveDesktop *piad;
    if (SUCCEEDED(CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (void **)&piad)))
    {
        COMPONENT   comp;
        TCHAR       componentSource[INTERNET_MAX_URL_LENGTH];

        comp.dwSize = sizeof(comp);
        if (SUCCEEDED(piad->GetDesktopItem(uID - SFVIDM_DESKTOPHTML_ADDSEPARATOR - 1, &comp, 0)))
        {
            SHUnicodeToTChar(comp.wszSource, componentSource, ARRAYSIZE(componentSource));
            comp.fChecked = !comp.fChecked;

//  98/12/16 vtan #250938: If ICW has not been run to completion then only
//  allow the user to toggle components that are local pictures of some sort.
//  If not then notify the user of the problem and launch ICW.

            if (comp.fChecked && !IsICWCompleted() && !IsLocalPicture(componentSource))
            {
                ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE(IDS_COMP_ICW_TOGGLE), MAKEINTRESOURCE(IDS_COMP_ICW_TITLE), MB_OK);
                LaunchICW();
            }
            else
            {
                piad->ModifyDesktopItem(&comp, COMP_ELEM_CHECKED);
                // Note: Do not use AD_APPLY_FORCE here; It will force saving Wallpaper and pattern 
                // which will cause a complete refresh of desktop causing a flicker.
                // What we really want to save is just the component that toggled.
                piad->ApplyChanges(AD_APPLY_ALL |AD_APPLY_DYNAMICREFRESH);
            }
        }
        piad->Release();
    }

    return 0;
}

DWORD NewDesktopItem(LPVOID lpParameter)
{
    IActiveDesktop * piad;

    CoInitialize(0);

    if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_IActiveDesktop, (void **)&piad)))
    {
        if (NewComponent(*(HWND *)lpParameter, piad, FALSE, NULL) >= 0)
        {
            // If we succeeded in adding an item then ensure AD is on and then apply
            // the changes.
            COMPONENTSOPT co;
            IADesktopP2 * piadp2;

            co.dwSize = sizeof(COMPONENTSOPT);
            piad->GetDesktopItemOptions(&co, 0);
            co.fActiveDesktop = TRUE;
            piad->SetDesktopItemOptions(&co, 0);
            // If we need to turn AD on at this point we need to refresh the wallpaper state
            // in our piad object, use IADesktopP2 to do so.  Otherwise if there was an html
            // wallpaper it won't show up when we apply changes.
            if(SUCCEEDED(piad->QueryInterface(IID_IADesktopP2, (void **)&piadp2)))
            {
                piadp2->ReReadWallpaper();
                piadp2->Release();
            }
            piad->ApplyChanges(AD_APPLY_ALL | AD_APPLY_DYNAMICREFRESH);
        }
        piad->Release();
    }

    CoUninitialize();

    return 0;
}

void CDefView::_DoColumnsMenu(int x, int y) // X and Y are screen coordinates
{
    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        AddColumnsToMenu(hmenu, SFVIDM_COLUMN_FIRST);

        int item = TrackPopupMenu(hmenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
                                  x, y, 0, _hwndListview, NULL);
        DestroyMenu(hmenu);

        // validate item first
        if (item == SFVIDM_VIEW_COLSETTINGS)
        {
            CColumnDlg ccd(this);

            if (!_bLoadedColumns)
                AddColumns();

            ccd.ShowDialog();
        }
        else if (item > SFVIDM_COLUMN_FIRST)
            _HandleColumnToggle(item-SFVIDM_COLUMN_FIRST, TRUE);
    }
}

LRESULT CDefView::Command(IContextMenu *pcmSel, WPARAM wParam, LPARAM lParam)
{
    TCHAR szSettings[MAX_PATH];
    DWORD dwStyle;
    UINT fvmNew;
    UINT uCMBias = SFVIDM_CONTEXT_FIRST;
    int iItem;
    UINT uID = GET_WM_COMMAND_ID(wParam, lParam);

    if (InRange(uID, SFVIDM_VIEW_EXTFIRST, SFVIDM_VIEW_EXTLAST))
    {
        return SwitchToHyperText(uID, FALSE);
    }

    if (InRange(uID, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST) ||
        InRange(uID, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST))
    {
        BOOL bRenameIt = FALSE;

        IContextMenu * pcmFree = NULL;

        if (pcmSel == NULL)
        {
            if (InRange(uID, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST))
            {
                pcmSel = _pcmBackground;
                uCMBias = SFVIDM_BACK_CONTEXT_FIRST;
                if (pcmSel)
                {
                    pcmSel->AddRef();
                }
            }
            else
            {
                // We hopefully have a pcmSel object cached away for the current
                // selection.  If not this wont work anyway so blow out.
                pcmSel = _pcmSel;
                uCMBias = SFVIDM_CONTEXT_FIRST;

                if (pcmSel)
                {
                    pcmSel->AddRef();
                }
                // Extended views don't cache the context menu because the
                // top defview isn't notified of selection changes, so the
                // cache would be wrong.
                else if (!_HasNormalView())
                {
                    pcmSel = _GetContextMenuFromSelection();
                }
            }

            pcmFree = pcmSel;
        }

        if (pcmSel)
        {
            CHAR szCmdA[MAX_PATH];
            TCHAR szCommandString[MAX_PATH];
            // make sure this object exists throughout the operation.
            // ie we dont want a change attributes callback  delete this
            // object while we are using it...
            pcmSel->AddRef();

            szCommandString[0] = 0;
            szCmdA[0] = 0;

            HRESULT hres = pcmSel->GetCommandString(uID - uCMBias, GCS_VERBA, NULL,
                szCmdA, ARRAYSIZE(szCmdA));
   
            if (SUCCEEDED(hres))
                SHAnsiToTChar(szCmdA, szCommandString, ARRAYSIZE(szCommandString));

            if (szCommandString[0] == 0)
            {
                WCHAR szCmdW[60];
                szCmdW[0] = 0;
                hres = pcmSel->GetCommandString(uID - uCMBias, GCS_VERBW, NULL,
                        (LPSTR)szCmdW, ARRAYSIZE(szCmdW));
                SHUnicodeToTChar(szCmdW, szCommandString, ARRAYSIZE(szCommandString));
            }

            // We need to special case the rename command
            if (lstrcmpi(szCommandString, c_szRename)==0)
            {
                bRenameIt = TRUE;
            }
            else
            {
                CMINVOKECOMMANDINFOEX ici = { 0 };

                ici.cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
                ici.hwnd = _hwndMain;
                ici.lpVerb = (LPSTR)MAKEINTRESOURCE(uID - uCMBias);
                ici.nShow = SW_NORMAL;

                // Set the invoke point for this command
                {
                    POINT ptSelect;
                    int iItemSelect = ListView_GetNextItem(_hwndListview, -1, LVNI_SELECTED);
                    if (iItemSelect != -1)
                    {
                        RECT rcItem;
                        ListView_GetItemRect(_hwndListview, iItemSelect, &rcItem, LVIR_BOUNDS);
                        MapWindowPoints(_hwndListview, HWND_DESKTOP, (LPPOINT)&rcItem, 2);
                        ptSelect.x = (rcItem.left + rcItem.right) / 2;
                        ptSelect.y = (rcItem.top + rcItem.bottom) / 2;
                        ici.ptInvoke = ptSelect;
                        ici.fMask |= CMIC_MASK_PTINVOKE;
                    }
                }

                // record if shift or control was being held down
                SetICIKeyModifiers(&ici.fMask);

                _InvokeCommand(pcmSel, &ici);
            }

            //Since we are releaseing our only hold on the context menu, release the site.
            IUnknown_SetSite(pcmSel, NULL);

            // And release our use of it.
            ATOMICRELEASE(pcmSel);
            ATOMICRELEASE(pcmFree);

            if (bRenameIt)
            {
                if (_HasNormalView())
                {
                    goto RenameIt_NormalView;
                }
                else
                {
                    TraceMsg(TF_ERROR, "Extended View: Command can't rename");
                }
            }
        }

        return 0;
    }


    // Is the ID within the client's range?
    if (InRange(uID, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && this->HasCB())
    {
        // Yes; pass it on to the callback
        CallCB(SFVM_INVOKECOMMAND, uID - SFVIDM_CLIENT_FIRST, 0);
        return 0;
    }

    //
    // Special commands for the desktop menu
    //
    if (InRange(uID, SFVIDM_DESKTOPHTML_ADDSEPARATOR + 1, SFVIDM_DESKTOP_LAST))
    {
        return ToggleComponent(uID);
    }

    //
    // First check for commands that always go to this defview
    //
    switch (uID)
    {
    case SFVIDM_EDIT_UNDO:
        // if we are in label edit mode, don't allowany of the buttons......
        if ( _fInLabelEdit || (m_cFrame.IsSFVExtension() && m_cFrame.IsExtendedSFVModal()))
        {
            MessageBeep(0);
            return 0;
        }

        Undo(_hwndMain);
        break;

    case SFVIDM_VIEW_COLSETTINGS:
        {
            CColumnDlg ccd(this);

            if (!_bLoadedColumns)
                this->AddColumns();

            ccd.ShowDialog();
            break;
        }

    case SFVIDM_VIEW_VIEWMENU:
        {
            // if we are in label edit mode, don't allowany of the buttons......
            if ( _fInLabelEdit || (m_cFrame.IsSFVExtension() && m_cFrame.IsExtendedSFVModal()))
            {
                MessageBeep(0);
                return 0;
            }

            LPCDFVCMDDATA pcd = (LPCDFVCMDDATA)lParam;
            if (pcd)
            {
                //
                // First, let's get that view menu.
                //
                HMENU hmenuCtx = SHLoadPopupMenu(HINST_THISDLL, POPUP_SFV_BACKGROUND);
                if (hmenuCtx)
                {
                    MENUITEMINFO mii = {0};

                    m_cFrame.MergeExtViewsMenu(hmenuCtx, this);
                    CheckCurrentViewMenuItem(hmenuCtx);

                    mii.cbSize = SIZEOF(mii);
                    mii.fMask = MIIM_SUBMENU;
                    GetMenuItemInfo(hmenuCtx, SFVIDM_MENU_VIEW, MF_BYCOMMAND, &mii);
                    HMENU hmenuView = mii.hSubMenu;

                    if (pcd->pva && pcd->pva->byref)
                    {
                        //
                        // We have X,Y coordinates, let's bring up a context
                        // menu at that location.
                        //
                        LPRECT prect = (LPRECT)pcd->pva->byref;
                        int idCmd = TrackPopupMenu(hmenuView, TPM_RETURNCMD, prect->left, prect->bottom, 0, pcd->hwnd, NULL);
                        if (idCmd)
                            Command(NULL, GET_WM_COMMAND_MPS(idCmd, 0, 0));
                    }

// 226474: view button changing from BTNS_DROPDOWN (split dropdown) to BTNS_WHOLEDROPDOWN
// (unsplit dropdown) so we just need to handle the dropdown behavior.
//
// i'm leaving the old code in for now under #if 0 in case we change our minds.  remove by nt5 beta3.
//
#if 0
                    else
                    {
                        //
                        // No X,Y coordinates, user clicked on button portion
                        // of the view combo.  Let's cycle to the next view.
                        //
                        BOOL fSeparator = FALSE;
                        int iChecked = -1;

                        for (int i=0; ; i++)
                        {
                            //
                            // Find the currently selected view.
                            //
                            ZeroMemory(&mii, SIZEOF(mii));
                            mii.cbSize = SIZEOF(mii);
                            mii.fMask = MIIM_STATE | MIIM_TYPE;
                            if (GetMenuItemInfo(hmenuView, i, MF_BYPOSITION, &mii) == FALSE)
                            {
                                break;
                            }

                            if (mii.fType & MFT_SEPARATOR)
                            {
                                fSeparator = TRUE;
                            }

                            if ((mii.fState & MFS_CHECKED) && (mii.fType & MFT_RADIOCHECK))
                            {
                                iChecked = i;
                                break;
                            }
                        }

                        if (iChecked != -1)
                        {
                            int wNextID = -1;

                            //
                            // We found the radio chekced item, now
                            // find the next view.
                            //
                            ZeroMemory(&mii, SIZEOF(mii));
                            mii.cbSize = SIZEOF(mii);
                            mii.fMask = MIIM_ID;
                            if (GetMenuItemInfo(hmenuView, iChecked + 1, MF_BYPOSITION, &mii))
                            {
                                wNextID = mii.wID;
                            }
                            else
                            {
                                //
                                // We fell off the end of the menu, so
                                // find the first selectable view.
                                //
                                TCHAR szText[MAX_PATH];
                                ZeroMemory(&mii, SIZEOF(mii));
                                mii.cbSize = SIZEOF(mii);
                                mii.fMask = MIIM_ID | MIIM_TYPE;
                                mii.dwTypeData = szText;
                                mii.cch = ARRAYSIZE( szText );
                                for (i=0; ; i++)
                                {
                                    if (GetMenuItemInfo(hmenuView, i, MF_BYPOSITION, &mii) == FALSE)
                                    {
                                        break;
                                    }

                                    if (fSeparator)
                                    {
                                        //
                                        // Need to skip past the separator
                                        // and all of the toggle items before it.
                                        //
                                        if (mii.fType & MFT_SEPARATOR)
                                        {
                                            fSeparator = FALSE;
                                        }
                                        continue;
                                    }

                                    //
                                    // Found the first item.
                                    //
                                    wNextID = mii.wID;
                                    break;
                                }
                            }

                            if (wNextID != -1)
                            {
                                Command(pcmSel, MAKELONG(wNextID, 0), 0);
                            }
                        }
                    }
#endif // 0

                    DestroyMenu(hmenuCtx);
                }
            }
        }
        break;

    case SFVIDM_VIEW_ICON:
        dwStyle = LVS_ICON;
        _bClearItemPos = FALSE;
        fvmNew = FVM_ICON;
        goto SetStyle;

    case SFVIDM_VIEW_SMALLICON:
        dwStyle = LVS_SMALLICON;
        _bClearItemPos = FALSE;
        fvmNew = FVM_SMALLICON;
        goto SetStyle;

    case SFVIDM_VIEW_LIST:
        dwStyle = LVS_LIST;
        _bClearItemPos = TRUE;
        fvmNew = FVM_LIST;
        goto SetStyle;

    case SFVIDM_VIEW_DETAILS:
        dwStyle = LVS_REPORT;
        _bClearItemPos = TRUE;
        fvmNew = FVM_DETAILS;
SetStyle:
        _SwitchToViewFVM(fvmNew, dwStyle, m_cFrame.IsWebView() ? FALSE : TRUE);
        break;

    case SFVIDM_DESKTOPHTML_WEBCONTENT:
        {
            bool        bHasVisibleNonLocalPicture;
            SHELLSTATE  ss;

            SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE); // Get the setting
            ss.fDesktopHTML  = !ss.fDesktopHTML;           // Toggle the state
            bHasVisibleNonLocalPicture = false;
            if (ss.fDesktopHTML && !IsICWCompleted())
            {
                IActiveDesktop  *pIAD;

                if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_IActiveDesktop, reinterpret_cast<void**>(&pIAD))))
                {
                    bHasVisibleNonLocalPicture = (DisableUndisplayableComponents(pIAD) != 0);
                    pIAD->Release();
                }
            }
            if (!bHasVisibleNonLocalPicture)
            {
                SHELLSTATE ss2;

                SHGetSetSettings(&ss, SSF_DESKTOPHTML, TRUE);  // Write back the new

                // Now read back the current setting - only call _SwitchDesktopHTML if the current
                // setting and the one we just set agree.  If they don't that means someone changed
                // the setting during the above call and we shouldn't do any more work or our state
                // will get messed up.
                SHGetSetSettings(&ss2, SSF_DESKTOPHTML, FALSE); 
                if (ss.fDesktopHTML == ss2.fDesktopHTML)
                    _SwitchDesktopHTML(ss.fDesktopHTML, FALSE);
            }
        }
        break;

    case SFVIDM_DESKTOPHTML_SYNCHRONIZE:
        {
            IOleCommandTarget* pct;
            // Call the command target to Update and refresh even if the desktop frame
            // is in offline mode.
            ASSERT(m_cFrame.IsWebView());  //This command is valid only for extended views.
            if (SUCCEEDED(_psb->QueryInterface(IID_IOleCommandTarget, (void **)&pct)))
            {
                pct->Exec(&CGID_ShellDocView, SHDVID_UPDATEOFFLINEDESKTOP, 0, NULL, NULL);
                pct->Release();
            }
        }
        break;

    case SFVIDM_DESKTOPHTML_CUSTOMIZE:
        LoadString(HINST_THISDLL, IDS_COMPSETTINGS, szSettings, ARRAYSIZE(szSettings));
        SHRunControlPanel(szSettings, NULL);
        break;

    case SFVIDM_DESKTOPHTML_NEWITEM:
        CloseHandle(CreateThread(NULL, 0, NewDesktopItem, &_hwndMain, 0, &dwStyle));
        break;

    case SFVIDM_DESKTOPHTML_ICONS:
    case SFVIDM_ARRANGE_DISPLAYICONS:
        {
            SHELLSTATE ss;
            DWORD dwValue;

            // Toggle the cached state
            _fs.fFlags ^= FWF_NOICONS;

            ss.fHideIcons = ((_fs.fFlags & FWF_NOICONS) != 0);
            dwValue = ss.fHideIcons ? 1 : 0;

            // Since this value is currrently stored under the "advanced" reg tree we need
            // to explicitly write to the registry or the value won't persist properly via
            // SHGetSetSettings.
            SHSetValue(HKEY_CURRENT_USER,
                    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                    TEXT("HideIcons"), REG_DWORD, &dwValue, SIZEOF(dwValue));

            // Finally set the ShellState and perform the action!
            SHGetSetSettings(&ss, SSF_HIDEICONS, TRUE);
            // Since this SFVIDM_ comes from the menu, we better already be active (or
            // this SW_SHOW could make us visible before we want to be seen).
            ASSERT(_uState != SVUIA_DEACTIVATE);
            ShowWindow(_hwndListview, (DV_SHOWICONS(this) ? SW_SHOW : SW_HIDE));
        }
        break;

    case SFVIDM_DESKTOPHTML_LOCK:
        {
            DWORD dwFlags = GetDesktopFlags();
            dwFlags ^= COMPONENTS_LOCKED;
            SetDesktopFlags(COMPONENTS_LOCKED, dwFlags);
        }
        break;

    case SFVIDM_EDIT_COPYTO:
    case SFVIDM_EDIT_MOVETO:
        {
            // if we are in label edit mode, don't allowany of the buttons......
            if ( _fInLabelEdit || (m_cFrame.IsSFVExtension() && m_cFrame.IsExtendedSFVModal()))
            {
                MessageBeep(0);
                return 0;
            }

            CLSID clsid;
            IContextMenu *pcm;

            if (uID == SFVIDM_EDIT_COPYTO)
                clsid = CLSID_CopyToMenu;
            else
                clsid = CLSID_MoveToMenu;

            if (SUCCEEDED(SHCoCreateInstance(NULL, &clsid, NULL, IID_IContextMenu, (void **)&pcm)))
            {
                IShellExtInit* psei;

                IUnknown_SetSite(pcm, SAFECAST(this, IDropTarget *)); // Needed to go modal during UI

                if (SUCCEEDED(pcm->QueryInterface(IID_IShellExtInit, (void **)&psei)))
                {
                    IDataObject* pdobj;

                    if (SUCCEEDED(DefView_GetUIObjectFromItem(this, IID_IDataObject, (void **)&pdobj, SVGIO_SELECTION)))
                    {
                        LPITEMIDLIST pidlFolder = _GetViewPidl();
                        CMINVOKECOMMANDINFO ici = {0};
                        
                        psei->Initialize(pidlFolder, pdobj, NULL);

                        ici.hwnd = _hwndMain;
                        pcm->InvokeCommand(&ici);

                        ILFree(pidlFolder); // OK if NULL
                        pdobj->Release();
                    }
                    psei->Release();
                }

                IUnknown_SetSite(pcm, NULL);
                pcm->Release();
            }
        }
        break;

    case SFVIDM_FILE_PROPERTIES:

        if (SHRestricted(REST_NOVIEWCONTEXTMENU)) {
             break;
        }

         // else fall through...

    case SFVIDM_EDIT_PASTE:
    case SFVIDM_EDIT_PASTELINK:
    case SFVIDM_EDIT_COPY:
    case SFVIDM_EDIT_CUT:
    case SFVIDM_FILE_LINK:
    case SFVIDM_FILE_DELETE:
        // if we are in label edit mode, don't allowany of the buttons......
        if (_fInLabelEdit || (m_cFrame.IsSFVExtension() && m_cFrame.IsExtendedSFVModal()))
        {
            MessageBeep(0);
            return 0;
        }

        if (DefView_AllowCommand(this, uID, wParam, lParam))
        {
            if (FAILED(DefView_ExplorerCommand(this, GET_WM_COMMAND_ID(wParam, lParam))))
                MessageBeep(0);
        }
        else
        {
            LPDFVCMDDATA pcd = (LPDFVCMDDATA)lParam;
            // Try translating the SFVIDM value into a standard
            // OLECMDID value, so that the caller can try applying
            // it to a different object.
            if (!IsBadWritePtr(pcd, SIZEOF(*pcd)))
            {
                pcd->nCmdIDTranslated = OlecmdidFromSfvidm(uID);
            }
        }
        break;

    case SFVIDM_TOOL_OPTIONS:
        if (!SHRestricted(REST_NOFOLDEROPTIONS))
        {
            IUnknown_Exec(_psb, &CGID_Explorer, SBCMDID_OPTIONS, 0, NULL, NULL);
        }
        break;

    case SFVIDM_HELP_TOPIC:
        //
        // HACK: Don't call WinHelp when we are in the common dialog.
        //
        if (!DV_CDB_IsCommonDialog(this))
        {
            // Use a callback to see if the namespace has requested a different help file name and/or topic
            SFVM_HELPTOPIC_DATA htd;
            HWND hwndDesktop = GetDesktopWindow();
            SHTCharToUnicode(c_szHtmlWindowsHlp, htd.wszHelpFile, ARRAYSIZE(htd.wszHelpFile));
            htd.wszHelpTopic[0] = L'\0';
            if (SUCCEEDED(CallCB(SFVM_GETHELPTOPIC, 0, (LPARAM)&htd)))
            {
                TCHAR szHelpFile[MAX_PATH];
                TCHAR szHelpTopic[MAX_PATH];
                SHUnicodeToTChar(htd.wszHelpFile, szHelpFile, ARRAYSIZE(szHelpFile));
                SHUnicodeToTChar(htd.wszHelpTopic, szHelpTopic, ARRAYSIZE(szHelpTopic));
                HtmlHelp(hwndDesktop, szHelpFile, HH_HELP_FINDER, szHelpTopic[0] ? (DWORD_PTR)szHelpTopic : 0);
            }
            else
            {
                HtmlHelp(hwndDesktop, c_szHtmlWindowsHlp, HH_HELP_FINDER, 0);
            }
        }
        break;

    case SFVIDM_VIEW_CUSTOMWIZARD:
        //ShellExec the Wizard to customize this folder.
        _InvokeCustomWizard();
        break;

    case SFVIDM_MISC_SETWEBVIEW:
        if ((BOOL_PTR)lParam)   // We have to set WebView
        {
            SHELLVIEWID vid = VID_WebView;
            _SwitchToViewPVID(&vid, TRUE);
        }
        else
        {
            if (!m_cFrame.IsSFVExtension())
            {
                _SwitchToViewFVM(_fs.ViewMode, GetWindowStyle(_hwndListview) & LVS_TYPEMASK, TRUE);
            }
            else
            {
                // We don't have a function to switch off only webview without switching off the extended view.
                // So, let's first switch off all extended views then switch back to the current extended view.
                int uView = m_cFrame.m_uView;
                _SwitchToViewID((UINT)CSFVFrame::HIDEEXTVIEW, FALSE);
                _SwitchToViewID(uView, FALSE);
            }
        }
        break;

    case SFVIDM_MISC_REFRESH:
        Refresh();
        break;

    default:
        if (this->m_cFrame.IsSFVExtension())
        {
            IShellFolderView * pSFV = this->m_cFrame.GetExtendedISFV();
            IShellView2 * pView = this->m_cFrame.GetExtendedISV();

            HRESULT hr = E_NOTIMPL;
            switch(uID)
            {
            case SFVIDM_SELECT_ALL:
                hr = pSFV->Select( SFVS_SELECT_ALLITEMS );
                break;
            case SFVIDM_DESELECT_ALL:
                hr = pSFV->Select( SFVS_SELECT_NONE );
                break;
            case SFVIDM_SELECT_INVERT:
                hr = pSFV->Select( SFVS_SELECT_INVERT );
                break;
            case SFVIDM_FILE_RENAME:
            {
                // get the selected items..
                LPCITEMIDLIST * apidl = NULL;
                UINT cidl = 0;
                hr = pSFV->GetSelectedObjects(&apidl, &cidl);
                if (hr != S_OK || cidl <= 0)
                {
                    MessageBeep(0);
                }
                else
                {
                    ASSERT(NULL != apidl);
                    ASSERT(NULL != apidl[cidl - 1]);

                    hr = pView->HandleRename(apidl[cidl -1]);
                }

                if (apidl != NULL)
                    LocalFree((void *)apidl);
                break;
            }

            case SFVIDM_ARRANGE_AUTO:
                hr = pSFV->AutoArrange();
                break;
            case SFVIDM_ARRANGE_GRID:
                hr = pSFV->ArrangeGrid();
                break;
            }
        }
        else
        //
        // Second check for commands that need to be sent to the active object
        //
        // An overload of the fDontForward flag allows us to always tell it to do the
        // right thing for the Def-view regardless of whether we are looking at
        // an extension view or not..
        //
        {
            //Commands that can never go to an extended view!
            switch (uID)
            {
            case SFVIDM_ARRANGE_AUTO:
                _fs.fFlags ^= FWF_AUTOARRANGE;     // toggle
                _bItemsMoved = FALSE;              // positions are all gone
                SetWindowLongPtr(_hwndListview, GWL_STYLE
                            , GetWindowStyle(_hwndListview) ^ LVS_AUTOARRANGE);
                break;

            case SFVIDM_ARRANGE_GRID:
#ifdef  TEST_LVDISABLE
                {
                    static BOOL fFlag = 0;
                    EnableWindow(_hwndListview, fFlag);
                    fFlag = !fFlag;
                }
#endif
                ListView_Arrange(_hwndListview, LVA_SNAPTOGRID);
                break;

            default:
                //
                // Normal view, we know what to do
                //
                switch (uID)
                {
                case SFVIDM_SELECT_ALL:
                {
                    DECLAREWAITCURSOR;

                    if (CallCB(SFVM_SELECTALL, 0, 0) != (S_FALSE)) {
                        SetWaitCursor();
                        
                        // BUGBUG: this should cause the DVOC to call "SetActiveObject" on
                        // trident, and it won't do this since we just set focus to ourself...
                        SetFocus(_hwndListview);
                        ListView_SetItemState(_hwndListview, -1, LVIS_SELECTED, LVIS_SELECTED);
                        ResetWaitCursor();
                    }
                    break;
                }

                case SFVIDM_DESELECT_ALL:
                    ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);
                    break;

                case SFVIDM_SELECT_INVERT:
                {
                    DECLAREWAITCURSOR;
                    SetWaitCursor();
                    // BUGBUG: this should cause the DVOC to call "SetActiveObject" on
                    // trident, and it won't do this since we just set focus to ourself...
                    SetFocus(_hwndListview);
                    iItem = -1;
                    while ((iItem = ListView_GetNextItem(_hwndListview, iItem, 0)) != -1)
                    {
                        // flip the selection bit on each item
                        UINT flag = ListView_GetItemState(_hwndListview, iItem, LVIS_SELECTED);
                        flag ^= LVNI_SELECTED;
                        ListView_SetItemState(_hwndListview, iItem, flag, LVIS_SELECTED);
                    }
                    ResetWaitCursor();
                    break;
                }

#if 0
                case SFVIDM_SELECT_FILELIST:
                {
                    int nItem = -1;
                    WIN32_FIND_DATA fd;

                    DECLAREWAITCURSOR;
                    SetWaitCursor();
                    
                    // BUGBUG: this should cause the DVOC to call "SetActiveObject" on
                    // trident, and it won't do this since we just set focus to ourself...
                    SetFocus(_hwndListview);

                    // First, deselect all...
                    ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);

                    // Second, select all files that match the criteria.
                    if (lParam)
                    {
                        HANDLE hfind = FindFirstFile((LPCSTR) lParam, &fd);
                        if (hfind != INVALID_HANDLE_VALUE)
                        {
                            do
                            {
                                LPITEMIDLIST pidl = ILCreateFromPath(fd.cFileName);
                                if (pidl)
                                {
                                    int nItem = _FindItem(pidl, NULL, FALSE);

                                    // flip the selection bit on each item
                                    UINT flag = ListView_GetItemState(_hwndListview, nItem, LVIS_SELECTED);
                                    SetFlag(flag, LVNI_SELECTED);
                                    ListView_SetItemState(_hwndListview, nItem, flag, LVIS_SELECTED);

                                    ILFree(pidl);
                                }
                            }
                            while (FindNextFile(hfind, &fd));
                            FindClose(hfind);
                        }
                    }

                    ResetWaitCursor();
                    break;
                }
#endif // 0

                case SFVIDM_FILE_RENAME:
                    // May need to add some more support here later, but...
                {
                    int iItemFocus;

RenameIt_NormalView:
                    iItemFocus = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED);
                    if (iItemFocus >= 0)
                    {
                        // Deselect all items...
                        UINT uState = ListView_GetItemState(_hwndListview, iItemFocus, LVIS_SELECTED);
                        int iNextItemSelect = ListView_GetNextItem(_hwndListview, iItemFocus, LVIS_SELECTED);
                        int iPrevItemSelect = ListView_GetNextItem(_hwndListview, -1, LVIS_SELECTED);
                        if (((iNextItemSelect >= 0) && (iNextItemSelect != iItemFocus))
                                || ((iPrevItemSelect >= 0) && (iPrevItemSelect != iItemFocus)))
                        {
                            // Deselect all items...
                            ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);
                            if (uState)
                                ListView_SetItemState(_hwndListview, iItemFocus, LVIS_SELECTED, LVIS_SELECTED);
                        }
                        ListView_EditLabel(_hwndListview, iItemFocus);
                    }
                    break;
                }

                default:
                    // BUGBUG: should not be any of these
                    //
                    if (!lParam)
                    {
#ifdef DEBUG
                        MessageBox(_hwndView, TEXT("This feature is not implemented yet"),
                               TEXT("UNDER CONSTRUCTION"), MB_OK);
                        TraceMsg(TF_ERROR, "command not processed in %s at %d (%x)",
                                    __FILE__, __LINE__, uID);
                        ASSERT(0);
#endif // DEBUG
                    }
                    return(1);
                }
            }  // End of switch statement.
        }  //if _HasNormalView view
    } // first switch for this-folder specific commands

    return(0);
}

LPITEMIDLIST CDefView::_ObjectExists(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlReal = NULL;

    SHGetRealIDL(_pshf, pidl, &pidlReal);
    return pidlReal;
}

void CDefView::_OnRename(LPCITEMIDLIST* ppidl)
{
    if (_pidlMonitor)
    {
        if (!ILIsParent(_pidlMonitor, ppidl[0], TRUE)) 
        {
            // move to this folder
            _OnFSNotify(SHCNE_CREATE, &ppidl[1]);
        } 
        else if (!ILIsParent(_pidlMonitor, ppidl[1], TRUE)) 
        {
            // move from this folder
            _OnFSNotify(SHCNE_DELETE, &ppidl[0]);
        } 
        else 
        {
            LPITEMIDLIST ppItems[2];

            // rename within this folder
            ppItems[0] = (LPITEMIDLIST)ILFindLastID(ppidl[0]);
            ppItems[1] = _ObjectExists(ILFindLastID(ppidl[1]));
            if (ppItems[1]) 
            {
                if (_UpdateObject(ppItems) == -1)
                    ILFree(ppItems[1]);
            }
        }
    }
}

//
//  SFVM_UPDATESTATUSBAR return values:
//
//  failure code = Callback did not do anything, we must do it all
//
//  Otherwise, the GetScode(hres) is a bitmask describing what the app
//  wants us to do.
//
//  0 - App wants us to do nothing (S_OK) - message handled completely
//  1 - App wants us to set the default text (but not initialize)
//
//  <other bits reserved for future use>

void DV_UpdateStatusBar(CDefView *pdsv, BOOL fInitialize)
{
    HRESULT hres = pdsv->CallCB(SFVM_UPDATESTATUSBAR, fInitialize, 0);

    if (FAILED(hres))
    {
        // Client wants us to do everything
        DV_DoDefaultStatusBar(pdsv, fInitialize);
    } else if (hres & SFVUSB_INITED)
    {
        // Client wants us to do text but not initialize
        DV_DoDefaultStatusBar(pdsv, FALSE);
    }
}


BOOL CDefView::_CanShowWebView()
{
    // Quattro Pro (QPW) is too stupid to know how SHChangeNotify works,
    // so when they want to refresh My Computer, they create an IShellView,
    // invoke its CreateViewWindow(), invoke its Refresh(), then DestroyWindow
    // the window and release the view.  The IShellBrowser they pass
    // to CreateViewWindow is allocated on the stack (!), and they expect
    // that their Release() be the last one.  Creating an async view keeps
    // the object alive, so when the view is complete, we try to talk to the
    // IShellBrowser and fault because it's already gone.
    //
    // The Zip Archives (from Aeco Systems) is another screwed up App.
    // They neither implement IPersistFolder2 (so we can't get their pidl) nor
    // set the pidl to the shellfolderviewcb object. They don't implement
    // IShellFolder2 either. Webview is practically useless for them.
   SHELLSTATE ss;

   SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_DESKTOPHTML | SSF_WEBVIEW, FALSE);

   return(!DV_CDB_IsCommonDialog(this) && !_IsDesktop() && ss.fWebView &&
        !(SHGetAppCompatFlags(ACF_OLDCREATEVIEWWND) & ACF_OLDCREATEVIEWWND) &&
        !(SHGetObjectCompatFlags(_pshf, NULL) & OBJCOMPATF_NO_WEBVIEW));
}

#define FSNDEBUG

//---------------------------------------------------------------------------
// Processes a WM_DSV_FSNOTIFY message
//
LRESULT CDefView::_OnFSNotify(LONG lNotification, LPCITEMIDLIST* ppidl)
{
    LPITEMIDLIST pidl;
    LPCITEMIDLIST pidlItem;

    //
    //  Note that renames between directories are changed to
    //  create/delete pairs by SHChangeNotify.
    //
#ifdef DEBUG
#ifdef FSNDEBUG
    TCHAR szPath[MAX_PATH];
    TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify, hwnd = %d  lEvent = %d", _hwndView, lNotification);

    switch (lNotification)
    {
    case SHCNE_RENAMEITEM:
    case SHCNE_RENAMEFOLDER:
        // two pidls
        SHGetPathFromIDList(ppidl[0], szPath);
        TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: hwnd %d, %s", _hwndView, szPath);
        SHGetPathFromIDList(ppidl[1], szPath);
        TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: hwnd %d, %s", _hwndView, szPath);
        break;

    case SHCNE_CREATE:
    case SHCNE_DELETE:
    case SHCNE_MKDIR:
    case SHCNE_RMDIR:
    case SHCNE_MEDIAINSERTED:
    case SHCNE_MEDIAREMOVED:
    case SHCNE_DRIVEREMOVED:
    case SHCNE_DRIVEADD:
    case SHCNE_NETSHARE:
    case SHCNE_NETUNSHARE:
    case SHCNE_ATTRIBUTES:
    case SHCNE_UPDATEDIR:
    case SHCNE_UPDATEITEM:
    case SHCNE_SERVERDISCONNECT:
    case SHCNE_DRIVEADDGUI:
    case SHCNE_EXTENDED_EVENT:
        // one pidl
        SHGetPathFromIDList(ppidl[0], szPath);
        TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: hwnd %d, %s", _hwndView, szPath);
        break;

    case SHCNE_UPDATEIMAGE:
        // DWORD wrapped inside a pidl
        TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: hwnd %d, %08x", _hwndView,
            ((LPSHChangeDWORDAsIDList)ppidl[0])->dwItem1);
        break;

    case SHCNE_ASSOCCHANGED:
        // No parameters
        break;
    }
#endif
#endif

    switch(lNotification) {

    case SHCNE_DRIVEADD:
    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        pidlItem = ILFindLastID(ppidl[0]);
        pidl = _ObjectExists(pidlItem);
        if (pidl)
        {
            // if the item is already in our listview, or if we fail to add it
            // then cleanup because we didnt' store the pidl.

            if (_FindItem(pidlItem, NULL, FALSE) != -1)
            {
                LPITEMIDLIST pp[2];
                pp[0] = (LPITEMIDLIST)pidlItem;
                pp[1] = pidl;
                if (_UpdateObject(pp) < 0) 
                    ILFree(pidl);   // we're bummed
            }
            else if (DefView_AddObject(this, pidl) == -1)
            {
                TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: Item already exists.");
                ILFree(pidl);
            }
            else
            {
                // Item got added OK, we'll need to save it's position.
                _bItemsMoved = TRUE;
            }
        }
        break;

    case SHCNE_DRIVEREMOVED:
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
        pidlItem = ILFindLastID(ppidl[0]);
        _RemoveObject((LPITEMIDLIST)pidlItem, FALSE);
        break;

    case SHCNE_RENAMEITEM:
    case SHCNE_RENAMEFOLDER:
        _OnRename(ppidl);
        break;

    case SHCNE_UPDATEIMAGE:
        //
        // the system image cache is changing
        //
        // ppidl[0] is a IDLIST of image indexs that have changed
        //

        if ( ppidl && ppidl[1])
        {
            // this event is generated instead of a normal UPDATEIMAGE so that we can handle the
            // cross process case....
            // handle the notification
            int iImage = SHHandleUpdateImage(ppidl[1]);
            if ( iImage != -1 )
            {
                DefView_UpdateImage( this, iImage );
            }
        }
        else if (ppidl && ppidl[0])
        {
            int iImage = *(int UNALIGNED *)((BYTE *)ppidl[0] + 2);
            DefView_UpdateImage(this, iImage);
        }
        break;

    case SHCNE_ASSOCCHANGED:
        // For this one we will call refresh as we may need to reextract
        // the icons and the like.  Later we can optimize this somewhat if
        // we can detect which ones changed and only update those.
        ReloadContent();
        break;

    case SHCNE_ATTRIBUTES:

        //
        // Also include SHCNE_ATTRIBUTE--the print folder uses this when
        // the details view of a printer changes, but not the icon view.
        // As an optimization, we can only do a SetItem in _UpdateObject
        // if we are in non-icon view, since the name and icon didn't
        // change.
        //
        if ((LVStyleFromView(this) & LVS_TYPEMASK) != LVS_REPORT)
        {
            //
            // Since we are not in report view, don't update the object.
            //
            // Since we won't call through to update object we may need to let automation know..
            // BUGBUG:: What happens later if we do change to report view, won't we show old attributes?
            CheckIfSelectedAndNotifyAutomation(ILFindLastID(ppidl[0]), -1);
            break;
        }

        //
        // Fall through to SHCNE_UPDATEITEM handling.
        //

    case SHCNE_MEDIAINSERTED:
    case SHCNE_MEDIAREMOVED:
    case SHCNE_NETUNSHARE:
    case SHCNE_NETSHARE:
    case SHCNE_UPDATEITEM:
        if (ppidl)
        {
            LPITEMIDLIST pp[2];
            
            pp[0] = ILFindLastID(ppidl[0]);
            pp[1] = _ObjectExists(pp[0]);
            if (pp[1]) 
            {
                if (_UpdateObject(pp) < 0)
                {
                    // something went wrong
                    DefView_Update(this);
                    ILFree(pp[1]);
                }
            }
            else    
            {
                // If we do not have any subobjects and the passed in pidl is the same as 
                // this views pidl then refresh all the items.

                LPITEMIDLIST pPidlView = _GetViewPidl();
                if (pPidlView)
                {
                    if (ILIsEqual(ppidl[0], pPidlView))
                        DefView_Update(this);

                    ILFree(pPidlView); 
                }
            }
        }
        else    // ppidl == NULL means update all items (re-enum them)
        {
            DefView_Update(this);
        }
        break;


    case SHCNE_FREESPACE:
        // BUGBUG: should only do this if the drive is ours
        // and we should change the bool to an enum
        if (_pidlMonitor) 
        {
            TCHAR szPath[MAX_PATH];
            if (SHGetPathFromIDList(_pidlMonitor, szPath)) 
            {
                DWORD dwChangedDrives = *(DWORD UNALIGNED *)((BYTE *)ppidl[0] + 2);
                int idDrive = PathGetDriveNumber(szPath);
                TraceMsg(TF_DEFVIEW, "Changed drives = %x", dwChangedDrives);
                if (idDrive != -1 &&
                    ((1 << idDrive) & dwChangedDrives )) {
                    DV_UpdateStatusBar(this, TRUE);
                }
            }
        }
        break;

    default:
        TraceMsg(TF_DEFVIEW, "DefView: unknown FSNotify %08lX, doing full update", lNotification);
        DefView_Update(this);
        break;
    }

    DV_UpdateStatusBar(this, FALSE);

    return 0;
}

BOOL CDefView::_GetItemPosition(LPCITEMIDLIST pidl, POINT *ppt)
{
    int i = _FindItem(pidl, NULL, FALSE);
    if (i != -1)
        return ListView_GetItemPosition(_hwndListview, i, ppt);
    return FALSE;
}



//---------------------------------------------------------------------------
// called when some of our objects get put on the clipboard
//
LRESULT DSV_OnSetClipboard(CDefView *pdsv, WPARAM idCmd)
{
    TraceMsg(TF_DEFVIEW, "DSV_OnSetClipboard");

    ASSERT((idCmd == DFM_CMD_MOVE) || (idCmd == DFM_CMD_COPY));

    if (idCmd == DFM_CMD_MOVE)  // move
    {
        //
        //  mark all selected items as being "cut"
        //
        int i = -1;
        while ((i = ListView_GetNextItem(pdsv->_hwndListview, i, LVIS_SELECTED)) != -1)
        {
            ListView_SetItemState(pdsv->_hwndListview, i, LVIS_CUT, LVIS_CUT);
            //ListView_RedrawItems(pdsv->_hwndListview, i, i+1);     // is there a better way?
            pdsv->_bHaveCutStuff = TRUE;
        }

        //
        // join the clipboard viewer chain so we will know when to
        // "uncut" our selected items.
        //
        if (pdsv->_bHaveCutStuff)
        {
            ASSERT(!pdsv->_bClipViewer);
            ASSERT(pdsv->_hwndNextViewer == NULL);

            pdsv->_hwndNextViewer = SetClipboardViewer(pdsv->_hwndView);
            pdsv->_bClipViewer = TRUE;
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
// called when the clipboard get changed, clear any items in the "cut" state
//
LRESULT DSV_OnClipboardChange(CDefView *pdsv)
{
    //
    //  if we dont have any cut stuff we dont care.
    //
    if (!pdsv->_bHaveCutStuff)
        return 0;

    ASSERT(pdsv->_bClipViewer);
    TraceMsg(TF_DEFVIEW, "DSV_OnDrawClipboard");

    pdsv->_RestoreAllGhostedFileView();
    pdsv->_bHaveCutStuff = FALSE;

    //
    // unhook from the clipboard viewer chain.
    //
    ChangeClipboardChain(pdsv->_hwndView, pdsv->_hwndNextViewer);
    pdsv->_bClipViewer = FALSE;
    pdsv->_hwndNextViewer = NULL;

    return 0;
}

//
// Note: this function returns the point in Listview Coordinate
// space.  So any hit testing done with this needs to be converted
// back to Client coordinate space...
BOOL DefView_GetDropPoint(CDefView *pdv, POINT *ppt)
{
    // Check whether we already have gotten the drop anchor (before any
    // menu processing)
    if (pdv->_bDropAnchor)
    {
        *ppt = pdv->_ptDrop;
        LVUtil_ClientToLV(pdv->_hwndListview, ppt);
    }
    else if (pdv->_bMouseMenu)
    {
        *ppt = pdv->_ptDragAnchor;
        return TRUE;
    }
    else
    {
        // We need the most up-to-date cursor information, since this
        // may be called during a drop, and the last time the current
        // thread called GetMessage was about 10 minutes ago
        GetCursorPos(ppt);
        LVUtil_ScreenToLV(pdv->_hwndListview, ppt);
    }

    return pdv->_bDropAnchor;
}


BOOL DefView_GetDragPoint(CDefView *pdv, POINT *ppt)
{
    BOOL fSource = pdv->_bDragSource || pdv->_bMouseMenu;
    if (fSource) {
        // if anchor from mouse activity
        *ppt = pdv->_ptDragAnchor;
    } else {
        // if anchor from keyboard activity...  use the focused item
        int i = ListView_GetNextItem(pdv->_hwndListview, -1, LVNI_FOCUSED);
        if (i != -1)
            ListView_GetItemPosition(pdv->_hwndListview, i, ppt);
    }
    return fSource;
}


void DV_PaintErrMsg(HWND hWnd, CDefView *pdsv)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rc;

    // if we're in an error state, make sure we're not in an extended view
    if (pdsv->m_cFrame.IsWebView())
    {
        pdsv->_SwitchToViewID((UINT)CSFVFrame::HIDEEXTVIEW, FALSE);
    }

    GetClientRect(hWnd, &rc);
#ifdef DEBUG
    TraceMsg(TF_DEFVIEW, "DV_PaintErrMsg is called (%x,%d)", pdsv->_hres, HRESULT_CODE(pdsv->_hres));
#endif
    DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT|BF_SOFT|BF_ADJUST|BF_MIDDLE);

    EndPaint(hWnd, &ps);
}

//
//  The default status bar looks like this:
//
//  No items selected:  "nn object(s)"              nn = total objects in folder
//  One item selected:  <InfoTip for selected item> if item supports InfoTip
//  Else:               "nn object(s) selected"     nn = num selected objects
//
//

void DV_DoDefaultStatusBar(CDefView *pdsv, BOOL fInitialize)
{
    if (pdsv->_psb)
    {
        // Some of the failure cases do not null hwnd...
        HWND hwndStatus=NULL;
        pdsv->_psb->GetControlWindow(FCW_STATUS, &hwndStatus);
        if (hwndStatus)
        {
            UINT uMsg;
            int nMsgParam;
            TCHAR szTemp[30];

            if (fInitialize)
            {
                int ciParts[] = {-1};
                SendMessage(hwndStatus, SB_SETPARTS, ARRAYSIZE(ciParts), (LPARAM)ciParts);
            }

            IQueryInfo *pqi;
            LPWSTR pwszTip = NULL;
            LPTSTR pszStatus = NULL;
            TCHAR szStatus[128];

            nMsgParam = ListView_GetSelectedCount(pdsv->_hwndListview);
            switch (nMsgParam)
            {
            case 0:
                // No objects selected; show total item count
                nMsgParam = ListView_GetItemCount(pdsv->_hwndListview);
                uMsg = IDS_FSSTATUSBASE;
                break;

            case 1:
                if (SUCCEEDED(DefView_GetUIObjectFromItem(pdsv, IID_IQueryInfo, (LPVOID*)&pqi, SVGIO_SELECTION)))
                {
                    pqi->GetInfoTip(0, &pwszTip);
                    pqi->Release();
                    if (pwszTip)
                    {
                        // Infotips often contain \t\r\n characters, so
                        // map control characters to spaces.  Also collapse
                        // consecutive spaces to make us look less stupid.
                        LPWSTR pwszDst, pwszSrc;

                        // BUGBUG NT342538: it would be nice to convert \t\r\n to a comma for readability
                        //
                        // Since we are unicode, we don't have to worry about DBCS.
                        for (pwszDst = pwszSrc = pwszTip; *pwszSrc; pwszSrc++) {
                            if ((UINT)*pwszSrc <= (UINT)L' ') {
                                if (pwszDst == pwszTip || pwszDst[-1] != L' ') {
                                    *pwszDst++ = L' ';
                                }
                            } else {
                                *pwszDst++ = *pwszSrc;
                            }
                        }
                        *pwszDst = L'\0';
                        // GetInfoTip can return a Null String too.
                        if (L'\0' == *pwszTip)
                        {
                            SHFree(pwszTip);
                            pwszTip = NULL;
                            uMsg = IDS_FSSTATUSSELECTED;
                        }
                        break;                        
                    }                                       
                }
                // Object doesn't support InfoTip; do it the old way

                // FALL THROUGH
            default:
                uMsg = IDS_FSSTATUSSELECTED;
                break;
            }

            if (pwszTip)
            {
                if (g_bBiDiW95Loc)
                {
                    szStatus[0] = szStatus[1] = TEXT('\t');
                    SHUnicodeToTChar(pwszTip, &szStatus[2], ARRAYSIZE(szStatus) - 2);
                    SendMessage(hwndStatus, SB_SETTEXT, (WPARAM)SBT_RTLREADING, (LPARAM)szStatus);
                }
                else
                {
                    SendMessage(hwndStatus, SB_SETTEXTW, (WPARAM)0, (LPARAM)pwszTip);
                }

                SHFree(pwszTip);
            }
            else
            {
                pszStatus = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(uMsg),
                                 AddCommas(nMsgParam, szTemp));
                if (pszStatus)
                {
                    if (g_bBiDiW95Loc)
                    {
                        szStatus[0] = szStatus[1] = TEXT('\t');
                        StrCpyN(&szStatus[2], pszStatus, ARRAYSIZE(szStatus)-2);
                        SendMessage(hwndStatus, SB_SETTEXT, (WPARAM)SBT_RTLREADING, (LPARAM)szStatus);
                    }
                    else
                    {
                        SendMessage(hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)pszStatus);
                    }

                    LocalFree(pszStatus);
                }
            }
        }
    }
}

#define DV_IDTIMER_BUFFERED_REFRESH  3
#define BUFFERED_REFRESH_TIMEOUT     5000   //5 seconds.

void DefView_Desktop_OnWinIniChange(CDefView *pdsv, WPARAM wParam, LPCTSTR lpszSection)
{
    if (lpszSection) {
        if (!lstrcmpi(lpszSection, TEXT("ToggleDesktop")))
            pdsv->Command(NULL, SFVIDM_DESKTOPHTML_WEBCONTENT, 0);
        else
        {
            if (!lstrcmpi(lpszSection, TEXT("RefreshDesktop")))
            {
                if (FAILED(pdsv->Refresh()))
                {
                    SHELLSTATE  ss;

                    //Refresh failed because the new template didn't exist
                    //Toggle the Registry settings back to Icons-only mode!
                    ss.fDesktopHTML = FALSE;
                    SHGetSetSettings(&ss, SSF_DESKTOPHTML, TRUE);  // Write back the new
                }
            }
            else
            {
                if(!lstrcmpi(lpszSection, TEXT("BufferedRefresh")))
                {
                    //See if we have already started a timer to refresh
                    if(!pdsv->_fRefreshBuffered)
                    {
                        TraceMsg(TF_DEFVIEW, "A Buffered refresh starts the timer");
                        SetTimer(pdsv->_hwndView, DV_IDTIMER_BUFFERED_REFRESH, BUFFERED_REFRESH_TIMEOUT, NULL);
                        pdsv->_fRefreshBuffered = TRUE;
                    }
                    else //If refresh is already buffered, don't do anything!
                    {
                        TraceMsg(TF_DEFVIEW, "A buffered refresh occured while another is pending");
                    }
                }
                else
                {
                    if (wParam == SPI_SETDESKWALLPAPER || wParam == SPI_SETDESKPATTERN)
                    {
                        DSV_SetFolderColors(pdsv);
                        InvalidateRect(pdsv->_hwndListview, NULL, TRUE);
                    }
                }
            }
        }
    }
    else
    {
        if (wParam == SPI_SETDESKWALLPAPER || wParam == SPI_SETDESKPATTERN) {
            DSV_SetFolderColors(pdsv);
            InvalidateRect(pdsv->_hwndListview, NULL, TRUE);
        }
    }
}

void DefView_OnWinIniChange(CDefView *pdsv, WPARAM wParam, LPCTSTR lpszSection)
{
    if ((wParam == SPI_GETICONTITLELOGFONT) ||
        ((wParam == 0) && lpszSection && !lstrcmpi(lpszSection, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\IconUnderline"))))
    {
        UpdateUnderlines(pdsv);
    }

    if (!wParam || (lpszSection && !lstrcmpi(lpszSection, TEXT("intl"))))
    {
        // has the time format changed while we're in details mode?
        if (FVM_DETAILS == pdsv->_fs.ViewMode)
        {
            int     iItem, iItemCount;
            UINT    uiRealColumn;

            InvalidateRect(pdsv->_hwndListview, NULL, TRUE);

            // 99/04/13 #320903 vtan: If the date format has changed then iterate
            // the entire list looking for extended columns of type date and
            // resetting them to LPSTR_TEXTCALLBACK effectively dumping the cache.

            // For performance improvement it's possible to collect an array of
            // visible columns and reset that array. It will still involve TWO
            // for loops.

            iItemCount = ListView_GetItemCount(pdsv->_hwndListview);
            for (iItem = 0; iItem < iItemCount; ++iItem)
            {
                for (uiRealColumn = 0; uiRealColumn < pdsv->m_cColItems; ++uiRealColumn)
                {
                    DWORD   dwFlags;

                    dwFlags = pdsv->m_pColItems[uiRealColumn].csFlags;
                    if (((dwFlags & SHCOLSTATE_EXTENDED) != 0) && 
                        ((dwFlags & SHCOLSTATE_TYPEMASK) == SHCOLSTATE_TYPE_DATE))
                    {
                        UINT    uiVisibleColumn;

                        if (SUCCEEDED(pdsv->MapRealToVisibleColumn(uiRealColumn, &uiVisibleColumn)))
                        {
                            ListView_SetItemText(pdsv->_hwndListview, iItem, uiVisibleColumn, LPSTR_TEXTCALLBACK);
                        }
                    }
                }
            }
        }
    }

    //
    // we may need to rebuild the icon cache.
    //
    if (wParam == SPI_SETICONMETRICS ||
        wParam == SPI_SETNONCLIENTMETRICS)
    {
        HIMAGELIST himlLarge, himlSmall;

        Shell_GetImageLists(&himlLarge, &himlSmall);
        ListView_SetImageList(pdsv->_hwndListview, himlLarge, LVSIL_NORMAL);
        ListView_SetImageList(pdsv->_hwndListview, himlSmall, LVSIL_SMALL);
    }

    //
    // we need to invalidate the cursor cache
    //
    if (wParam == SPI_SETCURSORS) {
        DAD_InvalidateCursors();
    }

    if (pdsv->_IsDesktop())
    {
        DefView_Desktop_OnWinIniChange(pdsv, wParam, lpszSection);
    }
}

void SetDefaultViewSettings(CDefView *pdsv)
{
    // only do this if we've actually shown the view...
    // (ie, there's no _hwndStatic)
    // and we're not the desktop
    // and we're not an exstended view
    if (!pdsv->_hwndStatic && !pdsv->_IsDesktop() && pdsv->_HasNormalView())
    {
        HWND hwndTree;
        pdsv->_psb->GetControlWindow(FCW_TREE, &hwndTree);

        // only do this for non-explorers
        if (!hwndTree) {
            SHELLSTATE ss;

            ss.lParamSort = pdsv->_dvState.lParamSort;
            ss.iSortDirection = pdsv->_dvState.iDirection;
            SHGetSetSettings(&ss, SSF_SORTCOLUMNS, TRUE);
        }
    }
}

HWND CDefView::GetChildViewWindow()
{
    if (m_cFrame.IsWebView())
        return m_cFrame.GetExtendedViewWindow();

    return _hwndListview;
}

void DefView_SetFocus(CDefView *pdsv)
{
    // if it's a combined view then we need to give focus to listview
    if (!pdsv->_fCombinedView && pdsv->m_cFrame.IsWebView())
    {
        pdsv->_OnViewWindowActive();

        if (pdsv->m_cFrame.m_pOleObj)
        {
            MSG msg = {pdsv->_hwndView, WM_KEYDOWN, VK_TAB, 0xf0001};

            // HACKHACK!!! MUST set state here! idealy shbrowse should call
            // UIActivate on the view but that breaks dochost stuff.
            // if we did not set the state here, trident would call
            // CSFVSite::ActivateMe that would not forward the call to obj::UIActivate
            // and therefore nothing would get focus (actually trident would have it
            // but it would not be visible). Note that this behavior happens only
            // second time around, i.e. on init UIActivate is called and everything
            // works fine, but if we tab from address bar onto the view, that's when
            // the stuff gets broken.
            pdsv->OnActivate(SVUIA_ACTIVATE_FOCUS);
            pdsv->m_cFrame._UIActivateIO(TRUE, &msg);
        }
    }
    else if (pdsv->_HasNormalView())
    {
        HWND hwnd = NULL;
        pdsv->CallCB(SFVM_SETFOCUS, 0, 0);
        if (pdsv->m_cFrame.IsSFVExtension())
            hwnd = pdsv->m_cFrame.GetExtendedViewWindow();
        else
            hwnd = pdsv->_hwndListview;
        if (hwnd)
            SetFocus(hwnd);
        if(!pdsv->_IsDesktop())
        {
            pdsv->m_cFrame.m_uState = SVUIA_ACTIVATE_FOCUS;
        }
    }
}

LRESULT CALLBACK DefView_WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    CDefView *pdsv = (CDefView *)GetWindowLongPtr(hWnd, 0);
    LRESULT l;
    DWORD dwID;

    if (iMessage == WM_CREATE) 
    {
        pdsv = (CDefView *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        return pdsv->_OnCreate(hWnd);
    }

    if (pdsv == NULL)
    {
        if (iMessage != WM_NCCALCSIZE && iMessage != WM_NCCREATE)
        {
            ASSERT(0);  // we are not supposed to hit this assert
        }
        goto DoDefWndProc;
    }

    switch (iMessage)
    {
    // IShellBrowser forwards these to the IShellView. In the case
    // of our IShellView extended views, we need to forward them
    // down too. Dochost also forwards them down to the IOleObject,
    // so we should do it too...
    //
    case WM_SYSCOLORCHANGE:
        {
            HDITEM hdi = {HDI_FORMAT, 0, NULL, NULL, 0, 0, 0, 0, 0};
            HWND hwndHead = ListView_GetHeader(pdsv->_hwndListview);

            // We only want to update the sort arrows if they are already present.
            if (hwndHead)
            {
                Header_GetItem(hwndHead, pdsv->_dvState.lParamSort, &hdi);
                if (hdi.fmt & HDF_BITMAP)
                    pdsv->_SetSortArrows();
            }
            // fall through
        }
    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    case WM_WININICHANGE:
    case WM_FONTCHANGE:
        if (pdsv->m_cFrame.IsWebView())
        {
            HWND hwndExt = pdsv->m_cFrame.GetExtendedViewWindow();
            if (hwndExt)
            {
                SendMessage(hwndExt, iMessage, wParam, lParam);
            }
        }
        break;
    }

    switch (iMessage) {

    case WM_DESTROY:
        if (GetKeyState(VK_CONTROL) < 0)
            SetDefaultViewSettings(pdsv);

        // since we're no longer flushing cached context menus
        // on WM_DSV_MENUTERM (because now both Edit and File use different ones
        // and they flush old ones before they create new ones)
        // we need to flush the context menus hanging around before we kill defview
        ATOMICRELEASE(pdsv->_pcmSel);
        ATOMICRELEASE(pdsv->_pcmBackground);

        pdsv->EmptyBkgrndThread( TRUE );

        // Depending on when it is closed we may have an outstanding post
        // to us about the rest of the fill data which we should try to
        // process in order to keep from leaking stuff...

        // logically hWnd == pdsv->_hwndView, but we already zeroed
        // pdsv->_hwndView so use hWnd

        DefView_CheckForFillDoneOnDestroy(hWnd);

        //
        //  remove ourself as a clipboard viewer
        //
        if (pdsv->_bClipViewer)
        {
            ChangeClipboardChain(hWnd, pdsv->_hwndNextViewer);
            pdsv->_bClipViewer = FALSE;
            pdsv->_hwndNextViewer = NULL;
        }

        if (pdsv->_uRegister)
        {
            ULONG uRegister = pdsv->_uRegister;
            pdsv->_uRegister = 0;
            SHChangeNotifyDeregister(uRegister);
        }

        ATOMICRELEASE(pdsv->_psd);
        ATOMICRELEASE(pdsv->_pdtgtBack);

        ASSERT(pdsv->_pdtobjHdrop == NULL);      // pdsv should not still be around

        if (pdsv->_hwndListview) {
            EnableCombinedView(pdsv, FALSE);

            if (pdsv->_bRegisteredDragDrop)
                RevokeDragDrop(pdsv->_hwndListview);
        }

        // Release any automation object we may be holding onto, simply call our own
        // SetAutomationObject with NULL
        pdsv->SetAutomationObject(NULL);

        break;

    case WM_NCDESTROY:

        pdsv->_hwndView = NULL;

        //
        // Now we can release it.
        //
        ATOMICRELEASE(pdsv);

        SetWindowLongPtr(hWnd, 0, 0);

        //
        //  get rid of extra junk in the icon cache
        //
        _IconCacheFlush(FALSE);

        break;

    case WM_ENABLE:
        pdsv->_fDisabled = !wParam;
        break;

    case WM_ERASEBKGND:
        {
            COLORREF cr = ListView_GetBkColor(pdsv->_hwndListview);
            if (cr == CLR_NONE)
                return SendMessage(pdsv->_hwndMain, iMessage, wParam, lParam);
            
            //Turning On EraseBkgnd. This is required so as to avoid the
            //painting issue - when the listview is not visible and  
            //invalidation occurs.

            HBRUSH hbr = CreateSolidBrush(cr);
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect((HDC)wParam, &rc, hbr);
            DeleteObject(hbr);
        }
        // We want to reduce flash
        return 1;

    case WM_PAINT:
        if (pdsv->_fEnumFailed)
            DV_PaintErrMsg(hWnd, pdsv);
        else
            goto DoDefWndProc;
        break;

    case WM_LBUTTONUP:
        if (pdsv->_fEnumFailed)
            PostMessage(hWnd, WM_KEYDOWN, (WPARAM)VK_F5, 0);
        else
            goto DoDefWndProc;
        break;

    case WM_SETFOCUS:
        if (pdsv->_hwndView)    // Ignore if we are destroying _hwndView.
        {
            DefView_SetFocus(pdsv);
        }
        break;

    case WM_MOUSEACTIVATE:
        //
        // this keeps our window from coming to the front on button down
        // instead, we activate the window on the up click
        //
        if (LOWORD(lParam) != HTCLIENT)
            goto DoDefWndProc;
        LV_HITTESTINFO lvhti;

        GetCursorPos(&lvhti.pt);
        ScreenToClient(pdsv->_hwndListview, &lvhti.pt);
        ListView_HitTest(pdsv->_hwndListview, &lvhti);
        if (lvhti.flags & LVHT_ONITEM)
            return MA_NOACTIVATE;
        else
            return MA_ACTIVATE;

    case WM_ACTIVATE:
        // force update on inactive to not ruin save bits
        if (wParam == WA_INACTIVE)
            UpdateWindow(pdsv->_hwndListview);
        // if active view created, call active object to allow it to visualize activation.
        if (pdsv->m_cFrame.m_pActive)        
            (pdsv->m_cFrame.m_pActive)->OnFrameWindowActivate((BOOL)wParam);
        break;

    case WM_SIZE:
         return pdsv->WndSize(hWnd);

    case WM_NOTIFY:
    {
#ifdef DEBUG
        // DefView_OnNotify sometimes destroys the pnm, so we need to save
        // the code while we can.  (E.g., common dialog single-click activate.
        // LVN_ITEMACTIVATE causes us to dismiss the common dialog, which
        // does a pdsv->DestroyViewWindow, which destroys the ListView
        // which destroys the NMHDR!)
        UINT code = ((NMHDR *)lParam)->code;
#endif
        pdsv->AddRef();             // just in case
        DefView_StartNotify(pdsv, code);
        l = pdsv->_OnNotify((NMHDR *)lParam);
        DefView_StopNotify(pdsv, code);
        pdsv->Release();            // release
        return l;
    }

    case WM_CONTEXTMENU:
        TraceMsg(TF_DEFVIEW, "GOT WM_CONTEXTMENU %d %d !!!!!!!!!", wParam, lParam);
        if (!pdsv->_fDisabled)
        {
            if (lParam != (LPARAM) -1)
            {
                pdsv->_bMouseMenu = TRUE;
                pdsv->_ptDragAnchor.x = GET_X_LPARAM(lParam);
                pdsv->_ptDragAnchor.y = GET_Y_LPARAM(lParam);
                LVUtil_ScreenToLV(pdsv->_hwndListview, &pdsv->_ptDragAnchor);
            }
            // Note: in deview inside a defview we can have problems of the
            // parent destroying us when we change views, so we better addref/release
            // around this...
            pdsv->AddRef();
            pdsv->_bContextMenuMode = TRUE;
            pdsv->ContextMenu((DWORD) lParam);
            pdsv->_bContextMenuMode = FALSE;
            if (lParam != (DWORD)-1) {
                pdsv->_bMouseMenu = FALSE;
            }
            ATOMICRELEASE(pdsv);
        }
        break;

    case WM_COMMAND:
        return pdsv->Command(NULL, wParam, lParam);

    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
    case WM_QUERYDROPOBJECT:
    case WM_DROPOBJECT:
    case WM_DROPFILES:
        return pdsv->OldDragMsgs(iMessage, wParam, (const DROPSTRUCT *)lParam);

    case WM_DSV_DISABLEACTIVEDESKTOP:
        DisableActiveDesktop();
        break;

    case WM_DSV_BACKGROUNDENUMDONE:
        pdsv->_PostEnumDoneMessage();
        pdsv->CallCB(SFVM_BACKGROUNDENUMDONE, 0, 0);
        break;

    case WM_DSV_FSNOTIFY:
        {
            LPITEMIDLIST *ppidl;
            LONG lEvent;

            TIMESTART(pdsv->_FSNotify);

            LPSHChangeNotificationLock pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
            if (pshcnl)
            {
                if (pdsv->_fDisabled ||
                    (pdsv->CallCB(SFVM_FSNOTIFY, (WPARAM)ppidl, (LPARAM)lEvent) == S_FALSE))
                {
                    lParam = 0;
                }
                else
                {
                    IShellChangeNotify *pscn;
                    if (pdsv->m_cFrame.IsSFVExtension() &&
                        SUCCEEDED(pdsv->m_cFrame.m_pActiveSFV->QueryInterface(IID_IShellChangeNotify, (void **)&pscn)))
                    {
                        lParam = pscn->OnChange(lEvent, ppidl[0], ppidl[1]);
                        pscn->Release();
                    }
                    lParam = pdsv->_OnFSNotify(lEvent, (LPCITEMIDLIST*)ppidl);
                }
                SHChangeNotification_Unlock(pshcnl);
            }
            TIMESTOP(pdsv->_FSNotify);
        }
        return lParam;

    case WM_DSV_DESTROYSTATIC:
        pdsv->FillDone((HDPA)lParam, pdsv->_pSaveHeader, pdsv->_uSaveHeaderLen, (BOOL)wParam, FALSE);
        break;

    //
    //  the background thread will post this message to us
    //  when it has finished extracting a icon in the background.
    //
    //      wParam is 0
    //      lParam contains PIDL, iIconIndex, and an "already used" flag
    //
    case WM_DSV_UPDATEICON:
        pdsv->_UpdateIcon((CDVGetIconTask *)lParam);
        break;

    case WM_DSV_UPDATECOLDATA:
        pdsv->_UpdateColData((CBackgroundColInfo*)lParam);
        break;

    case WM_DSV_UPDATEOVERLAY:
        pdsv->_UpdateOverlay((int)wParam, (int)lParam);
        break;

    case WM_DSV_SHOWDRAGIMAGE:
        return DAD_ShowDragImage((BOOL)lParam);

    case WM_DSV_DESKTOPCONTEXTMENU:
        {
            HMENU hmenu;
            if (hmenu = CDesktop_GetActiveDesktopMenu())
            {
                int idCmd;
                TPMPARAMS tpmp;

                tpmp.cbSize = sizeof(tpmp);
                GetWindowRect((HWND)wParam, &tpmp.rcExclude);

                idCmd = TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_NONOTIFY,
                               GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hWnd, &tpmp);
                DestroyMenu(hmenu);

                if (idCmd)
                    pdsv->Command(NULL, idCmd, 0);
            }
        }
        break;

    case GET_WM_CTLCOLOR_MSG(CTLCOLOR_STATIC):
        SetBkColor(GET_WM_CTLCOLOR_HDC(wParam, lParam, iMessage),
                GetSysColor(COLOR_WINDOW));
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);

    case WM_DRAWCLIPBOARD:
        if (pdsv->_hwndNextViewer != NULL)
            SendMessage(pdsv->_hwndNextViewer, iMessage, wParam, lParam);

        if (pdsv->_bClipViewer)
            return DSV_OnClipboardChange(pdsv);

        break;

    case WM_CHANGECBCHAIN:
        if ((HWND)wParam == pdsv->_hwndNextViewer)
        {
            pdsv->_hwndNextViewer = (HWND)lParam;
            return TRUE;
        }

        if (pdsv->_hwndNextViewer != NULL)
            return SendMessage(pdsv->_hwndNextViewer, iMessage, wParam, lParam);
        break;


    case WM_WININICHANGE:
        DefView_OnWinIniChange(pdsv, wParam, (LPCTSTR)lParam);
        SendMessage(pdsv->_hwndListview, iMessage, wParam, lParam);
        break;

    case WM_SHELLNOTIFY:
#define SHELLNOTIFY_SETDESKWALLPAPER 0x0004
        if (wParam == SHELLNOTIFY_SETDESKWALLPAPER) {
            if (pdsv->_IsDesktop()) {
                pdsv->_fHasDeskWallPaper = (lParam != 0);
                DSV_SetFolderColors(pdsv);
                InvalidateRect(pdsv->_hwndListview, NULL, TRUE);
            }
        }
        break;

    case WM_INITMENU:
        DefView_OnInitMenu(pdsv);
        break;

    case WM_INITMENUPOPUP:
        dwID = GetMenuItemID((HMENU)wParam,LOWORD(lParam));
        if (pdsv->OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam)))
            goto RelayToCM;
        break;

    case WM_DSV_MENUTERM:
        // HACKHACK: After the menu goes away, this command is posted to the
        // window, but note that any WM_DSV_MENUTERM from the menu would still
        // be in the queue, so we need to post another WM_DSV_MENUTERM to delete
        // the context menu.
        if (!lParam)
            PostMessage(hWnd, WM_DSV_MENUTERM, 0, 1);
        else
            pdsv->_OnMenuTermination();
        break;

    case WM_EXITMENULOOP:
        // Background: as the user navigates across the menuband, WM_EXITMENULOOP
        // is sent for each menu that closes, and WM_INITMENUPOPUP is sent for
        // the new menu that opens.
        //
        // Posting the private WM_DSV_MENUTERM puts a wrench in the gears, because
        // it can cause the cached _pcmSel to be freed *after* WM_INITMENUPOPUP
        // had initialized it for the new menu that is popping up.  So be sure
        // to post this message only if we have a cached psmSel to really cleanup.
        // Otherwise, don't bother.

        if (pdsv->_pcmSel || pdsv->_pcmBackground)
            PostMessage(hWnd, WM_DSV_MENUTERM, 0, 0);
        break;

    case WM_TIMER:
        ASSERT(hWnd == pdsv->_hwndView);
        KillTimer(hWnd, (UINT) wParam);

        if (DV_IDTIMER == wParam)
        {
            if (pdsv->_hwndStatic) 
            {
                WCHAR szName[128];
                HINSTANCE hinst;

                if (S_OK != pdsv->CallCB(SFVM_GETANIMATION, (WPARAM)&hinst, (LPARAM)szName))
                {
                    hinst = g_hinst;
                    StrCpyW(szName, L"#150");
                }

                // Animate_OpenEx() except we want the W version always
                SendMessage(pdsv->_hwndStatic, ACM_OPENW, (WPARAM)hinst, (LPARAM)szName);
            }
        }
        else if (DV_IDTIMER_BUFFERED_REFRESH == wParam)
        {
            if (pdsv->_fRefreshBuffered)
            {
                pdsv->_fRefreshBuffered = FALSE;
                PostMessage(pdsv->_hwndView, WM_KEYDOWN, (WPARAM)VK_F5, 0);
                TraceMsg(TF_DEFVIEW, "Buffered Refresh timer causes actual refresh");
            }
        }
        else if (DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE == wParam)
        {
            pdsv->_bAutoSelChangeTimerSet = FALSE;
            pdsv->NotifyAutomation(DISPID_SELECTIONCHANGED);
        }
        else
        {
            ASSERT(DV_IDTIMER_UPDATEPENDING == wParam);
            pdsv->_bUpdatePendingPending = FALSE;
        }
        break;

    case WM_SETCURSOR:
        if (pdsv->_hwndStatic) {
            //TraceMsg(TF_DEFVIEW, "########## SET WAIT CURSOR WM_SETCURSOR %d", pfc->iWaitCount);
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            return TRUE;
        } else
            goto DoDefWndProc;


    case WM_DRAWITEM:
        #define lpdis ((LPDRAWITEMSTRUCT)lParam)
        dwID = lpdis->itemID;

        if (lpdis->CtlType != ODT_MENU)
            return 0;
        if (InRange(lpdis->itemID, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && pdsv->HasCB())
        {
            pdsv->CallCB(SFVM_DRAWITEM, SFVIDM_CLIENT_FIRST, lParam);
            return 1;
        }
        else
            goto RelayToCM;
        #undef lpdis

    case WM_MEASUREITEM:
        #define lpmis ((LPMEASUREITEMSTRUCT)lParam)
        dwID = lpmis->itemID;

        if (lpmis->CtlType != ODT_MENU)
            return 0;
        if (InRange(lpmis->itemID, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && pdsv->HasCB())
        {
            pdsv->CallCB(SFVM_MEASUREITEM, SFVIDM_CLIENT_FIRST, lParam);
            return 1;
        }
RelayToCM:
        IContextMenu2 *pcm2;
        if (InRange(dwID, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST))
        {
            if (pdsv->_pcmBackground)
            {
                if (SUCCEEDED(pdsv->_pcmBackground->QueryInterface(IID_IContextMenu2, (void **)&pcm2)))
                {
                    pcm2->HandleMenuMsg(iMessage, wParam, lParam);
                    ATOMICRELEASE(pcm2);
                }
            }
        }
        else if (pdsv->_pcmSel || pdsv->_pcmBackground)
        {
            if (SUCCEEDED((pdsv->_pcmSel ? pdsv->_pcmSel : pdsv->_pcmBackground)
                ->QueryInterface(IID_IContextMenu2, (void **)&pcm2)))
            {
                pcm2->HandleMenuMsg(iMessage, wParam, lParam);
                ATOMICRELEASE(pcm2);
            }
        }
        return 0;
    case WM_MENUCHAR:
    case WM_NEXTMENU:
        {
            IContextMenu3 *pcm3;
            LRESULT ic3Result;
            HRESULT hResic3 = E_FAIL;
            if (pdsv->_pcmSel)
            {
                if (SUCCEEDED(pdsv->_pcmSel->QueryInterface(IID_IContextMenu3, (void **)&pcm3)))
                {
                    hResic3 = pcm3->HandleMenuMsg2(iMessage, wParam, lParam,&ic3Result);
                    ATOMICRELEASE(pcm3);
                }
                if (hResic3 == S_OK)
                    return ic3Result;
            }
            //Forward these messages to IContextMenu3 on the DefCm.
            if (pdsv->_pcmBackground)
            {
                if (SUCCEEDED(pdsv->_pcmBackground->QueryInterface(IID_IContextMenu3, (void **)&pcm3)))
                {
                    hResic3 = pcm3->HandleMenuMsg2(iMessage, wParam, lParam,&ic3Result);
                    ATOMICRELEASE(pcm3);
                }
                if (hResic3 == S_OK)
                    return ic3Result;
            }

            return MAKELONG(0,MNC_IGNORE);

            break;
        }



    // there are two possible ways to put help texts in the
    // status bar, (1) processing WM_MENUSELECT or (2) handling MenuHelp
    // messages. (1) is compatible with OLE, but (2) is required anyway
    // for tooltips.
    //
    case WM_MENUSELECT:
        DefView_OnMenuSelect(pdsv, GET_WM_MENUSELECT_CMD(wParam, lParam), GET_WM_MENUSELECT_FLAGS(wParam, lParam), GET_WM_MENUSELECT_HMENU(wParam, lParam));
        break;

    case WM_SYSCOLORCHANGE:
        DSV_SetFolderColors(pdsv);
        SendMessage(pdsv->_hwndListview, iMessage, wParam, lParam);
        break;

    // BUGBUG: some of these are dead

    case SVM_SELECTITEM:
        pdsv->SelectItem((LPCITEMIDLIST)lParam, (int) wParam);
        break;

    case SVM_MOVESELECTEDITEMS:
        DefView_MoveSelectedItems(pdsv, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), FALSE);
        break;

    case SVM_GETANCHORPOINT:
        if (wParam)
            return DefView_GetDragPoint(pdsv, (POINT*)lParam);
        else
            return DefView_GetDropPoint(pdsv, (POINT*)lParam);


    case SVM_GETITEMPOSITION:
        return pdsv->_GetItemPosition((LPCITEMIDLIST)wParam, (POINT*)lParam);

    case SVM_SELECTANDPOSITIONITEM:
    {
        UINT i;
        SFM_SAP * psap = (SFM_SAP*)lParam;
        for (i = 0; i < wParam; psap++, i++)
            pdsv->SelectAndPositionItem(psap->pidl, psap->uSelectFlags, psap->fMove ? &psap->pt : NULL);
        break;
    }

    case WM_PALETTECHANGED:
    case WM_QUERYNEWPALETTE:
    {
        HWND hwndT = pdsv->GetChildViewWindow();
        if (!hwndT)
            goto DoDefWndProc;

        return SendMessage(hwndT, iMessage, wParam, lParam);
    }

    case WM_DSV_REARRANGELISTVIEW:
        pdsv->_ShowAndActivate();
        break;

    case WM_DSV_SENDSELECTIONCHANGED:
        pdsv->_fSelectionChangePending = FALSE;
        DV_UpdateStatusBar(pdsv, FALSE);
        pdsv->_EnableDisableTBButtons();

        // Send out the selection changed notification to the automation after a delay.
        // Set the timer for the delay here...
        pdsv->_bAutoSelChangeTimerSet = TRUE;
        SetTimer(hWnd, DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE, NOTIFY_AUTOMATION_SELCHANGE_TIMEOUT, NULL);
        break;

    case WM_DSV_DESKHTML_CHANGES:
        if (pdsv->_IsDesktop())
        {
            IADesktopP2     *piadp2;

            if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_IADesktopP2, (void **)&piadp2)))
            {
                IActiveDesktopP     *piadpp;

                TraceMsg(TF_DEFVIEW, "WM_DSV_DESKHTML_CHANGES");

//  98/11/23 #254482 vtan: When making changes using dynamic
//  HTML don't forget to update the "desktop.htt" file so
//  that it's in sync with the registry BEFORE using DHTML.

                if (SUCCEEDED(piadp2->QueryInterface(IID_IActiveDesktopP, reinterpret_cast<void**>(&piadpp))))
                {
                    piadpp->EnsureUpdateHTML();     // ignore result
                    piadpp->Release();
                }
                piadp2->MakeDynamicChanges(pdsv->m_cFrame.m_pOleObj);
                piadp2->Release();
            }
        }
        break;

    case WM_DSV_FILELISTENUMDONE:
        pdsv->NotifyAutomation(DISPID_FILELISTENUMDONE);
        break;

    default:
        // Handle the magellan mousewheel message.
        if (iMessage == g_msgMSWheel)
        {
            HWND hwndT = pdsv->GetChildViewWindow();

            if (!hwndT)
                return 1;

            return SendMessage(hwndT, iMessage, wParam, lParam);
        }

DoDefWndProc:
        return DefWindowProc(hWnd, iMessage, wParam, lParam);
    }

    return 0;
}


BOOL DefView_RegisterWindow(void)
{
    WNDCLASS wc;

    if (!GetClassInfo(HINST_THISDLL, c_szDefViewClass, &wc))
    {
        // don't want vredraw and hredraw because that causes horrible
        // flicker expecially with full drag
        wc.style         = CS_PARENTDC;
        wc.lpfnWndProc   = DefView_WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = SIZEOF(CDefView *);
        wc.hInstance     = HINST_THISDLL;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = c_szDefViewClass;

        return RegisterClass(&wc);
    }
    return TRUE;
}


CDefView::~CDefView()
{
    _uState = SVUIA_DEACTIVATE;
    DV_FlushCachedMenu(this);

    DebugMsg(TF_LIFE, TEXT("dtor CDefView %x"), this);

    //
    // Just in case, there is a left over.
    //
    _dvdt.LeaveAndReleaseData(this);

    //
    // We need to give it a chance to clean up.
    //
    CallCB(SFVM_PRERELEASE, 0, 0);

    DestroyViewWindow();

    if (_pSaveHeader)
        LocalFree(_pSaveHeader);

    //
    // We should release _psb after _pshf (for docfindx)
    //
    ATOMICRELEASE(_pshf);
    ATOMICRELEASE(_pshf2);
    ATOMICRELEASE(_psi);
    ATOMICRELEASE(_psio);
    ATOMICRELEASE(_pcdb);
    ATOMICRELEASE(_psb);
    ATOMICRELEASE(_psd);
    ATOMICRELEASE(_pcmSel);

    //  NOTE we dont release psvOuter
    //  it has a ref on us
    
    if (m_pColItems)
        LocalFree(m_pColItems);
    if (_pcp)
        delete _pcp;

    if (_pbtn)
        LocalFree(_pbtn);

    //
    // Cleanup _dvdt
    //
    _dvdt.ReleaseDataObject();
    _dvdt.ReleaseCurrentDropTarget();

    _ClearSelectList();

    ATOMICRELEASE(m_pauto);

    //
    // clean up advisory connection
    //
    ATOMICRELEASE(_padvise);

    //
    // and our async icon stuff too
    //
    if (_AsyncIconEvent)
    {
        CloseHandle(_AsyncIconEvent);

        if (_AsyncIconCount > 0)
        {
            DebugMsg(TF_WARNING, TEXT("****** possible PIDL memory leak %d"), _AsyncIconCount);
        }
    }

    //Make sure the task scheduler is gone.
    ATOMICRELEASE(_pScheduler);
}

BOOL DefView_IdleDoStuff(CDefView *pdsv, LPRUNNABLETASK pTask, REFTASKOWNERID rTID, DWORD_PTR lParam, DWORD dwPriority )
{
    HRESULT hr;

    ASSERT( pTask );
    ASSERT( dwPriority > 0 );

    if (pdsv->_pScheduler == NULL)
    {
        HRESULT hr = CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC, IID_IShellTaskScheduler, (void **)&(pdsv->_pScheduler));
        if ( FAILED( hr ))
        {
            return FALSE;
        }

        // set a 60 second timeout
        pdsv->_pScheduler->Status( ITSSFLAG_KILL_ON_DESTROY, DEFVIEW_THREAD_IDLE_TIMEOUT );
    }


    hr = pdsv->_pScheduler->AddTask( pTask, rTID, lParam, dwPriority );

    return SUCCEEDED( hr );
}

//  Get the number of running tasks of the indicated task ID.
UINT CDefView::_GetBackgroundTaskCount(REFTASKOWNERID rtid)
{
    if( NULL == _pScheduler )
        return 0 ;

    return _pScheduler->CountTasks( rtid ) ;
}


const TBBUTTON c_tbDefView[] = {
    { VIEW_MOVETO | IN_VIEW_BMP,    SFVIDM_EDIT_MOVETO,     TBSTATE_ENABLED,    BTNS_BUTTON,   {0,0}, 0, -1},
    { VIEW_COPYTO | IN_VIEW_BMP,    SFVIDM_EDIT_COPYTO,     TBSTATE_ENABLED,    BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_DELETE | IN_STD_BMP,      SFVIDM_FILE_DELETE,     TBSTATE_ENABLED,    BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_UNDO | IN_STD_BMP,        SFVIDM_EDIT_UNDO,       TBSTATE_ENABLED,    BTNS_BUTTON,   {0,0}, 0, -1},
    { 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    { VIEW_VIEWMENU | IN_VIEW_BMP,  SFVIDM_VIEW_VIEWMENU,   TBSTATE_ENABLED,    BTNS_WHOLEDROPDOWN, {0,0}, 0, -1},
    // hidden buttons (off by default, available only via customize dialog)
    { STD_PROPERTIES | IN_STD_BMP,  SFVIDM_FILE_PROPERTIES, TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_CUT | IN_STD_BMP,         SFVIDM_EDIT_CUT,        TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_COPY | IN_STD_BMP,        SFVIDM_EDIT_COPY,       TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_PASTE | IN_STD_BMP,       SFVIDM_EDIT_PASTE,      TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { VIEW_OPTIONS | IN_VIEW_BMP,   SFVIDM_TOOL_OPTIONS,    TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
};

// win95 defview toolbar, used for corel apphack
const TBBUTTON c_tbDefView95[] = {
    { STD_CUT | IN_STD_BMP,         SFVIDM_EDIT_CUT,        TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { STD_COPY | IN_STD_BMP,        SFVIDM_EDIT_COPY,       TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { STD_PASTE | IN_STD_BMP,       SFVIDM_EDIT_PASTE,      TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    { STD_UNDO | IN_STD_BMP,        SFVIDM_EDIT_UNDO,       TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    { STD_DELETE | IN_STD_BMP,      SFVIDM_FILE_DELETE,     TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { STD_PROPERTIES | IN_STD_BMP,  SFVIDM_FILE_PROPERTIES, TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    // the bitmap indexes here are relative to the view bitmap
    { VIEW_LARGEICONS | IN_VIEW_BMP, SFVIDM_VIEW_ICON,      TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0L, -1 },
    { VIEW_SMALLICONS | IN_VIEW_BMP, SFVIDM_VIEW_SMALLICON, TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0L, -1 },
    { VIEW_LIST       | IN_VIEW_BMP, SFVIDM_VIEW_LIST,      TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0L, -1 },
    { VIEW_DETAILS    | IN_VIEW_BMP, SFVIDM_VIEW_DETAILS,   TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0L, -1 },
};


LRESULT CDefView::_TBNotify(NMHDR *pnm)
{
    LPTBNOTIFY ptbn = (LPTBNOTIFY)pnm;

    switch (pnm->code) 
    {
    case TBN_BEGINDRAG:
        DefView_OnMenuSelect(this, ptbn->iItem, 0, 0);
        break;

    }
    return 0;
}

BOOL CDefView::_MergeIExplorerToolbar(UINT cExtButtons)
{
    BOOL fRet = FALSE;
    IExplorerToolbar *piet;
    if (SUCCEEDED(IUnknown_QueryService(_psb, SID_SExplorerToolbar, IID_IExplorerToolbar, (void **)&piet))) 
    {
        BOOL fGotClsid = TRUE;

        DWORD dwFlags = VBF_TOOLS | VBF_ADDRESS | VBF_BRAND;

        if (cExtButtons == 0) 
        {
            // This shf has no buttons to merge in; use the standard defview 
            // clsid so that the shf shares standard toolbar customization.
            _clsid = CGID_DefViewFrame2;

        }
        else if (SUCCEEDED(IUnknown_GetClassID(_pshf, &_clsid))) 
        {
            // This shf has buttons to merge in; use its clsid 
            // so that this shf gets separate customization persistence.

            // The shf might expect us to provide room for two lines of
            // text (since that was the default in IE4).
            dwFlags |= VBF_TWOLINESTEXT;
        }
        else 
        {
            // This shf has buttons to merge in but doesn't implement
            // IPersist::GetClassID; so we can't use IExplorerToolbar mechanism.
            fGotClsid = FALSE;
        }

        if (fGotClsid) 
        {
            HRESULT hr = piet->SetCommandTarget((IUnknown *)SAFECAST(this, IOleCommandTarget *), &_clsid, dwFlags);
            if (SUCCEEDED(hr))
            {
                // If hr == S_FALSE, another defview merged in its buttons under the 
                // same clsid, and they're still there.  So no need to call AddButtons.

                if (hr != S_FALSE)
                    hr = piet->AddButtons(&_clsid, _cButtons, _pbtn);

                if (SUCCEEDED(hr))
                {
                    if (m_cFrame.IsSFVExtension())
                        _EnableDisableTBButtons();
                    fRet = TRUE;
                }
            }
        }
        piet->Release();
    }
    return fRet;
}

int _FirstHiddenButton(TBBUTTON* ptbn, int cButtons)
{
    int i;

    for (i = 0; i < cButtons; i++) 
    {
        if (ptbn[i].fsState & TBSTATE_HIDDEN)
            break;
    }

    return i;
}

void CDefView::_CopyDefViewButton(PTBBUTTON ptbbDest, PTBBUTTON ptbbSrc)
{
    *ptbbDest = *ptbbSrc;

    if (!(ptbbDest->fsStyle & BTNS_SEP))
    {
        // Fix up bitmap offset depending on whether this is a "view" bitmap or a "standard" bitmap
        if (ptbbDest->iBitmap & IN_VIEW_BMP)
            ptbbDest->iBitmap = (int)((ptbbDest->iBitmap & ~PRIVATE_TB_FLAGS) + _iViewBMOffset);
        else
            ptbbDest->iBitmap = (int)(ptbbDest->iBitmap + _iStdBMOffset);
    }
}

//
// Here's the deal with _GetButtons
//
// DefView has some buttons, and its callback client may have some buttons.
//
// Some of defview's buttons are visible on the toolbar by default, and some only show
// up if you customize the toolbar.
//
// We specify which buttons are hidden by default by marking them with TBSTATE_HIDDEN in
// the declaration of c_tbDefView.  We assume all such buttons are in a continuous block at
// the end of c_tbDefView.
//
// We return in ppbtn a pointer to an array of all the buttons, including those not shown 
// by default.  We put the buttons not shown by default at the end of this array.  We pass
// back in pcButtons the count of visible buttons, and in pcTotalButtons the count of visible
// and hidden buttons.
//
// The int return value is the number of client buttons in the array.
//
int CDefView::_GetButtons(PTBBUTTON* ppbtn, LPINT pcButtons, LPINT pcTotalButtons)
{
    int cVisibleBtns = 0;   // count of visible defview + client buttons

    TBINFO tbinfo;
    tbinfo.uFlags = TBIF_APPEND;
    tbinfo.cbuttons = 0;

    // Does the client want to prepend/append a toolbar?
    CallCB(SFVM_GETBUTTONINFO, 0, (LPARAM)&tbinfo);

    _uDefToolbar = HIWORD(tbinfo.uFlags);
    tbinfo.uFlags &= 0xffff;


    // tbDefView needs to be big enough to hold either c_tbDefView or c_tbDefView95
    COMPILETIME_ASSERT(ARRAYSIZE(c_tbDefView95) >= ARRAYSIZE(c_tbDefView));

    TBBUTTON tbDefView[ARRAYSIZE(c_tbDefView95)];
    int cDefViewBtns;   // total count of defview buttons

    if (SHGetAppCompatFlags(ACF_WIN95DEFVIEW) & ACF_WIN95DEFVIEW) 
    {
        memcpy(tbDefView, c_tbDefView95, SIZEOF(TBBUTTON) * ARRAYSIZE(c_tbDefView95));
        cDefViewBtns = ARRAYSIZE(c_tbDefView95);
    }
    else 
    {
        memcpy(tbDefView, c_tbDefView, SIZEOF(TBBUTTON) * ARRAYSIZE(c_tbDefView));
        cDefViewBtns = ARRAYSIZE(c_tbDefView);
    }

    int cVisibleDefViewBtns = _FirstHiddenButton(tbDefView, cDefViewBtns);  // count of visible defview buttons

    LPTBBUTTON pbtn = (LPTBBUTTON)LocalAlloc(LPTR, (cDefViewBtns + tbinfo.cbuttons) * SIZEOF(TBBUTTON));
    if (pbtn)
    {
        int iStart = 0;
        cVisibleBtns = tbinfo.cbuttons + cVisibleDefViewBtns;

        // Have the client fill in its buttons
        switch (tbinfo.uFlags)
        {
        case TBIF_PREPEND:
            CallCB(SFVM_GETBUTTONS,
                         MAKEWPARAM(SFVIDM_CLIENT_FIRST, tbinfo.cbuttons),
                         (LPARAM)pbtn);
            iStart = tbinfo.cbuttons;
            break;

        case TBIF_APPEND:
            CallCB(SFVM_GETBUTTONS,
                         MAKEWPARAM(SFVIDM_CLIENT_FIRST, tbinfo.cbuttons),
                         (LPARAM)&pbtn[cVisibleDefViewBtns]);
            iStart = 0;
            break;

        case TBIF_REPLACE:
            CallCB(SFVM_GETBUTTONS,
                         MAKEWPARAM(SFVIDM_CLIENT_FIRST, tbinfo.cbuttons),
                         (LPARAM)pbtn);

            cVisibleBtns = tbinfo.cbuttons;
            cVisibleDefViewBtns = 0;
            break;

        default:
            RIPMSG(0, "View callback passed an invalid TBINFO flag");
            break;
        }

        // Fill in visible defview buttons
        for (int i = 0; i < cVisibleDefViewBtns; i++)
        {
            // Visible defview button block gets added at iStart
            _CopyDefViewButton(&pbtn[i + iStart], &tbDefView[i]);
        }

        // Fill in hidden defview buttons
        for (i = cVisibleDefViewBtns; i < cDefViewBtns; i++)
        {
            // Hidden defview button block gets added after visible & client buttons
            _CopyDefViewButton(&pbtn[i + tbinfo.cbuttons], &tbDefView[i]);

            // If this rips a visible button got mixed in with the hidden block
            ASSERT(pbtn[i + tbinfo.cbuttons].fsState & TBSTATE_HIDDEN);

            // Rip off the hidden bit
            pbtn[i + tbinfo.cbuttons].fsState &= ~TBSTATE_HIDDEN;
        }
    }

    ASSERT(ppbtn);
    ASSERT(pcButtons);
    ASSERT(pcTotalButtons);

    *ppbtn = pbtn;
    *pcButtons = cVisibleBtns;
    *pcTotalButtons = tbinfo.cbuttons + cDefViewBtns;

    return tbinfo.cbuttons;
}


void CDefView::MergeToolBar(BOOL bCanRestore)
{

    TBADDBITMAP ab;

    ab.hInst = HINST_COMMCTRL;          // hinstCommctrl
    ab.nID   = IDB_STD_SMALL_COLOR;     // std bitmaps
    _psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 8, (LPARAM)&ab, &_iStdBMOffset);

    ab.nID   = IDB_VIEW_SMALL_COLOR;    // std view bitmaps
    _psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 8, (LPARAM)&ab, &_iViewBMOffset);

    if (_pbtn)
        LocalFree(_pbtn);

    int cExtButtons = _GetButtons(&_pbtn, &_cButtons, &_cTotalButtons);

    if (_pbtn && !_MergeIExplorerToolbar(cExtButtons)) 
    {
        // if we're able to do the new IExplorerToolbar merge method, great...
        // if not, we use the old style
        _psb->SetToolbarItems(_pbtn, _cButtons, FCT_MERGE);
        CDefView::CheckToolbar();
    }
}

STDMETHODIMP CDefView::GetWindow(HWND *phwnd)
{
    *phwnd = _hwndView;
    return S_OK;
}

STDMETHODIMP CDefView::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDefView::EnableModeless(BOOL fEnable)
{
    // We have no modeless window to be enabled/disabled
    return S_OK;
}

HRESULT CDefView::_ReloadListviewContent()
{
    //
    // HACK: We always call IsShared with fUpdateCache=FALSE for performance.
    //  However, we need to update the cache when the user explicitly tell
    //  us to "Refresh". This is not the ideal place to put this code, but
    //  we have no other choice.
    //
    // BUGBUG: this update cache thing is old and bogus I think... talk to the net guys
    //
    TCHAR szPathAny[MAX_PATH];

    this->UpdateSelectionMode();

    // finish any pending edits
    SendMessage(_hwndListview, LVM_EDITLABEL, (WPARAM)-1, 0);

    GetWindowsDirectory(szPathAny, ARRAYSIZE(szPathAny));
    IsShared(szPathAny, TRUE);

    // First we have to save all the icon positions, so they will be restored
    // properly during the FillObjects
    SaveViewState();

    // 99/04/07 #309965 vtan: Persist the view state (above). Make sure
    // our internal representation is the same as the one on the disk
    // by dumping our cache and reloading the information.

    if (_pSaveHeader != NULL)
    {
        LocalFree(_pSaveHeader);
        _pSaveHeader = NULL;
    }
    _uSaveHeaderLen = _GetSaveHeader(&_pSaveHeader);

    // HACK: strange way to notify folder that we're refreshing
    ULONG rgf = SFGAO_VALIDATE;
    _pshf->GetAttributesOf(0, NULL, &rgf);

    //
    // if a item is selected, make sure it gets nuked from the icon
    // cache, this is a last resort type thing, select a item and
    // hit F5 to fix all your problems.
    //
    int iItem = ListView_GetNextItem(_hwndListview, -1, LVNI_SELECTED);
    if (iItem != -1)
        Icon_FSEvent(SHCNE_UPDATEITEM, _GetPIDL(iItem), NULL);

    // We should use the existing sort parameters instead of getting the
    // defaults. So, the following line is commented out!
    // _GetSortDefaults(&_dvState);

    return FillObjectsShowHide(TRUE, NULL, 0, TRUE);
}

HRESULT CDefView::ReloadContent(BOOL fForce)
{
    HRESULT hresExtView = NOERROR;
    HRESULT hresNormalView = NOERROR;
    HRESULT hresSFVExt = NOERROR;
    SHELLSTATE ss;
    BOOL fHasDVOC = _fGetWindowLV;

    // Tell the defview client that this window is about to be refreshed
    CallCB(SFVM_REFRESH, TRUE, 0);


    // The call to GetExtViews below reorders the m_lViews list; m_uView & m_uActiveExtendedView are indices
    // into this list.  When this list is reordered m_uView & m_uActiveExtendededView point to the wrong
    // views.  The problem came forward when we were in thumbnail view, then customized to a web view...
    // REVIEW:  Should this code, which holds on the GUIDs for m_uView & m_uActiveExtendedView be put
    // into GetExtViews?  
    GUID idViewGUID = GUID_NULL;
    GUID idActiveExtViewGUID = GUID_NULL;
    if (m_cFrame.IsSFVExtension()) // IShellView extension
    {
        int iView = m_cFrame.m_lViews.NthUnique(m_cFrame.m_uView + 1);
        if(iView != -1)
        {
            SFVVIEWSDATA* pItem = m_cFrame.m_lViews.GetPtr(iView);
            if (pItem)
            {
                idViewGUID = pItem->idExtShellView;
            }
        }
        iView = m_cFrame.m_lViews.NthUnique(m_cFrame.m_uActiveExtendedView + 1);
        if(iView != -1)
        {
            SFVVIEWSDATA* pItem = m_cFrame.m_lViews.GetPtr(iView);
            if (pItem)
            {
                idActiveExtViewGUID = pItem->idExtShellView;
            }
        }
    }

    //Reload the Templates even if we are not in extended view.
    m_cFrame.GetExtViews(TRUE);  //Reload the template names from registry again.

    // Part 2 to the code above; after saving the guids for the views held in m_uView & m_uActiveExtendedView
    // we reset the indices to the correct position in the m_lViews list.
    if (IsEqualGUID(idViewGUID, GUID_NULL) == FALSE)
    {
        int iView;
        for (int i = 0; (iView = m_cFrame.m_lViews.NthUnique(i + 1)) != -1; i++)
        {
            SFVVIEWSDATA* pItem2 = m_cFrame.m_lViews.GetPtr(iView);
            if (pItem2 && IsEqualGUID(idViewGUID, pItem2->idExtShellView))
            {
                m_cFrame.m_uView = i;
                break;
            }
        }
    }
    if (IsEqualGUID(idActiveExtViewGUID, GUID_NULL) == FALSE)
    {
        int iView;
        for (int i = 0; (iView = m_cFrame.m_lViews.NthUnique(i + 1)) != -1; i++)
        {
            SFVVIEWSDATA* pItem2 = m_cFrame.m_lViews.GetPtr(iView);
            if (pItem2 && IsEqualGUID(idActiveExtViewGUID, pItem2->idExtShellView))
            {
                m_cFrame.m_uActiveExtendedView = i;
                break;
            }
        }
    }

    // If the global SSF_WIN95CLASSIC state changed, we need to muck with the UI.
    SHGetSetSettings(&ss, SSF_WIN95CLASSIC, FALSE);
    //Show webview and pane again if we are forced OR the view has changed.
    if (fForce || (BOOLIFY(ss.fWin95Classic) != BOOLIFY(_fClassic)))
    {
        DebugMsg(TF_DEFVIEW, TEXT("Global state SSF_WIN95CLASSIC changed to [%d] for DefView 0x%x"), _fClassic ? 1:0, this);
        _UpdateListviewColors(ss.fWin95Classic);
    }
    
    if (m_cFrame.IsSFVExtension()) // IShellView extension
    {
        hresSFVExt = m_cFrame.GetExtendedISV()->Refresh();
    }

    // BUGBUG PERF: when switching to normal view, we don't need to do this case!
    // We need to special case Desktop here because _CanShowWebView() does not work for desktop.
    if (_IsDesktop() ? m_cFrame.IsWebView() : _CanShowWebView()) // DocObject extension
    {
        // We need to save the icon positions before we hide the view.
        SaveViewState();

        hresExtView = _SwitchToViewPVID(&VID_WebView, TRUE); //Show ext view again.

        // if our listview isn't going to be shown in this view, mark it for refresh
        if (!_HasNormalView() && !_bUpdatePendingPending)
        {
            _bUpdatePending = TRUE;
        }
    }
    else
    {
        // Note: _UpdateListviewColors() function sets _fClassic to whatever is passed in as the parameter.
        // Our intention here is NOT to change _fClassic; So, we pass that itself in.
        _UpdateListviewColors(_fClassic);
    }

    // fHasDVOC is a Cache of whether we had a DVOC when we entered this routine. Because of the
    // view switch above, _fGetWindowLV may now be false because MSHTML has not had enough time to
    // recreate the DVOC, so we check the cached copy as well.
    if (_HasNormalView() || fHasDVOC) // normal view
    {
        //We want to preserve the earlier error if any;
        hresNormalView = _ReloadListviewContent();
    }

    return FAILED(hresSFVExt) ? hresSFVExt : (FAILED(hresExtView) ? hresExtView : hresNormalView);
}

STDMETHODIMP CDefView::Refresh()
{
    //See if some refreshes were buffered
    if (_fRefreshBuffered)
    {
        //Since we are refreshing it right now. Kill the timer.
        TraceMsg(TF_DEFVIEW, "Buffered Refresh Timer Killed by regular Refresh");
        KillTimer(_hwndView, DV_IDTIMER_BUFFERED_REFRESH);
        _fRefreshBuffered = FALSE;
    }

    //If desktop is in modal state, do not attempt to refresh.
    //If we do, we endup destroying Trident object when it is in modal state.
    if (_IsDesktop() && _fDesktopModal)
    {
        // Remember that we could not refresh the desktop because it was in
        // a modal state.
        _fDesktopRefreshPending = TRUE;
        return S_OK;
    }

    //  make sure we have the latest
    SHRefreshSettings();

    _UpdateRegFlags();

    // If we are on a drive we invalidate the
    // drive...
    if (_pidlMonitor) 
    {
        TCHAR szPath[MAX_PATH];
        if (SHGetPathFromIDList(_pidlMonitor, szPath))
        {
            int iDrive = PathGetDriveNumber(szPath);
            InvalidateDriveType(iDrive);
        }
    }

    if (_IsDesktop()) 
    {
        SHELLSTATE ss;
        SHGetSetSettings(&ss, SSF_HIDEICONS | SSF_DESKTOPHTML, FALSE);

        if (ss.fHideIcons)
            _fs.fFlags |= FWF_NOICONS;
        else
            _fs.fFlags &= ~FWF_NOICONS;

        _SwitchDesktopHTML(ss.fDesktopHTML, TRUE);

        if (ss.fDesktopHTML)
        {
            HMODULE hmod;

            // ActiveDesktop is not part of shdocvw's browser session count
            // so when we refresh, we must tell wininet to reset the session
            // count otherwise we will not hit the net.
            MyInternetSetOption(NULL, INTERNET_OPTION_RESET_URLCACHE_SESSION, NULL, 0);

            // In IE40 integrated shell mode, the Java VM only reloads classes
            // on a hard refresh.  Therefore we need to kick it once in a while.
            // Currently this is expense because they throw away all their jitted
            // classes.  Future versions of the VM should check with wininet for
            // changes before throwing classes away.  That will make this call
            // much less expensive.
            hmod = GetModuleHandle(TEXT("msjava.dll"));
            if (hmod)
            {
                typedef HRESULT (*PFNNOTIFYBROWSERSHUTDOWN)(void *);
                FARPROC fp = GetProcAddress(hmod, "NotifyBrowserShutdown");
                if (fp)
                {
                    HRESULT hr = ((PFNNOTIFYBROWSERSHUTDOWN)fp)(NULL);
                    ASSERT(SUCCEEDED(hr));
                }
            }
        }
    }

    return ReloadContent(TRUE);
}

STDMETHODIMP CDefView::CreateViewWindow(IShellView *lpPrevView,
        LPCFOLDERSETTINGS lpfs, IShellBrowser *psb, RECT *prc, HWND *phWnd)
{
    SV2CVW2_PARAMS cParams;

    cParams.cbSize   = SIZEOF(SV2CVW2_PARAMS);
    cParams.psvPrev  = lpPrevView;
    cParams.pfs      = lpfs;
    cParams.psbOwner = psb;
    cParams.prcView  = prc;
    cParams.pvid     = NULL;

    HRESULT hres = CreateViewWindow2(&cParams);

    *phWnd = cParams.hwndView;

    if (SUCCEEDED(hres) &&
        (SHGetAppCompatFlags(ACF_OLDCREATEVIEWWND) & ACF_OLDCREATEVIEWWND))
    {
        //
        //  CreateViewWindow was documented as returning S_OK on success,
        //  but IE4 changed the function to return S_FALSE if the defview
        //  was created async.
        //
        //  PowerDesk relies on the old behavior.
        //  So does Quattro Pro.
        //
        hres = S_OK;
    }

    return hres;
}

SHELLVIEWID const *SVIDFromViewMode(UINT ViewMode)
{
    switch (ViewMode) 
    {
    case FVM_SMALLICON:
        return &VID_SmallIcons;
    case FVM_LIST:
        return &VID_List;
    case FVM_DETAILS:
        return &VID_Details;
    case FVM_ICON:
    default:
        return &VID_LargeIcons;
    }
}

void ViewModeFromSVID(SHELLVIEWID const *pvid, UINT  *pViewMode)
{
    if (IsEqualIID(*pvid, VID_LargeIcons))
        *pViewMode = FVM_ICON;

    else if (IsEqualIID(*pvid, VID_SmallIcons))
        *pViewMode = FVM_SMALLICON;

    else if (IsEqualIID(*pvid, VID_List))
        *pViewMode = FVM_LIST;

    if (IsEqualIID(*pvid, VID_Details))
        *pViewMode = FVM_DETAILS;

}

STDMETHODIMP CDefView::HandleRename(LPCITEMIDLIST pidl)
{
    HRESULT hr = E_FAIL;

    // Gross, but if no PIDL passed in use the GetObject(-2) hack to get the selected object...
    // Don't need to free as it wsa not cloned...
    if (!pidl)
    {
        GetObject((LPITEMIDLIST*)&pidl, (UINT)-2);
    }
    else
    {
        RIP(ILFindLastID(pidl) == pidl);
        if (ILFindLastID(pidl) != pidl)
        {
            return E_INVALIDARG;
        }
    }

    if (m_cFrame.IsSFVExtension())
    {
        hr = m_cFrame.GetExtendedISV()->HandleRename(pidl);
    }
    else if (_HasNormalView())
    {
        hr = SelectAndPositionItem(pidl, SVSI_DESELECTOTHERS, NULL);

        if (SUCCEEDED(hr))
            hr = SelectAndPositionItem(pidl, SVSI_EDIT, NULL);
    }

    return hr;
}

// IViewObject
HRESULT CDefView::GetColorSet(DWORD dwAspect, LONG lindex, void *pvAspect,
    DVTARGETDEVICE *ptd, HDC hicTargetDev, LOGPALETTE **ppColorSet)
{
    if (m_cFrame.IsWebView() && m_cFrame.m_pvoActive)
    {
        return m_cFrame.m_pvoActive->GetColorSet(dwAspect, lindex, pvAspect,
            ptd, hicTargetDev, ppColorSet);
    }

    if (ppColorSet)
        *ppColorSet = NULL;

    return E_FAIL;
}

HRESULT CDefView::Freeze(DWORD, LONG, void *, DWORD *pdwFreeze)
{
    return E_NOTIMPL;
}

HRESULT CDefView::Unfreeze(DWORD)
{
    return E_NOTIMPL;
}

HRESULT CDefView::SetAdvise(DWORD dwAspect, DWORD advf, IAdviseSink *pSink)
{
    if (dwAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    if (advf & ~(ADVF_PRIMEFIRST | ADVF_ONLYONCE))
        return E_INVALIDARG;

    if (pSink != _padvise)
    {
        ATOMICRELEASE(_padvise);

        _padvise = pSink;

        if (_padvise)
            _padvise->AddRef();
    }

    if (_padvise)
    {
        _advise_aspect = dwAspect;
        _advise_advf = advf;

        if (advf & ADVF_PRIMEFIRST)
            PropagateOnViewChange(dwAspect, -1);
    }
    else
        _advise_aspect = _advise_advf = 0;

    return S_OK;
}

HRESULT CDefView::GetAdvise(DWORD *pdwAspect, DWORD *padvf,
    IAdviseSink **ppSink)
{
    if (pdwAspect)
        *pdwAspect = _advise_aspect;

    if (padvf)
        *padvf = _advise_advf;

    if (ppSink)
    {
        if (_padvise)
            _padvise->AddRef();

        *ppSink = _padvise;
    }

    return S_OK;
}

HRESULT CDefView::Draw(DWORD, LONG, void *, DVTARGETDEVICE *, HDC, HDC,
    const RECTL *, const RECTL *, BOOL (*)(ULONG_PTR), ULONG_PTR)
{
    return E_NOTIMPL;
}

void CDefView::PropagateOnViewChange(DWORD dwAspect, LONG lindex)
{
    dwAspect &= _advise_aspect;

    if (dwAspect && _padvise)
    {
        IAdviseSink *pSink = _padvise;
        IUnknown *punkRelease;

        if (_advise_advf & ADVF_ONLYONCE)
        {
            punkRelease = pSink;
            _padvise = NULL;
            _advise_aspect = _advise_advf = 0;
        }
        else
            punkRelease = NULL;

        pSink->OnViewChange(dwAspect, lindex);

        ATOMICRELEASE(punkRelease);
    }
}

void CDefView::PropagateOnClose()
{
    //
    // we aren't closing ourselves, just somebody under us...
    // ...reflect this up the chain as a view change.
    //
    if (_padvise)
        PropagateOnViewChange(_advise_aspect, -1);
}


STDMETHODIMP CDefView::GetView(SHELLVIEWID* pvid, ULONG uView)
{
    HRESULT hres = m_cFrame.GetView(pvid, uView);
    if (NOERROR == hres)
    {
        return hres;
    }

    switch (uView)
    {
    case SV2GV_CURRENTVIEW:
        *pvid = *SVIDFromViewMode(_fs.ViewMode);
        break;

    case SV2GV_DEFAULTVIEW:
        *pvid = VID_LargeIcons;
        break;

    default:
        return hres;
    }

    return NOERROR;
}

// For Folder Advanced Options flags that we check often, it's better
// to cache the values as flags. Update them here.
void CDefView::_UpdateRegFlags()
{
    DWORD cbSize;
    DWORD dwValue;

    cbSize = SIZEOF(dwValue);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
            TEXT("ClassicViewState"), NULL, &dwValue, &cbSize)
        && dwValue)
    {
        _fWin95ViewState = TRUE;
    }
    else
    {
        _fWin95ViewState = FALSE;
    }
}


STDMETHODIMP CDefView::CreateViewWindow2(LPSV2CVW2_PARAMS lpParams)
{

    if (g_dwProfileCAP & 0x00000001)
        StopCAP();

    if (lpParams->cbSize < SIZEOF(SV2CVW2_PARAMS))
        return E_INVALIDARG;

    IShellView *lpPrevView  =  lpParams->psvPrev;
    LPCFOLDERSETTINGS lpfs  =  lpParams->pfs;
    IShellBrowser *psb      =  lpParams->psbOwner;
    RECT *prc               =  lpParams->prcView;
    HWND *phWnd             = &lpParams->hwndView;
    SHELLVIEWID const *pvid =  lpParams->pvid;
    SHELLSTATE ss;

    if (pvid && IsEqualIID(*pvid, VID_WebView))
        pvid = NULL;

    HRESULT hres;
    UINT uLen;

    *phWnd = NULL;

    // the client needs this info to be able to do anything with us,
    // so set it REALLY early on in the creation process
    IUnknown_SetSite(this->m_cCallback.GetSFVCB(), SAFECAST(this, IShellFolderView*));

    if (_hwndView || !DefView_RegisterWindow() || !psb)
        return E_UNEXPECTED;

    ASSERT(_pshf);
    _pshf->QueryInterface(IID_IShellIcon, (void **)&_psi);
    _pshf->QueryInterface(IID_IShellIconOverlay, (void **)&_psio);

    // We need to make sure to store this before doing the GetWindowRect
    //
    _psb = psb;
    psb->AddRef();

    psb->QueryInterface(IID_ICommDlgBrowser, (void **)&_pcdb);

    _fs = *lpfs;

    BOOL fIsOwnerData = FALSE;
    CallCB(SFVM_ISOWNERDATA, 0, (LONG_PTR)&fIsOwnerData);
    if (fIsOwnerData)
        _fs.fFlags |= FWF_OWNERDATA;
    else
        _fs.fFlags &= ~FWF_OWNERDATA;


    // This should never fail
    _psb->GetWindow(&_hwndMain);
    ASSERT(IsWindow(_hwndMain));
    CallCB(SFVM_HWNDMAIN, 0, (LPARAM)_hwndMain);

    // We need to restore the column widths before showing the window
    ASSERT(!_pSaveHeader);
    uLen = _GetSaveHeader(&_pSaveHeader);
    _uSaveHeaderLen = uLen;

    // Grab other default values we need to set before showing the window
    //PDVSAVEHEADEREX pSaveHeaderEx = _GetSaveHeaderEx(_pSaveHeader);

    if ( _pSaveHeader )
    {
        // set the default sort order incase we are creating an extension view.
        _dvState = _pSaveHeader->dvState;
    }
    
    //
    // if there was a previous view that we know about, poke around a bit
    //
    if (_fWin95ViewState)
    {
        CDefView *pdsvPrev;
        if (lpPrevView &&
            SUCCEEDED(lpPrevView->QueryInterface(IID_CDefView, (void **)&pdsvPrev)))
        {
            //
            // preserve stuff like sort order
            //
            _dvState = pdsvPrev->_dvState;
            if (_pSaveHeader)
                _pSaveHeader->dvState = pdsvPrev->_dvState;

            ATOMICRELEASE(pdsvPrev);
        }
    }

    // See if we should map a VID to the right view mode
    if (pvid)
        ViewModeFromSVID(pvid, &_fs.ViewMode);


    if (!CreateWindowEx(IS_WINDOW_RTL_MIRRORED(_hwndMain) ? dwExStyleRTLMirrorWnd : 0L, c_szDefViewClass, szNULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP,
            prc->left, prc->top, prc->right-prc->left, prc->bottom-prc->top,
            _hwndMain, NULL, HINST_THISDLL, this))
    {
        return E_OUTOFMEMORY;
    }

    // If it's the desktop folder, force it to reload the ext views
    // - when the desktop is created, GetExtViews() would have
    // been called from shdocvw, but it's actual implementation wouldn't
    // have had enough info to figure out if it was the actual desktop
    // or a desktop folder. So we force it to reload it now.

    m_cFrame.GetExtViews(_IsViewDesktop());

    SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_DESKTOPHTML | SSF_WEBVIEW, FALSE);
    if (_IsDesktop())  //Is this the Desktop?
    {
        // Does the user want desktop in HyperText view?
        if (ss.fDesktopHTML)
        {
            pvid = &VID_WebView;
        }

        // Done immediately below, no need to do it here.
        //this->UpdateSelectionMode();
    }

    this->UpdateSelectionMode();

    // Also see if the view would like to handle their own view state...

    // See if they want to overwrite the selection object
    if (fIsOwnerData)
    {
        // Only used in owner data.
        ILVRange *plvr = NULL;
        CallCB(SFVM_GETODRANGEOBJECT, LVSR_SELECTION, (LPARAM)&plvr);
        if (plvr)
        {
            ListView_SetLVRangeObject(_hwndListview, LVSR_SELECTION, plvr);
            plvr->Release();    // We assume the lv will hold onto it...
        }

        plvr = NULL;
        CallCB(SFVM_GETODRANGEOBJECT, LVSR_CUT, (LPARAM)&plvr);
        if (plvr)
        {
            ListView_SetLVRangeObject(_hwndListview, LVSR_CUT, plvr);
            plvr->Release();    // We assume the lv will hold onto it...
        }
    }

    // Switch into Web View (if it's on).
    // But don't do this if the app requires old-style defview.

    hres = S_OK; // assume synchronous (ex: Large Icon view)
    if (_CanShowWebView())
    {
        // This returns S_FALSE if we're waiting for the view
        // to go ReadyStateInteractive, S_OK otherwise.
        //
        hres = _SwitchToViewPVID(&VID_WebView, FALSE);
        if (FAILED(hres))
        {
            TraceMsg(TF_WARNING, "DefView failed extended view switch during creation");
            hres = S_OK;
        }
    }

    // The browser will ask SHDVID_CANACTIVATENOW at some point,
    // and we have to remember how to answer.  Once we answer S_OK
    // to that, our view becomes active.
    //
    _fCanActivateNow = (S_OK == hres); // S_FALSE implies async waiting for ReadyStateInteractive

    if (pvid)
    {
        // We manually map VIDs for the 4 normal views to normal
        // views -- don't waste time trying a CoCreateInstance
        //
        if (!(IsEqualIID(*pvid, VID_LargeIcons) ||
            IsEqualIID(*pvid, VID_SmallIcons) ||
            IsEqualIID(*pvid, VID_List) ||
            IsEqualIID(*pvid, VID_Details)))
        {
            hres = _SwitchToViewPVID(pvid, FALSE);

            // Oops, the extended view is no longer there,
            // we already went back to large icon, so assume
            // success here.
            //
            if (FAILED(hres))
            {
                TraceMsg(TF_WARNING, "DefView failed extended view switch during creation");
                hres = S_OK;
            }

            // WebView is handled specifically above for non-desktop,
            // so all pvid's coming through here had better be
            // synchronous ISV extended views.  Which means S_OK was
            // the return result.
            ASSERT(_IsDesktop() || (S_OK==hres));

            // _IsDesktop() cannot go async because CDesktopBrowser removes
            // the SBSC_RELATIVE bit and forces SBSC_NEWWINDOW, which breaks
            // CBaseBrowser's Async Activation notify method that we use
            // when the readystate goes interactive... Force the desktop
            // to go synchronous.
            if (_IsDesktop())
            {
                hres = S_OK;
            }
        }
    }

    // turn on proper background and colors
    // BUGBUG: Since we haven't switchted to the view yet, these colors
    // may come out wrong. We should test this...
    _UpdateListviewColors(ss.fWin95Classic);

    // NB - Nasty side effect - this needs to be done
    // before calling _BestFit (in DV_FillObjects) so that the
    // parent can handle size changes effectively.
    *phWnd = _hwndView;
    
    //
    // since ::FillObjects can take a while we force a paint now
    // before any items are added so we don't see the gray background of
    // the cabinet window for a long time.
    //
    // The below if looks redundant, ASSERT that it is for now -- remove redundancy later
    ASSERT(((S_OK == hres) && _fCanActivateNow) || !((S_OK == hres) && _fCanActivateNow));
    if ((S_OK == hres) && _fCanActivateNow)
    {
        // Show the window early (what old code did)
        SetWindowPos(_hwndView, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
        UpdateWindow(_hwndView);
        _OnMoveWindowToTop(_hwndView);

        TraceMsg(TF_DEFVIEW, "Creating sync DefView");
        _fIsAsyncDefView = FALSE;
    }
    else
    {
        TraceMsg(TF_DEFVIEW, "Creating ASYNC DefView");
        _fIsAsyncDefView = TRUE;
    }

    // Always start filling the listview with objects now. Even if we
    // have an extended view, it's pretty likely that it will use
    // the defviewoc to get at our listview.
    // BUGBUG PERF NOTE: we could improve perf for such non-listview
    // extended views by checking the bits.
    //
    _bUpdatePending = TRUE;
    {
        HRESULT hresT = FillObjectsShowHide(TRUE, _pSaveHeader, uLen, TRUE);
        if (FAILED(hresT))
        {
            //
            // The fill objects failed for some reason, we should
            // return an error.
            //
            this->DestroyViewWindow();
            ASSERT(_hwndView == NULL);
            *phWnd = NULL;

            //
            // Note that we don't need to clean _psb or _pcdb
            // because this object will be deleted anyway.
            //
            return hresT;
        }
        _PostEnumDoneMessage();
    }
    // this needs to be done after the enumeration
    if (_pidlMonitor || _lFSEvents) // check both so that views can register for everything
    {
        int fSources = (_lFSEvents & SHCNE_DISKEVENTS) ? SHCNRF_ShellLevel | SHCNRF_InterruptLevel : SHCNRF_ShellLevel;
        UINT uFSEvents = _lFSEvents | SHCNE_UPDATEIMAGE | SHCNE_UPDATEDIR;
        SHChangeNotifyEntry fsne;

        if (FAILED(CallCB(SFVM_QUERYFSNOTIFY, 0, (LPARAM)&fsne)))
        {
            // Reset entry
            fsne.pidl = _pidlMonitor;
            fsne.fRecursive = FALSE;
        }

        _uRegister = SHChangeNotifyRegister(_hwndView, SHCNRF_NewDelivery | fSources,
                                                 uFSEvents, WM_DSV_FSNOTIFY, 1, &fsne);
    }

    // We do the toolbar before the menu bar to avoid flash
    if (!_IsDesktop())
        MergeToolBar(TRUE);

    // BUGBUG: commdlg should support drag drop too!
    ASSERT(_pdtgtBack == NULL);

    // this may fail
    _pshf->CreateViewObject(_hwndMain, IID_IDropTarget, (void **)&_pdtgtBack);

    // we don't really need to register drag drop when in the shell because
    // our frame does it for us.   we still need it here for comdlg and other
    // hosts.. but for the desktop, let the desktpo frame take care of this
    // so that they can do webbar d/d creation
    if (!_IsDesktop()) 
    {
        THR(RegisterDragDrop(_hwndListview, SAFECAST(this, IDropTarget*)));
        _bRegisteredDragDrop = TRUE;
    }

    ASSERT(SUCCEEDED(hres))

    ViewWindow_BestFit(this, FALSE);

    // Tell the defview client that this windows has been initialized
    CallCB(SFVM_WINDOWCREATED, (WPARAM)_hwndView, 0);
    if (SUCCEEDED(CallCB(SFVM_QUERYCOPYHOOK, 0, 0)))
        AddCopyHook();

    // BUGBUG:: See if there is a cleaner place to do this.
    // 
    if (SUCCEEDED(_GetIPersistHistoryObject(NULL)))
    {
        IBrowserService *pbs;
        if (SUCCEEDED(_psb->QueryInterface(IID_IBrowserService, (void **)&pbs))) 
        {
            IOleObject *pole;
            IStream *pstm;
            IBindCtx *pbc;
            pbs->GetHistoryObject(&pole, &pstm, &pbc);
            if (pole) 
            {
                IUnknown_SetSite(pole, SAFECAST(this, IShellView2*));      // Set the back pointer.
                if (pstm)
                {
                    IPersistHistory *pph;
                    if (SUCCEEDED(pole->QueryInterface(IID_IPersistHistory, (void **)&pph)))
                    {
                        pph->LoadHistory(pstm, pbc);
                        pph->Release();
                    }
                    pstm->Release();
                }
                IUnknown_SetSite(pole, NULL);  // just to be safe...
                if (pbc)
                    pbc->Release();
                pole->Release();
            }
            pbs->Release();
        }
    }

    return hres;
}


STDMETHODIMP CDefView::DestroyViewWindow()
{
    int     i, iHeaderCount;
    HWND    hwndHeader;

    // 99/04/16 #326158 vtan: Loop thru the headers looking for
    // stray HBITMAPs which need to be DeleteObject'd. Don't bother
    // setting it back the header is about to be dumped.
    // NOTE: Make sure this gets executed BEFORE the view gets
    // dumped below in DestoryViewWindow().

    hwndHeader = ListView_GetHeader(_hwndListview);
    iHeaderCount = Header_GetItemCount(hwndHeader);
    for (i = 0; i < iHeaderCount; ++i)
    {
        HDITEM  hdi;

        hdi.mask = HDI_BITMAP;
        Header_GetItem(hwndHeader, i, &hdi);
        if (hdi.hbm != NULL)
            TBOOL(DeleteObject(hdi.hbm));
    }

    m_cFrame.ShowExtView(CSFVFrame::NOEXTVIEW, TRUE);

    //
    // Just in case...
    //
    OnDeactivate();

    if (_hwndView)
    {
        HWND hwndTemp = _hwndView;

        //
        // This is a bit lazy implementation, but minimum code.
        //
        RemoveCopyHook();

        // Put NULL in _hwndView indicating that we are destroying.
        _hwndView = NULL;

        // Tell the defview client that this window will be destroyed
        CallCB(SFVM_WINDOWDESTROY, (WPARAM)hwndTemp, 0);

        DestroyWindow(hwndTemp);
    }

    IUnknown_SetSite(this->m_cCallback.GetSFVCB(), NULL);

    return S_OK;
}

void CDefView::MergeViewMenu(HMENU hmenu, HMENU hmenuMerge)
{
    HMENU hmenuView = _GetMenuFromID(hmenu, FCIDM_MENU_VIEW);
    if (hmenuView)
    {
        //
        // Find the "options" separator in the view menu.
        //
        int index = MenuIndexFromID(hmenuView, FCIDM_MENU_VIEW_SEP_OPTIONS);

        //
        // Here, index is the index of he "optoins" separator if it has;
        // otherwise, it is -1.
        //

        // Add the separator above (in addition to existing one if any).
        InsertMenu(hmenuView, index, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        // Then merge our menu between two separators (or right below if only one).
        if (index != -1) {
            index++;
        }

        Shell_MergeMenus(hmenuView, hmenuMerge, (UINT)index, 0, (UINT)-1, MM_SUBMENUSHAVEIDS);

        m_cFrame.MergeExtViewsMenu(hmenuView, this); // add extended views to top of menu
    }
}

void CDefView::_SetUpMenus(UINT uState)
{
    //
    // If this is desktop, don't bother creating menu
    //
    if (!_IsDesktop())
    {
        HMENU hMenu;

        OnDeactivate();

        ASSERT(_hmenuCur == NULL);

        hMenu = CreateMenu();

        if (hMenu)
        {
            HMENU hMergeMenu;
            OLEMENUGROUPWIDTHS mwidth = { { 0, 0, 0, 0, 0, 0 } };

            _hmenuCur = hMenu;
            _psb->InsertMenusSB(hMenu, &mwidth);

            if (uState == SVUIA_ACTIVATE_FOCUS)
            {
                hMergeMenu = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(POPUP_SFV_MAINMERGE));
                if (hMergeMenu)
                {
                    // NOTE: hard coded references to offsets in this menu

                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_FILE),
                            GetSubMenu(hMergeMenu, 0), (UINT)0, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS);

                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_EDIT),
                            GetSubMenu(hMergeMenu, 1), (UINT)-1, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS);

                    MergeViewMenu(hMenu, GetSubMenu(hMergeMenu, 2));
                    // remove Choose Columns from the view menu if not in Details view
                    if (!_IsReportView())
                        DeleteMenu(hMenu, SFVIDM_VIEW_COLSETTINGS, MF_BYCOMMAND);

                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_HELP),
                            GetSubMenu(hMergeMenu, 3), (UINT)0, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

                    DestroyMenu(hMergeMenu);
                }

            }
            else
            {
                hMergeMenu = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(POPUP_SFV_MAINMERGENF));
                if (hMergeMenu)
                {
                    // NOTE: hard coded references to offsets in this menu

                    // top half of edit menu
                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_EDIT),
                            GetSubMenu(hMergeMenu, 0), (UINT)0, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

                    // bottom half of edit menu
                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_EDIT),
                            GetSubMenu(hMergeMenu, 1), (UINT)-1, 0, (UINT)-1,
                            MM_SUBMENUSHAVEIDS);

                    // view menu
                    MergeViewMenu(hMenu, GetSubMenu(hMergeMenu, 2));
                    // remove Choose Columns from the view menu if not in Details view
                    if (!_IsReportView())
                        DeleteMenu(hMenu, SFVIDM_VIEW_COLSETTINGS, MF_BYCOMMAND);

                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_HELP),
                            GetSubMenu(hMergeMenu, 3), (UINT)0, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

                    DestroyMenu(hMergeMenu);
                }
            }

            // Allow the client to merge its own menus
            UINT indexClient = GetMenuItemCount(hMenu)-1;
            QCMINFO info = { hMenu, indexClient, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST };
            CallCB(SFVM_MERGEMENU, 0, (LPARAM)&info);

            _psb->SetMenuSB(hMenu, NULL, _hwndView);
        }
    }
}

// set up the menus based on our activation state
//
BOOL CDefView::OnActivate(UINT uState)
{
    if (_uState != uState)
    {
        _SetUpMenus(uState);
        _uState = uState;
    }

    return TRUE;
}

void CDefView::SwapWindow(void)
{
    if (EVAL(_pocWinMan))
    {
        _fGetWindowLV = FALSE;
        HWND hwndXV = m_cFrame.GetExtendedViewWindow();
        if (hwndXV)
        {
            if ((SUCCEEDED(_pocWinMan->SwapWindow(hwndXV, &_pocWinMan))))
                _fGetWindowLV = TRUE;
            ASSERT(m_cFrame.GetExtendedISFV());
            ASSERT(m_pauto);
            m_cFrame.GetExtendedISFV()->SetAutomationObject(m_pauto);
        }
    }
}

BOOL CDefView::OnDeactivate()
{
    if (_hmenuCur || (_uState != SVUIA_DEACTIVATE))
    {
        if (!_IsDesktop())
        {
            ASSERT(_hmenuCur);

            CallCB(SFVM_UNMERGEMENU, 0, (LPARAM)_hmenuCur);

            _psb->SetMenuSB(NULL, NULL, NULL);
            _psb->RemoveMenusSB(_hmenuCur);
            DestroyMenu(_hmenuCur);
            _hmenuCur = NULL;
        }
        _uState = SVUIA_DEACTIVATE;
        DV_FlushCachedMenu(this);
    }
    return TRUE;
}

void CDefView::_OnMoveWindowToTop(HWND hwnd)
{
    //
    // Let the browser know that this has happened
    //
    VARIANT var;
    var.vt = VT_INT_PTR;
    var.byref = hwnd;

    IUnknown_Exec(_psb, &CGID_Explorer, SBCMDID_ONVIEWMOVETOTOP, 0, &var, NULL);
}

//
//  This function activates the view window. Note that activating it
// will not change the focus (while setting the focus will activate it).
//
STDMETHODIMP CDefView::UIActivate(UINT uState)
{
    if (SVUIA_DEACTIVATE == uState)
    {
        OnDeactivate();
        ASSERT(_hmenuCur==NULL);
    }
    else
    {
        if (_fIsAsyncDefView)
        {
           //Show Defview only if it is Async - Bug 275266
           SetWindowPos(_hwndView, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
           UpdateWindow(_hwndView);
           _OnMoveWindowToTop(_hwndView);
        }
        if (uState == SVUIA_ACTIVATE_NOFOCUS)
        {
            // we lost focus
            // if in web view and we have valid ole obj (just being paranoid)
            if (!_fCombinedView && m_cFrame.IsWebView() && m_cFrame.m_pOleObj)
            {
                m_cFrame._UIActivateIO(FALSE, NULL);
            }
        }

        // We may be waiting for ReadyState_Interactive. If requested,
        // we should activate before then...
        //
        // When we boot, the desktop paints ugly white screen for several
        // seconds before it shows the HTML content. This is because: the
        // following code switches the oleobj even before it reaches readystate
        // interactive. For desktop, we skip this here. When the new object
        // reaches readystate interactive, we will show it!
        if (!_IsDesktop())
        {
            BOOL bOldHasFocus = _bExtHasFocus;

            m_cFrame._SwitchToNewOleObj();
            // thumbnailview calls defview->UIActivate when
            // we try to set focus to it. since we call defview_setfocus here
            // and it calls setfocus(thumbnailview) we get infinite loop
            // however we still need to call setfocus from here because this
            // method gets called when user is tabbing around so if we don't set
            // focus thumbnailview will be inaccessible.
            // we track SVUIA_ACTIVATE_FOCUS state of an extension with
            // bExtHasFocus, and only when it does not have focus we call
            // DefView_SetFocus
            if (m_cFrame.IsSFVExtension())
            {
                if (m_cFrame.m_hActiveSVExtHwnd && !m_cFrame.IsWebView())    // Just to be safe
                {
                    // Show the SFVExt window
                    SetWindowPos(m_cFrame.m_hActiveSVExtHwnd, NULL, 0, 0, 0, 0, 
                            SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW | SWP_NOZORDER);
                }
                _bExtHasFocus = (uState == SVUIA_ACTIVATE_FOCUS);
            }

            // NOTE: The browser IP/UI-activates us when we become the
            // current active view!  We want to resize and show windows
            // at that time.  But if we're still waiting for _fCanActivateNow
            // (ie, ReadyStateInteractive), then we need to cache this request
            // and do it later.  NOTE: if Trident caches the focus (done w/ TAB)
            // then we don't need to do anything here...
            //
            if (uState == SVUIA_ACTIVATE_FOCUS && (!m_cFrame.IsSFVExtension() || !bOldHasFocus))
            {
                DefView_SetFocus(this);
                // DefView_SetFocus can set _uState without causing our menu to
                // get created and merged.  Clear it here so that OnActivate does the
                // right thing.
                if (!_hmenuCur)
                    _uState = SVUIA_DEACTIVATE;
            }

        }
        // else we are the desktop; do we also need to steal focus?
        else if (uState == SVUIA_ACTIVATE_FOCUS)
        {
            HWND hwnd = GetFocus();
            if (SHIsChildOrSelf(_hwndView, hwnd) != S_OK)
                DefView_SetFocus(this);
        }

        // OnActivate must follow DefView_SetFocus
        OnActivate(uState);

        if (_fShowListviewIconsOnActivate)
        {
            _ShowListviewIcons();
            _fShowListviewIconsOnActivate = FALSE;
        }

        ASSERT(_IsDesktop() || _hmenuCur);

        m_cFrame._UpdateZonesStatusPane(NULL);
    }
    
    return S_OK;
}

STDMETHODIMP CDefView::GetCurrentInfo(LPFOLDERSETTINGS lpfs)
{
    *lpfs = _fs;

    return S_OK;
}

BOOL IsBackSpace(LPMSG pMsg)
{
    return pMsg && (pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_BACK);
}

extern int IsVK_TABCycler(MSG *pMsg);

//***
// NOTES
//  try ListView->TA first
//  then if that fails try WebView->TA iff it has focus.
//  then if that fails and it's a TAB we do WebView->UIAct
STDMETHODIMP CDefView::TranslateAccelerator(LPMSG pmsg)
{
    // 1st, try ListView
    if (_HasNormalView())
    {
        if (_fInLabelEdit)
        {
            // the second clause stops us passing mouse key clicks to the toolbar if we are in label edit mode...
            if (WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message)
            {
                // process this msg so the exploer does not get to translate
                TranslateMessage(pmsg);
                DispatchMessage(pmsg);
                return S_OK;            // we handled it
            }
            else
                return S_FALSE;
        }
        // If we are in classic mode and if it's a tab and the listview doesn't have focus already, receive the tab.
        else if (IsVK_TABCycler(pmsg) && !m_cFrame.IsSFVExtension() && !m_cFrame.IsWebView() && (GetFocus() != _hwndListview))
        {
            DefView_SetFocus(this);
            return S_OK;
        }


        if (GetFocus() == _hwndListview )        
        {
            if (::TranslateAccelerator(_hwndView, _hAccel, pmsg))
            {
                // we know we have a normal view, therefore this is
                // the right translate accelerator to use, otherwise the
                // common dialogs will fail to get any accelerated keys.
                return S_OK;
            }
            else if (WM_KEYDOWN == pmsg->message || WM_SYSKEYDOWN == pmsg->message)
            {
                // MSHTML eats these keys for frameset scrolling, but we
                // want to get them to our wndproc . . . translate 'em ourself
                //
                switch (pmsg->wParam)
                {
                case VK_LEFT:
                case VK_RIGHT:
                    // only go through here if alt is not down.
                    // don't intercept all alt combinations because
                    // alt-enter means something
                    // this is for alt-left/right compat with IE
                    if (GetAsyncKeyState(VK_MENU) < 0)
                        break;
                    // fall through
                    
                case VK_UP:
                case VK_DOWN:
                case VK_HOME:
                case VK_END:
                case VK_PRIOR:
                case VK_NEXT:
                case VK_RETURN:
                case VK_F10:
                    TranslateMessage(pmsg);
                    DispatchMessage(pmsg);
                    return S_OK;
                }
            }
        }
    }

    // 1.25th (CDTURNER)
    // to avoid passing it to the frame before SFV extension views...
    // SFVExtensions behave very much like the listview of defview, so they
    // must be given a chance to do this before we EVEN consider passing
    // it to frame, otherwise we are deep in it...
    if (m_cFrame.IsSFVExtension())
    {
        // if focus is in the view extension have a go at the frame accelerators
        // first so that the menu keys work....
        if (SHIsChildOrSelf(GetFocus(), m_cFrame.GetExtendedViewWindow()) == S_OK)
        {
            if (::TranslateAccelerator(_hwndView, _hAccel, pmsg))
            {
                return S_OK;
            }
        }
        if (this->m_cFrame.m_pActiveSVExt
                && this->m_cFrame.m_pActiveSVExt->TranslateAccelerator(pmsg) == S_OK)
        {
            return S_OK;
        }
    }

    // 1.5th, before we pass it down, see whether shell browser handles it.
    // we do this to make sure that webview has the same accelerator semantics
    // no matter what view(s) are active.
    // note that this is arguably inconsistent w/ the 'pass it to whoever has
    // focus'.
    //
    // however *don't* do this if:
    //   - we're in a dialog (in which case the buttons should come 1st)
    //   (comdlg's shellbrowser xxx::TA impl is broken it always does S_OK)
    //   - it's a TAB (which is always checked last)
    //   - it's a BACKSPACE (we should give the currently active object the first chance).
    //    However, in this case, we should call TranslateAcceleratorSB() AFTER we've tried
    //    calling TranslateAccelerator() on the currently active control (m_pActive) in
    //    m_cFrame->OnTranslateAccelerator().
    //
    // note: if you muck w/ this code careful not to regress the following:
    //  - ie41:62140: mnemonics broken after folder selected in organize favs
    //  - ie41:62419: TAB activates addr and menu if folder selected in explorer
    if (!DV_CDB_IsCommonDialog(this) && !IsVK_TABCycler(pmsg) && !IsBackSpace(pmsg))
        if (S_OK == _psb->TranslateAcceleratorSB(pmsg, 0))
            return S_OK;

    BOOL bTabOffLastTridentStop = FALSE;
    BOOL bHadIOFocus = (m_cFrame._HasFocusIO() == S_OK);  // Cache this here before the m_cFrame.OnTA() call below
    // 2nd, try WebView if it's active
    // note, SFVextensions are handled above....
    if (m_cFrame.IsWebView())
    {
        // Let the extension try it
        if (this->m_cFrame.OnTranslateAccelerator(pmsg, &bTabOffLastTridentStop) == S_OK)
        {
            return S_OK;
        }
    }

    // We've given m_pActive->TranslateAccelerator() the first shot in
    // m_cFrame.OnTranslateAccelerator, but it failed. Let's try the shell browser.
    if (IsBackSpace(pmsg))
        if (S_OK == _psb->TranslateAcceleratorSB(pmsg, 0))
            return S_OK;

    // 3rd, ???
    if (::TranslateAccelerator(_hwndView, _hAccel, pmsg))
        return S_OK;

    // 4th, if it's a TAB, cycle to next guy
    // hack: we fake a bunch of the TAB-activation handshaking
    if (IsVK_TABCycler(pmsg) && _HasNormalView() && m_cFrame.IsWebView())
    {
        HRESULT hr;
        BOOL fBack = (GetAsyncKeyState(VK_SHIFT) < 0);

        TraceMsg(TF_FOCUS, "cdv.ta(lpMsg=TAB)");
        if (!bHadIOFocus && bTabOffLastTridentStop)
        {
            // We were at the last tab stop in trident when the browser called defview->TA().
            // When we called TA() on trident above, it must've told us that we are tabbing
            // off the last tab stop (bTabOffLastTridentStop). This will leave us not setting focus
            // on anything. But, we have to set focus to something. We can do this by calling TA()
            // on trident again, which will set focus on the first tab stop again.
            return m_cFrame.OnTranslateAccelerator(pmsg, &bTabOffLastTridentStop);
        }
        else if (m_cFrame._HasFocusIO() == S_OK)
        {
            // ExtView has focus, and doesn't want the TAB.
            // this means we're TABing off of it.
            // no matter what, deactivate it (since we're TABing off).
            // if the view is next in the TAB order, (pseudo-)activate it,
            // and return S_OK since we've handled it.
            // o.w. return S_OK so our parent will activate whoever's next
            // in the TAB order.
            TraceMsg(TF_FOCUS, "cdv.ta: deact/uiaio ExtView");
            hr = m_cFrame._UIActivateIO(FALSE, NULL);
            ASSERT(hr == S_OK);

            // in web view listview already has focus so don't give it again
            // that's not the case with desktop
            if (fBack && _IsDesktop())
            {
                TraceMsg(TF_FOCUS, "cdv.ta: act/sf ListView ret S_OK");
                SetFocus(_hwndListview);
                return S_OK;
            }

            TraceMsg(TF_FOCUS, "cdv.ta: ret S_FALSE");
            return S_FALSE;
        }
        else
        {
            TraceMsg(TF_FOCUS, "cdv.ta: ExtView !act");
            if (!fBack)
            {
                TraceMsg(TF_FOCUS, "cdv.ta: act/uiaio ListView ret S_OK(?)");
                hr = m_cFrame._UIActivateIO(TRUE, pmsg);
                ASSERT(hr == S_OK || hr == S_FALSE);
                return hr;
            }
        }
    }

    return S_FALSE;
}

const UINT c_aiNonCustomizableFolders[] = {
    CSIDL_WINDOWS, 
    CSIDL_SYSTEM, 
    CSIDL_SYSTEMX86, 
    CSIDL_PROGRAM_FILES, 
    CSIDL_PROGRAM_FILESX86, 
    -1
};

// pass an array of CSIDL values (-1 terminated)

BOOL PathIsOneOf(const UINT rgFolders[], LPCTSTR pszFolder)
{
    for (int i = 0; rgFolders[i] != -1; i++)
    {
        TCHAR szParent[MAX_PATH];
        SHGetFolderPath(NULL, rgFolders[i] | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szParent);

        // the trailing slashes are assumed to match
        if (lstrcmpi(szParent, pszFolder) == 0)
            return TRUE;
    }
    return FALSE;
}

void CDefView::InitViewMenu(HMENU hmInit)
{
    UINT uiFlags = 0;
    int iCurViewMenuItem;

    iCurViewMenuItem = CheckCurrentViewMenuItem(hmInit);

    UINT uAAEnable = ((iCurViewMenuItem == SFVIDM_VIEW_LIST) ||
                    (iCurViewMenuItem == SFVIDM_VIEW_DETAILS) ||
                    (!_HasNormalView())) ?
                    (MF_GRAYED | MF_BYCOMMAND) : (MF_ENABLED | MF_BYCOMMAND);
    UINT uAGEnable = uAAEnable;

    if ( m_cFrame.IsSFVExtension() )
    {
        // ask the view extension...
        UINT dwSupport = SFVQS_AUTO_ARRANGE | SFVQS_ARRANGE_GRID;

        HRESULT hr = m_cFrame.GetExtendedISFV()->QuerySupport( & dwSupport );
        if ( hr == S_OK )
        {
            if ( dwSupport & SFVQS_AUTO_ARRANGE )
                uAAEnable = (MF_ENABLED | MF_BYCOMMAND );
            if ( dwSupport & SFVQS_ARRANGE_GRID )
                uAGEnable = (MF_ENABLED | MF_BYCOMMAND );
        }
    }

    if (!DV_SHOWICONS(this))
        uAAEnable = (MF_GRAYED | MF_BYCOMMAND);

    // The Folder Option "Classic style" and the shell restriction WIN95CLASSIC
    // should be the same. (Per ChristoB, otherwise admin's never understand what
    // the restriction means.) Since we want this to change DEFAULTs, and still
    // allow the user to turn on Web View, we don't remove the customize wizard here.
    if (_IsDesktop() || DV_CDB_IsCommonDialog(this))
    {
        int iIndex = MenuIndexFromID(hmInit, SFVIDM_VIEW_CUSTOMWIZARD);
        if (iIndex != -1)
        {
            DeleteMenu(hmInit, iIndex + 1, MF_BYPOSITION); // Remove Menu seperator
            DeleteMenu(hmInit, iIndex, MF_BYPOSITION);     // Remove Customize
        }
    }
    else
    {
        // Check if we already know if this folder is customizable!
        if (m_iCustomizable == DONTKNOW_IF_CUSTOMIZABLE)
        {
            m_iCustomizable = NOT_CUSTOMIZABLE;

            if (0 == SHRestricted(REST_NOCUSTOMIZETHISFOLDER))
            {
                // Check if this is a file system folder.
                // customization requires the folder being a regular file system
                // folder. FILESYSTEMANCESTOR is the key bit here

                #define SFGAO_CUST_BITS (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR)
                ULONG rgfFolderAttr = SFGAO_CUST_BITS;
                TCHAR szPath[MAX_PATH];
                if (SUCCEEDED(_GetNameAndFlags(SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), &rgfFolderAttr)) &&
                    (SFGAO_CUST_BITS == (rgfFolderAttr & SFGAO_CUST_BITS)))
                {
                    if (PathIsOneOf(c_aiNonCustomizableFolders, szPath))
                        m_iCustomizable = NOT_CUSTOMIZABLE;
                    else
                        m_iCustomizable = MAYBE_CUSTOMIZABLE;
                }
            }
        }

        if (m_iCustomizable == NOT_CUSTOMIZABLE || SHRestricted(REST_CLASSICSHELL))
        {
            int iIndex = MenuIndexFromID(hmInit, SFVIDM_VIEW_CUSTOMWIZARD);
            if (iIndex != -1)
            {
                DeleteMenu(hmInit, iIndex + 1, MF_BYPOSITION); // Remove Seperator
                DeleteMenu(hmInit, iIndex, MF_BYPOSITION);     // Remove Customize
            }
        }
        else
        {
            // Do we even have to do this any more?
            EnableMenuItem(hmInit, SFVIDM_VIEW_CUSTOMWIZARD, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    EnableMenuItem(hmInit, SFVIDM_ARRANGE_GRID, uAAEnable);
    EnableMenuItem(hmInit, SFVIDM_ARRANGE_AUTO, uAGEnable);

    // If we are in extended view and have an active child, get the folder
    // settings from the child.
    if (m_cFrame.IsSFVExtension())
    {
        uiFlags = 0;
        if (m_cFrame.GetExtendedISFV()->GetAutoArrange() == S_OK)
        {
            uiFlags = FWF_AUTOARRANGE;
        }
    }
    else
        uiFlags = _fs.fFlags;  //else, use the parent's flags.

    CheckMenuItem(hmInit, SFVIDM_ARRANGE_AUTO,
                  ((uAAEnable == (MF_ENABLED | MF_BYCOMMAND)) && (uiFlags & FWF_AUTOARRANGE)) ? MF_CHECKED : MF_UNCHECKED);
    DeleteMenu(hmInit, SFVIDM_ARRANGE_DISPLAYICONS, MF_BYCOMMAND);

    _SHPrettyMenu(hmInit);
    // extended views that don't have smarts can't auto arrange...
    // nor can we auto arrange if we aren't showing icons...
    if ((!_HasNormalView() && !m_cFrame.IsSFVExtension()) ||
        (!DV_SHOWICONS(this)))
    {
        EnableMenuItem(hmInit, SFVIDM_MENU_ARRANGE, MF_GRAYED | MF_BYCOMMAND);
    }
    else
    {
        EnableMenuItem(hmInit, SFVIDM_MENU_ARRANGE, MF_ENABLED | MF_BYCOMMAND);
    }
}


void DV_GetCBText(CDefView *pdsv, UINT_PTR id, UINT uMsgT, UINT uMsgA, UINT uMsgW, LPTSTR psz, UINT cch)
{
    *psz = 0;

    WCHAR szW[MAX_PATH];
    if (SUCCEEDED(pdsv->CallCB(uMsgW, MAKEWPARAM(id - SFVIDM_CLIENT_FIRST, ARRAYSIZE(szW)), (LPARAM)szW)))
        SHUnicodeToTChar(szW, psz, cch);
    else
    {
        char szA[MAX_PATH];
        if (SUCCEEDED(pdsv->CallCB(uMsgA, MAKEWPARAM(id - SFVIDM_CLIENT_FIRST, ARRAYSIZE(szA)), (LPARAM)szA)))
            SHAnsiToTChar(szA, psz, cch);
        else
            pdsv->CallCB(uMsgT, MAKEWPARAM(id - SFVIDM_CLIENT_FIRST, cch), (LPARAM)psz);
    }
}

void DV_GetMenuHelpText(CDefView *pdsv, UINT_PTR id, LPTSTR pszText, UINT cchText)
{
    VDATEINPUTBUF(pszText, TCHAR, cchText);
    *pszText = 0;

    if (InRange(id, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST) && pdsv->_pcmSel)
    {
        *pszText = 0;
        // First try to get the stardard help string
        pdsv->_pcmSel->GetCommandString(
                        id - SFVIDM_CONTEXT_FIRST, GCS_HELPTEXT, NULL,
                        (LPSTR)pszText, cchText);
        if (*pszText == 0)
        {
            // If we didn't get anything, try to grab the other version of the
            // (ansi for a unicode build, or unicode for an ansi build)
#ifdef UNICODE
            CHAR szText[MAX_PATH];
            szText[0] = 0;   // Don't start with garbage in case of failure...
            pdsv->_pcmSel->GetCommandString(
                        id - SFVIDM_CONTEXT_FIRST, GCS_HELPTEXTA, NULL,
                        szText, ARRAYSIZE(szText));
            SHAnsiToUnicode(szText, pszText, cchText);
#else
            WCHAR szText[MAX_PATH];
            szText[0] = 0;   // Don't start with garbage in case of failure...
            pdsv->_pcmSel->GetCommandString(
                        id - SFVIDM_CONTEXT_FIRST, GCS_HELPTEXTW, NULL,
                        (LPSTR)szText, ARRAYSIZE(szText));
            SHUnicodeToAnsi(szText, pszText, cchText);
#endif
        }

#ifdef SN_TRACE
        if (GetKeyState(VK_CONTROL) < 0)
        {
            UINT cch;
            lstrcat(pszText, TEXT(" (debug only) Canonical Verb = "));
            cch = lstrlen(pszText);
            pdsv->_pcmSel->GetCommandString(
                            id - SFVIDM_CONTEXT_FIRST, GCS_VERB, NULL,
                            (LPSTR)(pszText+cch), cchText-cch);
            if (*(pszText+cch)==TEXT('\0'))
            {
#ifdef UNICODE
                CHAR szVerb[MAX_PATH];
                pdsv->_pcmSel->GetCommandString(
                            id - SFVIDM_CONTEXT_FIRST, GCS_VERBA, NULL,
                            szVerb, ARRAYSIZE(szVerb));
                SHAnsiToUnicode(szVerb, pszText + cch, cchText - cch);
#else
                WCHAR szVerb[MAX_PATH];
                pdsv->_pcmSel->GetCommandString(
                            id - SFVIDM_CONTEXT_FIRST, GCS_VERBW, NULL,
                            (LPSTR)szVerb, ARRAYSIZE(szVerb));
                SHUnicodeToAnsi(szVerb, pszText + cch, cchText - cch);
#endif
            }
        }
#endif
    }
    else if (InRange(id, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && pdsv->HasCB())
    {
        DV_GetCBText(pdsv, id, SFVM_GETHELPTEXT, SFVM_GETHELPTEXTA, SFVM_GETHELPTEXTW, pszText, cchText);
    }
    else if (InRange(id, SFVIDM_VIEW_EXTFIRST, SFVIDM_VIEW_EXTLAST))
    {
        pdsv->m_cFrame._StringFromView(pdsv->m_cFrame.UidFromCmdId((UINT) id), pszText, cchText, ID_EXTVIEWHELPTEXT);
    }
    else if (InRange(id, SFVIDM_FIRST, SFVIDM_LAST))
    {
        if ((id == SFVIDM_EDIT_UNDO) && IsUndoAvailable()) {
            GetUndoText(PeekUndoAtom(), pszText, cchText, UNDO_STATUSTEXT);
        } else {
            LoadString(HINST_THISDLL, (UINT)(id + SFVIDS_MH_FIRST), pszText, cchText);
        }
    }
}

#ifdef UNICODE
#define DV_GetToolTipTextW  DV_GetToolTipText
#else
void DV_GetToolTipTextW(CDefView *pdsv, UINT_PTR id, LPWSTR pwzText, UINT cchText)
{
    LPTSTR pszText = (LPTSTR) LocalAlloc(LPTR, cchText * SIZEOF(TCHAR));
    if (pszText)
    {
        DV_GetToolTipText(pdsv, id, pszText, cchText);
        SHTCharToUnicode(pszText, pwzText, cchText);
        LocalFree(pszText);
    }
}
#endif

void DV_GetToolTipText(CDefView *pdsv, UINT_PTR id, LPTSTR pszText, UINT cchText)
{
    VDATEINPUTBUF(pszText, TCHAR, cchText);
    *pszText = 0;

    if (InRange(id, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && pdsv->HasCB())
    {
        DV_GetCBText(pdsv, id, SFVM_GETTOOLTIPTEXT, SFVM_GETTOOLTIPTEXTA, SFVM_GETTOOLTIPTEXTW, pszText, cchText);
    }
    else if (InRange(id, SFVIDM_VIEW_EXTFIRST, SFVIDM_VIEW_EXTLAST))
    {
        pdsv->m_cFrame._StringFromView(pdsv->m_cFrame.UidFromCmdId((UINT) id), pszText, cchText, ID_EXTVIEWTTTEXT);
    }
    else if (InRange(id, SFVIDM_FIRST, SFVIDM_LAST))
    {
        if (id == SFVIDM_EDIT_UNDO)
        {
            if (IsUndoAvailable())
            {
                GetUndoText(PeekUndoAtom(), pszText, cchText, UNDO_MENUTEXT);
                return;
            }
        }
        LoadString(HINST_THISDLL, (UINT)(IDS_TT_SFVIDM_FIRST + id), pszText, cchText);
    }
    else
    {
        // REVIEW: This might be an assert situation: missing tooltip info...
        TraceMsg(TF_WARNING, "DV_GetToolTipText: tip request for unknown object");
    }
}



LRESULT DefView_OnMenuSelect(CDefView *pdsv, UINT id, UINT mf, HMENU hmenu)
{
    TCHAR szHelpText[80 + 2*MAX_PATH];   // Lots of stack!

    // If we dismissed the edit restore our status bar...
    if (!hmenu && LOWORD(mf)==0xffff)
    {
        pdsv->_psb->SendControlMsg(
                FCW_STATUS, SB_SIMPLE, 0, 0, NULL);
        return 0;
    }


    if (mf & (MF_SYSMENU | MF_SEPARATOR))
        return 0;


    szHelpText[0] = 0;   // in case of failures below

    if (mf & MF_POPUP)
    {
        MENUITEMINFO miiSubMenu;

        miiSubMenu.cbSize = SIZEOF(MENUITEMINFO);
        miiSubMenu.fMask = MIIM_ID;
        miiSubMenu.cch = 0;     // just in case

        if (!GetMenuItemInfo(hmenu, id, TRUE, &miiSubMenu))
            return 0;

        // Change the parameters to simulate a "normal" menu item
        id = miiSubMenu.wID;
        mf &= ~MF_POPUP;
    }

    // a-msadek; needed only for BiDi Win95 loc
    // Mirroring will take care of that over NT5 & BiDi Win98
    if(g_bBiDiW95Loc)
    {
        szHelpText[0] = szHelpText[1] = TEXT('\t');
        szHelpText[2] = TEXT('\0');
        DV_GetMenuHelpText(pdsv, id, &szHelpText[2], ARRAYSIZE(szHelpText)-2);

        pdsv->_psb->SendControlMsg(
            FCW_STATUS, SB_SETTEXT, SBT_RTLREADING | SBT_NOBORDERS | 255, (LPARAM)szHelpText, NULL);
    }
    else                         
    {
        DV_GetMenuHelpText(pdsv, id, szHelpText, ARRAYSIZE(szHelpText));
        pdsv->_psb->SendControlMsg(
            FCW_STATUS, SB_SETTEXT, SBT_NOBORDERS | 255, (LPARAM)szHelpText, NULL);
    }
    pdsv->_psb->SendControlMsg(FCW_STATUS, SB_SIMPLE, 1, 0, NULL);

    return 0;
}

//
// This function dismisses the name edit mode if there is any.
//
// REVIEW: Moving the focus away from the edit window will
//  dismiss the name edit mode. Should we introduce
//  a LV_DISMISSEDIT instead?
//
void DefView_DismissEdit(CDefView *pdsv)
{
    if (pdsv->_uState == SVUIA_ACTIVATE_FOCUS)
    {
        HWND hwndFocus = GetFocus();
        if (hwndFocus && pdsv->_hwndListview && GetParent(hwndFocus)==pdsv->_hwndListview)
        {
            SetFocus(pdsv->_hwndListview);
        }
    }
}

void DefView_OnInitMenu(CDefView *pdsv)
{
    //
    // We need to dismiss the edit mode if it is any.
    //
    DefView_DismissEdit(pdsv);
}

void _RemoveContextMenuItems(HMENU hmInit)
{
    int i;

    for (i = GetMenuItemCount(hmInit) - 1; i >= 0; --i)
    {
        MENUITEMINFO mii;
        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_ID | MIIM_ID;
        mii.cch = 0;     // just in case

        if (GetMenuItemInfo(hmInit, i, TRUE, &mii))
        {
            if (InRange(mii.wID, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST) ||
                        InRange(mii.wID, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST))
            {
                TraceMsg(TF_DEFVIEW, "OnInitMenuPopup: setting bDeleteItems at %d, %d", i, mii.wID);
                //bDeleteItems = TRUE;
                DeleteMenu(hmInit, i, MF_BYPOSITION);
            }
        }
    }
}

BOOL HasClientItems(HMENU hmenu)
{
    int cItems = GetMenuItemCount(hmenu);
    for (int i=0; i < cItems; i++)
    {
        UINT id = GetMenuItemID(hmenu, i);

        if (InRange(id, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST))
            return TRUE;
    }
    return FALSE;
}

LRESULT CDefView::OnInitMenuPopup(HMENU hmInit, int nIndex, BOOL fSystemMenu)
{
    MENUITEMINFO mii;
    ULONG dwAttr;

    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_SUBMENU|MIIM_ID;
    mii.cch = 0;     // just in case

    if (!_hmenuCur)
        return 1;

    if (!GetMenuItemInfo(_hmenuCur, nIndex, TRUE, &mii) ||
        mii.hSubMenu != hmInit)
    {
        // if we failed to get mii or the menu we are about to show
        // is some sub sub menu (definitely not file, edit or view --
        // which are the only ones we care about bellow) check if there
        // is a callback and if there are any items added by it and do
        // the notify
        if ( this->HasCB() && HasClientItems(hmInit))
            goto NotifyClient;
            
        return 1;
    }

    switch (mii.wID)
    {
    case FCIDM_MENU_FILE:
        //
        // Don't touch the file menu unless we have the focus.
        //
        if (_uState == SVUIA_ACTIVATE_FOCUS)
        {
            //DWORD dwMenuState;
            //BOOL bDeleteItems = FALSE; //what is this used for?
            IContextMenu *pcmSel;
            IContextMenu *pcmBack;
            //
            // First of all, remove all the menu items we've added.
            //
            _RemoveContextMenuItems(hmInit);

            // don't remove any separator! it could be named one and we need it!
            // for this we call shprettymenu!
            //dwMenuState = GetMenuState(hmInit, 0, MF_BYPOSITION);
            //if ((dwMenuState & MF_SEPARATOR) &&
            //    !(dwMenuState & MF_POPUP))
            //{
            //    DeleteMenu(hmInit, 0, MF_BYPOSITION);
            //}

            // Let the object add the separator.

            //
            // Now add item specific commands to the menu
            // This is done by seeing if we already have a context menu
            // object for our selection.  If not we generate it now.
            //
            // since we're now adding extensions to both file and edit menus
            // we can no longer use cached context menu so free it
            DV_FlushCachedMenu(this);

            pcmSel = _GetContextMenuFromSelection();
            //ASSERT(pcmSel == _pcmSel || m_cFrame.IsWebView());

            if (SUCCEEDED(_pshf->CreateViewObject(_hwndMain,IID_IContextMenu,
                    (void **)&pcmBack)))
            {
                if (!pcmSel)
                {
                    pcmSel = pcmBack;
                    pcmBack = NULL;
                }

                if (pcmBack)
                {
                    if (_pcmBackground == NULL) 
                    {
                        // cache the contextmenu so that the status bar helptext
                        // and the command execution for menu items in a viewextension works
                        _pcmBackground = pcmBack;

                        IUnknown_SetSite(pcmBack, SAFECAST(this,IShellView2*));
                        pcmBack->QueryContextMenu(
                            hmInit, 0, SFVIDM_BACK_CONTEXT_FIRST,
                            SFVIDM_BACK_CONTEXT_LAST, CMF_DVFILE|CMF_NODEFAULT);
                        // assumed refcount of pcmBack in _pcmBackground
                    } 
                    else
                        ATOMICRELEASE(pcmBack);
                }
            }

            if (NULL != pcmSel)
            {
                if (_pcmSel == NULL)
                {
                    // cache the contextmenu so that the status bar helptext
                    // and the command execution for menu items in a viewextension works
                    _pcmSel = pcmSel;
                    pcmSel->AddRef();
                }

                IUnknown_SetSite(pcmSel, SAFECAST(this, IShellView2*));

                pcmSel->QueryContextMenu(hmInit, 0, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST, CMF_DVFILE | CMF_NODEFAULT);

                ATOMICRELEASE(pcmSel);
            }

            //
            // Enable/disable menuitems in the "File" pulldown.
            //
            dwAttr = DefView_GetAttributesFromSelection(this,
                SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_CANLINK | SFGAO_HASPROPSHEET);
            Def_InitFileCommands(dwAttr, hmInit, SFVIDM_FIRST, FALSE);

            _SHPrettyMenu(hmInit);
        } 
        else if (_uState == SVUIA_ACTIVATE_NOFOCUS && _pcmBackground == NULL) 
        {
            IContextMenu *pcmBack;
            if (SUCCEEDED(_pshf->CreateViewObject(_hwndMain,IID_IContextMenu,
                    (void **)&pcmBack)))
            {
                if (_pcmBackground == NULL)
                {
                    // cache the contextmenu so that the status bar helptext
                    // and the command execution for menu items in a viewextension works
                    _pcmBackground = pcmBack;

                    IUnknown_SetSite(pcmBack, SAFECAST(this,IShellView2*));
                    pcmBack->QueryContextMenu(hmInit, 0, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST, CMF_DVFILE);

                    // assumed refcount of pcmBack in _pcmBackground
                } 
                else
                    ATOMICRELEASE(pcmBack);
            }
        }

        _SHPrettyMenu(hmInit);

        break;

    case FCIDM_MENU_EDIT:
        //
        // Enable/disable menuitems in the "Edit" pulldown.
        //
        dwAttr = DefView_GetAttributesFromSelection(this, SFGAO_CANCOPY|SFGAO_CANMOVE);

        Def_InitEditCommands(dwAttr, hmInit, SFVIDM_FIRST, _pdtgtBack, 0);

        // Extended views are dumb if they have no DefViewActiveObject
        if (!_HasNormalView())
        {
            // if we are a view extension, query it so see what is supported
            if (m_cFrame.IsSFVExtension())
            {
                UINT dwSupport = SFVQS_SELECT_ALL | SFVQS_SELECT_INVERT;
                HRESULT hr = m_cFrame.GetExtendedISFV()->QuerySupport(&dwSupport);

                UINT uSelectAll;
                UINT uInvertSel;

                if (hr != S_OK)
                    dwSupport = 0;

                uSelectAll = (dwSupport & SFVQS_SELECT_ALL ) ?
                                MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND;
                uInvertSel = (dwSupport & SFVQS_SELECT_INVERT ) ?
                                MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND;

                EnableMenuItem(hmInit, SFVIDM_SELECT_ALL, uSelectAll);
                EnableMenuItem(hmInit, SFVIDM_SELECT_INVERT, uInvertSel);
            }
            else
            {
                UINT uEnable = (!_HasNormalView()) ?
                    (MF_GRAYED | MF_BYCOMMAND)  :  (MF_ENABLED | MF_BYCOMMAND);

                EnableMenuItem(hmInit, SFVIDM_SELECT_ALL, uEnable);
                EnableMenuItem(hmInit, SFVIDM_SELECT_INVERT, uEnable);
            }
        }

        _SHPrettyMenu(hmInit);

        break;

    case FCIDM_MENU_VIEW:
        InitViewMenu(hmInit);
        break;
    }

    // do the notify if there is a callback and it added some items.
    // before we used to do this only in special case, e.g. for file
    // menu when defview has focus (SVUIA_ACTIVATE_FOCUS), but now
    // we do that all the time to give callback a chance to enable/
    // disable items it added. (e.g. recycle bin needs to enable/disable
    // Empty Recycle bin item no matter who has the focus.
    if (this->HasCB() && HasClientItems(hmInit))
    {
NotifyClient:
        // Yes; pass it on to the callback
        CallCB(SFVM_INITMENUPOPUP,
                     MAKEWPARAM(SFVIDM_CLIENT_FIRST, nIndex),
                     (LPARAM)hmInit);
    }

    return 0;
}


//
// Member:  CDefView::AddPropertySheetPages
//
STDMETHODIMP CDefView::AddPropertySheetPages(
    IN DWORD dwReserved,
    IN LPFNADDPROPSHEETPAGE lpfn,
    IN LPARAM lParam)
{
    SFVM_PROPPAGE_DATA data;

    ASSERT(IS_VALID_CODE_PTR(lpfn, FNADDPROPSHEETPAGE));

    data.dwReserved = dwReserved;
    data.pfn        = lpfn;
    data.lParam     = lParam;

    // try any view extensions that are loaded ...
    if (m_cFrame.IsSFVExtension())
    {
        m_cFrame.GetExtendedISV()->AddPropertySheetPages(dwReserved, lpfn, lParam);

        // ignore the return result as we don't really care if they added any or not...
    }

    // Call the callback to add pages
    CallCB(SFVM_ADDPROPERTYPAGES, 0, (LPARAM)&data);

    return S_OK;
}

HRESULT CDefView::_SaveViewState (IStream *pstm)

{
    HRESULT hres;
    LARGE_INTEGER dlibMove = {0, 0};
    ULARGE_INTEGER libCurPosition;
    ULONG ulWrite;
    struct {
        DVSAVEHEADER dvSaveHeader;
        DVSAVEHEADEREX dvSaveHeaderEx;
    } dv;
    BOOL bDefaultCols;

    // Position the stream right after the headers, and save the starting
    // position at the same time
    dlibMove.LowPart = SIZEOF(dv);
    hres = pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libCurPosition);
    if (FAILED(hres))
        return(hres);       // ungraceful exit - let the caller clean up pstm
    // HACK: Avoid 2 calls to seek by just subtracting
    libCurPosition.LowPart -= SIZEOF(dv);

    // Save column order and size info
    bDefaultCols = SaveCols(pstm);

    ZeroMemory(&dv, SIZEOF(dv));
    dv.dvSaveHeader.cbSize = SIZEOF(dv.dvSaveHeader);
    // We save the view mode to determine if the scroll positions are
    // still valid on restore
    dv.dvSaveHeader.ViewMode = _fs.ViewMode;
    dv.dvSaveHeader.ptScroll.x = (SHORT) GetScrollPos(_hwndListview, SB_HORZ);
    dv.dvSaveHeader.ptScroll.y = (SHORT) GetScrollPos(_hwndListview, SB_VERT);
    dv.dvSaveHeader.dvState = _dvState;

    // dvSaveHeaderEx.cbColOffset holds the true offset.
    // Win95 gets confused when cbColOffset points to the new
    // format. Zeroing this out tells Win95 to use default widths
    // (after uninstall of ie40).
    //
    // dv.dvSaveHeader.cbColOffset = 0;

    dv.dvSaveHeaderEx.dwSignature = DVSAVEHEADEREX_SIGNATURE;
    dv.dvSaveHeaderEx.cbSize = SIZEOF(dv.dvSaveHeaderEx);
    dv.dvSaveHeaderEx.wVersion = DVSAVEHEADEREX_VERSION;


    // Check whether everything is the default; clear out the stream if so
    if (_bClearItemPos && IsDefaultState(dv.dvSaveHeader) && bDefaultCols)
    {
        goto SetSize;
    }

    if (bDefaultCols)
    {
        // No need to save column info
        dv.dvSaveHeaderEx.cbColOffset = 0;
        dv.dvSaveHeader.cbPosOffset = SIZEOF(dv);
        dlibMove.LowPart = libCurPosition.LowPart + dv.dvSaveHeader.cbPosOffset;
        pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    }
    else
    {
        ULARGE_INTEGER libPosPosition;
        dlibMove.LowPart = 0;
        pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libPosPosition);
        dv.dvSaveHeaderEx.cbColOffset = SIZEOF(dv);
        dv.dvSaveHeader.cbPosOffset = (USHORT)(libPosPosition.LowPart - libCurPosition.LowPart);
    }

    // Save potision info, currently stream is positioned immediately after column info
    hres = SavePos(pstm);
    if (FAILED(hres))
    {
        goto SetSize;
    }

    // Win95 expects cbPosOffset to be at the end of the stream --
    // don't change it's value and never store anything after
    // the position information.

    // Calculate size of total information saved.
    // This is needed when we read the stream.
    {
        ULARGE_INTEGER libEndPosition;
        dlibMove.LowPart = 0;
        pstm->Seek(dlibMove, STREAM_SEEK_CUR, &libEndPosition);
        ASSERT(0 == libEndPosition.HighPart);
        dv.dvSaveHeaderEx.cbStreamSize = (DWORD)(libEndPosition.LowPart - libCurPosition.LowPart);
    }

    // Now save the header information
    dlibMove.LowPart = libCurPosition.LowPart;
    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    hres = pstm->Write(&dv, SIZEOF(dv), &ulWrite);
    if (FAILED(hres) || ulWrite != SIZEOF(dv))
    {
        goto SetSize;
    }

    // Make sure we save all information written so far
    libCurPosition.LowPart += dv.dvSaveHeaderEx.cbStreamSize;

SetSize:
    dlibMove.LowPart = libCurPosition.LowPart;
    pstm->Seek(dlibMove, STREAM_SEEK_SET, NULL);
    pstm->SetSize(libCurPosition);
    hres = S_OK;

    return(hres);
}

//
// Member:  CDefView::SaveViewState
//
STDMETHODIMP CDefView::SaveViewState()
{
    IStream *pstm;
    HRESULT hres;

    // do we have any view extensions...
    // REVIEW: what stops each one overwritting the data from each other ?
    // if ( m_cFrame.IsSFVExtension() )
    // {
    //    m_cFrame.GetExtendedISV()->SaveViewState();
    // }
    // NOTE: This has been commented out until the SaveViewState() mechanism changes...

    hres = _psb->GetViewStateStream(STGM_WRITE, &pstm);
    if (FAILED(hres))
    {
        // There are cases where we may not save out the complete view state
        // but we do want to save out the column information (like Docfind...)
        SaveCols(NULL);
        return hres;
    }

    hres = _SaveViewState(pstm);
    ATOMICRELEASE(pstm);

    return hres;
}



HRESULT CDefView::_GetStorageStream (DWORD grfMode, IStream* *ppIStream)

//  99/02/05 #226140 vtan: Function used to get the storage
//  stream for the default view state of the current DefView.
//  Typically this will be CLSID_ShellFSFolder but can be
//  others.

{
    *ppIStream = NULL;

    CLSID clsid;
    HRESULT hres = IUnknown_GetClassID(_pshf, &clsid);
    if (SUCCEEDED(hres))
    {
        TCHAR szCLSID[64];      // enough for the CLSID

        if (IsEqualGUID(CLSID_MyDocuments, clsid))
            clsid = CLSID_ShellFSFolder;

        TINT(SHStringFromGUID(clsid, szCLSID, ARRAYSIZE(szCLSID)));
        *ppIStream = OpenRegStream(HKEY_CURRENT_USER,
                                   REGSTR_PATH_EXPLORER TEXT("\\Streams\\Defaults"), szCLSID, grfMode);
        if (*ppIStream == NULL)
            hres = E_FAIL;
    }
    return hres;
}

HRESULT CDefView::_SaveGlobalViewState (void)

//  99/02/05 #226140 vtan: Function called from DefView's
//  implementation of IOleCommandTarget::Exec() which is
//  invoked from CShellBrowser2::SetAsDefFolderSettings().

{
    HRESULT     hResult;
    IStream     *pIStream;

    hResult = _GetStorageStream(STGM_WRITE, &pIStream);
    if (pIStream != NULL)
    {
        hResult = _SaveViewState(pIStream);
        pIStream->Release();
    }
    return(hResult);
}

HRESULT CDefView::_LoadGlobalViewState (IStream* *ppIStream)

//  99/02/05 #226140 vtan: Function called from
//  _GetSaveHeader to get the default view state
//  for this class.

{
    return(_GetStorageStream(STGM_READ, ppIStream));
}

HRESULT CDefView::_ResetGlobalViewState (void)

//  99/02/09 #226140 vtan: Function used to reset the
//  global view states stored by deleting the key
//  that stores all of them.

{
    return(RegDeleteKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\Streams\\Defaults")));
}


BOOL CDefView::_IsGhosted(LPCITEMIDLIST pidl)
{
    DWORD uFlags = SFGAO_GHOSTED;
    if (S_OK == _pshf->GetAttributesOf(1, &pidl, &uFlags))
    {
       if (uFlags & SFGAO_GHOSTED)
          return TRUE;
    }
    return FALSE;
}


void CDefView::_RestoreAllGhostedFileView()
{
   UINT i , j;

   ListView_SetItemState(_hwndListview, -1, 0, LVIS_CUT);

   i = ListView_GetItemCount(_hwndListview);

   for (j = 0; j < i; ++j)
   {
       if (_IsGhosted((LPCITEMIDLIST)_GetPIDL(j)))
       {
           ListView_SetItemState(_hwndListview, j, LVIS_CUT, LVIS_CUT);
       }
   }
   return;
}


HRESULT CDefView::SelectAndPositionItem(LPCITEMIDLIST pidlItem, UINT uFlags, POINT *ppt)
{
    int i;

    if (m_cFrame.IsSFVExtension())
    {
        return m_cFrame.GetExtendedISV()->SelectAndPositionItem(pidlItem, uFlags, ppt);
    }

    // See if we should first deselect everything else
    if (!pidlItem)
    {
        if (uFlags != SVSI_DESELECTOTHERS)
        {
            // I only know how to deselect everything
            return(E_INVALIDARG);
        }

        ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);
        _RestoreAllGhostedFileView();
        return S_OK;
    }

    RIP(ILFindLastID(pidlItem) == pidlItem);
    if (ILFindLastID(pidlItem) != pidlItem)
    {
        return E_INVALIDARG;
    }

    if (uFlags & SVSI_TRANSLATEPT)
    {
        //The caller is asking us to take this point and convert it from screen Coords
        // to the Client of the Listview.

        LVUtil_ScreenToLV(_hwndListview, ppt);
    }

    i = _FindItem(pidlItem, NULL, FALSE);
    if (i != -1)
    {
        // set the position first so that the ensure visible scrolls to
        // the new position
        if (ppt)
        {
            ListView_SetItemPosition32(_hwndListview, i, ppt->x, ppt->y);

            _bItemsMoved = TRUE;
            _bClearItemPos = FALSE;
        }

        // The SVSI_EDIT flag also contains SVSI_SELECT and as such
        // a simple & wont work!
        if ((uFlags & SVSI_EDIT) == SVSI_EDIT)
        {
            // Grab focus if the listview (or any of it's children) don't already have focus
            HWND hwndFocus = GetFocus();
            if (SHIsChildOrSelf(_hwndListview, hwndFocus) != S_OK)
                SetFocus(_hwndListview);

            ListView_EditLabel(_hwndListview, i);
        }
        else
        {
            UINT stateMask = LVIS_SELECTED;
            UINT state = (uFlags & SVSI_SELECT) ? LVIS_SELECTED : 0;
            if (uFlags & SVSI_FOCUSED)
            {
                state |= LVIS_FOCUSED;
                stateMask |= LVIS_FOCUSED;
            }

            // See if we should first deselect everything else
            if (uFlags & SVSI_DESELECTOTHERS)
            {
               ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);
               _RestoreAllGhostedFileView();
            }

            ListView_SetItemState(_hwndListview, i, state, stateMask);

            if (uFlags & SVSI_ENSUREVISIBLE)
                ListView_EnsureVisible(_hwndListview, i, FALSE);

            // BUGBUG:: we should only set focus when SVUIA_ACTIVATE_FOCUS 
            // bug fixing that might break find target code
            if (uFlags & SVSI_FOCUSED)
                SetFocus(_hwndListview);
        }

        // BUGBUG:: Is this call needed here?  There was an ILFree of pidlSelect here.
        _ClearSelectList();
        return S_OK;
    }

    return E_FAIL;
}
//
// Member:  CDefView::SelectItem
//
STDMETHODIMP CDefView::SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags)
{
    // if we're filling in the background, this item may not be visible yet.
    // if we're an extended view, we don't know how to do anything
    // defer this action until later.
    // Likewise if we are in the process of being created we should defer.
    if (_fDeferSelect())
    {
        if (!_hdsaSelect)
        {
            _hdsaSelect = DSA_Create(sizeof(DVDelaySelItem), 4);
            if (!_hdsaSelect)
                return E_OUTOFMEMORY;
        }

        DVDelaySelItem dvdsi;
        dvdsi.pidl = ILClone(pidlItem);
        if (!dvdsi.pidl)
            return E_OUTOFMEMORY;
        dvdsi.uFlagsSelect = uFlags;

        if (DSA_AppendItem(_hdsaSelect, &dvdsi) == DSA_ERR)
        {
            ILFree(dvdsi.pidl);
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

    // if we are looking at an extension view then tell it to select something...
    if (m_cFrame.IsSFVExtension())
        return this->m_cFrame.GetExtendedISV()->SelectItem(pidlItem, uFlags);
    else
        return SelectAndPositionItem(pidlItem, uFlags, NULL);
}


void CDefView::_ClearSelectList()
{
    if (_hdsaSelect)
    {
        HDSA hdsa = _hdsaSelect;
        _hdsaSelect = NULL;
        int cItems = DSA_GetItemCount(hdsa);
        int i;
        for (i = 0; i < cItems; i++)
        {
            DVDelaySelItem *pdvdsi = (DVDelaySelItem*)DSA_GetItemPtr(hdsa, i);
            if (pdvdsi)
                ILFree(pdvdsi->pidl);
        }
        DSA_Destroy(hdsa);
    }
}

// Call this whenever the state changes such that SelectItem (above)
void CDefView::SelectSelectedItems()
{
    if (_hdsaSelect && !_fDeferSelect())
    {
        HDSA hdsa = _hdsaSelect;
        _hdsaSelect = NULL;
        int cItems = DSA_GetItemCount(hdsa);
        int i;
        for (i = 0; i < cItems; i++)
        {
            DVDelaySelItem *pdvdsi = (DVDelaySelItem*)DSA_GetItemPtr(hdsa, i);
            if (pdvdsi)
            {
                this->SelectItem(pdvdsi->pidl, pdvdsi->uFlagsSelect);
                ILFree(pdvdsi->pidl);
            }
        }
        DSA_Destroy(hdsa);
    }
}

//
// To be called back from within CDefFolderMenu
//
// Returns:
//      S_OK, if successfully processed.
//      (S_FALSE), if default code should be used.
//
HRESULT CALLBACK DefView_DFMCallBackBG(IShellFolder *psf, HWND hwndOwner,
        IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres;

    switch(uMsg) 
    {
    case DFM_VALIDATECMD:
    case DFM_INVOKECOMMAND:
        hres = S_FALSE;
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }

    return hres;
}

HRESULT CDefView::_GetIPersistHistoryObject(IPersistHistory **ppph)
{
    // See to see if specific folder wants to handle it...
    HRESULT hr;

    hr = CallCB(SFVM_GETIPERSISTHISTORY, 0, (LPARAM)ppph);
    if (FAILED(hr))
    {
        // Here we can decide if we want to default should be to always save
        // the default defview stuff or not.  For now we will assume that we do
        if (ppph)
        {
            CDefViewPersistHistory *pdvph;
            pdvph = new CDefViewPersistHistory();
            if (pdvph)
            {
                hr = pdvph->QueryInterface(IID_IPersistHistory, (void**)ppph);
                pdvph->Release();
            }
            else 
            {
                *ppph = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
        else
            hr = S_FALSE;   // still succeeds but can detect on other side if desired...
    }
    return hr;
}


//
// Member:  CDefView::GetItemObject
//
STDMETHODIMP CDefView::GetItemObject(UINT uItem, REFIID riid, void **ppv)
{
    *ppv = NULL;

    switch (uItem)
    {
    case SVGIO_BACKGROUND:
        if (IsEqualIID(riid, IID_IContextMenu) ||
            IsEqualIID(riid, IID_IContextMenu2) ||
            IsEqualIID(riid, IID_IContextMenu3) )
        {
            return CBackgrndMenu_CreateInstance( this, riid, ppv );
        }

        if (IsEqualIID(riid, IID_IDispatch) ||
            IsEqualIID(riid, IID_IDefViewScript))
        {
            if (!m_pauto)
            {
                // try to create an Instance of the Shell disipatch for folder views...
                HRESULT hres = SHExtCoCreateInstance(NULL, &CLSID_ShellFolderView, NULL,
                                                  IID_IDispatch, (void **)&m_pauto);
                if (SUCCEEDED(hres))
                {
                    IShellService *pss;
                    hres = m_pauto->QueryInterface(IID_IShellService, (void **)&pss);
                    if (SUCCEEDED(hres))
                    {
                        // Pass a ref to ourselves
                        pss->SetOwner((IShellFolderView *)this);
                        pss->Release();
                    }
                    else
                    {
                        // Failure...
                        ATOMICRELEASE(m_pauto);
                        ASSERT(m_pauto == NULL);
                    }
                }
            }

            // return the IDispath interface.
            if (m_pauto)
                return m_pauto->QueryInterface(riid, ppv);

            break;
        }

        if (IsEqualIID(riid, IID_IPersistHistory))
        {
            // See if the folder wants a chance at this.  The main
            // case for this is the search results windows.
            // BUGBUG:: what if DVOC also wants a crack?
            _GetIPersistHistoryObject((IPersistHistory**)ppv);
            if (*ppv)
            {
                IUnknown_SetSite((IUnknown*)*ppv, SAFECAST(this, IShellView2*));
                return S_OK;
            }
        }

        // fall through...

        // we don't know what it is, maybe our extended view does
        if (m_cFrame.IsWebView() && m_cFrame.m_pOleObj)
            return m_cFrame.m_pOleObj->QueryInterface(riid, ppv);

        break;

    case SVGIO_ALLVIEW:
        if (_hwndStatic)
        {
            DECLAREWAITCURSOR;

            SetWaitCursor();

            do
            {
                // If _hwndStatic is around, we must be filling the
                // view in a background thread, so we will peek for
                // messages to it (so SendMessages will get through)
                // and dispatch only _hwndStatic messages so we get the
                // animation effect.
                // Note there is no timeout, so this could take
                // a while on a slow link, but there really isn't
                // much else I can do

                MSG msg;

                // Since _hwndStatic can only be destroyed on a DESTROYSTATIC
                // message, we should never get a RIP
                if (PeekMessage(&msg, _hwndView, WM_DSV_DESTROYSTATIC,
                                WM_DSV_DESTROYSTATIC, PM_REMOVE) ||
                    PeekMessage(&msg, _hwndStatic, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } while (_hwndStatic);

            ResetWaitCursor();
        }

        // Fall through

    case SVGIO_SELECTION:
        // if we are looking at a view extension, then ask it for the selection
        if (m_cFrame.IsSFVExtension())
            return m_cFrame.GetExtendedISV()->GetItemObject(uItem, riid, ppv);

        // hitting this is bad, cuz we wont answer correctly...
        ASSERT(_HasNormalView());

        return DefView_GetUIObjectFromItem(this, riid, ppv, uItem);
    }

    return E_NOTIMPL;
}


STDMETHODIMP CDefView::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDefView, IShellView2),                    // IID_IShellView2
        QITABENTMULTI(CDefView, IShellView, IShellView2),   // IID_IShellView
        QITABENT(CDefView, IViewObject),                    // IID_IViewObject
        QITABENT(CDefView, IDropTarget),                    // IID_IDropTarget
        QITABENT(CDefView, IShellFolderView),               // IID_IShellFolderView
        QITABENT(CDefView, IOleCommandTarget),              // IID_IOleCommandTarget
        QITABENT(CDefView, IServiceProvider),               // IID_IServiceProvider
        QITABENT(CDefView, IDefViewFrame2),                 // IID_IDefViewFrame
        QITABENTMULTI(CDefView, IDefViewFrame, IDefViewFrame2), // IID_IDefViewFrame
        QITABENT(CDefView, IDocViewSite),                   // IID_IDocViewSite 
        QITABENT(CDefView, IInternetSecurityMgrSite),       // IID_IInternetSecurityMgrSite
        { 0 }
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
    {
        // special case this one as it simply casts this...
        if (IsEqualIID(riid, IID_CDefView))
        {
            *ppvObj = (void *)this;
            AddRef();
            hres = S_OK;
        }
    }
    return hres;
}

STDMETHODIMP_(ULONG) CDefView::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDefView::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

//===========================================================================
// Constructor of CDefView class
//===========================================================================

CDefView::CDefView(IShellFolder *psf, IShellFolderViewCB* psfvcb,
    IShellView* psvOuter) : _cRef(1), m_cCallback(psfvcb)
{
    DebugMsg(TF_LIFE, TEXT("ctor CDefView %x"), this);

    psf->QueryInterface(IID_IShellFolder, (void **)&_pshf);
    psf->QueryInterface(IID_IShellFolder2, (void **)&_pshf2);

    LPCITEMIDLIST pidl = NULL;
    LONG lEvents = 0;

    if (SUCCEEDED(CallCB(SFVM_GETNOTIFY, (WPARAM)&pidl, (LPARAM)&lEvents)))
    {
        _pidlMonitor = pidl;
        _lFSEvents = lEvents;
    }

    BOOL fIsOwnerData = FALSE;
    CallCB(SFVM_ISOWNERDATA, 0, (LONG_PTR)&fIsOwnerData);
    if (fIsOwnerData)
        _fs.fFlags |= FWF_OWNERDATA;
    else
        _fs.fFlags &= ~FWF_OWNERDATA;

    _GetSortDefaults(&_dvState);
    _cRefForIdle = -1;
    _dwAttrSel = (DWORD)-1;

    //  NOTE we dont AddRef() psvOuter
    //  it has a ref on us
    _psvOuter = psvOuter;

    for (int i = 0; i < ARRAYSIZE(_crCustomColors); i++)
        _crCustomColors[i] = CLR_MYINVALID;

    _UpdateRegFlags();

    IDLData_InitializeClipboardFormats();
}

#ifdef DO_NEXT
STDAPI SHCreateShellFolderViewA(const SFV_CREATE* pcsfv, IShellView ** ppsv)
{
    if (!pcsfv || SIZEOF(*pcsfv)!=pcsfv->cbSize)
    {
        return E_INVALIDARG;
    }

    CDefView *pdsv = new CDefView(pcsfv->_pshf, pcsfv->psfvcb, pcsfv->psvOuter);
    if (pdsv)
    {
        *ppsv = pdsv;
        return NOERROR;
    }
    return E_OUTOFMEMORY;  // error
}


// BugBug we need to handle both...
STDAPI SHCreateShellFolderViewW(const SFV_CREATE* pcsfv,  IShellView ** ppsv)
#else
STDAPI SHCreateShellFolderView(const SFV_CREATE* pcsfv, IShellView ** ppsv)
#endif
{
    *ppsv = NULL;

    if (!pcsfv || SIZEOF(*pcsfv) != pcsfv->cbSize)
    {
        return E_INVALIDARG;
    }

    CDefView *pdsv = new CDefView(pcsfv->pshf, pcsfv->psfvcb, pcsfv->psvOuter);
    if (pdsv)
    {
        *ppsv = pdsv;
        return NOERROR;
    }
    return E_OUTOFMEMORY;  // error

}

//===========================================================================
// Drag & Drop
//===========================================================================


void CDVDropTarget::LeaveAndReleaseData(CDefView *that)
{
    that->_dvdt.DragLeave();

    if (that->_pdtobjHdrop)
    {
        TraceMsg(TF_DEFVIEW, "releasing 3.1 HDROP data object");

        ATOMICRELEASE(that->_pdtobjHdrop);
    }
}

void CDefView::DropFiles(HDROP hdrop)
{
    if (_pdtobjHdrop)
    {
        DWORD dwEffect = DROPEFFECT_COPY;
        POINT pt;
        POINTL ptl;
        // hdrop becomes owned by this data object, it will free it
        HRESULT hres = DataObj_SetGlobal(_pdtobjHdrop, CF_HDROP, hdrop);

        // we created this data object so this should not fail
        ASSERT(SUCCEEDED(hres));

        DragQueryPoint(hdrop, &pt);     // in client coords of window dropped on
        ClientToScreen(_hwndListview, &pt);

        ptl.x = pt.x;
        ptl.y = pt.y;

        // MK_LBUTTON means treat as a default drop
        _dvdt.Drop(_pdtobjHdrop, MK_LBUTTON, ptl, &dwEffect);

        _dvdt.LeaveAndReleaseData(this);
    }
    else
    {
        TraceMsg(TF_ERROR, "WM_DROPFILES with no pdtgt or no pdtobjHDrop");
        DragFinish(hdrop);              // be sure to free this
    }
}

//
// This function processes win 3.1 Drag & Drop messages
//
LRESULT CDefView::OldDragMsgs(UINT uMsg, WPARAM wParam, const DROPSTRUCT * lpds)
{
    DWORD dwEffect = DROPEFFECT_COPY;
    //
    // We don't need to do this hack if NT defined POINT as typedef POINTL.
    //
    union {
        POINT ptScreen;
        POINTL ptlScreen;
    } drop;

    ASSERT(SIZEOF(drop.ptScreen)==SIZEOF(drop.ptlScreen));

    if (lpds)   // Notes: lpds is NULL, if uMsg is WM_DROPFILES.
    {
        drop.ptScreen = lpds->ptDrop;
        ClientToScreen(_hwndMain, &drop.ptScreen);
    }

    switch (uMsg) {
    case WM_DRAGSELECT:
        // WM_DRAGSELECT is sent to a sink whenever an new object is dragged inside of it.
        // wParam: TRUE if the sink is being entered, FALSE if it's being exited.

        if (wParam)
        {
            TraceMsg(TF_DEFVIEW, "3.1 drag enter");

            if (_pdtobjHdrop)
            {
                // can happen if old target fails to generate drag leave properly
                TraceMsg(TF_DEFVIEW, "generating DragLeave on old drag enter");
                _dvdt.LeaveAndReleaseData(this);
            }

            if (SUCCEEDED(CIDLData_CreateFromIDArray(NULL, 0, NULL, &_pdtobjHdrop)))
            {
                // promise the CF_HDROP by setting a NULL handle
                // indicating that this dataobject will have an hdrop at Drop() time

                DataObj_SetGlobal(_pdtobjHdrop, CF_HDROP, (HGLOBAL)NULL);

                _dvdt.DragEnter(_pdtobjHdrop, MK_LBUTTON, drop.ptlScreen, &dwEffect);
            }
        }
        else
        {
            TraceMsg(TF_DEFVIEW, "3.1 drag leave");

            _dvdt.LeaveAndReleaseData(this);
        }
        break;

    case WM_DRAGMOVE:
        // WM_DRAGMOVE is sent to a sink as the object is being dragged within it.
        // wParam: Unused
        if (_pdtobjHdrop)
        {
            _dvdt.DragOver(MK_LBUTTON, drop.ptlScreen, &dwEffect);
        }
        break;

    case WM_QUERYDROPOBJECT:

        switch (lpds->wFmt) {
        case DOF_SHELLDATA:
        case DOF_DIRECTORY:
        case DOF_DOCUMENT:
        case DOF_MULTIPLE:
        case DOF_EXECUTABLE:
            // assume all targets can accept HDROP if we don't have the data object yet
            return TRUE;        // we will accept the drop
        }
        return FALSE;           // don't accept

    case WM_DROPOBJECT:
        if (!_pdtobjHdrop)
            return FALSE;

        // Check the format of dragged object.
        switch (lpds->wFmt) {
        case DOF_EXECUTABLE:
        case DOF_DOCUMENT:
        case DOF_DIRECTORY:
        case DOF_MULTIPLE:
        case DOF_PROGMAN:
        case DOF_SHELLDATA:
            // We need to unlock this window if this drag&drop is originated
            // from the shell itself.
            DAD_DragLeave();

            // The source is Win 3.1 app (probably FileMan), request HDROP.
            return DO_DROPFILE;     // Send us a WM_DROPFILES with HDROP
        }
        break;

    case WM_DROPFILES:
        DropFiles((HDROP)wParam);
        break;
    }

    return 0;   // Unknown format. Don't drop any
}

//=============================================================================
// CDVDropTarget : member
//=============================================================================
void CDVDropTarget::ReleaseDataObject()
{
    if (this->pdtobj)
    {
        //
        // JUST-IN-CASE: Put NULL in this->pdtobj before we release it.
        //  We do this just in case because we don't know what OLE does
        //  from inside Release (it might call back one of our members).
        //
        IDataObject *pdtobj = this->pdtobj;
        this->pdtobj = NULL;
        ATOMICRELEASE(pdtobj);
    }
}

void CDVDropTarget::ReleaseCurrentDropTarget()
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);
    if (this->pdtgtCur)
    {
        this->pdtgtCur->DragLeave();
        ATOMICRELEASE(this->pdtgtCur);
    }
    pdv->_itemCur = -2;

    // WARNING: Never touch pdv->itemOver in this function.
}

HRESULT CDVDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);
    HWND hwndLock;
    IOleCommandTarget* pct;

    // We can be re-entered due to ui on thread
    // or will will also want to prevent the drag/copy if the defview
    // is hosted by MSHTML.
    if ((this->pdtobj != NULL) || (S_OK != pdv->_ZoneCheck(PUAF_NOUI, URLACTION_SHELL_WEBVIEW_VERB)))
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    // Don't allow a drop from our extended view to ourself!
    if (pdv->m_cFrame.IsWebView() && SUCCEEDED(pdv->m_cFrame.GetCommandTarget(&pct)))
    {
        HRESULT hres;
        VARIANTARG v;
        v.vt = VT_I4;
        v.lVal = 0;

        hres = pct->Exec(&CGID_ShellDocView, SHDVID_ISDRAGSOURCE, 0, NULL, &v);
        ATOMICRELEASE(pct);
        if (SUCCEEDED(hres) && (v.lVal != 0)) {
            *pdwEffect = DROPEFFECT_NONE;
            return S_OK;
        }
    }

    g_fDraggingOverSource = FALSE;

    TraceMsg(TF_DEFVIEW, "CDVDropTarget::DragEnter with *pdwEffect=%x", *pdwEffect);

    pdtobj->AddRef();
    this->pdtobj = pdtobj;
    this->grfKeyState = grfKeyState;

    ASSERT(this->pdtgtCur == NULL);
    // don't really need to do this, but this sets the target state
    ReleaseCurrentDropTarget();
    this->itemOver = -2;

    //
    // In case of Desktop, we should not lock the enter screen.
    //
    hwndLock = pdv->_IsDesktop() ? pdv->_hwndView : pdv->_hwndMain;
    GetWindowRect(hwndLock, &this->rcLockWindow);

    _DragEnter(hwndLock, ptl, pdtobj);

    DAD_InitScrollData(&this->asd);

    this->ptLast.x = this->ptLast.y = 0x7fffffff; // put bogus value to force redraw

    return S_OK;
}

#define DVAE_BEFORE 0x01
#define DVAE_AFTER  0x02

// this MUST set pdwEffect to 0 or DROPEFFECT_MOVE if it's a default drag drop
// in the same window
void DV_AlterEffect(CDefView *pdv, DWORD grfKeyState, DWORD *pdwEffect, UINT uFlags)
{
    g_fDraggingOverSource = FALSE;

    if (DV_IsDropOnSource(pdv, NULL))
    {
        if (DV_ISANYICONMODE(pdv->_fs.ViewMode))
        {
            // If this is default drag & drop, enable move.
            if (uFlags & DVAE_AFTER)
            {
                if ((grfKeyState & (MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_ALT)) == MK_LBUTTON)
                {
                    *pdwEffect = DROPEFFECT_MOVE;
                    g_fDraggingOverSource = TRUE;
                }
                else if (grfKeyState & MK_RBUTTON)
                {
                    *pdwEffect |= DROPEFFECT_MOVE;
                }
            }
        }
        else
        {
            if (uFlags & DVAE_BEFORE)
            {
                // No. Disable move.
                *pdwEffect &= ~DROPEFFECT_MOVE;

                // default drag & drop, disable all.
                if ((grfKeyState & (MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_ALT)) == MK_LBUTTON)
                {
                    *pdwEffect = 0;
                }
            }
        }
    }
}

HRESULT CDVDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    HRESULT hres = S_OK;
    CDefView *pdv = IToClass(CDefView, _dvdt, this);
    int itemNew;
    POINT pt;
    RECT rc;
    BOOL fInRect;
    DWORD dwEffectScroll = 0;
    DWORD dwEffectOut = 0;
    BOOL fSameImage = FALSE;

    // We will want to prevent the drop/paste if the defview
    // is hosted by MSHTML.
    if ((this->pdtobj == NULL) || (S_OK != pdv->_ZoneCheck(PUAF_NOUI, URLACTION_SHELL_WEBVIEW_VERB)))
    {
        return E_FAIL;
    }

    pt.x = ptl.x;       // in screen coords
    pt.y = ptl.y;

    GetWindowRect(pdv->_hwndListview, &rc);
    fInRect = PtInRect(&rc, pt);

    ScreenToClient(pdv->_hwndListview, &pt);

    // assume coords of our window match listview
    if (DAD_AutoScroll(pdv->_hwndListview, &this->asd, &pt))
        dwEffectScroll = DROPEFFECT_SCROLL;

    // hilight an item, or unhilight all items (DropTarget returns -1)
    if (fInRect)
        itemNew = DV_HitTest(pdv, &pt);
    else
        itemNew = -1;

    //
    //  If we are dragging over on a different item, get its IDropTarget
    // interface or adjust itemNew to -1.
    //
    if (this->itemOver != itemNew)
    {
        IDropTarget *pdtgtNew = NULL;

        this->dwLastTime = GetTickCount();     // keep track for auto-expanding the tree

        this->itemOver = itemNew;

        // Avoid dropping onto drag source objects.
        if ((itemNew != -1) && pdv->_bDragSource)
        {
            UINT uState = ListView_GetItemState(pdv->_hwndListview, itemNew, LVIS_SELECTED);
            if (uState & LVIS_SELECTED)
                itemNew = -1;
        }

        // If we are dragging over an item, try to get its IDropTarget.
        if (itemNew != -1)
        {
            // We are dragging over an item.
            LPCITEMIDLIST apidl[1] = { pdv->_GetPIDL(itemNew) };
            if (apidl[0])
            {
                DWORD dwAttr = SFGAO_DROPTARGET;
                hres = pdv->_pshf->GetAttributesOf(1, apidl, &dwAttr);
                if (SUCCEEDED(hres) && (dwAttr & SFGAO_DROPTARGET))
                {
                    hres = pdv->_pshf->GetUIObjectOf(pdv->_hwndMain, 1, apidl, IID_IDropTarget, NULL, (void **)&pdtgtNew);
                    ASSERT(itemNew != pdv->_itemCur);    // MUST not be the same
                }
            }

            if (pdtgtNew == NULL)
            {
                // If the item is not a drop target, don't hightlight it
                // treat it as transparent.
                itemNew = -1;
            }
        }

        //
        //  If the new target is different from the current one, switch it.
        //
        if (pdv->_itemCur != itemNew)
        {
            // Release previous drop target, if any.
            ReleaseCurrentDropTarget();
            ASSERT(this->pdtgtCur==NULL);

            // Update pdv->_itemCur which indicates the current target.
            //  (Note that it might be different from this->itemOver).
            pdv->_itemCur = itemNew;

            // If we are dragging over the background or over non-sink item,
            // get the drop target for the folder.
            if (itemNew == -1)
            {
                // We are dragging over the background, this can be NULL
                ASSERT(pdtgtNew == NULL);
                this->pdtgtCur = pdv->_pdtgtBack;
                if (this->pdtgtCur)
                    this->pdtgtCur->AddRef();
            }
            else
            {
                ASSERT(pdtgtNew);
                this->pdtgtCur = pdtgtNew;
            }

            // Hilight the sink item (itemNew != -1) or unhilight all (-1).
            LVUtil_DragSelectItem(pdv->_hwndListview, itemNew);

            // Call IDropTarget::DragEnter of the target object.
            if (this->pdtgtCur)
            {
                //
                // Note that pdwEffect is in/out parameter.
                //
                dwEffectOut = *pdwEffect;       // pdwEffect in

                // Special case if we are dragging within a source window
                DV_AlterEffect(pdv, grfKeyState, &dwEffectOut, DVAE_BEFORE);
                hres = this->pdtgtCur->DragEnter(this->pdtobj, grfKeyState, ptl, &dwEffectOut);
                DV_AlterEffect(pdv, grfKeyState, &dwEffectOut, DVAE_AFTER);
            }
            else
            {
                ASSERT(dwEffectOut==0);
                DV_AlterEffect(pdv, grfKeyState, &dwEffectOut, DVAE_BEFORE | DVAE_AFTER);
            }

            TraceMsg(TF_DEFVIEW, "CDV::DragOver dwEIn=%x, dwEOut=%x", *pdwEffect, dwEffectOut);
        }
        else
        {
            ASSERT(pdtgtNew == NULL);   // It must be NULL
            goto NoChange;
        }
    }
    else
    {
NoChange:

        if (this->itemOver != -1)
        {
            DWORD dwNow = GetTickCount();

            if ((dwNow - this->dwLastTime) >= 1000)
            {
                this->dwLastTime = dwNow;
                // DAD_ShowDragImage(FALSE);
                // OpenItem(pdv, this->itemOver);
                // DAD_ShowDragImage(TRUE);
            }
        }

        //
        // No change in the selection. We assume that *pdwEffect stays
        // the same during the same drag-loop as long as the key state doesn't change.
        //
        if ((this->grfKeyState != grfKeyState) && this->pdtgtCur)
        {
            // Note that pdwEffect is in/out parameter.
            dwEffectOut = *pdwEffect;   // pdwEffect in
            // Special case if we are dragging within a source window
            DV_AlterEffect(pdv, grfKeyState, &dwEffectOut, DVAE_BEFORE);
            hres = this->pdtgtCur->DragOver(grfKeyState, ptl, &dwEffectOut);
            DV_AlterEffect(pdv, grfKeyState, &dwEffectOut, DVAE_AFTER);
        }
        else
        {
            // Same item and same key state. Use the previous dwEffectOut.
            dwEffectOut = this->dwEffectOut;
            fSameImage = TRUE;
            hres = S_OK;
        }
    }

    this->grfKeyState = grfKeyState;    // store these for the next Drop
    this->dwEffectOut = dwEffectOut;    // and DragOver

    //
    // HACK ALERT:
    //   OLE does not call IDropTarget::Drop if we return something
    //  valid. We force OLE call it by returning DROPEFFECT_SCROLL.
    //
    if (g_fDraggingOverSource)
        dwEffectScroll = DROPEFFECT_SCROLL;

    *pdwEffect = dwEffectOut | dwEffectScroll;  // pdwEffect out

    if (!(fSameImage && pt.x==this->ptLast.x && pt.y==this->ptLast.y))
    {
        HWND hwndLock = pdv->_IsDesktop() ? pdv->_hwndView : pdv->_hwndMain;
        _DragMove(hwndLock, ptl);
        this->ptLast.x = ptl.x;
        this->ptLast.y = ptl.y;
    }

    return hres;
}

HRESULT CDVDropTarget::DragLeave()
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);

    //
    // Make it possible to call it more than necessary.
    //
    if (this->pdtobj)
    {
        TraceMsg(TF_DEFVIEW, "CDVDropTarget::DragLeave");

        ReleaseCurrentDropTarget();
        this->itemOver = -2;
        ReleaseDataObject();

        DAD_DragLeave();
        LVUtil_DragSelectItem(pdv->_hwndListview, -1);
    }

    g_fDraggingOverSource = FALSE;

    ASSERT(this->pdtgtCur == NULL);
    ASSERT(this->pdtobj == NULL);

    return S_OK;
}


HRESULT CDVDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);

    //
    // Notes: OLE will give us a different data object (fully marshalled)
    //  from the one we've got on DragEnter.
    //
    if (pdtobj != this->pdtobj)
    {
        ATOMICRELEASE(this->pdtobj);
        this->pdtobj = pdtobj;
        this->pdtobj->AddRef();
    }

    pdv->_ptDrop.x = pt.x;
    pdv->_ptDrop.y = pt.y;

    ScreenToClient(pdv->_hwndListview, &pdv->_ptDrop);

    //
    // handle moves within the same window here.
    // depend on DV_AlterEffect forcing in DROPEFFECT_MOVE and only
    // dropeffect move when drag in same window
    //
    // Notes: We need to use this->grfKeyState instead of grfKeyState
    //  to see if the left mouse was used or not during dragging.
    //
    DV_AlterEffect(pdv, this->grfKeyState, pdwEffect, DVAE_BEFORE | DVAE_AFTER);

    if ((this->grfKeyState & MK_LBUTTON) && (*pdwEffect == DROPEFFECT_MOVE) &&
        (DV_IsDropOnSource(pdv, NULL)))
    {
        // This means we are left-dropping on ourselves, so we just move
        // the icons.
        DAD_DragLeave();

        pdv->_SameViewMoveIcons();

        SetForegroundWindow(pdv->_hwndMain);

        ASSERT(pdv->_bDropAnchor == FALSE);

        *pdwEffect = 0;  // the underlying objects didn't 'move' anywhere

        if (this->pdtgtCur)
        {
            this->pdtgtCur->DragLeave();
            this->pdtgtCur->Release();
            this->pdtgtCur = NULL;
        }
    }
    else if (this->pdtgtCur)
    {
        // use this local because if pdtgtCur::Drop does a UnlockWindow
        // then hits an error and needs to put up a dialog,
        // we could get re-entered and clobber the defview's pdtgtCur
        IDropTarget *pdtgtCur = this->pdtgtCur;
        this->pdtgtCur = NULL;

        //
        // HACK ALERT!!!!
        //
        //  If we don't call LVUtil_DragEnd here, we'll be able to leave
        // dragged icons visible when the menu is displayed. However, because
        // we are calling IDropTarget::Drop() which may create some modeless
        // dialog box or something, we can not ensure the locked state of
        // the list view -- LockWindowUpdate() can lock only one window at
        // a time. Therefore, we skip this call only if the pdtgtCur
        // is a subclass of CIDLDropTarget, assuming its Drop calls
        // CDefView::DragEnd (or CIDLDropTarget_DragDropMenu) appropriately.
        //
        pdv->_bDropAnchor = TRUE;

        if (!DoesDropTargetSupportDAD(pdtgtCur))
        {
            //
            // This will hide the dragged image.
            //
            DAD_DragLeave();

            //
            //  We need to reset the drag image list so that the user
            // can start another drag&drop while we are in this
            // Drop() member function call.
            //
            DAD_SetDragImage(NULL, NULL);
        }

        // Special case if we are dragging within a source window
        DV_AlterEffect(pdv, grfKeyState, pdwEffect, DVAE_BEFORE | DVAE_AFTER);

        IUnknown_SetSite(pdtgtCur, SAFECAST(pdv, IShellView2*));
        
        pdtgtCur->Drop(pdtobj, grfKeyState, pt, pdwEffect);

        IUnknown_SetSite(pdtgtCur, NULL);

        pdtgtCur->Release();

        DAD_DragLeave();

        pdv->_bDropAnchor = FALSE;

        ASSERT(!DAD_IsDragging());
    }
    else
    {
        //
        // We come here if Drop is called without DragMove (with DragEnter).
        //
        TraceMsg(TF_DEFVIEW, "CDV::Drop this->pdtgtCur == 0");
        *pdwEffect = 0;
    }

    DragLeave();    // DoDragDrop does not call DragLeave() after Drop()

    return S_OK;
}

//
// HACK ALERT!!! (see CDVDropTarget::Drop as well)
//
//  All the subclasses of CIDLDropTarget MUST call this function from
// within its Drop() member function. Calling CIDLDropTarget_DragDropMenu()
// is sufficient because it calls CDefView::UnlockWindow.
//

// lego... make this a #define in defview.h
#ifndef DefView_UnlockWindow
void DefView_UnlockWindow()
{
    DAD_DragLeave();
}
#endif


STDAPI_(CDefView *) DV_HwndMain2DefView(HWND _hwndMain)
{
    CDefView *pdsv = NULL;
    IShellBrowser *psb = FileCabinet_GetIShellBrowser(_hwndMain);
    if (psb)
    {
        IShellView *psv;
        if (SUCCEEDED(psb->QueryActiveShellView(&psv)))
        {
            if (SUCCEEDED(psv->QueryInterface(IID_CDefView, (void **)&pdsv)))
            {
                // HACK: We'll release this now and assume the pointer
                // is still OK afterwards.  We really should release this
                // in the calling function
                if (pdsv)
                    pdsv->Release();
            }
            else
            {
                // We might have the _psvOuter, query for the pdsv
                IShellView *psvInner;
                if (SUCCEEDED(psv->QueryInterface(IID_IShellView, (void **)&psvInner)))
                {
                    if (SUCCEEDED(psvInner->QueryInterface(IID_CDefView, (void **)&pdsv)))
                    {
                        // HACK: We'll release this now and assume the pointer
                        // is still OK afterwards.  We really should release this
                        // in the calling function
                        if (pdsv)
                            pdsv->Release();
                    }
                    ATOMICRELEASE(psvInner);
                }
            }
            ATOMICRELEASE(psv);
        }
    }
    return pdsv;
}

STDAPI_(HWND) DV_HwndMain2HwndView(HWND _hwndMain)
{
    CDefView *pdsv = DV_HwndMain2DefView(_hwndMain);
    return pdsv ? pdsv->_hwndView : NULL;
}

STDAPI_(IShellFolderViewCB *) SHGetShellFolderViewCB(HWND _hwndMain)
{
    CDefView *pdsv = DV_HwndMain2DefView(_hwndMain);
    return pdsv ? pdsv->m_cCallback.GetSFVCB() : NULL;
}


BOOL DefView_IsBkDropTarget(CDefView *pdsv, IDropTarget *pdtg)
{
    BOOL fRet = FALSE;
    POINT pt;

    if (pdsv->_bContextMenuMode) {
        if (ListView_GetSelectedCount(pdsv->_hwndListview) == 0) {
            fRet = TRUE;
        }
    }
    if (!fRet && DefView_GetDropPoint(pdsv, &pt)) {
        // The Drop point is returned in internal listview coordinates
        // space, so we need to convert it back to client space
        // before we call this function...

        LVUtil_LVToClient(pdsv->_hwndListview, &pt);
        if (DV_HitTest(pdsv, &pt) == -1) {
            fRet = TRUE;
        }
    }
    return fRet;
}


STDMETHODIMP CDefView::Rearrange(LPARAM lParamSort)
{
    HRESULT hr = S_OK ;
    DECLAREWAITCURSOR;

    _dvState.iLastColumnClick = (int) _dvState.lParamSort;
    _dvState.lParamSort = lParamSort;

    SetWaitCursor();

    // First see if this wants to be handled externally by callback
    if (FAILED(CallCB(SFVM_ARRANGE, 0, lParamSort)))
    {
        // Nope...
        if (this->m_cFrame.IsSFVExtension())
        {
            this->m_cFrame.GetExtendedISFV()->Rearrange(lParamSort);
        }

        // we always sort the main listview, so the settings match...
        {
            _SetSortArrows() ;
            hr = _InternalRearrange() ? S_OK : HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE) ;
        }
    }

    ResetWaitCursor();

    return hr;
}


STDMETHODIMP CDefView::ArrangeGrid()
{
    this->Command(NULL, GET_WM_COMMAND_MPS(SFVIDM_ARRANGE_GRID, 0, 0));
    return NOERROR;
}

STDMETHODIMP CDefView::AutoArrange()
{
    this->Command(NULL, GET_WM_COMMAND_MPS(SFVIDM_ARRANGE_AUTO, 0, 0));
    return NOERROR;
}

STDMETHODIMP CDefView::GetAutoArrange()
{
    // Even for IShellView extensions, don't use their autoarrange flags.
    // The top-level defview needs to track this so it persists correctly.
    return _fs.fFlags & FWF_AUTOARRANGE ? S_OK : S_FALSE;
}


STDMETHODIMP CDefView::GetArrangeParam(LPARAM *plParamSort)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->GetArrangeParam(plParamSort);
    }

    *plParamSort = _dvState.lParamSort;
    return NOERROR;
}


STDMETHODIMP CDefView::AddObject(LPITEMIDLIST pidl, UINT *puItem)
{
    HRESULT hr = NOERROR;

    // BUGBUG: we fail this for DocObject extended views!!
    *puItem = DefView_AddObject(this, pidl, TRUE);

    // let the def-view have first go so that it can do the work to find out if
    // if it is really needed (could be filtered out in the common dialogs ..)
    if (this->m_cFrame.IsSFVExtension() && (*puItem >= 0))
    {
        // delegate to the view extension....
        hr = this->m_cFrame.GetExtendedISFV()->AddObject(pidl, puItem);

        if (FAILED(hr) || *puItem < 0)
            return E_OUTOFMEMORY;
    }

    // BUGBUG: There should be more error values
    return *puItem >= 0 ? NOERROR : E_OUTOFMEMORY;
}


STDMETHODIMP CDefView::GetObjectCount(UINT *puCount)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->GetObjectCount(puCount);
    }

    *puCount = ListView_GetItemCount(_hwndListview);
    return NOERROR;
}


STDMETHODIMP CDefView::SetObjectCount(UINT uCount, UINT dwFlags)
{
    // Mask over to the flags that map directly accross
    DWORD dw = dwFlags & SFVSOC_NOSCROLL;
    HRESULT hres;
    UINT uCountOld = 0;

    GetObjectCount(&uCountOld);

    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        this->m_cFrame.GetExtendedISFV()->SetObjectCount(uCount, dwFlags);

        // fallthrough so that both views have the same count set...
    }

    if ((dwFlags & SFVSOC_INVALIDATE_ALL) == 0)
        dw |= LVSICF_NOINVALIDATEALL; // gross transform

    hres = (HRESULT)SendMessage(_hwndListview, LVM_SETITEMCOUNT, (WPARAM)uCount, (LPARAM)dw);

    // Notify automation if we're going from 0 to 1 or more items
    if (!uCountOld && uCount)
        _PostSelChangedMessage();

    return hres;
}


STDMETHODIMP CDefView::GetObject(LPITEMIDLIST *ppidl, UINT uItem)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->GetObject(ppidl, uItem);
    }

    // Worse hack, if -42 then return our own pidl...
    if (uItem == (UINT)-42)
    {
        *ppidl = (LPITEMIDLIST)_pidlMonitor;
        return(*ppidl ? NOERROR : E_UNEXPECTED);
    }

    // Hack, if item is -2, this implies return the focused item
    if (uItem == (UINT)-2)
        uItem = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED);

    *ppidl = _GetPIDL(uItem);
    return *ppidl ? NOERROR : E_UNEXPECTED;
}


STDMETHODIMP CDefView::RemoveObject(LPITEMIDLIST pidl, UINT *puItem)
{
    *puItem = _RemoveObject(pidl, FALSE);

    if (this->m_cFrame.IsSFVExtension() && (*puItem >= 0))
    {
        // delegate to the view extension....
        HRESULT hr = this->m_cFrame.GetExtendedISFV()->RemoveObject( pidl, puItem );
        if (FAILED(hr) || *puItem < 0)
            return E_INVALIDARG;
    }

    return(*puItem >= 0 ? NOERROR : E_INVALIDARG);
}


STDMETHODIMP CDefView::UpdateObject(LPITEMIDLIST pidlOld, LPITEMIDLIST pidlNew,
    UINT *puItem)
{
    LPITEMIDLIST apidl[2] = { pidlOld, pidlNew };

    *puItem = _UpdateObject(apidl, TRUE);

    if (this->m_cFrame.IsSFVExtension() && (*puItem >= 0))
    {
        // delegate to the view extension....
        HRESULT hr = this->m_cFrame.GetExtendedISFV()->UpdateObject(pidlOld, pidlNew, puItem);

        if (FAILED(hr) || *puItem < 0)
            return E_INVALIDARG;

        // drop through for the def-view
    }

    return((int)(*puItem) >= 0 ? NOERROR : E_INVALIDARG);
}


STDMETHODIMP CDefView::RefreshObject(LPITEMIDLIST pidl, UINT *puItem)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        HRESULT hr = this->m_cFrame.GetExtendedISFV()->RefreshObject(pidl, puItem);

        if (FAILED(hr) || *puItem < 0)
            return E_INVALIDARG;

        // drop through ..
    }

    *puItem = _RefreshObject(&pidl);
    return *puItem >= 0 ? NOERROR : E_INVALIDARG;
}


STDMETHODIMP CDefView::SetRedraw(BOOL bRedraw)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->SetRedraw(bRedraw);

        // no drop through, assuming this only aplies to the currently visible view. ..
    }

    SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)bRedraw, 0);
    return NOERROR;
}


STDMETHODIMP CDefView::GetSelectedObjects(LPCITEMIDLIST **pppidl, UINT *puItems)
{
    UINT    cItems;
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->GetSelectedObjects(pppidl, puItems);
    }

    HRESULT hres = DefView_GetItemObjects(this, pppidl, SVGIO_SELECTION, &cItems);
    if (SUCCEEDED(hres))
    {
        *puItems = cItems;
        return NOERROR;
    }
    return hres;
}


STDMETHODIMP CDefView::GetSelectedCount(UINT *puSelected)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->GetSelectedCount(puSelected);
    }

    *puSelected = ListView_GetSelectedCount(_hwndListview);
    return NOERROR;
}


STDMETHODIMP CDefView::IsDropOnSource(IDropTarget *pDropTarget)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->IsDropOnSource(pDropTarget);
    }

    return DV_IsDropOnSource(this, pDropTarget) ? S_OK : S_FALSE;
}


STDMETHODIMP CDefView::MoveIcons(IDataObject *pDataObject)
{
    return E_NOTIMPL;
}


STDMETHODIMP CDefView::GetDropPoint(POINT *ppt)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->GetDropPoint( ppt );
    }

    return DefView_GetDropPoint(this, ppt) ? S_OK : S_FALSE;
}


STDMETHODIMP CDefView::GetDragPoint(POINT *ppt)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->GetDragPoint(ppt);
    }

    return DefView_GetDragPoint(this, ppt) ? S_OK : S_FALSE;
}


STDMETHODIMP CDefView::SetItemPos(LPCITEMIDLIST pidl, POINT *ppt)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->SetItemPos(pidl, ppt);
    }

    SFV_SETITEMPOS sip;
    sip.pidl = pidl;
    sip.pt = *ppt;

    DefView_SetItemPos(this, &sip);
    return NOERROR;
}


STDMETHODIMP CDefView::IsBkDropTarget(IDropTarget *pDropTarget)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->IsBkDropTarget(pDropTarget);
    }

    return DefView_IsBkDropTarget(this, pDropTarget) ? S_OK : S_FALSE;
}


STDMETHODIMP CDefView::SetClipboard(BOOL bMove)
{
    DSV_OnSetClipboard(this, bMove ? DFM_CMD_MOVE : DFM_CMD_COPY);

    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        this->m_cFrame.GetExtendedISFV()->SetClipboard(bMove);

        // drop through so both defview reflects the change as well
    }

    return NOERROR;
}


STDMETHODIMP CDefView::SetPoints(IDataObject *pDataObject)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->SetPoints(pDataObject);
    }

    DefView_SetPoints(this, pDataObject);
    return NOERROR;
}


STDMETHODIMP CDefView::GetItemSpacing(ITEMSPACING *pSpacing)
{
    if (this->m_cFrame.IsSFVExtension())
    {
        // delegate to the view extension....
        return this->m_cFrame.GetExtendedISFV()->GetItemSpacing(pSpacing);
    }

    return(DV_GetItemSpacing(this, pSpacing) ? S_OK : S_FALSE);
}


STDMETHODIMP CDefView::SetCallback(IShellFolderViewCB* pNewCB, IShellFolderViewCB** ppOldCB)
{
    *ppOldCB = NULL;

    HRESULT hr = this->m_cCallback.SetCallback(pNewCB, ppOldCB);

    // allow both IShellFolderViews to hold the Callback ....
    if (this->m_cFrame.IsSFVExtension())
    {
        IShellFolderViewCB* pcb;

        // delegate to the view extension....
        hr = this->m_cFrame.GetExtendedISFV()->SetCallback(pNewCB, &pcb);

        // we don't care about the old callback
        if (SUCCEEDED(hr))
            ATOMICRELEASE(pcb);
    }

    return hr;
}

const UINT c_rgiSelectFlags[][2] =
{
    { SFVS_SELECT_ALLITEMS, SFVIDM_SELECT_ALL },
    { SFVS_SELECT_NONE, SFVIDM_DESELECT_ALL },
    { SFVS_SELECT_INVERT, SFVIDM_SELECT_INVERT }
};

STDMETHODIMP CDefView::Select(UINT dwFlags )
{
    // translate the flag into the menu ID
    for ( int i = 0; i < ARRAYSIZE(c_rgiSelectFlags); i ++ )
    {
        if ( c_rgiSelectFlags[i][0] == dwFlags )
        {
            // note, if we are looking at a view-extension, then Command() will delegate to it..
            return (HRESULT) this->Command( NULL, c_rgiSelectFlags[i][1], 0);
        }
    }

    return E_INVALIDARG;
}


STDMETHODIMP CDefView::QuerySupport(UINT * pdwSupport )
{
    if (this->m_cFrame.IsSFVExtension())
    {
        return this->m_cFrame.GetExtendedISFV()->QuerySupport(pdwSupport);
    }

    // DefView supports all the operations...
    return S_OK;
}

STDMETHODIMP CDefView::SetAutomationObject(IDispatch *pdisp )
{
    // Release any previous automation objects we may have...
    if (m_pauto)
    {
        // Tell the automation object to release the back pointer to us...
        IShellService *pss;
        if (SUCCEEDED(m_pauto->QueryInterface(IID_IShellService, (void **)&pss)))
        {
            // Pass an Unknown pointer to this guy...
            pss->SetOwner(NULL);
            ATOMICRELEASE(pss);
        }

        //release our use of it, make sure won't be hit by any callback...
        IDispatch * pauto = m_pauto;
        m_pauto = NULL;
        ATOMICRELEASE(pauto);
    }

    if (pdisp)
    {
        // Hold onto the object...
        m_pauto = pdisp;
        pdisp->AddRef();
        // let sfvextensions handle the automation. . .
        if (m_cFrame.IsSFVExtension())
        {
            ASSERT(m_cFrame.GetExtendedISFV());
            ASSERT(m_pauto);
            m_cFrame.GetExtendedISFV()->SetAutomationObject(m_pauto);
        }

    }

    return NOERROR;
}



//===========================================================================
// A common function that given an IShellFolder and and an Idlist that is
// contained in it, get back the index into the system image list.
//===========================================================================

HRESULT _GetILIndexGivenPXIcon(IExtractIcon *pxicon, UINT uFlags, LPCITEMIDLIST pidl, int *piImage, BOOL fAnsiCrossOver)
{
    TCHAR szIconFile[MAX_PATH];
#ifdef UNICODE
    CHAR szIconFileA[MAX_PATH];
    IExtractIconA *pxiconA = (IExtractIconA *)pxicon;
#endif
    int iIndex;
    int iImage = -1;
    UINT wFlags=0;
    HRESULT hres;

    ASSERT(g_himlIcons);      // you must initialize the icon cache first

#ifdef UNICODE
    if (fAnsiCrossOver)
    {
        szIconFileA[0] = '\0';
        hres = pxiconA->GetIconLocation(uFlags | GIL_FORSHELL,
                    szIconFileA, ARRAYSIZE(szIconFileA), &iIndex, &wFlags);
        SHAnsiToUnicode(szIconFileA, szIconFile, ARRAYSIZE(szIconFile));
    }
    else
    {
#endif
        hres = pxicon->GetIconLocation(uFlags | GIL_FORSHELL,
                    szIconFile, ARRAYSIZE(szIconFile), &iIndex, &wFlags);
#ifdef UNICODE
    }
#endif

    //
    //  "*" as the file name means iIndex is already a system
    //  icon index, we are done.
    //
    //  this is a hack for our own internal icon handler
    //
    if (SUCCEEDED(hres) && (wFlags & GIL_NOTFILENAME) &&
        szIconFile[0] == TEXT('*') && szIconFile[1] == 0)
    {
        *piImage = iIndex;
        return hres;
    }

    if (SUCCEEDED(hres))
    {
        //
        // if GIL_DONTCACHE was returned by the icon handler, dont
        // lookup the previous icon, assume a cache miss.
        //
        if (!(wFlags & GIL_DONTCACHE))
            iImage = LookupIconIndex(PathFindFileName(szIconFile), iIndex, wFlags);

        // if we miss our cache...
        if (iImage == -1)
        {
            HICON hiconLarge = NULL;
            HICON hiconSmall = NULL;

            if (uFlags & GIL_ASYNC)
            {
                // force a lookup incase we are not in explorer.exe
                *piImage = Shell_GetCachedImageIndex(c_szShell32Dll, II_DOCNOASSOC, 0);
                return E_PENDING;
            }

            // try getting it from the ExtractIcon member fuction
#ifdef UNICODE
            if (fAnsiCrossOver)
            {
                hres=pxiconA->Extract(szIconFileA, iIndex,
                   &hiconLarge, &hiconSmall, MAKELONG(g_cxIcon, g_cxSmIcon));
            }
            else
            {
#endif
                hres=pxicon->Extract(szIconFile, iIndex,
                    &hiconLarge, &hiconSmall, MAKELONG(g_cxIcon, g_cxSmIcon));
#ifdef UNICODE
            }
#endif
            // S_FALSE means, can you please do it...Thanks

            if (hres == S_FALSE && !(wFlags & GIL_NOTFILENAME))
            {
                hres = SHDefExtractIcon(szIconFile, iIndex, wFlags,
                    &hiconLarge, &hiconSmall, MAKELONG(g_cxIcon, g_cxSmIcon));
            }

            //  if we extracted a icon add it to the cache.

            if (hiconLarge)
            {
                // yes!  got it, stuff it into our cache
                iImage = SHAddIconsToCache(hiconLarge, hiconSmall, szIconFile, iIndex, wFlags);
            }

            // if we failed in any way pick a default icon

            if (iImage == -1)
            {
                if (wFlags & GIL_SIMULATEDOC)
                    iImage = II_DOCUMENT;
                else if ((wFlags & GIL_PERINSTANCE) && PathIsExe(szIconFile))
                    iImage = II_APPLICATION;
                else
                    iImage = II_DOCNOASSOC;

                // force a lookup incase we are not in explorer.exe
                iImage = Shell_GetCachedImageIndex(c_szShell32Dll, iImage, 0);
                // if the handler failed dont cache this default icon.
                // so we will try again later and mabey get the right icon.
                //
                // handlers should only fail if they cant access the file
                // or something equaly bad.

                if (SUCCEEDED(hres))
                    AddToIconTable(szIconFile, iIndex, wFlags, iImage);
                else
                    TraceMsg(TF_DEFVIEW, "not caching icon for '%s' because cant access file", szIconFile);
            }
        }
    }

    if (iImage < 0)
        iImage = Shell_GetCachedImageIndex(c_szShell32Dll, II_DOCNOASSOC, 0);

    *piImage = iImage;
    return hres;
}

//===========================================================================
// A common function that given an IShellFolder and and an Idlist that is
// contained in it, get back the index into the system image list.
//===========================================================================

STDAPI SHGetIconFromPIDL(IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, int *piImage)
{
    IExtractIcon *pxi;
    HRESULT hres;

    ASSERT(g_himlIcons);      // you must initialize the icon cache first

    if (psi)
    {
#ifdef DEBUG
        *piImage = -1;
#endif
        hres = psi->GetIconOf(pidl, flags, piImage);

        if (hres == S_OK)
        {
            ASSERT(*piImage != -1);
            return hres;
        }

        if (hres == E_PENDING)
        {
            ASSERT(flags & GIL_ASYNC);
            ASSERT(*piImage != -1);
            return hres;
        }
    }

    *piImage = Shell_GetCachedImageIndex( c_szShell32Dll, II_DOCNOASSOC, 0);

    hres = psf->GetUIObjectOf(NULL, 1, pidl ? &pidl : NULL, IID_IExtractIcon, NULL, (void **)&pxi);

    if (SUCCEEDED(hres))
    {
        hres = _GetILIndexGivenPXIcon(pxi, flags, pidl, piImage, FALSE);
        pxi->Release();
    }
#ifdef UNICODE
    else
    {
        // Try the ANSI interface, see if we are dealing with an old set of code
        IExtractIconA *pxiA;

        hres = psf->GetUIObjectOf(NULL, 1, pidl ? &pidl : NULL, IID_IExtractIconA, NULL, (void **)&pxiA);
        if (SUCCEEDED(hres))
        {
            hres = _GetILIndexGivenPXIcon((IExtractIcon *)pxiA, flags, pidl, piImage, TRUE);
            pxiA->Release();
        }
    }
#endif

    return hres;
}

//===========================================================================
// A common function that given an IShellFolder and and an Idlist that is
// contained in it, get back the index into the system image list.
//===========================================================================
int WINAPI SHMapPIDLToSystemImageListIndex(IShellFolder *psf, LPCITEMIDLIST pidl, int *piIndexSel)
{
    int iIndex;

    ASSERT(g_himlIcons);      // you must initialize the icon cache first

    if (piIndexSel)
        SHGetIconFromPIDL(psf, NULL, pidl, GIL_OPENICON, piIndexSel);

    SHGetIconFromPIDL(psf, NULL, pidl,  0, &iIndex);
    return iIndex;
}

// -------------- auto scroll stuff --------------

BOOL _AddTimeSample(AUTO_SCROLL_DATA *pad, const POINT *ppt, DWORD dwTime)
{
    pad->pts[pad->iNextSample] = *ppt;
    pad->dwTimes[pad->iNextSample] = dwTime;

    pad->iNextSample++;

    if (pad->iNextSample == ARRAYSIZE(pad->pts))
        pad->bFull = TRUE;

    pad->iNextSample = pad->iNextSample % ARRAYSIZE(pad->pts);

    return pad->bFull;
}

#ifdef DEBUG
// for debugging, verify we have good averages
DWORD g_time = 0;
int g_distance = 0;
#endif

int _CurrentVelocity(AUTO_SCROLL_DATA *pad)
{
    int i, iStart, iNext;
    int dx, dy, distance;
    DWORD time;

    ASSERT(pad->bFull);

    distance = 0;
    time = 1;   // avoid div by zero

    i = iStart = pad->iNextSample % ARRAYSIZE(pad->pts);

    do {
        iNext = (i + 1) % ARRAYSIZE(pad->pts);

        dx = abs(pad->pts[i].x - pad->pts[iNext].x);
        dy = abs(pad->pts[i].y - pad->pts[iNext].y);
        distance += (dx + dy);
        time += abs(pad->dwTimes[i] - pad->dwTimes[iNext]);

        i = iNext;

    } while (i != iStart);

#ifdef DEBUG
    g_time = time;
    g_distance = distance;
#endif

    // scale this so we don't loose accuracy
    return (distance * 1024) / time;
}



// NOTE: this is duplicated in shell32.dll
//
// checks to see if we are at the end position of a scroll bar
// to avoid scrolling when not needed (avoid flashing)
//
// in:
//      code        SB_VERT or SB_HORZ
//      bDown       FALSE is up or left
//                  TRUE  is down or right

BOOL CanScroll(HWND hwnd, int code, BOOL bDown)
{
    SCROLLINFO si;

    si.cbSize = SIZEOF(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd, code, &si);

    if (bDown)
    {
        if (si.nPage)
            si.nMax -= si.nPage - 1;
        return si.nPos < si.nMax;
    }
    else
    {
        return si.nPos > si.nMin;
    }
}

#define DSD_NONE                0x0000
#define DSD_UP                  0x0001
#define DSD_DOWN                0x0002
#define DSD_LEFT                0x0004
#define DSD_RIGHT               0x0008

//---------------------------------------------------------------------------
DWORD ListView_GetWindowStyle(HWND hwnd)
{
    DWORD dwStyle;

#ifdef FLAT_SCROLLBAR
    LONG lExStyle;
    lExStyle = ListView_GetExtendedListViewStyle(hwnd);
    if (!(lExStyle & LVS_EX_FLATSB) ||
        !FlatSB_GetScrollProp(hwnd, WSB_PROP_WINSTYLE, (int*)&dwStyle))
#endif
    {
        dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    }

    return dwStyle;
}

DWORD DAD_DragScrollDirection(HWND hwnd, const POINT *ppt)
{
    RECT rcOuter, rc;
    DWORD dwDSD = DSD_NONE;
    DWORD dwStyle = ListView_GetWindowStyle(hwnd);

    // BUGBUG: do these as globals
#define g_cxVScroll GetSystemMetrics(SM_CXVSCROLL)
#define g_cyHScroll GetSystemMetrics(SM_CYHSCROLL)

    GetClientRect(hwnd, &rc);

    if (dwStyle & WS_HSCROLL)
        rc.bottom -= g_cyHScroll;

    if (dwStyle & WS_VSCROLL)
        rc.right -= g_cxVScroll;

    // the explorer forwards us drag/drop things outside of our client area
    // so we need to explictly test for that before we do things
    //
    rcOuter = rc;
    InflateRect(&rcOuter, g_cxSmIcon, g_cySmIcon);

    InflateRect(&rc, -g_cxIcon, -g_cyIcon);

    if (!PtInRect(&rc, *ppt) && PtInRect(&rcOuter, *ppt))
    {
        // Yep - can we scroll?
        if (dwStyle & WS_HSCROLL)
        {
            if (ppt->x < rc.left)
            {
                if (CanScroll(hwnd, SB_HORZ, FALSE))
                    dwDSD |= DSD_LEFT;
            }
            else if (ppt->x > rc.right)
            {
                if (CanScroll(hwnd, SB_HORZ, TRUE))
                    dwDSD |= DSD_RIGHT;
            }
        }
        if (dwStyle & WS_VSCROLL)
        {
            if (ppt->y < rc.top)
            {
                if (CanScroll(hwnd, SB_VERT, FALSE))
                    dwDSD |= DSD_UP;
            }
            else if (ppt->y > rc.bottom)
            {
                if (CanScroll(hwnd, SB_VERT, TRUE))
                    dwDSD |= DSD_DOWN;
            }
        }
    }
    return dwDSD;
}


#define SCROLL_FREQUENCY        (GetDoubleClickTime()/2)        // 1 line scroll every 1/4 second
#define MIN_SCROLL_VELOCITY     20      // scaled mouse velocity

BOOL WINAPI DAD_AutoScroll(HWND hwnd, AUTO_SCROLL_DATA *pad, const POINT *pptNow)
{
    // first time we've been called, init our state
    int v;
    DWORD dwTimeNow = GetTickCount();
    DWORD dwDSD = DAD_DragScrollDirection(hwnd, pptNow);

    if (!_AddTimeSample(pad, pptNow, dwTimeNow))
        return dwDSD;

    v = _CurrentVelocity(pad);

    if (v <= MIN_SCROLL_VELOCITY)
    {
        // Nope, do some scrolling.
        if ((dwTimeNow - pad->dwLastScroll) < SCROLL_FREQUENCY)
            dwDSD = 0;

        if (dwDSD & DSD_UP)
        {
            DAD_ShowDragImage(FALSE);
            FORWARD_WM_VSCROLL(hwnd, NULL, SB_LINEUP, 1, SendMessage);
        }
        else if (dwDSD & DSD_DOWN)
        {
            DAD_ShowDragImage(FALSE);
            FORWARD_WM_VSCROLL(hwnd, NULL, SB_LINEDOWN, 1, SendMessage);
        }
        if (dwDSD & DSD_LEFT)
        {
            DAD_ShowDragImage(FALSE);
            FORWARD_WM_HSCROLL(hwnd, NULL, SB_LINEUP, 1, SendMessage);
        }
        else if (dwDSD & DSD_RIGHT)
        {
            DAD_ShowDragImage(FALSE);
            FORWARD_WM_HSCROLL(hwnd, NULL, SB_LINEDOWN, 1, SendMessage);
        }

        DAD_ShowDragImage(TRUE);

        if (dwDSD)
        {
            TraceMsg(TF_DEFVIEW, "v=%d", v);
            pad->dwLastScroll = dwTimeNow;
        }
    }
    return dwDSD;       // bits set if in scroll region
}

//
//  We need to store this array in per-instance data because we don't
// want to marshal COPYHOOKINFO data structure across process boundary.
// However, this implementation does not support multiple instance of
// the shell.
//
HDSA g_hdsaDefViewCopyHook = NULL;

typedef struct _DVCOPYHOOK {
    HWND        _hwndView;
    CDefView *  pdv;
} DVCOPYHOOK, *LPDVCOPYHOOK;

void CDefView::AddCopyHook()
{
    ENTERCRITICAL;
    if (!g_hdsaDefViewCopyHook)
    {
        g_hdsaDefViewCopyHook = DSA_Create(SIZEOF(DVCOPYHOOK), 4);
        TraceMsg(TF_DEFVIEW, "AddCopyHook creating the dsa");
    }

    if (g_hdsaDefViewCopyHook)
    {
        DVCOPYHOOK dvch = { _hwndView, this };
        ASSERT(dvch._hwndView);
        if (DSA_AppendItem(g_hdsaDefViewCopyHook, &dvch)!=-1)
        {
            this->AddRef();
            TraceMsg(TF_DEFVIEW, "AddCopyHook successfully added (total=%d)",
                     DSA_GetItemCount(g_hdsaDefViewCopyHook));
        }
    }
    LEAVECRITICAL;
}

int CDefView::FindCopyHook(BOOL fRemoveInvalid)
{
    int item;
    ASSERTCRITICAL;

    if (g_hdsaDefViewCopyHook==NULL) {
        return -1;
    }

    item = DSA_GetItemCount(g_hdsaDefViewCopyHook);

    while(--item>=0)
    {
        const DVCOPYHOOK * pdvch=(const DVCOPYHOOK *)DSA_GetItemPtr(g_hdsaDefViewCopyHook, item);
        if (pdvch)
        {
            if (fRemoveInvalid) {
                if (!IsWindow(pdvch->_hwndView))
                {
                    TraceMsg(TF_WARNING, "FindCopyHook: found a invalid element, removing...");
                    DSA_DeleteItem(g_hdsaDefViewCopyHook, item);
                    continue;
                }
            }

            if ((pdvch->_hwndView==_hwndView) && (pdvch->pdv==this)) {
                return item;
            }
        }
        else
        {
            ASSERT(0);
        }
    }

    return -1;  // not found
}

void CDefView::RemoveCopyHook()
{
    IShellView *psv = NULL;
    ENTERCRITICAL;
    if (g_hdsaDefViewCopyHook)
    {
        int  item = FindCopyHook(TRUE);
        if (item!=-1)
        {
            LPDVCOPYHOOK pdvch=(LPDVCOPYHOOK)DSA_GetItemPtr(g_hdsaDefViewCopyHook, item);
            psv = pdvch->pdv;
            TraceMsg(TF_DEFVIEW, "RemoveCopyHook removing an element");
            DSA_DeleteItem(g_hdsaDefViewCopyHook, item);

            //
            // If this is the last guy, destroy it.
            //
            if (DSA_GetItemCount(g_hdsaDefViewCopyHook)==0)
            {
                TraceMsg(TF_DEFVIEW, "RemoveCopyHook destroying hdsa (no element)");
                DSA_Destroy(g_hdsaDefViewCopyHook);
                g_hdsaDefViewCopyHook=NULL;
            }
        }
    }
    LEAVECRITICAL;

    //
    // Release it outside the critical section.
    //
    ATOMICRELEASE(psv);
}


extern "C" UINT DefView_CopyHook(const COPYHOOKINFO *pchi)
{
    int item;
    UINT idRet = IDYES;

    if (g_hdsaDefViewCopyHook==NULL) {
        return idRet;
    }

    for (item=0;;item++)
    {
        DVCOPYHOOK dvch = { NULL, NULL };

        //
        //  We should minimize this critical section (and must not
        // call pfnCallBack which may popup UI!).
        //
        ENTERCRITICAL;
        if (g_hdsaDefViewCopyHook && DSA_GetItem(g_hdsaDefViewCopyHook, item, &dvch)) {
            dvch.pdv->AddRef();
        }
        LEAVECRITICAL;

        if (dvch.pdv)
        {
            if (IsWindow(dvch._hwndView))
            {
                HRESULT hres = dvch.pdv->CallCB(SFVM_NOTIFYCOPYHOOK, 0, (LPARAM)pchi);

                ATOMICRELEASE(dvch.pdv);
                if (SUCCEEDED(hres) && (hres != S_OK))
                {
                    idRet = HRESULT_CODE(hres);
                    ASSERT(idRet==IDYES || idRet==IDCANCEL || idRet==IDNO);
                    break;
                }
                item++;
            }
            else
            {
                TraceMsg(TF_DEFVIEW, "DefView_CopyHook list has an invalid element");
                ATOMICRELEASE(dvch.pdv);
            }
        }
        else
        {
            break;      // no more item.
        }
    }

    return idRet;
}


// IOleCommandTarget stuff - just forward to the extended view
STDMETHODIMP CDefView::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;
    BOOL fQSCalled = FALSE;

    if (m_cFrame.IsWebView())
    {
        IOleCommandTarget* pct;

        if (SUCCEEDED(m_cFrame.GetCommandTarget(&pct)))
        {
            hres = pct->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
            fQSCalled = SUCCEEDED(hres);
            ATOMICRELEASE(pct);
        }
    }

    if (pguidCmdGroup == NULL)
    {
        if (_HasNormalView())
        {
            if (rgCmds == NULL)
                return E_INVALIDARG;

            UINT i;
            for (i=0 ; i<cCmds ; i++)
            {
                // ONLY say that we support the stuff we support in ::OnExec
                switch (rgCmds[i].cmdID)
                {
                case OLECMDID_REFRESH:
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                    break;

                default:
                    // don't disable if the extended view has already answered
                    if (!fQSCalled)
                    {
                        rgCmds[i].cmdf = 0;
                    }
                    break;
                }
            }
        }
    }
    else if (IsEqualGUID(_clsid, *pguidCmdGroup))
    {
        if (pcmdtext)
        {
            switch (pcmdtext->cmdtextf)
            {
            case OLECMDTEXTF_NAME:
                // It's a query for the button tooltip text.
                ASSERT(cCmds == 1);
                DV_GetToolTipTextW(this, rgCmds[0].cmdID, pcmdtext->rgwz, pcmdtext->cwBuf);

                // ensure NULL termination
                pcmdtext->rgwz[pcmdtext->cwBuf - 1] = 0;
                pcmdtext->cwActual = lstrlenW(pcmdtext->rgwz);

                hres = S_OK;
                break;
            
            default:
                hres = E_FAIL;
                break;
            }
        }
        else
        {
            DWORD dwAttr = DefView_GetAttributesFromSelection(this, SFGAO_RELEVANT);

            for (UINT i = 0; i < cCmds; i++)
            {
                if (_ShouldEnableButton(rgCmds[i].cmdID, dwAttr, -1))
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                else
                    rgCmds[i].cmdf = 0;
            }

            hres = S_OK;
        }
    }

    return hres;
}
STDMETHODIMP CDefView::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    if (pguidCmdGroup == NULL)
    {
        switch (nCmdID)
        {
        case OLECMDID_REFRESH:
            if (FAILED(ReloadContent()))
            {
               //This invalidation deletes the WebView and also avoid 
               //unpainted areas in ListView  areas whose paint messages
               //are eaten by the visible WebView
               InvalidateRect(_hwndView, NULL, TRUE);
            }
            hres = S_OK;
            break;
        }
    }
    else if (IsEqualGUID(CGID_DefView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
            case DVID_SETASDEFAULT:

//  99/02/05 #226140 vtan: Exec command issued from
//  CShellBrowser2::_SaveDefViewDefaultFolderSettings()
//  when user clicks "Like Current Folder" in folder
//  options "View" tab.

                ASSERTMSG(nCmdexecopt == OLECMDEXECOPT_DODEFAULT, "nCmdexecopt must be OLECMDEXECOPT_DODEFAULT");
                ASSERTMSG(pvarargIn == NULL, "pvarargIn must be NULL");
                ASSERTMSG(pvarargOut == NULL, "pvarargOut must be NULL");
                hres = _SaveGlobalViewState();
                break;
            case DVID_RESETDEFAULT:

//  99/02/05 #226140 vtan: Exec command issued from
//  CShellBrowser2::_ResetDefViewDefaultFolderSettings()
//  when user clicks "Reset All Folders" in folder
//  options "View" tab.

                ASSERTMSG(nCmdexecopt == OLECMDEXECOPT_DODEFAULT, "nCmdexecopt must be OLECMDEXECOPT_DODEFAULT");
                ASSERTMSG(pvarargIn == NULL, "pvarargIn must be NULL");
                ASSERTMSG(pvarargOut == NULL, "pvarargOut must be NULL");
                hres = _ResetGlobalViewState();
                break;
            default:
                break;
        }
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SHDVID_CANACTIVATENOW:
        {
            hres = (_fCanActivateNow) ? S_OK : S_FALSE;
            TraceMsg(TF_DEFVIEW, "Defview SHDVID_CANACTIVATENOW returning 0x%x", hres);
            return hres;
        }

        // NOTE: for a long time IOleCommandTarget was implemented
        // BUT it wasn't in the QI! At this late stage of the game
        // I'll be paranoid and not forward everything down to the
        // extended view. We'll just pick off CANACTIVATENOW...
        //
        default:
            return OLECMDERR_E_UNKNOWNGROUP;
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SBCMDID_GETPANE:
            V_I4(pvarargOut) = PANE_NONE;
            CallCB(SFVM_GETPANE, nCmdexecopt, (LPARAM)&V_I4(pvarargOut));
            return S_OK;

        default:
            break;
        }
    }
    else if (IsEqualGUID(IID_IExplorerToolbar, *pguidCmdGroup)) 
    {
        // handle the ones coming FROM itbar:
        switch(nCmdID) {
        case ETCMDID_GETBUTTONS:
            pvarargOut->vt = VT_BYREF;
            pvarargOut->byref = (void *)_pbtn;
            *pvarargIn->plVal = _cTotalButtons;
            return S_OK;

        case ETCMDID_RELOADBUTTONS:
            MergeToolBar(TRUE);
            return S_OK;
        }
    }
    else if (IsEqualGUID(_clsid, *pguidCmdGroup)) 
    {
        UEMFireEvent(&UEMIID_BROWSER, UEME_UITOOLBAR, UEMF_XEVENT, UIG_OTHER, nCmdID);

        DFVCMDDATA cd;
        cd.pva = pvarargIn;
        cd.hwnd = _hwndMain;
        cd.nCmdIDTranslated = 0;
        Command(NULL, nCmdID, (LPARAM)&cd);
    }

    // no need to pass OLECMDID_REFRESH on to the extended view, as we
    // just nuked and replaced the extended view above -- a super refresh of sorts.
    //
    if (m_cFrame.IsWebView() && hres!=S_OK)
    {
        //  Do not pass IDM_PARSECOMPLETE back to MSHTML.  This will cause them to load mshtmled.dll
        //  unecessarily for webview which is a significant performance hit.
        if (!(pguidCmdGroup && IsEqualGUID(CGID_MSHTML, *pguidCmdGroup) && (nCmdID == IDM_PARSECOMPLETE)))
        {
            IOleCommandTarget* pct;

            if (SUCCEEDED(m_cFrame.GetCommandTarget(&pct)))
            {
                hres = pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
                ATOMICRELEASE(pct);
            }
        }
    }

    return hres;
}


// BUGBUG: remove this and roll into caller...
void CDefView::_ShowAndActivate()
{
    // Can't call SetFocus because it rips focus away from such nice
    // UI elements like the TREE pane...
    // UIActivate will steal focus only if _uState is SVUIA_ACTIVATE_FOCUS
    UIActivate(_uState);
}

// IDefViewFrame2 (available only through QueryService from sfvext!)
//
HRESULT CDefView::GetWindowLV2(HWND * phwnd, IUnknown * punk)
{

    ASSERT(punk);
    if (punk && !_IsDesktop() && SUCCEEDED(punk->QueryInterface(IID_IWebViewOCWinMan, (void **)&_pocWinMan)))
    {
        if (!_fGetWindowLV)
        {
            _fGetWindowLV = TRUE;
            if (m_cFrame.IsSFVExtension())
            {
                HWND hwndXV;

                if (!(hwndXV = m_cFrame.GetExtendedViewWindow()))
                    return E_FAIL;
                *phwnd = hwndXV;
                m_cFrame.SetViewWindowStyle(WS_EX_CLIENTEDGE, 0);

                ASSERT(m_cFrame.GetExtendedISFV());
                ASSERT(m_pauto);
                m_cFrame.GetExtendedISFV()->SetAutomationObject(m_pauto);
            }
            else
            {
                // if shellview extension hosted in extended view(thumbnail in webview)
                // return the hwnd of the shellview extension.
                *phwnd = _hwndListview;
                if (_bUpdatePending)
                    _ReloadListviewContent();
            }
            
            // BUGBUG: This is in the WRONG place, should happen when we
            // go InPlaceActive.  Also, this only calls UIActivate, so we
            // should rename the message (and the helper function).  Also,
            // We might want to *cache* the UIActivate call (and NOT do it)
            // until we go inplace active, and then do it at that time.
            //
            // NOTE: The browser IP/UI-activates us when we become the
            // current active view!  We want to resize and show windows
            // at that time.  But if we're still waiting for _fCanActivateNow
            // Ie, ReadyStateInteractive, then we need to cache this request
            // and do it later.  NOTE: if Trident caches the focus (done w/ TAB)
            // then we don't need to do anything here. (Ie, remove below _ShowAndActivate
            // calls.)
            //
            if (!_fCanActivateNow)
                PostMessage(_hwndView, WM_DSV_REARRANGELISTVIEW, 0, 0);
            else
                _ShowAndActivate();
        }
        TraceMsg(TF_DEFVIEW, "GetWindowLV - TAKEN");
        return S_OK;
    }
    else
    {
        *phwnd = NULL;
        return E_FAIL;
    }
}

// IDefViewFrame
//
HRESULT CDefView::GetWindowLV(HWND * phwnd)
{
    ASSERT(0);  // What should I do here?
    return GetWindowLV2(phwnd, NULL);
}

HRESULT CDefView::ReleaseWindowLV()
{
    _fGetWindowLV = FALSE;

    WndSize(_hwndView);             // Make sure we resize _hwndListview
    ATOMICRELEASE(_pocWinMan);      // may be NULL 
    return S_OK;
}

HRESULT CDefView::GetShellFolder(IShellFolder **ppsf)
{
    *ppsf = _pshf;
    if (*ppsf)
        _pshf->AddRef();

    return *ppsf ? S_OK : E_FAIL;
}

// IServiceProvider
//

STDMETHODIMP CDefView::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;

    if (guidService == SID_DefView)
    {
        // DefViewOCs request this interface
        if (riid != IID_IDefViewFrame || !_IsDesktop())
            hr = QueryInterface(riid, ppv);
    } 
    else if (guidService == SID_ShellFolderViewCB)
    {
        IShellFolderViewCB * psfvcb = m_cCallback.GetSFVCB();
        if (psfvcb)
            hr = psfvcb->QueryInterface(riid, ppv);
    } 
    else
    {
        hr = IUnknown_QueryService(_psb, guidService, riid, ppv);
    }
    return hr;
}

STDMETHODIMP CDefView::OnSetTitle(VARIANTARG *pvTitle)
{
    DebugMsg(DM_ERROR, TEXT("CDefView::OnSetTitle - not implemented"));
    return E_NOTIMPL;
}

// friend create function for the Background context menu wrapper
HRESULT CBackgrndMenu_CreateInstance(CDefView *pDefView, REFIID riid, void **ppvObj)
{
    HRESULT hr = NOERROR;
    CBackgrndMenu * pMenu = new CBackgrndMenu( pDefView, &hr );
    if ( !pMenu )
    {
        return E_OUTOFMEMORY;
    }

    if ( FAILED( hr ))
    {
        delete pMenu;
        return hr;
    }

    hr = pMenu->QueryInterface( riid, ppvObj );
    ATOMICRELEASE(pMenu);

    return hr;
}

// The background ContextMenu handler. this is a wrapper for menu creation and
// the old menu handler than only responds to GetCommandString. This object is
// obtained by doing DefView->GetItemObjects( SVGIO_BACKGROUND, IID_IContextMenu );
CBackgrndMenu::CBackgrndMenu( CDefView *pDefView, HRESULT * pHr )
    : m_pDefView( pDefView ), m_cRef ( 1 )
{
    if ( !m_pDefView )
    {
        *pHr = E_INVALIDARG;
        return;
    }

    m_pDefView->AddRef();
    m_powsSite = NULL;

    // create the folder menu that used to be created (this is used to
    // handle the getcommand string)

    LPITEMIDLIST pidlFolder = pDefView->_GetViewPidl();
    *pHr = CDefFolderMenu_Create( pidlFolder,
        m_pDefView->_hwndMain,
        0, NULL,
        m_pDefView->_pshf,
        DefView_DFMCallBackBG,
        NULL, NULL,
        (IContextMenu **) &m_pFolderMenu);
    ILFree(pidlFolder);
}

CBackgrndMenu::~CBackgrndMenu()
{
    if (!m_fpcmSelAlreadyThere && m_pDefView && m_pcmSel && !m_fFlush )
    {
        if (m_pDefView->_pcmSel == m_pcmSel)
        {
            DV_FlushCachedMenu(m_pDefView);
        }
    }

    // robustness: we leak if we don't get SetSite-d to NULL
    if (m_pcmSel)
    {
        IUnknown_SetSite(SAFECAST(m_pcmSel,IContextMenu*), NULL);
        m_pcmSel->Release();
        m_pcmSel = NULL;
    }
    ATOMICRELEASE(m_powsSite);
    ATOMICRELEASE(m_pDefView);
    ATOMICRELEASE(m_pFolderMenu);
}

STDMETHODIMP CBackgrndMenu::QueryInterface ( REFIID riid, void **ppvObj )
{
    static const QITAB qit[] = {
        QITABENT(CBackgrndMenu, IContextMenu3),                  // IID_IContextMenu3
        QITABENTMULTI(CBackgrndMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CBackgrndMenu, IContextMenu, IContextMenu3), // IID_IContextMenu
        QITABENT(CBackgrndMenu, IObjectWithSite),                // IID_IObjectWithSite
        { 0 }
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CBackgrndMenu::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CBackgrndMenu::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;
    delete this;
    return 0;
}

STDMETHODIMP CBackgrndMenu::QueryContextMenu ( HMENU hmenu, UINT indexMenu,
    UINT idCmdFirst, UINT idCmdLast, UINT uFlags )
{

    HMENU hmContext;
    HRESULT hr = NOERROR;

    if (!(uFlags & (CMF_VERBSONLY | CMF_DVFILE)))
    {
        hmContext = SHLoadPopupMenu(HINST_THISDLL, POPUP_SFV_BACKGROUND);
        if (!hmContext)
        {
            // BUGBUG: There should be an error message here
            return E_OUTOFMEMORY;
        }

        // HACK: we are only initializing the Paste command, so we don't
        // need any attributes
        Def_InitEditCommands(0, hmContext, SFVIDM_FIRST, m_pDefView->_pdtgtBack,
            DIEC_BACKGROUNDCONTEXT);

        // If this defview is a part of common dialog, don't add the extended
        // view menu items. (except the ones marked as standard)
        m_pDefView->m_cFrame.MergeExtViewsMenu(hmContext, m_pDefView);

        m_pDefView->InitViewMenu(hmContext);
    }
    else if (!(hmContext = CreatePopupMenu()))
    {
        // BUGBUG: There should be an error message here
        return E_OUTOFMEMORY;
    }

    if (m_pDefView->_pcmSel == NULL)
    {
        IContextMenu *pcm = NULL;

        // it's okay to create and cache this context menu even if we have
        // an extended view because we're doing the background context menu
        // here, which is handled by the parent defview. _pcmSel is also
        // flushed at the end of this function.
        //
        hr = m_pDefView->_pshf->CreateViewObject(m_pDefView->_hwndMain,
                        IID_IContextMenu, (void **)&pcm);
        if ( SUCCEEDED( hr ))
        {
            m_pDefView->_pcmSel = pcm;
            m_pcmSel = pcm;
            m_pcmSel->AddRef();
        }
    }
    else
    {
        m_fpcmSelAlreadyThere = TRUE;
        m_pcmSel = m_pDefView->_pcmSel;
        m_pcmSel->AddRef();
    }

    if ( SUCCEEDED( hr ))
    {
        IUnknown_SetSite(m_pcmSel, m_powsSite);

        hr = m_pDefView->_pcmSel->QueryContextMenu( hmContext, (UINT) -1,
            SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST, uFlags);
    }

    // Always merge in evan if error.  RNAUI - fails the CreateViewObject and they rely on
    // simply having the default stuff...
    Shell_MergeMenus( hmenu, hmContext, indexMenu, idCmdFirst, idCmdLast, uFlags);

    DestroyMenu( hmContext );

    return hr;
}


STDMETHODIMP CBackgrndMenu::InvokeCommand ( LPCMINVOKECOMMANDINFO lpici )
{
    UINT idCmd = 0;
    HRESULT hr = E_FAIL;

    // Some of the commands passed to defview can release this CBackgrndMenu object even when
    // we are still processing this message. So, add an AddRef() - Release() pair of calls.
    AddRef();
    if (!IS_INTRESOURCE( lpici->lpVerb ))
    {
        // the general view commands don't accept verbs, only the
        // folder is likely to accept verbs, so pass it straight through...
        hr = m_pFolderMenu->InvokeCommand( lpici );

        if (FAILED(hr))
        {
            hr = m_pcmSel->InvokeCommand(lpici);
        }
    }
    else

    {
        idCmd = PtrToUlong((PVOID) lpici->lpVerb);

        hr = (HRESULT) m_pDefView->Command(m_pcmSel, GET_WM_COMMAND_MPS(idCmd, 0, 0));
    }

    if (!m_fpcmSelAlreadyThere)
    {
        if (m_pDefView->_pcmSel == m_pcmSel)
        {
            DV_FlushCachedMenu(m_pDefView);
        }
    }
    // we have passed the point when we would have flushed the selection menu..
    m_fFlush = TRUE;

    Release();

    return hr;
}


STDMETHODIMP CBackgrndMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwRes, LPSTR pszName, UINT cchMax)
{
    return m_pFolderMenu->GetCommandString(idCmd, uType, pwRes, pszName, cchMax);
}

STDMETHODIMP CBackgrndMenu::HandleMenuMsg(UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

STDMETHODIMP CBackgrndMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam,
                                           LPARAM lParam, LRESULT* plResult)
{
    // we cannot just pass this through to the DefView_WndProc because it will in
    // turn delegate back to the cached context menu (us)
    switch( uMsg )
    {
        case WM_INITMENUPOPUP:
            if (!(m_pDefView->OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam))))
                return NOERROR;
            break;

        case WM_MEASUREITEM:
            #define lpmis ((LPMEASUREITEMSTRUCT)lParam)

            if (lpmis->CtlType != ODT_MENU)
                return NOERROR;
            if (InRange(lpmis->itemID, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && m_pDefView->HasCB())
            {
                m_pDefView->CallCB(SFVM_MEASUREITEM, SFVIDM_CLIENT_FIRST, lParam);
                return NOERROR;
            }
            break;

        case WM_DRAWITEM:
            #define lpdis ((LPDRAWITEMSTRUCT)lParam)

            if (lpdis->CtlType != ODT_MENU)
                return NOERROR;
            if (InRange(lpdis->itemID, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && m_pDefView->HasCB())
            {
                m_pDefView->CallCB(SFVM_DRAWITEM, SFVIDM_CLIENT_FIRST, lParam);
                return NOERROR;
            }
            #undef lpdis
            break;
    }

    if (m_pcmSel)
    {
        IContextMenu2 *pcm2;
        IContextMenu3 *pcm3;
        if (SUCCEEDED(m_pcmSel->QueryInterface(IID_IContextMenu3, (void **)&pcm3)))
        {
            pcm3->HandleMenuMsg2(uMsg, wParam, lParam,plResult);
            ATOMICRELEASE(pcm3);
        }
        else if (SUCCEEDED(m_pcmSel->QueryInterface(IID_IContextMenu2, (void **)&pcm2)))
        {
            pcm2->HandleMenuMsg(uMsg, wParam, lParam);
            ATOMICRELEASE(pcm2);
        }
    }
    return NOERROR;
}

STDMETHODIMP CBackgrndMenu::SetSite(IUnknown* pSite)
{
    ATOMICRELEASE(m_powsSite);

    m_powsSite = pSite;

    if (pSite)
        m_powsSite->AddRef();

    if (m_pcmSel)
        IUnknown_SetSite(SAFECAST(m_pcmSel,IContextMenu*), pSite);
    return NOERROR;

}

STDMETHODIMP CBackgrndMenu::GetSite(REFIID riid,void** ppvObj)
{
    if (m_powsSite)
        return m_powsSite->QueryInterface(riid,ppvObj);
    else
    {
        ASSERT(ppvObj != NULL);
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

BOOL ShowInfoTip()
{
    // find out if infotips are on or off, from the registry settings
    SHELLSTATE ss;
    // force a refresh
    SHGetSetSettings(&ss, 0, TRUE);
    SHGetSetSettings(&ss, SSF_SHOWINFOTIP, FALSE);
    return ss.fShowInfoTip;
}

CBackgroundColInfo::CBackgroundColInfo (LPCITEMIDLIST pidl, UINT uiCol, STRRET& strRet) :
    _pidl(pidl),
    _uiCol(uiCol)

{
    StrRetToBuf(&strRet, NULL, const_cast<TCHAR*>(_szText), ARRAYSIZE(_szText));
}

CBackgroundColInfo::~CBackgroundColInfo (void)

{
    ILFree(const_cast<LPITEMIDLIST>(_pidl));
}
