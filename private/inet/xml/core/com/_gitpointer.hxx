/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _gitpointer_hxx
#define _gitpointer_hxx


// BUGBUG (sambent) Unix does not support the Global Interface Table, so
// we have to fake it as best we can using CoMarshalInterface et al. directly.
// This is not great - there are bugs and limitations in CoMarshal that are
// fixed in GIT.  So get rid of this define (and keep the GIT implementation)
// as soon as GIT is ported to Unix.

#undef USE_COMARSHAL_FOR_GIT
#ifdef UNIX
#  define USE_COMARSHAL_FOR_GIT
#else
// nothing - use the standard GIT implementation
#endif


// --------------------------------------------------------------------------
// This template is useful for holding onto single threaded COM objects
// and then being able to call methods from different threads safely.
// --------------------------------------------------------------------------

class NOVTABLE __gitpointer  // base class containing code shared by the templates.
{
public:  
                            __gitpointer(const IID * riid);
    void                    reset();

protected: 
    virtual                 ~__gitpointer();

protected:
    void                    _assign(IUnknown* ptr);
    HRESULT                 _setPointer(IUnknown* ptr);
    IUnknown*               _getPointer();

protected:
    const IID *             _riid;
    IUnknown*               _ptr;
    DWORD                   _dwMainThread;
#ifdef USE_COMARSHAL_FOR_GIT
    HGLOBAL                 _hGlobal;
    unsigned                _fAddRefOnMainThread:1;
#else
    DWORD                   _dwCookie;  
    IGlobalInterfaceTable * _pGIT;
#endif
};


template <class I, const IID* I_IID> class _gitpointer : public __gitpointer
{
public:
    _gitpointer() : __gitpointer(I_IID)
    {
    }

    ~_gitpointer()
    {
    }   

    HRESULT setPointer(I* ptr)
    {
        return _setPointer((IUnknown*)ptr);    
    }

    I* operator->()
    { 
        return (I*)_getPointer();
    }

    operator I * () 
    { 
        return (I*)_getPointer(); 
    }    

    int operator==(void* p)
    {
        return (_ptr == p);
    }

    int operator!=(void* p)
    {
        return (_ptr != p);
    }

    _gitpointer & operator=(I* ptr) 
    {
        _assign(ptr); return *this;
    }
};

#endif
