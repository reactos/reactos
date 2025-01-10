/*
 * file system folder drop target
 *
 * Copyright 1997             Marcus Meissner
 * Copyright 1998, 1999, 2002 Juergen Schmied
 * Copyright 2009             Andrew Hill
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

#ifndef _CFSDROPTARGET_H_
#define _CFSDROPTARGET_H_

class CFSDropTarget :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget,
    public IObjectWithSite
{
    private:
        UINT m_cfShellIDList;    /* clipboardformat for IDropTarget */
        BOOL m_fAcceptFmt;       /* flag for pending Drop */
        LPWSTR m_sPathTarget;
        HWND m_hwndSite;
        DWORD m_grfKeyState;
        DWORD m_dwDefaultEffect;
        DWORD m_AllowedEffects;
        CComPtr<IUnknown> m_site;

        BOOL _QueryDrop (DWORD dwKeyState, LPDWORD pdwEffect);
        HRESULT _DoDrop(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
        HRESULT _CopyItems(IShellFolder *pSFFrom, UINT cidl, LPCITEMIDLIST *apidl, BOOL bCopy);
        BOOL _GetUniqueFileName(LPCWSTR pwszBasePath, LPCWSTR pwszExt, LPWSTR pwszTarget, BOOL bShortcut);
        static DWORD WINAPI _DoDropThreadProc(LPVOID lpParameter);
        HRESULT _GetEffectFromMenu(IDataObject *pDataObject, POINTL pt, DWORD *pdwEffect, DWORD dwAvailableEffects);
        HRESULT _RepositionItems(IShellFolderView *psfv, IDataObject *pDataObject, POINTL pt);

    public:
        CFSDropTarget();
        ~CFSDropTarget();
        HRESULT Initialize(LPWSTR PathTarget);

        // IDropTarget
        STDMETHOD(DragEnter)(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;
        STDMETHOD(DragOver)(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;
        STDMETHOD(DragLeave)() override;
        STDMETHOD(Drop)(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;

        // IObjectWithSite
        STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
        STDMETHOD(GetSite)(REFIID riid, void **ppvSite) override;

        DECLARE_NOT_AGGREGATABLE(CFSDropTarget)

        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CFSDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        END_COM_MAP()

};

struct _DoDropData {
    CFSDropTarget *This;
    IStream *pStream;
    DWORD dwKeyState;
    POINTL pt;
    DWORD pdwEffect;
};

HRESULT CFSDropTarget_CreateInstance(LPWSTR sPathTarget, REFIID riid, LPVOID * ppvOut);

#endif /* _CFSFOLDER_H_ */
