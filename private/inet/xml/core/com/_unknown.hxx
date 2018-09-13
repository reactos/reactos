/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _UNKNOWN_HXX
#define _UNKNOWN_HXX

#if DBG==1
extern void * g_pUnkTrace;  // trace refs on this object
#endif

const ULONG_PTR REF_REFERENCE    = 1;
const ULONG_PTR REF_MARKED       = 2;
#ifdef RENTAL_MODEL
const ULONG_PTR REF_RENTAL       = 4;
#endif
const ULONG_PTR REF_LOCKED      = -1;

#ifdef RENTAL_MODEL
const LONG_PTR REF_OFFSET       = 8;
const LONG_PTR REF_SHIFT        = 3;
#else
const LONG_PTR REF_OFFSET       = 4;
const LONG_PTR REF_SHIFT        = 2;
#endif

extern long IncrementComponents();
extern long DecrementComponents();

class NOVTABLE __unknown
{
protected:  const IID * _riid;
private:  ULONG _lRefs;

public: __unknown(const IID * riid);

#ifdef RENTAL_MODEL
public: __unknown(const IID * riid, RentalEnum re);
#endif

protected: virtual ~__unknown() { }

public: HRESULT QueryInterface(IUnknown * punk, REFIID riid, void ** ppvObject);
    
public: ULONG AddRef();

public: ULONG Release();

public: ULONG Decrement();

public: ULONG Refs() { return PtrToUlong((VOID*)(_lRefs)); }

public: ULONG RefCount() { return PtrToUlong((VOID*)(_lRefs >> REF_SHIFT)); }

#ifdef RENTAL_MODEL
public: RentalEnum model();
#endif

        // use this only in the constructor of a derived class !
protected: void SetBits(ULONG ulBits)
    {
        Assert((ulBits >> REF_SHIFT) == 0);
        _lRefs = (_lRefs & (ULONG)(~(REF_REFERENCE | REF_MARKED | REF_RENTAL))) | ulBits;
    }

public: void ReleaseAndDelete()
    {
        ULONG ulRC = Release();
        Assert( ulRC == 0);
    }

public: void EmbeddedRelease()
    {
        Assert((_lRefs >> REF_SHIFT) == 1);
        _lRefs -= REF_OFFSET;
    }
};

//===========================================================================
// This template implements the IUnknown portion of a given COM interface.

template <class I, const IID * I_IID> class NOVTABLE _unknown : public I, public __unknown
{
public:        
        _unknown() : __unknown(I_IID) {
            TraceTag((tagRefCount, "IncrementComponents - __unknown")); 
            ::IncrementComponents();
        }

#ifdef RENTAL_MODEL
        _unknown(RentalEnum re) : __unknown(I_IID, re) {
            TraceTag((tagRefCount, "IncrementComponents - __unknown"));
            ::IncrementComponents();
        }
#endif

        ~_unknown() { 
            TraceTag((tagRefCount, "DecrementComponents - ~__unknown"));
            ::DecrementComponents();
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            return __unknown::QueryInterface((I *)this, riid, ppvObject);
        }
    
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return __unknown::AddRef();
        }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            return __unknown::Release();
        }
};

// This is an Unknown class that is used for INTERNAL objects only and therefore
// it does NOT Increment the Component count.
class SimpleIUnknown : public IUnknown, public __unknown
{
public:        
        SimpleIUnknown() : __unknown(&IID_IUnknown) { }

#ifdef RENTAL_MODEL
        SimpleIUnknown(RentalEnum re) : __unknown(&IID_IUnknown, re) { }
#endif

        ~SimpleIUnknown() { }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            return __unknown::QueryInterface((IUnknown *)this, riid, ppvObject);
        }
    
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return __unknown::AddRef();
        }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            return __unknown::Release();
        }
};


#endif _UNKNOWN_HXX



