/*
 * @(#)provideclassinfo.hxx 1.0 9/2/98
 * 
 * Declaration of IProvideClassInfo tearoff
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _PROVIDECLASSINFO_HXX_
#define _PROVIDECLASSINFO_HXX_


//////////////////////////////////////////////////////////////////////////
// ProvideClassInfo
//////////////////////////////////////////////////////////////////////////
class ProvideClassInfo : public IProvideClassInfo
{
private:
    IUnknown *_punk;
    ULONG _cRef;
	REFIID _libid;
	REFIID _co_class;  // WIN64 Note: I had to rename this due to a naming conflict under WIN64

public:
    ProvideClassInfo(IUnknown *punk, REFIID libid, REFIID coclass);

// IUnknown members
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

// IProvideClassInfo members

// winuser.h has #define GetClassInfo GetClassInfoW.  w95wraps.h then has 
// #define GetClassInfoW GetClassInfoWrapW.  The following #define is required 
// to make all of this work.
#undef GetClassInfoW

public:
    HRESULT STDMETHODCALLTYPE GetClassInfo(ITypeInfo **ppTI);

};

#endif // _PROVIDECLASSINFO_HXX_