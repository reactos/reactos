/*
 * @(#)IPersistMoniker.hxx 1.0 11/21/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CORE_IO_IPERSISTMONIKER
#define _CORE_IO_IPERSISTMONIKER

#ifndef __ocidl_h__
#include "ocidl.h"
#endif

class NOVTABLE PersistMoniker : public Object
{
  public: virtual CLSID getClassID() = 0;

  public: virtual void load(bool fFullyAvailable, IMoniker *pmk, LPBC pbc, DWORD grfMode) = 0;
        
  public: virtual void getCurMoniker(IMoniker** ppimkName) = 0;

  public: virtual HRESULT GetLastError() = 0;
};

class IPersistMonikerWrapper : public _comexport<PersistMoniker, IPersistMoniker, &IID_IPersistMoniker>
{
  public: IPersistMonikerWrapper(PersistMoniker * p, Mutex * pMutex)
                : _comexport<PersistMoniker, IPersistMoniker, &IID_IPersistMoniker>(p)
            {
                _pMutex = pMutex;
            }

        virtual HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [out] */ CLSID __RPC_FAR *pClassID);
        
        virtual HRESULT STDMETHODCALLTYPE IsDirty( void);
        
        virtual HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ BOOL fFullyAvailable,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc,
            /* [in] */ DWORD grfMode);
        
        virtual HRESULT STDMETHODCALLTYPE Save( 
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pbc,
            /* [in] */ BOOL fRemember);
        
        virtual HRESULT STDMETHODCALLTYPE SaveCompleted( 
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc);
        
        virtual HRESULT STDMETHODCALLTYPE GetCurMoniker( 
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName);

  private:

      RMutex _pMutex;
};


#endif _CORE_IO_IPERSISTMONIKER
