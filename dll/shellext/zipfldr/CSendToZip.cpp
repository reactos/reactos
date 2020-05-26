/*
 * PROJECT:   ReactOS Zip Shell Extension
 * LICENSE:   GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:   SendTo handler
 * COPYRIGHT: Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 *            Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

STDMETHODIMP
CSendToZip::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt,
                      DWORD *pdwEffect)
{
    m_pDataObject = pDataObj;

    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    m_fCanDragDrop = SUCCEEDED(pDataObj->QueryGetData(&etc));

    if (m_fCanDragDrop)
        *pdwEffect &= DROPEFFECT_COPY;
    else
        *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}

STDMETHODIMP CSendToZip::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (m_fCanDragDrop)
        *pdwEffect &= DROPEFFECT_COPY;
    else
        *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}

STDMETHODIMP CSendToZip::DragLeave()
{
    m_fCanDragDrop = FALSE;
    m_pDataObject.Release();
    return S_OK;
}

STDMETHODIMP
CSendToZip::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt,
                 DWORD *pdwEffect)
{
    m_pDataObject = pDataObj;

    if (!pDataObj || !m_fCanDragDrop)
    {
        DPRINT1("Drop failed: %d %d\n", !pDataObj, !m_fCanDragDrop);
        *pdwEffect = 0;
        DragLeave();
        return E_FAIL;
    }

    STGMEDIUM stg;
    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = pDataObj->GetData(&etc, &stg);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        *pdwEffect = 0;
        DragLeave();
        return E_FAIL;
    }

    HDROP hDrop = reinterpret_cast<HDROP>(stg.hGlobal);
    UINT cItems = ::DragQueryFileW(hDrop, -1, NULL, 0);

    CZipCreator *pCreator = CZipCreator::DoCreate();

    for (UINT iItem = 0; iItem < cItems; ++iItem)
    {
        WCHAR szPath[MAX_PATH];
        DragQueryFileW(hDrop, iItem, szPath, _countof(szPath));

        pCreator->DoAddItem(szPath);
    }

    ::ReleaseStgMedium(&stg);

    CZipCreator::runThread(pCreator);   // pCreator is deleted in runThread

    DragLeave();
    return hr;
}
