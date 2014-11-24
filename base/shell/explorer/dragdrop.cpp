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

class CDropTarget :
    public CComCoClass<CDropTarget>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget
{
    HWND hwndTarget;
    CComPtr<IDropTargetHelper> DropTargetHelper;
    PVOID Context;
    BOOL CanDrop;
    DROPTARGET_CALLBACKS Callbacks;
    DWORD FormatsCount;
    FORMATETC* Formats;
    
    const FORMATETC *
        FindSupportedFormat(IN IDataObject *pDataObject)
    {
        FORMATETC *Current, *Last;
        HRESULT hr;

        /* NOTE: we could use IDataObject::EnumFormatEtc(),
                 but this appears to be a lot easier! */
        Last = Formats + FormatsCount;
        for (Current = Formats;
            Current != Last;
            Current++)
        {
            hr = pDataObject->QueryGetData(Current);
            if (SUCCEEDED(hr))
                return Current;
        }

        return NULL;
    }

public:
    CDropTarget() :
        hwndTarget(NULL),
        Context(NULL),
        CanDrop(FALSE),
        FormatsCount(0),
        Formats(NULL)
    {
        ZeroMemory(&Callbacks, sizeof(Callbacks));
    }

    virtual ~CDropTarget() { }

    HRESULT Initialize(IN HWND hwndTarget,
        IN DWORD nSupportedFormats,
        IN const FORMATETC *formats OPTIONAL,
        IN PVOID Context  OPTIONAL,
        IN const DROPTARGET_CALLBACKS *Callbacks OPTIONAL)
    {
        this->hwndTarget = hwndTarget;
        FormatsCount = nSupportedFormats;
        if (nSupportedFormats != 0)
        {
            Formats = new FORMATETC[nSupportedFormats];
            CopyMemory(Formats,
                formats,
                sizeof(formats[0]) * nSupportedFormats);
        }

        this->Context = Context;
        if (Callbacks != NULL)
        {
            CopyMemory(&this->Callbacks,
                Callbacks,
                sizeof(*Callbacks));
        }

        HRESULT hr = CoCreateInstance(CLSID_DragDropHelper,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARG(IDropTargetHelper, &DropTargetHelper));

        return hr;
    }

    virtual HRESULT STDMETHODCALLTYPE DragEnter(
        IN IDataObject *pDataObject,
        IN DWORD grfKeyState,
        IN POINTL pt,
        IN OUT DWORD *pdwEffect)
    {
        const FORMATETC *Format;
        HRESULT hr;

        if (pDataObject == NULL)
            return E_INVALIDARG;

        CanDrop = FALSE;

        hr = DropTargetHelper->DragEnter(
            hwndTarget,
            pDataObject,
            (POINT *) &pt,
            *pdwEffect);

        if (SUCCEEDED(hr))
        {
            Format = FindSupportedFormat(
                pDataObject);
            if (Format != NULL)
            {
                /* We found a format that we support! */
                if (Callbacks.OnDragEnter != NULL)
                {
                    hr = Callbacks.OnDragEnter(this,
                        Context,
                        Format,
                        grfKeyState,
                        pt,
                        pdwEffect);
                    if (SUCCEEDED(hr))
                    {
                        if (hr == S_OK)
                            CanDrop = TRUE;
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

    virtual HRESULT STDMETHODCALLTYPE DragOver(
        IN DWORD grfKeyState,
        IN POINTL pt,
        IN OUT DWORD *pdwEffect)
    {
        HRESULT hr;

        hr = DropTargetHelper->DragOver(
            (POINT *) &pt,
            *pdwEffect);

        if (SUCCEEDED(hr))
        {
            if (CanDrop)
            {
                if (Callbacks.OnDragOver != NULL)
                {
                    hr = Callbacks.OnDragOver(this,
                        Context,
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

    virtual HRESULT STDMETHODCALLTYPE DragLeave()
    {
        HRESULT hr;

        hr = DropTargetHelper->DragLeave();
        if (SUCCEEDED(hr))
        {
            if (Callbacks.OnDragLeave != NULL)
            {
                hr = Callbacks.OnDragLeave(this,
                    Context);
            }
        }

        return hr;
    }

    virtual HRESULT STDMETHODCALLTYPE        Drop(
        IN IDataObject *pDataObject,
        IN DWORD grfKeyState,
        IN POINTL pt,
        IN OUT DWORD *pdwEffect)
    {
        const FORMATETC *Format;
        HRESULT hr;

        if (pDataObject == NULL)
            return E_INVALIDARG;

        hr = DropTargetHelper->Drop(
            pDataObject,
            (POINT *) &pt,
            *pdwEffect);

        if (SUCCEEDED(hr) && CanDrop)
        {
            Format = FindSupportedFormat(pDataObject);
            if (Format != NULL)
            {
                /* We found a format that we support! */
                if (Callbacks.OnDrop != NULL)
                {
                    hr = Callbacks.OnDrop(this,
                        Context,
                        Format,
                        grfKeyState,
                        pt,
                        pdwEffect);
                    if (SUCCEEDED(hr))
                    {
                        if (hr == S_OK)
                            CanDrop = TRUE;
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

    DECLARE_NOT_AGGREGATABLE(CDropTarget)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
    END_COM_MAP()
};

IDropTarget *
CreateDropTarget(IN HWND hwndTarget,
                 IN DWORD nSupportedFormats,
                 IN const FORMATETC *Formats  OPTIONAL,
                 IN PVOID Context  OPTIONAL,
                 IN const DROPTARGET_CALLBACKS *Callbacks  OPTIONAL)
{
    IDropTarget *dt;

    HRESULT hr = ShellObjectCreatorInit<CDropTarget>(hwndTarget, nSupportedFormats, Formats, Context, Callbacks, IID_IDropTarget, &dt);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    return dt;
}
