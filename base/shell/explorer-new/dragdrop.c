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

static const IDropTargetVtbl IDropTargetImpl_Vtbl;

/*
 * IDropTarget
 */

typedef struct
{
    const IDropTargetVtbl *lpVtbl;
    LONG Ref;
    HWND hwndTarget;
    IDropTargetHelper *DropTargetHelper;
    PVOID Context;
    BOOL CanDrop;
    DROPTARGET_CALLBACKS Callbacks;
    DWORD FormatsCount;
    FORMATETC Formats[0];
} IDropTargetImpl;

static IUnknown *
IUnknown_from_impl(IDropTargetImpl *This)
{
    return (IUnknown *)&This->lpVtbl;
}

static IDropTarget *
IDropTarget_from_impl(IDropTargetImpl *This)
{
    return (IDropTarget *)&This->lpVtbl;
}

static IDropTargetImpl *
impl_from_IDropTarget(IDropTarget *iface)
{
    return (IDropTargetImpl *)((ULONG_PTR)iface - FIELD_OFFSET(IDropTargetImpl,
                                                               lpVtbl));
}

static VOID
IDropTargetImpl_Free(IDropTargetImpl *This)
{
    IDropTargetHelper_Release(This->DropTargetHelper);
}

static ULONG STDMETHODCALLTYPE
IDropTargetImpl_Release(IN OUT IDropTarget *iface)
{
    IDropTargetImpl *This = impl_from_IDropTarget(iface);
    ULONG Ret;

    Ret = InterlockedDecrement(&This->Ref);
    if (Ret == 0)
        IDropTargetImpl_Free(This);

    return Ret;
}

static ULONG STDMETHODCALLTYPE
IDropTargetImpl_AddRef(IN OUT IDropTarget *iface)
{
    IDropTargetImpl *This = impl_from_IDropTarget(iface);

    return InterlockedIncrement(&This->Ref);
}

static HRESULT STDMETHODCALLTYPE
IDropTargetImpl_QueryInterface(IN OUT IDropTarget *iface,
                               IN REFIID riid,
                               OUT LPVOID *ppvObj)
{
    IDropTargetImpl *This;

    if (ppvObj == NULL)
        return E_POINTER;

    This = impl_from_IDropTarget(iface);

    if (IsEqualIID(riid,
                   &IID_IUnknown))
    {
        *ppvObj = IUnknown_from_impl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IDropTarget))
    {
        *ppvObj = IDropTarget_from_impl(This);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    IDropTargetImpl_AddRef(iface);
    return S_OK;
}

IDropTarget *
CreateDropTarget(IN HWND hwndTarget,
                 IN DWORD nSupportedFormats,
                 IN const FORMATETC *Formats  OPTIONAL,
                 IN PVOID Context  OPTIONAL,
                 IN const DROPTARGET_CALLBACKS *Callbacks  OPTIONAL)
{
    IDropTargetImpl *This;
    HRESULT hr;

    This = (IDropTargetImpl *)HeapAlloc(hProcessHeap,
                                        0,
                                        FIELD_OFFSET(IDropTargetImpl,
                                                     Formats[nSupportedFormats]));
    if (This != NULL)
    {
        ZeroMemory(This,
                   sizeof(*This));

        This->lpVtbl = &IDropTargetImpl_Vtbl;
        This->Ref = 1;
        This->hwndTarget = hwndTarget;
        This->FormatsCount = nSupportedFormats;
        if (nSupportedFormats != 0)
        {
            CopyMemory(This->Formats,
                       Formats,
                       sizeof(Formats[0]) * nSupportedFormats);
        }

        This->Context = Context;
        if (Callbacks != NULL)
        {
            CopyMemory(&This->Callbacks,
                       Callbacks,
                       sizeof(Callbacks));
        }

        hr = CoCreateInstance(&CLSID_DragDropHelper,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              &IID_IDropTargetHelper,
                              (PVOID)&This->DropTargetHelper);

        if (!SUCCEEDED(hr))
        {
            HeapFree(hProcessHeap,
                     0,
                     This);
            return NULL;
        }

        return IDropTarget_from_impl(This);
    }

    return NULL;
}

static const FORMATETC *
IDropTargetImpl_FindSupportedFormat(IN OUT IDropTargetImpl *This,
                                    IN IDataObject *pDataObject)
{
    FORMATETC *Current, *Last;
    HRESULT hr;

    /* NOTE: we could use IDataObject::EnumFormatEtc(),
             but this appears to be a lot easier! */
    Last = This->Formats + This->FormatsCount;
    for (Current = This->Formats;
         Current != Last;
         Current++)
    {
        hr = IDataObject_QueryGetData(pDataObject,
                                      Current);
        if (SUCCEEDED(hr))
            return Current;
    }

    return NULL;
}

static HRESULT STDMETHODCALLTYPE
IDropTargetImpl_DragEnter(IN OUT IDropTarget *iface,
                          IN IDataObject *pDataObject,
                          IN DWORD grfKeyState,
                          IN POINTL pt,
                          IN OUT DWORD *pdwEffect)
{
    IDropTargetImpl *This = impl_from_IDropTarget(iface);
    const FORMATETC *Format;
    HRESULT hr;

    if (pDataObject == NULL)
        return E_INVALIDARG;

    This->CanDrop = FALSE;

    hr = IDropTargetHelper_DragEnter(This->DropTargetHelper,
                                     This->hwndTarget,
                                     pDataObject,
                                     (POINT *)&pt,
                                     *pdwEffect);

    if (SUCCEEDED(hr))
    {
        Format = IDropTargetImpl_FindSupportedFormat(This,
                                                     pDataObject);
        if (Format != NULL)
        {
            /* We found a format that we support! */
            if (This->Callbacks.OnDragEnter != NULL)
            {
                hr = This->Callbacks.OnDragEnter(iface,
                                                 This->Context,
                                                 Format,
                                                 grfKeyState,
                                                 pt,
                                                 pdwEffect);
                if (SUCCEEDED(hr))
                {
                    if (hr == S_OK)
                        This->CanDrop = TRUE;
                    else
                    {
                        /* Special return value by the callback routine,
                           doesn't want to allow dragging */
                        *pdwEffect = DROPEFFECT_NONE;
                    }

                    hr = S_OK;
                }
                else
                {
                    *pdwEffect = DROPEFFECT_NONE;
                    hr = S_OK;
                }
            }
            else
                *pdwEffect = DROPEFFECT_NONE;
        }
        else
            *pdwEffect = DROPEFFECT_NONE;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE
IDropTargetImpl_DragOver(IN OUT IDropTarget *iface,
                         IN DWORD grfKeyState,
                         IN POINTL pt,
                         IN OUT DWORD *pdwEffect)
{
    IDropTargetImpl *This = impl_from_IDropTarget(iface);
    HRESULT hr;

    hr = IDropTargetHelper_DragOver(This->DropTargetHelper,
                                    (POINT *)&pt,
                                    *pdwEffect);

    if (SUCCEEDED(hr))
    {
        if (This->CanDrop)
        {
            if (This->Callbacks.OnDragOver != NULL)
            {
                hr = This->Callbacks.OnDragOver(iface,
                                                This->Context,
                                                grfKeyState,
                                                pt,
                                                pdwEffect);
                if (SUCCEEDED(hr))
                {
                    if (hr != S_OK)
                    {
                        /* Special return value by the callback routine,
                           doesn't want to allow dropping here */
                        *pdwEffect = DROPEFFECT_NONE;
                    }

                    hr = S_OK;
                }
                else
                {
                    *pdwEffect = DROPEFFECT_NONE;
                    hr = S_OK;
                }
            }
            else
                *pdwEffect = DROPEFFECT_NONE;
        }
        else
            *pdwEffect = DROPEFFECT_NONE;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE
IDropTargetImpl_DragLeave(IN OUT IDropTarget *iface)
{
    IDropTargetImpl *This = impl_from_IDropTarget(iface);
    HRESULT hr;

    hr = IDropTargetHelper_DragLeave(This->DropTargetHelper);
    if (SUCCEEDED(hr))
    {
        if (This->Callbacks.OnDragLeave != NULL)
        {
            hr = This->Callbacks.OnDragLeave(iface,
                                             This->Context);
        }
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE
IDropTargetImpl_Drop(IN OUT IDropTarget *iface,
                     IN IDataObject *pDataObject,
                     IN DWORD grfKeyState,
                     IN POINTL pt,
                     IN OUT DWORD *pdwEffect)
{
    IDropTargetImpl *This = impl_from_IDropTarget(iface);
    const FORMATETC *Format;
    HRESULT hr;

    if (pDataObject == NULL)
        return E_INVALIDARG;

    hr = IDropTargetHelper_Drop(This->DropTargetHelper,
                                pDataObject,
                                (POINT *)&pt,
                                *pdwEffect);

    if (SUCCEEDED(hr) && This->CanDrop)
    {
        Format = IDropTargetImpl_FindSupportedFormat(This,
                                                     pDataObject);
        if (Format != NULL)
        {
            /* We found a format that we support! */
            if (This->Callbacks.OnDrop != NULL)
            {
                hr = This->Callbacks.OnDrop(iface,
                                            This->Context,
                                            Format,
                                            grfKeyState,
                                            pt,
                                            pdwEffect);
                if (SUCCEEDED(hr))
                {
                    if (hr == S_OK)
                        This->CanDrop = TRUE;
                    else
                    {
                        /* Special return value by the callback routine,
                           doesn't want to allow dragging */
                        *pdwEffect = DROPEFFECT_NONE;
                    }

                    hr = S_OK;
                }
                else
                {
                    *pdwEffect = DROPEFFECT_NONE;
                    hr = S_OK;
                }
            }
            else
                *pdwEffect = DROPEFFECT_NONE;
        }
        else
            *pdwEffect = DROPEFFECT_NONE;
    }

    return hr;
}

static const IDropTargetVtbl IDropTargetImpl_Vtbl =
{
    /* IUnknown */
    IDropTargetImpl_QueryInterface,
    IDropTargetImpl_AddRef,
    IDropTargetImpl_Release,
    /* IDropTarget */
    IDropTargetImpl_DragEnter,
    IDropTargetImpl_DragOver,
    IDropTargetImpl_DragLeave,
    IDropTargetImpl_Drop
};
