
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       Mft.hxx (sample stackable filter)
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
#ifndef _CMFT_HXX_
#define _CMFT_HXX_

#include <urlmon.hxx>

//+---------------------------------------------------------------------------
//
//  Class:       CMft 
//
//  Purpose:     Mime filter 
//
//  Interface:   [support all IOInetProtocol     interfaces] 
//               [support all IOInetProtocolSink interfaces] 
//
//  History:     11-24-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
class CMft : public IOInetProtocol, 
             public IOInetProtocolSink,
             public IOInetProtocolSinkStackable 
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj)
    {
        HRESULT hr = E_NOINTERFACE;
        *ppvObj = NULL;

        if( (iid == IID_IUnknown) ||
            (iid == IID_IOInetProtocol) ||
            (iid == IID_IOInetProtocolRoot) )
        {
            *ppvObj = (IOInetProtocol*)this;
        }    
        else
        if( iid == IID_IOInetProtocolSink )
        {
            *ppvObj = (IOInetProtocolSink*)this;
        }
        else
        if( iid == IID_IOInetProtocolSinkStackable )
        {
            *ppvObj = (IOInetProtocolSinkStackable*)this;
        }
        
        if( *ppvObj )
        {
            hr = NOERROR;
            AddRef();
        }
        return hr;
    }

    STDMETHODIMP_(ULONG) AddRef(void)
    {
        LONG lRet = ++_CRefs;
        return lRet;
    }

    STDMETHODIMP_(ULONG) Release(void)
    {
        LONG lRet = --_CRefs;
        if( !lRet )
        {
            delete this;
        }
        return lRet;
    }

    // IOInetProtocol
    STDMETHODIMP Start(
        LPCWSTR szUrl,
        IOInetProtocolSink *pProtSink,
        IOInetBindInfo *pOIBindInfo,
        DWORD grfSTI,
        DWORD dwReserved
    )
    {
        PROTOCOLFILTERDATA* FiltData = (PROTOCOLFILTERDATA*) dwReserved;
        _pProt = FiltData->pProtocol;
        _pProtSink = pProtSink;

        _pProt->AddRef();
        _pProtSink->AddRef();
        
        return NOERROR;
    }


    STDMETHODIMP Continue( PROTOCOLDATA *pStateInfo)
    {
        return _pProt->Continue(pStateInfo);
    }

    STDMETHODIMP Abort( HRESULT hrReason, DWORD dwOptions)
    {
        return _pProt->Abort(hrReason, dwOptions);
    }

    STDMETHODIMP Terminate( DWORD dwOptions)
    {
        if( _pProtSink )
        {
            _pProtSink->Release();
            _pProtSink = NULL;
        } 

        return _pProt->Terminate(dwOptions);
    }

    STDMETHODIMP Suspend()
    {
        return _pProt->Suspend();
    }

    STDMETHODIMP Resume()
    {
        return _pProt->Resume();
    }

    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead)
    {
        return _pProt->Read(pv, cb, pcbRead);
    }

    STDMETHODIMP Seek( 
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition
    )
    {
        return _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);
    }

    STDMETHODIMP LockRequest(DWORD dwOptions)
    {
        return _pProt->LockRequest(dwOptions);
    }

    STDMETHODIMP UnlockRequest()
    {
        return _pProt->UnlockRequest();
    }

    STDMETHODIMP Switch( PROTOCOLDATA *pStateInfo)
    {
        return _pProtSink->Switch(pStateInfo);
    }

    STDMETHODIMP ReportProgress( ULONG ulStatusCode, LPCWSTR szStatusText)
    {
        return _pProtSink->ReportProgress(ulStatusCode, szStatusText);
    }

    STDMETHODIMP ReportData( 
        DWORD grfBSCF, 
        ULONG ulProgress, 
        ULONG ulProgressMax
    )
    {
        return _pProtSink->ReportData(grfBSCF, ulProgress, ulProgressMax);
    }

    STDMETHODIMP ReportResult(
        HRESULT hrResult,
        DWORD   dwError,
        LPCWSTR wzResult
    )
    {
        return _pProtSink->ReportResult(hrResult, dwError, wzResult);
    }

    STDMETHODIMP SwitchSink(IOInetProtocolSink* pSink)
    {
        HRESULT hr = NOERROR;
    
        // keep track the existing sink (support for Commit/Rollback)
        // the release of the old sink will be done at the Commit time
        _pProtSinkOld = _pProtSink;

        // -----------------------------------------------------------
        // BUG: remove this block once enable the Commit-Rollback func
        // release the old sink
        //
        if( _pProtSinkOld )
        {
            _pProtSinkOld->Release();
        }
        // -----------------------------------------------------------

        // Change the sink
        _pProtSink = pSink;
        _pProtSink->AddRef();

        return hr;
    }

    STDMETHODIMP CommitSwitch()
    {
        
        // release the old sink
        //if( _pProtSinkOld )
        //{
        //    _pProtSinkOld->Release();
        //}

        // reset
        //_pProtSinkOld = NULL;

        return NOERROR;
    }

    STDMETHODIMP RollbackSwitch()
    {

        // copy the old sink back, release the new sink
        // (new sink is AddRef'ed at SwitchSink time)
        //if( _pProtSink )
        //{
        //    _pProtSink->Release();
        //} 
        //_pProtSink = _pProtSinkOld;

        // reset
        //_pProtSinkOld = NULL;

        return NOERROR;
    }
        
    CMft()
        : _CRefs()
    {
        _pProtSink = NULL;
        _pProt = NULL;
        _pProtSinkOld = NULL;
    }
    virtual ~CMft()
    {
        if( _pProt )
        {
            _pProt->Release();
        }
    }
private:
    CRefCount            _CRefs;            
    IOInetProtocol*      _pProt;
    IOInetProtocolSink*  _pProtSink;
    IOInetProtocolSink*  _pProtSinkOld; 
};




#endif
