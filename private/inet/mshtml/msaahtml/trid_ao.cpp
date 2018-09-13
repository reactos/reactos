//================================================================================
//      File:   TRID_AO.CPP
//      Date:   5/21/97
//      Desc:   contains implementation of CTridentAO class.  CTridentAO 
//              is the base class for all accessible Trident objects.
//================================================================================

//=======================================================================
// Includes
//=======================================================================

#include "stdafx.h"
#include "trid_ae.h"
#include "trid_ao.h"
#include "aommgr.h"
#include "document.h"
#include "enumvar.h"
#include "text.h"
#include "window.h"
#include "prxymgr.h"
#include "anchor.h"


#define FAIL_IF_DETACHED if (!m_pAOContainer || \
                             m_pAOContainer->DoBlockForDetach())\
                                return E_FAIL;

//================================================================================
// Defines
//================================================================================

#define CACHE_BSTR_DISPLAY      0x00000001
#define CACHE_BSTR_STYLE        0x00000002

//================================================================================
// Constants
//================================================================================

static const WCHAR wszKeyboardShortcutPrefix[]  = L"Alt+";

//================================================================================
// Externally declared variables
//================================================================================

extern  CProxyManager   *g_pProxyMgr;
    
//================================================================================
// CImplIAccessible class definition
//================================================================================

class CImplIAccessible : public IAccessible
{
public:

    //--------------------------------------------------
    // IUnknown
    //--------------------------------------------------
    
    virtual STDMETHODIMP        QueryInterface(REFIID riid, void** ppv);
    virtual STDMETHODIMP_(ULONG)    AddRef(void);
    virtual STDMETHODIMP_(ULONG)    Release(void);
    
    //--------------------------------------------------
    // IDispatch
    //--------------------------------------------------
    
    virtual STDMETHODIMP    GetTypeInfoCount(UINT* pctinfo);
    virtual STDMETHODIMP    GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual STDMETHODIMP    GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT                    cNames, LCID lcid, DISPID* rgdispid);
    virtual STDMETHODIMP    Invoke(DISPID dispidMember, REFIID riid, LCID lcid,WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);
    
    //--------------------------------------------------
    // IAccessible
    //--------------------------------------------------

    virtual STDMETHODIMP    get_accParent(IDispatch ** ppdispParent);
    virtual STDMETHODIMP    get_accChildCount(long* pChildCount);
    virtual STDMETHODIMP    get_accChild(VARIANT varChild, IDispatch ** ppdispChild);
    virtual STDMETHODIMP    get_accName(VARIANT varChild, BSTR* pbstrName);
    virtual STDMETHODIMP    get_accValue(VARIANT varChild, BSTR* pbstrValue);
    virtual STDMETHODIMP    get_accDescription(VARIANT varChild, BSTR* pbstrDescription);
    virtual STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT *pvarRole);
    virtual STDMETHODIMP    get_accState(VARIANT varChild, VARIANT *pvarState); 
    virtual STDMETHODIMP    get_accHelp(VARIANT varChild, BSTR* pbstrHelp);
    virtual STDMETHODIMP    get_accHelpTopic(BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic);
    virtual STDMETHODIMP    get_accKeyboardShortcut(VARIANT varChild, BSTR* pbstrKeyboardShortcut);
    virtual STDMETHODIMP    get_accFocus(VARIANT * pvarFocusChild);
    virtual STDMETHODIMP    get_accSelection(VARIANT * pvarSelectedChildren);
    virtual STDMETHODIMP    get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction);
    virtual STDMETHODIMP    accSelect(long flagsSel, VARIANT varChild);
    virtual STDMETHODIMP    accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
    virtual STDMETHODIMP    accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);
    virtual STDMETHODIMP    accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);
    virtual STDMETHODIMP    accDoDefaultAction(VARIANT varChild);
    virtual STDMETHODIMP    put_accName(VARIANT varChild, BSTR bstrName);
    virtual STDMETHODIMP    put_accValue(VARIANT varChild, BSTR pbstrValue);

    //--------------------------------------------------
    // CImplIAccessible creation/destruction/maintenance 
    //--------------------------------------------------
    
    CImplIAccessible(IUnknown * pIUnknown,CTridentAO * pAOContainer);
    ~CImplIAccessible();

    protected:

    //--------------------------------------------------
    // methods
    //--------------------------------------------------

    HRESULT validateVariant(VARIANT * pVar);
    HRESULT packReturnVariant(IUnknown * pIUnknown,VARIANT *pvarToPack);
    HRESULT getTypeInfo( ITypeInfo** ppITypeInfo );
    HRESULT prefixKeyboardShortcut( BSTR* pbstrKS );

    //------------------------------------------------
    // Members
    //------------------------------------------------

    IUnknown    * m_pIUnknown;
    CTridentAO  * m_pAOContainer;
};


//================================================================================
// CImplIOleWindow class definition
//================================================================================

class CImplIOleWindow : public IOleWindow
{
public :
    //------------------------------------------------
    // IUnknown
    //------------------------------------------------

    virtual STDMETHODIMP        QueryInterface(REFIID riid, void** ppv);
    virtual STDMETHODIMP_(ULONG)    AddRef(void);
    virtual STDMETHODIMP_(ULONG)    Release(void);

    //--------------------------------------------------
    // IOleWindow
    //--------------------------------------------------
    
    virtual STDMETHODIMP    GetWindow(HWND* phwnd);
    virtual STDMETHODIMP    ContextSensitiveHelp(BOOL fEnterMode);
    
    //--------------------------------------------------
    // CImplIOleWindow creation/destruction/maintenance 
    //--------------------------------------------------
    
    CImplIOleWindow(IUnknown * pIUnknown,CTridentAO * pAOContainer);
    ~CImplIOleWindow();

protected:
    //------------------------------------------------
    // Members
    //------------------------------------------------
    IUnknown    * m_pIUnknown;
    CTridentAO  * m_pAOContainer;
};


//=======================================================================
// CImplIAccessible class implementation : public methods
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIAccessible::CImplIAccessible()
//
//  DESCRIPTION:
//
//      CTridentAE class constructor.
//
//  PARAMETERS:
//
//      pIUnknown   pointer to owner IUnknown.
//      pAOContainer        pointer to owner object
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CImplIAccessible::CImplIAccessible(IUnknown * pIUnknown,CTridentAO *pAOContainer)
{
    //------------------------------------------------
    // assign controlling IUnknown;
    //------------------------------------------------
    
    m_pIUnknown     = pIUnknown;
    
    //------------------------------------------------
    // assign parent pointer : even though this is 
    // the same value as the m_pIUnknown pointer, it
    // makes more sense to access parent functionality
    // through a non OLE pointer.
    //------------------------------------------------

    m_pAOContainer      = pAOContainer;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::~CImplIAccessible()
//
//  DESCRIPTION:
//
//      CImplIAccessible class destructor.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CImplIAccessible::~CImplIAccessible()
{

}


//=======================================================================
// CImplIAccessible : IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIAccessible::QueryInterface()
//
//  DESCRIPTION:
//
//  only implements IAccessible interface.
//
//  PARAMETERS:
//
//      riid        REFIID of requested interface.
//      ppv         pointer to interface in.
//
//  RETURNS:
//
//      E_NOINTERFACE | NOERROR.
//
// ----------------------------------------------------------------------

STDMETHODIMP CImplIAccessible::QueryInterface(REFIID riid, void** ppv)
{
    if ( !ppv )
        return E_INVALIDARG;


    *ppv = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // this class only knows about the IAccessible
    // interface, so if another interface is desired,
    // delegate to owner
    //------------------------------------------------

    if (riid == IID_IAccessible )
    {
        *ppv = (IAccessible *)this;
        ((IAccessible *)*ppv)->AddRef();
        return NOERROR;
    }
    else
        return m_pIUnknown->QueryInterface(riid,ppv);
}


//-----------------------------------------------------------------------
//  CImplIAccessible::AddRef()
//
//  DESCRIPTION:
//
//  Handled in owner class
//
//  PARAMETERS:
//
//
//  RETURNS:
//
//  reference count AFTER increment.
//
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImplIAccessible::AddRef( void )
{
    //------------------------------------------------
    // delegate ref counting to owner
    //------------------------------------------------

    return m_pIUnknown->AddRef();
}

//-----------------------------------------------------------------------
//  CImplIAccessible::Release()
//
//  DESCRIPTION:
//
//  Handled in owner class.
//
//  PARAMETERS:
//
//
//  RETURNS:
//
//  reference count AFTER decrement.
//
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImplIAccessible::Release( void )
{
    //------------------------------------------------
    // delegate ref counting to owner
    //------------------------------------------------

    return m_pIUnknown->Release();
}


//=======================================================================
// CImplIAccessible : IDispatch interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIAccessible::GetTypeInfoCount()
//
//  DESCRIPTION:
//
//      Implements the IDispatch interface method GetTypeInfoCount().
//
//      Retrieves the number of type information interfaces that an
//      object provides (either 0 or 1).
//
//  PARAMETERS:
//
//      pctInfo     [out] Points to location that receives the
//                          number of type information interfaces
//                          that the object provides. If the object
//                          provides type information, this number
//                          is set to 1; otherwise it's set to 0.
//
//  RETURNS:
//
//      HRESULT           S_OK if the function succeeds or 
//                          E_INVALIDARG if pctInfo is invalid.
//
//-----------------------------------------------------------------------

STDMETHODIMP CImplIAccessible::GetTypeInfoCount( UINT *pctInfo )
{
    if ( !pctInfo )
        return E_INVALIDARG;

    *pctInfo = 1;

    return S_OK;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::GetTypeInfo()
//
//  DESCRIPTION:
//
//      Implements the IDispatch interface method GetTypeInfo().
//
//      Retrieves a type information object, which can be used to
//      get the type information for an interface.
//
//  PARAMETERS:
//
//      itinfo      [in]  The type information to return. If this value
//                          is 0, the type information for the IDispatch
//                          implementation is to be retrieved.
//
//      lcid        [in]  The locale ID for the type information.
//
//      ppITypeInfo [out] Receives a pointer to the type information
//                          object requested.
//
//  RETURNS:
//
//      HRESULT           S_OK if the function succeeded (the TypeInfo
//                          element exists), TYPE_E_ELEMENTNOTFOUND if
//                          itinfo is not equal to zero, or 
//                          E_INVALIDARG if ppITypeInfo is invalid.
//
//-----------------------------------------------------------------------

STDMETHODIMP CImplIAccessible::GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** ppITypeInfo )
{
    if ( !ppITypeInfo )
        return E_INVALIDARG;

    if ( itinfo != 0 )
        return TYPE_E_ELEMENTNOTFOUND;

    *ppITypeInfo = NULL;

    return getTypeInfo( ppITypeInfo );
}


//-----------------------------------------------------------------------
//  CImplIAccessible::GetIDsOfNames()
//
//  DESCRIPTION:
//
//      Implements the IDispatch interface method GetIDsOfNames().
//
//      Maps a single member and an optional set of argument names
//      to a corresponding set of integer DISPIDs, which may be used
//      on subsequent calls to IDispatch::Invoke.
//
//  PARAMETERS:
//
//      riid        [in]  Reserved for future use. Must be NULL.
//
//      rgszNames   [in]  Passed-in array of names to be mapped.
//
//      cNames      [in]  Count of the names to be mapped.
//
//      lcid        [in]  The locale context in which to interpret
//                          the names.
//
//      rgdispid    [out] Caller-allocated array, each element of
//                          which contains an ID corresponding to
//                          one of the names passed in the rgszNames
//                          array.  The first element represents the
//                          member name; the subsequent elements
//                          represent each of the member's parameters.
//
//  RETURNS:
//
//      HRESULT           S_OK if the function succeeded,
//                          E_OUTOFMEMORY if there is not enough
//                          memory to complete the call,
//                          DISP_E_UNKNOWNNAME if one or more of
//                          the names were not known, or
//                          DISP_E_UNKNOWNLCID if the LCID was
//                          not recognized.
//
//  NOTES:
//
//      This method simply delegates the call to
//      ITypeInfo::GetIDsOfNames().
//-----------------------------------------------------------------------

STDMETHODIMP CImplIAccessible::GetIDsOfNames( REFIID riid, OLECHAR ** rgszNames, UINT cNames,
                                        LCID lcid, DISPID * rgdispid )
{
    HRESULT     hr;
    ITypeInfo*  pITypeInfo = NULL;

    FAIL_IF_DETACHED;

    hr = getTypeInfo( &pITypeInfo );

    if ( hr != S_OK )
        return hr;

    assert( pITypeInfo );

    hr = pITypeInfo->GetIDsOfNames( rgszNames, cNames, rgdispid );

    pITypeInfo->Release();

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::Invoke()
//
//  DESCRIPTION:
//
//      Implements the IDispatch interface method Invoke().
//
//      Provides access to properties and methods exposed by the
//      Accessible object.
//
//  PARAMETERS:
//
//      dispidMember    [in]  Identifies the dispatch member.
//
//      riid            [in]  Reserved for future use. Must be NULL.
//
//      lcid            [in]  The locale context in which to interpret
//                              the names.
//
//      wFlags          [in]  Flags describing the context of the
//                                  Invoke call.
//
//      pdispparams     [in,] Pointer to a structure containing an
//                      [out]   array of arguments, array of argument
//                              dispatch IDs for named arguments, and
//                              counts for number of elements in the
//                              arrays.
//
//      pvarResult      [in,] Pointer to where the result is to be
//                      [out]   stored, or NULL if the caller expects
//                              no result.  This argument is ignored
//                              if DISPATCH_PROPERTYPUT or
//                              DISPATCH_PROPERTYPUTREF is specified.
//
//      pexcepinfo      [out] Pointer to a structure containing
//                              exception information.  This structure
//                              should be filled in if DISP_E_EXCEPTION
//                              is returned.
//
//      puArgErr        [out] The index within rgvarg of the first
//                              argument that has an error.  Arguments
//                              are stored in pdispparams->rgvarg in
//                              reverse order, so the first argument
//                              is the one with the highest index in
//                              the array.
//
//  RETURNS:
//
//      HRESULT           S_OK on success, dispatch error (DISP_E_*)
//                          or E_NOTIMPL otherwise.
//
//  NOTES:
//
//      This method simply delegates the call to ITypeInfo::Invoke().
//-----------------------------------------------------------------------

STDMETHODIMP CImplIAccessible::Invoke( DISPID dispid,
                                 REFIID riid,
                                 LCID lcid,
                                 WORD wFlags,
                                 DISPPARAMS * pdispparams,
                                 VARIANT *pvarResult,
                                 EXCEPINFO *pexcepinfo,
                                 UINT *puArgErr )
{
    HRESULT     hr;
    ITypeInfo*  pITypeInfo = NULL;

    FAIL_IF_DETACHED;

    hr = getTypeInfo( &pITypeInfo );

    if ( hr != S_OK )
        return hr;

    assert( pITypeInfo );

    hr = pITypeInfo->Invoke( (IAccessible *)this,
                                dispid,
                                wFlags,
                                pdispparams,
                                pvarResult,
                                pexcepinfo,
                                puArgErr );

    pITypeInfo->Release();

    return hr;
}


//=======================================================================
// CImplIAccessible : IAccessible interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIAccessible::get_accParent()
//
//  DESCRIPTION:
//
//      get parent object.
//
//  PARAMETERS:
//
//      ppdispParent    IDispatch pointer to return to client.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG | DISP_E_MEMBERNOTFOUND | 

//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accParent(IDispatch ** ppdispParent)
{
    if ( !ppdispParent )
        return E_INVALIDARG;

    *ppdispParent = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // call parent helper method (override helper
    // method for customized handling)
    //------------------------------------------------

    return m_pAOContainer->GetAccParent( ppdispParent );
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accChildCount()
//
//  DESCRIPTION:
//
//      get count of children
//
//  PARAMETERS:
//
//      plChildCount        pointer to long to return to client w/
//                          child count.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG | DISP_E_MEMBERNOTFOUND | S_FALSE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accChildCount(long * plChildCount)
{
    if ( !plChildCount )
        return E_INVALIDARG;

    *plChildCount = 0;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // call parent helper method (override helper
    // method for customized handling)
    //------------------------------------------------

    return m_pAOContainer->GetAccChildCount( plChildCount );
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accChild()
//
//  DESCRIPTION:
//
//      get IDispatch pointer of specific child
//
//  PARAMETERS:
//
//      varChild            contains specified child object.
//      ppdispChild         pointer to IDispatch interface to return to
//                          client.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG | DISP_E_MEMBERNOTFOUND | S_FALSE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accChild(VARIANT varChild, IDispatch ** ppdispChild)
{
    HRESULT hr  = E_FAIL;
    long lChild = 0;
    
    
    //------------------------------------------------
    // validate out parameter
    //------------------------------------------------

    if ( !ppdispChild )
        return E_INVALIDARG;

    *ppdispChild = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    if ( varChild.lVal == CHILDID_SELF )
        return E_INVALIDARG;


    lChild = varChild.lVal;


    //------------------------------------------------
    // call parent helper method (override this method 
    // to provide custom implementation)
    //------------------------------------------------

    hr = m_pAOContainer->GetAccChild(lChild,ppdispChild);


    //--------------------------------------------------
    // if the child ID specified an element, then the
    // client needs to know that they cannot get
    // an IAccessible interface to the element, and
    // need to drive it via its parent (the m_pAOContainer
    // object)
    //--------------------------------------------------
    
    if ( hr == E_NOINTERFACE )
        return S_FALSE;
    else
        return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accName()
//
//  DESCRIPTION:
//
//      get Name of specific child
//
//  PARAMETERS:
//
//      varChild            contains specified child.
//      pbstrName               pointer to BSTR to copy string into
//            `
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG | DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accName(VARIANT varChild, BSTR* pbstrName)
{
    HRESULT hr  = E_FAIL;
    long lChild = 0;


    //------------------------------------------------
    // validate out parameter
    //------------------------------------------------

    if ( !pbstrName )
        return E_INVALIDARG;

    *pbstrName = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    lChild = varChild.lVal;


    //------------------------------------------------
    // get the accName of the requested child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method 
        // to provide custom implementation)
        //------------------------------------------------

        return m_pAOContainer->GetAccName( lChild, pbstrName );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr != S_OK )
            return hr;

        //--------------------------------------------------
        // make sure that the pointer returned is valid.
        // if it is not, that means that a childID was
        // not found.
        //--------------------------------------------------

        if ( pAEChild )
            return pAEChild->GetAccName( lChild, pbstrName );
        else 
            return E_INVALIDARG;
    }
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accValue()
//
//  DESCRIPTION:
//
//      get Value string of specific child
//
//  PARAMETERS:
//
//      varChild            contains specified child.
//      pbstrValue          pointer to BSTR to copy string into
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------


STDMETHODIMP    CImplIAccessible::get_accValue(VARIANT varChild, BSTR* pbstrValue)
{
    HRESULT hr  = E_FAIL;
    long lChild = 0;
    

    //------------------------------------------------
    // validate the out parameter
    //------------------------------------------------

    if ( !pbstrValue )
        return E_INVALIDARG;

    *pbstrValue = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    lChild = varChild.lVal;


    //------------------------------------------------
    // get the accValue of the requested child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method 
        // to provide custom implementation)
        //------------------------------------------------

        return m_pAOContainer->GetAccValue( lChild, pbstrValue );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr != S_OK )
            return hr;

        //--------------------------------------------------
        // make sure that the pointer returned is valid.
        // if it is not, that means that a childID was
        // not found.
        //--------------------------------------------------

        if ( pAEChild )
            return pAEChild->GetAccValue( lChild, pbstrValue );
        else
            return E_INVALIDARG;
    }
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accDescription()
//
//  DESCRIPTION:
//
//      get Description string of specific child
//
//  PARAMETERS:
//
//      varChild            contains specified child.
//      pbstrDescription        pointer to BSTR to copy string into
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accDescription(VARIANT varChild, BSTR* pbstrDescription)
{
    HRESULT hr  = E_FAIL;
    long lChild = NULL;
    

    //------------------------------------------------
    // validate the out parameter
    //------------------------------------------------

    if ( !pbstrDescription )
        return E_INVALIDARG;

    *pbstrDescription = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    lChild = varChild.lVal;


    //------------------------------------------------
    // get the accDescription of the requested child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method 
        // to provide custom implementation)
        //------------------------------------------------

        return m_pAOContainer->GetAccDescription( lChild, pbstrDescription );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr != S_OK )
            return hr;

        //--------------------------------------------------
        // make sure that the pointer returned is valid.
        // if it is not, that means that a childID was
        // not found.
        //--------------------------------------------------

        if ( pAEChild )
            return pAEChild->GetAccDescription( lChild, pbstrDescription );
        else
            return E_INVALIDARG;
    }
}

                    
//-----------------------------------------------------------------------
//  CImplIAccessible::get_accRole()
//
//  DESCRIPTION:
//
//      return ROLE ID / ROLE string for this object.
//
//  PARAMETERS:
//
//      varChild        contains specified child object to get role for
//      pVarRole        VARIANT * to return role constant/string in
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    HRESULT hr  = E_FAIL;


    //------------------------------------------------
    // validate the out parameter
    //------------------------------------------------

    if ( !pvarRole )
        return E_INVALIDARG;

    //------------------------------------------------
    // clear the out parameter
    //------------------------------------------------

    hr = VariantClear( pvarRole );

    if ( hr != S_OK )
        return hr;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // get the accRole of the requested child
    //------------------------------------------------

    long lRoleConstant = 0;

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method
        // to provide custom implementation)
        //------------------------------------------------

        hr = m_pAOContainer->GetAccRole( lChild, &lRoleConstant );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );


        if ( hr != S_OK )
            return hr;

        //--------------------------------------------------
        // make sure that the pointer returned is valid.
        // if it is not, that means that a childID was
        // not found.
        //--------------------------------------------------

        if ( pAEChild )
            hr = pAEChild->GetAccRole( lChild, &lRoleConstant );
        else
            hr = E_INVALIDARG;
    }
   
    if ( hr == S_OK )
    {
        //--------------------------------------------------
        // pack role into out parameter
        //--------------------------------------------------

        pvarRole->vt = VT_I4;
        pvarRole->lVal = lRoleConstant;
    }


    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accState()
//
//  DESCRIPTION:
//
//      return State ID / ROLE string for this object.
//
//  PARAMETERS:
//
//      varChild        contains specified child object to get role for
//      pVarRole        VARIANT * to return role constant/string in
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    HRESULT hr  = E_FAIL;


    //------------------------------------------------
    // validate the out parameter
    //------------------------------------------------

    if ( !pvarState )
        return E_INVALIDARG;


    //------------------------------------------------
    // clear the out parameter
    //------------------------------------------------

    hr = VariantClear( pvarState );

    if ( hr != S_OK )
        return hr;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant( &varChild );

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // get the accState of the requested child
    //------------------------------------------------

    long lStateConstant;

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method
        // to provide custom implementation)
        //------------------------------------------------
        
        hr = m_pAOContainer->GetAccState( lChild, &lStateConstant );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr != S_OK )
            return hr;

        //--------------------------------------------------
        // make sure that the pointer returned is valid.
        // if it is not, that means that a childID was
        // not found.
        //--------------------------------------------------

        if ( pAEChild )
            hr = pAEChild->GetAccState( lChild, &lStateConstant );
        else
            hr = E_INVALIDARG;
    }

    if ( hr == S_OK )
    {
        //------------------------------------------------
        // if the state was successfully returned, pack
        // and return to the client.
        //------------------------------------------------

        pvarState->vt = VT_I4;
        pvarState->lVal = lStateConstant;
    }


    return(S_OK);
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accHelp()
//
//  DESCRIPTION:
//
//      get the help string for the object
//
//  PARAMETERS:
//
//      varChild                contains specified child object.
//      pbstrHelp                   pointer to BSTR to return kbd short
//                              cut information in.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accHelp(VARIANT varChild, BSTR* pbstrHelp)
{
    HRESULT hr  = E_FAIL;
    

    //------------------------------------------------
    // validate the out parameter
    //------------------------------------------------

    if ( !pbstrHelp )
        return E_INVALIDARG;

    *pbstrHelp = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // get the accHelp of the requested child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method 
        // to provide custom implementation)
        //------------------------------------------------

        hr = m_pAOContainer->GetAccHelp( lChild, pbstrHelp );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );


        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->GetAccHelp( lChild, pbstrHelp );
            else
                hr = E_INVALIDARG;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accHelpTopic()
//
//  DESCRIPTION:
//      returns a pointer to a help file and an offset into that help file. 
//      
//  PARAMETERS:
//
//      pbstrHelpFile               helpfile string
//      varChild                variant holding child information
//      pidTopic                long * specifiying offset to topic.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accHelpTopic(BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic)
{
    HRESULT hr  = E_FAIL;
    
    
    //------------------------------------------------
    // validate the pointer parameters
    //------------------------------------------------

    if ( !pbstrHelpFile || !pidTopic )
        return E_INVALIDARG;

    *pidTopic = 0;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // get the accHelpTopic of the requested child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helpermethod (override this method 
        // to provide custom implementation)
        //------------------------------------------------

        hr = m_pAOContainer->GetAccHelpTopic( pbstrHelpFile, lChild, pidTopic );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->GetAccHelpTopic( pbstrHelpFile, lChild, pidTopic );
            else
                hr = E_INVALIDARG;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accKeyboardShortcut()
//
//  DESCRIPTION:
//
//      get Kbd shortcut string of specific child
//
//  PARAMETERS:
//
//      varChild                contains specified child object.
//      pbstrKeyboardShortcut       pointer to BSTR to return kbd short
//                              cut information in.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accKeyboardShortcut(VARIANT varChild, BSTR*   pbstrKeyboardShortcut)
{
    HRESULT hr  = E_FAIL;
    

    //------------------------------------------------
    // validate the out parameter
    //------------------------------------------------

    if ( !pbstrKeyboardShortcut )
        return E_INVALIDARG;

    *pbstrKeyboardShortcut = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // get the accKeyboardShortcut of the requested child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method 
        // to provide custom implementation)
        //------------------------------------------------
        
        hr = m_pAOContainer->GetAccKeyboardShortcut( lChild, pbstrKeyboardShortcut );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );
    
        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->GetAccKeyboardShortcut( lChild, pbstrKeyboardShortcut );
            else
                hr = E_INVALIDARG;
        }
    }

    if ( hr == S_OK && *pbstrKeyboardShortcut )
        hr = prefixKeyboardShortcut( pbstrKeyboardShortcut );

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accFocus()
//
//  DESCRIPTION:
//
//      get current object that has focus
//
//  PARAMETERS:
//
//      pvarFocusChild      pointer to VARIANT to store focused object ID/
//                          string in.
//  
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accFocus(VARIANT* pvarFocusChild)
{
    HRESULT hr              = E_FAIL;
    IUnknown  * pIUnknown   = NULL;


    //------------------------------------------------
    // validate out parameter
    //------------------------------------------------

    if ( !pvarFocusChild )
        return E_INVALIDARG;

    //------------------------------------------------
    // clear the out parameter
    //------------------------------------------------

    hr = VariantClear( pvarFocusChild );

    if ( hr != S_OK )
        return hr;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // call parent helper method (override this method
    // to provide custom implementation)
    //------------------------------------------------

    hr = m_pAOContainer->GetAccFocus( &pIUnknown );

    if ( hr == S_OK )
    {
        if ( pIUnknown )
        {
            hr = packReturnVariant( pIUnknown, pvarFocusChild );
        }
        else
        {
            //--------------------------------------------------
            // if the method returned S_OK w/o setting pIUnknown
            // method was implemented incorrectly.  
            // alert developer.
            //--------------------------------------------------
            
            assert( pIUnknown );
            hr = E_FAIL;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accSelection()
//
//  DESCRIPTION:
//
//      returns one of the following, based on what is selected :
//
//      selection:          return:
//      ----------------------------------------------------------
//      single object       object ID or ppDisp of that object.
//      multiple objects    IUnknown * of IEnumVariant interface.
//      no object           VT_EMPTY
//
//  PARAMETERS:
//
//      pvarSelectedChildren    pointer to VARIANT to store selection
//                              return information in.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accSelection(VARIANT* pvarSelectedChildren)
{                              
    HRESULT     hr          = E_FAIL;
    LPUNKNOWN   pIUnknown   = NULL;


    //------------------------------------------------
    // validate output parameter
    //------------------------------------------------

    if ( !pvarSelectedChildren )
        return E_INVALIDARG;


    //------------------------------------------------
    // clear the out parameter
    //------------------------------------------------

    hr = VariantClear( pvarSelectedChildren );

    if ( hr != S_OK )
        return hr;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // Query to find out what kind of selection exists:
    //  override this method to customize implementation
    //------------------------------------------------

    hr = m_pAOContainer->GetAccSelection( &pIUnknown );

    if ( hr == S_OK )
    {
        if( pIUnknown )
        {
            hr = packReturnVariant( pIUnknown, pvarSelectedChildren );
        }
        else
        {
            //--------------------------------------------------
            // If call returned S_OK w/o setting pIUnknown, 
            // the call was implemented incorrectly.
            // Alert developer!
            //--------------------------------------------------

            assert( pIUnknown );
            hr = E_FAIL;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::get_accDefaultAction()
//
//  DESCRIPTION:
//
//      get Default action string of specific child
//
//  PARAMETERS:
//
//      varChild            VARIANT containing child ID.
//      pbstrDefaultAction  BSTR pointer to return string information in.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::get_accDefaultAction(VARIANT varChild, BSTR* pbstrDefaultAction)
{
    HRESULT hr  = E_FAIL;
    

    //------------------------------------------------
    // validate output parameter
    //------------------------------------------------

    if ( !pbstrDefaultAction )
        return E_INVALIDARG;

    *pbstrDefaultAction = NULL;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // get the accDefaultAction of the requested child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method
        // to provide custom implementation)
        //------------------------------------------------
        
        hr = m_pAOContainer->GetAccDefaultAction( lChild, pbstrDefaultAction );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->GetAccDefaultAction( lChild, pbstrDefaultAction );
            else
                hr = E_INVALIDARG;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::accSelect()
//
//  DESCRIPTION:
//
//      select an object
//
//  PARAMETERS:
//
//      flagsSel            selection flags.
//      varChild            VARIANT containing child ID.
//      
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::accSelect( long flagsSel, VARIANT varChild )
{
    HRESULT hr  = E_FAIL;
    
    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // attempt to select the desired child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method
        // to provide custom implementation)
        //------------------------------------------------
        
        hr = m_pAOContainer->AccSelect( flagsSel, lChild );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->AccSelect( flagsSel, lChild );
            else
                hr = E_INVALIDARG;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::accLocation()
//
//  DESCRIPTION:
//
//      returns the location and size of an object specified by ID
//
//  PARAMETERS:
//
//      pxLeft      pointer to long containing left coord
//      pyTop       pointer to long containing top coord
//      pcxWidth    pointer to long containing width
//      pcyHeight   pointer to long containing height
//      varChild    VARIANT containg child ID
//      
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP CImplIAccessible::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    HRESULT hr  = E_FAIL;

    
    //------------------------------------------------
    // validate output parameter
    //------------------------------------------------

    if( !pxLeft || !pyTop || !pcxWidth || !pcyHeight )
        return E_INVALIDARG;

    *pxLeft = *pyTop = *pcxWidth = *pcyHeight = 0;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant(&varChild);

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // determine the location of the desired child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method
        // to provide custom implementation)
        //------------------------------------------------
        
        hr = m_pAOContainer->AccLocation( pxLeft, pyTop, pcxWidth, pcyHeight, lChild );
    }
    else
    {
        CAccElement * pAEChild = NULL;
        
        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );


        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->AccLocation( pxLeft, pyTop, pcxWidth, pcyHeight, lChild );
            else
                hr = E_INVALIDARG;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::accNavigate()
//
//  DESCRIPTION:
//
//      navigates in specified direction : returns
//      next object ID
//
//  PARAMETERS:
//
//      navDir      direction to navigate in
//      varStart    VARIANT containg ID of starting object
//      pvarEndUpAt pointer to structure to house ID of ending object in.
//      
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    HRESULT hr              = E_FAIL;
    IUnknown * pIUnknown    = NULL;


    //------------------------------------------------
    // validate output parameter
    //------------------------------------------------

    if ( !pvarEndUpAt )
        return E_INVALIDARG;

    hr = VariantClear( pvarEndUpAt );

    if ( hr != S_OK )
        return hr;

    FAIL_IF_DETACHED;


    //------------------------------------------------
    // unpack varStart
    //------------------------------------------------

    hr = validateVariant(&varStart);

    if ( hr != S_OK )
        return hr;

    if ( navDir == NAVDIR_FIRSTCHILD || navDir == NAVDIR_LASTCHILD )
        if ( varStart.lVal != CHILDID_SELF )
            return E_INVALIDARG;

    long lStart = varStart.lVal;

    //------------------------------------------------
    // call parent helper method to do actual navigation
    //------------------------------------------------

    hr = m_pAOContainer->AccNavigate( navDir, lStart, &pIUnknown );

    if ( hr == S_OK )
    {
        if ( pIUnknown )
        {
            hr = packReturnVariant( pIUnknown, pvarEndUpAt );
        }
        else
        {
            //--------------------------------------------------
            // getting here means that method returned S_OK
            // with a NULL pIUnknown : method was implemented
            // incorrectly. alert developer!
            //--------------------------------------------------
            
            assert( pIUnknown );
            hr = E_FAIL;
        }

    }


    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::accHitTest()
//
//  DESCRIPTION:
//
//      tests to see if point intersects with an object : returns 
//      ID of object if there is an intersection
//
//  PARAMETERS:
//
//      xLeft               x coord
//      yTop                y coord
//      pvarChildAtPoint    VARIANT containg child ID
//      
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::accHitTest(long xLeft, long yTop, VARIANT* pvarChildAtPoint)
{
    HRESULT    hr        = E_FAIL;
    IUnknown * pIUnknown = NULL;

    //------------------------------------------------
    // validate output parameter
    //------------------------------------------------

    if ( !pvarChildAtPoint )
        return E_INVALIDARG;


    hr = VariantClear( pvarChildAtPoint );

    if ( hr != S_OK )
        return hr;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // call helper method to test for object at point.
    //------------------------------------------------

    hr = m_pAOContainer->AccHitTest( xLeft, yTop, &pIUnknown );

    if ( hr == S_OK && pIUnknown )
        hr = packReturnVariant( pIUnknown, pvarChildAtPoint );

    //------------------------------------------------
    // if pIUnknown == NULL, the hit test point
    //  is outside the object.  in this case,
    //  according to MSAA spec, we return VT_EMPTY.
    //  *pvarChildAtPoint has already been cleared.
    //------------------------------------------------

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::accDoDefaultAction()
//
//  DESCRIPTION:
//
//      executes default action of object
//
//  PARAMETERS:
//
//      varChild    VARIANT containg child ID
//      
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::accDoDefaultAction(VARIANT varChild)
{
    HRESULT hr  = E_FAIL;
    
    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant( &varChild );

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // do the default action for the desired child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method
        // to provide custom implementation)
        //------------------------------------------------
        
        hr = m_pAOContainer->AccDoDefaultAction( lChild );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->AccDoDefaultAction( lChild );
            else
                hr = E_INVALIDARG;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::put_accName()
//
//  DESCRIPTION:
//
//      replaces object name.
//
//  PARAMETERS:
//
//      varChild    VARIANT containg child ID
//      bstrValue       string to put
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::put_accName(VARIANT varChild, BSTR bstrName)
{   
    HRESULT hr  = E_FAIL;
    
    
    //--------------------------------------------------
    // validate input BSTR parameter.
    //--------------------------------------------------

    if ( !bstrName )
        return E_INVALIDARG;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant( &varChild );

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // put the accName for the desired child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method
        // to provide custom implementation).
        // Helper method is responsible for deleting
        // allocated TCHAR string.
        //------------------------------------------------

        hr = m_pAOContainer->SetAccName( lChild, bstrName );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->SetAccName( lChild, bstrName );
            else
                hr = E_INVALIDARG;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::put_accValue()
//
//  DESCRIPTION:
//
//      replaces object value.
//
//  PARAMETERS:
//
//      varChild    VARIANT containg child ID
//      bstrValue       string to put
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIAccessible::put_accValue(VARIANT varChild, BSTR bstrValue)
{
    HRESULT hr  = E_FAIL;

    //--------------------------------------------------
    // validate input BSTR parameter.
    //--------------------------------------------------

    if ( !bstrValue )
        return E_INVALIDARG;

    FAIL_IF_DETACHED;

    //------------------------------------------------
    // unpack varChild
    //------------------------------------------------

    hr = validateVariant( &varChild );

    if ( hr != S_OK )
        return hr;

    long lChild = varChild.lVal;


    //------------------------------------------------
    // put the accValue for the desired child
    //------------------------------------------------

    if ( lChild == CHILDID_SELF )
    {
        //------------------------------------------------
        // call parent helper method (override this method
        // to provide custom implementation).
        // Helper method is responsible for deleting
        // allocated TCHAR string.
        //------------------------------------------------
        
        hr = m_pAOContainer->SetAccValue( lChild, bstrValue );
    }
    else
    {
        CAccElement * pAEChild = NULL;

        //--------------------------------------------------
        // iterate through child objects to match IDs
        //--------------------------------------------------

        hr = m_pAOContainer->GetAccElemFromChildID( lChild, &pAEChild );

        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // make sure that the pointer returned is valid.
            // if it is not, that means that a childID was
            // not found.
            //--------------------------------------------------

            if ( pAEChild )
                hr = pAEChild->SetAccValue( lChild, bstrValue );
            else
                hr = E_INVALIDARG;
        }
    }

    return hr;
}


//=======================================================================
// CImplIAccessible protected methods
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIAccessible::validateVariant()
//
//  DESCRIPTION:
//
//      makes sure that input variant has type VT_I4 : forces VT_EMPTY
//      variant types to VT_I4
//
//  PARAMETERS:
//
//      pVar            pointer to the VARIANT
//
//  RETURNS:
//
//      HRESULT S_OK | E_INVALIDARG
//
//  NOTES:
//
//      Copied implementation of CAccessible::ValidateChild()
//      from OLEACC.DLL's default.cpp.  This way we can handle
//      variants passed in from Automation clients like VBA.
//
//-----------------------------------------------------------------------

HRESULT CImplIAccessible::validateVariant(VARIANT *pVar)
{
    HRESULT hr = S_OK;
    

    assert(pVar);


    switch ( pVar->vt )
    {
        case VT_VARIANT | VT_BYREF:
            VariantCopy( pVar, pVar->pvarVal );
            hr = validateVariant( pVar );
            break;

        case VT_ERROR:
            if ( pVar->scode != DISP_E_PARAMNOTFOUND )
            {
                hr = E_INVALIDARG;
                break;
            }
            // FALL THRU!

        case VT_EMPTY:
            pVar->vt = VT_I4;
            pVar->lVal = CHILDID_SELF;
            // FALL THRU!

        case VT_I4:
            break;

        default:
            hr = E_INVALIDARG;
    }


    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::packReturnVariant()
//
//  DESCRIPTION:
//
//      package return variant : packs input object based on 
//      type of interface supported.
//      
//      IAccessible interface :     implies AO object. return this interface 
//                                  to client.
//      IEnumVariant interface :    implies a list.  return the IEnumVariant
//                                  object to client.
//      
//      E_NOINTERFACE:              implies an AE object : return the childID
//                                  to the client.
//      
//
//  PARAMETERS:
//
//      pIUnknown
//
//      pvarToPack
//
//  RETURNS:
//
//      HRESULT S_OK | E_INVALIDARG
//
//  NOTES:
//
//      Caller must initialize or clear the VARIANT pointed to by
//      pvarToPack before calling this method!
//
// ----------------------------------------------------------------------

HRESULT CImplIAccessible::packReturnVariant(IUnknown * pIUnknown, VARIANT *pvarToPack)
{
    HRESULT hr      = E_FAIL;
    void * pVoid    = NULL;


    assert( pIUnknown );
    assert( pvarToPack );

    //------------------------------------------------
    // Get the type of the object : QI for 
    // IAccessible.  If fail, then this might be an
    // enumerator or a CAccElement derived object.
    //------------------------------------------------

    hr = pIUnknown->QueryInterface( IID_IAccessible, &pVoid );

    if ( hr == S_OK )
    {
        //--------------------------------------------------
        // check to see if the interface pointer returned
        // is this current object.  If so, return CHILDID_SELF.
        // Otherwise, the pointer is a child object's 
        // IDispatch, so return it to the client.
        //--------------------------------------------------
        
        if ( pVoid == this )
        {
            pvarToPack->vt = VT_I4;
            pvarToPack->lVal = CHILDID_SELF;
        }
        else
        {
            pvarToPack->vt = VT_DISPATCH;
            hr = ((IUnknown*)pVoid)->QueryInterface( IID_IDispatch, (void**)&pvarToPack->pdispVal );
        }

        ((LPUNKNOWN)(pVoid))->Release();
    }

    else if ( hr == E_NOINTERFACE )
    {
        //--------------------------------------------------
        // QI to see if this is an EnumVariant interface. 
        //--------------------------------------------------

        hr = pIUnknown->QueryInterface( IID_IEnumVARIANT, &pVoid );

        if ( hr == S_OK )
        {
            //--------------------------------------------------
            // object is an enumerator, so return object's
            // IUnknown* to client.
            //--------------------------------------------------
            
            ((LPUNKNOWN)(pVoid))->Release();
            
            pvarToPack->vt = VT_UNKNOWN;
            pvarToPack->punkVal = pIUnknown;
        }

        else if ( hr == E_NOINTERFACE )
        {
            //--------------------------------------------------
            // focused object is CAccElement: return
            // child ID to client.
            //
            // **NOTE** the layout of the CAccElement
            // object makes this cast possible.
            //--------------------------------------------------

            pvarToPack->vt = VT_I4;
            pvarToPack->lVal = ((CAccElement *)pIUnknown)->GetChildID();

            hr = S_OK;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::getTypeInfo()
//
//  DESCRIPTION:
//
//      Gets an ITypeInfo* to the IAccessible type description.  This
//      ITypeInfo pointer is used by CImplIAccessible to implement IDispatch.
//
//  PARAMETERS:
//
//      ppITypeInfo
//
//  RETURNS:
//
//      HRESULT     S_OK or standard COM error
//
// ----------------------------------------------------------------------

HRESULT CImplIAccessible::getTypeInfo( /* out */ ITypeInfo** ppITypeInfo )
{
    HRESULT     hr;
    ITypeLib*   pITypeLib = NULL;


    assert( ppITypeInfo );
    assert( *ppITypeInfo == NULL );


    //-----------------------------------------------------
    //  First, attempt to load the Accessibility type
    //    library version 1.x using the registry.
    //
    //  BUGBUG: Research locale IDs.  How will we support
    //          multiple LCIDs?
    //
    //  NOTE:   Version 1.1 of the type lib in OLEACC.DLL
    //          contains the IAccessibleHandler interface,
    //          Version 1.0 does not.
    //-----------------------------------------------------

    hr = LoadRegTypeLib( LIBID_Accessibility, 1, 1, 0, &pITypeLib );

    if ( hr != S_OK )
        return hr;

    assert( pITypeLib );


    //-----------------------------------------------------
    //  The type library has been successfully loaded, so
    //    attempt to get the IAccessible type description.
    //-----------------------------------------------------

    hr = pITypeLib->GetTypeInfoOfGuid( IID_IAccessible, ppITypeInfo );


    //-----------------------------------------------------
    //  Release the type library interface.
    //-----------------------------------------------------

    if ( pITypeLib )
        pITypeLib->Release();


    //-----------------------------------------------------
    //  Double check the out parameter.
    //-----------------------------------------------------

    if ( hr != S_OK )
    {
        assert( *ppITypeInfo == NULL );

        if ( *ppITypeInfo != NULL )
        {
            (*ppITypeInfo)->Release();
            *ppITypeInfo = NULL;
        }
    }

#ifdef _DEBUG
    else  // hr == S_OK
        assert( *ppITypeInfo );
#endif


    return hr;
}


//-----------------------------------------------------------------------
//  CImplIAccessible::prefixKeyboardShortcut()
//
//  DESCRIPTION:
//
//      Prefixes "Alt+" to the keyboard shortcut obtained from the
//      AO or AE.
//
//      The accKeyboardShortcut maps to the TEO ACCESSKEY property.
//
//  PARAMETERS:
//
//      pbstrKS     [in/out] pointer to the keyboard shortcut
//
//  RETURNS:
//
//      HRESULT     S_OK or standard COM error
//
//  NOTE:
//
//      On failure, this method takes responsibility for freeing the BSTR.
// ----------------------------------------------------------------------

HRESULT CImplIAccessible::prefixKeyboardShortcut( /* in-out */ BSTR* pbstrKS )
{
    HRESULT hr = S_OK;
    WCHAR*  lpwsz = NULL;
    long    len = wcslen( wszKeyboardShortcutPrefix ) + wcslen( *pbstrKS ) + 1;

    lpwsz = new WCHAR[len];

    if ( lpwsz )
    {
        BOOL    bSuccess;

        wcscpy( lpwsz, wszKeyboardShortcutPrefix );
        wcscat( lpwsz, *pbstrKS );

        bSuccess = (BOOL) SysReAllocString( pbstrKS, lpwsz );

        if ( !bSuccess )
        {
            SysFreeString( *pbstrKS );
            *pbstrKS = NULL;
            hr = E_OUTOFMEMORY;
        }

        delete [] lpwsz;
    }
    else
    {
        SysFreeString( *pbstrKS );
        *pbstrKS = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



//=======================================================================
// CImplIOleWindow class implementation : public methods
//=======================================================================


//-----------------------------------------------------------------------
//  CImplIOleWindow::CImplIOleWindow()
//
//  DESCRIPTION:
//
//      CImplIOleWindow class ctor.
//
//  PARAMETERS:
//
//      pIUnknown       pointer to owner IUnknown.
//      pAOContainer    pointer to owner object
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CImplIOleWindow::CImplIOleWindow(IUnknown * pIUnknown,CTridentAO *pAOContainer)
{
    //------------------------------------------------
    // assign controlling IUnknown;
    //------------------------------------------------
    
    m_pIUnknown     = pIUnknown;
    
    //------------------------------------------------
    // assign parent pointer : even though this is 
    // the same value as the m_pIUnknown pointer, it
    // makes more sense to access parent functionality
    // through a non OLE pointer.
    //------------------------------------------------

    m_pAOContainer      = pAOContainer;
}


//-----------------------------------------------------------------------
//  CImplIOleWindow::~CImplIOleWindow()
//
//  DESCRIPTION:
//
//      CImplIOleWindow class destructor.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CImplIOleWindow::~CImplIOleWindow()
{

}


//=======================================================================
// CImplIOleWindow : IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIOleWindow::QueryInterface()
//
//  DESCRIPTION:
//      
//      only handles riid == IID_IOLEWINDOW : defers all 
//      other inteface handling to controlling IUnknown.
//      
//  PARAMETERS:
//
//      riid        REFIID of requested interface.
//      ppv         pointer to interface in.
//
//  RETURNS:
//
//      E_NOINTERFACE | NOERROR.
//
// ----------------------------------------------------------------------

STDMETHODIMP CImplIOleWindow::QueryInterface(REFIID riid, void** ppv)
{
    if ( !ppv )
        return E_INVALIDARG;


    *ppv = NULL;


    FAIL_IF_DETACHED;

    //------------------------------------------------
    // this class only knows about IOleWindow
    //------------------------------------------------

    if (riid == IID_IOleWindow )
    {
        *ppv = (IOleWindow *)this;
        ((IOleWindow *)*ppv)->AddRef();

        return NOERROR;
    }
    else
    {
        return m_pIUnknown->QueryInterface( riid, ppv );
    }

}


//-----------------------------------------------------------------------
//  CImplIOleWindow::AddRef()
//
//  DESCRIPTION:
//
//  Handled in owner class
//
//  PARAMETERS:
//
//
//  RETURNS:
//
//  reference count AFTER increment.
//
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG)    CImplIOleWindow::AddRef(void)
{
    //------------------------------------------------
    // delegate ref counting to owner
    //------------------------------------------------
    
    return m_pIUnknown->AddRef();
}


//-----------------------------------------------------------------------
//  CImplIOleWindow::Release()
//
//  DESCRIPTION:
//
//  Handled in Owner class.
//
//  PARAMETERS:
//
//
//  RETURNS:
//
//  reference count AFTER decrement.
//
// ----------------------------------------------------------------------

STDMETHODIMP_(ULONG)    CImplIOleWindow::Release(void)
{
    //------------------------------------------------
    // delegate ref counting to owner
    //------------------------------------------------
    
    return m_pIUnknown->Release();
}


//=======================================================================
// CImplIOleWindow : IOleWindow Inteface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CImplIOleWindow::GetWindow()
//
//  DESCRIPTION:
//
//      returns pointer to HWND corresponding to this object.
//      **NOTE** 
//      this method is not implemented for any object except for the 
//      Document object : no other object proxies an hwnd.
//
//
//  PARAMETERS:
//
//      phwnd   pointer to window handle: returned to client
//
//  RETURNS:
//
//      E_NOIMPL | S_OK 
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIOleWindow::GetWindow( HWND* phwnd )
{
    if ( !phwnd )
        return E_INVALIDARG;

    FAIL_IF_DETACHED;

    //--------------------------------------------------
    // delegate to containing AO : let it either
    // handle/return E_NOIMPL
    //--------------------------------------------------
    
    *phwnd = m_pAOContainer->GetWindowHandle();

    return S_OK;
}


//-----------------------------------------------------------------------
//  CImplIOleWindow::ContextSensitiveHelp()
//
//  DESCRIPTION:
//
//      starts context sensitive help
//      **NOTE**    
//      this method is not supported for accessible objects.
//
//
//  PARAMETERS:
//
//      phwnd   pointer to window handle: returned to client
//
//  RETURNS:
//
//      E_NOIMPL
//
// ----------------------------------------------------------------------

STDMETHODIMP    CImplIOleWindow::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}




//=======================================================================
// CTridentAO class implementation : public methods
//=======================================================================

//-----------------------------------------------------------------------
//  CTridentAO::CTridentAO()
//
//  DESCRIPTION:
//
//      CTridentAO class constructor.
//
//  PARAMETERS:
//
//      pAOParent           pointer to the parent accessible object in 
//                          the AOM tree
//      
//      pDocAO              pointer to closest document object.
//
//      nTOMIndex           index of the element from the TOM document.all 
//                          collection.
//      
//      nChildID            Child ID.
//
//      hWnd                pointer to the window of the trident object that 
//                          this object corresponds to.
//
//      bSupported          BOOL indicating whether this AO is supported or not -
//                          defaults to TRUE.
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CTridentAO::CTridentAO(CTridentAO * pAOParent,CDocumentAO * pDocAO,UINT nTOMIndex,UINT nChildID,HWND hWnd,BOOL bSupportedTag) :
CAccObject(nChildID,hWnd,bSupportedTag)
{
    m_pParent = pAOParent;

    m_pDocAO = pDocAO;
    if (m_pDocAO && m_pDocAO !=this)
        m_pDocAO->AddRef();  // we depend heavily on this being around

    //------------------------------------------------
    // assign the delegating IUnknown to CTridentAE :
    // this member will be overridden in derived class
    // constructors so that the delegating IUnknown 
    // will always be at the derived class level.
    //------------------------------------------------

    m_pIUnknown     = (IUnknown *)this;
    m_pTOMObjIUnk   = NULL;

    //--------------------------------------------------
    // all collection index.
    //--------------------------------------------------

    m_nTOMIndex     = nTOMIndex;        

    //--------------------------------------------------
    // state location bool
    //--------------------------------------------------

    m_bOffScreen    = FALSE;
    
    //--------------------------------------------------
    // tree resolution BOOL : this is set after
    // tree has been fully resolved (built)under this node.
    //--------------------------------------------------

    m_bResolvedState = FALSE;

    //--------------------------------------------------
    // initialize object's detached and zombified flags.
    // used in cleaning up the tree.
    //--------------------------------------------------

    m_bDetached = FALSE;
    m_bZombified = FALSE;

    //------------------------------------------------
    // initialize IAccessible implementor pointer
    //------------------------------------------------

    m_pImplIAccessible = NULL;
    
    //------------------------------------------------
    // initialize IOleWindow implementor pointer
    //------------------------------------------------

    m_pImplIOleWindow = NULL;

    //--------------------------------------------------
    // init all property data values.
    //--------------------------------------------------

    m_pIHTMLElement     = NULL;
    m_pIHTMLStyle       = NULL;
    m_bstrStyle         = NULL;
    m_bstrDisplay       = NULL;

    m_bstrName          = NULL;           
    m_bstrValue         = NULL;          
    m_bstrDescription   = NULL;    
    m_bstrDefaultAction = NULL;  
    m_bstrKbdShortcut   = NULL;    

    //--------------------------------------------------
    //  For those derived objects whose accName and
    //  accDescription settings are interdependent,
    //  those values will be determined and cached
    //  in one method.
    //--------------------------------------------------

    m_bNameAndDescriptionResolved = FALSE;

    //--------------------------------------------------
    //  Set the item type to unsupported by default.
    //  This will be overridden in derived classes.
    //--------------------------------------------------

    m_lAOMType = AOMITEM_NOTSUPPORTED;

    //------------------------------------------------
    //  NOTE : reference counter initialized 
    //  in CTridentAO base class.
    //------------------------------------------------

#ifdef _DEBUG
    _tcscpy(m_szAOMName,_T("Generic CTridentAO"));
#endif

}


//-----------------------------------------------------------------------
//  CTridentAO::~CTridentAO()
//
//  DESCRIPTION: DTOR
//
//  PARAMETERS: None
//
//  RETURNS:    None.
//
// ----------------------------------------------------------------------

CTridentAO::~CTridentAO()
{
    //--------------------------------------------------
    // PREVENT CRASHES AND MEMORY LEAKS:-
    //
    // DON'T CALL THIS DIRECTLY.  CALL DETACH INSTEAD.
    //
    //  see the ::Detach() Fx Description for the reasons.
    //
    //--------------------------------------------------
    if (m_pDocAO && m_pDocAO !=this)
        m_pDocAO->Release();

    freeDataMembers();

}


//-----------------------------------------------------------------------
//  CTridentAO::Init()
//
//  DESCRIPTION:
//
//      Initialization : set values of data members
//
//
//      **NOTE** this is one place where we use a standard COM interface
//      pointer instead of an ATL CComQIPtr.  This is because this pointer 
//      needs to exist for the lifetime of the app -- its existence must
//      ABSOLUTELY be guaranteed for the duration of the lifetime of the object.
//      It is a lot more obvious to explicitly release the interface at object 
//      destruction, instead of having it implicitly released.
//
//
//  PARAMETERS:
//
//      pTOMObjIUnk     pointer to IUnknown of TOM object.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::Init(IUnknown * pTOMObjIUnk)
{
    HRESULT hr  = E_FAIL;

    
    assert(pTOMObjIUnk);

    if ( !pTOMObjIUnk )
        return E_INVALIDARG;

    m_pTOMObjIUnk = pTOMObjIUnk;
    m_pTOMObjIUnk->AddRef();

    //--------------------------------------------------
    // Cache TOM interface pointers and static TOM data
    //--------------------------------------------------

    if (!m_pIHTMLElement)
        hr = pTOMObjIUnk->QueryInterface(IID_IHTMLElement,(void **)&m_pIHTMLElement);

    if ( hr == S_OK && m_lAOMType != AOMITEM_NOTSUPPORTED )
        hr = createInterfaceImplementors();

    return hr;
}

//-----------------------------------------------------------------------
//  CTridentAO::Detach()
//
//  DESCRIPTION: Since the client App may have pts to the AO we can't just 
//      delete it, instead we need to detach them from the tree.  If the
//      refcount is 0, clean up and delete. Otherwise, set the m_bDetached
//      flag and clean up all the children.  Every IAccessible Method checks
//      this flag before doing any work, and returns an error if the client
//      is accessing a detached AO
//
//  BUGBUG (carled) - Currently Detach is only called on Window destruction or
//     on unsupported object removal. If this changes, and Detach is called on
//     arbitrary objects, then the AO/AE DTOR's need to go to the parent's 
//     childlist and find & remove themselves.
//
//  PARAMETERS: None
//
//  RETURNS:    Nothing
//
// ----------------------------------------------------------------------

void 
CTridentAO::Detach () 
{ 
    m_bDetached = TRUE;

    DetachChildren();

    ReleaseTridentInterfaces();

    // if our refcount is 1 (CTOR) then delete this
    Release();
}

void
CTridentAO::Zombify()
{
    std::list<CAccElement *>::iterator itCurPos;

    m_bZombified = TRUE;

    //--------------------------------------------------
    // while there is a child list, Zombify all my children.
    //--------------------------------------------------
    itCurPos = m_AEList.begin();

    while( itCurPos != m_AEList.end() )
    {
        (*itCurPos)->Zombify();

        itCurPos++;
    }

    ReleaseTridentInterfaces();
}

//-----------------------------------------------------------------------
//  CTridentAO::DetachChildren()
//
//  DESCRIPTION:
//
//  PARAMETERS:
//
//
//  RETURNS: S_OK
//
//-----------------------------------------------------------------------

HRESULT
CTridentAO::DetachChildren()
{
    CAOMMgr * pAOMMgr = NULL;
    std::list<CAccElement *>::iterator itCurPos;

    //--------------------------------------------------
    // while there is a child list, Detach all my children.
    //--------------------------------------------------

    while (m_AEList.size())
    {
        itCurPos = m_AEList.begin();

        if (!(*itCurPos)->IsDetached())
        {
            (*itCurPos)->Detach();
        }

        m_AEList.erase(itCurPos);
    }


    if (S_OK != GetDocumentAO()->GetAOMMgr(&pAOMMgr))
        return E_FAIL;

    if (pAOMMgr)
        pAOMMgr->ResetState();
    
    return (S_OK);
}

//-----------------------------------------------------------------------
//  CTridentAO::DoBlockForDetach ()
//
//  DESCRIPTION: DoBlockForDetach detaches the window in the global slot.
//          this is necesary due to process reentrancy from our various
//          clients.
//
//  PARAMETERS: None
//      
//  RETURNS: None
//
// ----------------------------------------------------------------------

BOOL 
CTridentAO::DoBlockForDetach ()
{
    if (m_pDocAO->CanDetachTreeNow() )
    {
        g_pProxyMgr->DetachReadyTrees();
        return TRUE;
    }
    else
        return IsDetached();
}


//-----------------------------------------------------------------------
//  CTridentAO::ReleaseTridentInterfaces()
//
//  DESCRIPTION: Calls release on all cached Trident object interface
//               pointers.  The idea being that if we are detached we
//               don't need to hold on to our Trident objects because
//               DoBlockForDetach() will prevent any useful access to
//               us.  This way we can allow Trident to clean itself up
//               in a more timely manner.
//
//  PARAMETERS: None
//      
//  RETURNS: None
//
// ----------------------------------------------------------------------

void 
CTridentAO::ReleaseTridentInterfaces ()
{
    if(m_pTOMObjIUnk)
    {
        m_pTOMObjIUnk->Release();
        m_pTOMObjIUnk = NULL;
    }

    if(m_pIHTMLElement)
    {
        m_pIHTMLElement->Release();
        m_pIHTMLElement = NULL;     
    }

    if(m_pIHTMLStyle)
    {
        m_pIHTMLStyle->Release();
        m_pIHTMLStyle = NULL;       
    }
}


//=======================================================================
// IUnknown interface implementation
//=======================================================================

//-----------------------------------------------------------------------
//  CTridentAO::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CTridentAE object implements
//      IUnknown and IAccessible.
//      TODO :implement IDispatch
//
//  PARAMETERS:
//
//      riid        REFIID of requested interface.
//      ppv         pointer to interface in.
//
//  RETURNS:
//
//      E_NOINTERFACE | NOERROR.
//
// ----------------------------------------------------------------------

STDMETHODIMP CTridentAO::QueryInterface(REFIID riid, void** ppv)
{
    if ( !ppv )
        return E_INVALIDARG;


    *ppv = NULL;


    //------------------------------------------------
    // Delegate to the IUnknown implementor set in 
    // the constructor method of the most derived 
    // class. 
    // Otherwise, delegate to the implementor objects
    // of the IAccessible or IOleWindow interfaces.
    //------------------------------------------------

    if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown *)m_pIUnknown;
    }
    else if ((riid == IID_IAccessible) || (riid == IID_IDispatch) )
    {
        assert( m_pImplIAccessible );

        *ppv = (IAccessible *)m_pImplIAccessible;
    }
    else if(riid == IID_IOleWindow )
    {
        assert( m_pImplIOleWindow );

        *ppv = (IOleWindow *)m_pImplIOleWindow;
    }
    else if (riid == IID_IEnumVARIANT)
    {
        //--------------------------------------------------
        // instead of returning a pointer to an IEnumVARIANT
        // interface on this object, we create an enumerator
        // object and return a pointer to its IEnumVARIANT 
        // interface.
        // Although this differs from MSAA behavior
        // (MSAA expects the object itself to support
        // IEnumVARIANT!), this is standard COM behavior.
        //--------------------------------------------------

        CEnumVariant*   pcenum;
        HRESULT         hr;

        hr = createEnumerator( &pcenum );

        if ( FAILED( hr ) )
            return hr;

        *ppv = (IEnumVARIANT *)pcenum;
        return(NOERROR);
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }
    

    //------------------------------------------------
    // ref count maintained in most derived class.
    //------------------------------------------------

    ((LPUNKNOWN) *ppv)->AddRef();
    return(NOERROR);
}


//=======================================================================
// helper methods (override these to provide custom functionality for
// CTridentAO - derived methods )
//=======================================================================

//----------------------------------------------------------------------- 
//  CTridentAO::GetAccFocus()
//
//  DESCRIPTION:
//
//      Get the object that has the focus, return the 
//      type of the focused object to the user.  
//
//  PARAMETERS:
//
//      ppIUnknown : pointer to IUnknown of returned object.  This object
//                  can be a CAccElement or a CAccObject.
//                  QI for IAccessible to find out.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccFocus(IUnknown **ppIUnknown)
{
    CAccElement*    pAccElem = NULL;
    UINT            nTOMIndex = 0;
    HRESULT         hr = E_FAIL;


    assert( m_pDocAO );
    assert( ppIUnknown );


    //------------------------------------------------
    // Get AO or AE with current focus
    //------------------------------------------------

    hr = m_pDocAO->GetFocusedItem( &nTOMIndex );

    if ( hr == S_OK )
    {
        //------------------------------------------------
        // Are we the focused item?
        //------------------------------------------------

        if ( nTOMIndex == m_nTOMIndex )
        {
            *ppIUnknown = (LPUNKNOWN)this;
        }
        else
        {
            //--------------------------------------------------
            // get AOMMgr pointer to build/navigate tree to
            // next object in the focus chain.
            //--------------------------------------------------

            CAOMMgr* pAOMMgr = NULL;

            hr = GetAOMMgr( &pAOMMgr );

            if ( hr == S_OK )
            {   
                //--------------------------------------------------
                //  get next object in focus chain to return to client
                //--------------------------------------------------

                hr = pAOMMgr->GetFocusedObjectFromID( this, nTOMIndex, &pAccElem );

                if ( hr == S_OK )
                {
                    if ( pAccElem )
                        *ppIUnknown = (LPUNKNOWN)pAccElem;
                    else
                        hr = E_FAIL;
                }
            }
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccSelection()
//
//  DESCRIPTION:
//
//      returns the id or the IDispatch * to the selected object (if
//      single selection), or the IUnknown * to the IEnumVariant object
//      that contains the VARIANTs of the selected objects.
//
//  PARAMETERS:
//      
//  ppIUnknown      pointer to IUnknown of CAccElement, CAccObject,
//                  or IEnumVariant.  QI on this interface to find out
//                  what type of object was selected.
//
//  RETURNS:
//
//      HRESULT     S_OK | S_FALSE | E_FAIL | E_INVALIDARG
//
//                  (S_FALSE => nothing selected)
//
//-----------------------------------------------------------------------

HRESULT CTridentAO::GetAccSelection( IUnknown** ppIUnknown )
{
    HRESULT hr = E_FAIL;
    BOOL    bIsSelected = FALSE;


    assert(ppIUnknown);
    assert(m_pDocAO);

    
    *ppIUnknown = NULL;


    //--------------------------------------------------
    //  Tree must be fully built prior to getting the 
    //  selected objects.
    //--------------------------------------------------

    if ( hr = ensureResolvedTree() )
        return hr;


    //------------------------------------------------
    //  Does the text range of the TEO associated with
    //  this AO intersect the range selection?
    //
    //  If not, neither this AO nor any of its
    //  children (if it has any) are selected, so
    //  return S_FALSE.
    //------------------------------------------------

    assert( m_pIHTMLElement );

    if ( !m_pIHTMLElement )
        return E_NOINTERFACE;

    hr = m_pDocAO->IsTEOSelected( m_pIHTMLElement, &bIsSelected );

    if ( hr == S_OK )
    {
        if ( !bIsSelected )
            hr = S_FALSE;
        else
        {
            //------------------------------------------------
            //  The text range of the TEO associated with
            //  this AO intersects the range selection, so
            //  does this AO have any children?
            //------------------------------------------------

            if ( !m_AEList.size() )
            {
                //------------------------------------------------
                //  This AO has no children--it alone is selected.
                //------------------------------------------------

                *ppIUnknown = (LPUNKNOWN)this;
            }
            else
            {
                //------------------------------------------------
                //  This AO has children, so one or more of them
                //  are selected.
                //------------------------------------------------

                hr = getSelectedChildren( ppIUnknown );
            }
        }
    }

    return hr;
}

//-----------------------------------------------------------------------
//  CTridentAO::AccNavigate()
//
//  DESCRIPTION:
//      moves from one object to the next
//  
//  PARAMETERS:
//
//      navdir      navigation direction : 
//          NAVDIR_DOWN 
//          NAVDIR_LASTCHILD 
//          NAVDIR_LEFT 
//          NAVDIR_NEXT 
//          NAVDIR_PREVIOUS 
//          NAVDIR_RIGHT 
//          NAVDIR_UP 
//
//      lStart              specifies whether to start at child or self
//      ppIUnknown          IUnknown of object navigated to: either a 
//                          CAccElement or a CAccObject.
//                          QI on this object to get type.
//                                         
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------


HRESULT CTridentAO::AccNavigate(long navDir, long lStart,IUnknown **ppIUnknown)
{
    HRESULT hr = E_FAIL;

    std::list<CAccElement *>::iterator itChildPos;

    assert(ppIUnknown);


    
    //------------------------------------------------
    // Otherwise navigate in relation to the object 
    //  itself or one of its specified children
    //------------------------------------------------

    if ( lStart == CHILDID_SELF )
    {
        //------------------------------------------------
        // Navigate w/ respect to the object, so only
        //  first/lastchild navigation is valid
        //------------------------------------------------

        switch( navDir )
        {
            case NAVDIR_FIRSTCHILD :

                //--------------------------------------------------
                // tree must be fully built in order to 
                // successfully navigate to a child.
                //--------------------------------------------------

                if(hr = ensureResolvedTree())
                    return(hr);

                //------------------------------------------------
                // list size is only a factor if the caller
                // is trying to navigate children. Otherwise it 
                // is ignored.
                //------------------------------------------------

                if ( m_AEList.size()<= 0 )
                    return( S_FALSE );


                itChildPos  = m_AEList.begin();
                *ppIUnknown = (LPUNKNOWN)*itChildPos;
                break;

            case NAVDIR_LASTCHILD :


                //--------------------------------------------------
                // tree must be fully built in order to 
                // successfully navigate to a child.
                //--------------------------------------------------

                if(hr = ensureResolvedTree())
                    return(hr);

                //------------------------------------------------
                // list size is only a factor if the caller
                // is trying to navigate children. Otherwise it 
                // is ignored.
                //------------------------------------------------

                if ( m_AEList.size()<= 0 )
                    return( S_FALSE );

                if(m_AEList.size() == 1)
                {
                    itChildPos  = m_AEList.begin();
                    *ppIUnknown = (LPUNKNOWN)*itChildPos;
                }
                else
                {
                    //------------------------------------
                    //  Standard case : > 1 child in list.
                    //  Don't check against end of list, which
                    //  is just past the *last* list item,
                    //  and referencing undefined stuff.
                    //------------------------------------

                    itChildPos  = --m_AEList.end();
                    *ppIUnknown = (LPUNKNOWN)*itChildPos;
                }

                break;

            case NAVDIR_NEXT :
            case NAVDIR_PREVIOUS :
                
                
                //--------------------------------------------------
                // sibling navigation: we need to delegate to parent
                // passing this object's child ID to the parent.
                //--------------------------------------------------

                if(m_pParent)
                    return(m_pParent->AccNavigate(navDir,GetChildID(),ppIUnknown));
                else
                    return(S_FALSE);
                

            default :

                return( DISP_E_MEMBERNOTFOUND );
        }
    }
    else
    {

        //--------------------------------------------------
        // tree must be fully built in order to 
        // successfully navigate to a child.
        //--------------------------------------------------

        if(hr = ensureResolvedTree())
            return(hr);


        if ( m_AEList.size() <= 0 )
            return( DISP_E_MEMBERNOTFOUND );

        //------------------------------------------------
        // Navigate w/ respect to one of the children, so
        //  only prev/nextchild navigation is valid
        //------------------------------------------------

        CAccElement* pAccElement;

        if ( GetAccElemFromChildID( lStart, &pAccElement, &itChildPos ) == S_FALSE )
            return( DISP_E_MEMBERNOTFOUND );

        switch( navDir )
        {
            case NAVDIR_FIRSTCHILD :
            case NAVDIR_LASTCHILD  :

                return( DISP_E_MEMBERNOTFOUND );

            case NAVDIR_NEXT :

                //------------------------------------
                // Don't check against end of list, which
                //  is just past the *last* list item,
                //  and referencing undefined stuff.
                //------------------------------------

                if ( itChildPos == --m_AEList.end() )
                    return( S_FALSE );

                *ppIUnknown = LPUNKNOWN(*(++itChildPos));
                break;

            case NAVDIR_PREVIOUS :

                if ( itChildPos == m_AEList.begin() )
                    return( S_FALSE );

                *ppIUnknown = LPUNKNOWN(*(--itChildPos));
                break;

            default :

                return( DISP_E_MEMBERNOTFOUND );
        }
    }

    assert( *ppIUnknown );


    return(S_OK);
}


//-----------------------------------------------------------------------
//  CTridentAO::AccHitTest()
//
//  DESCRIPTION:
//      returns ID of object at selected point.  Non DocumentAO/FrameAO
//      objects delegate to the closest Document/Frame by calling their parent
//  
//  PARAMETERS:
//
//      xLeft           left screen coordinate
//      yTop            top screen coordinate
//      ppIUnknown      pointer to IUnknown interface of returned object:
//                      returned object is either a CAccElement or
//                      a CAccObject.  QI on this interface to get
//                      the type of object.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::AccHitTest(long xLeft, long yTop,IUnknown ** ppIUnknown)
{
    HRESULT hr              = E_FAIL;
    CAccElement * pAccEl    = NULL;


    assert( ppIUnknown );

    *ppIUnknown = NULL;

    //--------------------------------------------------
    // feed coordinates into a POINT structure 
    // for easier manipulation.
    //--------------------------------------------------

    POINT pt;

    pt.x = xLeft;
    pt.y = yTop;

    //--------------------------------------------------
    // get AOMMgr pointer to build/navigate tree to
    // next hit object.
    //--------------------------------------------------
    
    CAOMMgr *pAOMMgr;
    
    if(hr = GetAOMMgr(&pAOMMgr))
    {   
        return(hr);
    }

    //--------------------------------------------------
    // convert point into client coords for correct
    // evaluation on Trident side.
    //--------------------------------------------------

    assert(m_hWnd);

    ScreenToClient(m_hWnd,&pt);

    //--------------------------------------------------
    // get next hit object.
    //--------------------------------------------------

    hr = pAOMMgr->GetAccessibleObjectFromPt(this,&pt,&pAccEl);

    if ( SUCCEEDED( hr ) )
    {
        //--------------------------------------------------
        // CAOMMgr::GetAccessibleObjectFromPt() may return
        //  success but not have set pAccEl.  in this case,
        //  the point lies outside the object, which is fine.
        //--------------------------------------------------

        if ( pAccEl )
        {
            //--------------------------------------------------
            // cast the returned object and return it.
            // the addref happened
            //--------------------------------------------------

            *ppIUnknown = (IUnknown *)pAccEl;
        }

        hr = S_OK;
    }

    return hr;
}

 

//-----------------------------------------------------------------------
//  CTridentAO::GetAccParent()
//
//  DESCRIPTION:
//
//  gets the parent of this object and returns its IDispatch pointer
//                               
//  PARAMETERS:
//
//  ppdispParent : pointer to parent interface to return to caller.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccParent(IDispatch ** ppdispParent)
{
    assert( ppdispParent );

    //------------------------------------------------
    // check for parent : if no parent, set out
    // parameter to NULL and return S_FALSE
    //------------------------------------------------

    HRESULT hr  = E_FAIL;

    if ( m_pParent )
        hr = m_pParent->QueryInterface(IID_IAccessible,(void **)ppdispParent);

    if ( hr != S_OK )
    {
        *ppdispParent = NULL;
        hr = S_FALSE;
    }

    return hr;
}



//-----------------------------------------------------------------------
//  CTridentAO::GetAccChildCount()
//
//  DESCRIPTION:
//
//      returns number of children.
//
//  PARAMETERS:
//
//      plChildCount    pointer to long var to fill w/ child count
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccChildCount(long* plChildCount)
{
    HRESULT hr = E_FAIL;

    assert ( plChildCount );

    //--------------------------------------------------
    // make sure that tree is built before querying it.
    //--------------------------------------------------

    if(hr = ensureResolvedTree())
        return(hr);


    *plChildCount = m_AEList.size();
    return( S_OK );
}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccChild()
//`//   DESCRIPTION:
//
//      returns specified child IDispatch interface
//
//  PARAMETERS:
//
//      lChild          child ID / self ID
//      ppdispChild     pointer to dispatch Interface of child.
//      
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccChild(long lChild, IDispatch ** ppdispChild)
{
    HRESULT hr              = E_FAIL;

    assert ( ppdispChild );
    
    CAccElement* pAEChild   = NULL;
    
    *ppdispChild            = NULL;
                                            
    
    
    //--------------------------------------------------
    // search for child object in parent list.
    //--------------------------------------------------
    
    hr = GetAccElemFromChildID(lChild,&pAEChild);


    if(hr != S_OK)
        return(hr);

    //--------------------------------------------------
    // make sure that the pointer returned is valid.
    // if it is not, that means that a childID was
    // not found.
    //--------------------------------------------------


    if(pAEChild)
    {
        //--------------------------------------------------
        // **NOTE** this call will fail if the child 
        // is an AE, which doesnt support an IDispatch
        // interface. This is by design : it tells the 
        // client that they need to access the child element
        // via the parent object.
        //
        // if the QI fails, the ppdispChild value will 
        // not have to be explicitly set to NULL because 
        // it was initialized as NULL.
        //--------------------------------------------------

        return(pAEChild->QueryInterface( IID_IDispatch, (void**)ppdispChild ));

    }
    else
    {
        //--------------------------------------------------
        // reaching here means that the child was not found
        //--------------------------------------------------
        return( hr);
    }
        
}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccName()
//
//  DESCRIPTION:
//
//      returns name of object/element
//
//  PARAMETERS:
//
//      lChild          child ID / Self ID
//      pbstrName           returned name.
//      
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccName(long lChild, BSTR * pbstrName)
{
    return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccValue()
//
//  DESCRIPTION:
//
//      returns value string to client.
//
//  PARAMETERS:
//
//      lChild      ChildID/SelfID
//
//      pbstrValue  returned value string
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccValue(long lChild, BSTR * pbstrValue)
{
    return(DISP_E_MEMBERNOTFOUND);
}

//-----------------------------------------------------------------------
//  CTridentAO::GetAccDescription()
//
//  DESCRIPTION:
//
//      returns description string to client.
//
//  PARAMETERS:
//
//      lChild              Child/Self ID
//
//      pbstrDescription        Description string returned to client.
//  
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
    return(DISP_E_MEMBERNOTFOUND);
}



//-----------------------------------------------------------------------
//  CTridentAO::GetAccState()
//
//  DESCRIPTION:
//
//      returns object state to client
//
//  PARAMETERS:
//
//      lChild      Child/Self ID 
//
//      plState     pointer to state constant to be set
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccState( long lChild, long* plState )
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // Determine if the AO is visible. NOTE, this 
    //  implementation only checks for style attribs
    //  on an element, and not for stylesheets.  Hence,
    //  it could report incomplete information.  IE50
    //  provides better access to stylesheet info, which
    //  also needs checking.
    //--------------------------------------------------

    assert(m_pIHTMLElement);

    if (!m_pIHTMLStyle)
    {
        if (S_OK != (hr = m_pIHTMLElement->get_style( &m_pIHTMLStyle )))
            return hr;

        assert(m_pIHTMLStyle);

        if (!m_cache.IsDirty(CACHE_BSTR_STYLE))
        {
            if (S_OK != (hr = m_pIHTMLStyle->get_visibility( &m_bstrStyle )))
                return hr;

            m_cache.Dirty(CACHE_BSTR_STYLE);
        }

        if (!m_cache.IsDirty(CACHE_BSTR_DISPLAY))
        {
            if (S_OK != (hr = m_pIHTMLStyle->get_display( &m_bstrDisplay )))
                return hr;

            m_cache.Dirty(CACHE_BSTR_DISPLAY);
        }
    }

    if ( m_bstrStyle )
    {
        // UNDONE: use fast 7-bit string compare instead
        if (!_wcsicmp(m_bstrStyle, L"VISIBLE"))
        {
            *plState |= STATE_SYSTEM_INVISIBLE;
            return S_OK;
        }
    }

    //--------------------------------------------------
    //  Check the style.display property.
    //
    //  The display property has two values: "" and
    //  "none".  If display equals "", bstrStyle will
    //  be NULL after get_display().  So, if bstrStyle
    //  is non-NULL, it's value is "none" and we know
    //  that the element object is not visible.
    //--------------------------------------------------

    if ( m_bstrDisplay )
    {
        *plState |= STATE_SYSTEM_INVISIBLE;
        return S_OK;
    }

    return DISP_E_MEMBERNOTFOUND;
}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccHelp()
//
//  DESCRIPTION:
//
//      Get Help string
//
//  PARAMETERS:
//
//      lChild      child/self ID
//      pbstrHelp       help string
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccHelp(long lChild, BSTR * pbstrHelp)
{

#ifdef _DEBUG
    WCHAR   wstr[128];

    swprintf( wstr, L"PID: %lX, TID: %lX", GetCurrentProcessId(), GetCurrentThreadId() );

    *pbstrHelp = SysAllocString( wstr );

    assert( *pbstrHelp );

    return S_OK;
#else
    return DISP_E_MEMBERNOTFOUND;
#endif

}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccHelpTopic()
//
//  DESCRIPTION:
//
//      Get Help file and topic in file
//
//  PARAMETERS:
//
//      pbstrHelpFile       help file
//
//      lChild          child/self ID
//
//      pidTopic        topic index
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic)
{
    return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccKeyboardShortcut()
//
//  DESCRIPTION:
//
//      Get shortcut string
//
//  PARAMETERS:
//
//      lChild                  child/self ID 
//
//      pbstrKeyboardShortcut       returned string containing kbd shortcut.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut)
{
    return(DISP_E_MEMBERNOTFOUND);
}



//-----------------------------------------------------------------------
//  CTridentAO::GetAccDefaultAction()
//
//  DESCRIPTION:
//      returns description string for default action
//  
//  PARAMETERS:
//
//      lChild          child /self ID
//
//      pbstrDefAction  returned description string.
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccDefaultAction(long lChild, BSTR * pbstrDefAction)
{
    return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//  CTridentAO::AccSelect()
//
//  DESCRIPTION:
//      selects specified object: selection based on flags
//  
//  PARAMETERS:
//
//      flagsSel    selection flags : 
//
//          SELFLAG_NONE            = 0,
//          SELFLAG_TAKEFOCUS       = 1,
//          SELFLAG_TAKESELECTION   = 2,
//          SELFLAG_EXTENDSELECTION = 4,
//          SELFLAG_ADDSELECTION    = 8,
//          SELFLAG_REMOVESELECTION = 16
//
//      lChild      child /self ID 
//
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::AccSelect(long flagsSel, long lChild)
{
    return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//  CTridentAO::AccLocation()
//
//  DESCRIPTION:
//      returns location of specified object
//  
//  PARAMETERS:
//
//      pxLeft      left screen coordinate
//      pyTop       top screen coordinate
//      pcxWidth    screen width of object
//      pcyHeight   screen height of object
//      lChild      child/self ID
//
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
    HRESULT hr;

    long xLeft      =0;
    long yTop       =0;
    long cxWidth    =0;
    long cyHeight   =0;

    assert( pxLeft );
    assert( pyTop );
    assert( pcxWidth );
    assert( pcyHeight );

    //--------------------------------------------------
    // initialize the out parameters
    //--------------------------------------------------

    *pxLeft     = 0;
    *pyTop      = 0;
    *pcxWidth   = 0;
    *pcyHeight  = 0;

    //--------------------------------------------------
    // get screen locations
    //--------------------------------------------------

    assert(m_pIHTMLElement);

    if(!m_pIHTMLElement)
        return(E_NOINTERFACE);

    hr = m_pIHTMLElement->get_offsetLeft(&xLeft);
    if(hr != S_OK)
        return(hr);

    hr = m_pIHTMLElement->get_offsetTop(&yTop);
    if(hr != S_OK)
        return(hr);

    hr = m_pIHTMLElement->get_offsetWidth(&cxWidth);
    if(hr != S_OK)
        return(hr);

    hr = m_pIHTMLElement->get_offsetHeight(&cyHeight);
    if(hr != S_OK)
        return(hr);

    //--------------------------------------------------
    // if the width or the height are zero, the object
    // is not visible.  otherwise, assume the object is
    // visible and wait to be proven wrong.
    //--------------------------------------------------

    if ( cxWidth == 0 || cyHeight == 0 )
    {
        m_bOffScreen = TRUE;
        return S_OK;
    }

    m_bOffScreen = FALSE;

    //--------------------------------------------------
    // get offset from top left corner of HTML document.
    //--------------------------------------------------

    hr = adjustOffsetToRootParent(&xLeft,&yTop);

    if(hr != S_OK)
        return(hr);


    //--------------------------------------------------
    // adjust point to account for client area
    //--------------------------------------------------
    
    hr = adjustOffsetForClientArea(&xLeft,&yTop);

    //--------------------------------------------------
    // if the adjusted x and y values are not on the 
    // screen, toggle on screen state boolean.
    //--------------------------------------------------

    if(hr != S_OK)
    {
        if(hr == S_FALSE)
            m_bOffScreen = TRUE;
        else
            return(hr);
    }

    //--------------------------------------------------
    // Set out parameters with calculated location.
    //--------------------------------------------------

    *pxLeft     = xLeft;
    *pyTop      = yTop;
    *pcxWidth   = cxWidth;
    *pcyHeight  = cyHeight;

    return(S_OK);
}


//-----------------------------------------------------------------------
//  CTridentAO::AccDoDefaultAction()
//
//  DESCRIPTION:
//      executes default action of object.
//  
//  PARAMETERS:
//
//      lChild      child / self ID
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::AccDoDefaultAction(long lChild)
{
    return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//  CTridentAO::SetAccName()
//
//  DESCRIPTION:
//      sets name string of object
//  
//  PARAMETERS:
//
//      lChild      child / self ID 
//      pbstrName       name string
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::SetAccName(long lChild, BSTR  bstrName)
{
    return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//  CTridentAO::SetAccValue()
//
//  DESCRIPTION:
//      sets name string of object
//  
//  PARAMETERS:
//
//      lChild      child / self ID 
//      pbstrValue      name string
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::SetAccValue(long lChild, BSTR bstrValue)
{
    return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//  CTridentAO::AddChild()
//
//  DESCRIPTION:
//      Adds child element to the child list of this Accessible Object.
//      Child element can be AE or AO.
//      
//      **NOTE** insertion is at the end of the list.  No option
//      to insert at the head of the list.  
//  
//  PARAMETERS:
//
//      pAEChild    pointer to child AE or AO to be added.
//      
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
//-----------------------------------------------------------------------

HRESULT CTridentAO::AddChild(CAccElement * pChild)
{
    if(!pChild)
        return(E_INVALIDARG);

    // always insert to back of list
    m_AEList.push_back( pChild );

    return S_OK;
}


//--------------------------------------------------------------------------------
//  CTridentAO::GetFocusedTOMElementIndex()
//
//  DESCRIPTION:
//
//      Returns TOM index of currently focused element.
//      
//  
//  PARAMETERS:
//
//      puTOMIndex [out]    TOM index of focused element
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
//  NOTES:
//      
//      the caller of this method needs to validate the out parameter even
//      if the return value is good.
//--------------------------------------------------------------------------------
    
HRESULT CTridentAO::GetFocusedTOMElementIndex(UINT *puTOMIndex)
{
    assert( puTOMIndex );
    assert(m_pDocAO );

    return( m_pDocAO->GetFocusedItem( puTOMIndex ) );
}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccElemFromChildID()
//
//  DESCRIPTION:
//
//      gets an Accessible Element * from the object child list. The
//      caller needs to determine whether this is an AE or an AO.
//
//  PARAMETERS:
//
//      lChild      child ID to search for.
//
//      ppAEChild   pointer to store returned value of AE in
//
//      ppitPos     pointer to a child list iterator that gets updated
//                  with the list position of a found element. If this
//                  pointer is NULL upon entry, then it is ignored.
//
//  RETURNS:
//
//      S_OK if no errors.  If the child ID is not found, 
//      *ppAEChild is set to NULL.  The caller must check this value
//      before using the returned var.
//
//  NOTES:
//      
//      The caller of this method needs to validate the out parameter even
//      if the return value is good.
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccElemFromChildID(LONG lChild,CAccElement ** ppAEChild,
                                          std::list<CAccElement *>::iterator *ppitPos,
                                          BOOL bFullTreeSearch)
{
    std::list<CAccElement *>::iterator  itCurPos;
    HRESULT                             hr;
    CAccElement                         *pAEChild   = NULL;
    BOOL                                bChildFound = FALSE;


    assert( lChild != CHILDID_SELF );
    assert( ppAEChild );
    
    //--------------------------------------------------
    // TODO:  Determine if we really need to 
    // fully resolve child list here.
    //--------------------------------------------------

    itCurPos = m_AEList.begin();

    while( itCurPos != m_AEList.end() && !bChildFound )
    {
        pAEChild = (CAccElement*) *itCurPos;

        if ( pAEChild->GetChildID() == lChild )
            bChildFound = TRUE;
        else
        {
            //--------------------------------------------------
            // if this is a full tree search, check to see 
            // if the current element is an AO.  If it is,
            // then recurse.
            //--------------------------------------------------

            if ( bFullTreeSearch )
            {
                if ( IsAOMTypeAO(*itCurPos) )
                {
                    CTridentAO* pAO = (CTridentAO*) *itCurPos;

                    hr = pAO->GetAccElemFromChildID(lChild,ppAEChild,ppitPos,bFullTreeSearch);

                    //--------------------------------------------------
                    // if the return code is good, a match has 
                    // been found.  If return code is FALSE, keep searching.
                    // break out right away for serious errors.
                    //--------------------------------------------------

                    if ( (hr == S_OK) && (*ppAEChild) )
                    {
                        //--------------------------------------------------
                        // set the pAEChild pointer so that the object found 
                        // in the recursive search is sucessfully transferred 
                        // to the input ppAEChild parameter.  
                        //--------------------------------------------------

                        pAEChild = *ppAEChild;
                        bChildFound = TRUE;
                        break;
                    }
                    else if ( hr != S_FALSE )
                    {
                        return hr;
                    }
                }
            }
            itCurPos++;
        }
    }

    if ( bChildFound )
    {
        *ppAEChild = pAEChild;

        if ( ppitPos )
            *ppitPos = itCurPos;
    }
    else
        return( S_FALSE );

    return( S_OK );
}


//-----------------------------------------------------------------------
//  CTridentAO::GetAccElemFromTomIndex()
//
//  DESCRIPTION:
//
//      gets an Accessible Element * from the object child list. The
//      caller needs to determine whether this is an AE or an AO.
//
//  PARAMETERS:
//
//      lIndex          child TOM Index to search for.
//
//      ppAEChild       pointer to store returned value of AE in
//                                        
//
//  RETURNS:
//
//      S_OK if no errors.  If the child ID is not found, 
//      *ppAEChild is set to NULL.  The caller must check this value
//      before using the returned var.
//
//  NOTES:
//      
//      The caller of this method needs to validate the out parameter even
//      if the return value is good.
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAccElemFromTomIndex(LONG lIndex,CAccElement ** ppAEChild)
{
    HRESULT hr = E_FAIL;

    std::list<CAccElement *>::iterator  itCurPos;
    
    CAccElement     *pAEChild   = NULL;
    BOOL            bChildFound = FALSE;


    assert( ppAEChild );
 
    //--------------------------------------------------
    // check to see if this object is the matching object
    //--------------------------------------------------

    if ( lIndex == GetTOMAllIndex() )
    {   
        *ppAEChild = (CAccElement *)this;
        return( S_OK );
    }

    //--------------------------------------------------
    // search this object's child list.
    //--------------------------------------------------
    
    itCurPos = m_AEList.begin();

    while( itCurPos != m_AEList.end() && !bChildFound )
    {
        CTridentAO * pTridentAO;
        CTridentAE * pTridentAE;
        
        pAEChild = (CAccElement*) *itCurPos;
            
        
        //--------------------------------------------------
        // an item in this list is either an AO or an AE.  
        // QI to find out which one it is.
        //--------------------------------------------------

        CComQIPtr<IAccessible,&IID_IAccessible> pIAccessible(pAEChild);

        if(pIAccessible)
        {
            
            pTridentAO = (CTridentAO *)pAEChild;

            if(pTridentAO->GetTOMAllIndex() == lIndex)
                bChildFound = TRUE;

        }
        else
        {
            pTridentAE = (CTridentAE *)pAEChild;
           
            if ( pTridentAE->GetTOMAllIndex() == lIndex )
                bChildFound = TRUE;
        }

        if(!(bChildFound))
        {
            itCurPos++;
        }
    }


    if ( bChildFound )
    {
        *ppAEChild = pAEChild;
    }
    else
        return( S_FALSE );

    return( S_OK );
    
}


//-----------------------------------------------------------------------
//  CTridentAO::ScrollIntoView()
//
//  DESCRIPTION:
//      scroll ELEMENT into view
//      
//  PARAMETERS:
//
//      none.
//
//  RETURNS:
//
//      S_OK | E_FAIL | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::ScrollIntoView(void)
{
    HRESULT hr;
    VARIANT varArgStart;

    VariantInit(&varArgStart);

    assert( m_pIHTMLElement );

    if(!m_pIHTMLElement)
        return(E_NOINTERFACE);

    //--------------------------------------------------
    // scroll ELEMENT into view
    //--------------------------------------------------

    hr = m_pIHTMLElement->scrollIntoView(varArgStart);

    return(hr);
}


//-----------------------------------------------------------------------
//  CTridentAO::ensureResolvedTree()
//
//  DESCRIPTION:
//  This method ensures that the structure of the tree 
//  beneath this object has all children fully resolved (no 
//  unsupported children in immediate child list)
//
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  HRESULT S_OK | standard COM error
// ----------------------------------------------------------------------

HRESULT CTridentAO::ensureResolvedTree(void)
{
    HRESULT hr = E_FAIL;


    //--------------------------------------------------
    // no need to go any further if list is already
    // built.
    //--------------------------------------------------

    if(m_bResolvedState)
    {
        return(S_OK);
    }

    //--------------------------------------------------
    // get AOMMgr pointer
    //--------------------------------------------------
    
    CAOMMgr *pAOMMgr;
    
    if(hr = GetAOMMgr(&pAOMMgr))
    {   
        return(hr);
    }

    assert(pAOMMgr);
    

    //--------------------------------------------------
    //  resolve tree for full child building by resolving the 
    //  entire child tree beneath this object.
    //--------------------------------------------------

    if(hr = pAOMMgr->ResolveChildren(this))
    {
        return(hr);
    }

    //--------------------------------------------------
    // AOMMgr will set the resolved state : ensure 
    // that it has been set upon good return.
    //--------------------------------------------------

    assert(m_bResolvedState);
    
    return(S_OK);

}


//-----------------------------------------------------------------------
//  CTridentAO::GetAOMMgr()
//
//  DESCRIPTION:
//  returns AOMMgr.  This method is overridden in the CDocumentAO and 
//  CWindowAO classes to provide access to the CAOMMgr member of CWindowAO
//
//  PARAMETERS:
//  ppAOMMgr        pointer to return AOMMgr in.
//
//  RETURNS:
//
//  S_OK | E_FAIL
// ----------------------------------------------------------------------

HRESULT CTridentAO::GetAOMMgr(CAOMMgr ** ppAOMMgr)
{
    assert(ppAOMMgr);

    return(m_pDocAO->GetAOMMgr(ppAOMMgr));

}


//-----------------------------------------------------------------------
//  CTridentAO::GetChildList()
//
//  DESCRIPTION:
//  returns pointer to the child list of the TridentAO.
//
//  PARAMETERS:
//
//  ppChildList     pointer to return child list in.
//
//  RETURNS:
//
//  S_OK | std COM error.
// ----------------------------------------------------------------------


HRESULT CTridentAO::GetChildList(std::list<CAccElement *> **ppChildList)
{
    assert(ppChildList);

    *ppChildList = &m_AEList;

    return(S_OK);
        
}



//-----------------------------------------------------------------------
//  CTridentAO::IsPointInTextChildren()
//
//  DESCRIPTION:
//
//      return a CTextAE in the out param if one exists that contains the point. 
//
//  PARAMETERS:
//
//      xLeft           [in ] x param of point
//      yTop            [in ] y param of point
//      ppIUnk          [out] Pointer to a pointer to the matched CTextAE.
//
//  RETURNS:
//
//      HRESULT         S_OK if the hit element was found, else
//                      S_FALSE if no text elements contained the point, 
//                      elsestandard COM error.
//
//  NOTES:
//
//  this method ASSUMES that the points have already been converted
//  to client coordinates. It converts them back to
//  screen coordinates for the compare against the 
//   TextAEs bounding rect.
//      
//-----------------------------------------------------------------------

HRESULT CTridentAO::IsPointInTextChildren(  long xLeft,
                                            long yTop,
                                            IHTMLTxtRange * pDocTxtRange,
                                            CAccElement **ppAccEl)
{
    HRESULT hr              = E_FAIL;
    long iListSize          = 0;
    long iCurCount          = 0;
    CAccElement * pAccEl    = NULL;
    CTridentAE * pAE        = NULL;
    std::list<CAccElement *>::iterator itCurPos;
    BOOL bInTextRange       = FALSE;
    short sExpand           = 0;
    CTextAE * pTextAE       = NULL;

    //--------------------------------------------------
    // validate inputs : if there is no text 
    // range to validate on, return S_FALSE (no, 
    // point is not in text children)
    //--------------------------------------------------

    
    if(!pDocTxtRange)
        return(S_FALSE);

    assert( ppAccEl );

    if(!ppAccEl )
        return(E_INVALIDARG);


    *ppAccEl = NULL;

    
    //--------------------------------------------------
    // if we haven't fully resolved the child list of 
    // this parent, don't go any further. All text 
    // children need to exist at this point, and the
    // only time that they all exist is after the child
    // list has been fully resolved.
    //--------------------------------------------------

    if(!m_bResolvedState)
    {
        return(S_FALSE);
    }

    
    //--------------------------------------------------
    // move the input text range to the input point.
    //
    // TODO REVIEW: we can reduce the parameter list
    // by moving the input text range to the input
    // point BEFORE calling this method, but wouldn't
    // that change the name of this method ? Passing 
    // in a point seems clearer.
    //--------------------------------------------------

    if(hr = pDocTxtRange->moveToPoint(xLeft,yTop))
    {
        return(hr);
    }

    //--------------------------------------------------
    // try to expand the insertion point by one char in 
    // order to resolve ambiguity.  Even if expansion 
    // fails, as long as it doesn't return an error,
    // we have done what we can to resolve ambiguity.
    //--------------------------------------------------

    // TODO: make bstr global to component

    BSTR bstr = SysAllocString( L"character" );

    if(hr = pDocTxtRange->expand(bstr,&sExpand) )
    {
        SysFreeString( bstr );
        return(hr);
    }

    SysFreeString( bstr );

    //--------------------------------------------------
    // iterate throught all children in the list, looking
    // for text children.
    //--------------------------------------------------

    itCurPos    = m_AEList.begin();
    iListSize   = m_AEList.size();

    for(iCurCount = 0; iCurCount < iListSize; iCurCount ++)
    {

        assert(itCurPos != m_AEList.end());

        pAccEl = *itCurPos;

        if(pAccEl->GetAOMType() == AOMITEM_TEXT)
        {
            pTextAE = (CTextAE *)pAccEl;

            if(hr = pTextAE->ContainsPoint(xLeft,yTop,pDocTxtRange))
            {
                if(hr != S_FALSE)
                    return(hr);
            }
            else
            {

                //--------------------------------------------------
                // the textAE contained the input text range : 
                // does its bounding rect contain the point ?
                // so its the hit object.
                //--------------------------------------------------

                *ppAccEl = pTextAE;
                return(S_OK);
            }
        }

        itCurPos++;
    }

    //--------------------------------------------------
    // reaching here means that no elements contained
    // the point.
    //--------------------------------------------------

    return(S_FALSE);
}


//-----------------------------------------------------------------------
//  CTridentAO::IsAncestorAnchor()
//
//  DESCRIPTION:
//      Walks parent chain starting at object looking for an CAnchorAO.
//
//      This method looks for a "near" CAnchorAO ancestor.  In this
//      context, near means that when looking up the parent chain, an
//      anchor is encountered before any one of the following objects:
//      area, button, checkbox, document, editfield, frame, image,
//      imagebutton, plugin, radiobutton, selectlist, table, tablecell,
//      or window.
//
//
//  PARAMETERS:
//      ppAnchorAncestor    [out]   pointer to a CAnchorAO*
//
//  RETURNS:
//      HRESULT     S_OK if object has a "near" anchor ancestor (out param
//                      will be set) and no error occurred, or
//                  S_FALSE if object doesn't have anchor ancestor and
//                      no error occurred, or
//                  standard COM error
//-----------------------------------------------------------------------

HRESULT
CTridentAO::IsAncestorAnchor( /* out */ CAnchorAO** ppAnchorAncestor )
{
    HRESULT     hr = S_OK;
    CTridentAO* pAO = this;

    assert( ppAnchorAncestor );
    *ppAnchorAncestor = NULL;

    do {
        if ( !pAO->GetParent() )
            hr = S_FALSE;
        else
            switch ( pAO->GetAOMType() )
            {
                case AOMITEM_ANCHOR:
                    *ppAnchorAncestor = (CAnchorAO*) pAO;
                    break;

                case AOMITEM_AREA:
                case AOMITEM_BUTTON:
                case AOMITEM_CHECKBOX:
                case AOMITEM_EDITFIELD:
                case AOMITEM_FRAME:
                case AOMITEM_IMAGE:
                case AOMITEM_PLUGIN:
                case AOMITEM_RADIOBUTTON:
                case AOMITEM_SELECTLIST:
                case AOMITEM_TABLE:
                case AOMITEM_TABLECELL:
                case AOMITEM_IMAGEBUTTON:
                case AOMITEM_DOCUMENT:
                case AOMITEM_WINDOW:
                    hr = S_FALSE;
                    break;

                default:
                    pAO = pAO->GetParent();
                    break;
            }
    } while ( !*ppAnchorAncestor && hr == S_OK );

#ifdef _DEBUG
    if ( hr == S_OK )
        assert( *ppAnchorAncestor );
    else
        assert( !*ppAnchorAncestor );
#endif

    return hr;
}


//================================================================================
// CTridentAO protected methods implementation
//================================================================================


//-----------------------------------------------------------------------
//  CTridentAO::createInterfaceImplementors()
//
//  DESCRIPTION:
//      Creates the member objects that implement the IAccessible
//      and IOleWindow interfaces for the CTridentAO-derived object.
//      if they do not exist
//
//      If a derived class does not call CTridentAO::Init(), it
//      must call this method to create the CImplIAccessible
//      and the CImplIOleWindow members.  (CTridentAO::Init()
//      calls this method internally.)
//
//  PARAMETERS:
//      pIUnk       LPUNKNOWN to controlling object (NULL by default).
//
//  RETURNS:
//      HRESULT     S_OK | E_OUTOFMEMORY
//
//-----------------------------------------------------------------------

HRESULT 
CTridentAO::createInterfaceImplementors( LPUNKNOWN pIUnk )
{
    // create IAccessible implementor
    if ( !m_pImplIAccessible )
        m_pImplIAccessible = new CImplIAccessible( ( pIUnk ? pIUnk : this ), this );

    if ( !m_pImplIOleWindow )
        m_pImplIOleWindow = new CImplIOleWindow( ( pIUnk ? pIUnk : this ), this );

    return ( m_pImplIAccessible && m_pImplIOleWindow ) ? S_OK : E_OUTOFMEMORY;
}



//-----------------------------------------------------------------------
//  CTridentAO::freeDataMembers()
//
//  DESCRIPTION:
//  frees all allocated data members, NOT child list.
//
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  none.
//
// ----------------------------------------------------------------------

void 
CTridentAO::freeDataMembers(void)
{
    //------------------------------------------------
    // contained object cleanup. Set to NULL for safety
    //------------------------------------------------

    if(m_pImplIAccessible)
    {
        delete m_pImplIAccessible;
        m_pImplIAccessible = NULL;
    }

    if(m_pImplIOleWindow)
    {
        delete m_pImplIOleWindow;
        m_pImplIOleWindow=NULL;
    }
    
    //--------------------------------------------------
    // free all allocated strings. Set to NULL for safety
    //--------------------------------------------------

    if(m_bstrName)
    {
        SysFreeString(m_bstrName);
        m_bstrName=NULL;
    }

    if(m_bstrValue)
    {
        SysFreeString(m_bstrValue);
        m_bstrValue = NULL;
    }
    
    if(m_bstrDescription)
    {
        SysFreeString(m_bstrDescription);
        m_bstrDescription = NULL;
    }

    if(m_bstrDefaultAction)
    {
        SysFreeString(m_bstrDefaultAction);
        m_bstrDefaultAction = NULL;
    }
    
    if(m_bstrKbdShortcut)
    {
        SysFreeString(m_bstrKbdShortcut);
        m_bstrKbdShortcut = NULL;
    }

    if(m_bstrStyle)
    {
        SysFreeString(m_bstrStyle);
        m_bstrStyle = NULL;
    }

    if(m_bstrDisplay)
    {
        SysFreeString(m_bstrDisplay);
        m_bstrDisplay = NULL;
    }

    //--------------------------------------------------
    // release allocated interface pointer
    //--------------------------------------------------

    ReleaseTridentInterfaces();
}

//-----------------------------------------------------------------------
//  CTridentAO::freeChildren()
//
//  DESCRIPTION:
//      walks tree, deletes children, frees tree.
//  
//  PARAMETERS:
//
//      none.
//      
//
//  RETURNS:
//
//      HRESULT S_OK | E_FAIL | E_INVALIDARG |DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::freeChildren(void)
{
    std::list<CAccElement *>::iterator itCurPos;
    
    if(m_AEList.size())
    {
        itCurPos = m_AEList.begin();

        while(itCurPos != m_AEList.end())
        {
            // call virtual destructor here.
            (*itCurPos)->Detach();
            itCurPos++;
        }

        // free list.
        m_AEList.erase(m_AEList.begin(),m_AEList.end());
    }

    return S_OK;
}


//-----------------------------------------------------------------------
//  CTridentAO::createEnumerator()
//
//  DESCRIPTION:
//
//      Create a VARIANT enumerator that contains one VARIANT structure
//      for each child of CAccClient.
//
//  PARAMETERS:
//
//      ppcenum         [out] Double pointer to the object that
//                            implements the VARIANT enumerator.
//
//      pAccElemList    [in]  Pointer to list of CAccElements
//                            to add into the enumerator. If NULL, the
//                            CTridentAO's internal child list is used.
//
//  RETURNS:
//
//      HRESULT         S_OK if the enumerator was created and
//                        initialized successfully, COM error code
//                        otherwise.
//-----------------------------------------------------------------------

HRESULT CTridentAO::createEnumerator( CEnumVariant** ppcenum, std::list<CAccElement *> *pAccElemList )
{
    HRESULT         hr          = E_FAIL;
    VARIANT         var;
    SAFEARRAY FAR*  psa         = NULL;
    SAFEARRAYBOUND  rgsbnd[1];
    long            ix[1];
    int             i           = 0;
    CAccElement *   pAEChild    = NULL;
    
    
    assert ( ppcenum );

    *ppcenum = NULL;
    
    //--------------------------------------------------
    // the child list has to be resolved before it can 
    // be returned.
    //--------------------------------------------------

    if(hr = ensureResolvedTree())
    {
        return(hr);
    }

    std::list<CAccElement *> *pList;
    std::list<CAccElement *>::iterator itCurPos;


    pList = (pAccElemList != NULL) ? pAccElemList : &m_AEList;

    //-----------------------------------------------------
    //  Create a SAFEARRAY to hold the child ID VARIANTs.
    //    Note that the number of elements of the array
    //    is the number of children of CAccClient.
    //-----------------------------------------------------

    rgsbnd[0].lLbound   = 0;
    rgsbnd[0].cElements = pList->size();

    psa = SafeArrayCreate( VT_VARIANT, 1, rgsbnd );

    if ( psa == NULL )
        return( E_OUTOFMEMORY );

    //-----------------------------------------------------
    //  For each child, set the VARIANT's lVal member
    //    to the child's external (Accessible) ID and
    //    add an element to the SAFEARRAY.
    //-----------------------------------------------------

    itCurPos = pList->begin();

    while( itCurPos != pList->end() )
    {
        VariantInit(&var);

        pAEChild = (CAccElement*) *itCurPos;

        //--------------------------------------------------
        //  Determine whether the child is an AE or an AO
        //  by checking its CAccElement AOM type.
        //--------------------------------------------------

        if ( IsAOMTypeAE( pAEChild ) )
        {
            //--------------------------------------------------
            // Load child IDs for all AEs
            //--------------------------------------------------

            var.vt = VT_I4;
            var.lVal = pAEChild->GetChildID();
        }
        else
        {
            if ( pAEChild->GetChildID() == GetChildID() )
            {
                //--------------------------------------------------
                // if ChildID() = m_nChildID(), return VT_I4 w/
                // CHILDID_SELF
                //--------------------------------------------------

                var.vt = VT_I4;
                var.lVal = CHILDID_SELF;
            }
            else
            {
                //--------------------------------------------------
                // Load LPDISPATCHs for all AOs
                //--------------------------------------------------

                var.vt = VT_DISPATCH;
                var.pdispVal = (LPDISPATCH)(LPACCESSIBLE)pAEChild;
            }
        }

        //--------------------------------------------------
        // Set the SAFEARRAY index and increment for 
        //  the next time.
        //--------------------------------------------------

        ix[0] = i++;

        //--------------------------------------------------
        // Store the variant in the safe array : data is 
        //  copied, not allocated for SAFEARRAYs
        //--------------------------------------------------

        hr = SafeArrayPutElement( psa, ix, &var );

        if ( hr!= S_OK )
        {
            SafeArrayDestroy( psa );
            return( hr );
        }

        itCurPos++;
    }

    //-----------------------------------------------------
    //  Create the enumerator.
    //-----------------------------------------------------

    hr = CEnumVariant::Create( psa, rgsbnd[0].cElements, ppcenum );

    if ( hr == S_OK )
    {
        assert( *ppcenum );

        ((LPUNKNOWN)(*ppcenum))->AddRef();
    }

    //-----------------------------------------------------
    //  Clean up by destroying the SAFEARRAY.
    //-----------------------------------------------------

    SafeArrayDestroy( psa );

    return( hr );
}



//-----------------------------------------------------------------------
//  CTridentAO::getSelectedChildren()
//
//  DESCRIPTION:
//
//      returns the id or the IDispatch * to the selected object (if
//      single selection), or the IUnknown * to the IEnumVariant object
//      that contains the VARIANTs of the selected objects.
//          
//  PARAMETERS:
//      
//  ppIUnknown  - pointer to IUnknown of CAccElement, CAccObject,
//                  or IEnumVariant.  QI on this interface to find out what
//                  type of object was selected.
//
//  RETURNS:
//
//      HRESULT S_OK | S_FALSE | E_FAIL | E_INVALIDARG 
// ----------------------------------------------------------------------

HRESULT CTridentAO::getSelectedChildren( IUnknown** ppIUnknown )
{
    HRESULT     hr;
    BOOL        bIsSelected;
    long        iCurCount, iListSize;
    CAccElement *pAccEl;

    std::list<CAccElement *>            selectionList;
    std::list<CAccElement *>::iterator  itCurPos;


    itCurPos  = m_AEList.begin();
    iListSize = m_AEList.size();


    //------------------------------------------------
    //  Iterate through child list to get the selected
    //  children.
    //------------------------------------------------

    for (iCurCount = 0; iCurCount < iListSize; iCurCount++, itCurPos++)
    {
        bIsSelected = FALSE;

        pAccEl = *itCurPos;

        //--------------------------------------------------
        //  Is the current child an AE or an AO?
        //  Check the AOM type.
        //--------------------------------------------------

        if ( IsAOMTypeAE( pAccEl ) )
        {
            //--------------------------------------------------
            //  Only AEs supported are CTextAEs.
            //--------------------------------------------------

            assert( pAccEl->GetAOMType() == AOMITEM_TEXT );

            hr = m_pDocAO->IsTextRangeSelected( ((CTextAE*)pAccEl)->GetTextRangePtr(), &bIsSelected );
        }
        else
        {
            //--------------------------------------------------
            //  Special case controls here (radio buttons, check
            //  boxes, edit fields, buttons, image buttons,
            //  etc.) because they can show up in the Trident
            //  selection range, but for MSAA purposes, they
            //  shouldn't be selected.
            //--------------------------------------------------

            if ( IsAOMTypeControl( pAccEl ) )
            {
                bIsSelected = FALSE;
                hr = S_OK;
            }
            else
                hr = m_pDocAO->IsTEOSelected( ((CTridentAO*)pAccEl)->GetTEOIHTMLElement(), &bIsSelected );
        }

        if ( hr != S_OK )
            break;
        else if ( bIsSelected )
        {
            selectionList.push_back( pAccEl );
        }
    }


    if ( hr != S_OK )
        return hr;
    else if ( !selectionList.size() )
        return E_UNEXPECTED;
    else
    {
        //------------------------------------------------
        // Single item selected?
        //------------------------------------------------

        if ( selectionList.size() == 1 )
        {
            itCurPos    = selectionList.begin();
            *ppIUnknown = LPUNKNOWN(*itCurPos); 
        }

        //------------------------------------------------
        // Multiple items selected?
        //------------------------------------------------

        else
        {
            CEnumVariant    *pEnum;
            
            hr = createEnumerator( &pEnum, &selectionList );

            if ( FAILED( hr ) )
                return( hr );
                
            *ppIUnknown = (LPUNKNOWN)pEnum;
        }
    }

    return hr;
}



//-----------------------------------------------------------------------
//  CTridentAO::getTitleFromIHTMLElement()
//
//  DESCRIPTION:
//        this method gets the title, but the contents of that title
//        need to be validated by the caller.
//  
//  PARAMETERS:
//
//      pbstrTitle.
//
//  RETURNS:
//
//      S_OK if title call succeeds, else standard COM error
//
//  NOTES:
//      
//      the caller of this method needs to validate the out parameter even
//      if the return value is good.
// ----------------------------------------------------------------------

HRESULT CTridentAO::getTitleFromIHTMLElement(BSTR *pbstrTitle)
{
    HRESULT hr  = E_FAIL;

    assert( pbstrTitle );
    assert( m_pIHTMLElement );

    if(!m_pIHTMLElement)
        return(E_NOINTERFACE);

    hr = m_pIHTMLElement->get_title(pbstrTitle);

    return(hr);
}


//--------------------------------------------------------------------------------
//  CTridentAO::adjustOffsetToRootParent()
//
//  DESCRIPTION:
//      adjust input points to root parent window
//  
//  PARAMETERS:
//
//      pxLeft  - pointer to x coord
//      pyTop   - pointer to y coord
//
//  RETURNS:
//
//      S_OK | E_FAIL | E_NOINTERFACE
//
//--------------------------------------------------------------------------------

HRESULT CTridentAO::adjustOffsetToRootParent(long * pxLeft,long * pyTop)
{
    HRESULT hr;
    long xLeft =        * pxLeft;
    long yTop =         * pyTop;
    IHTMLElement        * pIHTMLElement = NULL;
    IHTMLElement        * pIHTMLParentElement = NULL;
    IHTMLControlElement * pControlElem = NULL;
    long                  lTempLeft = 0;
    long                  lTempTop  = 0;
    long                  lClientLeft = 0;
    long                  lClientTop  = 0;

    assert( pxLeft );
    assert( pyTop );
    assert(m_pIHTMLElement);

    //--------------------------------------------------
    // now adjust by parent coordinates : move up parent
    // list to the root and add left, top parent coords.
    //-------------------------------------------------

    pIHTMLElement = m_pIHTMLElement;
    pIHTMLElement->AddRef();


    while ( (hr = pIHTMLElement->get_offsetParent( &pIHTMLParentElement )) == S_OK )
    {
        //--------------------------------------------------
        // method can return S_OK, with NULL parent, so 
        // check for valid parent
        //--------------------------------------------------

        if ( pIHTMLParentElement == NULL )
            break;

        //--------------------------------------------------
        // get the offsetLeft and offsetTop metrics of
        // the parent
        //--------------------------------------------------

        hr = pIHTMLParentElement->get_offsetLeft( &lTempLeft );
        if ( hr != S_OK )
            goto CleanUp;
        
        hr = pIHTMLParentElement->get_offsetTop( &lTempTop );
        if ( hr != S_OK )
            goto CleanUp;

        //--------------------------------------------------
        // Get the clientTop and clientWidth ... to adjust 
        //   for border thickness
        //--------------------------------------------------

        hr = pIHTMLParentElement->QueryInterface(IID_IHTMLControlElement,
                                                 (VOID**) &pControlElem);
        if ( hr == S_OK )
        {
            if ( !pControlElem )
                goto CleanUp;

            hr = pControlElem->get_clientTop( &lClientTop );
            if (hr != S_OK)
                goto CleanUp;

            hr = pControlElem->get_clientLeft( &lClientLeft);
            if (hr != S_OK)
                goto CleanUp;

            pControlElem->Release();
            pControlElem=NULL;
        }
        else
            goto CleanUp;

        xLeft += lTempLeft + lClientLeft;
        yTop  += lTempTop  + lClientTop;

        //--------------------------------------------------
        // make the parent the child
        //--------------------------------------------------

        pIHTMLElement->Release();
        pIHTMLElement = pIHTMLParentElement;
        pIHTMLParentElement = NULL;
    }

    *pxLeft = xLeft;
    *pyTop  = yTop;


CleanUp:
    if ( pIHTMLElement )
        pIHTMLElement->Release();

    if ( pIHTMLParentElement )
        pIHTMLParentElement->Release();

    if (pControlElem)
        pControlElem->Release();

    return hr;
}



//--------------------------------------------------------------------------------
//  CTridentAO::adjustOffsetForClientArea()
//
//  DESCRIPTION:
//      modify input point to account for client area. If it is above/below/right/left
//      of current client area, then return S_FALSE, 
//  
//  PARAMETERS:
//
//      pxLeft      - pointer to x coord
//      pyTop       - pointer to y coord
//      
//
//  RETURNS:
//
//      S_OK | E_FAIL | E_NOINTERFACE
//      S_FALSE if point is not in current client area

//--------------------------------------------------------------------------------

HRESULT CTridentAO::adjustOffsetForClientArea(long *pxLeft,long *pyTop)
{
    HRESULT hr;
    long    xLeft   = *pxLeft;
    long    yTop    = *pyTop;

    //--------------------------------------------------
    // get the scroll offsets : 
    //--------------------------------------------------
    assert( m_pDocAO );

    POINT ptScrollOffset;

    hr = m_pDocAO->GetScrollOffset(&ptScrollOffset);

    if(hr != S_OK)
        return(hr);

    //--------------------------------------------------
    // subtracting the scroll offsets from the total
    // offset will give the position of the element 
    // in the client area of the owner document
    //--------------------------------------------------

    xLeft   -= ptScrollOffset.x;
    yTop    -= ptScrollOffset.y;

    //--------------------------------------------------
    // determine if xLeft or yTop is to the right 
    // | below the current client area.
    // if unadjusted value > scroll offset + 
    // client (width | height) then the point is right 
    // | below the current client area
    //--------------------------------------------------

    long lDocLeft    =0;
    long lDocTop     =0;
    long lDocWidth   =0;
    long lDocHeight  =0;
    long lcxWidth    =0;
    long lcyHeight   =0;

    //--------------------------------------------------
    // the document accLocation defers to the body to return 
    //   its offset properties. This is outside of the bodies
    //   borders and so we can safely add the top,left to our
    //   point, without duplicating the border thickness
    //
    //--------------------------------------------------

    hr = m_pDocAO->AccLocation(&lDocLeft,
                               &lDocTop,
                               &lDocWidth,
                               &lDocHeight,
                               CHILDID_SELF);

    if(hr != S_OK)
        return(hr);

    //--------------------------------------------------
    // adjusting xLeft and yTop by the document location
    // parameters gives its true position in relation 
    // to the root document.
    //--------------------------------------------------

    xLeft += lDocLeft;
    yTop  += lDocTop;

    //--------------------------------------------------
    // compare the 'fully adjusted' xLeft and yTop with
    // the 'fully adjusted' client area.  If either
    // of these expressions evaluate to TRUE, that 
    // means that the point is offscreen.
    //
    // unfortunately this requires that we get the width 
    // and height
    //--------------------------------------------------

    if(m_pIHTMLElement)
    {
        hr = m_pIHTMLElement->get_offsetWidth(&lcxWidth);
        if(hr != S_OK)
            return(hr);

        hr = m_pIHTMLElement->get_offsetHeight(&lcyHeight);
        if(hr != S_OK)
            return(hr);
    }

    if( (xLeft > lDocLeft + lDocWidth) ||  // off the right
        (yTop  > lDocTop + lDocHeight) ||  // off the bottom
        (xLeft + lcxWidth  < lDocLeft) ||  // off to the left
        (yTop  + lcyHeight < lDocTop)   )  // off to the top
    {
        hr = S_FALSE;
    }

    *pxLeft = xLeft;
    *pyTop  = yTop;

    return hr;
}

//-----------------------------------------------------------------------
//  CTridentAO::click()
//
//  DESCRIPTION:
//      this method calls the 'click' method of the IHTMLElement.
//      the 'click' action is the default action for many AOs
//  
//  PARAMETERS:
//
//      none..
//
//  RETURNS:
//
//      S_OK if title call succeeds, else standard COM error
//
// ----------------------------------------------------------------------

HRESULT CTridentAO::click(void)
{
    if ( !m_pIHTMLElement )
        return E_NOINTERFACE;

    return m_pIHTMLElement->click();
}


//-----------------------------------------------------------------------
//  CTridentAO::getVisibleCorner()
//
//  DESCRIPTION:
//  gets the corner of the element that is currently onscreen.
//  if the object is entirely onscreen, the top left corner is returned.
//  otherwise, the following corners are given in order :
//  
//  top left:   returned for the following conditions:
//              only top left is onscreen | 
//              entire object is onscreen |
//              top left and top right are onscreen |
//              top left bottom left are onscreen.
//
//  bottom left returned for the following conditions:
//              only bottom left is onscreen |
//              bottom left and bottom right are onscreen.
//
//  top right   returned for the following conditions: 
//              only top right is onscreen |
//              top right and bottom right are onscreen.
//
//  bottom right returned for the following conditions: 
//               only bottom right is onscreen. 
//
//
//  PARAMETERS:
//
//  pPt         pointer to POINT structure to store point value in.
//  pdwCorner   pointer to DWORD to contain bitmask in.  Bitmask 
//              corresponds to which corner of the window is present, 
//
//  RETURNS:
//
//  S_OK if a corner returned, S_FALSE if entire object is offscreen, 
//  else  standard COM error.
// ----------------------------------------------------------------------

HRESULT CTridentAO::getVisibleCorner(POINT * pPt,DWORD  *pdwCorner)
{
    HRESULT hr          = E_FAIL;

    long xLeft          = 0;
    long yTop           = 0;
    long xRight         = 0;
    long yBottom        = 0;
    long cxWidth        = 0;
    long cyHeight       = 0;
    long xDocLeft       = 0;
    long yDocTop        = 0;
    long cxDocWidth     = 0;
    long cyDocHeight    = 0;
    long xClientLeft    = 0;
    long yClientTop     = 0;
    
    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert(pPt);
    assert(pdwCorner);

    //--------------------------------------------------
    // get accLocation to determine whether object
    // is completely offscreen. **ENSURE THAT DEFAULT
    // METHOD IS USED **
    //--------------------------------------------------

    if(hr = CTridentAO::AccLocation(&xLeft,&yTop,&cxWidth,&cyHeight,0))
        return(hr);

    if(!xLeft || !yTop || !cxWidth || !cyHeight)
        return(S_FALSE);

    //--------------------------------------------------
    // calculate right and bottom.
    //--------------------------------------------------

    xRight = xLeft + cxWidth;

    yBottom = yTop + cyHeight;

    //--------------------------------------------------
    // get location of document to see if any of the 
    // returned location coords for this object are 
    // offscreen.
    //--------------------------------------------------

    if(hr  = m_pDocAO->AccLocation(&xDocLeft,&yDocTop,&cxDocWidth,&cyDocHeight,0))
        return(hr);

    //--------------------------------------------------
    // is xLeft offscreen ?
    //--------------------------------------------------
    
    if((xLeft - xDocLeft ) < 0)
    {

        assert((xRight - xDocLeft) >= 0);
        
        
        //--------------------------------------------------
        // is xRight offscreen ? If its not, then
        // set the corner bitmask to contain xRight.  If
        // both left and right are offscreen, then we 
        // assume that the middle is visible. (this will 
        // break in situations where the element is so large
        // that all corners and the middle are offscreen, but 
        // this is rare.)
        //--------------------------------------------------

        if((xDocLeft + cxDocWidth - xRight) > 0)
            *pdwCorner |= POINT_XRIGHT;
        else
            *pdwCorner |= POINT_XMID;
    }
    else
        *pdwCorner |= POINT_XLEFT;

    
    if((yTop - yDocTop) < 0)
    {
        assert((yBottom - yDocTop) >= 0);


        //--------------------------------------------------
        // is yBottom offscreen ? If not, then set the corner
        // bitmask to contain yBottom.  If both top and bottom
        // are offscreen, then set point value to middle of element.
        // (this will break in situations where the element is so large
        // that all corners and the middle are offscreen, but 
        // this is rare.)
        //--------------------------------------------------
        
        if((yDocTop + cyDocHeight - yBottom ) > 0)
            *pdwCorner |= POINT_YBOTTOM;
        else
            *pdwCorner |= POINT_YMID;
    }
    else
        *pdwCorner |= POINT_YTOP;


    //--------------------------------------------------
    // get the internal border offsets if they exist.
    //--------------------------------------------------

    CComQIPtr<IHTMLControlElement,&IID_IHTMLControlElement> pIHTMLControlElement(m_pTOMObjIUnk);
    

    if(pIHTMLControlElement)
    {
        if(hr = pIHTMLControlElement->get_clientTop(&xClientLeft))
            return(hr);

        if(hr = pIHTMLControlElement->get_clientLeft(&yClientTop))
            return(hr);
    }

    //--------------------------------------------------
    // set the point value based on which
    // corner is visible.
    //--------------------------------------------------
    
    switch(*pdwCorner & POINT_XMASK)
    {
    case POINT_XLEFT:
        pPt->x = xLeft + xClientLeft;
        break;
    case POINT_XRIGHT:
        pPt->x = xRight - xClientLeft;
        break;
    case POINT_XMID:
        pPt->x = (xRight - xLeft)/2;
        break;
    }   
    
    switch(*pdwCorner & POINT_YMASK  )
    {
    case POINT_YTOP:
        pPt->y = yTop + yClientTop;
        break;
    case POINT_YBOTTOM:
        pPt->y = yBottom - yClientTop;
        break;
    case POINT_YMID:
        pPt->y = (yBottom - yTop)/2;
    }

    return(S_OK);

}


//-----------------------------------------------------------------------
//  CTridentAO::resolveNameAndDescription()
//
//  DESCRIPTION:
//
//      The accName and accDescription properties for the object
//      are based on the TITLE attribute of the associated TEO and
//      the string returned by getDescriptionString(), respectively.
//
//  PARAMETERS:
//
//      None.
//
//  RETURNS:
//
//      HRESULT
//
//-----------------------------------------------------------------------

HRESULT CTridentAO::resolveNameAndDescription( void )
{
    HRESULT hr = E_FAIL;
    BSTR    bstrTmp = NULL;


    //------------------------------------------------
    //  Initialize affected member variables.
    //------------------------------------------------

    m_bNameAndDescriptionResolved = FALSE;

    if ( m_bstrName )
    {
        SysFreeString( m_bstrName );
        m_bstrName = NULL;
    }

    if ( m_bstrDescription )
    {
        SysFreeString( m_bstrDescription );
        m_bstrDescription = NULL;
    }


    //------------------------------------------------
    //  Get accName first, then accDescription.
    //------------------------------------------------

    //--------------------------------------------------
    //  If the TITLE property is defined, use it for
    //  the object's name.
    //--------------------------------------------------

    hr = getTitleFromIHTMLElement( &bstrTmp );

    if ( hr != S_OK )
        return hr;

    if ( bstrTmp )
    {
        m_bstrName = SysAllocString( bstrTmp );
        SysFreeString( bstrTmp );

        //--------------------------------------------------
        //  The accName property has been cached, now let's
        //  try to get the accDescription.
        //
        //  Call the, possibly overriden, protected member
        //  method, getDescriptionString(), for the object's
        //  description.
        //--------------------------------------------------

        bstrTmp = NULL;

        hr = getDescriptionString( &bstrTmp );

        if ( hr != S_OK )
        {
            SysFreeString( m_bstrName );
            return hr;
        }
        else
        {
            if ( bstrTmp )
            {
                m_bstrDescription = SysAllocString( bstrTmp );
                SysFreeString( bstrTmp );
            }

            m_bNameAndDescriptionResolved = TRUE;
        }
    }

    else
    {
        //--------------------------------------------------
        //  The object's TEO doesn't have a TITLE attribute,
        //  so use the object's description string, if it
        //  exists, for the object's name.  Note that this
        //  means that the object's description will be empty.
        //--------------------------------------------------

        hr = getDescriptionString( &bstrTmp );

        if ( hr == S_OK )
        {
            if ( bstrTmp )
            {
                m_bstrName = SysAllocString( bstrTmp );
                SysFreeString( bstrTmp );
            }

            m_bNameAndDescriptionResolved = TRUE;
        }
    }


    return hr;
}



//-----------------------------------------------------------------------
//  CTridentAO::getDescriptionString()
//
//  DESCRIPTION:
//
//      Obtains the text of the associated TEO's INNERTEXT property.
//      This text will be used as the accDescription of the derived
//      object.
//
//  PARAMETERS:
//
//      pbstrDescStr    [out]   pointer to the BSTR to hold the INNERTEXT
//
//  RETURNS:
//
//      HRESULT
//
//  NOTES:
//
//      This virtual method should be overriden in derived classes
//      that 1) have interdependent accName and accDescription values
//      and 2) don't use the INNERTEXT of their TEO as their
//      accDescription.
//
//-----------------------------------------------------------------------

HRESULT CTridentAO::getDescriptionString( BSTR* pbstrDescStr )
{
    HRESULT hr;


    assert( pbstrDescStr );
    *pbstrDescStr = NULL;

    if ( !m_pIHTMLElement )
        hr = E_NOINTERFACE;
    else
        hr = m_pIHTMLElement->get_innerText( pbstrDescStr );

    return hr;
}

//----  End of TRID_AO.CPP  ----
