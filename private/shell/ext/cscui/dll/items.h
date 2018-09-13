//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       items.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_ITEMS_H
#define _INC_CSCUI_ITEMS_H


#ifndef _INC_SHELL_IDLDATA_H
#   include "idldata.h"
#endif

class COfflineItemsData : public CIDLData
{
    public:
        COfflineItemsData(LPCITEMIDLIST pidlFolder, 
                          UINT cidl, 
                          LPCITEMIDLIST *apidl, 
                          HWND hwndParent,
                          IShellFolder *psfOwner = NULL,
                          IDataObject *pdtInner = NULL);

        ~COfflineItemsData(void);

        STDMETHODIMP GetData(FORMATETC *pFEIn, STGMEDIUM *pstm);
        STDMETHODIMP SetData(FORMATETC *pFEIn, STGMEDIUM *pstm, BOOL fRelease);
        STDMETHODIMP QueryGetData(FORMATETC *pFE);


        static HRESULT CreateInstance(COfflineItemsData **ppOut,
                                      LPCITEMIDLIST pidlFolder, 
                                      UINT cidl, 
                                      LPCITEMIDLIST *apidl, 
                                      HWND hwndParent,
                                      IShellFolder *psfOwner = NULL,
                                      IDataObject *pdtInner = NULL);

        static HRESULT CreateInstance(IDataObject **ppOut,
                                      LPCITEMIDLIST pidlFolder, 
                                      UINT cidl, 
                                      LPCITEMIDLIST *apidl, 
                                      HWND hwndParent,
                                      IShellFolder *psfOwner = NULL,
                                      IDataObject *pdtInner = NULL);

        HRESULT CtorResult(void) const
            { return m_hrCtor; }

    protected:
        HRESULT ProvideFormats(CEnumFormatEtc *pEnumFmtEtc);

    private:
        HWND                m_hwndParent;
        LPCOLID            *m_rgpolid; // Pidls cloned in private format.
        int                 m_cItems;
        HRESULT             m_hrCtor;
        DWORD               m_dwPreferredEffect;
        DWORD               m_dwPerformedEffect;
        DWORD               m_dwLogicalPerformedEffect;

        static CLIPFORMAT m_cfHDROP;
        static CLIPFORMAT m_cfFileContents;
        static CLIPFORMAT m_cfFileDesc;
        static CLIPFORMAT m_cfPreferedEffect;
        static CLIPFORMAT m_cfPerformedEffect;
        static CLIPFORMAT m_cfLogicalPerformedEffect;
        static CLIPFORMAT m_cfDataSrcClsid;
       
        HRESULT CreateFileDescriptor(STGMEDIUM *pstm);
        HRESULT CreateFileContents(STGMEDIUM *pstm, LONG lindex);
        HRESULT CreatePrefDropEffect(STGMEDIUM *pstm);
        HRESULT CreatePerformedDropEffect(STGMEDIUM *pstm);
        HRESULT CreateLogicalPerformedDropEffect(STGMEDIUM *pstm);
        HRESULT CreateHDROP(STGMEDIUM *pstm);
        HRESULT CreateDataSrcClsid(STGMEDIUM *pstm);
        HRESULT CreateDWORD(STGMEDIUM *pstm, DWORD dwEffect);
        DWORD GetDataDWORD(FORMATETC *pfe, STGMEDIUM *pstm, DWORD *pdwOut);

        //
        // Prevent copy.
        //
        COfflineItemsData(const COfflineItemsData& rhs);
        COfflineItemsData& operator = (const COfflineItemsData& rhs);
};




class COfflineItems : public IContextMenu, 
                      public IQueryInfo
{
    public:
        HRESULT Initialize(UINT cidl, LPCITEMIDLIST *ppidl);

        //
        // IUnknown Methods
        //
        STDMETHODIMP QueryInterface(REFIID,void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
    
        //
        // IContextMenu Methods
        //
        STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
        STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType,UINT *pwReserved, LPSTR pszName, UINT cchMax);

        //
        // IQueryInfo Methods
        //
        STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);
        STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);
    
        static HRESULT CreateInstance(COfflineFilesFolder *pfolder, 
                                      HWND hwnd, 
                                      UINT cidl, 
                                      LPCITEMIDLIST *ppidl, 
                                      REFIID riid, 
                                      void **ppv);
    private:
        //
        // Only create through CreateInstance() static function.
        //
        COfflineItems(COfflineFilesFolder *pfolder, HWND hwnd);
        ~COfflineItems();

        LONG                 m_cRef;        // reference count
        COfflineFilesFolder *m_pFolder;     // back pointer to our shell folder
        UINT                 m_cItems;      // number of items we represent
        LPCOLID             *m_ppolid;      // variable size array of items
        HWND                 m_hwndBrowser;
};

#endif // _INC_CSCUI_ITEMS_H
