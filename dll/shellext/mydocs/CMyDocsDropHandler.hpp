/*
 * PROJECT:     mydocs
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     MyDocs implementation
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#include "resource.h"

class CMyDocsDropHandler :
    public CComCoClass<CMyDocsDropHandler, &CLSID_MyDocsDropHandler>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget,
    public IPersistFile
{
public:
    CMyDocsDropHandler();
    ~CMyDocsDropHandler();

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pDataObject, DWORD dwKeyState,
                           POINTL pt, DWORD *pdwEffect) override;
    STDMETHODIMP DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHODIMP DragLeave() override;
    STDMETHODIMP Drop(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt,
                      DWORD *pdwEffect) override;

    // IPersist
    STDMETHODIMP GetClassID(CLSID *lpClassId) override;

    // IPersistFile
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName) override;
    STDMETHODIMP IsDirty() override;
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode) override;
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember) override;
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_MYDOCS)
    DECLARE_NOT_AGGREGATABLE(CMyDocsDropHandler)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMyDocsDropHandler)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
    END_COM_MAP()
};
