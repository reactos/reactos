/*
 * PROJECT:     ReactOS shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Copy as Path Menu implementation
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#pragma once

class CCopyAsPathMenu :
    public CComCoClass<CCopyAsPathMenu, &CLSID_CopyAsPathMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget
{
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_COPYASPATHMENU)
    DECLARE_NOT_AGGREGATABLE(CCopyAsPathMenu)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCopyAsPathMenu)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
    END_COM_MAP()

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
    {
        *pdwEffect &= DROPEFFECT_COPY;
        return S_OK;
    }
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
    {
        *pdwEffect &= DROPEFFECT_COPY;
        return S_OK;
    }
    STDMETHODIMP DragLeave()
    {
        return S_OK;
    }
    STDMETHODIMP Drop(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
};
