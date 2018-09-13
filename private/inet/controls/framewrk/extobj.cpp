////
//
// CExpandoObject
//
// Notes:
// 1) If the LCID passed to this object changes from call to call we are screwed. This is hard to
// create an ASSERT for because it would require memoizing the LCID at some point.
// 2) There is a maximum on the number of slots allowed (this is currently 2048)
// 3) This is not a thread safe structure.
// 4) I'm currently using malloc -- this is probably wrong for IE.
//

// for ASSERT and FAIL
//

#include "IPServer.H"
#include "LocalSrv.H"
#include "Globals.H"
#include "extobj.h"
#include "Util.H"
#define GTR_MALLOC(size)  CoTaskMemAlloc(size)
#define GTR_FREE(pv) CoTaskMemFree(pv)

SZTHISFILE
////
//
// Private Utility Functions
//
////

////
//
// Get the ID of a Name
//

HRESULT CExpandoObject::GetIDOfName(LPOLESTR name, LCID lcid, BOOL caseSensitive, DISPID* id)
{
	HRESULT hr = NOERROR;
	ULONG hash = LHashValOfName(lcid, name);
	UINT hashIndex = hash % kSlotHashTableSize;
	CExpandoObjectSlot* slot;

	for (slot=GetHashTableHead(hashIndex); slot!=NULL; slot=slot->Next(m_slots))
	{
		if (slot->CompareName(name, hash, caseSensitive))
		{
			*id = slot->DispId();
			goto Exit;
		}
	}

	// not found
	hr = DISP_E_UNKNOWNNAME;
	*id = DISPID_UNKNOWN;

Exit:
	return hr;
}

////
//
// Add a new slot to the object
//

HRESULT CExpandoObject::AddSlot(LPOLESTR name, LCID lcid, BOOL caseSensitive, VARIANT* initialValue, DISPID* id)
{
	HRESULT hr = NOERROR;
	ULONG hash = LHashValOfName(lcid, name);
	UINT hashIndex = hash % kSlotHashTableSize;
	CExpandoObjectSlot* slot;
	DISPID	dispId;

	// first check if the slot exists
	for (slot=GetHashTableHead(hashIndex); slot!=NULL; slot=slot->Next(m_slots))
	{
		// bail if the name matches
		if (slot->CompareName(name, hash, caseSensitive))
		{
			hr = E_INVALIDARG;
			goto Exit;
		}
	}

	// allocate a slot
	dispId = (DISPID) m_totalSlots;
	slot = AllocSlot();
	if (slot == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// Initialize it
	// BUGBUG robwell 8May96 If this fails and the initialValue is not VT_EMTPY or VT_NULL
	// there in no cleanup code.
	hr = slot->Init(name, lcid, dispId + m_dispIdBase, initialValue);
	if (FAILED(hr))
	{
		// free the slot and dispId
		m_totalSlots -= 1;
		goto Exit;
	}

	// intern the slot into the proper hash table
	slot->Insert(m_slots, m_hashTable[hashIndex]);

	// set the DISPID return value
	*id = slot->DispId();

Exit:
	return hr;
}

////
//
// Slot allocation
//
// Because slots are never freed there is no free method
//

CExpandoObjectSlot* CExpandoObject::AllocSlot()
{
	// limit on the number of slots
	if (m_totalSlots >= kMaxTotalSlots)
		return NULL;

	// do we need to realloc the array?
	if (m_totalSlots == m_slotTableSize)
	{
		UINT i;
		UINT newSize;
		CExpandoObjectSlot* newSlots;

		// allocate twice as many slots unless first time around
		if (m_slotTableSize == 0)
			newSize = kInitialSlotTableSize;
		else
			newSize = m_slotTableSize * 2;

		// allocate the space for the slots
		newSlots = (CExpandoObjectSlot*) GTR_MALLOC(sizeof(CExpandoObjectSlot)*newSize);
		if (newSlots == NULL)
			return NULL;

		// copy the old values if the old m_slots is not NULL
		if (m_slots)
		{
			// copy the slots
			memcpy(newSlots, m_slots, sizeof(CExpandoObjectSlot)*m_totalSlots);
			// free the old values
			GTR_FREE(m_slots);
		}

		// construct all of the unused slots
		for (i=m_totalSlots; i<newSize; ++i)
			newSlots[i].Construct();

		// make the new array the new table and fix the total size
		m_slots = newSlots;
		m_slotTableSize = newSize;
	}

	// return a pointer to the slot and bump the totalSlots count
	return &m_slots[m_totalSlots++];
}

////
//
// Free all of the slots
//

void CExpandoObject::FreeAllSlots()
{
	UINT i;
	UINT initedSlotCount;
	CExpandoObjectSlot* slots;

	// first clear the hash table
	ClearHashTable();

	// detach the slots
	slots = m_slots;
	initedSlotCount = m_totalSlots;

	// clear the object info
	m_totalSlots = 0;
	m_slotTableSize = 0;
	m_slots = NULL;

	// only need to destruct those slots in use
	for (i=0; i<initedSlotCount; ++i)
		slots[i].Destruct();

	// free the storage
	if (slots)
		GTR_FREE(slots);
}



////
//
// IDispatch Methods
//
////

HRESULT CExpandoObject::GetTypeInfoCount(UINT *pctinfo)
{
	*pctinfo = 0;
	return NOERROR;
}

HRESULT CExpandoObject::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
	*pptinfo = NULL;
	return E_NOTIMPL;
}

HRESULT CExpandoObject::GetIDsOfNames(
	REFIID riid,
	LPOLESTR *prgpsz,
	UINT cpsz,
	LCID lcid,
	DISPID *prgdispid
)
{
	HRESULT hr;

	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	// First see if the outer object knows about the name
	if (m_pdisp)
	{
		hr = m_pdisp->GetIDsOfNames(
			riid,
			prgpsz,
			cpsz,
			lcid,
			prgdispid);

		// if so, just return
		if (SUCCEEDED(hr))
			return hr;
	}

	// Otherwise look on our expanded properties

	if (cpsz == 0)
		return NOERROR;

	// get the ids for the name
	hr = GetIDOfName(prgpsz[0], lcid, FALSE, &prgdispid[0]);

	// clear the rest of the array
	for (unsigned int i = 1; i < cpsz; i++)
	{
		if (SUCCEEDED(hr))
			hr = DISP_E_UNKNOWNNAME;
		prgdispid[i] = DISPID_UNKNOWN;
	}

	return hr;
}

HRESULT CExpandoObject::Invoke(
	DISPID dispID,
	REFIID riid,
	LCID lcid,
	WORD wFlags,
	DISPPARAMS *pdispparams,
	VARIANT *pvarRes,
	EXCEPINFO *pexcepinfo,
	UINT *puArgErr
)
{
	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	HRESULT hr;

	// First try the outer object's invoke
	if (m_pdisp)
	{
		hr = m_pdisp->Invoke(
				dispID,
				riid,
				lcid,
				wFlags,
				pdispparams,
				pvarRes,
				pexcepinfo,
				puArgErr
		);

		// If that succeeded, we're done
		if (SUCCEEDED(hr))
			return hr;
	}
	
	// Otherwise, try the expando object's invoke	
	if (NULL != puArgErr)
		*puArgErr = 0;

	if (wFlags & DISPATCH_PROPERTYGET)
	{
		if (NULL == pvarRes)
			return NOERROR;

		if (NULL != pdispparams && 0 != pdispparams->cArgs)
			return E_INVALIDARG;

		// clear the result slot
		pvarRes->vt = VT_EMPTY;
		return GetSlot(dispID, pvarRes);
	}

	if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
	{
		if (NULL == pdispparams
		|| 1 != pdispparams->cArgs
		|| 1 != pdispparams->cNamedArgs
		|| DISPID_PROPERTYPUT != pdispparams->rgdispidNamedArgs[0]
		)
			return DISP_E_PARAMNOTOPTIONAL;

		return SetSlot(dispID, &pdispparams->rgvarg[0]);
	}

	return DISP_E_MEMBERNOTFOUND;
}

////
//
// IDispatchEx methods
//
////

// Get dispID for names, with options
HRESULT STDMETHODCALLTYPE CExpandoObject::GetIDsOfNamesEx(
	REFIID riid,
	LPOLESTR *prgpsz,
	UINT cpsz,
	LCID lcid,
	DISPID *prgid,
	DWORD grfdex
)
{
	HRESULT hr;
	BOOL caseSensitive = ((grfdex & fdexCaseSensitive) != 0);


	// First see if the outer object knows about the name
	if (m_pdisp)
	{
		hr = m_pdisp->GetIDsOfNames(
			riid,
			prgpsz,
			cpsz,
			lcid,
			prgid);

		// if so, just return
		if (SUCCEEDED(hr))
			return hr;
	}


	if (IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	if (cpsz == 0)
		return NOERROR;

	// check the array arguments
	if (prgpsz == NULL || prgid == NULL)
		return E_INVALIDARG;

	// get the id from the name
	hr = GetIDOfName(prgpsz[0], lcid, caseSensitive, &prgid[0]);

	// create the slot?
	if (hr == DISP_E_UNKNOWNNAME && (grfdex & fdexDontCreate) == 0)
	{
		VARIANT initialValue;

		if (grfdex & fdexInitNull)
			initialValue.vt = VT_NULL;
		else
			initialValue.vt = VT_EMPTY;

		hr = AddSlot(prgpsz[0], lcid, caseSensitive, &initialValue, &prgid[0]);
	}

	// clear the rest of the array
	for (unsigned int i = 1; i < cpsz; i++)
	{
		hr = DISP_E_UNKNOWNNAME;
		prgid[i] = DISPID_UNKNOWN;
	}

	return hr;
}

// Enumerate dispIDs and their associated "names".
// Returns S_FALSE if the enumeration is done, NOERROR if it's not, an
// error code if the call fails.
HRESULT STDMETHODCALLTYPE CExpandoObject::GetNextDispID(
	DISPID id,
	DISPID *pid,
	BSTR *pbstrName
)
{
	HRESULT hr;
	CExpandoObjectSlot* slot;

	// check the outgoing parameters
	if (pid == NULL || pbstrName == NULL)
		return E_INVALIDARG;

	// set to the default failure case
	*pid = DISPID_UNKNOWN;
	*pbstrName = NULL;

	// get the next slot
	hr = Next(id, slot);
	if (hr == NOERROR)
	{
		BSTR name;

		// allocate the result string
		name = SysAllocString(slot->Name());
		if (name == NULL)
			return E_OUTOFMEMORY;

		// fill in the outgoing parameters
		*pid = slot->DispId();
		*pbstrName = name;
	}

	return hr;
}

// Copy all of the expando-object properties from obj
CExpandoObject::CloneProperties(CExpandoObject& obj)
{
    // BUGBUG  PhilBo
    // The initialization code below is copied from the default constructor.
    // This should be factored out into a shared method.

	// Copy each of the properties from the original object
    HRESULT hr = S_OK;
    DISPID dispid = 0;
    BSTR bstrName = NULL;

    while (obj.GetNextDispID(dispid, &dispid, &bstrName) == S_OK)
    {
        // Get the value of the property from the original object
        VARIANT varResult;
        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
        VariantInit(&varResult);

        hr = obj.Invoke(
		        dispid,
		        IID_NULL,
		        LOCALE_SYSTEM_DEFAULT,
		        DISPATCH_PROPERTYGET,
		        &dispparamsNoArgs, &varResult, NULL, NULL);

        ASSERT(SUCCEEDED(hr), "");
        if (FAILED(hr))
            continue;

        // Set the property on the new object
        DISPID dispidNew = 0;
	    hr = GetIDsOfNamesEx(IID_NULL, &bstrName, 1, LOCALE_SYSTEM_DEFAULT,
		    &dispidNew, 0);

        ASSERT(SUCCEEDED(hr), "");
        if (FAILED(hr))
            continue;

        DISPPARAMS dispparams = {0};
        dispparams.rgvarg[0] = varResult;

        DISPID rgdispid[] = {DISPID_PROPERTYPUT};
        dispparams.rgdispidNamedArgs = rgdispid;
        dispparams.cArgs = 1;
        dispparams.cNamedArgs = 1;

        hr = Invoke(
		    dispidNew,
		    IID_NULL,
		    LOCALE_SYSTEM_DEFAULT,
		    DISPATCH_PROPERTYPUT,
		    &dispparams, NULL, NULL, NULL);
    }

    return hr;
}
