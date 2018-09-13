/**************************************************************\
    FILE: addrlist.h

    DESCRIPTION:
        This is a class that all Address Lists can inherite
    from.  This will give them the IAddressList interface
    so they can work in the AddressBand/Bar.
\**************************************************************/

#ifndef _ADDRLIST_H
#define _ADDRLIST_H

#include "shellurl.h"
#define ACP_LIST_MAX_CONST            25

/**************************************************************\
    CLASS: CAddressList

    DESCRIPTION:
        This is a class that all Address Lists can inherite
    from.  This will give them the IAddressList interface
    so they can work in the AddressBand/Bar.

    NOTE:
        This is a virtual class!
\**************************************************************/

class CAddressList 
                : public IAddressList   // (Includes IWinEventHandler)
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plre);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd) { return E_NOTIMPL; }

    // *** IAddressList methods ***
    virtual STDMETHODIMP Connect(BOOL fConnect, HWND hwnd, IBrowserService * pbs, IBandProxy * pbp, IAutoComplete * pac);
    virtual STDMETHODIMP NavigationComplete(LPVOID pvCShellUrl);
    virtual STDMETHODIMP Refresh(DWORD dwType) { return S_OK; }      // Force subclasses to handle.
    virtual STDMETHODIMP Load(void) {return E_NOTIMPL;}
    virtual STDMETHODIMP Save(void) {return E_NOTIMPL;}
    virtual STDMETHODIMP SetToListIndex(int nIndex, LPVOID pvShelLUrl);
    virtual STDMETHODIMP FileSysChangeAL(DWORD dw, LPCITEMIDLIST* ppidl);

protected:
    //////////////////////////////////////////////////////
    // Private Member Functions
    //////////////////////////////////////////////////////

    // Constructor / Destructor
    CAddressList();
    virtual ~CAddressList(void);        // This is now an OLE Object and cannot be used as a normal Class.


    // Address Band Specific Functions
    virtual LRESULT _OnNotify(LPNMHDR pnm);
    virtual LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void _InitCombobox(void);
    virtual HRESULT _Populate(void) = 0;        // This is a PURE function.
    virtual void _PurgeComboBox();

    // Helper Functions 
    void _ComboBoxInsertURL(LPCTSTR pszURL, int cchStrSize, int nMaxComboBoxSize);
    BOOL _MoveAddressToTopOfList(int iSel);
    HRESULT _GetUrlUI(CShellUrl * psu, LPCTSTR szUrl, int *piImage, int *piImageSelected);
    HRESULT _GetFastPathIcons(LPCTSTR pszPath, int *piImage, int *piSelectedImage);
    HRESULT _GetPidlIcon(LPCITEMIDLIST pidl, int *piImage, int *piSelectedImage);
    virtual LPITEMIDLIST _GetDragDropPidl(LPNMCBEDRAGBEGINW pnmcbe);
    LRESULT  _OnDragBeginW(LPNMCBEDRAGBEGINW pnmcbe);
    LRESULT  _OnDragBeginA(LPNMCBEDRAGBEGINA pnmcbe) ;
    HRESULT _SetPreferedDropEffect(IDataObject *pdtobj, DWORD dwEffect);

    //////////////////////////////////////////////////////
    //  Private Member Variables 
    //////////////////////////////////////////////////////
    int                 _cRef;
    BOOL                _fVisible:1;        // TRUE when the toolbar is visible.
    HWND                _hwnd;              // The window
    IBrowserService *   _pbs;
    IBandProxy *        _pbp;
    CShellUrl *         _pshuUrl;
};



HRESULT GetCBListIndex(HWND hwnd, int iItem, LPTSTR szAddress, int cchAddressSize);


IAddressList * CSNSList_Create(void);
IAddressList * CMRUList_Create(void);
IAddressList * CACPList_Create(void);

//===========================================================================
// IMRU: Interface to CMRUList.  Note that this interface never leaves browseui

#undef INTERFACE
#define INTERFACE IMRU
DECLARE_INTERFACE_(IMRU, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IMRU Methods ***
    STDMETHOD(AddEntry) (THIS_ LPCWSTR pszEntry) PURE;
};

#endif // _ADDRLIST_H
