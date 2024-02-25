/*
 * Folder options.
 *
 * Copyright (C) 2016 Mark Jansen
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

#ifndef _CFOLDEROPTIONS_H_
#define _CFOLDEROPTIONS_H_

class CFolderOptions :
    public CComCoClass<CFolderOptions, &CLSID_ShellFldSetExt>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellPropSheetExt,
    public IShellExtInit,
    public IObjectWithSite
{
    private:
        CComPtr<IUnknown> m_pSite;
        //LPITEMIDLIST pidl;
        //INT iIdEmpty;
        //UINT cfShellIDList;
        //void SF_RegisterClipFmt();
        //BOOL fAcceptFmt;       /* flag for pending Drop */
        //BOOL QueryDrop (DWORD dwKeyState, LPDWORD pdwEffect);
        //BOOL RecycleBinIsEmpty();

    public:
        CFolderOptions();
        ~CFolderOptions();

        // IShellPropSheetExt
        virtual HRESULT STDMETHODCALLTYPE AddPages(LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
        virtual HRESULT STDMETHODCALLTYPE ReplacePage(EXPPS uPageID, LPFNSVADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

        // IShellExtInit
        virtual HRESULT STDMETHODCALLTYPE Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

        // IObjectWithSite
        virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
        virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

        DECLARE_REGISTRY_RESOURCEID(IDR_FOLDEROPTIONS)
        DECLARE_NOT_AGGREGATABLE(CFolderOptions)

        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CFolderOptions)
        COM_INTERFACE_ENTRY_IID(IID_IShellPropSheetExt, IShellPropSheetExt)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        END_COM_MAP()
};

#endif /* _CFOLDEROPTIONS_H_ */
