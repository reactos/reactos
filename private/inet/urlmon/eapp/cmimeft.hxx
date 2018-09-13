
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CMimeFt.hxx
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
#ifndef _CMIMEFT_HXX_
#define _CMIMEFT_HXX_

#include <urlmon.hxx>

#define FT_IBUF_SIZE 8192+2	        // input buffer size default to 8K
#define FT_OBUF_SIZE 6*8192         // output buffer size default to 6*IN


//+---------------------------------------------------------------------------
//
//  Class:       CMimeFt 
//
//  Purpose:     Decompressor MIME filter 
//
//  Interface:   [support all IOInetProtocol     interfaces] 
//               [support all IOInetProtocolSink interfaces] 
//
//  History:     04-16-97   DanpoZ (Danpo Zhang)  Created
//               11-24-97   DanpoZ (Danpo Zhang)  Added Stackable interface 
//
//----------------------------------------------------------------------------
class CMimeFt : public IOInetProtocol, 
                public IOInetProtocolSink, 
                public IOInetProtocolSinkStackable 
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IOInetProtocol
    STDMETHODIMP Start(
        LPCWSTR szUrl,
        IOInetProtocolSink *pProtSink,
        IOInetBindInfo *pOIBindInfo,
        DWORD grfSTI,
        DWORD_PTR dwReserved
    );
    STDMETHODIMP Continue( PROTOCOLDATA *pStateInfo);
    STDMETHODIMP Abort( HRESULT hrReason, DWORD dwOptions);
    STDMETHODIMP Terminate( DWORD dwOptions);
    STDMETHODIMP Suspend();
    STDMETHODIMP Resume();

    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Seek( 
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition
    );
    STDMETHODIMP LockRequest(DWORD dwOptions);
    STDMETHODIMP UnlockRequest();

    // IOInetProtocolSink
    STDMETHODIMP Switch( PROTOCOLDATA *pStateInfo);
    STDMETHODIMP ReportProgress( ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHODIMP ReportData( 
        DWORD grfBSCF, 
        ULONG ulProgress, 
        ULONG ulProgressMax
    );

    STDMETHODIMP ReportResult(
        HRESULT hrResult,
        DWORD   dwError,
        LPCWSTR wzResult
    );

    // IOInetProtocolSinkStackable 
    STDMETHODIMP SwitchSink(IOInetProtocolSink* pSink);
    STDMETHODIMP CommitSwitch();
    STDMETHODIMP RollbackSwitch();

    // static create method
    static HRESULT Create(
        CMimeFt** ppMft
    ); 
    virtual ~CMimeFt();
private:
    // CMimeFt
    CMimeFt();
    HRESULT CreateBuffer();
    HRESULT SmartRead(void *pv, ULONG cb, ULONG *pcbRead, BOOL fSniff);
    
private:
    CRefCount           _CRefs;             // ref count 
    ULONG               _ulCurSizeFmtIn;    // processed incoming data 
    ULONG               _ulCurSizeFmtOut;   // generated outgoing data 
    ULONG               _ulTotalSizeFmtIn;  // total expected incoming data
    ULONG               _ulTotalSizeFmtOut; // total expected outgoing data  
    ULONG               _ulOutAvailable;    // available for outgoing
    ULONG               _ulInBufferLeft;    // left data in the compress buf
    ULONG               _ulContentLength;   // today bytes uncompressed

    BYTE*               _pOutBuf;           // holds incoming data           
    BYTE*               _pInBuf;            // holds outgoing data
  
    IOInetProtocol*         _pProt;             // incoming
    IOInetProtocolSink*     _pProtSink;         // outgoing
    IDataFilter*            _pDF;               // filter
    IOInetProtocolSink*     _pProtSinkOld;      // for Commit-Rollback  
   
    DWORD               _grfBindF;              // binding flags
    DWORD               _grfBSCF;               // status
    BOOL                _bEncoding;             // encoding or decoding?
    BOOL                _bMimeReported;         // mime type reported?
    BOOL                _bMimeVerified;         // mime type verified?
    CHAR                _szFileName[MAX_PATH];  // cache file name
    CHAR                _szURL[MAX_PATH];       // url name
    LPWSTR              _pwzMimeSuggested;      // server content type
    HANDLE              _hFile;                 // File Handle

    BOOL                _fDelayReport;          // delay report result?
    BOOL                _fSniffed;              // data sniff done?
    BOOL                _fSniffInProgress;      // sniff in progress?
    HRESULT             _hrResult;              // hr (delay report result)
    DWORD               _dwError;               // Err(delay report result)
    LPCWSTR             _wzResult;              // stt(delay report result)
};

#endif
