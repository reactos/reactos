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
 ** ITaskBand ****************************************************************
 *****************************************************************************/

const GUID CLSID_ITaskBand = {0x68284FAA,0x6A48,0x11D0,{0x8C,0x78,0x00,0xC0,0x4F,0xD9,0x18,0xB4}};

static const ITaskBandVtbl ITaskBandImpl_Vtbl;
static const IDeskBandVtbl IDeskBandImpl_Vtbl;
static const IObjectWithSiteVtbl IObjectWithSiteImpl_Vtbl;
static const IDeskBarVtbl IDeskBarImpl_Vtbl;
static const IPersistStreamVtbl IPersistStreamImpl_Vtbl;
static const IWindowEventHandlerVtbl IWindowEventHandlerImpl_Vtbl;

typedef struct
{
    const ITaskBandVtbl *lpVtbl;
    const IDeskBandVtbl *lpDeskBandVtbl;
    const IObjectWithSiteVtbl *lpObjectWithSiteVtbl;
    const IDeskBarVtbl *lpDeskBarVtbl;
    const IPersistStreamVtbl *lpPersistStreamVtbl;
    const IWindowEventHandlerVtbl *lpWindowEventHandlerVtbl;
    /* FIXME: Implement IOleCommandTarget */
    LONG Ref;

    ITrayWindow *Tray;
    IUnknown *punkSite;

    HWND hWnd;
    DWORD dwBandID;
} ITaskBandImpl;

static IUnknown *
IUnknown_from_ITaskBandImpl(ITaskBandImpl *This)
{
    return (IUnknown *)&This->lpVtbl;
}

IMPL_CASTS(ITaskBand, ITaskBand, lpVtbl)
IMPL_CASTS(IDeskBand, ITaskBand, lpDeskBandVtbl)
IMPL_CASTS(IObjectWithSite, ITaskBand, lpObjectWithSiteVtbl)
IMPL_CASTS(IDeskBar, ITaskBand, lpDeskBarVtbl)
IMPL_CASTS(IPersistStream, ITaskBand, lpPersistStreamVtbl)
IMPL_CASTS(IWindowEventHandler, ITaskBand, lpWindowEventHandlerVtbl)

static ULONG STDMETHODCALLTYPE
ITaskBandImpl_AddRef(IN OUT ITaskBand *iface)
{
    ITaskBandImpl *This = ITaskBandImpl_from_ITaskBand(iface);

    return InterlockedIncrement(&This->Ref);
}

static VOID
ITaskBandImpl_Free(IN OUT ITaskBandImpl *This)
{
    if (This->punkSite != NULL)
    {
        IUnknown_Release(This->punkSite);
        This->punkSite = NULL;
    }

    HeapFree(hProcessHeap,
             0,
             This);
}

static ULONG STDMETHODCALLTYPE
ITaskBandImpl_Release(IN OUT ITaskBand *iface)
{
    ITaskBandImpl *This = ITaskBandImpl_from_ITaskBand(iface);
    ULONG Ret;

    Ret = InterlockedDecrement(&This->Ref);

    if (Ret == 0)
        ITaskBandImpl_Free(This);

    return Ret;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_QueryInterface(IN OUT ITaskBand *iface,
                             IN REFIID riid,
                             OUT LPVOID *ppvObj)
{
    ITaskBandImpl *This;

    if (ppvObj == NULL)
        return E_POINTER;

    This = ITaskBandImpl_from_ITaskBand(iface);

    if (IsEqualIID(riid,
                   &IID_IUnknown))
    {
        *ppvObj = IUnknown_from_ITaskBandImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IDeskBand) ||
             IsEqualIID(riid,
                        &IID_IOleWindow) ||
             IsEqualIID(riid,
                        &IID_IDockingWindow))
    {
        *ppvObj = IDeskBand_from_ITaskBandImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IObjectWithSite))
    {
        *ppvObj = IObjectWithSite_from_ITaskBandImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IDeskBar))
    {
        *ppvObj = IDeskBar_from_ITaskBandImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IWindowEventHandler))
    {
        /* When run on Windows the system queries this interface, which is completely
           undocumented :( It's queried during initialization of the tray band site.
           The system apparently uses this interface to forward messages to be handled
           by the band child window. This interface appears to be implemented by a number
           of classes provided by the shell, including the IBandSite interface. In that
           we (the host application) forward messages to the default message handler (in
           our case the IBandSite default message handler for the Rebar control)! This
           interface in the ITaskBand implementation is only actually used if we use
           the same interface to forward messages to the IBandSite implementation of
           the shell! */
        *ppvObj = IWindowEventHandler_from_ITaskBandImpl(This);
    }
#if 0
    else if (IsEqualIID(riid,
                        &IID_IPersistStream) ||
             IsEqualIID(riid,
                        &IID_IPersist))
    {
        *ppvObj = IPersistStream_from_ITaskBandImpl(This);
    }
#endif
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ITaskBandImpl_AddRef(iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_GetRebarBandID(IN OUT ITaskBand *iface,
                             OUT DWORD *pdwBandID)
{
    ITaskBandImpl *This = ITaskBandImpl_from_ITaskBand(iface);

    if (This->dwBandID != (DWORD)-1)
    {
        if (pdwBandID != NULL)
            *pdwBandID = This->dwBandID;

        return S_OK;
    }

    return E_FAIL;
}

static const ITaskBandVtbl ITaskBandImpl_Vtbl =
{
    /*** IUnknown methods ***/
    ITaskBandImpl_QueryInterface,
    ITaskBandImpl_AddRef,
    ITaskBandImpl_Release,
    /*** ITaskBand methods ***/
    ITaskBandImpl_GetRebarBandID
};

/*****************************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IDeskBand, ITaskBand)
METHOD_IUNKNOWN_INHERITED_RELEASE(IDeskBand, ITaskBand)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IDeskBand, ITaskBand)

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_GetWindow(IN OUT IDeskBand *iface,
                        OUT HWND *phwnd)
{
    ITaskBandImpl *This = ITaskBandImpl_from_IDeskBand(iface);

    /* NOTE: We have to return the tray window here so that ITaskBarClient
             knows the parent window of the Rebar control it creates when
             calling ITaskBarClient::SetDeskBarSite()! However, once we
             created a window we return the task switch window! */
    if (This->hWnd != NULL)
        *phwnd = This->hWnd;
    else
        *phwnd = ITrayWindow_GetHWND(This->Tray);

    DbgPrint("ITaskBand::GetWindow(0x%p->0x%p)\n", phwnd, *phwnd);

    if (*phwnd != NULL)
        return S_OK;

    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_ContextSensitiveHelp(IN OUT IDeskBand *iface,
                                   IN BOOL fEnterMode)
{
    /* FIXME: Implement */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_ShowDW(IN OUT IDeskBand *iface,
                     IN BOOL bShow)
{
    /* We don't do anything... */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_CloseDW(IN OUT IDeskBand *iface,
                      IN DWORD dwReserved)
{
    /* We don't do anything... */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_ResizeBoderDW(IN OUT IDeskBand *iface,
                            IN LPCRECT prcBorder,
                            IN IUnknown *punkToolbarSite,
                            IN BOOL fReserved)
{
    /* No need to implement this method */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_GetBandInfo(IN OUT IDeskBand *iface,
                          IN DWORD dwBandID,
                          IN DWORD dwViewMode,
                          IN OUT DESKBANDINFO *pdbi)
{
    ITaskBandImpl *This = ITaskBandImpl_from_IDeskBand(iface);
    DbgPrint("ITaskBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, This->hWnd);

    /* NOTE: We could save dwBandID in the instance in case we need it later... */

    if (This->hWnd != NULL)
    {
        /* The task band never has a title */
        pdbi->dwMask &= ~DBIM_TITLE;

        /* NOTE: We don't return DBIMF_UNDELETEABLE here, the band site will
                 handle us differently and add this flag for us. The reason for
                 this is future changes that might allow it to be deletable.
                 We want the band site to be in charge of this decision rather
                 the band itself! */
        /* FIXME: What about DBIMF_NOGRIPPER and DBIMF_ALWAYSGRIPPER */
        pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT;

        if (dwViewMode & DBIF_VIEWMODE_VERTICAL)
        {
            pdbi->ptIntegral.y = 1;
            pdbi->ptMinSize.y = 1;
            /* FIXME: Get the button metrics from the task bar object!!! */
            pdbi->ptMinSize.x = (3 * GetSystemMetrics(SM_CXEDGE) / 2) + /* FIXME: Might be wrong if only one column! */
                                GetSystemMetrics(SM_CXSIZE) + (2 * GetSystemMetrics(SM_CXEDGE)); /* FIXME: Min button size, query!!! */
        }
        else
        {
            pdbi->ptMinSize.y = GetSystemMetrics(SM_CYSIZE) + (2 * GetSystemMetrics(SM_CYEDGE)); /* FIXME: Query */
            pdbi->ptIntegral.y = pdbi->ptMinSize.y + (3 * GetSystemMetrics(SM_CYEDGE) / 2); /* FIXME: Query metrics */
            /* We're not going to allow task bands where not even the minimum button size fits into the band */
            pdbi->ptMinSize.x = pdbi->ptIntegral.y;
        }

        /* Ignored: pdbi->ptMaxSize.x */
        pdbi->ptMaxSize.y = -1;

        /* FIXME: We should query the height from the task bar object!!! */
        pdbi->ptActual.y = GetSystemMetrics(SM_CYSIZE) + (2 * GetSystemMetrics(SM_CYEDGE));

        /* Save the band ID for future use in case we need to check whether a given band
           is the task band */
        This->dwBandID = dwBandID;

        DbgPrint("H: %d, Min: %d,%d, Integral.y: %d Actual: %d,%d\n", (dwViewMode & DBIF_VIEWMODE_VERTICAL) == 0,
                                                        pdbi->ptMinSize.x, pdbi->ptMinSize.y, pdbi->ptIntegral.y,
                                                        pdbi->ptActual.x,pdbi->ptActual.y);

        return S_OK;
    }

    return E_FAIL;
}

static const IDeskBandVtbl IDeskBandImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IDeskBand, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IDeskBand, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IDeskBand, ITaskBand),
    /*** IOleWindow methods ***/
    ITaskBandImpl_GetWindow,
    ITaskBandImpl_ContextSensitiveHelp,
    /*** IDockingWindow methods ***/
    ITaskBandImpl_ShowDW,
    ITaskBandImpl_CloseDW,
    ITaskBandImpl_ResizeBoderDW,
    /*** IDeskBand methods ***/
    ITaskBandImpl_GetBandInfo
};

/*****************************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IDeskBar, ITaskBand)
METHOD_IUNKNOWN_INHERITED_RELEASE(IDeskBar, ITaskBand)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IDeskBar, ITaskBand)

static HRESULT STDMETHODCALLTYPE
IDeskBarImpl_GetWindow(IN OUT IDeskBar *iface,
                       OUT HWND *phwnd)
{
    ITaskBandImpl *This = ITaskBandImpl_from_IDeskBar(iface);
    IDeskBand *DeskBand = IDeskBand_from_ITaskBandImpl(This);

    /* Proxy to IDeskBand interface */
    return IDeskBand_GetWindow(DeskBand,
                               phwnd);
}

static HRESULT STDMETHODCALLTYPE
IDeskBarImpl_ContextSensitiveHelp(IN OUT IDeskBar *iface,
                                  IN BOOL fEnterMode)
{
    ITaskBandImpl *This = ITaskBandImpl_from_IDeskBar(iface);
    IDeskBand *DeskBand = IDeskBand_from_ITaskBandImpl(This);

    /* Proxy to IDeskBand interface */
    return IDeskBand_ContextSensitiveHelp(DeskBand,
                                          fEnterMode);
}

static HRESULT STDMETHODCALLTYPE
IDeskBarImpl_SetClient(IN OUT IDeskBar *iface,
                       IN IUnknown *punkClient)
{
    DbgPrint("IDeskBar::SetClient(0x%p)\n", punkClient);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDeskBarImpl_GetClient(IN OUT IDeskBar *iface,
                       OUT IUnknown **ppunkClient)
{
    DbgPrint("IDeskBar::GetClient(0x%p)\n", ppunkClient);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDeskBarImpl_OnPosRectChangeDB(IN OUT IDeskBar *iface,
                               IN RECT *prc)
{
    DbgPrint("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);
    if (prc->bottom - prc->top == 0)
        return S_OK;

    return S_FALSE;
}

static const IDeskBarVtbl IDeskBarImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IDeskBar, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IDeskBar, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IDeskBar, ITaskBand),
    /*** IOleWindow methods ***/
    IDeskBarImpl_GetWindow,
    IDeskBarImpl_ContextSensitiveHelp,
    /*** IDeskBar methods ***/
    IDeskBarImpl_SetClient,
    IDeskBarImpl_GetClient,
    IDeskBarImpl_OnPosRectChangeDB
};

/*****************************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IPersistStream, ITaskBand)
METHOD_IUNKNOWN_INHERITED_RELEASE(IPersistStream, ITaskBand)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IPersistStream, ITaskBand)

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_GetClassID(IN OUT IPersistStream *iface,
                         OUT CLSID *pClassID)
{
    DbgPrint("ITaskBand::GetClassID(0x%p)\n", pClassID);
    /* We're going to return the (internal!) CLSID of the task band interface */
    *pClassID = CLSID_ITaskBand;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_IsDirty(IN OUT IPersistStream *iface)
{
    /* The object hasn't changed since the last save! */
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_Load(IN OUT IPersistStream *iface,
                   IN IStream *pStm)
{
    DbgPrint("ITaskBand::Load called\n");
    /* Nothing to do */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_Save(IN OUT IPersistStream *iface,
                   IN IStream *pStm,
                   IN BOOL fClearDirty)
{
    /* Nothing to do */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_GetSizeMax(IN OUT IPersistStream *iface,
                         OUT ULARGE_INTEGER *pcbSize)
{
    DbgPrint("ITaskBand::GetSizeMax called\n");
    /* We don't need any space for the task band */
    pcbSize->QuadPart = 0;
    return S_OK;
}

static const IPersistStreamVtbl IPersistStreamImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IPersistStream, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IPersistStream, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IPersistStream, ITaskBand),
    /*** IPersist methods ***/
    ITaskBandImpl_GetClassID,
    /*** IPersistStream methods ***/
    ITaskBandImpl_IsDirty,
    ITaskBandImpl_Load,
    ITaskBandImpl_Save,
    ITaskBandImpl_GetSizeMax
};

/*****************************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IObjectWithSite, ITaskBand)
METHOD_IUNKNOWN_INHERITED_RELEASE(IObjectWithSite, ITaskBand)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IObjectWithSite, ITaskBand)

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_SetSite(IN OUT IObjectWithSite *iface,
                      IN IUnknown* pUnkSite)
{
    ITaskBandImpl *This = ITaskBandImpl_from_IObjectWithSite(iface);
    HRESULT hRet = E_FAIL;

    DbgPrint("ITaskBand::SetSite(0x%p)\n", pUnkSite);

    /* Release the current site */
    if (This->punkSite != NULL)
    {
        IUnknown_Release(This->punkSite);
    }

    This->punkSite = NULL;
    This->hWnd = NULL;

    if (pUnkSite != NULL)
    {
        IOleWindow *OleWindow;

        /* Check if the site supports IOleWindow */
        hRet = IUnknown_QueryInterface(pUnkSite,
                                       &IID_IOleWindow,
                                       (PVOID*)&OleWindow);
        if (SUCCEEDED(hRet))
        {
            HWND hWndParent = NULL;

            hRet = IOleWindow_GetWindow(OleWindow,
                                        &hWndParent);
            if (SUCCEEDED(hRet))
            {
                /* Attempt to create the task switch window */

                DbgPrint("CreateTaskSwitchWnd(Parent: 0x%p)\n", hWndParent);
                This->hWnd = CreateTaskSwitchWnd(hWndParent,
                                                 This->Tray);
                if (This->hWnd != NULL)
                {
                    This->punkSite = pUnkSite;
                    hRet = S_OK;
                }
                else
                {
                    DbgPrint("CreateTaskSwitchWnd() failed!\n");
                    IUnknown_Release(OleWindow);
                    hRet = E_FAIL;
                }
            }
            else
                IUnknown_Release(OleWindow);
        }
        else
            DbgPrint("Querying IOleWindow failed: 0x%x\n", hRet);
    }

    return hRet;
}

static HRESULT STDMETHODCALLTYPE
ITaskBandImpl_GetSite(IN OUT IObjectWithSite *iface,
                      IN REFIID riid,
                      OUT VOID **ppvSite)
{
    ITaskBandImpl *This = ITaskBandImpl_from_IObjectWithSite(iface);
    DbgPrint("ITaskBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

    if (This->punkSite != NULL)
    {
        return IUnknown_QueryInterface(This->punkSite,
                                       riid,
                                       ppvSite);
    }

    *ppvSite = NULL;
    return E_FAIL;
}

static const IObjectWithSiteVtbl IObjectWithSiteImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IObjectWithSite, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IObjectWithSite, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IObjectWithSite, ITaskBand),
    /*** IObjectWithSite methods ***/
    ITaskBandImpl_SetSite,
    ITaskBandImpl_GetSite
};


/*****************************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IWindowEventHandler, ITaskBand)
METHOD_IUNKNOWN_INHERITED_RELEASE(IWindowEventHandler, ITaskBand)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IWindowEventHandler, ITaskBand)

static HRESULT STDMETHODCALLTYPE
IWindowEventHandlerImpl_ProcessMessage(IN OUT IWindowEventHandler *iface,
                                       IN HWND hWnd,
                                       IN UINT uMsg,
                                       IN WPARAM wParam,
                                       IN LPARAM lParam,
                                       OUT LRESULT *plrResult)
{
    DbgPrint("ITaskBand: IWindowEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p)\n", hWnd, uMsg, wParam, lParam, plrResult);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IWindowEventHandlerImpl_ContainsWindow(IN OUT IWindowEventHandler *iface,
                                       IN HWND hWnd)
{
    ITaskBandImpl *This = ITaskBandImpl_from_IWindowEventHandler(iface);
    HRESULT hRet = S_OK;

    if (This->hWnd != hWnd ||
        !IsChild(This->hWnd,
                 hWnd))
    {
        hRet = S_FALSE;
    }

    DbgPrint("ITaskBand::ContainsWindow(0x%p) returns %s\n", hWnd, hRet == S_OK ? "S_OK" : "S_FALSE");

    return hRet;
}

static const IWindowEventHandlerVtbl IWindowEventHandlerImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IWindowEventHandler, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IWindowEventHandler, ITaskBand),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IWindowEventHandler, ITaskBand),
    /*** IWindowEventHandler methods ***/
    IWindowEventHandlerImpl_ProcessMessage,
    IWindowEventHandlerImpl_ContainsWindow
};

/*****************************************************************************/

static ITaskBandImpl *
ITaskBandImpl_Construct(IN OUT ITrayWindow *Tray)
{
    ITaskBandImpl *This;

    This = HeapAlloc(hProcessHeap,
                     0,
                     sizeof(*This));
    if (This == NULL)
        return NULL;

    ZeroMemory(This,
               sizeof(*This));
    This->lpVtbl = &ITaskBandImpl_Vtbl;
    This->lpDeskBandVtbl = &IDeskBandImpl_Vtbl;
    This->lpObjectWithSiteVtbl = &IObjectWithSiteImpl_Vtbl;
    This->lpDeskBarVtbl = &IDeskBarImpl_Vtbl;
    This->lpPersistStreamVtbl = &IPersistStreamImpl_Vtbl;
    This->lpWindowEventHandlerVtbl = &IWindowEventHandlerImpl_Vtbl;
    This->Ref = 1;

    This->Tray = Tray;
    This->dwBandID = (DWORD)-1;

    return This;
}

ITaskBand *
CreateTaskBand(IN OUT ITrayWindow *Tray)
{
    ITaskBandImpl *This;

    This = ITaskBandImpl_Construct(Tray);
    if (This != NULL)
    {
        return ITaskBand_from_ITaskBandImpl(This);
    }

    return NULL;
}
