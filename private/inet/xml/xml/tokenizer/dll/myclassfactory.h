/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
typedef HRESULT (STDMETHODCALLTYPE *PFN_CREATEINSTANCE)(REFIID, void **);

class MyClassFactory : public IClassFactory
{
public:
    MyClassFactory(PFN_CREATEINSTANCE pfnCreateInstance)
    {
        _ulRefs = 1;
        _pfnCreateInstance = pfnCreateInstance;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID riid,void ** ppv);

    virtual ULONG __stdcall AddRef() 
        { return ++_ulRefs; }
    virtual ULONG __stdcall Release()
        { 
            ULONG ul = --_ulRefs; 
            if (!ul) 
                delete this; 
            return ul; 
        }

    // IClassFactory methods
    virtual HRESULT __stdcall LockServer(BOOL fLock);

    virtual HRESULT __stdcall CreateInstance(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj);

    ULONG _ulRefs;
    PFN_CREATEINSTANCE _pfnCreateInstance;
};

