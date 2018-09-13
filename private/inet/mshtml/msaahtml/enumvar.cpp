//=======================================================================
//              File:   ENUMVAR.CPP
//              Date:   4-1-97
//
//              Desc:   This file describes the implementation of the
//                              CEnumVariant object, a VARIANT enumerator that
//                              uses a SAFEARRAY to store its elements.
//
//              Notes:  This file was copied from an existing Microsoft Source
//                              Code OLE Automation Collection sample.  It was originally
//                              written by Microsoft Product Support Services, Windows
//                              Developer Support, and was modified as needed.
//=======================================================================

//=======================================================================
//              Include Files
//=======================================================================

#include "stdafx.h"
#include <windows.h>
#include "enumvar.h"  


//=======================================================================
//              CEnumVariant Public Methods
//=======================================================================

//-----------------------------------------------------------------------
//      CEnumVariant::Create()
//
//      DESCRIPTION:
//
//              Creates an instance of the VARIANT enumerator object and
//                      initializes it.
//
//      PARAMETERS:
//
//              psa                             [in] SAFEARRAY containing items to be enumerated.
//              cElements               [in] Number of items to be enumerated. 
//              ppenumvariant   [out] Returns enumerator object.
//
//      RETURNS:
//
//              HRESULT                 NOERROR on success.
//                                              E_OUTOFMEMORY or the return value from either
//                                                      SafeArrayGetLBound() or SafeArrayCopy()
//                                                      on failure.
//
//      NOTES:
//
//              This is a static class method.
//
//-----------------------------------------------------------------------

HRESULT
CEnumVariant::Create( SAFEARRAY FAR* psa, ULONG cElements, CEnumVariant FAR* FAR* ppenumvariant )
{   
    HRESULT                             hr;
    CEnumVariant FAR*   penumvariant = NULL;
    long                                lLBound;
		      

	if ( !psa || !ppenumvariant )
		return E_INVALIDARG;

    
	*ppenumvariant = NULL;
    

	//---------------------------------------------------------
	//  Create the enumerator object.
	//---------------------------------------------------------

    penumvariant = new CEnumVariant();
    if ( penumvariant == NULL )
	goto error; 
	

	//---------------------------------------------------------
    //  Get the lower bound of the 1st array dimension
	//    of the SAFEARRAY in-parameter.  This will be
	//    used as the lower bound for the SAFEARRAY member.
	//---------------------------------------------------------
    
	hr = SafeArrayGetLBound( psa, 1, &lLBound );
    if ( FAILED( hr ) )
	goto error;


	//---------------------------------------------------------
    //  Initialize the SAFEARRAY member of the enumerator
	//    by copying the SAFEARRAY in-parameter.
	//---------------------------------------------------------

    hr = SafeArrayCopy( psa, &penumvariant->m_psa );
    if ( FAILED( hr ) )
       goto error;
    

	//---------------------------------------------------------
	//  Initialize the state of the enumerator.
	//---------------------------------------------------------

    penumvariant->m_cRef = 0;
    penumvariant->m_cElements = cElements;    
    penumvariant->m_lLBound = lLBound;
    penumvariant->m_lCurrent = lLBound;                  
    

	//---------------------------------------------------------
	//  Set the out-parameter and return success.
	//---------------------------------------------------------

    *ppenumvariant = penumvariant;

    return NOERROR;

    
	//---------------------------------------------------------
	//  If an error occurs, ...
	//---------------------------------------------------------

error: 
	//---------------------------------------------------------
	//  If the enumerator creation failed, return
	//    E_OUTOFMEMORY.
	//---------------------------------------------------------

    if ( penumvariant == NULL )
	return E_OUTOFMEMORY;


	//---------------------------------------------------------
	//  Otherwise, cleanup and return error code.
	//---------------------------------------------------------

    if ( penumvariant->m_psa )
	SafeArrayDestroy( penumvariant->m_psa );
    penumvariant->m_psa = NULL;     
    delete penumvariant;


    return hr;
}




//-----------------------------------------------------------------------
//      CEnumVariant::CEnumVariant()
//
//      DESCRIPTION:
//
//              Object constructor.  Initializes m_psa, the SAFEARRAY pointer,
//                      to NULL.
//
//      PARAMETERS:
//
//              None.
//
//      RETURNS:
//
//              None.
//
//-----------------------------------------------------------------------

CEnumVariant::CEnumVariant()
{    
    m_psa = NULL;
}




//-----------------------------------------------------------------------
//      CEnumVariant::~CEnumVariant()
//
//      DESCRIPTION:
//
//              Object destructor.  If necessary, the SAFEARRAY pointed to
//              by m_psa is destroyed (freeing up its memory).
//
//      PARAMETERS:
//
//              None.
//
//      RETURNS:
//
//              None.
//
//-----------------------------------------------------------------------

CEnumVariant::~CEnumVariant()
{                   
    if ( m_psa != NULL )
		SafeArrayDestroy( m_psa );
}



//-----------------------------------------------------------------------
//      CEnumVariant::QueryInterface()
//
//      DESCRIPTION:
//
//              Implements the IUnknown interface method QueryInterface().
//
//      PARAMETERS:
//
//              iid                             [in]  The requested interface's IID.
//              ppv                             [out] If the requested interface is supported,
//                                                    ppv points to the location of a pointer
//                                                    to the requested interface.
//                                                    If the requested interface is not
//                                                    supported, ppv is set to NULL.
//
//      RETURNS:
//
//              HRESULT                 NOERROR if the interface is supported,
//                                              E_NOINTERFACE otherwise.
//
//      NOTES:
//
//              CEnumVariant supports the IUnknown and IEnumVARIANT interfaces.
//
//-----------------------------------------------------------------------

STDMETHODIMP
CEnumVariant::QueryInterface( REFIID iid, void FAR* FAR* ppv )
{
	if ( !ppv )
		return E_INVALIDARG;

    *ppv = NULL;
	
    if ( iid == IID_IUnknown )
	*ppv = (LPUNKNOWN) this;     
	else if ( iid == IID_IEnumVARIANT ) 
	*ppv = (IEnumVARIANT *) this;     
    else
		return E_NOINTERFACE;

    AddRef();

    return NOERROR;    
}




//-----------------------------------------------------------------------
//      CEnumVariant::AddRef()
//
//      DESCRIPTION:
//
//              Implements the IUnknown interface method AddRef().
//
//              Increments the reference count.
//
//      PARAMETERS:
//
//              None.
//
//      RETURNS:
//
//              ULONG                   Current reference count.
//
//-----------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CEnumVariant::AddRef( void )
{
    return ++m_cRef;
}




//-----------------------------------------------------------------------
//      CEnumVariant::Release()
//
//      DESCRIPTION:
//
//              Implements the IUnknown interface method Release().
//
//              Decrements the reference count, deleting the object when
//                      the reference count reaches zero.
//
//      PARAMETERS:
//
//              None.
//
//      RETURNS:
//
//              ULONG                   Current reference count.
//
//-----------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CEnumVariant::Release( void )
{
    if( --m_cRef == 0 )
    {
	delete this;
	return 0L;
    }

    return m_cRef;
}




//-----------------------------------------------------------------------
//      CEnumVariant::Next()
//
//      DESCRIPTION:
//
//              Implements the IEnumVARIANT interface method Next().
//
//              Attempts to retrieve the next cVars VARIANTs in the
//                      enumeration sequence and return them through the
//                      array pointed to by rgVar.
//
//      PARAMETERS:
//
//              cVars                           [in] The number of VARIANTs to be returned.
//              rgVar                           [in,out] An array of at least size cVars
//                                                               in which the VARIANTs are to be
//                                                               returned.
//              pcVarsFetched           [out] Pointer to the number of VARIANTs
//                                                            returned in rgVar.
//
//      RETURNS:
//
//              HRESULT                 S_OK if exactly cVars VARIANTS were retrieved.
//                                              S_FALSE if less than cVars VARIANTS were
//                                                      retrieved.
//                                              E_INVALIDARG if the parameters are invalid.
//
//-----------------------------------------------------------------------

STDMETHODIMP
CEnumVariant::Next( ULONG cVars, VARIANT FAR* rgVar, ULONG FAR* pcVarsFetched )
{ 
    HRESULT             hr;
    ULONG               l, l2;
    long                l1;
    

	//---------------------------------------------------------
	//  Validate the rgVar and pcVarsFetched parameters.
	//---------------------------------------------------------

    if ( rgVar == NULL || pcVarsFetched == NULL )
		return E_INVALIDARG;


	//---------------------------------------------------------
	//  Initialize pcVarsFetched.
	//---------------------------------------------------------

	*pcVarsFetched = 0;
    

	//---------------------------------------------------------
	//  Initialize the VARIANTs in rgVar.
	//---------------------------------------------------------

    for ( l = 0; l < cVars; l++ )
    {
        hr = VariantClear( &rgVar[l] );

        if ( hr != S_OK )
            return hr;
    }
	

	//---------------------------------------------------------
    //  Retrieve the next cVars VARIANTs.
	//---------------------------------------------------------

    for ( l1 = m_lCurrent, l2 = 0;
	      l1 < (long)(m_lLBound + m_cElements) && l2 < cVars;
		  l1++, l2++ )
    {
       hr = SafeArrayGetElement( m_psa, &l1, &rgVar[l2] );
       if ( FAILED( hr ) )
	   goto error; 
    }
	

	//---------------------------------------------------------
    //  Set the count of the VARIANTs retrieved.
	//---------------------------------------------------------

	*pcVarsFetched = l2;
	

	//---------------------------------------------------------
    //  Update the current enumerator element.
	//---------------------------------------------------------

    m_lCurrent = l1;
    

	//---------------------------------------------------------
    //  Return either S_OK or S_FALSE depending on how many
	//  VARIANTs were actually retrieved.
	//---------------------------------------------------------

    return ( l2 < cVars ) ? S_FALSE : S_OK;


	//---------------------------------------------------------
    //  On error, ...
	//---------------------------------------------------------

error:
	//---------------------------------------------------------
    //  Clear all the VARIANTs in rgVar and return the
	//  error code.
	//---------------------------------------------------------

    for ( l = 0; l < cVars; l++ )
	VariantClear( &rgVar[l] );

    return hr;    
}




//-----------------------------------------------------------------------
//      CEnumVariant::Skip()
//
//      DESCRIPTION:
//
//              Implements the IEnumVARIANT interface method Skip().
//
//              Attempts to skip over the next cElements elements
//                      (VARIANTS) in the enumeration sequence.
//
//      PARAMETERS:
//
//              cVarsToSkip             [in] The number of VARIANTs to be skipped.
//
//      RETURNS:
//
//              HRESULT                 S_OK if the specified number of VARIANTS
//                                                      were skipped.
//                                              S_FALSE if the end of the enumeration
//                                                      sequence is encountered before the
//                                                      specified number VARIANTS were skipped.
//
//-----------------------------------------------------------------------

STDMETHODIMP
CEnumVariant::Skip( ULONG cVarsToSkip )
{
	//---------------------------------------------------------
    //  If the specified number of VARIANTs to skip is
	//    greater than the number left in the enumeration
	//    sequence, set the current VARIANT index to the
	//    last VARIANT and return S_FALSE.
	//---------------------------------------------------------

	if ( (long)(m_lCurrent + cVarsToSkip) > (long)(m_lLBound + m_cElements) )
	{
	m_lCurrent = (long)(m_lLBound + m_cElements);
	return S_FALSE;
	}

	
	//---------------------------------------------------------
    //  Otherwise, update the current VARIANT index
	//    return S_OK.
	//---------------------------------------------------------

	else
	{
		m_lCurrent += (long)cVarsToSkip;
	return S_OK;
	}
}




//-----------------------------------------------------------------------
//      CEnumVariant::Reset()
//
//      DESCRIPTION:
//
//              Implements the IEnumVARIANT interface method Reset().
//
//              Resets the current VARIANT index to the beginning.
//
//      PARAMETERS:
//
//              None.
//
//      RETURNS:
//
//              HRESULT                 S_OK if the current VARIANT index is reset to
//                                                      the beginning of the enumeration sequence.
//                                              S_FALSE on error.
//
//-----------------------------------------------------------------------

STDMETHODIMP
CEnumVariant::Reset( void )
{ 
    m_lCurrent = m_lLBound;

    return S_OK;
}



//-----------------------------------------------------------------------
//      CEnumVariant::Clone()
//
//      DESCRIPTION:
//
//              Implements the IEnumVARIANT interface method Clone().
//
//              Creates a new VARIANT enumerator with the same elements and
//                      enumeration state as the current object.
//
//      PARAMETERS:
//
//              ppenumvariant   [out] Point to a pointer to the IEnumVARIANT
//                                                    interface of the new enumerator object.
//
//      RETURNS:
//
//              HRESULT                 S_OK on success.
//                                              For failure return codes, see the listed
//                                                      return values for CEnumVariant::Create().
//
//-----------------------------------------------------------------------

STDMETHODIMP
CEnumVariant::Clone( IEnumVARIANT FAR* FAR* ppenum )
{
    CEnumVariant FAR* penum = NULL;
    HRESULT hr;
    

	if ( !ppenum )
		return E_INVALIDARG;


    *ppenum = NULL;
    
    
	//---------------------------------------------------------
    //  Create a new CEnumVariant with this CEnumVariant's
	//        SAFEARRAY and count of elements.
	//---------------------------------------------------------

	hr = CEnumVariant::Create(m_psa, m_cElements, &penum);
    if ( FAILED( hr ) )
	return hr;

    
	//---------------------------------------------------------
    //  Increase the reference count on the new CEnumVariant.
	//---------------------------------------------------------

	penum->AddRef();

    
	//---------------------------------------------------------
    //  Initialize the current VARIANT index of the new
	//        CEnumVariant from the index of this CEnumVariant.
	//---------------------------------------------------------

    penum->m_lCurrent = m_lCurrent; 
    

    
	//---------------------------------------------------------
    //  Point the out-parameter at the new enumerator and
	//        return success.
	//---------------------------------------------------------

    *ppenum = penum;        


    return S_OK;
}



//----  End of ENUMVAR.CPP  ----
