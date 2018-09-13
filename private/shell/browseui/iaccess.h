#ifndef _IAccessible_h
#define _IAccessible_h

#include "oleacc.h"
#include "mnbase.h"
#include "menuband.h"

#define CHILDID_SELF 0

#define MB_STATE_TRACK 1
#define MB_STATE_MENU  2
#define MB_STATE_ITEM  4

#define TOOLBAR_MASK 0x80000000

// BUGBUG (lamadio): The designers of the Accessibility interface did not know
// the rule about COM identity. They allow a QI for the external object IEnumVariant

class CAccessible : public IAccessible, public IEnumVARIANT, public IOleWindow
{
    int             _cRef;
    // IDispatch Support
    ITypeInfo*      _pTypeInfo;
    BOOL            _LoadTypeLib();


    // Track menu popup Support
    IAccessible*    _pInnerAcc;
    HWND            _hwndMenuWindow;
    HMENU           _hMenu;
    WORD            _wID;

    // Menuband Support
    CMenuToolbarBase* _pmtbBottom;
    CMenuToolbarBase* _pmtbTop;
    IShellMenuAcc*    _psma;
    IMenuBand*        _pmb;

    // Menuband Item Support
    CMenuToolbarBase*  _pmtbItem;

    int               _iAccIndex;
    int               _iIndex;  
    int               _iEnumIndex;
    int               _idCmd;


    // Object info
    BITBOOL         _fInitialized: 1;
    BITBOOL         _fState: 3;

    HRESULT _GetVariantFromChildIndex(HWND hwnd, int iIndex, VARIANT* pvarChild);
    HRESULT _GetChildFromVariant(VARIANT* pvarChild, CMenuToolbarBase** ppmtb, int* iIndex);
    HRESULT _GetAccessibleItem(int iIndex, IDispatch** ppdisp);
    HRESULT _GetAccName(BSTR* pbstr);
    HRESULT _Navigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);


public:
    CAccessible(HMENU, WORD);
    CAccessible(IMenuBand*);
    CAccessible(IMenuBand*, int iIndex);
    virtual ~CAccessible();
    HRESULT InitAcc();

    // *** IUnknown methods ***
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT FAR* pctinfo);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames,
        LCID lcid, DISPID FAR* rgdispid);
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,
        UINT FAR* puArgErr);


    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);


    // *** IAccessible methods ***
    virtual STDMETHODIMP get_accParent(IDispatch * FAR* ppdispParent);
    virtual STDMETHODIMP get_accChildCount(long FAR* pChildCount);
    virtual STDMETHODIMP get_accChild(VARIANT varChildIndex, IDispatch * FAR* ppdispChild);

    virtual STDMETHODIMP get_accName(VARIANT varChild, BSTR* pszName);
    virtual STDMETHODIMP get_accValue(VARIANT varChild, BSTR* pszValue);
    virtual STDMETHODIMP get_accDescription(VARIANT varChild, BSTR FAR* pszDescription);
    virtual STDMETHODIMP get_accRole(VARIANT varChild, VARIANT *pvarRole);
    virtual STDMETHODIMP get_accState(VARIANT varChild, VARIANT *pvarState);

    
    virtual STDMETHODIMP get_accHelp(VARIANT varChild, BSTR* pszHelp);
    virtual STDMETHODIMP get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic);
    virtual STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut);

    virtual STDMETHODIMP get_accFocus(VARIANT FAR * pvarFocusChild);

    virtual STDMETHODIMP get_accSelection(VARIANT FAR * pvarSelectedChildren);
    
    virtual STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);

    virtual STDMETHODIMP accSelect(long flagsSelect, VARIANT varChild);

    virtual STDMETHODIMP accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);

    virtual STDMETHODIMP accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);
    virtual STDMETHODIMP accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);

    virtual STDMETHODIMP accDoDefaultAction(VARIANT varChild);

    virtual STDMETHODIMP put_accName(VARIANT varChild, BSTR szName);
    virtual STDMETHODIMP put_accValue(VARIANT varChild, BSTR pszValue);


    // *** IEnumVARIANT methods ***
    virtual STDMETHODIMP Next(unsigned long celt, 
                            VARIANT FAR* rgvar, 
                            unsigned long FAR* pceltFetched); 
    virtual STDMETHODIMP Skip(unsigned long celt); 
    virtual STDMETHODIMP Reset(); 
    virtual STDMETHODIMP Clone(IEnumVARIANT FAR* FAR* ppenum); 
};

extern "C"
{
    LRESULT LresultFromObject(REFIID riid, WPARAM wParam, IUnknown*);
    void WINAPI NotifyWinEvent(DWORD event, HWND hwnd,
        LONG idObject, LONG idChild);
}


#endif
