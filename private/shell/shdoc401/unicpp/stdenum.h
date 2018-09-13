//=--------------------------------------------------------------------------=
// StandardEnum.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// object definition for a generic enumerator object.
//
#ifndef _STANDARDENUM_H_

// A specific implementation of the Clone function
void WINAPI CopyAndAddRefObject(void *pDest, const void *pSource, DWORD dwSize);

// An IEnumConnectionPoints creation function
HRESULT CreateInstance_IEnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum, DWORD count, ...);

// to support a generic Enumerator object, we'll just define this
// interface.  it can be safely cast to any other enumerator, since all
// they differ in is their pointer type in Next().
//
class IEnumGeneric: public IUnknown {

  public:
    virtual HRESULT __stdcall Next(ULONG celt, LPVOID rgelt, ULONG *pceltFetched) = 0;
    virtual HRESULT __stdcall Skip(ULONG celt) = 0;
    virtual HRESULT __stdcall Reset(void) = 0;
    virtual HRESULT __stdcall Clone(IEnumGeneric **ppenum) = 0;
};

//=--------------------------------------------------------------------------=
// StandardEnum
//=--------------------------------------------------------------------------=
// a generic enumerator object.  given a pointer to generic data, some
// information about the elements, and a function to copy the elements,
// we can implement a generic enumerator.
//
// NOTE: this class assumes that rgElements is HeapAlloc'd, and will free it
//       in it's destructor [although it IS valid for this to be NULL if there
//       are no elements to enumerate over.]
//
class CStandardEnum: public IEnumGeneric {

public:
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IEnumVariant methods
    //
    STDMETHOD(Next)(unsigned long celt, void * rgvar, unsigned long * pceltFetched); 
    STDMETHOD(Skip)(unsigned long celt); 
    STDMETHOD(Reset)(); 
    STDMETHOD(Clone)(IEnumGeneric **ppEnumOut); 

    CStandardEnum(REFIID riid, BOOL fMembersAreInterfaces, int cElement, int cbElement, void *rgElements,
                 void (WINAPI * pfnCopyElement)(void *, const void *, DWORD));
    ~CStandardEnum();

private:
    int m_cRef;

    IID m_iid;                        // type of enumerator that we are
    int m_cElements;                  // Total number of elements
    int m_cbElementSize;              // Size of each element
    int m_iCurrent;                   // Current position: 0 = front, m_cElt = end
    BOOL m_fMembersAreInterfaces;   // Indicates that members of the enumerated array are
                                      // Interfaces and that we need to hold refs on them
    VOID * m_rgElements;              // Array of elements  
    CStandardEnum *m_pEnumClonedFrom; // If we were cloned, from whom?
    void  (WINAPI * m_pfnCopyElement)(void *, const void *, DWORD);
};


extern "C" void* CStandardEnum_CreateInstance(REFIID riid, BOOL fMembersAreInterfaces, int cElement, int cbElement, void *rgElements,
                 void (WINAPI * pfnCopyElement)(void *, const void *, DWORD));


#define _STANDARDENUM_H_
#endif // _STANDARDENUM_H_

