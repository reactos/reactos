//--------------------------------------------------------------------
// Microsoft Nile to STD Mapping Layer
// (c) 1996 Microsoft Corporation.  All Rights Reserved.
//
//  Author:     Sam Bent (sambent)
//
//  Contents:   CDBProperties object implementation
//
//  History:
//  07/31/96    SamBent     Creation

#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

DeclareTag(tagDBProperties, "CDBProperties", "OLE_DB provider properties");

MtDefine(CDBProperties, DataBind, "CDBProperties")
MtDefine(CDBProperties_aPropSets, CDBProperties, "CDBProperties::_aPropSets")
MtDefine(CDBProperties_rgProperties, CDBProperties, "CDBProperties::_aPropSets::_rgProperties")

////////////////////////////////////////////////////////////////////////////////
//
//  CDBProperties specific interfaces
//
////////////////////////////////////////////////////////////////////////////////


//+---------------------------------------------------------------------------
//  Member:     constructor (public member)
//
//  Synopsis:   instantiate a CDBProperties
//
//  Arguments:  none

CDBProperties::CDBProperties():
	_cPropSets(0),
	_aPropSets(0)
{
    TraceTag((tagDBProperties,
             "CDBProperties::constructor -> %p", this ));
}


//+---------------------------------------------------------------------------
//  Member:     destructor (public member)
//
//  Synopsis:   release storage used by CDBProperties

CDBProperties::~CDBProperties()
{
    TraceTag((tagDBProperties,
             "CDBProperties::destructor -> %p", this ));

	ULONG iPropSet;
	for (iPropSet=0; iPropSet<_cPropSets; ++iPropSet)
	{
		delete [] _aPropSets[iPropSet].rgProperties;
	}
	delete [] _aPropSets;
}


//+---------------------------------------------------------------------------
//  Member:     GetPropertySet (public member)
//
//  Synopsis:   look up a property set by its GUID
//
//  Arguments:  guid            GUID of desired property set
//
//  Returns:    pointer to desired property set, or 0 if not found

DBPROPSET*
CDBProperties::GetPropertySet(const GUID& guid) const
{
    TraceTag((tagDBProperties,
             "CDBProperties::GetPropertySet({%p} %p)", this, guid ));

	DBPROPSET* pPropSet = 0;		// the answer, assume not found
	
	// linear search
	ULONG iPropSet;
	for (iPropSet=0; iPropSet<_cPropSets; ++iPropSet)
	{
		if (IsEqualGUID(guid, _aPropSets[iPropSet].guidPropertySet))
		{
			pPropSet = &_aPropSets[iPropSet];
			break;
		}
	}

	return pPropSet;
}


//+---------------------------------------------------------------------------
//  Member:     CopyPropertySet (public member)
//
//  Synopsis:   make a copy of a property set, given its GUID
//
//  Arguments:  guid            GUID of desired property set
//              pPropSetDst     ptr to DBPROPSET where copy should go
//
//  Returns:    S_OK            it worked
//              E_OUTOFMEMORY   no bytes
//              E_FAIL          no property set for given GUID

HRESULT
CDBProperties::CopyPropertySet(const GUID& guid, DBPROPSET* pPropSetDst) const
{
    TraceTag((tagDBProperties,
             "CDBProperties::CopyPropertySet({%p} %p, %p)", this, guid, pPropSetDst ));
    Assert(pPropSetDst && "must supply a PropSet pointer");

    HRESULT hr = S_OK;
	const DBPROPSET* pPropSetSrc = GetPropertySet(guid);
	ULONG iProp;

    if (pPropSetSrc == 0)       // not found
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // start with shallow copy
    *pPropSetDst = *pPropSetSrc;
    
    // allocate property array
    pPropSetDst->rgProperties = (DBPROP*)
                    CoTaskMemAlloc(pPropSetSrc->cProperties * sizeof(DBPROP));
    if (pPropSetDst->rgProperties == 0)
    {
        pPropSetDst->cProperties = 0;       // defensive
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // copy the property array
    for (iProp=0; iProp<pPropSetSrc->cProperties; ++iProp)
    {
        pPropSetDst->rgProperties[iProp] = pPropSetSrc->rgProperties[iProp];
    }
    
Cleanup:
	return hr;
}


//+---------------------------------------------------------------------------
//  Member:     GetProperty (public member)
//
//  Synopsis:   look up a property by its property set GUID and ID.
//
//  Arguments:  guid            guid of desired property
//              dwId            id of desired property
//
//  Returns:    pointer to DBPROP for the property, or 0 if not found

const DBPROP*
CDBProperties::GetProperty(const GUID& guid, DBPROPID dwId) const
{
    TraceTag((tagDBProperties,
             "CDBProperties::GetProperty({%p} %p, %lu)", this, guid, dwId ));

	ULONG iProp;
	const DBPROPSET* pPropSet = GetPropertySet(guid);
	const DBPROP*	pProp = 0;		// the answer, assume not found

	if (pPropSet == 0)			// no properties for desired property set
		goto Cleanup;
	
	// look up the desired property in the property set
	for (iProp=0; iProp<pPropSet->cProperties; ++iProp)
	{
		if (dwId == pPropSet->rgProperties[iProp].dwPropertyID)
		{
			pProp = & pPropSet->rgProperties[iProp];
			break;
		}
	}

Cleanup:
	return pProp;
}


//+---------------------------------------------------------------------------
//  Member:     SetProperty (public member)
//
//  Synopsis:   add a new property, or reset an existing one
//
//  Arguments:  guid			property set GUID of desired property
//								prop.dwPropertyID contains its ID.
//				prop			(reference to) DBPROP describing new property.
//
//  Returns:    S_OK            property added/reset
//              E_OUTOFMEMORY   no memory for new property set or new property

HRESULT
CDBProperties::SetProperty(const GUID& guid, const DBPROP& prop)
{
    TraceTag((tagDBProperties,
             "CDBProperties::SetProperty({%p}, %p, %p)", this, guid, prop ));

	HRESULT hr;
	DBPROP *pProp;			// pointer to array entry for new property
	ULONG iProp;
	DBPROPSET* pPropSet = GetPropertySet(guid);

	if (pPropSet == 0)				// no properties yet in desired property set
	{
		ULONG iPropSet;
		
		// get a new property set array
		DBPROPSET * aNewPropSets = new(Mt(CDBProperties_aPropSets)) DBPROPSET[_cPropSets + 1];
		if (aNewPropSets == 0)
		{
			hr = E_OUTOFMEMORY;
			goto Cleanup;
		}

		// copy the old array into the new
		for (iPropSet=0; iPropSet<_cPropSets; ++iPropSet)
		{
			aNewPropSets[iPropSet] = _aPropSets[iPropSet];
		}

		// add the new property set
		pPropSet = & aNewPropSets[_cPropSets];
		pPropSet->guidPropertySet = guid;
		pPropSet->cProperties = 0;
		pPropSet->rgProperties = 0;

		// release the old array, install the new one
		delete [] _aPropSets;
		_aPropSets = aNewPropSets;
		++ _cPropSets;
	}

	// look for the desired property.
	pProp = 0;
	for (iProp=0; iProp<pPropSet->cProperties; ++iProp)
	{
		if (pPropSet->rgProperties[iProp].dwPropertyID == prop.dwPropertyID)
		{
			pProp = &pPropSet->rgProperties[iProp];
			break;
		}
	}

	// if it's a new property, add it.  OLE-DB doesn't provide for any "unused"
	// portion in the array of DBPROPS, so we must reallocate the array every
	// time we add a property.  
	if (pProp == 0)
	{
		// allocate new property array
		DBPROP* aNewProperties = new(Mt(CDBProperties_rgProperties)) DBPROP[pPropSet->cProperties + 1];
		if (aNewProperties == 0)
		{
			hr = E_OUTOFMEMORY;
			goto Cleanup;
		}

		// copy old array into new
		for (iProp=0; iProp<pPropSet->cProperties; ++iProp)
		{
			aNewProperties[iProp] = pPropSet->rgProperties[iProp];
		}

		// prepare to use new property entry
		pProp = & aNewProperties[pPropSet->cProperties];

		// release old array, install new
		delete [] pPropSet->rgProperties;
		pPropSet->rgProperties = aNewProperties;
		++ pPropSet->cProperties;
	}

	// copy the property into my array
	*pProp = prop;

	hr = S_OK;

Cleanup:
	RRETURN(hr);
}

