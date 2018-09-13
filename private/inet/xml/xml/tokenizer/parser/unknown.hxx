/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _UNKNOWN_HXX
#define _UNKNOWN_HXX

#define checkhr2(a) hr = (a); if (hr != S_OK) return hr;

//===========================================================================
// This template implements the IUnknown portion of a given COM interface.

template <class I, IID* I_IID> class _unknown : public I
{
private:    long _refcount;

public:        
        _unknown<I>() 
        { 
            _refcount = 0;
        }

        virtual ~_unknown()
        {
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            if (riid == IID_IUnknown)
            {
                *ppvObject = static_cast<IUnknown*>(this);
            }
            else if (riid == *I_IID)
            {
                *ppvObject = static_cast<I*>(this);
            }
            reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
            return S_OK;
        }
    
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return InterlockedIncrement(&_refcount);
        }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            if (InterlockedDecrement(&_refcount) == 0)
            {
                delete this;
                return 0;
            }
            return _refcount;
        }
};    

#endif _UNKNOWN_HXX