/*
 * desklink drop target handler
 *
 * Copyright 2019 Katayama Hirofumi MZ.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _CDESKLINKDROPHANDLER_H_
#define _CDESKLINKDROPHANDLER_H_

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
                           POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt,
                      DWORD *pdwEffect);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *lpClassId);

    // IPersistFile
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);

    DECLARE_REGISTRY_RESOURCEID(IDR_DESKLINK)
    DECLARE_NOT_AGGREGATABLE(CDeskLinkDropHandler)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CDeskLinkDropHandler)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
    END_COM_MAP()
};

#endif /* _CDESKLINKDROPHANDLER_H_ */
