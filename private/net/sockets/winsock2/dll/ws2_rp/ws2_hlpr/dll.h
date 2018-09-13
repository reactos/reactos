//--------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Rights Reserved
//
// dll.h
//
//--------------------------------------------------------------------

#ifndef _DLL_H_
#define _DLL_H_

// extern void ObjectDestroyed(void);


// This class factory object creates RestrictedProcess objects.

class CRestrictedProcessClassFactory : public IClassFactory
    {
    protected:
        ULONG           m_cRef;

    public:
        CRestrictedProcessClassFactory(void);
        ~CRestrictedProcessClassFactory(void);

        //IUnknown members
        STDMETHODIMP QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, PPVOID);
        STDMETHODIMP LockServer(BOOL);
    };

typedef CRestrictedProcessClassFactory *PCRestrictedProcessClassFactory;

#endif // _DLL_H_
