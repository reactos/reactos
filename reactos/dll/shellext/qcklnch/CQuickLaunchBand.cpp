/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CQuickLaunchBand.cpp
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcklnch);

// {260CB95D-4544-44F6-A079-575BAA60B72F}
static const GUID CLSID_QuickLaunchBand = { 0x260cb95d, 0x4544, 0x44f6, { 0xa0, 0x79, 0x57, 0x5b, 0xaa, 0x60, 0xb7, 0x2f } };

//RegComCat function
HRESULT RegisterComCat()
{
    ICatRegister *pcr;
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pcr);
    if (SUCCEEDED(hr))
    {
        CATID catid = CATID_DeskBand;
        hr = pcr->RegisterClassImplCategories(CLSID_QuickLaunchBand, 1, &catid);
        pcr->Release();
    }
    return hr;
}

//CQuickLaunchBand

    CQuickLaunchBand::CQuickLaunchBand() :
        m_hWnd(NULL),
        m_BandID(0)
    {

    }

    CQuickLaunchBand::~CQuickLaunchBand() { }

/*****************************************************************************/
//IObjectWithSite
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::SetSite(IUnknown *pUnkSite)
    {
        /*HRESULT hRet;
        HWND hwndSite;

        TRACE("CQuickLaunchBand::SetSite(0x%p)\n", pUnkSite);

        hRet = IUnknown_GetWindow(pUnkSite, &hwndSite);
        if (FAILED(hRet))
        {
            TRACE("Querying site window failed: 0x%x\n", hRet);
            return hRet;
        }

        TRACE("CreateTaskSwitchWnd(Parent: 0x%p)\n", hwndSite);

        HWND hwndTaskSwitch = CreateTaskSwitchWnd(hwndSite, m_Tray);
        if (!hwndTaskSwitch)
        {
            ERR("CreateTaskSwitchWnd failed");
            return E_FAIL;
        }

        m_Site = pUnkSite;
        m_hWnd = hwndTaskSwitch;*/

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetSite(
        IN REFIID riid,
        OUT VOID **ppvSite)
    {
        TRACE("CQuickLaunchBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

       /* if (m_Site != NULL)
        {
            return m_Site->QueryInterface(riid, ppvSite);
        }

        *ppvSite = NULL;*/
        return E_FAIL;
    }

/*****************************************************************************/
//IDeskBand
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetWindow(OUT HWND *phwnd)
    {
        /*if (!m_hWnd)
            return E_FAIL;
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;*/
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        /* FIXME: Implement */
        return E_NOTIMPL;
    }

	HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ShowDW(
        IN BOOL bShow)
    {
        /* We don't do anything... */
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::CloseDW(
        IN DWORD dwReserved)
    {
        /* We don't do anything... */
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ResizeBorderDW(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved) 
    {
        /* No need to implement this method */
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetBandInfo(
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi)
    {
        TRACE("CQuickLaunchBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, m_hWnd);
        return E_FAIL;
    }    

 /*****************************************************************************/
 //IDeskBar   
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::SetClient(
        IN IUnknown *punkClient)
    {
        TRACE("IDeskBar::SetClient(0x%p)\n", punkClient);
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetClient(
        OUT IUnknown **ppunkClient)
    {
        TRACE("IDeskBar::GetClient(0x%p)\n", ppunkClient);
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::OnPosRectChangeDB(
        IN RECT *prc)
    {
        /*TRACE("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);
        if (prc->bottom - prc->top == 0)
            return S_OK;*/

        return S_FALSE;
    }

/*****************************************************************************/
//IPersistStream
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetClassID(
        OUT CLSID *pClassID)
    {
        TRACE("CQuickLaunchBand::GetClassID(0x%p)\n", pClassID);
        /* We're going to return the (internal!) CLSID of the task band interface */
        *pClassID = CLSID_QuickLaunchBand;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::IsDirty()
    {
        /* The object hasn't changed since the last save! */
        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::Load(
        IN IStream *pStm)
    {
        TRACE("CQuickLaunchBand::Load called\n");
        /* Nothing to do */
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::Save(
        IN IStream *pStm,
        IN BOOL fClearDirty)
    {
        /* Nothing to do */
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::GetSizeMax(
        OUT ULARGE_INTEGER *pcbSize)
    {
        TRACE("CQuickLaunchBand::GetSizeMax called\n");
        /* We don't need any space for the task band */
        //pcbSize->QuadPart = 0;
        return S_OK;
    }
    

/*****************************************************************************/
//IWinEventHandler
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ProcessMessage(
        IN HWND hWnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam,
        OUT LRESULT *plrResult)
    {
        TRACE("CQuickLaunchBand: IWinEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p)\n", hWnd, uMsg, wParam, lParam, plrResult);
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::ContainsWindow(
        IN HWND hWnd)
    {
        /*if (hWnd == m_hWnd ||
            IsChild(m_hWnd, hWnd))
        {
            TRACE("CQuickLaunchBand::ContainsWindow(0x%p) returns S_OK\n", hWnd);
            return S_OK;
        }*/

        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::IsWindowOwner(HWND hWnd)
    {
        return (hWnd == m_hWnd) ? S_OK : S_FALSE;
    }
    
/*****************************************************************************/
// *** IOleCommandTarget methods ***
    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CQuickLaunchBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
    {
        /*if (IsEqualIID(*pguidCmdGroup, IID_IBandSite))
        {
            return S_OK;
        }

        if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
        {
            return S_OK;
        }*/

        UNIMPLEMENTED;
        return E_NOTIMPL;
    }  

