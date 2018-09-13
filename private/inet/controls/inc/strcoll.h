//=--------------------------------------------------------------------------=
// StrColl.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains the definitions for the various string collections we'll use
//
#ifndef _STRCOLL_H_

#include "CommDlgInterfaces.H"



//=--------------------------------------------------------------------------=
// the CStringsCollection class basically works with a safearray to expose the
// collection, and uses the safearray functions to maniplate it.
//=--------------------------------------------------------------------------=
// NOTES: 9.95 - this collection assumes that the safearray lbound is
//        zero!
//=--------------------------------------------------------------------------=
//
class CStringCollection {

  public:
    // a couple of methods that are common
    //
    STDMETHOD(get_Count)(THIS_ long FAR* pcStrings);
    STDMETHOD(get_Item)(THIS_ long lIndex, BSTR FAR* pbstrItem);
    STDMETHOD(get__NewEnum)(THIS_ IUnknown * FAR* ppUnkNewEnum);

    CStringCollection(SAFEARRAY *);
    virtual ~CStringCollection();

  protected:
    // what the collection will work with.
    //
    SAFEARRAY *m_psa;
};

class CStringDynaCollection : public CStringCollection {

  public:
    // in addition to the CStringCollection methods, we'll have
    //
    STDMETHOD(put_Item)(THIS_ long lIndex, BSTR bstrItem);
    STDMETHOD(Add)(THIS_ BSTR bstrNew);
    STDMETHOD(Remove)(THIS_ long lIndex);

    CStringDynaCollection(SAFEARRAY *);
    virtual ~CStringDynaCollection();

};


#define _STRCOLL_H_
#endif // _STRCOLL_H_
