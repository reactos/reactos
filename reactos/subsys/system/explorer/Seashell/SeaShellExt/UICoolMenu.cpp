////////////////////////////////////////////////////////////////
// CoolMenu 1997 Microsoft Systems Journal. 
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// ==========================================================================  
// HISTORY:   
// ==========================================================================
//    1.01  13 Aug 1998 - Andrew Bancroft [ABancroft@lgc.com] - Since we've already 
//                added the entire toolbar to the imagelist we need to 
//                increment nNextImage even if we didn't add this button to 
//                m_mapIDtoImage in the LoadToolbar() method.
//    1.01a 13 Aug 1998 - Peter Tewkesbury - Added AddSingleBitmap(...)
//                method for adding a single bitmap to a pulldown
//                menu item.
//    1.02  13 Aug 1998 - Omar L Francisco - Fixed bug with lpds->CtlType
//                and lpds->itemData item checking.
//    1.03  12 Nov 1998 - Fixes debug assert in system menu. - Wang Jun
//    1.04  17 Nov 1998 - Fixes debug assert when you maximize a view - Wang Jun
//                window, then try to use the system menu for the view.
//    1.05  09 Jan 1998 - Seain B. Conover [sc@tarasoft.com] - Fix for virtual 
//                key names.
//    1.06  24 Feb 1999 - Michael Lange [michael.home@topdogg.com] - Fix for memory 
//                leak in CMyItemData structure, added a destructor that 
//                calls text.Empty().
//              - Boris Kartamishev [kbv@omegasoftware.com] - Fix for resource
//                ID bug.
//              - Jeremy Horgan [jeremyhorgan@hotmail.com] - During 
//                accelerator key processing OnInitMenuPopup() calls 
//                ConvertMenu() which allocates a new CMyItemData for each 
//                  menu item. This is memory is normally freed by a call to 
//                  OnMenuSelect(), which is not called when processing 
//                accelerator keys. This results in a memory leak. This was
//                fixed by modifying the ~CCoolMenuManager() destructor.
//    1.07  24 Feb 1999 - Koji MATSUNAMI [kmatsu@inse.co.jp] - Fixed problem with 
//                popup menus being drawn correctly as cool menus.
//    1.08  7 Jul 2000 - Philip Oldaker [philip@masmex.com] - Added support for
//				IContextMenu2 as used by the windows shell for owner draw menus
// ==========================================================================
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UICoolMenu.h"

#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define safe_delete(p){if(p){delete p;p=NULL;}}

#define USES_ICON  -2

CCoolMenuManager g_CoolMenuManager;

// helpers
void PLFillRect(CDC& dc, const CRect& rc, COLORREF color);
void PLDrawEmbossed(CDC& dc, CImageList& il, int i,
  CPoint p, BOOL bColor=FALSE);
HBITMAP PLLoadSysColorBitmap(LPCTSTR lpResName, BOOL bMono=FALSE);
inline HBITMAP PLLoadSysColorBitmap(UINT nResID, BOOL bMono=FALSE) {
  return PLLoadSysColorBitmap(MAKEINTRESOURCE(nResID), bMono);
}

// if you want to see extra TRACE diagnostics, set below to TRUE
BOOL CCoolMenuManager::bTRACE = FALSE;

#ifdef _DEBUG
#define CMTRACEFN     \
  CTraceFn __fooble;  \
  if (bTRACE)       \
    TRACE
#define CMTRACE     \
  if (bTRACE)       \
    TRACE
#else
#define CMTRACEFN TRACE
#define CMTRACE   TRACE
#endif

// constants used for drawing
const CXGAP = 1;        // num pixels between button and text
const CXTEXTMARGIN = 2;   // num pixels after hilite to start text
const CXBUTTONMARGIN = 2; // num pixels wider button is than bitmap
const CYBUTTONMARGIN = 2; // ditto for height

// DrawText flags
const DT_MYSTANDARD = DT_SINGLELINE|DT_LEFT|DT_VCENTER;

// identifies owner-draw data as mine
const LONG MYITEMID = MAKELONG(MAKEWORD(_T('m'),_T('i')),MAKEWORD(_T('d'),_T('0')));

// private struct: one of these for each owner-draw menu item
struct CMyItemData {
    long    magicNum;   // magic number identifying me
    CString text;       // item text
    UINT    fType;      // original item type flags
    int     iButton;    // index of button image in image list (or USES_ICON)
    HICON   hIcon;
    void *  pContext;	// item data
	LPCONTEXTMENU lpcm; // if set this will handle menu messages (windows shell)
    CMyItemData()       { lpcm=NULL; magicNum = MYITEMID; iButton = -1; hIcon = 0; pContext = NULL; }
    ~CMyItemData()        { text.Empty(); if (hIcon) ::DestroyIcon(hIcon); if (lpcm) lpcm->Release(); }
    BOOL IsMyItemData()   { return this != NULL  &&  magicNum == MYITEMID; }
};

static CMyItemData *GetMyItemData(HMENU hMenu, UINT uItem, 
                                  BOOL bByPosition = FALSE)
{
    MENUITEMINFO mii;
	ZeroMemory(&mii,sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_DATA;
    ::GetMenuItemInfo(hMenu, uItem, bByPosition, &mii);
    return (CMyItemData *)mii.dwItemData;
}

static void SetMyItemData(CMyItemData *pmd, HMENU hMenu, UINT uItem, 
                          BOOL bByPosition = FALSE)
{
    MENUITEMINFO mii;
    mii.cbSize     = sizeof(mii);
    mii.fMask      = MIIM_DATA;
    mii.dwItemData = (DWORD)pmd;
    ::SetMenuItemInfo(hMenu, uItem, bByPosition, &mii);
}


IMPLEMENT_DYNAMIC(CCoolMenuManager, CSubclassWnd)

CCoolMenuManager::CCoolMenuManager()
{
	m_lpcm = NULL;		
	m_szBitmap = CSize(0,0); // will compute later
	m_szButton = CSize(0,0); 
	m_bShowButtons = TRUE;            // show buttons by default
	m_bAutoAccel = TRUE;              // auto accelerators by default
	m_pAccel = NULL;                // no accelerators loaded yet
	m_bUseDrawState = FALSE;          // use DrawEmbossed by default
	m_bDrawDisabledButtonsInColor = FALSE;  // use color for disabled buttons
	FixMFCDotBitmap();
}

CCoolMenuManager::~CCoolMenuManager()
{
  // Jeremy Horgan [jeremyhorgan@hotmail.com]

  while (!m_menuList.IsEmpty()) 
  {
      UnconvertMenu(CMenu::FromHandle((HMENU)m_menuList.RemoveHead()));
  }
  
  Destroy();
}

//////////////////
// Destroy everything. Called from destructor and Refresh.
//
void CCoolMenuManager::Destroy()
{
  m_ilButtons.DeleteImageList();
  m_ilTemp.DeleteImageList();
  m_mapIDtoImage.RemoveAll();
  m_mapHMENUtoID.RemoveAll();
  m_szBitmap = m_szButton = CSize(0,0);
  m_arToolbarID.RemoveAll();
  m_fontMenu.DeleteObject();
  DestroyAccel();
}

/////////////////
// Destroy accelerators
//
void CCoolMenuManager::DestroyAccel()
{
  m_mapIDtoAccel.RemoveAll();   // delete ACCEL entries in map
  safe_delete(m_pAccel);      // delete current accelerators
}

//////////////////
// Call this to install the menu manager. Install(NULL) to un-install.
//
void CCoolMenuManager::Install(CWnd* pFrame)
{
	m_pFrame = pFrame;
	HookWindow(pFrame);   // install message hook
}

void CCoolMenuManager::Uninstall()
{
	m_pFrame = NULL;
	HookWindow((HWND)NULL); 
}

// Addition: Phliip Oldaker
// call this to enable help text on the status bar
void CCoolMenuManager::SetShellContextMenu(LPCONTEXTMENU lpcm,UINT nIDFirst,UINT nIDLast)
{
	m_lpcm = lpcm;
	m_idShellMenuFirst = nIDFirst;
	m_idShellMenuLast = nIDLast;
}

//////////////////
// Load array of toolbar IDs.
//
BOOL CCoolMenuManager::LoadToolbars(const UINT* arID, int n)
{
  ASSERT(arID);
  BOOL bRet = TRUE;
  for (int i=0; i<n; i++)
    bRet |= LoadToolbar(arID[i]);
  return bRet;
}

// structure of RT_TOOLBAR resource
struct TOOLBARDATA {
  WORD wVersion;    // version # should be 1
  WORD wWidth;    // width of one bitmap
  WORD wHeight;   // height of one bitmap
  WORD wItemCount;  // number of items
  WORD items[1];    // array of command IDs, actual size is wItemCount
};

// Addition: Philip Oldaker
CSize CCoolMenuManager::SetButtonSize(CSize &sz)
{
	// Set defaults
	if (sz.cx == 0)
		sz.cx = 16;
	if (sz.cy == 0)
		sz.cy = 15;
	CSize szOld(m_szBitmap);
	m_szBitmap = sz;
	m_szButton = sz + CSize(CXBUTTONMARGIN<<1, CYBUTTONMARGIN<<1);
	return szOld;
}

//////////////////
// Load one toolbar. Assumes bg color is gray.
// 
//  * add toolbar bitmap to image list
//   * add each button ID to button map
//
BOOL CCoolMenuManager::LoadToolbar(UINT nIDToolbar)
{
  // load bitmap
  HBITMAP hbmToolbar = PLLoadSysColorBitmap(nIDToolbar);
  if (!hbmToolbar) {
    TRACE(_T("*** Can't load bitmap for toolbar %d!\n"), nIDToolbar);
    return FALSE;
  }

  // load toolbar
  LPTSTR lpResName = MAKEINTRESOURCE(nIDToolbar);
  HINSTANCE hInst;
  HRSRC hRsrc;
  TOOLBARDATA* ptbd;
  if ((hInst= AfxFindResourceHandle(lpResName, RT_TOOLBAR)) == NULL ||
     (hRsrc= FindResource(hInst, lpResName, RT_TOOLBAR))   == NULL ||
     (ptbd = (TOOLBARDATA*)LoadResource(hInst, hRsrc))     == NULL) {

    TRACE(_T("*** Can't load toolbar %d!\n"), nIDToolbar);
    return FALSE;
  }
  ASSERT(ptbd->wVersion==1);
    
  // OK, I have the bitmap and toolbar. 

  CSize sz(ptbd->wWidth, ptbd->wHeight);
  if (m_szBitmap.cx==0) {
    // First toolbar: initialized bitmap/button sizes and create image list.
    m_szBitmap = sz;
    m_szButton = sz + CSize(CXBUTTONMARGIN<<1, CYBUTTONMARGIN<<1);
    VERIFY(m_ilButtons.Create(sz.cx, sz.cy, ILC_COLOR32|ILC_MASK, 0, 10));
    VERIFY(m_ilTemp.   Create(sz.cx, sz.cy, ILC_COLOR32|ILC_MASK, 0, 10));

  } else if (m_szBitmap != sz) {
    // button sizes different -- oops
    TRACE(_T("*** Toolbar %d button size differs!\n"), nIDToolbar);
    return FALSE;
  }

  m_ilButtons.SetBkColor(::GetSysColor(COLOR_3DFACE));

  // I have a good toolbar: now add bitmap to the image list, and each
  // command ID to m_mapIDtoImage array. Note that LoadSysColorBitmap will
  // change gray -> COLOR_3DFACE, so use that for image list background.
  //
  int iNextImage = m_ilButtons.GetImageCount();
  m_ilButtons.Add(CBitmap::FromHandle(hbmToolbar), GetSysColor(COLOR_3DFACE));
  for (int i = 0; i < ptbd->wItemCount; i++) {
    UINT nID = ptbd->items[i];
    if (nID > 0) {
      if (GetButtonIndex(nID) >= 0) {
        TRACE(_T("*** Duplicate button ID %d ignored\n"), nID);
      } else {
        m_mapIDtoImage.SetAt(nID, (void*)iNextImage);
        TRACE(_T("CCoolMenuManager::LoadToolbar(). Added Menu Id %d, Button Number %d\n"), nID, iNextImage-1);
      }
      // AB. 13-08-98. Since we've already added the entire toolbar to the imagelist
      // we need to increment nNextImage even if we didn't add this button to
      // m_mapIDtoImage.
      iNextImage++;
    }
  }
  m_arToolbarID.Add(nIDToolbar);  // remember toolbar ID for Refresh
  ::DeleteObject(hbmToolbar);
  return TRUE; // success!
}

//////////////////
// Virtual CCoolMenuSubclassWnd window proc. All messages come here before frame
// window. Isn't it cool? Just like in the old days!
//
LRESULT CCoolMenuManager::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
  switch(msg) {
      case WM_SYSCOLORCHANGE:
      case WM_SETTINGCHANGE:
        Refresh();
        break;

      case WM_MEASUREITEM:
        if (OnMeasureItem((MEASUREITEMSTRUCT*)lp))
          return TRUE; // handled
        break;

      case WM_DRAWITEM:
        if (OnDrawItem((DRAWITEMSTRUCT*)lp))
          return TRUE; // handled
        break;

      case WM_INITMENUPOPUP:
        // Very important: must let frame window handle it first!
        // Because if someone calls CCmdUI::SetText, MFC will change item to
        // MFT_STRING, so I must change back to MFT_OWNERDRAW.
        //
        CSubclassWnd::WindowProc(msg, wp, lp);
        OnInitMenuPopup(CMenu::FromHandle((HMENU)wp),
          /*(UINT)LOWORD(lp)*/0, (BOOL)HIWORD(lp));
        return 0;

      case WM_MENUSELECT:
        OnMenuSelect((UINT)LOWORD(wp), (UINT)HIWORD(wp), (HMENU)lp);
        break;

      case WM_MENUCHAR:
        LRESULT lr = OnMenuChar((TCHAR)LOWORD(wp), (UINT)HIWORD(wp),
          CMenu::FromHandle((HMENU)lp));
        if (lr!=0)
          return lr;
        break;
  }
  return CSubclassWnd::WindowProc(msg, wp, lp);
}

//////////////////
// Refresh all colors, fonts, etc. For WM_SETTINGCHANGE, WM_SYSCOLORCHANGE.
//
void CCoolMenuManager::Refresh()
{
  // first copy list (array) of toolbar IDs now loaded.
  CUIntArray arToolbarID;
  arToolbarID.Copy(m_arToolbarID);

  // destroy everything
  Destroy();

  // re-load toolbars.
  int nToolbars = arToolbarID.GetSize();
  for (int i = 0; i < nToolbars; i++)
    LoadToolbar(arToolbarID[i]);
}

//////////////////
// Get menu font, creating if needed
//
CFont* CCoolMenuManager::GetMenuFont()
{
  if (!(HFONT)m_fontMenu) {
    NONCLIENTMETRICS info;
    info.cbSize = sizeof(info);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(info), &info, 0);
    VERIFY(m_fontMenu.CreateFontIndirect(&info.lfMenuFont));
  }
  return &m_fontMenu;
}

//////////////////
// Handle WM_MEASUREITEM on behalf of frame: compute menu item size.
//
BOOL CCoolMenuManager::OnMeasureItem(LPMEASUREITEMSTRUCT lpms)
{
	ASSERT(lpms);
	CMyItemData* pmd = (CMyItemData*)lpms->itemData;
	ASSERT(pmd);
	if (lpms->CtlType != ODT_MENU || !pmd->IsMyItemData())
		return FALSE; // not handled by me
	// Addition: Philip Oldaker
	// Pass it on to the shell if lpcm has been set
	if (pmd->lpcm)
	{
		IContextMenu2 *lpcm2=NULL;
		HRESULT hr = pmd->lpcm->QueryInterface(IID_IContextMenu2,(LPVOID*)&lpcm2);
		if (SUCCEEDED(hr))
		{
			// switch context item data
			lpms->itemData = (DWORD)pmd->pContext;
			lpcm2->HandleMenuMsg(WM_MEASUREITEM,0,(LPARAM)lpms);
			// clean up
			lpms->itemData = (DWORD)pmd;
			lpcm2->Release();
		}
		return TRUE;
	}
	/////////////////////
	if (pmd->fType & MFT_SEPARATOR) {
	// separator: use half system height and zero width
	lpms->itemHeight = GetSystemMetrics(SM_CYMENU)>>1;
	lpms->itemWidth  = 0;

	} else {

    // compute size of text: use DrawText with DT_CALCRECT

    CWindowDC dc(NULL); // screen DC--I won't actually draw on it
    CRect rcText(0,0,0,0);
    CFont* pOldFont = dc.SelectObject(GetMenuFont());
    dc.DrawText(pmd->text, rcText, DT_MYSTANDARD|DT_CALCRECT);
    dc.SelectObject(pOldFont);

    // height of item is just height of a standard menu item
    lpms->itemHeight= max(GetSystemMetrics(SM_CYMENU), rcText.Height());

    // width is width of text plus a bunch of stuff
    int cx = rcText.Width();  // text width 
    cx += CXTEXTMARGIN<<1;    // L/R margin for readability
    cx += CXGAP;          // space between button and menu text
    cx += m_szButton.cx<<1;   // button width (L=button; R=empty margin)

    // whatever value I return in lpms->itemWidth, Windows will add the
    // width of a menu checkmark, so I must subtract to defeat Windows. Argh.
    //
    cx -= GetSystemMetrics(SM_CXMENUCHECK)-1;
    lpms->itemWidth = cx;   // done deal

    CMTRACE(_T("OnMeasureItem for '%s':\tw=%d h=%d\n"), (LPCTSTR)pmd->text,
      lpms->itemWidth, lpms->itemHeight);
  }
  return TRUE; // handled
}

/////////////////
// Handle WM_DRAWITEM on behalf of frame. Note: in all that goes
// below, can't assume rcItem.left=0 because of multi-column menus!
//
BOOL CCoolMenuManager::OnDrawItem(LPDRAWITEMSTRUCT lpds)
{
	ASSERT(lpds);

	// Omar L Francisco
	if (lpds->CtlType != ODT_MENU)
		return FALSE;
  
	// Omar L Francisco
	CMyItemData* pmd = (CMyItemData*)lpds->itemData;
	if (!pmd->IsMyItemData())
		return FALSE;
	// Addition: Philip Oldaker
	// Pass it on to the shell if this has been set
	if (pmd->lpcm)
	{
		IContextMenu2 *lpcm2=NULL;
		HRESULT hr = pmd->lpcm->QueryInterface(IID_IContextMenu2,(LPVOID*)&lpcm2);
		if (SUCCEEDED(hr))
		{
			lpds->itemData = (DWORD)pmd->pContext;
			lpcm2->HandleMenuMsg(WM_DRAWITEM,0,(LPARAM)lpds);
			lpds->itemData = (DWORD)pmd;
			lpcm2->Release();
		}
		return TRUE;
	}
	////////////////////////////////
  ASSERT(lpds->itemAction != ODA_FOCUS);
  ASSERT(lpds->hDC);
  CDC dc;
  dc.Attach(lpds->hDC);

  const CRect& rcItem = lpds->rcItem;
  if (pmd->fType & MFT_SEPARATOR) {
    // draw separator
    CRect rc = rcItem;                // copy rect
    rc.top += rc.Height()>>1;           // vertical center
    dc.DrawEdge(&rc, EDGE_ETCHED, BF_TOP);    // draw separator line

  } else {                          // not a separator

    CMTRACE(_T("OnDrawItem for '%s':\tw=%d h=%d\n"), (LPCTSTR)pmd->text,
      rcItem.Width(), rcItem.Height());

    BOOL bDisabled = lpds->itemState & ODS_GRAYED;
    BOOL bSelected = lpds->itemState & ODS_SELECTED;
    BOOL bChecked  = lpds->itemState & ODS_CHECKED;
    BOOL bHaveButn=FALSE;

    // Paint button, or blank if none
    CRect rcButn(rcItem.TopLeft(), m_szButton); // button rect
    rcButn += CPoint(0,                 // center vertically
      (rcItem.Height() - rcButn.Height())>>1 );

    int iButton = pmd->iButton;
    if (iButton >= 0  ||  iButton == USES_ICON) 
    {
      // this item has a button!
      bHaveButn = TRUE;

      // compute point to start drawing
      CSize sz = rcButn.Size() - m_szBitmap;
      sz.cx >>= 1;
      sz.cy >>= 1;
      CPoint p(rcButn.TopLeft() + sz);

      // draw disabled or normal
      if (!bDisabled) {
        // normal: fill BG depending on state
        PLFillRect(dc, rcButn, GetSysColor(
          (bChecked && !bSelected) ? COLOR_3DLIGHT : COLOR_MENU));

        // draw pushed-in or popped-out edge
        if (bSelected || bChecked) {
          CRect rc2 = rcButn;
          dc.DrawEdge(rc2, bChecked ? BDR_SUNKENOUTER : BDR_RAISEDINNER,
            BF_RECT);
        }

        // draw the button!
        if (iButton != USES_ICON)
        {
            m_ilButtons.Draw(&dc, iButton, p, ILD_TRANSPARENT);
        }
        else
        {
            dc.DrawState(p, CSize(0,0), pmd->hIcon, DSS_NORMAL, 
                (HBRUSH)NULL);
        }

      } else if (m_bUseDrawState) {
        // use DrawState to draw disabled button: must convert to icon
        if (iButton != USES_ICON)
        {
            HICON hIcon=m_ilButtons.ExtractIcon(iButton);
            ASSERT(hIcon);
            dc.DrawState(p, CSize(0,0), hIcon, DSS_DISABLED, (HBRUSH)NULL);
            DestroyIcon(hIcon);
        }
        else
        {
            dc.DrawState(p, CSize(0,0), pmd->hIcon, DSS_DISABLED, 
                (HBRUSH)NULL);
        }
      } else
        // use DrawEmbossed to draw disabeld button, w/color flag
        if (iButton != USES_ICON)
        {
            PLDrawEmbossed(dc, m_ilButtons, iButton, p,
              m_bDrawDisabledButtonsInColor);
        }
        else
        {
            int i = m_ilTemp.Add(pmd->hIcon);
            PLDrawEmbossed(dc, m_ilTemp, i, p, 
                m_bDrawDisabledButtonsInColor);
            m_ilTemp.Remove(i);
        }
    } else {
      // no button: look for custom checked/unchecked bitmaps
      CMenuItemInfo info;
      info.fMask = MIIM_CHECKMARKS;
      GetMenuItemInfo((HMENU)lpds->hwndItem,
        lpds->itemID, MF_BYCOMMAND, &info);
      if (bChecked || info.hbmpUnchecked) {
        bHaveButn = Draw3DCheckmark(dc, rcButn, bSelected,
          bChecked ? info.hbmpChecked : info.hbmpUnchecked);
      }
    }

    // Done with button, now paint text. First do background if needed.
    int cxButn = m_szButton.cx;       // width of button
    COLORREF colorBG = GetSysColor(bSelected ? COLOR_HIGHLIGHT : COLOR_MENU);
    if (bSelected || lpds->itemAction==ODA_SELECT) {
      // selected or selection state changed: paint text background
      CRect rcBG = rcItem;              // whole rectangle
      if (bHaveButn)                  // if there's a button:
        rcBG.left += cxButn + CXGAP;      //  don't paint over it!
      PLFillRect(dc, rcBG, colorBG);  // paint it!
    }

    // compute text rectangle and colors
    CRect rcText = rcItem;         // start w/whole item
    rcText.left += cxButn + CXGAP + CXTEXTMARGIN; // left margin
    rcText.right -= cxButn;        // right margin
    dc.SetBkMode(TRANSPARENT);       // paint transparent text
    COLORREF colorText = GetSysColor(bDisabled ?  COLOR_GRAYTEXT :
      bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);

    // Now paint menu item text.  No need to select font,
    // because windows sets it up before sending WM_DRAWITEM
    //
    if (bDisabled && (!bSelected || colorText == colorBG)) {
      // disabled: draw hilite text shifted southeast 1 pixel for embossed
      // look. Don't do it if item is selected, tho--unless text color same
      // as menu highlight color. Got it?
      //
      DrawMenuText(dc, rcText + CPoint(1,1), pmd->text,
        GetSysColor(COLOR_3DHILIGHT));
    }
    DrawMenuText(dc, rcText, pmd->text, colorText); // finally!
  }
  dc.Detach();

  return TRUE; // handled
}

/////////////////
// Helper function to draw justified menu text. If the text contains a TAB,
// draw everything after the tab right-aligned
//
void CCoolMenuManager::DrawMenuText(CDC& dc, CRect rc, CString text,
  COLORREF color)
{
  CString left = text;
  CString right;
  int iTabPos = left.Find(_T('\t'));
  if (iTabPos >= 0) {
    right = left.Right(left.GetLength() - iTabPos - 1);
    left  = left.Left(iTabPos);
  }
  dc.SetTextColor(color);
  dc.DrawText(left, &rc, DT_MYSTANDARD);
  if (iTabPos > 0)
    dc.DrawText(right, &rc, DT_MYSTANDARD|DT_RIGHT);
}

#ifndef OBM_CHECK
#define OBM_CHECK 32760 // from winuser.h
#endif

//////////////////
// Draw 3D checkmark
//
//    dc        device context to draw in
//    rc        rectangle to center bitmap in
//    bSelected TRUE if button is also selected
//    hbmCheck    Checkmark bitmap to use, or NULL for default
//
BOOL CCoolMenuManager::Draw3DCheckmark(CDC& dc, const CRect& rc,
  BOOL bSelected, HBITMAP hbmCheck)
{
  // get checkmark bitmap if none, use Windows standard
  if (!hbmCheck) {
    CBitmap bm;
    VERIFY(bm.LoadOEMBitmap(OBM_CHECK));
    hbmCheck = (HBITMAP)bm.Detach();
    ASSERT(hbmCheck);
  }
  
  // center bitmap in caller's rectangle
  BITMAP bm;
  ::GetObject(hbmCheck, sizeof(bm), &bm);
  int cx = bm.bmWidth;
  int cy = bm.bmHeight;
  CRect rcDest = rc;
  CPoint p(0,0);
  CSize delta(CPoint((rc.Width() - cx)/2, (rc.Height() - cy)/2));
  if (rc.Width() > cx)
    rcDest = CRect(rc.TopLeft() + delta, CSize(cx, cy));
  else
    p -= delta;

  // select checkmark into memory DC
  CDC memdc;
  memdc.CreateCompatibleDC(&dc);
  HBITMAP hOldBM = (HBITMAP)::SelectObject(memdc, hbmCheck);

  // set BG color based on selected state
  COLORREF colorOld =
    dc.SetBkColor(GetSysColor(bSelected ? COLOR_MENU : COLOR_3DLIGHT));
  dc.BitBlt(rcDest.left, rcDest.top, rcDest.Width(), rcDest.Height(),
    &memdc, p.x, p.y, SRCCOPY);
  dc.SetBkColor(colorOld);

  ::SelectObject(memdc, hOldBM); // restore

  // draw pushed-in hilight.
  if (rc.Width() > cx)        // if room:
    rcDest.InflateRect(1,1);  // inflate checkmark by one pixel all around
  dc.DrawEdge(&rcDest, BDR_SUNKENOUTER, BF_RECT);

  return TRUE;
}

//////////////////
// Handle WM_INITMENUPOPUP on behalf of frame.
//
void CCoolMenuManager::OnInitMenuPopup(CMenu* pMenu,
  UINT nIndex, BOOL bSysMenu)
{
	// Addition: Philip Oldaker
	/////////////////////////
	// Let the shell handle it
	if (bSysMenu)
		return;
	for(UINT i=0;i < pMenu->GetMenuItemCount();i++)
	{
		CMyItemData* pmd = GetMyItemData(*pMenu,i,TRUE);
		if (pmd->IsMyItemData() && pmd->lpcm)
		{
			IContextMenu2 *lpcm2=NULL;
			HRESULT hr = pmd->lpcm->QueryInterface(IID_IContextMenu2,(LPVOID*)&lpcm2);
			if (SUCCEEDED(hr))
			{
				SetMyItemData((CMyItemData*)pmd->pContext,*pMenu,i,TRUE); 
				lpcm2->HandleMenuMsg(WM_INITMENUPOPUP,(WPARAM)pMenu->GetSafeHmenu(),0);
				SetMyItemData(pmd,*pMenu,i,TRUE); 
				lpcm2->Release();
			}
			return;
		}
	}
	//////////////////////
  if (m_bAutoAccel)
  {
    // check for new accels. If ASSERT bombs,
    // you forgot to call Install.
//    ASSERT_VALID(m_pFrame);
    
//    HACCEL hAccel = m_pFrame->GetDefaultAccelerator();
    HACCEL hAccel = NULL;
    
    if (hAccel != m_hAccel)
      LoadAccel(hAccel);

    // 12 Nov 1998  - Wang Jun - Fixes debug assert in system menu.
    // Check if click system menu.
    if (!bSysMenu) {
      ConvertMenu(pMenu, nIndex, bSysMenu, m_bShowButtons);
    }
  }
  ConvertMenu(pMenu, nIndex, bSysMenu, m_bShowButtons);
}

//////////////////
// Set the accelerator table used to generate automatic key
// names in menus. Delete previous table if any.
//
void CCoolMenuManager::LoadAccel(HACCEL hAccel)
{
  DestroyAccel();
  int nAccel;
  if (hAccel && (nAccel = CopyAcceleratorTable(hAccel, NULL, 0)) > 0) {
    m_pAccel = new ACCEL [nAccel];
    ASSERT(m_pAccel);
    CopyAcceleratorTable(hAccel, m_pAccel, nAccel);

    // Now I have the accelerators. Look over list, linking each command
    // ID with its ACCEL structure--i.e., m_mapIDtoAccel[nID] = ACCEL for
    // that ID. If more than one ACCEL for a given command (command has more
    // than one shortcut), fix up so ACCEL.cmd is offset of prev ACCEL
    // 
    for (int i=0; i<nAccel; i++) {
      ACCEL& ac = m_pAccel[i];
      ACCEL* pAccel = GetAccel(ac.cmd);
      m_mapIDtoAccel.SetAt(ac.cmd, &ac);
      ac.cmd = pAccel ? &ac - pAccel : 0; // ac.cmd = offset of prev, or 0
    }
  }
}

void CCoolMenuManager::ConvertMenu(CMenu *pMenu)
{
    ConvertMenu(pMenu, 0, FALSE, TRUE);
}

void CCoolMenuManager::UnconvertMenu(CMenu *pMenu)
{
    ConvertMenu(pMenu, 0, FALSE, FALSE);
}

void CCoolMenuManager::ConvertMenuItem(CMenu *pMenu, int iIndex, 
    bool bRecurse /*= true*/, bool bUnconvert /*= false*/)
{
    ASSERT_VALID(pMenu);
    CString   sItemName;

    // get menu item info
    TCHAR itemname[_MAX_PATH];
    CMenuItemInfo info;
    info.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE;
    info.dwTypeData = itemname;
    info.cch = sizeof(itemname);
    ::GetMenuItemInfo(*pMenu, iIndex, TRUE, &info);
    
    // O.S. - ignore separators
    if (info.fType & MFT_SEPARATOR)
        return;

    CMyItemData* pmd = (CMyItemData*)info.dwItemData;
    // O.S. - recurse sub-menus
    if (bRecurse && info.hSubMenu)
    {
        ConvertMenu(pMenu->GetSubMenu(iIndex), 0, FALSE, !bUnconvert);
    }
    
    if ((info.fType & MFT_OWNERDRAW)  && pmd  &&  !pmd->IsMyItemData()) {
        CMTRACE(_T("CCoolMenuManager: ignoring foreign owner-draw item\n"));
        return;
        // owner-draw menu item isn't mine--leave it alone
    }
    
/*    // Koji MATSUNAMI 1999.2.23
    //
    if (bSysMenu && info.wID >= 0xF000) {
        CMTRACE(_T("CCoolMenuManager: ignoring sys menu item\n"));
        continue; // don't do for system menu commands
    }
*/    
    // now that I have the info, I will modify it
    info.fMask = 0; // assume nothing to change
    
    if (!bUnconvert) 
    {
        // I'm showing buttons: convert to owner-draw
        
        if (!(info.fType & MFT_OWNERDRAW))
		{
            // If not already owner-draw, make it so. NOTE: If app calls
            // pCmdUI->SetText to change the text of a menu item, MFC will
            // turn the item to MFT_STRING. So I must set it back to
            // MFT_OWNERDRAW again. In this case, the menu item data (pmd)
            // will still be there.
            // 
            info.fType |= MFT_OWNERDRAW;
            info.fMask |= MIIM_TYPE;
            if (!pmd || (pmd && !pmd->IsMyItemData()))
			{                   // if no item data:
				if (pmd && !pmd->IsMyItemData())
				{
					CMyItemData* pnmd = new CMyItemData;    //   create one
					pnmd->pContext = pmd;
					pmd = pnmd;
				}
				else
					pmd = new CMyItemData;    //   create one
                ASSERT(pmd);              //   (I hope)
                pmd->fType = info.fType;  //   handy when drawing
                if (info.hSubMenu)
                {
                    int nID;
                    if (m_mapHMENUtoID.Lookup((void *)info.wID, (WORD&)nID))
                    {
                        pmd->iButton = GetButtonIndex(nID);
                    }
                    else
                    {
                        pmd->iButton = -1;
                    }
                }
                else
                    pmd->iButton = GetButtonIndex(info.wID);
                
                info.dwItemData = (DWORD)pmd;   //   set in menu item data
                info.fMask |= MIIM_DATA;      //   set item data
            }
            pmd->text = info.dwTypeData;      // copy menu item string

            // append accelerators to menu item name
            if (m_pAccel && m_bAutoAccel)
                AppendAccelName(pmd->text, info.wID);
        }
   } 
   else 
   {
        // no buttons -- I'm converting to strings
        
        if (info.fType & MFT_OWNERDRAW)  // if ownerdraw:
        { 
            info.fType &= ~MFT_OWNERDRAW;   //   turn it off
            info.fMask |= MIIM_TYPE;        //   change item type
            ASSERT(pmd);                    //   sanity check
            sItemName = pmd->text;          //   save name before deleting pmd
        } 
        else                    // otherwise: 
        {
            sItemName = info.dwTypeData;    //   use name from MENUITEMINFO
            pmd       = NULL;
        }
        
        if (pmd) {
            // NOTE: pmd (item data) could still be left hanging around even
            // if MFT_OWNERDRAW is not set, in case mentioned above where app
            // calls pCmdUI->SetText to set text of item and MFC sets the type
            // to MFT_STRING.
            //
            info.dwItemData = (DWORD)pmd->pContext; // item data<-user's context
            info.fMask |= MIIM_DATA;                // change it
            safe_delete(pmd);                       // and item data too
        }
        
        // possibly add accelerator name
        if (m_pAccel  && m_bAutoAccel && AppendAccelName(sItemName, info.wID))
            info.fMask |= MIIM_TYPE;      //  change item type (string)
        
        if (info.fMask & MIIM_TYPE) {
            // if setting name, copy name from CString to buffer and set cch
            _tcsncpy(itemname, sItemName, sizeof(itemname));
            info.dwTypeData = itemname;
            info.cch = sItemName.GetLength();
        }
    }
    
    // if after all the above, there is anything to change, change it
    if (info.fMask) {
        CMTRACE(_T("Converting '%s' to %s\n"), itemname,
            (info.fType & MFT_OWNERDRAW) ? _T("OWNERDRAW") : _T("STRING"));
        SetMenuItemInfo(*pMenu, iIndex, TRUE, &info);
    }
}

///////////////////////
// Addition: Philip Oldaker
DWORD CCoolMenuManager::SwitchContextItemData(CMenu *pMenu, int iCmd, DWORD dwItemData /*0*/, BOOL bByPosition /*TRUE*/)
{
	CMyItemData* pmd = GetMyItemData(*pMenu,iCmd,bByPosition);
	DWORD dwData=0;	
	if (pmd == NULL)
		return dwData;
	CMenuItemInfo info;
    info.fMask |= MIIM_DATA;                // change it
	if (pmd->IsMyItemData())
	{
		dwData= (DWORD)pmd;
        info.dwItemData = (DWORD)pmd->pContext;
        SetMenuItemInfo(*pMenu, iCmd, bByPosition, &info);
	}
	else if (dwItemData)
	{
		CMyItemData *pMyItemData = (CMyItemData*)dwItemData;
		if (pMyItemData->IsMyItemData())
		{
	        info.dwItemData = dwItemData;
		    SetMenuItemInfo(*pMenu, iCmd, bByPosition, &info);
		}
	}
	return dwData;
}
// This may not be the best way of doing it but it seems to work
// Just save the IContextMenu2 pointer on behalf of the windows shell
// and create new item data
void CCoolMenuManager::AddShellContextMenu(CMenu *pMenu, LPCONTEXTMENU lpm, int iIndex)
{
	CMyItemData* pmd = GetMyItemData(*pMenu,iIndex,TRUE);
	if (pmd == NULL || !pmd->IsMyItemData())
	{
		DWORD dw = (DWORD)pmd;
		pmd = new CMyItemData;
		pmd->pContext = (void*)dw;
	}
	pmd->lpcm = lpm;
	pmd->lpcm->AddRef();
    SetMyItemData(pmd,*pMenu,iIndex,TRUE);
}

///////////////////////////////

//////////////////
// This rather gnarly function is used both to convert the menu from strings to
// owner-draw and vice versa. In either case, it also appends automagic
// accelerator key names to the menu items, if m_bAutoAccel is TRUE.
//
void CCoolMenuManager::ConvertMenu(CMenu* pMenu, UINT nIndex, 
                                   BOOL bSysMenu, BOOL bShowButtons)
{
    ASSERT_VALID(pMenu);
    
    UINT i, nItem;

    // add/remove the menu to/from list of "converted" menus
    HMENU hmenu = pMenu->GetSafeHmenu();
    ASSERT(hmenu);
    POSITION p = m_menuList.Find(hmenu);
    if (!p)
    {
        if (bShowButtons)
            m_menuList.AddHead(hmenu);
    }
    else
    {
        if (!bShowButtons)
            m_menuList.RemoveAt(p);
    }
    
    nItem = pMenu->GetMenuItemCount();    

    for (i = 0; i < nItem; i++) 
    {  
        // loop over each item in menu
        ConvertMenuItem(pMenu, i, true, !bShowButtons);
    }
}

void *CCoolMenuManager::SetItemData(HMENU hMenu, void *pData, UINT uItem, BOOL bByPosition /*= FALSE*/)
{
    CMyItemData *pmd = GetMyItemData(hMenu, uItem, bByPosition);
    ASSERT(pmd->IsMyItemData());
    void *pOld = pmd->pContext;
    pmd->pContext = pData;
    SetMyItemData(pmd, hMenu, uItem, bByPosition);
    return pOld;
}

void *CCoolMenuManager::GetItemData(HMENU hMenu, UINT uItem, BOOL bByPosition /*= FALSE*/)
{
    MENUITEMINFO mii;
    mii.cbSize     = sizeof(mii);
    mii.fMask      = MIIM_DATA | MIIM_TYPE;
    mii.dwTypeData = NULL;
    mii.cch        = 0;
    if (!::GetMenuItemInfo(hMenu, uItem, bByPosition, &mii))
        return NULL;
if (mii.dwItemData == -1) _asm int 3;    
    CMyItemData *pmd = (CMyItemData *)mii.dwItemData;
    if ((mii.fType & MFT_OWNERDRAW)  &&  pmd  &&  pmd->IsMyItemData())
        return pmd->pContext;

    return pmd;
}

HICON CCoolMenuManager::SetItemIcon(HMENU hMenu, HICON hIcon, UINT uItem, BOOL bByPosition /*= FALSE*/)
{
    CMyItemData *pmd = GetMyItemData(hMenu, uItem, bByPosition);
    ASSERT(pmd->IsMyItemData());
    HICON hOldIcon = pmd->hIcon;
    pmd->hIcon = hIcon;
    if (hIcon)
        pmd->iButton = USES_ICON;
    else
        pmd->iButton = -1;
    SetMyItemData(pmd, hMenu, uItem, bByPosition);
    return hOldIcon;
}

HICON CCoolMenuManager::GetItemIcon(HMENU hMenu, UINT uItem, BOOL bByPosition /*= FALSE*/)
{
    CMyItemData *pmd = GetMyItemData(hMenu, uItem, bByPosition);
    ASSERT(pmd->IsMyItemData());
    return pmd->hIcon;
}

void CCoolMenuManager::SetSubMenuIcon(HMENU hSubMenu, int nID)
{
    m_mapHMENUtoID[(void *)hSubMenu] = nID;
}

void CCoolMenuManager::UnmanageSubMenu(HMENU hSubMenu)
{
    m_mapHMENUtoID.RemoveKey((void *)hSubMenu);
}


//////////////////
// User typed a char into menu. Look for item with & preceeding the char typed.
//
LRESULT CCoolMenuManager::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
	ASSERT_VALID(pMenu);
	CMyItemData* pmd = GetMyItemData(*pMenu,0,TRUE);
	// Addition: Philip Oldaker
	// Pass it on to the shell if this has been set
	if (pmd->IsMyItemData() && pmd->lpcm)
	{
		LRESULT lResult=0;
		LPCONTEXTMENU3 lpcm3=NULL;
		HRESULT hr = pmd->lpcm->QueryInterface(IID_IContextMenu3,(LPVOID*)&lpcm3);
		if (SUCCEEDED(hr))
		{
			// convert menu back to original item data
			UINT nCount = pMenu->GetMenuItemCount();
			DWORD *pArrItemData = new DWORD[nCount];
			for(UINT i1=0;i1 < nCount;i1++)
				pArrItemData[i1] = SwitchContextItemData(pMenu,i1);
			lpcm3->HandleMenuMsg2(WM_MENUCHAR,MAKELPARAM(nChar,nFlags),(LPARAM)pMenu->GetSafeHmenu(),&lResult);
			lpcm3->Release();
			// Convert menu back to back our item data
			for(UINT i2=0;i2 < nCount;i2++)
				SwitchContextItemData(pMenu,i2,pArrItemData[i2]);
			delete []pArrItemData;
		}
		return lResult;
	}
	////////////////////////////////

  UINT iCurrentItem = (UINT)-1; // guaranteed higher than any command ID
  CUIntArray arItemsMatched;    // items that match the character typed

  UINT nItem = pMenu->GetMenuItemCount();
  for (UINT i=0; i< nItem; i++) {
    // get menu info
    CMenuItemInfo info;
    info.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE;
    ::GetMenuItemInfo(*pMenu, i, TRUE, &info);

    CMyItemData* pmd = (CMyItemData*)info.dwItemData;
    if ((info.fType & MFT_OWNERDRAW) && pmd && pmd->IsMyItemData()) {
      CString& text = pmd->text;
      int iAmpersand = text.Find(_T('&'));
      if (iAmpersand >=0 && toupper(nChar)==toupper(text[iAmpersand+1]))
        arItemsMatched.Add(i);
    }
    if (info.fState & MFS_HILITE)
      iCurrentItem = i; // note index of current item
  }

  // arItemsMatched now contains indexes of items that match the char typed.
  //
  //   * if none: beep
  //   * if one:  execute it
  //   * if more than one: hilite next
  //
  UINT nFound = arItemsMatched.GetSize();
  if (nFound == 0)
    return 0;

  else if (nFound==1)
    return MAKELONG(arItemsMatched[0], MNC_EXECUTE);

  // more than one found--return 1st one past current selected item;
  UINT iSelect = 0;
  for (i=0; i < nFound; i++) {
    if (arItemsMatched[i] > iCurrentItem) {
      iSelect = i;
      break;
    }
  }
  return MAKELONG(arItemsMatched[iSelect], MNC_SELECT);
}

//////////////////
// Handle WM_MENUSELECT: check for menu closed
//
void CCoolMenuManager::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
	if (m_lpcm)
	{
		char szHelpText[MAX_PATH];
		ZeroMemory(szHelpText,sizeof(szHelpText));
		if ((nFlags & MF_SYSMENU) != MF_SYSMENU && (nFlags & MF_POPUP) != MF_POPUP)
		{
			if (nItemID >= m_idShellMenuFirst && nItemID <= m_idShellMenuLast)
			{
				m_lpcm->GetCommandString(nItemID-m_idShellMenuFirst,GCS_HELPTEXT,NULL,szHelpText,sizeof(szHelpText));
			}
		}
		CWnd *pWnd=m_pFrame;
		if (pWnd == NULL)
			pWnd = AfxGetMainWnd();
		if (pWnd)
			pWnd->SendMessage(WM_SETMESSAGESTRING,0,(LPARAM)szHelpText);
	}
	if (hSysMenu==NULL && nFlags==0xFFFF) 
	{
		if (m_lpcm)
		{
			TRACE0("Setting lpcm to null\n");
			m_lpcm = NULL;
		}
		/*
		//  de-comment this code for self destructing menus.s
		// Windows has closed the menu: restore all menus to original state
		while (!m_menuList.IsEmpty()) 
		{
		  ConvertMenu(CMenu::FromHandle((HMENU)m_menuList.RemoveHead()),
			0, FALSE, FALSE);
		}
		*/
	}
}

// fix for virtual key names - Seain Conover (1999/01/09) 

CString CCoolMenuManager::GetVirtualKeyName( const CString strVirtKey ) const
{
  CString strResult;
  
  if    ( strVirtKey == _T("Num 7"))    { strResult = _T("Home"); }
  else if ( strVirtKey == _T("Num 1"))    { strResult = _T("End");  }
  else if ( strVirtKey == _T("Num 9"))    { strResult = _T("PgUp"); }
  else if ( strVirtKey == _T("Num 3"))    { strResult = _T("PgDn"); }
  else if ( strVirtKey == _T("Num 0"))    { strResult = _T("Ins");  }
  else if ( strVirtKey == _T("Num Del"))    { strResult = _T("Del");  }
  else
  {
    strResult = strVirtKey;
  }
  
  return strResult;
}

//////////////////
// Append the name of accelerator for given command ID to menu string.
// sItemName is menu item name, which will have the accelerator appended.
// For example, might call with sItemName = "File &Open" and return with
// sItemName = "File &Open\tCtrl-O". Returns BOOL = whether string changed.
//
BOOL CCoolMenuManager::AppendAccelName(CString& sItemName, UINT nID)
{
  int iTabPos = sItemName.Find(_T('\t'));
  if (iTabPos > 0)
    sItemName = sItemName.Left(iTabPos);

  BOOL bFound = FALSE;
  for (ACCEL* pa = GetAccel(nID); pa; pa -= pa->cmd) {
    sItemName += bFound ? _T(", ") : _T("\t");
    if (pa->fVirt & FALT)     sItemName += _T("Alt+");
    if (pa->fVirt & FCONTROL)   sItemName += _T("Ctrl+");
    if (pa->fVirt & FSHIFT)     sItemName += _T("Shift+");
    if (pa->fVirt & FVIRTKEY) {
      TCHAR keyname[64];
      UINT vkey = MapVirtualKey(pa->key, 0)<<16;
      GetKeyNameText(vkey, keyname, sizeof(keyname));
      // Seain Conover (1999/01/09) 
      sItemName += GetVirtualKeyName( keyname );
    } else
      sItemName += (char)pa->key;

    bFound = TRUE;
    if (pa->cmd == 0)
      break;
  }
  return bFound;
}

//////////////////
// This function fixes MFC's diseased dot bitmap used for
// "radio-style" menu items (CCmdUI->SetRadio), which is completely
// wrong if the menu font is large. 
//
void CCoolMenuManager::FixMFCDotBitmap()
{
  HBITMAP hbmDot = GetMFCDotBitmap();
  if (hbmDot) {
    // Draw a centered dot of appropriate size
    BITMAP bm;
    ::GetObject(hbmDot, sizeof(bm), &bm);
    CRect rcDot(0,0, bm.bmWidth, bm.bmHeight);
    rcDot.DeflateRect((bm.bmWidth>>1)-2, (bm.bmHeight>>1)-2);

    CWindowDC dcScreen(NULL);
    CDC memdc;
    memdc.CreateCompatibleDC(&dcScreen);
    int nSave = memdc.SaveDC();
    memdc.SelectStockObject(BLACK_PEN);
    memdc.SelectStockObject(BLACK_BRUSH);
    memdc.SelectObject((HGDIOBJ)hbmDot);
    memdc.PatBlt(0, 0, bm.bmWidth, bm.bmHeight, WHITENESS);
    memdc.Ellipse(&rcDot);
    memdc.RestoreDC(nSave);
  }
}

//////////////////
// This function gets MFC's dot bitmap.
//
HBITMAP CCoolMenuManager::GetMFCDotBitmap()
{
  // The bitmap is stored in afxData.hbmMenuDot, but afxData is MFC-private,
  // so the only way to get it is create a menu, set the radio check,
  // and then see what bitmap MFC set in the menu item.
  CMenu menu;
  VERIFY(menu.CreateMenu());
  VERIFY(menu.AppendMenu(MFT_STRING, 0, (LPCTSTR)NULL));
  CCmdUI cui;
  cui.m_pMenu = &menu;
  cui.m_nIndex = 0;
  cui.m_nIndexMax = 1;
  cui.SetRadio(TRUE);
  CMenuItemInfo info;
  info.fMask = MIIM_CHECKMARKS;
  GetMenuItemInfo(menu, 0, MF_BYPOSITION, &info);
  HBITMAP hbmDot = info.hbmpChecked;
  menu.DestroyMenu();
  return hbmDot;
}

// Peter Tewkesbury
BOOL CCoolMenuManager::AddSingleBitmap(UINT nBitmapID, UINT n, UINT *nID)
{
  // load bitmap
  HBITMAP hbmBitmap = (HBITMAP)::LoadImage(AfxGetInstanceHandle(),
    MAKEINTRESOURCE(nBitmapID), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
  ASSERT(hbmBitmap);

  if (!hbmBitmap) {
    TRACE(_T("*** Can't load bitmap %d!\n"), nBitmapID);
    return FALSE;
  }

  // Assign Bitmap to CBitmap
  CBitmap bmBitmap;
  bmBitmap.Attach(hbmBitmap); // destructor will detach & destroy
  
  // OK, I have the bitmap - Check that Bitmaps are correct size.
  if (m_szBitmap.cx==0) 
  {
    // First toolbar: initialized bitmap/button sizes and create image list.
    CSize sz(16,15);
    m_szBitmap = sz;
    m_szButton = sz + CSize(CXBUTTONMARGIN<<1, CYBUTTONMARGIN<<1);
    VERIFY(m_ilButtons.Create(sz.cx, sz.cy, ILC_MASK, 0, 10));
    
  } 
  
  // Add Bitmap to ImageList
  int iNextImage = m_ilButtons.GetImageCount();
  m_ilButtons.Add(&bmBitmap, GetSysColor(COLOR_3DFACE)); 
  
  // Add ID to Map.
  for(UINT i=0;i<n;i++)
  {
    if (nID[i] > 0) 
    {
      if (GetButtonIndex(nID[i]) >= 0) 
      {
        TRACE(_T("*** Duplicate button ID %d ignored\n"), nID[i]);
      } 
      else m_mapIDtoImage.SetAt(nID[i], (void*)iNextImage++); 
    }
  }
  
  // All Done.
  return TRUE;
}

////////////////////////////////////////////////////////////////
// Helper functions

//////////////////
// Load a bitmap, converting the standard colors.
// Calls AfxLoadSysColorBitmap to do the work.
//
// RGB(0x00, 0x00, 0x00) (black)      --> COLOR_BTNTEXT
// RGB(0x80, 0x80, 0x80) (dark gray)  --> COLOR_3DSHADOW
// RGB(0xC0, 0xC0, 0xC0) (gray)       --> COLOR_3DFACE
// RGB(0xFF, 0xFF, 0xFF) (white)      --> COLOR_3DHILIGHT
// 
HBITMAP PLLoadSysColorBitmap(LPCTSTR lpResName, BOOL bMono)
{
  HINSTANCE hInst = AfxFindResourceHandle(lpResName, RT_BITMAP);
  HRSRC hRsrc = ::FindResource(hInst, lpResName, RT_BITMAP);
  if (hRsrc == NULL)
    return NULL;
  return AfxLoadSysColorBitmap(hInst, hRsrc, bMono);
}

//////////////////
// Shorthand to fill a rectangle with a solid color.
//
void PLFillRect(CDC& dc, const CRect& rc, COLORREF color)
{
  CBrush brush(color);
  CBrush* pOldBrush = dc.SelectObject(&brush);
  dc.PatBlt(rc.left, rc.top, rc.Width(), rc.Height(), PATCOPY);
  dc.SelectObject(pOldBrush);
}

// This is the magic ROP code used to generate the embossed look for
// a disabled button. It's listed in Appendix F of the Win32 Programmer's
// Reference as PSDPxax (!) which is a cryptic reverse-polish notation for
//
// ((Destination XOR Pattern) AND Source) XOR Pattern
//
// which I leave to you to figure out. In the case where I apply it,
// Source is a monochrome bitmap which I want to draw in such a way that
// the black pixels get transformed to the brush color and the white pixels
// draw transparently--i.e. leave the Destination alone.
//
// black ==> Pattern (brush)
// white ==> Destintation (ie, transparent)
//
// 0xb8074a is the ROP code that does this. For more info, see Charles
// Petzold, _Programming Windows_, 2nd Edition, p 622-624.
//
#define TRANSPARENTROP 0xb8074a

//////////////////
// Draw an image with the embossed (disabled) look.
//
//    dc      device context to draw in
//    il      image list containing image
//    i     index of image to draw
//    p     point in dc to draw image at
//    bColor  do color embossing. Default is B/W.
//
void PLDrawEmbossedOld(CDC& dc, CImageList& il, int i, CPoint p, BOOL bColor)
{
  IMAGEINFO info;
  VERIFY(il.GetImageInfo(0, &info));
  CRect rc = info.rcImage;
  int cx = rc.Width();
  int cy = rc.Height();

  // create memory dc
  CDC memdc;
  memdc.CreateCompatibleDC(&dc);

  // create mono or color bitmap
  CBitmap bm;
  if (bColor)
    bm.CreateCompatibleBitmap(&dc, cx, cy);
  else
    bm.CreateBitmap(cx, cy, 1, 1, NULL);

  // draw image into memory DC--fill BG white first
  CBitmap* pOldBitmap = memdc.SelectObject(&bm);
  memdc.PatBlt(0, 0, cx, cy, WHITENESS);
  il.Draw(&memdc, i, CPoint(0,0), ILD_TRANSPARENT);

  // This seems to be required. Why, I don't know. ???
  COLORREF colorOldBG = dc.SetBkColor(RGB(255,255,255)); // white

  // Draw using hilite offset by (1,1), then shadow
  CBrush brShadow(GetSysColor(COLOR_3DSHADOW));
  CBrush brHilite(GetSysColor(COLOR_3DHIGHLIGHT));
  CBrush* pOldBrush = dc.SelectObject(&brHilite);
  dc.BitBlt(p.x+1, p.y+1, cx, cy, &memdc, 0, 0, TRANSPARENTROP);
  dc.SelectObject(&brShadow);
  dc.BitBlt(p.x, p.y, cx, cy, &memdc, 0, 0, TRANSPARENTROP);
  dc.SelectObject(pOldBrush);
  dc.SetBkColor(colorOldBG);         // restore
  memdc.SelectObject(pOldBitmap);    // ...
}

#define NEWROP  0x00E20746L

void PLDrawEmbossed(CDC& dc, CImageList& il, int i, CPoint p, BOOL bColor)
{
    IMAGEINFO info;
    VERIFY(il.GetImageInfo(0, &info));
    CRect rc = info.rcImage;
    int cx = rc.Width();
    int cy = rc.Height();
    CBitmap patbmp, bmp;
	CDC memdc, patdc;
	patdc.CreateCompatibleDC(0);
	memdc.CreateCompatibleDC(0);
	patbmp.CreateCompatibleBitmap(&patdc, cx, cy);
	bmp.CreateCompatibleBitmap(&dc, cx, cy);
	CBitmap * pddcOldBmp = patdc.SelectObject(&patbmp);
    CBitmap * pdcOldBmp  = memdc.SelectObject(&bmp);

    il.Draw(&memdc, i, CPoint(0, 0), ILD_NORMAL); //ILD_TRANSPARENT);

	// build a mask
	patdc.PatBlt(0, 0, cx, cy, WHITENESS);
	memdc.SetBkColor(GetSysColor(COLOR_BTNFACE));
	patdc.BitBlt(0, 0, cx, cy, &memdc, 0, 0, SRCCOPY);
	memdc.SetBkColor(GetSysColor(COLOR_BTNHILIGHT));
	patdc.BitBlt(0, 0, cx, cy, &memdc, 0, 0, SRCPAINT);

	memdc.SetBkColor(RGB(0, 0, 0));
	memdc.SetTextColor(RGB(255, 255, 255));
	CBrush brShadow, brHilight;
	brHilight.CreateSolidBrush(GetSysColor(COLOR_BTNHILIGHT));
	brShadow.CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
	CBrush * pOldBrush = memdc.SelectObject(&brHilight);
	memdc.BitBlt(1, 1, cx, cy, &patdc, 0, 0, NEWROP);
	memdc.BitBlt(0, 0, cx, cy, &patdc, 0, 0, NEWROP);
	memdc.SelectObject(&brShadow);
	memdc.BitBlt(0, 0, cx, cy, &patdc, 0, 0, NEWROP);
	dc.BitBlt(p.x, p.y, cx, cy, &memdc, 0, 0, SRCCOPY);

    // reset DCs
	patdc.SelectObject(pddcOldBmp);
	patdc.DeleteDC();
	memdc.SelectObject(pOldBrush);
	memdc.SelectObject(pdcOldBmp);
	memdc.DeleteDC();

	brShadow.DeleteObject();
	brHilight.DeleteObject();
	patbmp.DeleteObject();
}
