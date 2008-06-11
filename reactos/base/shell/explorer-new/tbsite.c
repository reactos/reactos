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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

/*****************************************************************************
 ** ITrayBandSite ************************************************************
 *****************************************************************************/

static const ITrayBandSiteVtbl ITrayBandSiteImpl_Vtbl;
static const IBandSiteVtbl IBandSiteImpl_Vtbl;

typedef struct
{
    const ITrayBandSiteVtbl *lpVtbl;
    const IBandSiteVtbl *lpBandSiteVtbl;
    LONG Ref;

    ITrayWindow *Tray;

    IUnknown *punkInner;
    IBandSite *BandSite;
    ITaskBand *TaskBand;
    IWindowEventHandler *WindowEventHandler;
    IContextMenu *ContextMenu;

    HWND hWndRebar;

    union
    {
        DWORD dwFlags;
        struct
        {
            DWORD Locked : 1;
        };
    };
} ITrayBandSiteImpl;

static HRESULT
ITrayBandSiteImpl_Update(IN OUT ITrayBandSiteImpl *This);

static IUnknown *
IUnknown_from_ITrayBandSiteImpl(ITrayBandSiteImpl *This)
{
    return (IUnknown *)&This->lpVtbl;
}

IMPL_CASTS(ITrayBandSite, ITrayBandSite, lpVtbl)
IMPL_CASTS(IBandSite, ITrayBandSite, lpBandSiteVtbl)

static ULONG STDMETHODCALLTYPE
ITrayBandSiteImpl_AddRef(IN OUT ITrayBandSite *iface)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_ITrayBandSite(iface);

    return InterlockedIncrement(&This->Ref);
}

static VOID
ITrayBandSiteImpl_Free(IN OUT ITrayBandSiteImpl *This)
{
    if (This->BandSite != NULL)
    {
        IBandSite_Release(This->BandSite);
        This->BandSite = NULL;
    }

    if (This->WindowEventHandler != NULL)
    {
        IWindowEventHandler_Release(This->WindowEventHandler);
        This->WindowEventHandler = NULL;
    }

    if (This->ContextMenu != NULL)
    {
        IContextMenu_Release(This->ContextMenu);
        This->ContextMenu = NULL;
    }

    if (This->punkInner != NULL)
    {
        IUnknown_Release(This->punkInner);
        This->punkInner = NULL;
    }

    HeapFree(hProcessHeap,
             0,
             This);
}

static ULONG STDMETHODCALLTYPE
ITrayBandSiteImpl_Release(IN OUT ITrayBandSite *iface)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_ITrayBandSite(iface);
    ULONG Ret;

    Ret = InterlockedDecrement(&This->Ref);

    if (Ret == 0)
        ITrayBandSiteImpl_Free(This);

    return Ret;
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_QueryInterface(IN OUT ITrayBandSite *iface,
                                 IN REFIID riid,
                                 OUT LPVOID *ppvObj)
{
    ITrayBandSiteImpl *This;

    if (ppvObj == NULL)
        return E_POINTER;

    This = ITrayBandSiteImpl_from_ITrayBandSite(iface);

    if (IsEqualIID(riid,
                   &IID_IUnknown) ||
        IsEqualIID(riid,
                   &IID_IBandSiteStreamCallback))
    {
        /* NOTE: IID_IBandSiteStreamCallback is queried by the shell, we
                 implement this interface directly */
        *ppvObj = IUnknown_from_ITrayBandSiteImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IBandSite))
    {
        *ppvObj = IBandSite_from_ITrayBandSiteImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IWindowEventHandler))
    {
        DbgPrint("ITaskBandSite: IWindowEventHandler queried!\n");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    else if (This->punkInner != NULL)
    {
        return IUnknown_QueryInterface(This->punkInner,
                                       riid,
                                       ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ITrayBandSiteImpl_AddRef(iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_OnLoad(IN OUT ITrayBandSite *iface,
                         IN OUT IStream *pStm,
                         IN REFIID riid,
                         OUT PVOID *pvObj)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_ITrayBandSite(iface);
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
    ZeroMemory(&liPosZero,
               sizeof(liPosZero));
    hRet = IStream_Seek(pStm,
                        liPosZero,
                        STREAM_SEEK_CUR,
                        &liCurrent);

    if (SUCCEEDED(hRet))
    {
        /* Now let's read the CLSID from the stream and see if it's our task band */
#if defined(IStream_Read)
        hRet = IStream_Read(pStm,
                            &clsid,
                            sizeof(clsid),
                            &ulRead);
#else
        ulRead = sizeof(clsid);
        hRet = IStream_Read(pStm,
                            &clsid,
                            sizeof(clsid));
#endif
        if (SUCCEEDED(hRet) && ulRead == sizeof(clsid))
        {
            if (IsEqualGUID(&clsid,
                            &CLSID_ITaskBand))
            {
                ASSERT(This->TaskBand != NULL);
                /* We're trying to load the task band! Let's create it... */

                hRet = ITaskBand_QueryInterface(This->TaskBand,
                                                riid,
                                                pvObj);
                if (SUCCEEDED(hRet))
                {
                    /* Load the stream */
                    DbgPrint("IBandSiteStreamCallback::OnLoad intercepted the task band CLSID!\n");
                }

                return hRet;
            }
        }
    }

    /* Reset the position and let the shell do all the work for us */
    hRet = IStream_Seek(pStm,
                        *(LARGE_INTEGER*)&liCurrent,
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
        DbgPrint("IBandSiteStreamCallback::OnLoad(0x%p, 0x%p, 0x%p) returns 0x%x\n", pStm, riid, pvObj, hRet);
    }

    return hRet;
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_OnSave(IN OUT ITrayBandSite *iface,
                         IN OUT IUnknown *pUnk,
                         IN OUT IStream *pStm)
{
    /* NOTE: Callback routine called by the shell while saving the task band
             stream. We use it to intercept the default behavior when the task
             band is saved to the stream */
    /* FIXME: Implement */
    DbgPrint("IBandSiteStreamCallback::OnSave(0x%p, 0x%p) returns E_NOTIMPL\n", pUnk, pStm);
    return E_NOTIMPL;
}

static HRESULT
IsSameObject(IN IUnknown *punk1,
             IN IUnknown *punk2)
{
    HRESULT hRet;

    hRet = IUnknown_QueryInterface(punk1,
                                   &IID_IUnknown,
                                   (PVOID*)&punk1);
    if (!SUCCEEDED(hRet))
        return hRet;

    hRet = IUnknown_QueryInterface(punk2,
                                   &IID_IUnknown,
                                   (PVOID*)&punk2);
    IUnknown_Release(punk1);

    if (!SUCCEEDED(hRet))
        return hRet;

    IUnknown_Release(punk2);

    /* We're dealing with the same object if the IUnknown pointers are equal */
    return (punk1 == punk2) ? S_OK : S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_IsTaskBand(IN OUT ITrayBandSite *iface,
                             IN IUnknown *punk)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_ITrayBandSite(iface);
    return IsSameObject((IUnknown *)This->BandSite,
                        punk);
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_ProcessMessage(IN OUT ITrayBandSite *iface,
                                 IN HWND hWnd,
                                 IN UINT uMsg,
                                 IN WPARAM wParam,
                                 IN LPARAM lParam,
                                 OUT LRESULT *plResult)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_ITrayBandSite(iface);
    HRESULT hRet;

    ASSERT(This->hWndRebar != NULL);

    /* Custom task band behavior */
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            const NMHDR *nmh = (const NMHDR *)lParam;

            if (nmh->hwndFrom == This->hWndRebar)
            {
                switch (nmh->code)
                {
                    case NM_NCHITTEST:
                    {
                        LPNMMOUSE nmm = (LPNMMOUSE)lParam;

                        if (nmm->dwHitInfo == RBHT_CLIENT || nmm->dwHitInfo == RBHT_NOWHERE ||
                            nmm->dwItemSpec == (DWORD_PTR)-1)
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

            //DbgPrint("ITrayBandSite::ProcessMessage: WM_NOTIFY for 0x%p, From: 0x%p, Code: NM_FIRST-%u...\n", hWnd, nmh->hwndFrom, NM_FIRST - nmh->code);
            break;
        }
    };

    /* Forward to the shell's IWindowEventHandler interface to get the default
       shell behavior! */
    if (This->WindowEventHandler != NULL)
    {
        /*DbgPrint("Calling IWindowEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p) This->hWndRebar=0x%p\n", hWnd, uMsg, wParam, lParam, plResult, This->hWndRebar);*/
        hRet = IWindowEventHandler_ProcessMessage(This->WindowEventHandler,
                                                  hWnd,
                                                  uMsg,
                                                  wParam,
                                                  lParam,
                                                  plResult);
        if (!SUCCEEDED(hRet))
        {
            if (uMsg == WM_NOTIFY)
            {
                const NMHDR *nmh = (const NMHDR *)lParam;
                DbgPrint("ITrayBandSite->IWindowEventHandler::ProcessMessage: WM_NOTIFY for 0x%p, From: 0x%p, Code: NM_FIRST-%u returned 0x%x\n", hWnd, nmh->hwndFrom, NM_FIRST - nmh->code, hRet);
            }
            else
            {
                DbgPrint("ITrayBandSite->IWindowEventHandler::ProcessMessage(0x%p,0x%x,0x%p,0x%p,0x%p->0x%p) returned: 0x%x\n", hWnd, uMsg, wParam, lParam, plResult, *plResult, hRet);
            }
        }
    }
    else
        hRet = E_FAIL;

    return hRet;
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_AddContextMenus(IN OUT ITrayBandSite *iface,
                                  IN HMENU hmenu,
                                  IN UINT indexMenu,
                                  IN UINT idCmdFirst,
                                  IN UINT idCmdLast,
                                  IN UINT uFlags,
                                  OUT IContextMenu **ppcm)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_ITrayBandSite(iface);
    IShellService *pSs;
    HRESULT hRet;

    if (This->ContextMenu == NULL)
    {
        /* Cache the context menu so we don't need to CoCreateInstance all the time... */
        hRet = CoCreateInstance(&CLSID_IShellBandSiteMenu,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                &IID_IShellService,
                                (PVOID*)&pSs);
        DbgPrint("CoCreateInstance(CLSID_IShellBandSiteMenu) for IShellService returned: 0x%x\n", hRet);
        if (!SUCCEEDED(hRet))
            return hRet;

        hRet = IShellService_SetOwner(pSs,
                                      IUnknown_from_ITrayBandSiteImpl(This));
        if (!SUCCEEDED(hRet))
        {
            IShellService_Release(pSs);
            return hRet;
        }

        hRet = IShellService_QueryInterface(pSs,
                                            &IID_IContextMenu,
                                            (PVOID*)&This->ContextMenu);

        IShellService_Release(pSs);

        if (!SUCCEEDED(hRet))
            return hRet;
    }

    if (ppcm != NULL)
    {
        IContextMenu_AddRef(This->ContextMenu);
        *ppcm = This->ContextMenu;
    }

    /* Add the menu items */
    return IContextMenu_QueryContextMenu(This->ContextMenu,
                                         hmenu,
                                         indexMenu,
                                         idCmdFirst,
                                         idCmdLast,
                                         uFlags);
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_Lock(IN OUT ITrayBandSite *iface,
                       IN BOOL bLock)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_ITrayBandSite(iface);
    BOOL bPrevLocked = This->Locked;
    BANDSITEINFO bsi;
    HRESULT hRet;

    ASSERT(This->BandSite != NULL);

    if (bPrevLocked != bLock)
    {
        This->Locked = bLock;

        bsi.dwMask = BSIM_STYLE;
        bsi.dwStyle = (This->Locked ? BSIS_LOCKED | BSIS_NOGRIPPER : BSIS_AUTOGRIPPER);

        hRet = IBandSite_SetBandSiteInfo(This->BandSite,
                                         &bsi);
        if (SUCCEEDED(hRet))
        {
            hRet = ITrayBandSiteImpl_Update(This);
        }

        return hRet;
    }

    return S_FALSE;
}

static const ITrayBandSiteVtbl ITrayBandSiteImpl_Vtbl =
{
    /*** IUnknown methods ***/
    ITrayBandSiteImpl_QueryInterface,
    ITrayBandSiteImpl_AddRef,
    ITrayBandSiteImpl_Release,
    /*** IBandSiteStreamCallback methods ***/
    ITrayBandSiteImpl_OnLoad,
    ITrayBandSiteImpl_OnSave,
    /*** ITrayBandSite methods ***/
    ITrayBandSiteImpl_IsTaskBand,
    ITrayBandSiteImpl_ProcessMessage,
    ITrayBandSiteImpl_AddContextMenus,
    ITrayBandSiteImpl_Lock
};

/*******************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IBandSite, ITrayBandSite)
METHOD_IUNKNOWN_INHERITED_RELEASE(IBandSite, ITrayBandSite)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IBandSite, ITrayBandSite)

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_AddBand(IN OUT IBandSite *iface,
                          IN IUnknown *punk)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_IBandSite(iface);
    IOleCommandTarget *pOct;
    HRESULT hRet;

    hRet = IUnknown_QueryInterface(punk,
                                   &IID_IOleCommandTarget,
                                   (PVOID*)&pOct);
    if (SUCCEEDED(hRet))
    {
        /* Send the DBID_DELAYINIT command to initialize the band to be added */
        /* FIXME: Should be delayed */
        IOleCommandTarget_Exec(pOct,
                               &IID_IDeskBand,
                               DBID_DELAYINIT,
                               0,
                               NULL,
                               NULL);

        IOleCommandTarget_Release(pOct);
    }

    return IBandSite_AddBand(This->BandSite,
                             punk);
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_EnumBands(IN OUT IBandSite *iface,
                            IN UINT uBand,
                            OUT DWORD *pdwBandID)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_IBandSite(iface);
    return IBandSite_EnumBands(This->BandSite,
                               uBand,
                               pdwBandID);
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_QueryBand(IN OUT IBandSite *iface,
                            IN DWORD dwBandID,
                            OUT IDeskBand **ppstb,
                            OUT DWORD *pdwState,
                            OUT LPWSTR pszName,
                            IN int cchName)
{
    HRESULT hRet;
    IDeskBand *pstb = NULL;
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_IBandSite(iface);

    hRet = IBandSite_QueryBand(This->BandSite,
                               dwBandID,
                               &pstb,
                               pdwState,
                               pszName,
                               cchName);

    if (SUCCEEDED(hRet))
    {
        hRet = IsSameObject((IUnknown *)pstb,
                            (IUnknown *)This->TaskBand);
        if (hRet == S_OK)
        {
            /* Add the BSSF_UNDELETEABLE flag to pdwState because the task bar band shouldn't be deletable */
            if (pdwState != NULL)
                *pdwState |= BSSF_UNDELETEABLE;
        }
        else if (!SUCCEEDED(hRet))
        {
            IDeskBand_Release(pstb);
            pstb = NULL;
        }

        if (ppstb != NULL)
            *ppstb = pstb;
    }
    else if (ppstb != NULL)
        *ppstb = NULL;

    return hRet;
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_SetBandState(IN OUT IBandSite *iface,
                               IN DWORD dwBandID,
                               IN DWORD dwMask,
                               IN DWORD dwState)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_IBandSite(iface);
    return IBandSite_SetBandState(This->BandSite,
                                  dwBandID,
                                  dwMask,
                                  dwState);
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_RemoveBand(IN OUT IBandSite *iface,
                             IN DWORD dwBandID)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_IBandSite(iface);
    return IBandSite_RemoveBand(This->BandSite,
                                dwBandID);
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_GetBandObject(IN OUT IBandSite *iface,
                                IN DWORD dwBandID,
                                IN REFIID riid,
                                OUT VOID **ppv)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_IBandSite(iface);
    return IBandSite_GetBandObject(This->BandSite,
                                   dwBandID,
                                   riid,
                                   ppv);
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_SetBandSiteInfo(IN OUT IBandSite *iface,
                                  IN const BANDSITEINFO *pbsinfo)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_IBandSite(iface);
    return IBandSite_SetBandSiteInfo(This->BandSite,
                                     pbsinfo);
}

static HRESULT STDMETHODCALLTYPE
ITrayBandSiteImpl_GetBandSiteInfo(IN OUT IBandSite *iface,
                                  IN OUT BANDSITEINFO *pbsinfo)
{
    ITrayBandSiteImpl *This = ITrayBandSiteImpl_from_IBandSite(iface);
    return IBandSite_GetBandSiteInfo(This->BandSite,
                                     pbsinfo);
}

static const IBandSiteVtbl IBandSiteImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IBandSite, ITrayBandSite),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IBandSite, ITrayBandSite),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IBandSite, ITrayBandSite),
    /*** IBandSite methods ***/
    ITrayBandSiteImpl_AddBand,
    ITrayBandSiteImpl_EnumBands,
    ITrayBandSiteImpl_QueryBand,
    ITrayBandSiteImpl_SetBandState,
    ITrayBandSiteImpl_RemoveBand,
    ITrayBandSiteImpl_GetBandObject,
    ITrayBandSiteImpl_SetBandSiteInfo,
    ITrayBandSiteImpl_GetBandSiteInfo,
};

static BOOL
ITrayBandSiteImpl_HasTaskBand(IN OUT ITrayBandSiteImpl *This)
{
    ASSERT(This->TaskBand != NULL);

    return SUCCEEDED(ITaskBand_GetRebarBandID(This->TaskBand,
                                              NULL));
}

static HRESULT
ITrayBandSiteImpl_AddTaskBand(IN OUT ITrayBandSiteImpl *This)
{
#if 0
    /* FIXME: This is the code for the simple taskbar */
    IObjectWithSite *pOws;
    HRESULT hRet;

    hRet = ITaskBand_QueryInterface(This->TaskBand,
                                    &IID_IObjectWithSite,
                                    (PVOID*)&pOws);
    if (SUCCEEDED(hRet))
    {
        hRet = IObjectWithSite_SetSite(pOws,
                                       (IUnknown *)This->TaskBand);

        IObjectWithSite_Release(pOws);
    }

    return hRet;
#else
    if (!ITrayBandSiteImpl_HasTaskBand(This))
    {
        return IBandSite_AddBand(This->BandSite,
                                 (IUnknown *)This->TaskBand);
    }

    return S_OK;
#endif
}

static HRESULT
ITrayBandSiteImpl_Update(IN OUT ITrayBandSiteImpl *This)
{
    IOleCommandTarget *pOct;
    HRESULT hRet;

    hRet = IUnknown_QueryInterface(This->punkInner,
                                   &IID_IOleCommandTarget,
                                   (PVOID*)&pOct);
    if (SUCCEEDED(hRet))
    {
        /* Send the DBID_BANDINFOCHANGED command to update the band site */
        hRet = IOleCommandTarget_Exec(pOct,
                                      &IID_IDeskBand,
                                      DBID_BANDINFOCHANGED,
                                      0,
                                      NULL,
                                      NULL);

        IOleCommandTarget_Release(pOct);
    }

    return hRet;
}

static VOID
ITrayBandSiteImpl_BroadcastOleCommandExec(IN OUT ITrayBandSiteImpl *This,
                                          const GUID *pguidCmdGroup,
                                          DWORD nCmdID,
                                          DWORD nCmdExecOpt,
                                          VARIANTARG *pvaIn,
                                          VARIANTARG *pvaOut)
{
    IOleCommandTarget *pOct;
    DWORD dwBandID;
    UINT uBand = 0;

    /* Enumerate all bands */
    while (SUCCEEDED(IBandSite_EnumBands(This->BandSite,
                                         uBand,
                                         &dwBandID)))
    {
        if (SUCCEEDED(IBandSite_GetBandObject(This->BandSite,
                                              dwBandID,
                                              &IID_IOleCommandTarget,
                                              (PVOID*)&pOct)))
        {
            /* Execute the command */
            IOleCommandTarget_Exec(pOct,
                                   pguidCmdGroup,
                                   nCmdID,
                                   nCmdExecOpt,
                                   pvaIn,
                                   pvaOut);

            IOleCommandTarget_Release(pOct);
        }

        uBand++;
    }
}

static HRESULT
ITrayBandSiteImpl_FinishInit(IN OUT ITrayBandSiteImpl *This)
{
    /* Broadcast the DBID_FINISHINIT command */
    ITrayBandSiteImpl_BroadcastOleCommandExec(This,
                                              &IID_IDeskBand,
                                              DBID_FINISHINIT,
                                              0,
                                              NULL,
                                              NULL);

    return S_OK;
}

static HRESULT
ITrayBandSiteImpl_Show(IN OUT ITrayBandSiteImpl *This,
                       IN BOOL bShow)
{
    IDeskBarClient *pDbc;
    HRESULT hRet;

    hRet = IBandSite_QueryInterface(This->BandSite,
                                    &IID_IDeskBarClient,
                                    (PVOID*)&pDbc);
    if (SUCCEEDED(hRet))
    {
        hRet = IDeskBarClient_UIActivateDBC(pDbc,
                                            bShow ? DBC_SHOW : DBC_HIDE);
        IDeskBarClient_Release(pDbc);
    }

    return hRet;
}

static HRESULT
ITrayBandSiteImpl_LoadFromStream(IN OUT ITrayBandSiteImpl *This,
                                 IN OUT IStream *pStm)
{
    IPersistStream *pPStm;
    HRESULT hRet;

    ASSERT(This->BandSite != NULL);

    /* We implement the undocumented COM interface IBandSiteStreamCallback
       that the shell will query so that we can intercept and custom-load
       the task band when it finds the task band's CLSID (which is internal).
       This way we can prevent the shell from attempting to CoCreateInstance
       the (internal) task band, resulting in a failure... */
    hRet = IBandSite_QueryInterface(This->BandSite,
                                    &IID_IPersistStream,
                                    (PVOID*)&pPStm);
    if (SUCCEEDED(hRet))
    {
        hRet = IPersistStream_Load(pPStm,
                                   pStm);
        DbgPrint("IPersistStream_Load() returned 0x%x\n", hRet);
        IPersistStream_Release(pPStm);
    }

    return hRet;
}

static IStream *
GetUserBandsStream(IN DWORD grfMode)
{
    HKEY hkStreams;
    IStream *Stream = NULL;

    if (RegCreateKey(hkExplorer,
                     TEXT("Streams"),
                     &hkStreams) == ERROR_SUCCESS)
    {
        Stream = SHOpenRegStream(hkStreams,
                                 TEXT("Desktop"),
                                 TEXT("TaskbarWinXP"),
                                 grfMode);

        RegCloseKey(hkStreams);
    }

    return Stream;
}

static IStream *
GetDefaultBandsStream(IN DWORD grfMode)
{
    HKEY hkStreams;
    IStream *Stream = NULL;

    if (RegCreateKey(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Streams"),
                     &hkStreams) == ERROR_SUCCESS)
    {
        Stream = SHOpenRegStream(hkStreams,
                                 TEXT("Desktop"),
                                 TEXT("Default Taskbar"),
                                 grfMode);

        RegCloseKey(hkStreams);
    }

    return Stream;
}

static HRESULT
ITrayBandSiteImpl_Load(IN OUT ITrayBandSiteImpl *This)
{
    IStream *pStm;
    HRESULT hRet;

    /* Try to load the user's settings */
    pStm = GetUserBandsStream(STGM_READ);
    if (pStm != NULL)
    {
        hRet = ITrayBandSiteImpl_LoadFromStream(This,
                                                pStm);

        DbgPrint("Loaded user bands settings: 0x%x\n", hRet);
        IStream_Release(pStm);
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
            hRet = ITrayBandSiteImpl_LoadFromStream(This,
                                                    pStm);

            DbgPrint("Loaded default user bands settings: 0x%x\n", hRet);
            IStream_Release(pStm);
        }
        else
            hRet = E_FAIL;
    }

    return hRet;
}

static ITrayBandSiteImpl *
ITrayBandSiteImpl_Construct(IN OUT ITrayWindow *Tray,
                            OUT HWND *phWndRebar,
                            OUT HWND *phwndTaskSwitch)
{
    ITrayBandSiteImpl *This;
    IDeskBarClient *pDbc;
    IDeskBand *pDb;
    IOleWindow *pOw;
    HRESULT hRet;

    *phWndRebar = NULL;
    *phwndTaskSwitch = NULL;

    This = HeapAlloc(hProcessHeap,
                     0,
                     sizeof(*This));
    if (This == NULL)
        return NULL;

    ZeroMemory(This,
               sizeof(*This));
    This->lpVtbl = &ITrayBandSiteImpl_Vtbl;
    This->lpBandSiteVtbl = &IBandSiteImpl_Vtbl;
    This->Ref = 1;
    This->Tray = Tray;

    /* Create a RebarBandSite provided by the shell */
    hRet = CoCreateInstance(&CLSID_RebarBandSite,
                            (LPUNKNOWN)IBandSite_from_ITrayBandSiteImpl(This),
                            CLSCTX_INPROC_SERVER,
                            &IID_IUnknown,
                            (LPVOID*)&This->punkInner);
    if (!SUCCEEDED(hRet))
    {
        ITrayBandSiteImpl_Free(This);
        return NULL;
    }

    hRet = IUnknown_QueryInterface(This->punkInner,
                                   &IID_IBandSite,
                                   (PVOID*)&This->BandSite);
    if (!SUCCEEDED(hRet))
    {
        ITrayBandSiteImpl_Free(This);
        return NULL;
    }

    hRet = IUnknown_QueryInterface(This->punkInner,
                                   &IID_IWindowEventHandler,
                                   (PVOID*)&This->WindowEventHandler);
    if (!SUCCEEDED(hRet))
    {
        ITrayBandSiteImpl_Free(This);
        return NULL;
    }

    This->TaskBand = CreateTaskBand(Tray);
    if (This->TaskBand != NULL)
    {
        /* Add the task band to the site */
        hRet = IBandSite_QueryInterface(This->BandSite,
                                        &IID_IDeskBarClient,
                                        (PVOID*)&pDbc);
        if (SUCCEEDED(hRet))
        {
            hRet = ITaskBand_QueryInterface(This->TaskBand,
                                            &IID_IOleWindow,
                                            (PVOID*)&pOw);
            if (SUCCEEDED(hRet))
            {
                /* We cause IDeskBarClient to create the rebar control by passing the new
                   task band to it. The band reports the tray window handle as window handle
                   so that IDeskBarClient knows the parent window of the Rebar control that
                   it wants to create. */
                hRet = IDeskBarClient_SetDeskBarSite(pDbc,
                                                     (IUnknown *)pOw);

                if (SUCCEEDED(hRet))
                {
                    /* The Rebar control is now created, we can query the window handle */
                    hRet = IDeskBarClient_GetWindow(pDbc,
                                                    &This->hWndRebar);

                    if (SUCCEEDED(hRet))
                    {
                        /* We need to manually remove the RBS_BANDBORDERS style! */
                        SetWindowStyle(This->hWndRebar,
                                       RBS_BANDBORDERS,
                                       0);
                    }
                }

                IOleWindow_Release(pOw);
            }

            if (SUCCEEDED(hRet))
            {
                DWORD dwMode = 0;

                /* Set the Desk Bar mode to the current one */

                /* FIXME: We need to set the mode (and update) whenever the user docks
                          the tray window to another monitor edge! */

                if (!ITrayWindow_IsHorizontal(This->Tray))
                    dwMode = DBIF_VIEWMODE_VERTICAL;

                hRet = IDeskBarClient_SetModeDBC(pDbc,
                                                 dwMode);
            }

            IDeskBarClient_Release(pDbc);
        }

        /* Load the saved state of the task band site */
        /* FIXME: We should delay loading shell extensions, also see DBID_DELAYINIT */
        ITrayBandSiteImpl_Load(This);

        /* Add the task bar band if it hasn't been added already */
        hRet = ITrayBandSiteImpl_AddTaskBand(This);
        if (SUCCEEDED(hRet))
        {
            hRet = ITaskBand_QueryInterface(This->TaskBand,
                                            &IID_IDeskBand,
                                            (PVOID*)&pDb);
            if (SUCCEEDED(hRet))
            {
                hRet = IDeskBand_GetWindow(pDb,
                                           phwndTaskSwitch);
                if (!SUCCEEDED(hRet))
                    *phwndTaskSwitch = NULL;

                IDeskBand_Release(pDb);
            }
        }

        /* Should we send this after showing it? */
        ITrayBandSiteImpl_Update(This);

        /* FIXME: When should we send this? Does anyone care anyway? */
        ITrayBandSiteImpl_FinishInit(This);

        /* Activate the band site */
        ITrayBandSiteImpl_Show(This,
                               TRUE);
    }

    *phWndRebar = This->hWndRebar;

    return This;
}

/*******************************************************************/

ITrayBandSite *
CreateTrayBandSite(IN OUT ITrayWindow *Tray,
                   OUT HWND *phWndRebar,
                   OUT HWND *phWndTaskSwitch)
{
    ITrayBandSiteImpl *This;

    This = ITrayBandSiteImpl_Construct(Tray,
                                       phWndRebar,
                                       phWndTaskSwitch);
    if (This != NULL)
    {
        return ITrayBandSite_from_ITrayBandSiteImpl(This);
    }

    return NULL;
}
