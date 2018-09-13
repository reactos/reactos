#include "priv.h"
#include "accdel.h"

CDelegateAccessibleImpl::CDelegateAccessibleImpl()
{
    ASSERT(NULL == m_pDelegateAccObj);
}

CDelegateAccessibleImpl::~CDelegateAccessibleImpl()
{ 
    if (NULL != m_pDelegateAccObj)
    {
        m_pDelegateAccObj->Release();
    }
}

HRESULT CDelegateAccessibleImpl::_DefQueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAccessible) ||
        IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = SAFECAST(this, IAccessible*);
        AddRef();
    }
    else if ((NULL != m_pDelegateAccObj) && IsEqualIID(riid, IID_IEnumVARIANT))
    {
        //  Yikes! breaking COM identity rules -- trident does it, just following their lead here :)
        m_pDelegateAccObj->QueryInterface(IID_IEnumVARIANT, ppvObj);
    }

    return (NULL == *ppvObj) ? E_NOINTERFACE : S_OK;
}

// *** IDispatch ***
STDMETHODIMP CDelegateAccessibleImpl::GetTypeInfoCount(
    UINT* pctinfo)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->GetTypeInfoCount(pctinfo);
    }
    else
    {
        return E_UNEXPECTED;
    }
}
    
STDMETHODIMP CDelegateAccessibleImpl::GetTypeInfo(
    UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->GetTypeInfo(itinfo, lcid, pptinfo);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::GetIDsOfNames(
    REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    }
    else
    {
        return E_UNEXPECTED;
    }
}


STDMETHODIMP CDelegateAccessibleImpl::Invoke(
    DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
    DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
    UINT* puArgErr)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->Invoke(dispidMember, riid, lcid, wFlags,
                                 pdispparams, pvarResult, pexcepinfo, puArgErr);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

// *** IAccessible ***
STDMETHODIMP CDelegateAccessibleImpl::get_accParent( 
    IDispatch  **ppdispParent)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accParent(ppdispParent);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accChildCount( 
    long  *pcountChildren)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accChildCount(pcountChildren);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accChild( 
    VARIANT varChild,
    IDispatch  **ppdispChild)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accChild(varChild, ppdispChild);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accName( 
    VARIANT varChild,
    BSTR  *pszName)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accName(varChild, pszName);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accValue( 
    VARIANT varChild,
    BSTR  *pszValue)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accValue(varChild, pszValue);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accDescription( 
    VARIANT varChild,
    BSTR  *pszDescription)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accDescription(varChild, pszDescription);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accRole( 
    VARIANT varChild,
    VARIANT  *pvarRole)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accRole(varChild, pvarRole);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accState( 
    VARIANT varChild,
    VARIANT  *pvarState)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accState(varChild, pvarState);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accHelp( 
    VARIANT varChild,
    BSTR  *pszHelp)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accHelp(varChild, pszHelp);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accHelpTopic( 
    BSTR  *pszHelpFile,
    VARIANT varChild,
    long  *pidTopic)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accHelpTopic(pszHelpFile, varChild, pidTopic);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accKeyboardShortcut( 
    VARIANT varChild,
    BSTR  *pszKeyboardShortcut)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accKeyboardShortcut(varChild, pszKeyboardShortcut);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accFocus( 
    VARIANT  *pvarChild)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accFocus(pvarChild);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accSelection( 
    VARIANT  *pvarChildren)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accSelection(pvarChildren);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::get_accDefaultAction( 
    VARIANT varChild,
    BSTR  *pszDefaultAction)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->get_accDefaultAction(varChild, pszDefaultAction);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::accSelect( 
    long flagsSelect,
    VARIANT varChild)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->accSelect(flagsSelect, varChild);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::accLocation( 
    long  *pxLeft,
    long  *pyTop,
    long  *pcxWidth,
    long  *pcyHeight,
    VARIANT varChild)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::accNavigate( 
    long navDir,
    VARIANT varStart,
    VARIANT  *pvarEndUpAt)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->accNavigate(navDir, varStart, pvarEndUpAt);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::accHitTest( 
    long xLeft,
    long yTop,
    VARIANT  *pvarChild)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->accHitTest(xLeft, yTop, pvarChild);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::accDoDefaultAction( 
    VARIANT varChild)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->accDoDefaultAction(varChild);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::put_accName( 
    VARIANT varChild,
    BSTR szName)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->put_accName(varChild, szName);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CDelegateAccessibleImpl::put_accValue( 
    VARIANT varChild,
    BSTR szValue)
{
    if (NULL != m_pDelegateAccObj)
    {
        return m_pDelegateAccObj->put_accValue(varChild, szValue);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

