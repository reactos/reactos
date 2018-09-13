// DENames.cpp : Implementation of CDEGetBlockFmtNamesParam
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
#include "stdafx.h"
#include "DHTMLEd.h"
#include "DENames.h"

/////////////////////////////////////////////////////////////////////////////
// CDEGetBlockFmtNamesParam

CDEGetBlockFmtNamesParam::CDEGetBlockFmtNamesParam()
{
	m_pNamesArray = NULL;

	m_pNamesArray = SafeArrayCreateVector(VT_VARIANT, 0, 0);	
	_ASSERTE(m_pNamesArray);

}


CDEGetBlockFmtNamesParam::~CDEGetBlockFmtNamesParam()
{
	if (m_pNamesArray)
		SafeArrayDestroy(m_pNamesArray);
}


//	This will always retreive a SafeArray of Variants containing BSTRs.
//
STDMETHODIMP CDEGetBlockFmtNamesParam::get_Names(VARIANT * pVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(pVal);

	if (NULL == pVal)
		return E_INVALIDARG;

	VariantClear(pVal);

	V_VT(pVal) = VT_ARRAY|VT_VARIANT;
	hr = SafeArrayCopy(m_pNamesArray, &(V_ARRAY(pVal)));
	
	return hr;
}

//	The SafeArray gotten received from Trident is (currently) an array for BSTRs.
//	This works fine with VB, but not VBS or JScript.
//	We'll do the work at this end to copy the supplied array over to our private
//	storage as a SafeArray of Variants containing BSTRs.
//
STDMETHODIMP CDEGetBlockFmtNamesParam::put_Names(VARIANT* newVal)
{
	HRESULT hr = S_OK;
    LONG lLBound, lUBound, lIndex;

	_ASSERTE ( m_pNamesArray );
	_ASSERTE(VT_ARRAY == V_VT(newVal));
	_ASSERTE(NULL != V_ARRAY(newVal));

	if (VT_ARRAY != V_VT(newVal))
		return E_INVALIDARG;

	if (NULL == V_ARRAY(newVal))
		return E_INVALIDARG;

	SafeArrayGetLBound(V_ARRAY(newVal), 1, &lLBound);
	SafeArrayGetUBound(V_ARRAY(newVal), 1, &lUBound);

	SAFEARRAYBOUND rgsaBound[1];
	rgsaBound[0].lLbound	= 0;
	rgsaBound[0].cElements	= ( lUBound - lLBound ) + 1;
	hr = SafeArrayRedim ( m_pNamesArray, rgsaBound );
	_ASSERTE ( SUCCEEDED ( hr ) );
	if ( FAILED ( hr ) )
	{
		SafeArrayDestroy ( V_ARRAY(newVal) );
		return hr;
	}

	// Copy all BSTRs or Variants from the source array to Variants in the m_pNamesArray
	VARIANT	var;
	BSTR	bstr = NULL;

	VariantInit ( &var );

	for (lIndex=lLBound; lIndex<=lUBound; lIndex++)
	{
		if ( FADF_BSTR & V_ARRAY(newVal)->fFeatures )
		{
			hr = SafeArrayGetElement(V_ARRAY(newVal), &lIndex, &bstr);
			_ASSERTE ( SUCCEEDED ( hr ) );
			if ( FAILED ( hr ) )
				break;

			// BSTR was copied, we can stick it in a variant, no release or duplicating needed.
			var.vt = VT_BSTR;
			var.bstrVal = bstr;
		}
		else if ( FADF_VARIANT & V_ARRAY(newVal)->fFeatures )
		{
			hr = SafeArrayGetElement(V_ARRAY(newVal), &lIndex, &var);
			_ASSERTE ( SUCCEEDED ( hr ) );
			if ( FAILED ( hr ) )
				break;

			hr = VariantChangeType ( &var, &var, 0, VT_BSTR );
			_ASSERTE ( SUCCEEDED ( hr ) );
			if ( FAILED ( hr ) )
				break;
		}
		else
		{
			_ASSERTE ( ( FADF_BSTR | FADF_VARIANT ) & V_ARRAY(newVal)->fFeatures );
			hr = E_UNEXPECTED;
			break;
		}

		hr = SafeArrayPutElement ( m_pNamesArray, &lIndex, &var );
		_ASSERTE ( SUCCEEDED ( hr ) );
		VariantClear ( &var );
	}

	VariantClear ( &var );	// In case a break occurred.
	SafeArrayDestroy ( V_ARRAY(newVal) );

	return hr;
}
