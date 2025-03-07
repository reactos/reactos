/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

/*****************************************************************************
 ** CTaskBand ****************************************************************
 *****************************************************************************/

const GUID CLSID_ITaskBand = { 0x68284FAA, 0x6A48, 0x11D0, { 0x8C, 0x78, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0xB4 } };

class CTaskBand :
    public CComCoClass<CTaskBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IObjectWithSite,
    public IDeskBand,
    public IDeskBar,
    public IPersistStream,
    public IWinEventHandler,
    public IOleCommandTarget
{
    CComPtr<ITrayWindow> m_Tray;
    CComPtr<IUnknown> m_Site;
    CComPtr<IUnknown> m_TasksWnd;

    HWND m_hWnd;

public:
    CTaskBand() :
        m_hWnd(NULL)
    {
    }

    virtual ~CTaskBand() { }

    /*****************************************************************************/

    STDMETHODIMP
    GetWindow(OUT HWND *phwnd) override
    {
        if (!m_hWnd)
            return E_FAIL;
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;
        return S_OK;
    }

    STDMETHODIMP
    ContextSensitiveHelp(IN BOOL fEnterMode) override
    {
        /* FIXME: Implement */
        return E_NOTIMPL;
    }

    STDMETHODIMP
    ShowDW(IN BOOL bShow) override
    {
        /* We don't do anything... */
        return S_OK;
    }

    STDMETHODIMP
    CloseDW(IN DWORD dwReserved) override
    {
        /* We don't do anything... */
        return S_OK;
    }

    STDMETHODIMP
    ResizeBorderDW(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved) override
    {
        /* No need to implement this method */
        return E_NOTIMPL;
    }

    STDMETHODIMP
    GetBandInfo(
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi) override
    {
        TRACE("CTaskBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, m_hWnd);

        if (m_hWnd != NULL)
        {
            HWND hwndToolbar = ::GetWindow(m_hWnd, GW_CHILD);

            /* The task band never has a title */
            pdbi->dwMask &= ~DBIM_TITLE;

            /* NOTE: We don't return DBIMF_UNDELETEABLE here, the band site will
                     handle us differently and add this flag for us. The reason for
                     this is future changes that might allow it to be deletable.
                     We want the band site to be in charge of this decision rather
                     the band itself! */
            /* FIXME: What about DBIMF_NOGRIPPER and DBIMF_ALWAYSGRIPPER */
            pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT;

            /* Obtain the button size, to be used as the minimum size */
            DWORD size = SendMessageW(hwndToolbar, TB_GETBUTTONSIZE, 0, 0);
            pdbi->ptMinSize.x = 0;
            pdbi->ptMinSize.y = GET_Y_LPARAM(size);

            if (dwViewMode & DBIF_VIEWMODE_VERTICAL)
            {
                pdbi->ptIntegral.x = 0;
                pdbi->ptIntegral.y = 1;
            }
            else
            {
                pdbi->ptIntegral.x = 0;
                pdbi->ptIntegral.y = GET_Y_LPARAM(size);
            }

            /* Ignored: pdbi->ptMaxSize.x */
            pdbi->ptMaxSize.y = -1;

            RECT rcToolbar;
            ::GetWindowRect(hwndToolbar, &rcToolbar);
            /* FIXME: We should query the height from the task bar object!!! */
            pdbi->ptActual.x = rcToolbar.right - rcToolbar.left;
            pdbi->ptActual.y = rcToolbar.bottom - rcToolbar.top;

            TRACE("H: %d, Min: %d,%d, Integral.y: %d Actual: %d,%d\n", (dwViewMode & DBIF_VIEWMODE_VERTICAL) == 0,
                pdbi->ptMinSize.x, pdbi->ptMinSize.y, pdbi->ptIntegral.y,
                pdbi->ptActual.x, pdbi->ptActual.y);

            return S_OK;
        }

        return E_FAIL;
    }

    /*****************************************************************************/
    // *** IOleCommandTarget methods ***

    STDMETHODIMP
    QueryStatus(
        const GUID *pguidCmdGroup,
        ULONG cCmds,
        OLECMD prgCmds [],
        OLECMDTEXT *pCmdText) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    STDMETHODIMP
    Exec(
        const GUID *pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANT *pvaIn,
        VARIANT *pvaOut) override
    {
        if (IsEqualIID(*pguidCmdGroup, IID_IBandSite))
        {
            return S_OK;
        }

        if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
        {
            return S_OK;
        }

        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    /*****************************************************************************/

    STDMETHODIMP
    SetClient(IN IUnknown *punkClient) override
    {
        TRACE("IDeskBar::SetClient(0x%p)\n", punkClient);
        return E_NOTIMPL;
    }

    STDMETHODIMP
    GetClient(OUT IUnknown **ppunkClient) override
    {
        TRACE("IDeskBar::GetClient(0x%p)\n", ppunkClient);
        return E_NOTIMPL;
    }

    STDMETHODIMP
    OnPosRectChangeDB(IN RECT *prc) override
    {
        TRACE("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);
        if (prc->bottom - prc->top == 0)
            return S_OK;

        return S_FALSE;
    }

    /*****************************************************************************/

    STDMETHODIMP
    GetClassID(OUT CLSID *pClassID) override
    {
        TRACE("CTaskBand::GetClassID(0x%p)\n", pClassID);
        /* We're going to return the (internal!) CLSID of the task band interface */
        *pClassID = CLSID_ITaskBand;
        return S_OK;
    }

    STDMETHODIMP
    IsDirty() override
    {
        /* The object hasn't changed since the last save! */
        return S_FALSE;
    }

    STDMETHODIMP
    Load(IN IStream *pStm) override
    {
        TRACE("CTaskBand::Load called\n");
        /* Nothing to do */
        return S_OK;
    }

    STDMETHODIMP
    Save(
        IN IStream *pStm,
        IN BOOL fClearDirty) override
    {
        /* Nothing to do */
        return S_OK;
    }

    STDMETHODIMP
    GetSizeMax(OUT ULARGE_INTEGER *pcbSize) override
    {
        TRACE("CTaskBand::GetSizeMax called\n");
        /* We don't need any space for the task band */
        pcbSize->QuadPart = 0;
        return S_OK;
    }

    /*****************************************************************************/

    STDMETHODIMP
    SetSite(IUnknown *pUnkSite) override
    {
        HRESULT hRet;
        HWND hwndSite;

        TRACE("CTaskBand::SetSite(0x%p)\n", pUnkSite);

        hRet = IUnknown_GetWindow(pUnkSite, &hwndSite);
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        TRACE("CreateTaskSwitchWnd(Parent: 0x%p)\n", hwndSite);

        hRet = CTaskSwitchWnd_CreateInstance(hwndSite, m_Tray, IID_PPV_ARG(IUnknown, &m_TasksWnd));
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        hRet = IUnknown_GetWindow(m_TasksWnd, &m_hWnd);
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        m_Site = pUnkSite;

        return S_OK;
    }

    STDMETHODIMP
    GetSite(
        IN REFIID riid,
        OUT VOID **ppvSite) override
    {
        TRACE("CTaskBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

        if (m_Site != NULL)
        {
            return m_Site->QueryInterface(riid, ppvSite);
        }

        *ppvSite = NULL;
        return E_FAIL;
    }

    /*****************************************************************************/

    STDMETHODIMP
    OnWinEvent(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT *theResult) override
    {
        //UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    STDMETHODIMP
    IsWindowOwner(HWND hWnd) override
    {
        return (hWnd == m_hWnd) ? S_OK : S_FALSE;
    }

    /*****************************************************************************/

    HRESULT STDMETHODCALLTYPE Initialize(IN OUT ITrayWindow *tray, HWND hWndStartButton)
    {
        m_Tray = tray;
        return S_OK;
    }

    DECLARE_NOT_AGGREGATABLE(CTaskBand)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTaskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    END_COM_MAP()
};

HRESULT CTaskBand_CreateInstance(IN ITrayWindow *Tray, HWND hWndStartButton, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CTaskBand>(Tray, hWndStartButton, riid, ppv);
}
