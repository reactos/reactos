/*
 * @(#)IPersistStream.hxx 1.0 11/17/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CORE_IO_IPERSISTSTREAM
#define _CORE_IO_IPERSISTSTREAM

#ifndef __ocidl_h__
#include "ocidl.h"
#endif

class NOVTABLE PersistStream : public Object
{
    public: virtual CLSID getClassID() = 0;

    public: virtual void Load(IStream*) = 0;

    public: virtual void Save(IStream*) = 0; // avoid clash with Element::save.

    public: virtual HRESULT GetLastError() = 0;
};

class IPersistStreamWrapper : public _comexport<PersistStream, IPersistStreamInit, &IID_IPersistStreamInit>
{
  public: IPersistStreamWrapper(PersistStream * p, Mutex * pMutex)
                : _comexport<PersistStream, IPersistStreamInit, &IID_IPersistStreamInit>(p)
            {
                _pMutex = pMutex;
            }

        virtual HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [out] */ CLSID *pClassID);
        
        virtual HRESULT STDMETHODCALLTYPE Load( 
            /* [unique][in] */ IStream *pStm);
        
        virtual HRESULT STDMETHODCALLTYPE Save( 
            /* [unique][in] */ IStream *pStm,
            /* [in] */ BOOL fClearDirty);
        
        virtual HRESULT STDMETHODCALLTYPE IsDirty( void);

        virtual HRESULT STDMETHODCALLTYPE GetSizeMax( 
            /* [out] */ ULARGE_INTEGER *pcbSize);
            
        virtual HRESULT STDMETHODCALLTYPE InitNew( void);

  private:
        RMutex _pMutex;
};


#endif _CORE_IO_IPERSISTSTREAM
