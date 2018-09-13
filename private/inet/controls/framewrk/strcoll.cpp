//=--------------------------------------------------------------------------=
// StrColl.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation for our simple strings collections.
//
#include "IPServer.H"

#include "SimpleEnumVar.H"
#include "StringsColl.H"


// for asserts
//
SZTHISFILE


//=--------------------------------------------------------------------------=
// CStringsCollection::CStringsCollection
//=--------------------------------------------------------------------------=
// constructor. sets up the safearray pointer.
//
// Parameters:
//    SAFEARRAY        - [in] the collection we're working with.
//
// Notes:
//
CStringCollection::CStringCollection
(
    SAFEARRAY *psa
)
: m_psa(psa)
{
    ASSERT(m_psa, "Bogus Safearray pointer!");
}

//=--------------------------------------------------------------------------=
// CStringCollection::~CStringCollection
//=--------------------------------------------------------------------------=
//
// Notes:
//
CStringCollection::~CStringCollection()
{
}

//=--------------------------------------------------------------------------=
// CStringCollection::get_Count
//=--------------------------------------------------------------------------=
// returns the count of the things in the collection
//
// Parameters:
//    long *         - [out] the count
//
// Output:
//    HRESULT        - S_OK, one of the SAFEARRAY Scodes.
//
// Notes:
//    - we're assuming the safearray's lower bound is zero!
//
STDMETHODIMP CStringCollection::get_Count
(
    long *plCount
)
{
    HRESULT hr;

    ASSERT(m_psa, "Who created a collection without a SAFEARRAY?");

    CHECK_POINTER(plCount);

    // get the bounds.
    //
    hr = SafeArrayGetUBound(m_psa, 1, plCount);
    CLEARERRORINFORET_ON_FAILURE(hr);

    // add one since we're zero-offset
    //
    (*plCount)++;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CStringCollection::get_Item
//=--------------------------------------------------------------------------=
// returns a string given an INDEX
//
// Parameters:
//    long          - [in]  the index to get it from
//    BSTR *        - [out] the item
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
STDMETHODIMP CStringCollection::get_Item
(
    long  lIndex,
    BSTR *pbstrItem
)
{
    HRESULT hr;

    CHECK_POINTER(pbstrItem);

    // get the element from the safearray
    //
    hr = SafeArrayGetElement(m_psa, &lIndex, pbstrItem);
    CLEARERRORINFORET_ON_FAILURE(hr);

    // otherwise, we've got it, so we can return
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CStringCollection::get__NewEnum
//=--------------------------------------------------------------------------=
// returns a new IEnumVARIANT object with the collection in it.
//
// Parameters:
//    IUnknown     **    - [out] new enumvariant object.
//
// Output:
//    HRESULT            - S_OK, E_OUTOFMEMORY
//
// Notes:
//
STDMETHODIMP CStringCollection::get__NewEnum
(
    IUnknown **ppUnkNewEnum
)
{
    HRESULT hr;
    long    l;

    CHECK_POINTER(ppUnkNewEnum);

    // get the count of things in the SAFEARRAY
    //
    hr = get_Count(&l);
    CLEARERRORINFORET_ON_FAILURE(hr);

    // create the object.
    //
    *ppUnkNewEnum = (IUnknown *) new CSimpleEnumVariant(m_psa, l);
    if (!*ppUnkNewEnum)
        CLEARERRORINFORET(E_OUTOFMEMORY);

    // refcount is already 1, so we can leave.
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// CStringDynaCollection::CStringDynaCollection
//=--------------------------------------------------------------------------=
// constructor for this object.  doesn't do very much.
//
// Parameters:
//    same as for CStringCollection
//
// Notes:
//
CStringDynaCollection::CStringDynaCollection
(
    SAFEARRAY *psa
)
: CStringCollection(psa)
{
}

//=--------------------------------------------------------------------------=
// CStringDynaCollection::~CStringDynaCollection
//=--------------------------------------------------------------------------=
// destructor.
//
// Notes:
//
CStringDynaCollection::~CStringDynaCollection()
{
}

//=--------------------------------------------------------------------------=
// CStringDynaCollection::put_Item
//=--------------------------------------------------------------------------=
// sets the value of an item in the array.
//
// Parameters:
//    long         - [in] index at which to put it
//    BSTR         - [in] new value.
//
// Output:
//    HRESULT      - S_OK, safearray Scode.
//
// Notes:
//    - NULLs are converted to ""
//
STDMETHODIMP CStringDynaCollection::put_Item
(
    long lIndex,
    BSTR bstr
)
{
    HRESULT hr;
    long l;
    BSTR bstr2 = NULL;

    // get the count and verify our index
    //
    hr = get_Count(&l);
    RETURN_ON_FAILURE(hr);
    if (lIndex < 0 || lIndex >= l)
        CLEARERRORINFORET(E_INVALIDARG);
    
    // put out the string, convert NULLs to ""
    //
    if (!bstr) {
        bstr2 = SysAllocString(L"");
        RETURN_ON_NULLALLOC(bstr2);
    }

    hr = SafeArrayPutElement(m_psa, &lIndex, (bstr) ? bstr : bstr2);
    if (bstr2) SysFreeString(bstr2);
    CLEARERRORINFORET_ON_FAILURE(hr);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CStringDynaCollection::Add
//=--------------------------------------------------------------------------=
// adds a new string to the end of the collection.
//
// Parameters:
//    BSTR         - [in] the new string to add
//
// Notes:
//
STDMETHODIMP CStringDynaCollection::Add
(
    BSTR bstr
)
{
    SAFEARRAYBOUND sab;
    BSTR    bstr2 = NULL;
    HRESULT hr;
    long    l;

    // get the current size of the array.
    //
    hr = get_Count(&l);
    RETURN_ON_FAILURE(hr);

    // add one new elemnt
    //
    sab.cElements = l + 1;
    sab.lLbound = 0;

    // redim the array.
    //
    hr = SafeArrayRedim(m_psa, &sab);
    CLEARERRORINFORET_ON_FAILURE(hr);

    // put the out string, converting NULLs to ""
    //
    if (!bstr) {
        bstr2 = SysAllocString(L"");
        RETURN_ON_NULLALLOC(bstr2);
    }

    hr = SafeArrayPutElement(m_psa, &l, (bstr) ? bstr : bstr2);
    if (bstr2) SysFreeString(bstr2);
    CLEARERRORINFORET_ON_FAILURE(hr);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CStringDynaCollection::Remove
//=--------------------------------------------------------------------------=
// removes an element from the collection, and shuffles all the rest down to
// fill up the space.
//
// Parameters:
//    long         - [in] index of dude to remove.
//
// Output:
//    HRESULT      - S_OK, safearray Scodes.
//
// Notes:
//
STDMETHODIMP CStringDynaCollection::Remove
(
    long lIndex
)
{
    SAFEARRAYBOUND sab;
    HRESULT hr;
    BSTR    bstr;
    long    lCount;
    long    x, y;

    // first get the count of things in our array.
    //
    hr = get_Count(&lCount);
    RETURN_ON_FAILURE(hr);

    // check the index
    //
    if (lIndex < 0 || lIndex >= lCount)
        CLEARERRORINFORET(E_INVALIDARG);

    // let's go through, shuffling everything down one.
    //
    for (x = lIndex, y = x + 1; x < lCount - 1; x++, y++) {
        // get the next element.
        //
        hr = SafeArrayGetElement(m_psa, &y, &bstr);
        CLEARERRORINFORET_ON_FAILURE(hr);

        // set it at the current location
        //
        hr = SafeArrayPutElement(m_psa, &x, bstr);
        CLEARERRORINFORET_ON_FAILURE(hr);
    }

    // we're at the last element.  let's go and kill it.
    //
    sab.cElements = lCount - 1;
    sab.lLbound = 0;

    // CONSIDER: 9.95 -- there is a bug in oleaut32.dll which causes the
    //         below to fail if cElements = 0.
    //
    hr = SafeArrayRedim(m_psa, &sab);
    CLEARERRORINFORET_ON_FAILURE(hr);

    // we're done.  go bye-bye.
    //
    return S_OK;
}


