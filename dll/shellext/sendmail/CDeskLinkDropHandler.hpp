/*
 * PROJECT:     sendmail
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     DeskLink implementation
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifndef _CDESKLINKDROPHANDLER_HPP_
#define _CDESKLINKDROPHANDLER_HPP_

#include "resource.h"

class CDeskLinkDropHandler :
    public CComCoClass<CDeskLinkDropHandler, &CLSID_DeskLinkDropHandler>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget,
    public IPersistFile
{
public:
    CDeskLinkDropHandler();
    ~CDeskLinkDropHandler();

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

    DECLARE_REGISTRY_RESOURCEID(IDR_DESKLINK)
    DECLARE_NOT_AGGREGATABLE(CDeskLinkDropHandler)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CDeskLinkDropHandler)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
    END_COM_MAP()
};

#endif /* _CDESKLINKDROPHANDLER_HPP_ */
