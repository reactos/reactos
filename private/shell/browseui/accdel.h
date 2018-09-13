#ifndef _ACCDEL_HPP_
#define _ACCDEL_HPP_

// BUGBUG (lamadio): Conflicts with one defined in winuserp.h
#undef WINEVENT_VALID       //It's tripping on this...
#include "winable.h"
#include "oleacc.h"

class CDelegateAccessibleImpl : public IAccessible
{
public:
    CDelegateAccessibleImpl();

    // *** IDispatch ***
    STDMETHODIMP GetTypeInfoCount(
        UINT* pctinfo);
        
    STDMETHODIMP GetTypeInfo(
        UINT itinfo, LCID lcid, ITypeInfo** pptinfo);

    STDMETHODIMP GetIDsOfNames(
        REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);

    STDMETHODIMP Invoke(
        DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
        UINT* puArgErr);

    // *** IAccessible ***
    STDMETHODIMP get_accParent( 
        IDispatch  **ppdispParent);
    
    STDMETHODIMP get_accChildCount( 
        long  *pcountChildren);
    
    STDMETHODIMP get_accChild( 
        VARIANT varChild,
        IDispatch  **ppdispChild);
    
    STDMETHODIMP get_accName( 
        VARIANT varChild,
        BSTR  *pszName);
    
    STDMETHODIMP get_accValue( 
        VARIANT varChild,
        BSTR  *pszValue);
    
    STDMETHODIMP get_accDescription( 
        VARIANT varChild,
        BSTR  *pszDescription);
        
    STDMETHODIMP get_accRole( 
        VARIANT varChild,
        VARIANT  *pvarRole);
    
    STDMETHODIMP get_accState( 
        VARIANT varChild,
        VARIANT  *pvarState);
    
    STDMETHODIMP get_accHelp( 
        VARIANT varChild,
        BSTR  *pszHelp);
    
    STDMETHODIMP get_accHelpTopic( 
        BSTR  *pszHelpFile,
        VARIANT varChild,
        long  *pidTopic);
        
    STDMETHODIMP get_accKeyboardShortcut( 
        VARIANT varChild,
        BSTR  *pszKeyboardShortcut);
    
    STDMETHODIMP get_accFocus( 
        VARIANT  *pvarChild);
    
    STDMETHODIMP get_accSelection( 
        VARIANT  *pvarChildren);
    
    STDMETHODIMP get_accDefaultAction( 
        VARIANT varChild,
        BSTR  *pszDefaultAction);
    
    STDMETHODIMP accSelect( 
        long flagsSelect,
        VARIANT varChild);
    
    STDMETHODIMP accLocation( 
        long  *pxLeft,
        long  *pyTop,
        long  *pcxWidth,
        long  *pcyHeight,
        VARIANT varChild);
    
    STDMETHODIMP accNavigate( 
        long navDir,
        VARIANT varStart,
        VARIANT  *pvarEndUpAt);
    
    STDMETHODIMP accHitTest( 
        long xLeft,
        long yTop,
        VARIANT  *pvarChild);
    
    STDMETHODIMP accDoDefaultAction( 
        VARIANT varChild);
    
    STDMETHODIMP put_accName( 
        VARIANT varChild,
        BSTR szName);
    
    STDMETHODIMP put_accValue( 
        VARIANT varChild,
        BSTR szValue);

protected:
    IAccessible     *m_pDelegateAccObj;

    HRESULT _DefQueryInterface(REFIID riid, void **ppvObj);
    virtual ~CDelegateAccessibleImpl();
};



#endif // _ACCDEL_HPP_

