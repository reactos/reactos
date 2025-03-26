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

#include <shdeprecated.h>

/*****************************************************************************
 ** ITrayBandSite ************************************************************
 *****************************************************************************/

// WARNING: Can't use ATL for this class due to our ATL not fully supporting the AGGREGATION functions needed for this class to be an "outer" class
// it works just fine this way.
class CTrayBandSite :
    public ITrayBandSite,
    public IBandSite,
    public IBandSiteStreamCallback
    /* TODO: IWinEventHandler */
{
    volatile LONG m_RefCount;

    CComPtr<ITrayWindow> m_Tray;

    CComPtr<IUnknown> m_Inner;
    CComPtr<IBandSite> m_BandSite;
    CComPtr<IDeskBand> m_TaskBand;
    CComPtr<IWinEventHandler> m_WindowEventHandler;
    CComPtr<IContextMenu> m_ContextMenu;

    HWND m_Rebar;

    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD Locked : 1;
        };
    };

public:
    STDMETHODIMP_(ULONG)
    AddRef() override
    {
        return InterlockedIncrement(&m_RefCount);
    }

    STDMETHODIMP_(ULONG)
    Release() override
    {
        ULONG Ret = InterlockedDecrement(&m_RefCount);

        if (Ret == 0)
            delete this;

        return Ret;
    }

    STDMETHODIMP
    QueryInterface(IN REFIID riid, OUT LPVOID *ppvObj) override
    {
        if (ppvObj == NULL)
            return E_POINTER;

        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IBandSiteHelper))
        {
            // return IBandSiteStreamCallback's IUnknown
            *ppvObj = static_cast<IBandSiteStreamCallback*>(this);
        }
        else if (IsEqualIID(riid, IID_IBandSite))
        {
            *ppvObj = static_cast<IBandSite*>(this);
        }
        else if (IsEqualIID(riid, IID_IWinEventHandler))
        {
            TRACE("ITaskBandSite: IWinEventHandler queried!\n");
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }
        else if (m_Inner != NULL)
        {
            return m_Inner->QueryInterface(riid, ppvObj);
        }
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

public:
    CTrayBandSite() :
        m_RefCount(0),
        m_Rebar(NULL)
    {
    }

    virtual ~CTrayBandSite() { }

    STDMETHODIMP
    OnLoad(
        IN OUT IStream *pStm,
        IN REFIID riid,
        OUT PVOID *pvObj) override
    {
        LARGE_INTEGER liPosZero;
        ULARGE_INTEGER liCurrent;
        CLSID clsid;
        ULONG ulRead;
        HRESULT hRet;

        /* NOTE: Callback routine called by the shell while loading the task band
                 stream. We use it to intercept the default behavior when the task
                 band is loaded from the stream.

                 NOTE: riid always points to IID_IUnknown! This is because the shell hasn't
                 read anything from the stream and therefore doesn't know what CLSID
                 it's dealing with. We'll have to find it out ourselves by reading
                 the GUID from the stream. */

        /* Read the current position of the stream, we'll have to reset it everytime
           we read a CLSID that's not the task band... */
        ZeroMemory(&liPosZero, sizeof(liPosZero));
        hRet = pStm->Seek(liPosZero, STREAM_SEEK_CUR, &liCurrent);

        if (SUCCEEDED(hRet))
        {
            /* Now let's read the CLSID from the stream and see if it's our task band */
            hRet = pStm->Read(&clsid, (ULONG)sizeof(clsid), &ulRead);

            if (SUCCEEDED(hRet) && ulRead == sizeof(clsid))
            {
                if (IsEqualGUID(clsid, CLSID_ITaskBand))
                {
                    ASSERT(m_TaskBand != NULL);
                    /* We're trying to load the task band! Let's create it... */

                    hRet = m_TaskBand->QueryInterface(
                        riid,
                        pvObj);
                    if (SUCCEEDED(hRet))
                    {
                        /* Load the stream */
                        TRACE("IBandSiteStreamCallback::OnLoad intercepted the task band CLSID!\n");
                    }

                    return hRet;
                }
            }
        }

        /* Reset the position and let the shell do all the work for us */
        hRet = pStm->Seek(
            *(LARGE_INTEGER*) &liCurrent,
            STREAM_SEEK_SET,
            NULL);
        if (SUCCEEDED(hRet))
        {
            /* Let the shell handle everything else for us :) */
            hRet = OleLoadFromStream(pStm,
                riid,
                pvObj);
        }

        if (!SUCCEEDED(hRet))
        {
            TRACE("IBandSiteStreamCallback::OnLoad(0x%p, 0x%p, 0x%p) returns 0x%x\n", pStm, riid, pvObj, hRet);
        }

        return hRet;
    }

    STDMETHODIMP
    OnSave(
        IN OUT IUnknown *pUnk,
        IN OUT IStream *pStm) override
    {
        /* NOTE: Callback routine called by the shell while saving the task band
                 stream. We use it to intercept the default behavior when the task
                 band is saved to the stream */
        /* FIXME: Implement */
        TRACE("IBandSiteStreamCallback::OnSave(0x%p, 0x%p) returns E_NOTIMPL\n", pUnk, pStm);
        return E_NOTIMPL;
    }

    STDMETHODIMP
    IsTaskBand(IN IUnknown *punk) override
    {
        return IsSameObject(m_BandSite, punk);
    }

    STDMETHODIMP
    ProcessMessage(
        IN HWND hWnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam,
        OUT LRESULT *plResult) override
    {
        HRESULT hRet;

        ASSERT(m_Rebar != NULL);

        /* Custom task band behavior */
        switch (uMsg)
        {
        case WM_NOTIFY:
        {
            const NMHDR *nmh = (const NMHDR *) lParam;

            if (nmh->hwndFrom == m_Rebar)
            {
                switch (nmh->code)
                {
                case NM_NCHITTEST:
                {
                    LPNMMOUSE nmm = (LPNMMOUSE) lParam;

                    if (nmm->dwHitInfo == RBHT_CLIENT || nmm->dwHitInfo == RBHT_NOWHERE ||
                        nmm->dwItemSpec == (DWORD_PTR) -1)
                    {
                        /* Make the rebar control appear transparent so the user
                           can drag the tray window */
                        *plResult = HTTRANSPARENT;
                    }
                    return S_OK;
                }

                case RBN_MINMAX:
                    /* Deny if an Administrator disabled this "feature" */
                    *plResult = (SHRestricted(REST_NOMOVINGBAND) != 0);
                    return S_OK;
                }
            }

            //TRACE("ITrayBandSite::ProcessMessage: WM_NOTIFY for 0x%p, From: 0x%p, Code: NM_FIRST-%u...\n", hWnd, nmh->hwndFrom, NM_FIRST - nmh->code);
            break;
        }
        }

        /* Forward to the shell's IWinEventHandler interface to get the default shell behavior! */
        if (!m_WindowEventHandler)
            return E_FAIL;

        /*TRACE("Calling IWinEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p) hWndRebar=0x%p\n", hWnd, uMsg, wParam, lParam, plResult, hWndRebar);*/
        hRet = m_WindowEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, plResult);

#if 0
        if (FAILED(hRet))
        {
            if (uMsg == WM_NOTIFY)
            {
                const NMHDR *nmh = (const NMHDR *) lParam;
                ERR("ITrayBandSite->IWinEventHandler::ProcessMessage: WM_NOTIFY for 0x%p, From: 0x%p, Code: NM_FIRST-%u returned 0x%x\n", hWnd, nmh->hwndFrom, NM_FIRST - nmh->code, hRet);
            }
            else
            {
                ERR("ITrayBandSite->IWinEventHandler::ProcessMessage(0x%p,0x%x,0x%p,0x%p,0x%p->0x%p) returned: 0x%x\n", hWnd, uMsg, wParam, lParam, plResult, *plResult, hRet);
            }
        }
#endif

        return hRet;
    }

    STDMETHODIMP
    AddContextMenus(
        IN HMENU hmenu,
        IN UINT indexMenu,
        IN UINT idCmdFirst,
        IN UINT idCmdLast,
        IN UINT uFlags,
        OUT IContextMenu **ppcm) override
    {
        HRESULT hRet;

        if (m_ContextMenu == NULL)
        {
            /* Cache the context menu so we don't need to CoCreateInstance all the time... */
            hRet = _CBandSiteMenu_CreateInstance(IID_PPV_ARG(IContextMenu, &m_ContextMenu));
            if (FAILED_UNEXPECTEDLY(hRet))
                return hRet;

            hRet = IUnknown_SetOwner(m_ContextMenu, (IBandSite*)this);
            if (FAILED_UNEXPECTEDLY(hRet))
                return hRet;
        }

        if (ppcm != NULL)
        {
            *ppcm = m_ContextMenu;
            (*ppcm)->AddRef();
        }

        /* Add the menu items */
        hRet = m_ContextMenu->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        return S_OK;
    }

    STDMETHODIMP
    Lock(IN BOOL bLock) override
    {
        BOOL bPrevLocked = Locked;
        BANDSITEINFO bsi;
        HRESULT hRet;

        ASSERT(m_BandSite != NULL);

        if (bPrevLocked != bLock)
        {
            Locked = bLock;

            bsi.dwMask = BSIM_STYLE;
            bsi.dwStyle = (Locked ? BSIS_LOCKED | BSIS_NOGRIPPER : BSIS_AUTOGRIPPER);

            hRet = m_BandSite->SetBandSiteInfo(&bsi);
            if (SUCCEEDED(hRet))
            {
                hRet = Update();
            }

            return hRet;
        }

        return S_FALSE;
    }

    /*******************************************************************/

    STDMETHODIMP
    AddBand(IN IUnknown *punk) override
    {
        /* Send the DBID_DELAYINIT command to initialize the band to be added */
        /* FIXME: Should be delayed */
        IUnknown_Exec(punk, IID_IDeskBand, DBID_DELAYINIT, 0, NULL, NULL);

        HRESULT hr = m_BandSite->AddBand(punk);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        VARIANT vThemeName;
        V_VT(&vThemeName) = VT_BSTR;
        V_BSTR(&vThemeName) = SysAllocString(L"TaskBar");
        IUnknown_Exec(punk,
                      IID_IDeskBand,
                      DBID_SETWINDOWTHEME,
                      0,
                      &vThemeName,
                      NULL);

        SysFreeString(V_BSTR(&vThemeName));

        return S_OK;
    }

    STDMETHODIMP
    EnumBands(
        IN UINT uBand,
        OUT DWORD *pdwBandID) override
    {
        return m_BandSite->EnumBands(uBand, pdwBandID);
    }

    STDMETHODIMP
    QueryBand(
        IN DWORD dwBandID,
        OUT IDeskBand **ppstb,
        OUT DWORD *pdwState,
        OUT LPWSTR pszName,
        IN int cchName) override
    {
        HRESULT hRet;
        IDeskBand *pstb = NULL;

        hRet = m_BandSite->QueryBand(
            dwBandID,
            &pstb,
            pdwState,
            pszName,
            cchName);

        if (SUCCEEDED(hRet))
        {
            hRet = IsSameObject(pstb, m_TaskBand);
            if (hRet == S_OK)
            {
                /* Add the BSSF_UNDELETEABLE flag to pdwState because the task bar band shouldn't be deletable */
                if (pdwState != NULL)
                    *pdwState |= BSSF_UNDELETEABLE;
            }
            else if (!SUCCEEDED(hRet))
            {
                pstb->Release();
                pstb = NULL;
            }

            if (ppstb != NULL)
                *ppstb = pstb;
        }
        else if (ppstb != NULL)
            *ppstb = NULL;

        return hRet;
    }

    STDMETHODIMP
    SetBandState(
        IN DWORD dwBandID,
        IN DWORD dwMask,
        IN DWORD dwState) override
    {
        return m_BandSite->SetBandState(dwBandID, dwMask, dwState);
    }

    STDMETHODIMP
    RemoveBand(IN DWORD dwBandID) override
    {
        return m_BandSite->RemoveBand(dwBandID);
    }

    STDMETHODIMP
    GetBandObject(
        IN DWORD dwBandID,
        IN REFIID riid,
        OUT VOID **ppv) override
    {
        return m_BandSite->GetBandObject(dwBandID, riid, ppv);
    }

    STDMETHODIMP
    SetBandSiteInfo(IN const BANDSITEINFO *pbsinfo) override
    {
        return m_BandSite->SetBandSiteInfo(pbsinfo);
    }

    STDMETHODIMP
    GetBandSiteInfo(IN OUT BANDSITEINFO *pbsinfo) override
    {
        return m_BandSite->GetBandSiteInfo(pbsinfo);
    }

    virtual BOOL HasTaskBand()
    {
        CComPtr<IPersist> pBand;
        CLSID BandCLSID;
        DWORD dwBandID;
        UINT uBand = 0;

        /* Enumerate all bands */
        while (SUCCEEDED(m_BandSite->EnumBands(uBand, &dwBandID)))
        {
            if (dwBandID && SUCCEEDED(m_BandSite->GetBandObject(dwBandID, IID_PPV_ARG(IPersist, &pBand))))
            {
                if (SUCCEEDED(pBand->GetClassID(&BandCLSID)))
                {
                    if (IsEqualGUID(BandCLSID, CLSID_ITaskBand))
                    {
                        return TRUE;
                    }
                }
            }
            uBand++;
        }

        return FALSE;
    }

    virtual HRESULT Update()
    {
        return IUnknown_Exec(m_Inner,
                IID_IDeskBand,
                DBID_BANDINFOCHANGED,
                0,
                NULL,
                NULL);
    }

    virtual VOID BroadcastOleCommandExec(REFGUID pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdExecOpt,
        VARIANTARG *pvaIn,
        VARIANTARG *pvaOut)
    {
        IOleCommandTarget *pOct;
        DWORD dwBandID;
        UINT uBand = 0;

        /* Enumerate all bands */
        while (SUCCEEDED(m_BandSite->EnumBands(uBand, &dwBandID)))
        {
            if (SUCCEEDED(m_BandSite->GetBandObject(dwBandID, IID_PPV_ARG(IOleCommandTarget, &pOct))))
            {
                /* Execute the command */
                pOct->Exec(
                    &pguidCmdGroup,
                    nCmdID,
                    nCmdExecOpt,
                    pvaIn,
                    pvaOut);

                pOct->Release();
            }

            uBand++;
        }
    }

    virtual HRESULT FinishInit()
    {
        /* Broadcast the DBID_FINISHINIT command */
        BroadcastOleCommandExec(IID_IDeskBand, DBID_FINISHINIT, 0, NULL, NULL);

        return S_OK;
    }

    virtual HRESULT Show(IN BOOL bShow)
    {
        CComPtr<IDeskBarClient> pDbc;
        HRESULT hRet;

        hRet = m_BandSite->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pDbc));
        if (SUCCEEDED(hRet))
        {
            hRet = pDbc->UIActivateDBC(bShow ? DBC_SHOW : DBC_HIDE);
        }

        return hRet;
    }

    virtual HRESULT LoadFromStream(IN OUT IStream *pStm)
    {
        CComPtr<IPersistStream> pPStm;
        HRESULT hRet;

        ASSERT(m_BandSite != NULL);

        /* We implement the undocumented COM interface IBandSiteStreamCallback
           that the shell will query so that we can intercept and custom-load
           the task band when it finds the task band's CLSID (which is internal).
           This way we can prevent the shell from attempting to CoCreateInstance
           the (internal) task band, resulting in a failure... */
        hRet = m_BandSite->QueryInterface(IID_PPV_ARG(IPersistStream, &pPStm));
        if (SUCCEEDED(hRet))
        {
            hRet = pPStm->Load(pStm);
            TRACE("->Load() returned 0x%x\n", hRet);
        }

        return hRet;
    }

    virtual IStream * GetUserBandsStream(IN DWORD grfMode)
    {
        HKEY hkStreams;
        IStream *Stream = NULL;

        if (RegCreateKeyW(hkExplorer,
                          L"Streams",
                          &hkStreams) == ERROR_SUCCESS)
        {
            Stream = SHOpenRegStreamW(hkStreams,
                                      L"Desktop",
                                      L"TaskbarWinXP",
                                      grfMode);

            RegCloseKey(hkStreams);
        }

        return Stream;
    }

    virtual IStream * GetDefaultBandsStream(IN DWORD grfMode)
    {
        HKEY hkStreams;
        IStream *Stream = NULL;

        if (RegCreateKeyW(HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Streams",
            &hkStreams) == ERROR_SUCCESS)
        {
            Stream = SHOpenRegStreamW(hkStreams,
                                      L"Desktop",
                                      L"Default Taskbar",
                                      grfMode);

            RegCloseKey(hkStreams);
        }

        return Stream;
    }

    virtual HRESULT Load()
    {
        IStream *pStm;
        HRESULT hRet;

        /* Try to load the user's settings */
        pStm = GetUserBandsStream(STGM_READ);
        if (pStm != NULL)
        {
            hRet = LoadFromStream(pStm);

            TRACE("Loaded user bands settings: 0x%x\n", hRet);
            pStm->Release();
        }
        else
            hRet = E_FAIL;

        /* If the user's settings couldn't be loaded, try with
           default settings (ie. when the user logs in for the
           first time! */
        if (!SUCCEEDED(hRet))
        {
            pStm = GetDefaultBandsStream(STGM_READ);
            if (pStm != NULL)
            {
                hRet = LoadFromStream(pStm);

                TRACE("Loaded default user bands settings: 0x%x\n", hRet);
                pStm->Release();
            }
            else
                hRet = E_FAIL;
        }

        return hRet;
    }

    HRESULT _Init(IN ITrayWindow *tray, IN IDeskBand* pTaskBand)
    {
        CComPtr<IDeskBarClient> pDbc;
        CComPtr<IDeskBand> pDb;
        CComPtr<IOleWindow> pOw;
        HRESULT hRet;

        m_Tray = tray;
        m_TaskBand = pTaskBand;

        /* Create the RebarBandSite */
        hRet = _CBandSite_CreateInstance(static_cast<IBandSite*>(this), IID_PPV_ARG(IUnknown, &m_Inner));
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        hRet = m_Inner->QueryInterface(IID_PPV_ARG(IBandSite, &m_BandSite));
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        hRet = m_Inner->QueryInterface(IID_PPV_ARG(IWinEventHandler, &m_WindowEventHandler));
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        hRet = m_Inner->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pDbc));
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;




        /* Crete the rebar in the tray */
        hRet = pDbc->SetDeskBarSite(tray);
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        hRet = pDbc->GetWindow(&m_Rebar);
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        SetWindowStyle(m_Rebar, RBS_BANDBORDERS, 0);

        /* Set the Desk Bar mode to the current one */
        DWORD dwMode = 0;
        /* FIXME: We need to set the mode (and update) whenever the user docks
                  the tray window to another monitor edge! */
        if (!m_Tray->IsHorizontal())
            dwMode = DBIF_VIEWMODE_VERTICAL;

        hRet = pDbc->SetModeDBC(dwMode);

        /* Load the saved state of the task band site */
        /* FIXME: We should delay loading shell extensions, also see DBID_DELAYINIT */
        Load();

        /* Add the task bar band if it hasn't been added while loading */
        if (!HasTaskBand())
        {
            hRet = m_BandSite->AddBand(m_TaskBand);
            if (FAILED_UNEXPECTEDLY(hRet))
                return hRet;
        }

        /* Should we send this after showing it? */
        Update();

        /* FIXME: When should we send this? Does anyone care anyway? */
        FinishInit();

        /* Activate the band site */
        Show(TRUE);

        return S_OK;
    }
};
/*******************************************************************/

HRESULT CTrayBandSite_CreateInstance(IN ITrayWindow *tray, IN IDeskBand* pTaskBand, OUT ITrayBandSite** pBandSite)
{
    HRESULT hr;

    CTrayBandSite * tb = new CTrayBandSite();
    if (!tb)
        return E_FAIL;

    tb->AddRef();

    hr = tb->_Init(tray, pTaskBand);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        tb->Release();
        return hr;
    }

    *pBandSite = tb;

    return S_OK;
}
