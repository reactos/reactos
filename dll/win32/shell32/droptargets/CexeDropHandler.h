/*
 * executable drop target handler
 *
 * Copyright 2014              Huw Campbell
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

#ifndef _CEXEDROPHANDLER_H_
#define _CEXEDROPHANDLER_H_

class CExeDropHandler :
    public CComCoClass<CExeDropHandler, &CLSID_ExeDropHandler>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget,
    public IPersistFile
{
private:
    CLSID *pclsid;
    LPWSTR sPathTarget;
public:
    CExeDropHandler();
    ~CExeDropHandler();

    // IDropTarget
    STDMETHOD(DragEnter)(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragOver)(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragLeave)() override;
    STDMETHOD(Drop)(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;

    // IPersist
    STDMETHOD(GetClassID)(CLSID *lpClassId) override;

    //////// IPersistFile
    STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName) override;
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode) override;
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember) override;
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName) override;

DECLARE_REGISTRY_RESOURCEID(IDR_EXEDROPHANDLER)
DECLARE_NOT_AGGREGATABLE(CExeDropHandler)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CExeDropHandler)
    COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
    COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
    COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
END_COM_MAP()
};

#endif /* _CEXEDROPHANDLER_H_ */
