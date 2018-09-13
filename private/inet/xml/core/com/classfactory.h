/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
typedef HRESULT (__stdcall *PFN_CREATEINSTANCE)(REFIID, void **);

class CClassFactory : public IClassFactory
{
public:
    CClassFactory(PFN_CREATEINSTANCE pfnCreateInstance)
    {
        _ulRefs = 1;
        _pfnCreateInstance = pfnCreateInstance;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID riid,void ** ppv);

    virtual ULONG __stdcall AddRef()
    {
		return PtrToUlong((VOID*)InterlockedIncrement((PLONG)&_ulRefs) );
    }
        
    virtual ULONG __stdcall Release()
    {
        ULONG ulRval;
        if ( (ulRval = PtrToUlong((VOID*)InterlockedDecrement((PLONG)&_ulRefs) ) == 0) )
        {
            delete this;
        }
        return ulRval;
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

