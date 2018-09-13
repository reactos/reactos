#ifndef __SHDISP_H__
#define __SHDISP_H__

#ifdef __cplusplus
#include "dspsprt.h"
#include "cobjsafe.h"
#include "..\cowsite.h"
HRESULT MakeSafeForScripting(IUnknown** ppDisp);

class CImpConPtCont;
typedef CImpConPtCont *PCImpConPtCont;

class CConnectionPoint;
typedef CConnectionPoint *PCConnectionPoint;

class CImpISupportErrorInfo;
typedef CImpISupportErrorInfo *PCImpISupportErrorInfo;

class CFolder;

HRESULT GetObjectSafely(IShellFolderView *psfv, LPCITEMIDLIST *ppidl, UINT iType);
HRESULT GetApplicationObject(DWORD dwSafetyOptions, IUnknown *punkSite, IDispatch **ppid);

HRESULT CFolder_Create(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf, REFIID riid, void **ppv);
HRESULT CFolder_Create2(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf, CFolder **psdf);

HRESULT CFolderItems_Create(CFolder *psdf, BOOL fSelected, FolderItems **ppid);
HRESULT CFolderItem_Create(CFolder *psdf, LPCITEMIDLIST pidl, FolderItem **ppid);
HRESULT CFolderItem_CreateFromIDList(HWND hwnd, LPCITEMIDLIST pidl, FolderItem **ppid);
HRESULT CShortcut_CreateIDispatch(HWND hwnd, IShellFolder *psf, LPCITEMIDLIST pidl, IDispatch **ppid);
HRESULT CSDWindow_Create(HWND hwndFldr, IDispatch ** ppsw);

extern HRESULT InvokeVerbHelper(VARIANT vVerb, VARIANT vArgs, LPCITEMIDLIST *ppidl, int cItems, DWORD dwSafetyOptions, CFolder * psdf);


//==================================================================
// Folder items need a way back to the folder object so define folder
// object in header file...

class CFolderItem;
class CFolderItems;

class CFolder : public Folder2,
                public IPersistFolder2,
                public CObjectSafety,
                protected CImpIDispatch,
                public CObjectWithSite
{
    friend class CFolderItem;
    friend class CFolderItems;

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // Folder
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent(IDispatch **ppid);
    STDMETHODIMP get_ParentFolder(Folder **ppdf);

    STDMETHODIMP get_Title(BSTR * pbs);

    STDMETHODIMP Items(FolderItems **ppid);
    STDMETHODIMP ParseName(BSTR bName, FolderItem **ppid);

    STDMETHODIMP NewFolder(BSTR bName, VARIANT vOptions);
    STDMETHODIMP MoveHere(VARIANT vItem, VARIANT vOptions);
    STDMETHODIMP CopyHere(VARIANT vItem, VARIANT vOptions);
    STDMETHODIMP GetDetailsOf(VARIANT vItem, int iColumn, BSTR * pbs);

    // Folder2
    STDMETHODIMP get_Self(FolderItem **ppfi);
    STDMETHODIMP get_OfflineStatus(LONG *pul);
    STDMETHODIMP Synchronize(void);
    STDMETHODIMP get_HaveToShowWebViewBarricade(VARIANT_BOOL *pbHaveToShowWebViewBarricade);
    STDMETHODIMP DismissedWebViewBarricade();

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // CObjectWithSite overriding
    STDMETHODIMP SetSite(IUnknown *punkSite);

    CFolder();
    HRESULT Init(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf);

private:

    LONG m_cRef;
    IShellFolder   *m_psf;
    IShellFolder2  *m_psf2;
    IShellDetails  *m_psd;
    LPITEMIDLIST    m_pidl;
    IDispatch      *m_pidApp;   // Hold onto app object so can release 
    int             _fmt;
    HWND            m_hwnd;

    ~CFolder();

    friend HRESULT InvokeVerbHelper(VARIANT vVerb, VARIANT vArgs, LPCITEMIDLIST *ppidl, int cItems, DWORD dwSafetyOptions, CFolder * psdf);

    // Helper functions, not exported by interface
    STDMETHODIMP _ParentFolder(Folder **ppdf);
    HRESULT _FileOperation(UINT wFunc, VARIANT vItem, VARIANT vOptions);
    IShellDetails *_GetShellDetails(void);
    LPCITEMIDLIST _FolderItemIDList(const VARIANT *pv);
};

class CFolderItemVerbs;

class CFolderItem : public FolderItem2,
                    public IPersistFolder2,
                    public CObjectSafety,
                    protected CImpIDispatch
{
    friend class CFolderItemVerbs;
public:

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // FolderItem
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent(IDispatch **ppid);
    STDMETHODIMP get_Name(BSTR *pbs);
    STDMETHODIMP put_Name(BSTR bs);
    STDMETHODIMP get_Path(BSTR *bs);
    STDMETHODIMP get_GetLink(IDispatch **ppid);
    STDMETHODIMP get_GetFolder(IDispatch **ppid);
    STDMETHODIMP get_IsLink(VARIANT_BOOL * pb);
    STDMETHODIMP get_IsFolder(VARIANT_BOOL * pb);
    STDMETHODIMP get_IsFileSystem(VARIANT_BOOL * pb);
    STDMETHODIMP get_IsBrowsable(VARIANT_BOOL * pb);
    STDMETHODIMP get_ModifyDate(DATE *pdt);
    STDMETHODIMP put_ModifyDate(DATE dt);
    STDMETHODIMP get_Size(LONG *pdt);
    STDMETHODIMP get_Type(BSTR *pbs);
    STDMETHODIMP Verbs(FolderItemVerbs **ppfic);
    STDMETHODIMP InvokeVerb(VARIANT vVerb);

    // FolderItem2
    STDMETHODIMP InvokeVerbEx(VARIANT vVerb, VARIANT vArgs);
    STDMETHODIMP ExtendedProperty(BSTR bstrPropName, VARIANT *pvRet);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // publics, non interface methods
    CFolderItem();
    HRESULT Init(CFolder *psdf, LPCITEMIDLIST pidl);
    static LPCITEMIDLIST _GetIDListFromVariant(const VARIANT *pv);

private:
    HRESULT _CheckAttribute(ULONG ulAttr, VARIANT_BOOL *pb);
    HRESULT _GetUIObjectOf(REFIID riid, void **ppvOut);
    HRESULT _ItemName(UINT dwFlags, BSTR *pbs);
    BOOL    _SecurityVetosRequest();

    ~CFolderItem();

    LONG m_cRef;
    CFolder *m_psdf;             // The folder we came from...
    LPITEMIDLIST m_pidl;
};

#endif // __cplusplus

#endif // __SHDISP_H__
