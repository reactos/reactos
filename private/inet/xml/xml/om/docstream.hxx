/*
 * @(#)docstream.hxx 1.0 12/08/1998
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _XML_OM_DOCSTREAM
#define _XML_OM_DOCSTREAM

#include "document.hxx"

typedef _reference<IXMLParser> RXMLParser;
typedef _reference<IResponse> RResponse;

class DocStream : public _unknown<IStream, &IID_IStream>
{
private:
    enum EState { NONE, READING, WRITING } _dwState;
    RDocument   _pDoc;
    RXMLParser  _pParser;
    DWORD       _cbWritten;
    BYTE*       _pBytes;
    DWORD       _cbRead;
    RResponse   _pResponse;
    SAFEARRAY*  _psa;
    DWORD       _dwBuffered;

public:

    DocStream(Document* pDoc);  // wrap the document and read/write to document directly
    DocStream(IResponse* pResponse); // wrap the Response object and write to it.
    DocStream(IRequest* pRequest); // wrap the Request object and read from it.
    DocStream(SAFEARRAY* psa); // load document from given safearray.
    ~DocStream();

#ifdef _DEBUG
    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    {
        return _unknown<IStream, &IID_IStream>::AddRef();
    }
    
    virtual ULONG STDMETHODCALLTYPE Release( void)
    {
        return _unknown<IStream, &IID_IStream>::Release();
    }
#endif
    
    //
    // ISequentialStream
    //

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead);
        
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten);

    //
    // IStream
    //

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);
        
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize);
        
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten);
        
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags);
        
    virtual HRESULT STDMETHODCALLTYPE Revert(void);
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType);
        
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType);
        
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag);
        
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

private:
    HRESULT WriteSafeArray(const void *pv, ULONG cb);
    HRESULT FlushSafeArray();
    HRESULT SaveDocument();
};

#endif // _XML_OM_DOCSTREAM
