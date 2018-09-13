//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccBase.Cxx
//
//  Contents:   Accessible base object implementation
//
//----------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_ACCBASE_HXX_
#define X_ACCBASE_HXX_
#include "accbase.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

MtDefine(CAccBase, ObjectModel, "CAccBase")

//+----------------------------------------------------------------
//
//  member : classdesc
//
//  Synopsis : CBase Class Descriptor Structure
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CAccBase::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IAccessible,               // _piidDispinterface
    NULL,                           // _apHdlDesc
};


//----------------------------------------------------------------------------
//  IUnknown Interface Implementation
//----------------------------------------------------------------------------

//-----------------------------------------------------------------------
//  CAccBase::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CAccBase object implements
//      IDispatch and IAccessible.
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
//  TODO    : Add IEnumVariant.
// ----------------------------------------------------------------------
STDMETHODIMP 
CAccBase::PrivateQueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr = S_OK;
    
    if ( !ppv )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
  
    if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown *)((IPrivateUnknown *)this);
    }
    else if (riid == IID_IDispatch) 
    {
        *ppv = (IDispatch *)((IAccessible *)this);
    }
    else if ( riid == IID_IAccessible )
    {
        *ppv = (IAccessible *)this;
    }
    /*
    else if ( riid == IID_IEnumVARIANT )
    {
        // Get the enumerator for this instance. We don't
        // keep a reference for the enumerator, so we
        // don't have to AddRef it below.

        // BOLLOCKS - Breaks COM Rules
        hr = THR( GetEnumerator( (IEnumVARIANT **)ppv ) );
    }
    */
    else
    {
        //Delegate the call to the super;
        hr = THR( super::PrivateQueryInterface( riid, ppv));
        goto Cleanup;
    }

    //AddRef if the pointer is being returned.
    if ( *ppv /*&& (riid != IID_IEnumVARIANT)*/)
    {
        ((LPUNKNOWN) *ppv)->AddRef();
    }

Cleanup:
    RRETURN1( hr, E_NOINTERFACE);
}

//+---------------------------------------------------------------------------
//  GetTypeInfoCount
//  
//  DESCRIPTION:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBase::GetTypeInfoCount( UINT *pctInfo )
{
    // check the out parameter
    if ( !pctInfo )
    {
        RRETURN( E_POINTER );
    }
    
    //set the type information count
    *pctInfo = 1;

    RRETURN( S_OK );
}


//+---------------------------------------------------------------------------
//  GetTypeInfoCount
//  
//  DESCRIPTION:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBase::GetTypeInfo(  unsigned int iTInfo,
                        LCID ,
                        ITypeInfo FAR* FAR* ppTInfo)
{   
    HRESULT     hr = S_OK;

    if ( iTInfo )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    if ( !ppTInfo )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    hr = THR( EnsureAccTypeInfo( ppTInfo ) );
    
Cleanup:    
    RRETURN( hr ); 
}

//+---------------------------------------------------------------------------
//  GetIDsOfNames
//  
//  DESCRIPTION:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBase::GetIDsOfNames(    REFIID riid,
                            OLECHAR FAR* FAR* rgszNames,
                            unsigned int cNames,
                            LCID,
                            DISPID FAR* rgDispId)
{
    HRESULT     hr;
    ITypeInfo * pAccTypeInfo = NULL;
    
    hr = THR( EnsureAccTypeInfo( &pAccTypeInfo ) );
    if ( hr )
        RRETURN( hr );

    Assert( pAccTypeInfo );
    
    RRETURN( DispGetIDsOfNames( pAccTypeInfo, 
                                rgszNames, 
                                cNames, 
                                rgDispId));
}

//+---------------------------------------------------------------------------
//  Invoke
//  
//  DESCRIPTION:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBase::Invoke(   DISPID dispIdMember,
                    REFIID riid,
                    LCID,
                    WORD wFlags,
                    DISPPARAMS FAR* pDispParams,
                    VARIANT FAR* pVarResult,
                    EXCEPINFO FAR* pExcepInfo,
                    unsigned int FAR* puArgErr)
{
    HRESULT     hr;
    ITypeInfo * pAccTypeInfo = NULL;

    hr = THR( EnsureAccTypeInfo( &pAccTypeInfo ) );
    if ( hr )
        RRETURN( hr );

    Assert( pAccTypeInfo );

    RRETURN( DispInvoke(    DYNCAST( IAccessible, this),           
                            pAccTypeInfo,   
                            dispIdMember,   
                            wFlags,         
                            pDispParams,    
                            pVarResult,     
                            pExcepInfo,     
                            puArgErr) );
}


//----------------------------------------------------------------------------
//  DESCRIPTION :   
//          Not supported
//
//  PARAMETERS:
//      varChild     :   VARIANT containing the child ID
//      pbstrHelp    :   pointer to the BSTR to receive data.
//
//  RETURNS:
//      DISP_E_MEMBERNOTFOUND
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBase::get_accHelp(VARIANT varChild, BSTR* pbstrHelp)
{
    if ( pbstrHelp )
        *pbstrHelp = NULL;
        
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//  DESCRIPTION :   
//          Not supported
//
//  PARAMETERS:
//      pbstrHelpFile   :   Path for the help file.
//      varChild        :   VARIANT containing the child ID
//      pidTopic        :   address of the long to receive data
//
//  RETURNS:
//      DISP_E_MEMBERNOTFOUND
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBase::get_accHelpTopic(BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic)
{
    if ( pidTopic )
        *pidTopic = NULL;
        
    return E_NOTIMPL;
}


//----------------------------------------------------------------------------
//  DESCRIPTION :   
//          NYI
//
//  PARAMETERS:
//      varChild     :   VARIANT containing the child ID
//      pbstrValue   :   value bstr
//
//  RETURNS:
//      DISP_E_MEMBERNOTFOUND
//----------------------------------------------------------------------------
STDMETHODIMP 
CAccBase::put_accName(VARIANT varChild, BSTR bstrName)
{   
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//  IOleWindow implementation
//
//----------------------------------------------------------------------------
STDMETHODIMP    
CAccBase::GetWindow( HWND* phwnd )
{
    HRESULT hr = S_OK;
    
    if ( !phwnd )
    {
        hr = E_POINTER;
    }
    else
    {   
        Assert( GetAccDoc()->State() >= OS_INPLACE );
        
        //get the window handle from the document.
        *phwnd = GetAccDoc()->GetHWND();

        if ( !(*phwnd) )
            hr = E_FAIL;
    }

    RRETURN( hr );
}

STDMETHODIMP
CAccBase::ContextSensitiveHelp( BOOL fEnterMode )
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//      HELPER FUNCTIONS 
//
//----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//  ValidateChildID()
//
//  DESCRIPTION:
//      Validates the child id that is passed in in a variant variable. This is 
//      used only on in-parameters so that is it a safe operation to change it
//      and dereference in place.
//----------------------------------------------------------------------------
HRESULT 
CAccBase::ValidateChildID( VARIANT *pVar )
{
    HRESULT hr = S_OK;
    
    Assert(pVar);

    switch ( V_VT( pVar ) )
    {
        case VT_VARIANT | VT_BYREF:
            VariantCopy( pVar, pVar->pvarVal );
            hr = ValidateChildID( pVar );
            break;

        case VT_ERROR:
            if ( pVar->scode != DISP_E_PARAMNOTFOUND )
            {
                hr = E_INVALIDARG;
                break;
            }
          // Treat this as a VT_EMPTY

        case VT_EMPTY:
            V_VT(pVar ) = VT_I4;
            V_I4( pVar ) = CHILDID_SELF;
            break;

        case VT_I4:
            if ( V_I4( pVar ) < 0 )
                hr = E_INVALIDARG;
            break;

        default:
            hr = E_INVALIDARG;
    }

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//  GetAccParent
//  
//  DESCRIPTION:
//      Non-virtual helper that returns the accessible parent object for a 
//      CElement.
//      Although this function works with elements, it is still placed in the
//      base class, since the frames also have element parents, although they
//      are themselves accessible window objects.
//  
//  PARAMETERS:
//      pElem       :   CElement which we want the accessible parent of.
//      pAccParent  :   Pointer to the accessible parent object to be returned.
//  
//  RETURNS:
//      S_OK | E_POINTER | E_OUTOFMEMORY | E_FAIL
//----------------------------------------------------------------------------
HRESULT
CAccBase::GetAccParent( CElement * pElem,  CAccBase ** ppAccParent)
{
    HRESULT     hr = S_OK;
    CTreeNode * pNode = NULL;
    CTreeNode * pParentNode = NULL;
    
    if ( !ppAccParent )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppAccParent = NULL;

    //
    // get to the element's node
    //
    pNode = pElem->GetFirstBranch();

    // We have to have a tree node that is in the tree when we reach here
    if ( !pNode || pNode->IsDead())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // initialize for the loop
    pParentNode = pNode;

    //
    // loop until we find a parent node, that is connected to an 
    // HTML element that is a supported in the accessibility world.
    //
    // use do/while since the element passed with pElem may not be supported.
    do 
    {
        pParentNode = pParentNode->Parent();
    }
    while ( pParentNode && !IsSupportedElement( pParentNode->Element() ) );

    if ( !pParentNode )
    {
        hr = S_FALSE;   // according to the spec, this is what we should return
        goto Cleanup;
    }

    //
    // Since we have found the node for the supported element parent,
    // we can now create the accessible object for that element.
    //
    *ppAccParent = GetAccObjOfElement( pParentNode->Element() );

    if ( !(*ppAccParent)) 
        hr = E_OUTOFMEMORY;

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
HRESULT 
CAccBase::EnsureAccTypeInfo( ITypeInfo ** ppTI )
{
    HRESULT     hr = S_OK;
    CDoc *      pRootDoc = NULL;
    ITypeLib *  pTLib = NULL;
    
    // only called by internal methods
    Assert( ppTI );

    pRootDoc = GetAccDoc()->GetRootDoc();
    
    // if the root document already has a cached ITypeInfo for IAccessible
    if ( pRootDoc && pRootDoc->_pAccTypeInfo )
    {
        *ppTI = pRootDoc->_pAccTypeInfo;
    }
    else
    {
        OLECHAR     szFile[] = _T("OLEACC.DLL");
        
        // get the type information pointer from OLEACC
        hr = THR(LoadTypeLib( szFile, &pTLib));
        if ( hr )
            goto Cleanup;

        Assert( pTLib );

        // get the type information for the IID_IAccessible 
        hr = THR( pTLib->GetTypeInfoOfGuid( IID_IAccessible, ppTI));
        if ( hr )
            goto Cleanup;

        // it is implied with the 'if' above that we will have a valid *ppTI
        Assert( *ppTI );
        
        // cache the pointer if the call was successfull
        if ( pRootDoc )
            pRootDoc->_pAccTypeInfo = *ppTI;
    }
    
Cleanup:
    // we can release the pTLib, since we are holding on to the type
    // information from the same type library
    ReleaseInterface( pTLib );
    RRETURN( hr );
}


