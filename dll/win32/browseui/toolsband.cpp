/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
Implements the toolbar band of a cabinet window
*/

#include "precomp.h"

class CToolsBand :
    public CWindowImpl<CToolsBand, CWindow, CControlWinTraits>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream
{
private:
    CComPtr<IDockingWindowSite> fDockSite;
    HIMAGELIST m_himlNormal;
    HIMAGELIST m_himlHot;
public:
    CToolsBand();
    virtual ~CToolsBand();
public:
    // *** IDeskBand methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(const RECT* prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);
    virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // message handlers
    LRESULT OnGetButtonInfo(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled);

BEGIN_MSG_MAP(CToolsBand)
//    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
//    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    NOTIFY_HANDLER(0, TBN_GETBUTTONINFOW, OnGetButtonInfo)
END_MSG_MAP()

BEGIN_COM_MAP(CToolsBand)
    COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
    COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
    COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
    COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
    COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
END_COM_MAP()
};

CToolsBand::CToolsBand()
    : fDockSite(NULL)
{
}

CToolsBand::~CToolsBand()
{
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
    RECT actualRect;
    POINTL actualSize;
    POINTL idealSize;
    POINTL maxSize;
    POINTL itemSize;

    ::GetWindowRect(m_hWnd, &actualRect);
    actualSize.x = actualRect.right - actualRect.left;
    actualSize.y = actualRect.bottom - actualRect.top;

    /* Obtain the ideal size, to be used as min and max */
    SendMessageW(m_hWnd, TB_AUTOSIZE, 0, 0);
    SendMessageW(m_hWnd, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&maxSize));

    idealSize = maxSize;
    SendMessageW(m_hWnd, TB_GETIDEALSIZE, FALSE, reinterpret_cast<LPARAM>(&idealSize));

    /* Obtain the button size, to be used as the integral size */
    DWORD size = SendMessageW(m_hWnd, TB_GETBUTTONSIZE, 0, 0);
    itemSize.x = GET_X_LPARAM(size);
    itemSize.y = GET_Y_LPARAM(size);

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize = idealSize;
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        pdbi->ptMaxSize = maxSize;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral = itemSize;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual = actualSize;
    }
    if (pdbi->dwMask & DBIM_TITLE)
        wcscpy(pdbi->wszTitle, L"");
    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->crBkgnd = 0;
    return S_OK;
}

static const int backImageIndex = 0;
static const int forwardImageIndex = 1;
static const int favoritesImageIndex = 2;
// 3
// 4
static const int cutImageIndex = 5;
static const int copyImageIndex = 6;
static const int pasteImageIndex = 7;
static const int undoImageIndex = 8;
//static const int redoImageIndex = 9;
static const int deleteImageIndex = 10;
// 11
// 12
// 13
// 14
static const int propertiesImageIndex = 15;
// 16
static const int searchImageIndex = 17;
// 18
// 19
// 20
// 21
static const int viewsImageIndex = 22;
// 23
// 24
// 25
// 26
// 27
static const int upImageIndex = 28;
static const int mapDriveImageIndex = 29;
static const int disconnectImageIndex = 30;
// 31
//static const int viewsAltImageIndex = 32;       // same image as viewsImageIndex
// 33
// 34
// 35
// 36
// 37
//static const int viewsAlt2ImageIndex = 38;      // same image as viewsAltImageIndex & viewsImageIndex
// 39
// 40
// 41
// 42
static const int foldersImageIndex = 43;
static const int moveToImageIndex = 44;
static const int copyToImageIndex = 45;
static const int folderOptionsImageIndex = 46;

enum StandardToolbarButtons {
    BtnIdx_Back = 0,
    BtnIdx_Forward,
    BtnIdx_Up,
    BtnIdx_Search,
    BtnIdx_Folders,
    BtnIdx_MoveTo,
    BtnIdx_CopyTo,
    BtnIdx_Delete,
    BtnIdx_Undo,
    BtnIdx_Views,
    BtnIdx_Stop,
    BtnIdx_Refresh,
    BtnIdx_Home,
    BtnIdx_MapDrive,
    BtnIdx_Disconnect,
    BtnIdx_Favorites,
    BtnIdx_History,
    BtnIdx_FullScreen,
    BtnIdx_Properties,
    BtnIdx_Cut,
    BtnIdx_Copy,
    BtnIdx_Paste,
    BtnIdx_FolderOptions,
};

const int numShownButtons = 13;
const int numHiddenButtons = 13;
TBBUTTON tbButtonsAdd[numShownButtons + numHiddenButtons] =
{
    { backImageIndex, IDM_GOTO_BACK, TBSTATE_ENABLED, BTNS_DROPDOWN | BTNS_SHOWTEXT, { 0 }, 0, BtnIdx_Back },
    { forwardImageIndex, IDM_GOTO_FORWARD, TBSTATE_ENABLED, BTNS_DROPDOWN, { 0 }, 0,          BtnIdx_Forward },
    { upImageIndex, IDM_GOTO_UPONELEVEL, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0,                 BtnIdx_Up },
    { 6, -1, TBSTATE_ENABLED, BTNS_SEP },
    { searchImageIndex, gSearchCommandID, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, { 0 }, 0, BtnIdx_Search },
    { foldersImageIndex, gFoldersCommandID, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, { 0 }, 0, BtnIdx_Folders },
    { 6, -1, TBSTATE_ENABLED, BTNS_SEP },
    { moveToImageIndex, gMoveToCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_MoveTo },
    { copyToImageIndex, gCopyToCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_CopyTo },
    { deleteImageIndex, gDeleteCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Delete },
    { undoImageIndex, gUndoCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Undo },
    { 6, -1, TBSTATE_ENABLED, BTNS_SEP },
    { viewsImageIndex, gViewsCommandID, TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN, { 0 }, 0, BtnIdx_Views },

    { 0, gStopCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Stop },
    { 0, IDM_VIEW_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Refresh },
    { 0, gHomeCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Home },
    { mapDriveImageIndex, IDM_TOOLS_MAPNETWORKDRIVE, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_MapDrive },
    { disconnectImageIndex, IDM_TOOLS_DISCONNECTNETWORKDRIVE, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Disconnect },
    { favoritesImageIndex, gFavoritesCommandID, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, { 0 }, 0, BtnIdx_Favorites },
    { 0, gHistoryCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_History },
    { 0, gFullScreenCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_FullScreen },
    { propertiesImageIndex, gPropertiesCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Properties },
    { cutImageIndex, gCutCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Cut },
    { copyImageIndex, gCopyCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Copy },
    { pasteImageIndex, gPasteCommandID, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_Paste },
    { folderOptionsImageIndex, IDM_TOOLS_FOLDEROPTIONS, TBSTATE_ENABLED, BTNS_BUTTON, { 0 }, 0, BtnIdx_FolderOptions },
};

HRESULT STDMETHODCALLTYPE CToolsBand::SetSite(IUnknown* pUnkSite){
    HWND                    parentWindow;
    HWND                    toolbar;
    HRESULT                 hResult;

    if(fDockSite) fDockSite.Release();

    if (pUnkSite == NULL)
        return S_OK;
    hResult = pUnkSite->QueryInterface(IID_PPV_ARG(IDockingWindowSite, &fDockSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    parentWindow = NULL;
    hResult = IUnknown_GetWindow(pUnkSite, &parentWindow);
    if (FAILED(hResult) || !::IsWindow(parentWindow))
        return E_FAIL;

    toolbar = CreateWindowEx(
                    TBSTYLE_EX_DOUBLEBUFFER,
                    TOOLBARCLASSNAMEW, NULL,
                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                    TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_REGISTERDROP | TBSTYLE_LIST | TBSTYLE_FLAT |
                    CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP,
                    0, 0, 500, 20, parentWindow, NULL, _AtlBaseModule.GetModuleInstance(), 0);
    if (toolbar == NULL)
        return E_FAIL;
    SubclassWindow(toolbar);
    SendMessage(TB_ADDSTRINGW, (WPARAM) GetModuleHandle(L"browseui.dll"), IDS_STANDARD_TOOLBAR);

    SendMessage(WM_USER + 100, GetSystemMetrics(SM_CXEDGE) / 2, 0);
    SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(TB_SETMAXTEXTROWS, 1, 0);
    SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);

    m_himlNormal = ImageList_LoadImageW(_AtlBaseModule.GetResourceInstance(),
                                        MAKEINTRESOURCEW(IDB_SHELL_EXPLORER_LG),
                                        0, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);

    m_himlHot = ImageList_LoadImageW(_AtlBaseModule.GetResourceInstance(),
                                     MAKEINTRESOURCEW(IDB_SHELL_EXPLORER_LG_HOT),
                                     0, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);

    SendMessage(TB_SETIMAGELIST, 0, (LPARAM) m_himlNormal);
    SendMessage(TB_SETHOTIMAGELIST, 0, (LPARAM) m_himlHot);
    SendMessage(TB_ADDBUTTONSW, numShownButtons, (LPARAM)&tbButtonsAdd);

    return hResult;
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetSite(REFIID riid, void **ppvSite)
{
    if (fDockSite == NULL)
        return E_FAIL;
    return fDockSite->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CToolsBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (IsWindow())
        DestroyWindow();

    m_hWnd = NULL;

    if (fDockSite)
        fDockSite.Release();

    if (m_himlNormal)
        ImageList_Destroy(m_himlNormal);

    if (m_himlHot)
        ImageList_Destroy(m_himlHot);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CToolsBand::ResizeBorderDW(const RECT* prcBorder, IUnknown* punkToolbarSite, BOOL fReserved)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::ShowDW(BOOL fShow)
{
    if (m_hWnd)
    {
        if (fShow)
            ShowWindow(SW_SHOW);
        else
            ShowWindow(SW_HIDE);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CToolsBand::HasFocusIO()
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CToolsBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CToolsBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::Load(IStream *pStm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::Save(IStream *pStm, BOOL fClearDirty)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CToolsBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}

LRESULT CToolsBand::OnGetButtonInfo(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
    TBNOTIFYW *pTBntf = reinterpret_cast<TBNOTIFYW *>(pNMHDR);

    if (pTBntf->iItem >= 0 && pTBntf->iItem < (numShownButtons + numHiddenButtons))
    {
        pTBntf->tbButton = tbButtonsAdd[pTBntf->iItem];
        return TRUE;
    }
    else
        return FALSE;
    return 0;
}

HRESULT CToolsBand_CreateInstance(REFIID riid, void **ppv)
{
    return ShellObjectCreator<CToolsBand>(riid, ppv);
}

