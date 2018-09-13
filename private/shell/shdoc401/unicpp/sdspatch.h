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

class CSDFolder;

HRESULT GetApplicationObject(DWORD dwSafetyOptions, IUnknown *punkSite, IDispatch **ppid);

HRESULT CSDFldrItems_Create( CSDFolder *psdf, BOOL fSelected, FolderItems ** ppid);
HRESULT CSDFldrItem_Create(CSDFolder *psdf, LPITEMIDLIST pidl, FolderItem ** ppid);
HRESULT CSDShellLink_CreateIDispatch(HWND hwndFldr, LPSHELLFOLDER psfFldr, LPITEMIDLIST pidl, IDispatch ** ppid);
HRESULT CSDWindow_Create(HWND hwndFldr, IDispatch ** ppsw);
HRESULT CSDFolder_Create(HWND hwnd, LPITEMIDLIST pidl, IShellFolder *psf, CSDFolder **ppsf);

//==================================================================
// Internal interface to get pidls from different classes...
#undef  INTERFACE
#define INTERFACE   ISDGetPidl

DECLARE_INTERFACE_(ISDGetPidl, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** ISDGetPidl specific methods
    STDMETHOD(GetPidl)(THIS_ LPITEMIDLIST * ppidl) PURE;
};

//==================================================================
// Folder items need a way back to the folder object so define folder
// object in header file...

class CSDFldrItem;
class CSDFldrItems;
class CSDFolder : public Folder,
                  public ISDGetPidl,
                  public CObjectSafety,
                  protected CImpIDispatch,
                  public CObjectWithSite
{
    public:
        ULONG           m_cRef; //Public for debug checks

    protected:
        friend class CSDFldrItem;
        friend class CSDFldrItems;
        LPSHELLFOLDER   m_psf;
        IShellDetails  *m_psd;
        LPITEMIDLIST    m_pidl;
        IDispatch       *m_pidApp;   // Hold onto app object so can release 
        HWND            m_hwnd;     // Normally NULL when walking name space

    public:
        CSDFolder(HWND hwnd, IShellFolder *psf = NULL);
        ~CSDFolder(void);

        BOOL         Init(LPITEMIDLIST pidl);

        //IUnknown methods
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);


        //IDispatch members
        virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
            { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
        virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
            { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
        virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
            { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
        virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
            { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

        //Folder methods
        STDMETHODIMP get_Application(IDispatch **ppid);
        STDMETHODIMP get_Parent (IDispatch **ppid);
        STDMETHODIMP get_ParentFolder (Folder **ppdf);

        STDMETHODIMP get_Title (BSTR * pbs);

        STDMETHODIMP Items (FolderItems **ppid);
        STDMETHODIMP ParseName (BSTR bName, FolderItem **ppid);

        STDMETHODIMP NewFolder (BSTR bName, VARIANT vOptions);
        STDMETHODIMP MoveHere (VARIANT vItem, VARIANT vOptions);
        STDMETHODIMP CopyHere (VARIANT vItem, VARIANT vOptions);
        STDMETHODIMP GetDetailsOf (VARIANT vItem, int iColumn, BSTR * pbs);

        // ISDGetPidl methods
        STDMETHODIMP GetPidl (LPITEMIDLIST * ppidl);

        // Helper functions, not exported by interface, used by Folder View class...
        LPSHELLFOLDER _GetShellFolder(void);
        IShellDetails* _GetShellDetails(void);
        HWND _GetHwnd(void) {return m_hwnd;};

        // CObjectWithSite overriding
        virtual STDMETHODIMP SetSite(IUnknown *punkSite);

    protected:
        HRESULT _FileOperation(UINT wFunc, VARIANT vItem, VARIANT vOptions);
};

//==================================================================
// Folder Item Class definitions
class CSDFolderItemVerbs;

class CSDFldrItem : public FolderItem,
                    public ISDGetPidl,
                    public CObjectSafety,
                    protected CImpIDispatch
{
    public:
        ULONG           m_cRef; //Public for debug checks

    protected:
        friend class CSDFolderItemVerbs;

        CSDFolder       *m_psdf;             // The folder we came from...
        LPITEMIDLIST    m_pidl;

    public:
        CSDFldrItem(CSDFolder *psdf);
        ~CSDFldrItem(void);

        BOOL         Init(LPITEMIDLIST pidl);

        //IUnknown methods
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDispatch members
        virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
            { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
        virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
            { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
        virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
            { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
        virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
            { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

        //FolderItem methods
        STDMETHODIMP get_Application(IDispatch **ppid);
        STDMETHODIMP get_Parent (IDispatch **ppid);
        STDMETHODIMP get_Name (BSTR *pbs);
        STDMETHODIMP put_Name (BSTR bs);
        STDMETHODIMP get_Path (BSTR *bs);
        STDMETHODIMP get_GetLink (IDispatch **ppid);
        STDMETHODIMP get_GetFolder ( IDispatch **ppid);
        STDMETHODIMP get_IsLink ( VARIANT_BOOL * pb);
        STDMETHODIMP get_IsFolder ( VARIANT_BOOL * pb);
        STDMETHODIMP get_IsFileSystem ( VARIANT_BOOL * pb);
        STDMETHODIMP get_IsBrowsable ( VARIANT_BOOL * pb);
        STDMETHODIMP get_ModifyDate ( DATE *pdt);
        STDMETHODIMP put_ModifyDate ( DATE dt);
        STDMETHODIMP get_Size ( LONG *pdt);
        STDMETHODIMP get_Type ( BSTR *pbs);
        STDMETHODIMP Verbs (FolderItemVerbs **ppfic);
        STDMETHODIMP InvokeVerb (VARIANT vVerb);

        // ISDGetPidl methods
        STDMETHODIMP GetPidl (LPITEMIDLIST * ppidl);

        // Helper functions used internally...
        LPCITEMIDLIST Pidl(void) {return (LPCITEMIDLIST)m_pidl;}

    protected:
        HRESULT _CheckAttribute(ULONG ulAttr, VARIANT_BOOL *pb);
        HRESULT _GetUIObjectOf(REFIID riid, LPVOID * ppvOut);
};

// IID to verify we have our own CSDFolderItem...
EXTERN_C const GUID IID_ICSDFolderItem;   // {AAE84A70-40DB-11d0-94EB-00A0C91F3880}
EXTERN_C const GUID IID_ISDGetPidl;       // {C066D4E0-6400-11d0-9525-00A0C91F3880}

#endif // __cplusplus

#endif // __SHDISP_H__
